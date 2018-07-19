#include <stdio.h>
#include <string.h>
#include "Config.h"
#include "Parser.h"
#include "Sentence.h"
#include "BaseModel.h"
using namespace HieraParser;

int main(int argc, char **argv)
{
    //  read arg* and configuration
    //  EXIT_SUCCESS 0
    //  EXIT_FAILURE 1
    Config cfg;
    int threads =  static_cast < int >(std::thread::hardware_concurrency());

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // default main arguments true is the main args
    
    cfg.AddConfigEntry("input", "", "path to input file of the source language.", true);
    cfg.AddConfigEntry("model", "", "path to output file.", true);
    cfg.AddConfigEntry("threads", std::to_string(threads), "the number of threads to use for parsing.", true);
    cfg.AddConfigEntry("format", "order", "format of the output\n"
                                          " \t[order: reordered target indices, action: parse actions]",
                       true);
    cfg.AddConfigEntry("factors", "3",
                     "the number of factors to represent each token.", true);
    cfg.AddConfigEntry("beam", "20", "maximum beam size during parsing (default 20)", true);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (cfg.ReadMainArgs(argc, argv) > 0)
    {
        cfg.PrintUsage();
        return 1;
    }

    //check required arguments
    if (cfg.GetString("model").empty() || cfg.GetString("input").empty())
    {
        std::cerr << "Missing the required arguments [-model ?  -input?]" << std::endl;
        return 1;
    }

    // load input
    Parser parser(cfg);
    std::vector<Sentence*> sentences;
    LoadInput(sentences, cfg.GetString("input"), static_cast<size_t>(cfg.GetInt("factors")));

    // load model
    Model model(cfg);
    model.ReadModel(cfg.GetString("model"));

    // reorder
    parser.Permute(cfg.GetInt("threads"), sentences, cfg.GetString("format"), model);
    return 0;
}
