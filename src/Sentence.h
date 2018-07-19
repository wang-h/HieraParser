#ifndef SENTENCE_H__
#define SENTENCE_H__
#include <vector>
#include <string>
#include "utils/farmhash.h"
using namespace NAMESPACE_FOR_HASH_FUNCTIONS;
namespace HieraParser
{
// Source sentence class.

typedef std::vector<uint64_t> Token;
typedef std::vector<Token> SentenceTemplate;

inline uint64_t FeatureFingerprint(const std::string &s) {
  return Hash64(s.data(), s.size());
}

class Sentence : public SentenceTemplate
{
  // friend std::ostream &operator<<(std::ostream &, const Sentence &);
  static int sid;
public:
  Sentence(){};
  ~Sentence(){};
  static Sentence *CreateFromString(const std::string &str, const size_t kMaxFactorSize);
  std::string GetString() const;
  
};

} // namespace HieraParser
#endif // SENTENCE_H__