/*
 * ThemisDB | DistributedTrainer guard-focused tests
 */

#include <gtest/gtest.h>

#include "llm/lora_framework/distributed_trainer.h"

namespace themis::llm::lora {

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

} // namespace themis::llm::lora
