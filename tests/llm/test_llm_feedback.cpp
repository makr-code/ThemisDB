// Disabled: LLM feedback tests not runnable in current build.
#include <gtest/gtest.h>

TEST(DISABLED_LlmFeedback, Skipped)
{
    GTEST_SKIP() << "test_llm_feedback disabled for current build";
}
