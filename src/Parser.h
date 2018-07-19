#ifndef HIERAPARSER_H__
#define HIERAPARSER_H__
// #include <atomic>
// #include <cmath>
// #include <iostream>
// #include <iterator>
// #include <limits>
// #include <numeric>
// #include <queue>
// #include <sstream>
#include <string>
// #include <thread>

// #include <mutex>
// #include "util/Hash.h"

// #include "ParserState.h"
// #include "util/ThreadPool.h"
// #include "FeatureSet.h"
#include "BaseModel.h"
#include "Config.h"
#include "Constraint.h"
#include "Sentence.h"

namespace HieraParser
{

typedef std::pair<int, bool> ParserAction;


static void LoadInput(std::vector<Sentence*> &sentences, const std::string &path, const size_t kMaxFactorSize)
{
    std::cerr << "Loading input file " << path << std::endl;
    std::ifstream input_in(path);
    if (!input_in.good())
        THROW_ERROR("File not exist!");
    for (std::string line; !getline(input_in, line).eof();)
    {
        sentences.push_back(
            Sentence::CreateFromString(line, kMaxFactorSize));
    }
}

// ***************************************************************************
// ParserSpan Class
class ParserSpan
{
public:
  ParserSpan(int b, int e, int id) : bgn(b), end(e), action_id(id){};
  ParserSpan(){};
  bool operator==(const ParserSpan &other) const
  {
    return (other.bgn == bgn && other.end == end &&
            other.action_id == action_id);
  }
  int bgn;       // Beginning position of the span.
  int end;       // Ending position of the span.
  int action_id; // Id of the action which made this
};

// ***************************************************************************
// ParserState Class
class ParserState
{
public:
  // Make an initial state for the sentences with <len> tokens.
  // explicit ParserState(int len);
  ParserState(int len) : score(0.0), valid(true)
  {
    if (len >= 2)
    {
      stack.emplace_back(0, len, -1);
    }
  };

  // Make a new state by applying <action> to the old state.
  ParserState(const ParserState &state, const ParserAction &action,
              float new_score, bool new_valid)
      : stack(state.stack), actions(state.actions)
  {
    Advance(action, new_score, new_valid);
  }

  virtual ~ParserState(){};

  // Change the state by applying the specified action.
  void Advance(const ParserAction &action, float new_score, bool new_valid)
  {
    score = new_score;
    valid = new_valid;
    const ParserSpan span = stack.back();
    stack.pop_back();
    if (action.first - span.bgn >= 2)
    {
      stack.emplace_back(span.bgn, action.first, actions.size());
    }
    if (span.end - action.first >= 2)
    {
      stack.emplace_back(action.first, span.end, actions.size());
    }
    actions.push_back(action);
  };

  bool operator<(const ParserState &rhs) const { return (score > rhs.score); }
  float score;                       // Accumulated score.
  bool valid;                        // Whether the tree constructed so far satisfies constraints.
  std::vector<ParserSpan> stack;     // Stack of open spans.
  std::vector<ParserAction> actions; // History of actions.
};
// ***************************************************************************

typedef std::priority_queue<ParserState> Agenda;

struct BTGTree
{
  int bgn;
  int end;
  int pivot;
  bool label;

  BTGTree(int bgn_, int end_, int pivot_, bool label_)
      : bgn(bgn_), end(end_), pivot(pivot_), label(label_) {}

  bool operator==(const struct BTGTree &x) const
  {
    return (x.bgn == bgn && x.end == end && x.pivot == pivot &&
            x.label == label);
  }
};

// Parser Class
// ***************************************************************************
class Parser
{
public:
  Parser(const Config &cfg);
  // Parser(const Parser& rhs):kMaxFactorSize(rhs.kMaxFactorSize), beamSize(rhs.beamSize){};
  ~Parser(){};
  

  void Parse(const Sentence &sentence, const Constraint *constraint,
             std::vector<std::vector<ParserAction>> &nbestActions,
             std::vector<ParserAction> &oracleActions,
             const Model &model) const;

  void AddState(const ParserState &state, const std::vector<uint64_t> &features,
                const ParserAction &action, bool valid, Agenda &agenda,
                float &oracle_score, std::vector<ParserAction> &oracle_actions,
                int &num_valid, const Model &model) const;

  void ExtractFeatures(const Sentence &sentence, const ParserAction &action,
                       const ParserSpan &span, int pivot,
                       std::vector<size_t> &features) const;

  void Permute(int threads, const std::vector<Sentence*>& sentences, const std::string &format,
                     const Model &model) const;
  std::string OutputParseResults(const Sentence &sentence,
                                 const std::string &format,
                                 const Model &model) const;
  std::string
  ParserActionsToString(const std::vector<ParserAction> &actions) const;
  std::string
  ReorderedIndicesToString(const std::vector<ParserAction> &actions) const;


protected:
  int kMaxFactorSize;
  int beamSize;
};
} // namespace HieraParser

#endif // HIERAPARSER_H__