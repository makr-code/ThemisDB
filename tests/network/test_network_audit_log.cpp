// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

// Include directly so this unit also guards standalone header compilation.
#include "network/network_audit_log.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>

namespace {

using themis::network::AuditEvent;
using themis::network::NetworkAuditLog;

TEST(NetworkAuditLogTest, DefaultConstructorUsesDefaultCapacityAndCallback) {
    std::size_t callback_count = 0;
    NetworkAuditLog log;
    log.setEventCallback([&callback_count](const AuditEvent&) {
        ++callback_count;
    });

    for (std::size_t id = 0; id < 10'000; ++id) {
        log.recordConnectionOpen("127.0.0.1", id);
    }

    EXPECT_EQ(log.getStats().current_size, 10'000u);
    EXPECT_EQ(log.getStats().total_evicted, 0u);
    EXPECT_EQ(callback_count, 10'000u);

    log.recordConnectionOpen("127.0.0.1", 10'000);

    const auto stats = log.getStats();
    EXPECT_EQ(stats.total_recorded, 10'001u);
    EXPECT_EQ(stats.current_size, 10'000u);
    EXPECT_EQ(stats.total_evicted, 1u);
    EXPECT_EQ(callback_count, 10'001u);

    const auto events = log.getRecentEvents();
    ASSERT_EQ(events.size(), 10'000u);
    EXPECT_EQ(events.front().connection_id, 1u);
    EXPECT_EQ(events.back().connection_id, 10'000u);
}

TEST(NetworkAuditLogTest, ExplicitConfigUsesCustomCapacityAndDisablesCallback) {
    NetworkAuditLog::Config config;
    config.max_entries = 2;
    config.enable_callback = false;
    std::size_t callback_count = 0;
    NetworkAuditLog log(config);
    log.setEventCallback([&callback_count](const AuditEvent&) {
        ++callback_count;
    });

    for (std::size_t id = 0; id < 3; ++id) {
        log.recordConnectionOpen("127.0.0.1", id);
    }

    const auto stats = log.getStats();
    EXPECT_EQ(stats.total_recorded, 3u);
    EXPECT_EQ(stats.current_size, 2u);
    EXPECT_EQ(stats.total_evicted, 1u);
    EXPECT_EQ(callback_count, 0u);

    const auto events = log.getRecentEvents();
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events.front().connection_id, 1u);
    EXPECT_EQ(events.back().connection_id, 2u);
}

TEST(NetworkAuditLogTest, ExplicitConfigRejectsZeroCapacity) {
    NetworkAuditLog::Config config;
    config.max_entries = 0;

    EXPECT_THROW({ NetworkAuditLog log(config); }, std::invalid_argument);
}

TEST(NetworkAuditLogTest, ExplicitConfigAcceptsSingleEntryCapacity) {
    NetworkAuditLog::Config config;
    config.max_entries = 1;
    NetworkAuditLog log(config);

    log.recordConnectionOpen("127.0.0.1", 1);
    EXPECT_EQ(log.getStats().total_evicted, 0u);
    log.recordConnectionOpen("127.0.0.1", 2);

    const auto stats = log.getStats();
    EXPECT_EQ(stats.total_recorded, 2u);
    EXPECT_EQ(stats.current_size, 1u);
    EXPECT_EQ(stats.total_evicted, 1u);

    const auto events = log.getRecentEvents();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events.front().connection_id, 2u);
}

} // namespace
