#include "Parser.h"
#include "utils/ThreadPool.h"
#include "utils/farmhash.h"
#include "utils/ProgressBar.h"
#include "BaseModel.h"
#include "Config.h"
#include <iterator>
#include <string>


using namespace NAMESPACE_FOR_HASH_FUNCTIONS;

namespace HieraParser {



uint64_t *FeatureFingerprintNumbers(size_t n)
{
    uint64_t *a = new uint64_t[n];
    std::hash<size_t> hasher;
    for (int i = 0; i < static_cast<int>(n); i++)
    {
        a[i] = hasher(i);
    }
    return a;
}
inline uint64_t FeatureFingerPrintCat(uint64_t fp1, uint64_t fp2) {
  // return Hash128to64(uint128(fp1, fp2)); //cityhash
  return Hash128to64(Uint128(fp1, fp2)); // farmhash
}

inline uint64_t FeatureFingerPrintCat(uint64_t fp1, uint64_t fp2,
                                      uint64_t fp3) {
  return FeatureFingerPrintCat(FeatureFingerPrintCat(fp1, fp2), fp3);
}

inline uint64_t FeatureFingerPrintCat(uint64_t fp1, uint64_t fp2, uint64_t fp3,
                                      uint64_t fp4) {
  return FeatureFingerPrintCat(FeatureFingerPrintCat(fp1, fp2, fp3), fp4);
}

inline uint64_t FeatureFingerPrintCat(uint64_t fp1, uint64_t fp2, uint64_t fp3,
                                      uint64_t fp4, uint64_t fp5) {
  return FeatureFingerPrintCat(FeatureFingerPrintCat(fp1, fp2, fp3, fp4), fp5);
}

inline void PreCalculateSpan(const ParserSpan &span,
                             const Constraint &constraint,
                             std::vector<int> &lmin, std::vector<int> &rmin,
                             std::vector<int> &lmax, std::vector<int> &rmax) {
  int cur_lmin = -1;
  int cur_lmax = -1;
  for (int i = 0; i < span.end - span.bgn; i++) {
    int c = constraint[span.bgn + i];
    if (c >= 0) {
      if (cur_lmin < 0 || c < cur_lmin)
        cur_lmin = c;
      if (cur_lmax < 0 || c > cur_lmax)
        cur_lmax = c;
    }
    lmin[i] = cur_lmin;
    lmax[i] = cur_lmax;
  }
  int cur_rmin = -1;
  int cur_rmax = -1;
  for (int i = span.end - span.bgn - 1; i >= 0; i--) {
    int c = constraint[span.bgn + i];
    if (c >= 0) {
      if (cur_rmin < 0 || c < cur_rmin)
        cur_rmin = c;
      if (cur_rmax < 0 || c > cur_rmax)
        cur_rmax = c;
    }
    rmin[i] = cur_rmin;
    rmax[i] = cur_rmax;
  }
}

Parser::Parser(const Config &cfg) {
  kMaxFactorSize = cfg.GetInt("factors");
  beamSize = cfg.GetInt("beam");
}




void Parser::Parse(const Sentence &sentence, const Constraint *constraint,
                   std::vector<std::vector<ParserAction>> &nbestActions,
                   std::vector<ParserAction> &oracleActions,
                   const Model &model) const{
  Agenda old_agenda;
  Agenda new_agenda;
  size_t length = sentence.size();
  old_agenda.push(ParserState(length));
  std::vector<size_t> features;
  for (size_t transition = 0; transition < length - 1; transition++) {
    int num_valid = 0;
    float oracle_score = std::numeric_limits<float>::lowest();
    if (constraint != nullptr) {
      oracleActions.clear();
    }
    while (!old_agenda.empty()) {
      const ParserState &state = old_agenda.top();
      const ParserSpan &span = state.stack.back();
      std::vector<int> lmin(span.end - span.bgn);
      std::vector<int> lmax(span.end - span.bgn);
      std::vector<int> rmin(span.end - span.bgn);
      std::vector<int> rmax(span.end - span.bgn);
      if (constraint != nullptr) {
        PreCalculateSpan(span, *constraint, lmin, rmin, lmax, rmax);
      }
      const ParserAction &action = state.actions[span.action_id];
      // action is the parent action.
      for (int pivot = span.bgn + 1; pivot < span.end; pivot++) {
        bool valid = false;
        ExtractFeatures(sentence, action, span, pivot, features);
        // Consider the tree with STR node split at <pivot>.
        if (constraint != nullptr) {
          valid =
              (lmax[pivot - 1 - span.bgn] < 0 || rmin[pivot - span.bgn] < 0 ||
               lmax[pivot - 1 - span.bgn] <= rmin[pivot - span.bgn]) &&
              state.valid;
        }
        AddState(state, features, ParserAction(pivot, false), valid, new_agenda,
                 oracle_score, oracleActions, num_valid, model);
        // Consider the tree with INV node split at <pivot>.
        if (constraint != nullptr) {
          valid =
              (rmax[pivot - span.bgn] < 0 || lmin[pivot - 1 - span.bgn] < 0 ||
               rmax[pivot - span.bgn] <= lmin[pivot - 1 - span.bgn]) &&
              state.valid;
        }
        AddState(state, features, ParserAction(pivot, true), valid, new_agenda,
                 oracle_score, oracleActions, num_valid, model);
      }
      old_agenda.pop();
    }
    CHECK(new_agenda.size() >= 1, "No valid candidates");
    old_agenda.swap(new_agenda);
    if (constraint != nullptr && num_valid == 0) {
      // Early update.
      nbestActions.resize(1);
      nbestActions[0] = old_agenda.top().actions;
      return;
    }
  }
  nbestActions.resize(old_agenda.size());
  for (auto it = nbestActions.rbegin(); it != nbestActions.rend(); ++it) {
    const ParserState &state = old_agenda.top();
    CHECK(state.actions.size() == length - 1,
          "Invalid length of an action sequence");
    CHECK(state.stack.size() == 0, "Stack is not empty");
    *it = state.actions;
    old_agenda.pop();
  }
}

void Parser::AddState(const ParserState &state,
                      const std::vector<uint64_t> &features,
                      const ParserAction &action, bool valid, Agenda &agenda,
                      float &oracle_score,
                      std::vector<ParserAction> &oracle_actions, int &num_valid,
                      const Model &model) const{
  // Calculate the score.
  float score = state.score;
  // const MiniHashMap<uint64_t, float, Hash> &weights_mapping =
  //     model.weights[static_cast<size_t>(action.second)];
  const Weights &weights_mapping =
      model.weights[static_cast<size_t>(action.second)];
  for (const uint64_t feature : features) {
    const auto it = weights_mapping.find(feature);
    if (it != weights_mapping.end()) {
      score += it->second;
    }
  }
  if (valid && score > oracle_score) {
    oracle_score = score;
    oracle_actions = state.actions;
    oracle_actions.push_back(action);
  }
  // Remove the candidate with the least score if the agenda size exceeds the
  // beam size.
  if (agenda.size() >= static_cast<size_t>(20)) {
    if (score <= agenda.top().score)
      return;
    if (agenda.top().valid)
      num_valid--;
    agenda.pop();
  }
  if (valid)
    num_valid++;
  agenda.emplace(state, action, score, valid);
}

void Parser::ExtractFeatures(const Sentence &sentence,
                             const ParserAction &action, const ParserSpan &span,
                             int pivot, std::vector<uint64_t> &features) const {
  std::hash<std::string> hasher;
  static const size_t kFpEmpty = hasher("<s>");
  static const size_t kMaxFeatures = 120;
  static const size_t kMaxSpanSize = 100;
  static const size_t *kFpNumbers = FeatureFingerprintNumbers(kMaxFeatures);

  size_t id = 0;
  features.clear();
  features.reserve(kMaxFeatures);
  int lchild_bgn = span.bgn;
  int lchild_end = pivot;
  int rchild_bgn = pivot;
  int rchild_end = span.end;
  int sentence_size = sentence.size();
  int sn = 0;
  if (rchild_end - lchild_bgn >= static_cast<int>(kMaxSpanSize))
    sn = kMaxSpanSize - 1;
  else
    sn = rchild_end - lchild_bgn;
  int ln = lchild_end - lchild_bgn;
  int rn = rchild_end - rchild_bgn;
  int cl = (rn < ln) ? 0 : (rn > ln) ? 1 : 2;
  int nln = (ln > 5) ? 5 : ln;
  int nrn = (rn > 5) ? 5 : rn;
  // bias kFpNumbers
  features.push_back(kFpNumbers[id++]);
  // balance of children
  features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], cl));
  // tree size
  features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], sn));
  // subtree sizes
  features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], nln, nrn));
  //************************************************************
  int gp_nt;
  int gp_side;
  if (span.action_id < 0) {
    gp_nt = 2;
    gp_side = 2;
  } else {
    gp_nt = static_cast<int>(action.second);
    gp_side = static_cast<int>(span.bgn == action.first);
  }
  // NT
  features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], gp_nt));
  // NT&SIDE
  features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], gp_nt, gp_side));
  //************************************************************

  for (int i = 0; i < static_cast<int>(3); i++) {
    // Ll-
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++],
        (lchild_bgn - 1 >= 0) ? sentence[lchild_bgn - 1][i] : kFpEmpty));
    // Ll
    features.push_back(
        FeatureFingerPrintCat(kFpNumbers[id++], sentence[lchild_bgn][i]));
    // Lr
    features.push_back(
        FeatureFingerPrintCat(kFpNumbers[id++], sentence[lchild_end - 1][i]));
    // Rl
    features.push_back(
        FeatureFingerPrintCat(kFpNumbers[id++], sentence[rchild_bgn][i]));
    // Rr
    features.push_back(
        FeatureFingerPrintCat(kFpNumbers[id++], sentence[rchild_end - 1][i]));
    // Rr+
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++],
        (rchild_end < sentence_size) ? sentence[rchild_end][i] : kFpEmpty));

    // Ll-&Ll
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++],
        ((lchild_bgn - 1 >= 0) ? sentence[lchild_bgn - 1][i] : kFpEmpty),
        sentence[lchild_bgn][i]));
    // Ll&Lr
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++],
                                             sentence[lchild_bgn][i],
                                             sentence[lchild_end - 1][i]));
    // Ll&Rl
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++], sentence[lchild_bgn][i], sentence[rchild_bgn][i]));
    // Ll&Rr
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++],
                                             sentence[lchild_bgn][i],
                                             sentence[rchild_end - 1][i]));
    // Lr&Rl
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++],
                                             sentence[lchild_end - 1][i],
                                             sentence[rchild_bgn][i]));
    // Lr&Rr
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++],
                                             sentence[lchild_end - 1][i],
                                             sentence[rchild_end - 1][i]));
    // Rl&Rr
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++],
                                             sentence[rchild_bgn][i],
                                             sentence[rchild_end - 1][i]));
    // Rr&Rr+
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++], sentence[rchild_end - 1][i],
        ((rchild_end < sentence_size) ? sentence[rchild_end][i] : kFpEmpty)));

    // Lr-&Lr&Rl
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++],
        ((lchild_end - 2 >= 0) ? sentence[lchild_end - 2][i] : kFpEmpty),
        sentence[lchild_end - 1][i], sentence[rchild_bgn][i]));
    // Ll&Lr&Rl
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++], sentence[lchild_bgn][i], sentence[lchild_end - 1][i],
        sentence[rchild_bgn][i]));
    // Lr&Rl&Rr
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++], sentence[lchild_end - 1][i], sentence[rchild_bgn][i],
        sentence[rchild_end - 1][i]));

    // Lr&Rl&Rl+
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++], sentence[lchild_end - 1][i], sentence[rchild_bgn][i],
        ((rchild_bgn + 1 < sentence_size) ? sentence[rchild_bgn + 1][i]
                                          : kFpEmpty)));

    // Ll&Lr&Rl&Rr
    features.push_back(FeatureFingerPrintCat(
        kFpNumbers[id++], sentence[lchild_bgn][i], sentence[lchild_end - 1][i],
        sentence[rchild_bgn][i], sentence[rchild_end - 1][i]));

    // NT&SIDE&Ll
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], gp_nt, gp_side,
                                             sentence[lchild_bgn][i]));
    // NT&SIDE&Lr
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], gp_nt, gp_side,
                                             sentence[lchild_end - 1][i]));
    // NT&SIDE&Rl
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], gp_nt, gp_side,
                                             sentence[rchild_bgn][i]));
    // NT&SIDE&Rr
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], gp_nt, gp_side,
                                             sentence[rchild_end - 1][i]));

    // NT&SIDE&Ll&Rr
    features.push_back(FeatureFingerPrintCat(kFpNumbers[id++], gp_nt, gp_side,
                                             sentence[lchild_bgn][i],
                                             sentence[rchild_end - 1][i]));
  }
  CHECK(features.size() <= kMaxFeatures, "Too many features");
}

void Parser::Permute(int threads, const std::vector<Sentence*>& sentences, const std::string &format,
                     const Model &model) const {
  ThreadPool pool(threads);
  std::vector<std::future<std::string>> results;
  int i = 0;

  
  int step = int(sentences.size()/100);
  for (auto it = sentences.begin(); it != sentences.end(); ++it, i++) {

    if (i % step == 0 && i >0){
      std::stringstream log_string;
      std::cerr << std::flush << ProgressBar(i, sentences.size()-1, " ") << "\r";
    }
    if ( i == static_cast<int>(sentences.size()) -1 )
    {
      std::cerr << std::endl;
    }
    results.emplace_back(
        pool.enqueue(&Parser::OutputParseResults, *this, std::ref(*(*it)), std::ref(format), std::ref(model)));
    if (i % 1000 == 0 && i != 0) {
      for (auto &&result : results) {
        std::cout << result.get() << std::endl;
      }
      results.clear();
    }
  }
  if (results.size() > 0) {
    for (auto &&result : results) {
      std::cout << result.get() << std::endl;
    }
  }
}

std::string Parser::OutputParseResults(const Sentence &sentence,
                                       const std::string &format,
                                       const Model &model) const {
  std::vector<std::vector<ParserAction>> nbestActions;
  std::vector<ParserAction> actions_ref;
  Parse(sentence, nullptr, nbestActions, actions_ref, model);
  const std::vector<ParserAction> &actions_sys = nbestActions.front();
  if (format.compare("action") == 0) {
    return ParserActionsToString(actions_sys);
  } else if (format.compare("order") == 0) {
    return ReorderedIndicesToString(actions_sys);
  } else
    return " ";
}

std::string
Parser::ParserActionsToString(const std::vector<ParserAction> &actions) const {
  if (actions.size() == 0) {
    return "";
  }
  std::stringstream ret;
  for (size_t i = 0; i < actions.size(); i++) {
    const ParserAction &action = actions[i];
    if (i == 0)
      ret << action.first << "-" << action.second;
    else
      ret << " " << action.first << "-" << action.second;
  }
  return ret.str();
}

std::string Parser::ReorderedIndicesToString(
    const std::vector<ParserAction> &actions) const {
  std::vector<int> order;
  std::vector<BTGTree> subtrees;
  ParserState state(actions.size() + 1);
  for (const ParserAction &action : actions) {
    const ParserSpan &span = state.stack.back();
    subtrees.emplace_back(span.bgn, span.end, action.first, action.second);
    state.Advance(action, 0.0, false);
  }
  order.clear();
  for (size_t i = 0; i < actions.size() + 1; i++) {
    order.push_back(i);
  }
  // Traverse the nodes in a right-to-left bottom-up way.
  while (!subtrees.empty()) {
    const BTGTree &subtree = subtrees.back();
    if (subtree.label) {
      order.insert(order.begin() + subtree.end, order.begin() + subtree.bgn,
                   order.begin() + subtree.pivot);
      order.erase(order.begin() + subtree.bgn, order.begin() + subtree.pivot);
    }
    subtrees.pop_back();
  }
  std::stringstream result;
  std::copy(order.begin(), order.end(),
            std::ostream_iterator<int>(result, " "));
  return result.str();
}

} // namespace HieraParser
