#!/usr/bin/python
# Author: Hao WANG
##############################################################################################
# An implementation of paired bootstrap resampling for testing the statistical
# significance of the difference between two systems from (Koehn 2004 @ EMNLP)
# Specified for RIBES and BLEU.
# Usage: ./bootstrap-resampling.py hypothesis_1 hypothesis_2 reference_1 [ reference_2 ... ]
##############################################################################################

import sys
import os
import re
import datetime
import numpy as np
from math import exp, log
from tqdm import tqdm
from collections import Counter
TIMES_TO_REPEAT_SUBSAMPLING = 1000
SUBSAMPLE_SIZE = 0
MAX_NGRAMS = 4
alpha = 0.25
beta = 0.10

# "overlapping" substring counts ( string.count(x) returns "non-overlapping" counts... )

def overlapping_count(pattern, string):
    pos = string.find(pattern)
    if pos > -1:
        return 1 + overlapping_count(pattern, string[pos+1:])
    else:
        return 0

# calculate Kendall's tau



#####
def bootstrap_report(data, title, scoreFunc):
    subSampleScoreDiffArr, subSampleScore1Arr, subSampleScore2Arr = bootstrap_pass(data, scoreFunc)
    realScore1 = scoreFunc(data["refs"], data["hyp1"])
    realScore2 = scoreFunc(data["refs"], data["hyp2"])
    scorePValue = bootstrap_pvalue(
        subSampleScoreDiffArr, realScore1, realScore2)

    (scoreAvg1, scoreVar1) = bootstrap_interval(subSampleScore1Arr)
    (scoreAvg2, scoreVar2) = bootstrap_interval(subSampleScore2Arr)

    print("---=== %s score ===---" % title)
    print("actual score of hypothesis 1: %f"% realScore1)
    print("95/100 confidence interval for hypothesis 1 score: %f +- %f" %
          (scoreAvg1, scoreVar1) + "\n-----")
    print("actual score of hypothesis 1: %f" % realScore2)
    print("95/100 confidence interval for hypothesis 2 score:  %f +- %f" %
          (scoreAvg2, scoreVar2) + "\n-----")
    print("Assuming that essentially the same system generated the two hypothesis translations (null-hypothesis)")
    print("the probability of actually getting them (p-value) is: %f" %scorePValue)


#####
def bootstrap_pass(data, scoreFunc):
    subSampleDiffArr = []
    subSample1Arr = []
    subSample2Arr = []

    # applying sampling
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
    
    # get subsample difference mean
    averageSubSampleDiff = subSampleDiffArr.mean()

    # calculating p-value
    count = 0.0
    realDiff = abs(realScore2 - realScore1)

    for subSampleDiff in subSampleDiffArr:
        if subSampleDiff - averageSubSampleDiff >= realDiff:
            count += 1
    return count / TIMES_TO_REPEAT_SUBSAMPLING


def bootstrap_interval(subSampleArr):
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


def readlines(filename):
    l = []
    for line in open(filename):
        snt = {}
        snt["words"] = line[:-1].split()
        l.append(snt)
    return l


def readAllData(argv):
    assert len(argv[1:]) == 3
    hypFile1, hypFile2 = argv[1:3]
    refFiles = argv[3:]
    result = {}
    # reading hypotheses and checking for matching sizes
    result["hyp1"] = readlines(hypFile1)
    result["size"] = len(result["hyp1"])

    result["hyp2"] = readlines(hypFile2)
    assert len(result["hyp2"]) == len(result["hyp1"])
    result["ngramCounts"] = Counter()

    result["refs"] = []
    for refFile in refFiles:
        refDataX = readlines(refFile)
        updateCounts(result["ngramCounts"], refDataX)
        result["refs"].append(refDataX)
    return result


def preEvalHypo(data, hypId):
    for lineIdx in range(data["size"]):
        preEvalHypoSnt(data, hypId, lineIdx)


def preEvalHypoSnt(data, hypId, lineIdx):
    refNgramCounts = Counter()
    hypNgramCounts = Counter()
    hypSnt = data[hypId][lineIdx]

    # update total hyp len
    hypSnt["hyplen"] = len(hypSnt["words"])

    # update total ref len with closest current ref len
    hypSnt["reflen"] = getClosestLength(
        data["refs"], lineIdx, hypSnt["hyplen"])
    hypSnt["avgreflen"] = getAvgLength(data["refs"], lineIdx)

    hypSnt["correctNgrams"] = {}
    hypSnt["totalNgrams"] = {}

    # update ngram precision for each n-gram order
    for order in range(1, MAX_NGRAMS+1):
        # hyp ngrams
        hypNgramCounts = groupNgrams(hypSnt, order)

        # ref ngrams
        refNgramCounts = groupNgramsMultiSrc(data["refs"], lineIdx, order)

        correctNgramCounts = 0
        totalNgramCounts = 0
        #correct, total
        for ngram in hypNgramCounts.keys():
            coocUpd = min(hypNgramCounts[ngram], refNgramCounts[ngram])
            correctNgramCounts += coocUpd
            totalNgramCounts += hypNgramCounts[ngram]

        hypSnt["correctNgrams"][order] = correctNgramCounts
        hypSnt["totalNgrams"][order] = totalNgramCounts

    # additional processing for RIBES    
    stats = [getAscending(hypSnt, ref[lineIdx]) for ref in data["refs"]]
    hypSnt["pairs"] = max(x[1] for x in stats)
    hypSnt["ascending"] = max(x[0] for x in stats)
    

def getAscending(hypSnt, ref):
    intlist = []
    mapped_ref = " ".join(ref["words"])
    mapped_hyp = " ".join(hypSnt["words"])
    for i, w in enumerate(hypSnt["words"]):
        if w not in mapped_ref:
            pass
        elif mapped_ref.count(w) == 1 and mapped_hyp.count(w) == 1:
            intlist.append(mapped_ref.index(w))
        else:
            for window in range(1, max(i+1, hypSnt["hyplen"]-i+1)):
                if window <= i:
                    ngram = " ".join(hypSnt["words"][i-window:i+1])
                    if overlapping_count(ngram, mapped_ref) == 1 and overlapping_count(ngram, mapped_hyp) == 1:
                        intlist.append(mapped_ref.index(ngram) + len(ngram) - 1)
                        break
                if i+window < hypSnt["hyplen"]:
                    ngram = " ".join(hypSnt["words"][i:i+window+1])
                    if overlapping_count(ngram, mapped_ref) == 1 and overlapping_count(ngram, mapped_hyp) == 1:
                        intlist.append(mapped_ref.index(ngram))
                        break
    ascending = 0.0
    n = len(intlist)
    if n == 1 and len(ref) == 1:
        return 1.0, 1.0
    elif n < 2:
        return 0.0, 0.0
    for i in range(len(intlist)-1):
        for j in range(i+1, len(intlist)):
            if intlist[i] < intlist[j]:
                ascending += 1
    return ascending, len(intlist)

def groupAscendingMultiSrc(refs, lineIdx, order):
    result = Counter()
    for ref in refs:
        currNgramCounts = groupNgrams(ref[lineIdx], order)

        for currNgram in currNgramCounts.keys():
            result[currNgram] = max(
                result[currNgram], currNgramCounts[currNgram])
    return result

def getClosestLength(refs, lineIdx, hypothesisLength):

    bestDiff = float('inf')
    bestLen = float('inf')

    for ref in refs:
        currLen = len(ref[lineIdx]["words"])
        currDiff = abs(currLen - hypothesisLength)

        if (currDiff < bestDiff or (currDiff == bestDiff and currLen < bestLen)):
            bestDiff = currDiff
            bestLen = currLen
    return bestLen

def safeLog(x):
    if x > 0:
        return log(x)
    else:
        return -float("inf")


def getBleu(refs, hyp, idxs=[]):

    # default value for $idxs
    if len(idxs) > 0:
        idxs = idxs
    else:
        idxs = range(len(hyp))

    # vars
    hypothesisLength, referenceLength = 0, 0
    correctNgramCounts = Counter()
    totalNgramCounts = Counter()

    # gather info from each line
    for lineIdx in idxs:
        hypSnt = hyp[lineIdx]

        # update total hyp len
        hypothesisLength += hypSnt["hyplen"]

        # update total ref len with closest current ref len
        referenceLength += hypSnt["reflen"]

        # update ngram precision for each n-gram order
        for order in range(1, MAX_NGRAMS+1):
            correctNgramCounts[order] += hypSnt["correctNgrams"][order]
            totalNgramCounts[order] += hypSnt["totalNgrams"][order]

    # compose bleu score
    brevityPenalty = exp(1.0 - float(referenceLength) /
                              hypothesisLength) if (hypothesisLength < referenceLength) else 1.0

    logsum = 0

    for order in range(1, MAX_NGRAMS+1):
        logsum += safeLog(correctNgramCounts[order] / totalNgramCounts[order])

    return brevityPenalty * exp(logsum / MAX_NGRAMS)

def getRIBES(refs, hyp, idxs=[]):

    # default value for $idxs
    if len(idxs) > 0:
        idxs = idxs
    else:
        idxs = range(len(hyp))
    
    ribes_ = 0.
    # gather info from each line
    for lineIdx in idxs:
        hypSnt = hyp[lineIdx]
        # compute kendall tau
        ascending = hypSnt["ascending"]
        total = (hypSnt["pairs"] * (hypSnt["pairs"] - 1))/2
        bp = min(1.0, exp(1.0 - 1.0 * hypSnt["reflen"]/hypSnt["hyplen"]))
        nkt = 0.0 if total == 0 else ascending / total
        precision = float(hypSnt["pairs"]) / hypSnt["hyplen"]
        ribes_ += nkt * (precision ** alpha) * (bp ** beta)
    return ribes_/len(idxs)



def updateCounts(countHash, refData):
    for snt in refData:
        size = len(snt["words"])
        countHash[""] += size
        for order in range(1, MAX_NGRAMS+1):
            for i in range(size-order+1):
                ngram = " ".join(snt["words"][i:i + order])
                countHash[ngram] += 1


def getAvgLength(refs, lineIdx):
    result = 0.
    count = 0.
    for ref in refs:
        result += len(ref[lineIdx]["words"])
        count += 1

    return result / count


#####
#
#####
def groupNgrams(snt, order):
    result = Counter()
    size = len(snt["words"])
    for i in range(size-order+1):
        ngram = " ".join(snt["words"][i:i + order])
        result[ngram] += 1
    return result

#####
#
#####


def groupNgramsMultiSrc(refs, lineIdx, order):
    result = Counter()
    for ref in refs:
        currNgramCounts = groupNgrams(ref[lineIdx], order)

        for currNgram in currNgramCounts.keys():
            result[currNgram] = max(
                result[currNgram], currNgramCounts[currNgram])
    return result

#####
#
#####


def main(argv):
    # checking cmdline argument consistency
    if len(argv) != 4:
        print("Usage: ./bootstrap-resampling-ribes.py hypothesis_1 hypothesis_2 reference\n", file=sys.stderr)
        sys.exit(1)
    print("reading data", file=sys.stderr)
    # read all data
    data = readAllData(argv)
    # #calculate each sentence's contribution to BP and ngram precision
    print("performing preliminary calculations (hypothesis 1); ", file=sys.stderr)
    preEvalHypo(data, "hyp1")

    print("performing preliminary calculations (hypothesis 2); ", file=sys.stderr)
    preEvalHypo(data, "hyp2")

    # start comparing
    print("comparing hypotheses -- this may take some time; ", file=sys.stderr)
    bootstrap_report(data, "BLUE", getBleu)
    bootstrap_report(data, "RIBES", getRIBES)


if __name__ == '__main__':
    main(sys.argv)
