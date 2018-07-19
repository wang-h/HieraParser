#!/usr/bin/python
###############################################
# Usage: ./reorder.py source.txt order.txt > source.reordered.txt
###############################################

import sys

for i, (line, order) in enumerate(zip(open(sys.argv[1]), open(sys.argv[2]))):
	try:
		order = [int(x) for x in order.rstrip(" \n").split()]
		words = line.rstrip(" \n").split()
		print(" ".join(words[x] for x in order))
	except:
		print("Error at line ", i, file=sys.stderr)
