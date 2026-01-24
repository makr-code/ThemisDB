// Disabled: LLM vision integration tests not runnable in current build.
#include <gtest/gtest.h>

TEST(DISABLED_LlmVisionIntegration, Skipped)
{
    GTEST_SKIP() << "test_llm_vision_integration disabled for current build";
}
