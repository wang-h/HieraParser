#!/usr/bin/python
# Author: Hao WANG 
###############################################
# An implementation of paired bootstrap resampling for testing the statistical
# significance of the difference between two systems from (Koehn 2004 @ EMNLP)
# Specified for reordering Scores.
# Usage: ./bootstrap-resampling.py hypothesis_1 hypothesis_2 reference_1 [ reference_2 ... ]
###############################################


import sys
import numpy as np
from tqdm import tqdm
#constants
TIMES_TO_REPEAT_SUBSAMPLING = 1000
SUBSAMPLE_SIZE = 0 
# if 0 then subsample size is equal to the whole set
MAX_NGRAMS = 4



def less_or_equal(lhs, rhs):
  """Less-or-equal relation of source-side tokens.
  The definition is from ``Neubig et al.: Inducing a Discriminative Parser to
  Optimize Machine Translation Reordering''."""
  return min(lhs) <= min(rhs) and max(lhs) <= max(rhs)

def read_align(line):
  """Read one example from the alignment file."""
  if not line:
    return None
  line = line[:-1]
  fields = line.split()
  if len(fields) < 3:
    sys.exit('Too few fields.')
  if fields[1] != '|||':
    sys.exit('Wrong format.')
  values = fields[0].split('-')
  src_num = int(values[0])
  trg_num = int(values[1])

  # aligns[i] contains the indices of the target tokens which are aligned to
  # the (i+1)-th source token.
  aligns = [set() for _ in range(src_num)]
  for field in fields[2:]:
    values = field.split('-')
    src_id = int(values[0])
    trg_id = int(values[1])
    if src_id < 0 or src_id >= src_num or trg_id < 0 or trg_id >= trg_num:
      sys.stderr.write(line)
      sys.exit('Wrong alignment data: %s')
    aligns[src_id].add(trg_id)

  sorted_list = []
  for i in range(src_num):
    if not aligns[i]:
      continue
    pos = 0
    eq = False
    while pos < len(sorted_list):
      le = less_or_equal(aligns[i], aligns[sorted_list[pos][0]])
      ge = less_or_equal(aligns[sorted_list[pos][0]], aligns[i])
      eq = le and ge
      if not le and not ge:
        return []
      if le:
        break
      pos += 1
    if not eq:
      sorted_list.insert(pos, [])
    sorted_list[pos].append(i)
  alignment = [-1] * src_num
  for i in range(len(sorted_list)):
    for j in sorted_list[i]:
      alignment[j] = i
  alignment.append(len(sorted_list))
  return alignment

def read_order(line):
  """Read one example from the order file."""
  if not line:
    return None
  line = line[:-1]
  order = line.split()
  order = [int(item) for item in order]
  return order

def calculate_Tau(alignment, order):
  """Calculate Kendall's Tau."""
  src_num = len(order)
  if src_num <= 1:
    return 1.0
  errors = 0
  for i in range(src_num - 1):
    for j in range(i + 1, src_num):
      if alignment[order[i]] > alignment[order[j]]:
        errors += 1
  tau = 1.0 - float(errors) / (src_num * (src_num - 1) / 2)
  return tau


def calculate_FRS(alignment, order):
  """Calculate the fuzzy reordering score."""
  src_num = len(order)
  if src_num <= 1:
    return 1.0
  discont = 0
  for i in range(src_num + 1):
    trg_prv = alignment[order[i - 1]] if i - 1 >= 0 else -1
    trg_cur = alignment[order[i]] if i < src_num else alignment[-1]
    if trg_prv != trg_cur and trg_prv + 1 != trg_cur:
      discont += 1
  frs = 1.0 - float(discont) / (src_num + 1)
  return frs

def calculate_CMS(alignment, order):
  """Calculate the complete matching score."""
  if calculate_Tau(alignment, order) < 1.0:
      return 0.0
  else:
      return 1.0 

def getFRS(aligns, orders, indices=None):
    return _CalculateReorderingScores(aligns, orders, calculate_FRS, indices)

def getNKT(aligns, orders, indices=None):
    return _CalculateReorderingScores(aligns, orders, calculate_Tau, indices)

def getCMS(aligns, orders, indices=None):
    return _CalculateReorderingScores(aligns, orders, calculate_CMS, indices)


def _CalculateReorderingScores(aligns, orders, scoreFunc, indices=None):
    num = 0
    skipped = 0
    sum_ = []
    if indices is None:
        candidates = range(len(aligns))
    else:
        candidates = indices
    for idx in candidates:
        alignment = aligns[idx]
        order = orders[idx].copy()
        
        if not alignment:
            skipped += 1
            continue
        assert len(alignment) - 1 == len(order)

        # Remove unaligned tokens.
        for i, a in enumerate(alignment):
            if a < 0:
                order.remove(i)
        num += 1
        sum_.append(scoreFunc(alignment, order))
    return sum(sum_)/num



def main(argv):
    #checking cmdline argument consistency
    if len(argv) != 4:
        print("Usage: ./bootstrap-hypothesis-difference-significance.py hypothesis_1 hypothesis_2 reference\n", file=sys.stderr)
        sys.exit(1) 
    print("reading data", file=sys.stderr)
    #read all data
    data = readAllData(argv)
    # #calculate each sentence's contribution to BP and ngram precision
    # print("rperforming preliminary calculations (hypothesis 1); ", file=sys.stderr)
    # preEvalHypo(data, "hyp1")

    # print("rperforming preliminary calculations (hypothesis 2); ", file=sys.stderr)
    # preEvalHypo(data, "hyp2")

    #start comparing
    print("comparing hypotheses -- this may take some time; ", file=sys.stderr)

    bootstrap_report(data, "Fuzzy Reordering Scores",  getFRS)
    bootstrap_report(data, "Normalized Kendall's Tau", getNKT)
    bootstrap_report(data, "CMS", getCMS)

#####
def bootstrap_report(data, title, func):
    subSampleIndices = np.random.choice(data["size"], SUBSAMPLE_SIZE if SUBSAMPLE_SIZE > 0 else data["size"], replace=True)
    realScore1 = func(data["refs"], data["hyp1"], subSampleIndices)
    realScore2 = func(data["refs"], data["hyp2"], subSampleIndices)
    subSampleScoreDiffArr, subSampleScore1Arr, subSampleScore2Arr = bootstrap_pass(data, func)
    
    scorePValue = bootstrap_pvalue(subSampleScoreDiffArr, realScore1, realScore2)

    (scoreAvg1, scoreVar1) = bootstrap_interval(subSampleScore1Arr)
    (scoreAvg2, scoreVar2) = bootstrap_interval(subSampleScore2Arr)

    print ("\n---=== %s score ===---\n" % title)
    print ("actual score of hypothesis 1: %f" % realScore1)
    print ("95/100 confidence interval for hypothesis 1 score: %f +- %f"%(scoreAvg1, scoreVar1) + "\n-----\n")
    print ("actual score of hypothesis 1: %f" % realScore2)
    print ("95/100 confidence interval for hypothesis 2 score:  %f +- %f"%(scoreAvg2, scoreVar2)+ "\n-----\n")
    print ("Assuming that essentially the same system generated the two hypothesis translations (null-hypothesis),\n")
    print ("the probability of actually getting them (p-value) is: %f\n"% scorePValue)


#####
def bootstrap_pass(data, scoreFunc):
    subSampleDiffArr = []
    subSample1Arr = []
    subSample2Arr = []
    
    #applying sampling
    for idx in tqdm(range(TIMES_TO_REPEAT_SUBSAMPLING), ncols=80, postfix="Subsampling"):
        subSampleIndices = np.random.choice(data["size"], SUBSAMPLE_SIZE if SUBSAMPLE_SIZE > 0 else data["size"], replace=True)
        score1 = scoreFunc(data["refs"], data["hyp1"], subSampleIndices)
        score2 = scoreFunc(data["refs"], data["hyp2"], subSampleIndices)
        subSampleDiffArr.append(abs(score2 - score1))
        subSample1Arr.append(score1)
        subSample2Arr.append(score2)
    return np.array(subSampleDiffArr), np.array(subSample1Arr), np.array(subSample2Arr)
#####
#
#####
def bootstrap_pvalue(subSampleDiffArr, realScore1, realScore2):
    realDiff = abs(realScore2 - realScore1)

    #get subsample difference mean
    averageSubSampleDiff = subSampleDiffArr.mean()

    #calculating p-value
    count = 0.0

    for subSampleDiff in subSampleDiffArr:
        if subSampleDiff - averageSubSampleDiff >= realDiff:
            count += 1
    return count / TIMES_TO_REPEAT_SUBSAMPLING

#####
#
#####
def  bootstrap_interval(subSampleArr):
    sortedArr = sorted(subSampleArr, reverse=False)
    lowerIdx = int(TIMES_TO_REPEAT_SUBSAMPLING / 40)
    higherIdx = TIMES_TO_REPEAT_SUBSAMPLING - lowerIdx - 1

    lower = sortedArr[lowerIdx]
    higher = sortedArr[higherIdx]
    diff = higher - lower
    return (lower + 0.5 * diff, 0.5 * diff)

#####
# read 2 hyp and 1 to \infty ref data files
#####
def readAllData(argv):
    assert len(argv[1:]) == 3
    hypFile1, hypFile2 = argv[2:]
    refFile = argv[1]
    result = {}
    #reading hypotheses and checking for matching sizes
    result["hyp1"] = [read_order(line) for line in open(hypFile1)] 
    result["size"] = len(result["hyp1"])

    result["hyp2"]  = [read_order(line) for line in open(hypFile2)] 
    assert len(result["hyp2"]) ==  len(result["hyp1"])

    refDataX = [read_align(line) for line in open(refFile)] 
    # updateCounts($result{ngramCounts}, $refDataX);
    result["refs"] = refDataX
    return result


if __name__ == '__main__':
  main(sys.argv)
