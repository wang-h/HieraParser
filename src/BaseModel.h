#ifndef MODEL_H_
#define MODEL_H_
#include "Config.h"
#include "utils/AssertDef.h"
#include "utils/ThreadPool.h"
#include "utils/TypeDef.h"

namespace HieraParser {

struct DummyHash {
  size_t operator()(const uint64_t &v) const { return v; }
};

struct DummyKeyEqual {
  bool operator()(uint64_t lhs, uint64_t rhs) const {
    return lhs == rhs;
  }
};
// typedef std::unordered_map<uint64_t, float> FeatureWeightMapping;
// typedef std::vector<FeatureWeightMapping> Weights;
typedef std::unordered_map<uint64_t, float, DummyHash> Weights;
typedef std::unordered_map<uint64_t, int, DummyHash, DummyKeyEqual> Counts;
// typedef std::vector<MiniHashMap<uint64_t, float, DummyHash>> Weights;
typedef std::vector<Counts> FeaturesDiff;

class Model {

public:
  Model();
  Model(const Config &cfg);
  Model(const Model &copy);
  virtual ~Model(){};

  void WriteModel(const std::string &path, bool cached=true) const;
  void ReadModel(const std::string &path);
  void Save() const { WriteModel(m_path); };
  void Clear();
  // void Ensemble(std::vector<Model> &submodels);
  void ParaMix(std::vector<Model> &submodels,  const std::vector<int>& shardSize);
  void IterParaMix(std::vector<Model> &submodels,   const std::vector<int>& total_updates, float momentum);
  // void Update(const FeaturesDiff &featuresDiff, const float& tau, const
  // float& coefficient);
  // Weights of features used in training.
  // // The size of this vector is 2 (STR and INV).
  // std::vector<MiniHashMap<uint64_t, float, Hash>> weights;
  // std::vector<MiniHashMap<uint64_t, float, Hash>> cached_weights;
  std::vector<Weights> weights;
  std::vector<Weights> cached_weights;

private:
  std::string m_path;
};
} // namespace HieraParser
#endif // MODEL_H_
