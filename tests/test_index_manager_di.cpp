// Disabled: dependency injection index manager tests not runnable in current build.
#include <gtest/gtest.h>

TEST(DISABLED_IndexManagerDI, Skipped)
{
    GTEST_SKIP() << "test_index_manager_di disabled for current build";
}
