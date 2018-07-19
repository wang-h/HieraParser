#ifndef BATCH_TRAINER_H_
#define BATCH_TRAINER_H_
#include <iostream>
#include <sstream>
#include <queue>
#include "TrainerBase.h"
#include "../utils/ThreadPool.h"
namespace HieraParser
{


class BatchTrainer : public TrainerBase {
public:
  BatchTrainer(const Config &cfg): TrainerBase(cfg){
    pool = new ThreadPool(threads);
    batchSize = cfg.GetInt("batch");
    batch_norm = cfg.GetBool("batch_norm");
    if (threads > 1)
      std::cerr << "Loading [" << threads << "] threads ..." << std::endl;
  };
  virtual ~BatchTrainer(){};
  std::vector<int> TrainOneEpoch(int total_updates, int &num_updates,
                                std::vector<TrainingExample> &examples,
                                Model &model) const;
protected:
  int batchSize;
  bool batch_norm;
};
}

#endif // BATCH_TRAINER_H_