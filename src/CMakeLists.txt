# add_subdirectory(modules)
# add_subdirectory(trainers)
# add_subdirectory(utils)

add_library(HIERAP_MAIN STATIC
    Alignment.cc
    Constraint.cc
    Parser.cc
    Config.cc
    Sentence.cc
    BaseModel.cc
    trainers/TrainerBase.cc
    trainers/BatchTrainer.cc
    trainers/DistributedTrainer.cc
    trainers/IterDistributedTrainer.cc
    utils/farmhash.cc
)
target_link_libraries(HIERAP_MAIN)
