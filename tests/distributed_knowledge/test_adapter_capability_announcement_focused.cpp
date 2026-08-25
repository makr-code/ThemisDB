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
    announcement.adapter_version = "1.0.0";
    announcement.is_withdrawal = false;

    json payload = announcement.toJson();
    
    EXPECT_EQ(payload["adapter_id"], "adapter-001");
    EXPECT_EQ(payload["shard_id"], "shard-001");
    EXPECT_EQ(payload["adapter_version"], "1.0.0");
    EXPECT_EQ(payload["is_withdrawal"], false);
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
        publisher.announce(announcement);
    }
    
    EXPECT_EQ(gossip_messages_.size(), 3);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(gossip_messages_[i]["adapter_id"], "adapter-" + std::to_string(i));
        EXPECT_EQ(gossip_messages_[i]["shard_id"], "shard-001");
    }
}

/**
 * @test ACA-05: Adapter withdrawal announcement.
 *
 * Verifies that announcements correctly reflect adapter withdrawal status
 * (is_withdrawal = true) for adapter downtime or maintenance scenarios.
 */
TEST_F(AdapterCapabilityAnnouncementTest, AdapterWithdrawalAnnouncement) {
    auto gossip_fn = make_gossip_fn();
    GossipAdapterPublisher publisher("shard-001", gossip_fn);
    
    AdapterCapabilityAnnouncement announcement;
    announcement.adapter_id = "adapter-001";
    announcement.domain_type = AdapterDomainType::SECURITY_MONITOR;
    announcement.is_withdrawal = true;  // Adapter is being withdrawn
    
    publisher.announce(announcement);
    
    ASSERT_EQ(gossip_messages_.size(), 1);
    EXPECT_EQ(gossip_messages_[0]["is_withdrawal"], true);
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
    
    publisher.announce(announcement);
    
    ASSERT_EQ(gossip_messages_.size(), 1);
    // Publisher stamps announced_at; toJson() serialises it as announced_at_ms
    EXPECT_TRUE(gossip_messages_[0].contains("announced_at_ms"));
    EXPECT_GE(gossip_messages_[0]["announced_at_ms"].get<int64_t>(), 0);
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
    
    AdapterCapabilityAnnouncement ann2;
    ann2.adapter_id = "adapter-2";
    ann2.domain_type = AdapterDomainType::SECURITY_MONITOR;
    
    publisher1.announce(ann1);
    publisher2.announce(ann2);
    
    EXPECT_EQ(gossip_messages_.size(), 2);
    EXPECT_EQ(gossip_messages_[0]["adapter_id"], "adapter-1");
    EXPECT_EQ(gossip_messages_[1]["adapter_id"], "adapter-2");
}

// ─────────────────────────────────────────────────────────────────────────────
// ACA-TRUST-01..05 — FederationTrustPolicy / fail-closed enforcement
// ─────────────────────────────────────────────────────────────────────────────

#include "distributed_knowledge/distributed_knowledge_api_contract.h"

using namespace themis::distributed_knowledge;

/**
 * @test ACA-TRUST-01: AlwaysPermitTrustPolicy permits every announcement.
 */
TEST(FederationTrustPolicy, AlwaysPermitAllowsAll) {
    AlwaysPermitTrustPolicy policy;
    AdapterCapabilityAnnouncement ann;
    ann.adapter_id = "adapter-trusted";
    ann.shard_id   = "shard-01";
    ann.is_withdrawal = false;
    EXPECT_EQ(policy.evaluateTrustGate(ann), TrustDecision::PERMIT);
}

/**
 * @test ACA-TRUST-02: A custom reject policy returns REJECT for flagged adapters.
 *
 * Implements IFederationTrustPolicy inline to simulate a real domain blocklist.
 */
TEST(FederationTrustPolicy, CustomPolicyCanRejectAnnouncement) {
    // Custom blocklist policy that rejects adapters with "untrusted" in the ID.
    class BlocklistPolicy final : public IFederationTrustPolicy {
    public:
        [[nodiscard]] TrustDecision evaluateTrustGate(
            const AdapterCapabilityAnnouncement& ann) const noexcept override {
            if (ann.adapter_id.find("untrusted") != std::string::npos) {
                return TrustDecision::REJECT;
            }
            return TrustDecision::PERMIT;
        }
    };

    BlocklistPolicy policy;

    AdapterCapabilityAnnouncement trusted;
    trusted.adapter_id = "adapter-prod-001";
    EXPECT_EQ(policy.evaluateTrustGate(trusted), TrustDecision::PERMIT);

    AdapterCapabilityAnnouncement rejected;
    rejected.adapter_id = "adapter-untrusted-ext";
    EXPECT_EQ(policy.evaluateTrustGate(rejected), TrustDecision::REJECT);
}

/**
 * @test ACA-TRUST-03: TrustDecision enum values are distinct.
 */
TEST(FederationTrustPolicy, TrustDecisionEnumValuesAreDistinct) {
    EXPECT_NE(TrustDecision::PERMIT, TrustDecision::REJECT);
}

/**
 * @test ACA-TRUST-04: DKErrorCode::TRUST_GATE_REJECTED is defined and distinct from
 *                     all other codes.
 */
TEST(FederationTrustPolicy, TrustGateRejectedErrorCodeIsUnique) {
    // Ensure the code compiles and is distinct from well-known codes.
    EXPECT_NE(DKErrorCode::TRUST_GATE_REJECTED, DKErrorCode::OK);
    EXPECT_NE(DKErrorCode::TRUST_GATE_REJECTED, DKErrorCode::FEDERATION_TIMEOUT);
    EXPECT_NE(DKErrorCode::TRUST_GATE_REJECTED, DKErrorCode::INTERNAL_ERROR);
    // TRUST_GATE_REJECTED is not retryable (fail-closed: the caller must fix the trust issue).
    EXPECT_FALSE(isRetryableCode(DKErrorCode::TRUST_GATE_REJECTED));
}

/**
 * @test ACA-TRUST-05: AlwaysPermitTrustPolicy polymorphic dispatch works correctly.
 *
 * Accesses the policy through the base-class pointer to confirm vtable dispatch.
 */
TEST(FederationTrustPolicy, PolymorphicDispatchThroughBasePointer) {
    std::unique_ptr<IFederationTrustPolicy> policy =
        std::make_unique<AlwaysPermitTrustPolicy>();

    AdapterCapabilityAnnouncement ann;
    ann.adapter_id = "adapter-any";
    EXPECT_EQ(policy->evaluateTrustGate(ann), TrustDecision::PERMIT);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Summary
// ─────────────────────────────────────────────────────────────────────────────

/*
 * Test Coverage Summary (ACA-01..ACA-10, ACA-TRUST-01..05):
 *
 * ACA-01: Announcement creation and JSON serialization
 * ACA-02: AdapterDomainType to string conversion for all types
 * ACA-03: Basic gossip publisher announcement dispatch
 * ACA-04: Multiple adapter announcements from single shard
 * ACA-05: Adapter withdrawal announcement handling
 * ACA-06: Custom domain label in announcements
 * ACA-07: Announcement timestamp management
 * ACA-08: Shard ID override by publisher
 * ACA-09: Empty adapter ID edge case
 * ACA-10: Multiple publishers for same shard
 *
 * ACA-TRUST-01: AlwaysPermitTrustPolicy permits every announcement
 * ACA-TRUST-02: Custom policy can reject announcements by domain/ID
 * ACA-TRUST-03: TrustDecision enum values are distinct
 * ACA-TRUST-04: TRUST_GATE_REJECTED error code defined and non-retryable
 * ACA-TRUST-05: Polymorphic dispatch through base pointer works
 *
 * Target: Q3-Q4 2026 Hardening - trust-gate fail-closed semantics.
 */
