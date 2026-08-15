// Quick compile check
#define GTEST_NAMESPACE testing
namespace testing { class Test { protected: void SetUp() {} void TearDown() {} }; }
#include "test_analytics_concurrency_safety_focused.cpp"
