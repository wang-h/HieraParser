#ifndef DISTRIBUTED_TRAINER_H_
#define DISTRIBUTED_TRAINER_H_
#include "TrainerBase.h"
#include <vector>
// #include "../utils/ThreadPool.h"

namespace HieraParser
{


class DistributedTrainer : public TrainerBase {
public:
  DistributedTrainer(const Config &cfg): TrainerBase(cfg){
    pool = new ThreadPool(threads);
    if (threads > 1)
      std::cerr << "Loading [" << threads << "] threads ..." << std::endl;
  };
  virtual ~DistributedTrainer(){};

  void Train(std::vector<TrainingExample> &examples, Model &model) const;
};

} // namespace HieraParser
#endif //DISTRIBUTED_TRAINER_H_
