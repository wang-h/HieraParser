#include "BaseModel.h"
#include "Config.h"
#include "Sentence.h"
#include "trainers/BatchTrainer.h"
#include "trainers/DistributedTrainer.h"
#include "trainers/IterDistributedTrainer.h"
#include "trainers/TrainerBase.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>
using namespace HieraParser;

int main(int argc, char **argv) {
  // read arg* and configuration
  // EXIT_SUCCESS 0
  // EXIT_FAILURE 1
  Config cfg;
  unsigned threads = std::thread::hardware_concurrency();

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // main arguments
  cfg.AddConfigEntry("input", "", "path to source annotated file.", true);
  cfg.AddConfigEntry("align", "", "path to alignment file.", true);
  cfg.AddConfigEntry("model", "", "path to output file.", true);

  // optional arguments
  // for batch learning
  cfg.AddConfigEntry("strategy", "1",
                     "which strategy used for training"
                     "[1. batch learning,\n"
                     " 2. distributed learning (para-mix),\n"
                     " 3. distributed learning (iter-para-mix)]",
                     true);

  cfg.AddConfigEntry(
      "threads", std::to_string(threads),
      "the number of threads will be used for training (default: auto detected).",
      true);
  cfg.AddConfigEntry("factors", "3",
                     "the number of factors to represent each token.", true);
  cfg.AddConfigEntry("batch", "20", "the number of training examples in each mini-batch.", true);

  // for batch learning/online learning
  cfg.AddConfigEntry(
      "iterations", "20",
      "maximum iteration to training the reordering model (default: 20)", true);
  cfg.AddConfigEntry("beam", "20",
                     "maximum beam size during parsing (default: 20)", true);
  cfg.AddConfigEntry(
      "kbest", "5",
      "kbest should be a number less than beam size. (default: 5)", true);
  cfg.AddConfigEntry(
      "early_stop", "false",
      "using early stopping to avoid overfitting (default: false).", true);
  cfg.AddConfigEntry(
      "shuffle", "false",
      "shuffle the training examples during each iteration, recommended", true);
  // cfg.AddConfigEntry(
  //     "init_param", "false",
  //     "construct normal distribution for parameter initialization, recommended", true);
  cfg.AddConfigEntry("batch_norm", "false",
                     "normalize loss with batch size (default: false), "
                     "recommended for large datasets (>= 10K).",
                     true);
  cfg.AddConfigEntry("save_step", "0", "save the model after every k-epochs.",
                     true);
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  if (cfg.ReadMainArgs(argc, argv) > 0) {
    cfg.PrintUsage();
    return 1;
  }
  // check required arguments
  if (cfg.GetString("model").empty() || cfg.GetString("input").empty() ||
      cfg.GetString("align").empty()) {
    std::cerr << "Missing the required arguments [-model ? -align? -input?]"
              << std::endl;
    return 1;
  }
  if (cfg.GetBool("shuffle")){
    std::cerr << "Shuffle data: true."
              << std::endl;
  }
  std::string kbest = cfg.GetString("kbest");
  // init parser
  TrainerBase *trainer;
  int strategy = cfg.GetInt("strategy");
  if (strategy == 0) {
    std::cerr << "Using " << kbest << "-best Online Trainer ..." << std::endl;
    trainer = new TrainerBase(cfg);
  } else if (strategy == 1) {
    std::cerr << "Using " << kbest << "-best Batch Trainer..." << std::endl;
    trainer = new BatchTrainer(cfg);
  } else if (strategy == 2) {
    std::cerr << "Using " << kbest << "-best Distributed Trainer..."
              << std::endl;
    trainer = new DistributedTrainer(cfg);
  } else if (strategy == 3) {
    std::cerr << "Using " << kbest << "-best Iterative Distributed Trainer..."
              << std::endl;
    trainer = new IterDistributedTrainer(cfg);
  }
  std::vector<Sentence *> sentences;
  Model model(cfg);
  LoadInput(sentences, cfg.GetString("input"),
            static_cast<size_t>(cfg.GetInt("factors")));
  std::vector<TrainingExample> training_examples;
  LoadConstraint(training_examples, sentences, cfg.GetString("align"));
  trainer->Train(training_examples, model);

  std::cerr << "Saving reordering model to:  " << cfg.GetString("model")
            << std::endl;
  model.WriteModel(cfg.GetString("model"));
  return 0;
}
