/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_sampling_strategy.cpp                         ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:04:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     89                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 9cb3159dc  2026-02-21  Utils Module – Production Readiness (Phases 1–8) (#1344) ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "utils/tracing.h"

using namespace themis;

TEST(SamplingStrategy, AlwaysOnType) {
    EXPECT_EQ(SamplingStrategy::alwaysOn().type(), SamplingStrategy::Type::ALWAYS_ON);
}

TEST(SamplingStrategy, AlwaysOffType) {
    EXPECT_EQ(SamplingStrategy::alwaysOff().type(), SamplingStrategy::Type::ALWAYS_OFF);
}

TEST(SamplingStrategy, ProbabilityType) {
    auto s = SamplingStrategy::probability(0.42);
    EXPECT_EQ(s.type(), SamplingStrategy::Type::PROBABILITY);
    EXPECT_DOUBLE_EQ(s.probability(), 0.42);
}

TEST(SamplingStrategy, ParentBasedType) {
    auto s = SamplingStrategy::parentBased(0.8);
    EXPECT_EQ(s.type(), SamplingStrategy::Type::PARENT_BASED);
    EXPECT_DOUBLE_EQ(s.probability(), 0.8);
}

TEST(SamplingStrategy, AlwaysOnShouldSample) {
    auto s = SamplingStrategy::alwaysOn();
    EXPECT_TRUE(s.shouldSample());
    EXPECT_TRUE(s.shouldSample(false));
}

TEST(SamplingStrategy, AlwaysOffShouldNotSample) {
    auto s = SamplingStrategy::alwaysOff();
    EXPECT_FALSE(s.shouldSample());
    EXPECT_FALSE(s.shouldSample(true));
}

TEST(SamplingStrategy, ProbabilityOneAlwaysSamples) {
    auto s = SamplingStrategy::probability(1.0);
    for (int i = 0; i < 10; ++i) EXPECT_TRUE(s.shouldSample());
}

TEST(SamplingStrategy, ProbabilityZeroNeverSamples) {
    auto s = SamplingStrategy::probability(0.0);
    for (int i = 0; i < 10; ++i) EXPECT_FALSE(s.shouldSample());
}

TEST(SamplingStrategy, ParentBasedSamplesWhenParentSampled) {
    auto s = SamplingStrategy::parentBased(0.0); // root never
    for (int i = 0; i < 20; ++i) EXPECT_TRUE(s.shouldSample(/*parent_sampled=*/true));
}

TEST(SamplingStrategy, ParentBasedDoesNotSampleRootWhenProbabilityZero) {
    auto s = SamplingStrategy::parentBased(0.0);
    for (int i = 0; i < 20; ++i) EXPECT_FALSE(s.shouldSample(/*parent_sampled=*/false));
}

TEST(SamplingStrategy, DefaultConstructorIsAlwaysOn) {
    SamplingStrategy s;
    EXPECT_EQ(s.type(), SamplingStrategy::Type::ALWAYS_ON);
    EXPECT_TRUE(s.shouldSample());
}

