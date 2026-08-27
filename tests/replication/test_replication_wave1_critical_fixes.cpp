/**
 * @file test_replication_wave1_critical_fixes.cpp
 * @brief Contract tests for Wave1 replication critical-fix API availability.
 */

#include <gtest/gtest.h>

#include "replication/observability.h"
#include "replication/policy.h"
#include "replication/replication_manager.h"
#include "replication/logical_replication.h"

namespace themisdb::replication {

TEST(ReplicationWave1Contract, ObservabilityConfigAccessible) {
    ReplicationObserverConfig cfg;
    EXPECT_GT(cfg.critical_lag_threshold_ms, 0u);
}

TEST(ReplicationWave1Contract, ReplicationConfigConstructible) {
    ReplicationConfig cfg;
    EXPECT_GE(cfg.replication_timeout_ms, 1u);
}

TEST(ReplicationWave1Contract, LogicalReplicationConfigConstructible) {
    LogicalReplicationManager::Config cfg;
    EXPECT_GE(cfg.file_io_timeout_ms, 0u);
}

}  // namespace themisdb::replication
