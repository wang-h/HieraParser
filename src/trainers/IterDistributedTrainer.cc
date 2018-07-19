#include "IterDistributedTrainer.h"
#include "../utils/ThreadPool.h"
#include "../utils/GetTime.h"
#include "../utils/ProgressBar.h"
#include <numeric>
namespace HieraParser
{

void IterDistributedTrainer::Train(std::vector<TrainingExample> &examples, Model &model) const
{
    std::vector<Model> submodels(threads);
    std::vector<std::vector<TrainingExample>> shards(threads);
    int exampleSize = static_cast<int>(examples.size());
    
    ShardTrainExamples(examples, shards, std::min(threads, exampleSize));
    std::vector<std::future<std::vector<int>>> results;
    std::vector<int> shardSize(threads, 0);
    std::vector<int> num_updates(threads, 0);
    std::vector<int> total_updates(threads, 0);
    for (size_t i = 0; i < threads; i++){
        total_updates[i] =  iterations * static_cast<int>(shards[i].size());
        shardSize[i] =  static_cast<int>(shards[i].size());
    }
        


    double wall0 = get_wall_time();
    for (int iter = 0; iter < iterations; iter++)
    {
        int num_errors = 0;
        int num_unreachables = 0;
        std::stringstream log_string;
        for (size_t i = 0; i < threads; i++)
        {
            results.emplace_back(
                pool->enqueue(&TrainerBase::TrainOneEpoch, *this, total_updates[i],
                             std::ref(num_updates[i]),
                             std::ref(shards[i]), std::ref(submodels[i])));
        }
        for (auto &&result : results)
        {
            std::vector<int> nums = result.get();
            num_errors += nums[0];
            num_unreachables += nums[1];
        };
       
        double wall1 = get_wall_time();
        log_string << "Iteration=" << iter << ", Errors=" << num_errors
                   << ", Unreachables=" << num_unreachables
                   << ", Seconds=" <<  wall1 - wall0  << "                     ";
        std::cerr << log_string.str() << std::endl;

        model.IterParaMix(submodels, shardSize, (float)(iterations-iter)/iterations);
        
        if (early_stop && num_errors == 0)
            break;
        if (saveStep > 0 && iter % saveStep == 0){ 
            model.WriteModel(m_path + "." + std::to_string(iter));
        }
        results.clear();
        
    }
}
} // namespace HieraParser