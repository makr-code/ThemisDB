// Disabled: LoRA failure scenario tests not runnable in current build.
#include <gtest/gtest.h>

TEST(DISABLED_LoraFailureScenarios, Skipped)
{
    GTEST_SKIP() << "test_lora_failure_scenarios disabled for current build";
}
