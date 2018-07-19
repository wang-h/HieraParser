#ifndef TRAINER_H_
#define TRAINER_H_
#include "../BaseModel.h"
#include "../Config.h"
#include "../Constraint.h"
#include "../Parser.h"
#include "../Sentence.h"
#include "../utils/AssertDef.h"
#include "../utils/ProgressBar.h"
#include "../utils/ThreadPool.h"
#include "../utils/TypeDef.h"
#include <numeric>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <string>
#include <ctime>        // std::time
#include <cstdlib>      // std::rand, std::srand
namespace HieraParser
{


typedef std::pair<Sentence *, Constraint *> TrainingExample;

struct RNG {
    int operator() (int n) {
        return static_cast<int>(std::rand()/(static_cast<double>(RAND_MAX)+1) * n);
    }
};
static void LoadConstraint(std::vector<TrainingExample> &training_examples, std::vector<Sentence *> &sentences,
                           const std::string &path)
{
  std::ifstream input_in(path);
  if (!input_in.good())
    THROW_ERROR("File not exist!");
  size_t i = 0;
  auto it = sentences.begin();
  std::string line;
  for (; it != sentences.end() && !getline(input_in, line).eof(); ++it, ++i)
  {
    std::cerr << std::flush << i << '\r' ;
    Constraint *constraint = Constraint::CreateFromString(line);
    if (constraint != nullptr)
    {
      training_examples.emplace_back(*it, constraint);
    }
  }
  std::cerr << std::flush ;
  std::cerr << "# Number of training examples: " << training_examples.size()
            << std::endl;
}


static void show_bar(int tid, int step, int total)
{
  std::stringstream log_string;
  if (tid > 0)
  {
    std::cerr << std::flush << "[Thread " <<std::to_string(tid)<< "]"<< ProgressBar(step, total, " ") << "\r";
  }
  else{
    std::cerr << std::flush << ProgressBar(step, total, " ") << "\r";
  }
};
static std::mutex mtx;
class TrainerBase
{
public:
  TrainerBase(const Config &cfg);
  virtual ~TrainerBase(){};

  void CollectDiffFeatures(
      float &loss,
      const Sentence &sentence,
      const std::vector<ParserAction> &actions_ref,
      const std::vector<ParserAction> &actions_sys, const Model &model,
      FeaturesDiff &featuresDiff) const; // return loss per-sentence
  virtual void Train(std::vector<TrainingExample> &examples, Model &model) const{
    this->Train_(examples, model);
  };
  virtual void Train_(std::vector<TrainingExample> &examples, Model &model) const; //thread safe
  virtual std::vector<int> TrainOneEpoch(int total_updates, int &num_updates,
                                         std::vector<TrainingExample> &examples,
                                         Model &model) const;

  virtual std::vector<float> TrainOneSentence(const Sentence *sentence, const Constraint *constraint,
                                      const Model &model, FeaturesDiff &featuresDiff) const;

  void ShardTrainExamples(const std::vector<TrainingExample> &examples,
                          std::vector<std::vector<TrainingExample>> &shards,
                          const int &shardSize) const;

  

  void UpdateWeights(
      float loss,
      const FeaturesDiff &featuresDiff,
      float coefficient, Model &model) const;

protected:
  ThreadPool *pool;
  Parser *parser;
  std::string m_path;
  int threads;
  bool early_stop;
  int beamSize;
  int iterations;
  int saveStep;
  int trainer_type;
  int kbest;
  bool shuffle;
};

} // namespace HieraParser

#endif // TRAINER_H_