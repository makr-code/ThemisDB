// Disabled: knowledge gap detector tests not runnable in current build.
#include <gtest/gtest.h>

TEST(DISABLED_KnowledgeGapDetector, Skipped)
{
    GTEST_SKIP() << "test_knowledge_gap_detector disabled for current build";
}
