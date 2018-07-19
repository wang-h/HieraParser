#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>
#include "BatchTrainer.h"

#include "../utils/GetTime.h"
namespace HieraParser {

std::vector<int>
BatchTrainer::TrainOneEpoch(int total_updates, int &num_updates,
                            std::vector<TrainingExample> &examples,
                            Model &model) const {
  int l = 0;
  std::srand(std::time(NULL));

  
  if (shuffle)
      std::random_shuffle(examples.begin(), examples.end(), RNG());

  
  auto p = examples.begin();
  std::vector<int> result(2, 0);
  FeaturesDiff featuresDiff(2);
  std::vector<std::future<std::vector<float>>> batch;
  int step = std::max(static_cast<int>(examples.size() / 70), 1);

  while (p < examples.end()) {
    if (l % step == 0 && l != 0) {
      show_bar(-1, l, examples.size());
    }
    if (static_cast<int>(batch.size()) < batchSize) {
      const Sentence *sentence = p->first;
      const Constraint *constraint = p->second;
      batch.emplace_back(pool->enqueue(&TrainerBase::TrainOneSentence, *this,
                                       sentence, constraint, std::ref(model),
                                       std::ref(featuresDiff)));
    }
    if (static_cast<int>(batch.size()) == batchSize ||
        l == examples.size() - 1) {
      float loss = 0.0;
      for (auto &&b : batch) {
        const std::vector<float> v = b.get();
        loss += v[0];
        result[0] += v[1];
        result[1] += v[2];
        num_updates ++;
      }
      if (batch_norm){
        loss = loss / static_cast<int>(batch.size());
      }
      batch.clear();
      const float coefficient =
          static_cast<float>(total_updates - num_updates) / total_updates;
      UpdateWeights(loss, featuresDiff, coefficient, model);
      for (int i = 0; i < 2; i++) {
        featuresDiff[i].clear();
      }
      loss = 0.0;
    }
    ++p;
    ++l;
  }
  return result;
}

} // namespace HieraParser