HieraParser v2.0 - a parallel implementation of top-down BTG-based preorderer
==========


HieraParser is a fast and parallel implementation of Top-down Bracketing Transduction Grammar (BTG)-based preordering, extended to support multi-threading, parallel training and parallel parsing.

For the online training algorithm for Top-down BTG-based parser, please refer to the following paper,
[Tetsuji Nakagawa: Efficient Top-Down BTG Parsing for Machine Translation Preordering](https://github.com/google/topdown-btg-preordering) ,
[ACL-2015](http://www.aclweb.org/anthology/P15-1021)


Differing from Nakagawa's implementation in which uses the online Passive-Aggressive (PA) algorithm to train preorderer, we adopt several parallel techniques to parallize the PA algorithm and  train the preorderer in parallel. This application is more fast and robust when training on automatically aligned datasets.


## Compiling and Installation

Building `HieraParser` requires a modern C++ (>=C++11) compiler and the [CMake]() build system. 
Please make sure your complier version (gcc/clang) supports "pthread" before compiling. 

To compile, do the following 

    mkdir build

    cd build/

    cmake ..

    make
## Usage:

#### 1. training the model 

    ./bin/train -input data/train.en.annot -align data/train.en.aligned -model hierp.model
        [main arguments]:
            -beam: maximum beam size during parsing (default 20)
            -batch: batch size of training examples.
            -shuffle: shuffle the training examples during each iteration, recommended
            -iterations: maximum iteration to training the reordering model (default 20)
            -input: path to input file of the source language.
            -align: path to alignment file.
            -model: path to model file [output].
            -threads: the number of threads used for training.
            -kbest: using k-best derivations, it should be a number smaller than beam size. (default: 5)
            -strategy: which parallel strategy to use
                [ 1. mini-batch learning, 
                  2. distributed averaging,
                  3. iteratively distributed averaging ]

* 0. is avaliable for test (online training)

#### 2. parsing with the model 

    ./bin/parse -input data/train.en.annot -align data/train.en.aligned -model hierp.model
        [main arguments]:
            -beam: maximum beam size during parsing (default 20)
            -input: path to input file of the source language.
            -model: path to model file [input].
            -threads: the number of threads to use for parsing (default : auto detected).


## Features:

 1.  It was implemented with (C++11 STL) built-in multi-threading library.
 2.  farmhash is used, instead of boost::hash_combine or std::hasher in STL.
 3.  Added the multi-threading support for parsing.




## Comparison:
Compared to [TD-BTG-Preorderer](https://github.com/google/topdown-btg-preordering), HieraParser is more accurate and fast.

If you have any problem, please contact the author.
Copyright (C) 2017 Hao WANG, Waseda University.
