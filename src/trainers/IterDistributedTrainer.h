#ifndef ITER_DIST_TRAINER_H_
#define ITER_DIST_TRAINER_H_
#include "TrainerBase.h"
namespace HieraParser
{

class IterDistributedTrainer : public TrainerBase
{
  public:
    IterDistributedTrainer(const Config &cfg) : TrainerBase(cfg) {
      pool = new ThreadPool(threads);
      if (threads > 1)
        std::cerr << "Loading [" << threads << "] threads ..." << std::endl;
    };
    virtual ~IterDistributedTrainer(){};
    void Train(std::vector<TrainingExample> &examples, Model &model) const;
};
}

#endif // ITER_DIST_TRAINER_H_