/*
 * ThemisDB | DistributedTrainer guard-focused tests
 */

#include <gtest/gtest.h>

#include "llm/lora_framework/distributed_trainer.h"

namespace themis { namespace llm { namespace lora { 

TEST(DistributedTrainerGuardFocusedTest, InitializeFailsForWorldSizeZero) {
    DistributedConfig cfg;
    cfg.world_size = 0;
    cfg.rank = 0;

    DistributedTrainer trainer(cfg);
    EXPECT_FALSE(trainer.initialize());
}

TEST(DistributedTrainerGuardFocusedTest, InitializeFailsForNegativeWorldSize) {
    DistributedConfig cfg;
    cfg.world_size = -4;
    cfg.rank = 0;

    DistributedTrainer trainer(cfg);
    EXPECT_FALSE(trainer.initialize());
}

TEST(DistributedTrainerGuardFocusedTest, InitializeFailsForInvalidRankInSingleProcess) {
    DistributedConfig cfg;
    cfg.world_size = 1;
    cfg.rank = 1;

    DistributedTrainer trainer(cfg);
    EXPECT_FALSE(trainer.initialize());
}

TEST(DistributedTrainerGuardFocusedTest, InitializeFailsForNegativeRankInSingleProcess) {
    DistributedConfig cfg;
    cfg.world_size = 1;
    cfg.rank = -1;

    DistributedTrainer trainer(cfg);
    EXPECT_FALSE(trainer.initialize());
}

TEST(DistributedTrainerGuardFocusedTest, InitializeFailsForRankEqualWorldSize) {
    DistributedConfig cfg;
    cfg.world_size = 4;
    cfg.rank = 4;

    DistributedTrainer trainer(cfg);
    EXPECT_FALSE(trainer.initialize());
}

TEST(DistributedTrainerGuardFocusedTest, InitializeSucceedsForValidSingleProcessConfig) {
    DistributedConfig cfg;
    cfg.world_size = 1;
    cfg.rank = 0;

    DistributedTrainer trainer(cfg);
    EXPECT_TRUE(trainer.initialize());
}

TEST(DistributedTrainerGuardFocusedTest, SynchronizeGradientsFailsWithoutAllReduceInMultiRankMode) {
    DistributedConfig cfg;
    cfg.world_size = 2;
    cfg.rank = 0;

    DistributedTrainer trainer(cfg);
    std::vector<Tensor*> gradients;

    EXPECT_FALSE(trainer.synchronize_gradients(gradients));
}

TEST(DistributedTrainerGuardFocusedTest, BroadcastParametersFailsWithoutBroadcastFnInMultiRankMode) {
    DistributedConfig cfg;
    cfg.world_size = 2;
    cfg.rank = 1;

    DistributedTrainer trainer(cfg);
    std::vector<Tensor*> parameters;

    EXPECT_FALSE(trainer.broadcast_parameters(parameters));
}
} } } // namespace themis::llm::lora
