#include "BaseModel.h"
#include "Config.h"
#include <iostream>

namespace HieraParser
{

Model::Model(const Config &cfg)
    // : weights(2, MiniHashMap<uint64_t, float, Hash>(HUGE_VALF)),
    //   cached_weights(2, MiniHashMap<uint64_t, float, Hash>(HUGE_VALF)),
    : weights(2), cached_weights(2), m_path(cfg.GetString("model"))
{
}

Model::Model() : weights(2), cached_weights(2) {}

Model::Model(const Model &copy) : weights(copy.weights), cached_weights(copy.cached_weights)
{
}

void Model::ReadModel(const std::string &path)
{
    std::cerr << "Loading model from " << path << std::endl;
    std::ifstream file(path);
    int num = 0;
    for (int i = 0; i < 2; i++)
    {
        size_t size;
        CHECK(file.read(reinterpret_cast<char *>(&size), sizeof(size)),
              "Error, model file is empty.");
        auto &mapping = weights[i];
        mapping.clear();
        for (size_t j = 0; j < size; j++)
        {
            size_t feature;
            CHECK(
                file.read(reinterpret_cast<char *>(&feature), sizeof(feature)).good(),
                "Error, model fromat is incorrect.");
            float weight;
            CHECK(file.read(reinterpret_cast<char *>(&weight), sizeof(weight)).good(),
                  "Cannot read the model data");
            mapping[feature] = weight;
            num++;
        }
    }
    std::cerr << num << " feature weights were read." << std::endl;
    file.close();
}

void Model::WriteModel(const std::string &path, bool finish) const
{

    std::ofstream file(path);
    for (int i = 0; i < 2; i++)
    {
        const auto &mapping = cached_weights[i];
        const size_t size = mapping.size();
        CHECK(
            file.write(reinterpret_cast<const char *>(&size), sizeof(size)).good(),
            "Cannot write the model data");
        for (auto it = mapping.begin(); it != mapping.end(); ++it)
        {
            const size_t feature = it->first;
            CHECK(
                file.write(reinterpret_cast<const char *>(&feature), sizeof(feature))
                    .good(),
                "Cannot write the model data");
            const float weight = it->second;
            CHECK(file.write(reinterpret_cast<const char *>(&weight), sizeof(weight))
                      .good(),
                  "Cannot write the model data");
        }
    }
    file.close();
}
void Model::Clear()
{
    for (int i = 0; i < 2; i++)
    {
        weights[i].clear();
        cached_weights[i].clear();
    }
}

void Model::ParaMix(std::vector<Model> &submodels, const std::vector<int> &total_updates)
{
    int normalization = 0;
    for (auto &n : total_updates)
        normalization += n;
    Clear();
    for (size_t j = 0; j < submodels.size(); j++)
    {
        float rate = float(total_updates[j]) / normalization;
        for (size_t i = 0; i < 2; i++)
        {
            const Weights &mapping = submodels[j].cached_weights[i];
            for (auto it = mapping.begin(); it != mapping.end(); ++it)
            {
                cached_weights[i][it->first] += (it->second) * rate;
            }
        }
    }
}

void Model::IterParaMix(std::vector<Model> &submodels, const std::vector<int> &shardSize, float momentum)
{
    int normalization = 0;
    for (auto &n : shardSize)
        normalization += n;
    for (int i = 0; i < 2; i++)
    {
        weights[i].clear();
    }
    for (size_t j = 0; j < submodels.size(); j++)
    {
        float rate = float(shardSize[j]) / normalization;
        for (int i = 0; i < 2; i++)
        {
            Weights& sub_cached_weights = submodels[j].cached_weights[i];
            
            for (auto it = sub_cached_weights.begin(); it != sub_cached_weights.end(); ++it)
                cached_weights[i][it->first] +=  (it->second) * rate;
            
            sub_cached_weights.clear();
            Weights &sub_weights = submodels[j].weights[i];
            for (auto it = sub_weights.begin(); it != sub_weights.end(); ++it)
                weights[i][it->first] += (it->second) * rate;

            sub_weights.clear();
            
        }
    }
    for (size_t j = 0; j < submodels.size(); j++)
    {
        for (int i = 0; i < 2; i++)
        {
            submodels[j].weights[i] = Weights(weights[i]);
        }
    }
    // for (int i = 0; i < 2; i++)
    // {
    //     cached_weights[i] = Weights(weights[i]);
    // }
}

// void Model::Update(const FeaturesDiff &featuresDiff, const float& tau, const float& coefficient)
// {
//   for (int i = 0; i < 2; i++) {
//     for (auto it = featuresDiff[i].begin(); it != featuresDiff[i].end(); ++it) {
//       if (it->second != 0.0) {
//         weights[i][it->first] += tau * it->second;
//         cached_weights[i][it->first] += tau * it->second * coefficient;
//       }
//     }
//   }
// }
} // namespace HieraParser