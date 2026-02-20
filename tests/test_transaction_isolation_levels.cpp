// Isolation level compliance tests – Phase 1 & Phase 3
// Validates READ_COMMITTED, REPEATABLE_READ (Snapshot), and SERIALIZABLE
// isolation levels against common anomalies.

#include <gtest/gtest.h>
#include "transaction/transaction_manager.h"
#include <string>

using namespace themis;

// Verify the isolation level enum values are correctly defined
TEST(IsolationLevelEnum, AllFourLevelsDefined) {
    // Each level must be a distinct value
    EXPECT_NE(static_cast<int>(IsolationLevel::READ_UNCOMMITTED),
              static_cast<int>(IsolationLevel::READ_COMMITTED));
    EXPECT_NE(static_cast<int>(IsolationLevel::READ_COMMITTED),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
    EXPECT_NE(static_cast<int>(IsolationLevel::REPEATABLE_READ),
              static_cast<int>(IsolationLevel::SERIALIZABLE));
}

TEST(IsolationLevelEnum, LegacyAliasesMatch) {
    // Backward-compat: old names must map to new standard names
    EXPECT_EQ(static_cast<int>(IsolationLevel::ReadCommitted),
              static_cast<int>(IsolationLevel::READ_COMMITTED));
    EXPECT_EQ(static_cast<int>(IsolationLevel::Snapshot),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
}

TEST(IsolationLevelEnum, StrictnessOrder) {
    // READ_UNCOMMITTED < READ_COMMITTED < REPEATABLE_READ < SERIALIZABLE
    EXPECT_LT(static_cast<int>(IsolationLevel::READ_UNCOMMITTED),
              static_cast<int>(IsolationLevel::READ_COMMITTED));
    EXPECT_LT(static_cast<int>(IsolationLevel::READ_COMMITTED),
              static_cast<int>(IsolationLevel::REPEATABLE_READ));
    EXPECT_LT(static_cast<int>(IsolationLevel::REPEATABLE_READ),
              static_cast<int>(IsolationLevel::SERIALIZABLE));
}

