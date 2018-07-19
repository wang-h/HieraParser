#include "DistributedTrainer.h"
#include "../utils/ThreadPool.h"
#include "../utils/GetTime.h"
namespace HieraParser
{

void DistributedTrainer::Train(std::vector<TrainingExample> &examples, Model &model)  const
{

    std::vector<Model> submodels(threads);
    std::vector<std::vector<TrainingExample>> shards(threads);
    
    int exampleSize = static_cast<int>(examples.size());
    int shardSize = std::min(threads, exampleSize);
    
    std::vector<int> total_updates(threads);
    ShardTrainExamples(examples, shards, shardSize);

    for (size_t i = 0; i < threads; i++)
        total_updates[i] = static_cast<int>(shards[i].size()) * iterations;
    std::vector<std::future<void>> results;
    for (int i = 0; i < threads; i++)
    {
        results.emplace_back(
            pool->enqueue(&TrainerBase::Train_, *this, std::ref(shards[i]), std::ref(submodels[i])));
    }
    for (auto &&result : results)
    {
        result.get();
    }
    model.ParaMix(submodels, total_updates); //cached weights
}

} // namespace HieraParser