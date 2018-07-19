#include "TrainerBase.h"
#include "../Config.h"
#include "../Parser.h"
#include "../utils/GetTime.h"
#include "../utils/TypeDef.h"
#include "BatchTrainer.h"
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
namespace HieraParser {
TrainerBase::TrainerBase(const Config &cfg) {
  // init trainer
  kbest = cfg.GetInt("kbest");
  iterations = cfg.GetInt("iterations");
  saveStep = cfg.GetInt("save_step");
  threads = cfg.GetInt("threads");
  parser = new Parser(cfg);
  early_stop = cfg.GetBool("early_stop");
  m_path = cfg.GetString("model");
  trainer_type = cfg.GetInt("strategy");
  shuffle = cfg.GetBool("shuffle");
  //init_param = cfg.GetBool("init_param");
}

void TrainerBase::ShardTrainExamples(
    const std::vector<TrainingExample> &examples,
    std::vector<std::vector<TrainingExample>> &shards,
    const int &shardSize) const {
  int exampleSize = static_cast<int>(examples.size());
  std::vector<int> shardSizes(shardSize);
  int bunchSize = std::ceil((float)exampleSize / shardSize);
  shards.resize(shardSize);
  int pre_last = 0;
  std::stringstream shard_info;
  for (int i = 0; i < exampleSize; i += bunchSize) {
    auto last = std::min(exampleSize, i + bunchSize);
    auto index = i / bunchSize;
    shard_info << last - pre_last;
    if (i < exampleSize - 1)
      shard_info << ", ";
    shardSizes[index] = last - pre_last;
    pre_last = last;
    auto &vec = shards[index];
    vec.reserve(last - i);
    std::move(examples.begin() + i, examples.begin() + last,
              back_inserter(vec));
  }
  std::cerr << "Shards: [" << shard_info.str() << "]" << std::endl;
}

void TrainerBase::CollectDiffFeatures(
    float &loss, const Sentence &sentence,
    const std::vector<ParserAction> &actions_ref,
    const std::vector<ParserAction> &actions_sys, const Model &model,
    FeaturesDiff &featuresDiff) const {
  CHECK(actions_ref.size() == actions_sys.size(),
        "Inconsistent lengths of action sequences");
  ParserState state_ref(sentence.size());
  ParserState state_sys(sentence.size());
  int start = actions_ref.size();
  for (size_t i = 0; i < actions_ref.size(); i++) {
    if (actions_ref[i] != actions_sys[i]) {
      start = i;
      break;
    }
    state_ref.Advance(actions_ref[i], 0.0, false);
    state_sys.Advance(actions_sys[i], 0.0, false);
  }
  loss += sqrt(static_cast<float>(actions_ref.size() - start));
  for (size_t i = start; i < actions_ref.size(); i++) {
    const ParserSpan &span_ref = state_ref.stack.back();
    const ParserAction &action_ref = actions_ref[i];
    Counts &features_diff_ref =
        featuresDiff[static_cast<int>(action_ref.second)];
    std::vector<size_t> features;
    parser->ExtractFeatures(sentence, action_ref, span_ref, action_ref.first,
                            features);
    // const MiniHashMap<uint64_t, float, Hash> &weights_ref =
    //     model.weights[static_cast<int>(action_ref.second)];
    const Weights &weights_ref =
        model.weights[static_cast<int>(action_ref.second)];
    for (const size_t feature : features) {
      const auto it = weights_ref.find(feature);
      if (it != weights_ref.end()) {
        loss -= it->second;
      }
      std::lock_guard<std::mutex> lock(mtx);
      if (features_diff_ref.find(feature) == features_diff_ref.end())
        features_diff_ref[feature] = 1;
      else
        features_diff_ref[feature] += 1;
    }
    state_ref.Advance(action_ref, 0.0, false);

    const ParserSpan &span_sys = state_sys.stack.back();
    const ParserAction &action_sys = actions_sys[i];
    Counts &features_diff_sys =
        featuresDiff[static_cast<int>(action_sys.second)];
    // const MiniHashMap<uint64_t, float, Hash> &weights_sys =
    //     model.weights[static_cast<int>(action_sys.second)];
    const Weights &weights_sys =
        model.weights[static_cast<int>(action_sys.second)];
    parser->ExtractFeatures(sentence, action_sys, span_sys, action_sys.first,
                            features);
    for (const size_t feature : features) {
      const auto it = weights_sys.find(feature);
      if (it != weights_sys.end()) {
        loss += it->second;
      }
      std::lock_guard<std::mutex> lock(mtx);
      if (features_diff_sys.find(feature) == features_diff_sys.end())
        features_diff_sys[feature] = -1;
      else
        features_diff_sys[feature] -= 1;
    }
    state_sys.Advance(action_sys, 0.0, false);
  }
}

void TrainerBase::Train_(std::vector<TrainingExample> &examples,
                         Model &model) const {
  int num_updates = 0;
  int total_updates = iterations * examples.size();
  double wall0 = get_wall_time();
  for (int iter = 0; iter < iterations; ++iter) {
    std::stringstream log_string;

    std::vector<int> result =
        this->TrainOneEpoch(total_updates, num_updates, examples, model);
    int num_errors = result[0];
    int num_unreachables = result[1];
    double wall1 = get_wall_time();

    log_string << "Iteration=" << iter << ", Errors=" << num_errors
               << ", Unreachables=" << num_unreachables
               << ", Seconds=" << wall1 - wall0 << "                ";
    std::cerr << log_string.str() << std::endl;
    if (early_stop && num_errors == 0)
      break;
    if (trainer_type != 2) {
      if (saveStep > 0 && iter % saveStep == 0)
        model.WriteModel(m_path + "." + std::to_string(iter));
    }
  }
}

std::vector<int>
TrainerBase::TrainOneEpoch(int total_updates, int &num_updates,
                           std::vector<TrainingExample> &examples,
                           Model &model) const {
  int l = 0;
  std::vector<int> result(2, 0);
  std::srand(std::time(NULL));
  if (shuffle)
      std::random_shuffle(examples.begin(), examples.end(), RNG());
  auto example = examples.begin();
  int step = std::max(static_cast<int>(examples.size() / 70), 1);
  for (; example < examples.end(); ++example, l++) {
    // if (l % step == 0 && l != 0 && trainer_type != 2 )
    if (l % step == 0 && l != 0) {
      show_bar(-1, l, examples.size());
    }
    FeaturesDiff featuresDiff(2);
    const Sentence *sentence = example->first;
    const Constraint *constraint = example->second;
    const std::vector<float> array =
        this->TrainOneSentence(sentence, constraint, model, featuresDiff);
    float loss = array[0];
    result[0] += array[1];
    result[1] += array[2];
    num_updates++;
    const float coefficient =
        static_cast<float>(total_updates - num_updates) / total_updates;
    UpdateWeights(loss, featuresDiff, coefficient, model);
    for (int i = 0; i < 2; i++) {
      featuresDiff[i].clear();
    }
    
  }
  return result;
}

std::vector<float>
TrainerBase::TrainOneSentence(const Sentence *sentence,
                              const Constraint *constraint, const Model &model,
                              FeaturesDiff &featuresDiff) const {
  std::vector<float> ret(3, 0);
  std::vector<std::vector<ParserAction>> nbestActions;
  std::vector<ParserAction> actions_ref;
  parser->Parse(*sentence, constraint, nbestActions, actions_ref, model);
  const std::vector<ParserAction> &actions_sys = nbestActions.front();
  // if (actions_ref.empty()) {
  //   ret[2]++;
  // } else if (actions_sys.size() != sentence->size() - 1 ||
  //            actions_sys != actions_ref) {
  //   ret[1]++;
  //   this->
  // }

  if (actions_ref.empty())
  {
    ret[2]++;
  }
  else if(actions_sys == actions_ref)
  {
      return ret;
  }
  else 
  {
      ret[1]++;
      for(size_t i=0; i < std::min(nbestActions.size(), static_cast<size_t>(kbest)); i++)
      {
          const std::vector<ParserAction> &actions_sys = nbestActions[i];
          if (actions_sys.size() != sentence->size() - 1 || actions_sys != actions_ref)
              CollectDiffFeatures(ret[0], *sentence, actions_ref, actions_sys, model,
                      featuresDiff);
          
      } 
  }
  return ret;
}

void TrainerBase::UpdateWeights(float loss, const FeaturesDiff &featuresDiff,
                                float coefficient, Model &model) const {
  // Calculate tau.
  float sq_norm = 0;
  for (int i = 0; i < 2; i++) {
    for (auto it = featuresDiff[i].begin(); it != featuresDiff[i].end(); ++it) {
      if (it->second != 0.0) {
        sq_norm += it->second * it->second;
      }
    }
  }
  float tau = std::min(static_cast<float>(1.0), loss / sq_norm);
  for (int i = 0; i < 2; i++) {
    for (auto it = featuresDiff[i].begin(); it != featuresDiff[i].end(); ++it) {
      if (it->second != 0.0) {
        model.weights[i][it->first] += tau * it->second;
        model.cached_weights[i][it->first] += tau * it->second * coefficient;
      }
    }
  }
}
} // namespace HieraParser