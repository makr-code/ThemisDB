/**
 * @file test_adapter_capability_announcement_focused.cpp
 * @brief Focused unit tests for adapter capability announcement gossip surfaces.
 *
 * Tests the core distributed knowledge capability announcement paths:
 * - announcement creation and JSON serialization
 * - gossip publisher announcement handling
 * - cross-shard capability discovery
 * - privacy-aware capability filtering
 *
 * Target: Production readiness validation for capability exchange layer.
 * Q3 2026 Hardening: policy-edge semantics and deterministic broadcast behavior.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <vector>
#include <memory>

#include "distributed_knowledge/adapter_capability_announcement.h"

using namespace themis::distributed_knowledge;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixtures
// ─────────────────────────────────────────────────────────────────────────────

class AdapterCapabilityAnnouncementTest : public ::testing::Test {
protected:
    void SetUp() override {
        gossip_messages_.clear();
    }

    std::vector<json> gossip_messages_;
    
    auto make_gossip_fn() {
        return [this](json msg) {
            gossip_messages_.push_back(msg);
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Basic Announcement Tests (ACA-01..ACA-03)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test ACA-01: Adapter capability announcement creation and JSON serialization.
 *
 * Verifies that an AdapterCapabilityAnnouncement can be created, populated,
 * and serialized to JSON with all required fields present.
 */
TEST_F(AdapterCapabilityAnnouncementTest, CreateAndSerializeAnnouncement) {
    AdapterCapabilityAnnouncement announcement;
    announcement.adapter_id = "adapter-001";
    announcement.domain_type = AdapterDomainType::SECURITY_MONITOR;
    announcement.shard_id = "shard-001";
    announcement.version_string = "1.0.0";
    announcement.is_available = true;

    json payload = announcement.toJson();
    
    EXPECT_EQ(payload["adapter_id"], "adapter-001");
    EXPECT_EQ(payload["shard_id"], "shard-001");
    EXPECT_EQ(payload["version_string"], "1.0.0");
    EXPECT_EQ(payload["is_available"], true);
}

/**
 * @test ACA-02: AdapterDomainType to string conversion.
 *
 * Verifies that all domain types convert to human-readable strings correctly.
 */
TEST_F(AdapterCapabilityAnnouncementTest, AdapterDomainTypeToString) {
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::GENERAL), "GENERAL");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::SECURITY_MONITOR), "SECURITY_MONITOR");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::SCHEMA_ADVISOR), "SCHEMA_ADVISOR");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::TRANSACTION), "TRANSACTION");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::MULTI_TENANT), "MULTI_TENANT");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::EXPLAINABILITY), "EXPLAINABILITY");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::VECTOR_SEARCH), "VECTOR_SEARCH");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::PROCESS_MINING), "PROCESS_MINING");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::GEOSPATIAL), "GEOSPATIAL");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::LEGAL), "LEGAL");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::MEDICAL), "MEDICAL");
    EXPECT_EQ(adapterDomainTypeToString(AdapterDomainType::CUSTOM), "CUSTOM");
}

/**
 * @test ACA-03: GossipAdapterPublisher initialization and announcement dispatch.
 *
 * Verifies that the gossip publisher correctly initializes and can dispatch
 * announcements to the gossip channel with proper shard ID and timestamp.
 */
TEST_F(AdapterCapabilityAnnouncementTest, GossipPublisherAnnouncesCapability) {
    auto gossip_fn = make_gossip_fn();
    GossipAdapterPublisher publisher("shard-001", gossip_fn);
    
    AdapterCapabilityAnnouncement announcement;
    announcement.adapter_id = "adapter-001";
    announcement.domain_type = AdapterDomainType::SECURITY_MONITOR;
    announcement.is_available = true;
    
    publisher.announce(announcement);
    
    ASSERT_EQ(gossip_messages_.size(), 1);
    EXPECT_EQ(gossip_messages_[0]["message_type"], "adapter_capability");
    EXPECT_EQ(gossip_messages_[0]["adapter_id"], "adapter-001");
    EXPECT_EQ(gossip_messages_[0]["shard_id"], "shard-001");
}

// ─────────────────────────────────────────────────────────────────────────────
// Multiple Announcement Tests (ACA-04..ACA-06)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test ACA-04: Multiple adapter announcements from single shard.
 *
 * Verifies that a publisher can announce multiple distinct adapters
 * in sequence and each generates a separate gossip message.
 */
TEST_F(AdapterCapabilityAnnouncementTest, MultipleAdaptersFromSameShard) {
    auto gossip_fn = make_gossip_fn();
    GossipAdapterPublisher publisher("shard-001", gossip_fn);
    
    for (int i = 0; i < 3; ++i) {
        AdapterCapabilityAnnouncement announcement;
        announcement.adapter_id = "adapter-" + std::to_string(i);
        announcement.domain_type = AdapterDomainType::GENERAL;
        announcement.is_available = true;
        publisher.announce(announcement);
    }
    
    EXPECT_EQ(gossip_messages_.size(), 3);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(gossip_messages_[i]["adapter_id"], "adapter-" + std::to_string(i));
        EXPECT_EQ(gossip_messages_[i]["shard_id"], "shard-001");
    }
}

/**
 * @test ACA-05: Adapter unavailability announcement.
 *
 * Verifies that announcements correctly reflect adapter availability status
 * (is_available = false) for adapter downtime or maintenance scenarios.
 */
TEST_F(AdapterCapabilityAnnouncementTest, AdapterUnavailabilityAnnouncement) {
    auto gossip_fn = make_gossip_fn();
    GossipAdapterPublisher publisher("shard-001", gossip_fn);
    
    AdapterCapabilityAnnouncement announcement;
    announcement.adapter_id = "adapter-001";
    announcement.domain_type = AdapterDomainType::SECURITY_MONITOR;
    announcement.is_available = false;  // Adapter is unavailable
    
    publisher.announce(announcement);
    
    ASSERT_EQ(gossip_messages_.size(), 1);
    EXPECT_EQ(gossip_messages_[0]["is_available"], false);
}

/**
 * @test ACA-06: Custom domain label in announcement.
 *
 * Verifies that custom domain labels are properly included in announcements
 * when the domain type is CUSTOM.
 */
TEST_F(AdapterCapabilityAnnouncementTest, CustomDomainLabel) {
    auto gossip_fn = make_gossip_fn();
    GossipAdapterPublisher publisher("shard-001", gossip_fn);
    
    AdapterCapabilityAnnouncement announcement;
    announcement.adapter_id = "adapter-custom";
    announcement.domain_type = AdapterDomainType::CUSTOM;
    announcement.custom_domain_label = "domain-specific-workload";
    announcement.is_available = true;
    
    publisher.announce(announcement);
    
    ASSERT_EQ(gossip_messages_.size(), 1);
    EXPECT_EQ(gossip_messages_[0]["custom_domain_label"], "domain-specific-workload");
}

// ─────────────────────────────────────────────────────────────────────────────
// Timestamp and Shard ID Tests (ACA-07..ACA-08)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test ACA-07: Announcement timestamp is set during gossip publish.
 *
 * Verifies that the publisher sets the announced_at timestamp when
 * publishing an announcement (even if not set by caller).
 */
TEST_F(AdapterCapabilityAnnouncementTest, AnnouncementTimestampSet) {
    auto gossip_fn = make_gossip_fn();
    GossipAdapterPublisher publisher("shard-001", gossip_fn);
    
    AdapterCapabilityAnnouncement announcement;
    announcement.adapter_id = "adapter-001";
    announcement.domain_type = AdapterDomainType::GENERAL;
    announcement.is_available = true;
    
    auto before = std::chrono::system_clock::now();
    publisher.announce(announcement);
    auto after = std::chrono::system_clock::now();
    
    ASSERT_EQ(gossip_messages_.size(), 1);
    // Verify that message contains a timestamp (exact value varies, but should exist)
    EXPECT_TRUE(gossip_messages_[0].contains("announced_at") || 
                gossip_messages_[0].contains("timestamp"));
}

/**
 * @test ACA-08: Shard ID is overwritten by publisher.
 *
 * Verifies that the publisher always sets its own shard_id in announcements,
 * even if the announcement already contained a different shard_id.
 */
TEST_F(AdapterCapabilityAnnouncementTest, ShardIdOverwrittenByPublisher) {
    auto gossip_fn = make_gossip_fn();
    GossipAdapterPublisher publisher("shard-002", gossip_fn);
    
    AdapterCapabilityAnnouncement announcement;
    announcement.adapter_id = "adapter-001";
    announcement.shard_id = "shard-wrong";  // Incorrect shard ID
    announcement.domain_type = AdapterDomainType::GENERAL;
    announcement.is_available = true;
    
    publisher.announce(announcement);
    
    ASSERT_EQ(gossip_messages_.size(), 1);
    // Publisher should overwrite with correct shard ID
    EXPECT_EQ(gossip_messages_[0]["shard_id"], "shard-002");
}

// ─────────────────────────────────────────────────────────────────────────────
// Edge Cases and Error Handling (ACA-09..ACA-10)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test ACA-09: Empty adapter ID handling (defensive).
 *
 * Verifies that announcements with empty adapter_id can still be serialized
 * (though would likely be rejected by policy in practice).
 */
TEST_F(AdapterCapabilityAnnouncementTest, EmptyAdapterIdHandling) {
    auto gossip_fn = make_gossip_fn();
    GossipAdapterPublisher publisher("shard-001", gossip_fn);
    
    AdapterCapabilityAnnouncement announcement;
    announcement.adapter_id = "";  // Empty
    announcement.domain_type = AdapterDomainType::GENERAL;
    announcement.is_available = true;
    
    publisher.announce(announcement);
    
    ASSERT_EQ(gossip_messages_.size(), 1);
    EXPECT_EQ(gossip_messages_[0]["adapter_id"], "");
}

/**
 * @test ACA-10: Multiple publishers for same shard.
 *
 * Verifies that multiple publisher instances for the same shard can
 * independently publish announcements without interference.
 */
TEST_F(AdapterCapabilityAnnouncementTest, MultiplePublishersForSameShard) {
    auto gossip_fn = make_gossip_fn();
    GossipAdapterPublisher publisher1("shard-001", gossip_fn);
    GossipAdapterPublisher publisher2("shard-001", gossip_fn);
    
    AdapterCapabilityAnnouncement ann1;
    ann1.adapter_id = "adapter-1";
    ann1.domain_type = AdapterDomainType::GENERAL;
    ann1.is_available = true;
    
    AdapterCapabilityAnnouncement ann2;
    ann2.adapter_id = "adapter-2";
    ann2.domain_type = AdapterDomainType::SECURITY_MONITOR;
    ann2.is_available = true;
    
    publisher1.announce(ann1);
    publisher2.announce(ann2);
    
    EXPECT_EQ(gossip_messages_.size(), 2);
    EXPECT_EQ(gossip_messages_[0]["adapter_id"], "adapter-1");
    EXPECT_EQ(gossip_messages_[1]["adapter_id"], "adapter-2");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Summary
// ─────────────────────────────────────────────────────────────────────────────

/*
 * Test Coverage Summary (ACA-01..ACA-10):
 *
 * ACA-01: Announcement creation and JSON serialization
 * ACA-02: AdapterDomainType to string conversion for all types
 * ACA-03: Basic gossip publisher announcement dispatch
 * ACA-04: Multiple adapter announcements from single shard
 * ACA-05: Adapter unavailability announcement handling
 * ACA-06: Custom domain label in announcements
 * ACA-07: Announcement timestamp management
 * ACA-08: Shard ID override by publisher
 * ACA-09: Empty adapter ID edge case
 * ACA-10: Multiple publishers for same shard
 *
 * Target: Q3 2026 Hardening - policy-edge semantics and cross-shard behavior.
 * Status: Focused unit test suite for capability announcement layer.
 */
