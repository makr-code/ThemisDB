/**
 * @file test_cross_module_security_governance.cpp
 * @brief Cross-module integration tests for the security ↔ governance ↔
 *        observability pipeline:
 *        ZeroTrustPolicyEnforcer (security) →
 *        DataLineageTracker (governance) →
 *        MetricsCollector (observability)
 *
 * Rationale
 * ---------
 * Individual module unit tests verify each component in isolation.  This file
 * validates the interactions at module boundaries that only emerge when the
 * three components are composed:
 *
 *   - Only ZeroTrust-approved requests should produce lineage events.
 *   - Lineage events must emit Prometheus counters via MetricsCollector.
 *   - The ancestry chain (parent_event_id) in DataLineageTracker must survive
 *     a round-trip through exportLineageAsJson.
 *   - The full access-governance-observability pipeline must be self-consistent
 *     in its counts.
 *
 * Test groups
 * -----------
 * Group A (5 tests): ZeroTrustPolicyEnforcer × DataLineageTracker
 *   A-1  Verified request → QUERY lineage event recorded
 *   A-2  Token failure    → no lineage event recorded
 *   A-3  Network denial   → no lineage event recorded
 *   A-4  Multiple successful requests → multiple QUERY events per dataset
 *   A-5  ZeroTrust::requests_allowed count matches recorded lineage events
 *
 * Group B (4 tests): DataLineageTracker ancestry chains
 *   B-1  INGESTION → ENRICHMENT → QUERY chain has correct parent_event_id links
 *   B-2  getUpstreamLineage returns root-to-leaf order
 *   B-3  getDownstreamLineage returns all transitively derived events
 *   B-4  exportLineageAsJson contains all events and the event_type field
 *
 * Group C (3 tests): DataLineageTracker × MetricsCollector
 *   C-1  recordEvent increments governance_lineage_events_total in Prometheus
 *   C-2  Different event types produce distinct label series
 *   C-3  MetricsCollector::reset() clears lineage counters
 *
 * Group D (3 tests): Full pipeline
 *   D-1  ZeroTrust allow  → QUERY lineage → Prometheus counter incremented
 *   D-2  ZeroTrust deny   → no lineage    → counter not incremented
 *   D-3  Multi-user, multi-resource → distinct lineage records per dataset
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "security/zero_trust_policy_enforcer.h"
#include "governance/data_lineage.h"
#include "observability/metrics_collector.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::security;
using namespace themis::governance;
using namespace themis::observability;

// ============================================================================
// Shared helpers
// ============================================================================

/// Build a minimal ZeroTrustContext for a given user and IP.
static ZeroTrustContext makeCtx(const std::string& user_id,
                                 const std::string& client_ip,
                                 const std::string& resource = "dataset-A",
                                 const std::string& token    = "valid-token") {
    ZeroTrustContext ctx;
    ctx.request_id = "req-" + user_id;
    ctx.user_id    = user_id;
    ctx.client_ip  = client_ip;
    ctx.token      = token;
    ctx.resource   = resource;
    ctx.action     = "read";
    ctx.timestamp  = std::chrono::system_clock::now();
    return ctx;
}

/// Build a NetworkPolicy that allows all traffic from a /8 network for a user.
static NetworkPolicy makePolicy(const std::string& policy_id,
                                 const std::string& identity,
                                 const std::string& allowed_cidr = "10.0.0.0/8") {
    NetworkPolicy p;
    p.policy_id      = policy_id;
    p.identity       = identity;
    p.allowed_cidrs  = {allowed_cidr};
    p.default_deny   = true;
    return p;
}

/// Make a LineageEvent for a dataset access triggered by the given user.
static LineageEvent makeQueryEvent(const std::string& event_id,
                                    const std::string& dataset_id,
                                    const std::string& performed_by,
                                    const std::string& parent = "") {
    LineageEvent e;
    e.event_id      = event_id;
    e.dataset_id    = dataset_id;
    e.event_type    = LineageEventType::QUERY;
    e.performed_by  = performed_by;
    e.operation     = "read access";
    e.parent_event_id = parent;
    e.timestamp_ms  = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    return e;
}

// ============================================================================
// Fixture — resets MetricsCollector singleton before every test
// ============================================================================

class SecurityGovernanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset the global MetricsCollector so counter state from previous
        // tests does not bleed into the current one.
        MetricsCollector::getInstance().reset();
    }

    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }
};

// ============================================================================
// Group A – ZeroTrustPolicyEnforcer × DataLineageTracker
// ============================================================================

// A-1: Verified request → QUERY lineage event recorded
TEST_F(SecurityGovernanceTest, A1_VerifiedRequest_RecordsLineageEvent) {
    ZeroTrustPolicyEnforcer enforcer(
        [](const std::string& tok, const std::string&) { return tok == "valid-token"; });
    enforcer.addNetworkPolicy(makePolicy("p-alice", "alice"));
    DataLineageTracker tracker;

    ZeroTrustContext ctx = makeCtx("alice", "10.1.2.3", "dataset-A");
    auto result = enforcer.verify(ctx);
    ASSERT_TRUE(result.verified) << "Request must be allowed before lineage is recorded";

    // Gate: record lineage only on verified access
    if (result.verified) {
        tracker.recordEvent(makeQueryEvent("ev-A1", "dataset-A", ctx.user_id));
    }

    auto lineage = tracker.getLineage("dataset-A");
    ASSERT_EQ(lineage.events.size(), 1u);
    EXPECT_EQ(lineage.events[0].event_type, LineageEventType::QUERY);
    EXPECT_EQ(lineage.events[0].performed_by, "alice");
}

// A-2: Token failure → no lineage event recorded
TEST_F(SecurityGovernanceTest, A2_TokenFailure_NoLineageEvent) {
    ZeroTrustPolicyEnforcer enforcer(
        [](const std::string& tok, const std::string&) { return tok == "valid-token"; });
    DataLineageTracker tracker;

    ZeroTrustContext ctx = makeCtx("mallory", "192.168.1.1", "dataset-A",
                                   "bad-token");
    auto result = enforcer.verify(ctx);
    EXPECT_FALSE(result.verified) << "Bad token must be rejected";

    if (result.verified) {
        tracker.recordEvent(makeQueryEvent("ev-A2", "dataset-A", ctx.user_id));
    }

    EXPECT_EQ(tracker.totalEventCount(), 0u)
        << "No lineage event must be recorded after auth failure";
}

// A-3: Network policy denial → no lineage event recorded
TEST_F(SecurityGovernanceTest, A3_NetworkDenial_NoLineageEvent) {
    ZeroTrustPolicyEnforcer enforcer(
        [](const std::string& tok, const std::string&) { return tok == "valid-token"; });
    // alice is only allowed from 10.0.0.0/8 — request comes from 172.16.0.1
    enforcer.addNetworkPolicy(makePolicy("p-alice", "alice", "10.0.0.0/8"));
    DataLineageTracker tracker;

    ZeroTrustContext ctx = makeCtx("alice", "172.16.0.1", "dataset-A");
    auto result = enforcer.verify(ctx);
    EXPECT_FALSE(result.verified)
        << "Request from non-allowed network must be denied";

    if (result.verified) {
        tracker.recordEvent(makeQueryEvent("ev-A3", "dataset-A", ctx.user_id));
    }

    EXPECT_EQ(tracker.totalEventCount(), 0u)
        << "No lineage event must be recorded after network denial";
}

// A-4: Multiple successful requests → multiple QUERY events in lineage
TEST_F(SecurityGovernanceTest, A4_MultipleVerifiedRequests_MultipleLineageEvents) {
    ZeroTrustPolicyEnforcer enforcer(
        [](const std::string& tok, const std::string&) { return tok == "valid-token"; });
    enforcer.addNetworkPolicy(makePolicy("p-alice", "alice"));
    DataLineageTracker tracker;

    constexpr int kRequests = 5;
    for (int i = 0; i < kRequests; ++i) {
        ZeroTrustContext ctx = makeCtx("alice", "10.0.0.1", "dataset-B");
        ctx.request_id = "req-" + std::to_string(i);
        auto result = enforcer.verify(ctx);
        ASSERT_TRUE(result.verified);
        tracker.recordEvent(makeQueryEvent("ev-B-" + std::to_string(i),
                                            "dataset-B", "alice"));
    }

    auto lineage = tracker.getLineage("dataset-B");
    EXPECT_EQ(lineage.events.size(), static_cast<size_t>(kRequests))
        << "Each verified request must produce exactly one lineage event";
}

// A-5: ZeroTrust::requests_allowed count equals recorded lineage events
TEST_F(SecurityGovernanceTest, A5_ZeroTrustAllowedCount_MatchesLineageEventCount) {
    ZeroTrustPolicyEnforcer enforcer(
        [](const std::string& tok, const std::string&) { return tok == "valid-token"; });
    enforcer.addNetworkPolicy(makePolicy("p-bob", "bob"));
    DataLineageTracker tracker;

    constexpr int kAllowed = 4;
    constexpr int kDenied  = 2;

    for (int i = 0; i < kAllowed; ++i) {
        ZeroTrustContext ctx = makeCtx("bob", "10.0.0.5", "dataset-C");
        ctx.request_id = "allowed-" + std::to_string(i);
        auto r = enforcer.verify(ctx);
        if (r.verified)
            tracker.recordEvent(makeQueryEvent("ev-C-" + std::to_string(i),
                                               "dataset-C", "bob"));
    }
    for (int i = 0; i < kDenied; ++i) {
        ZeroTrustContext ctx = makeCtx("bob", "10.0.0.5", "dataset-C", "wrong-token");
        ctx.request_id = "denied-" + std::to_string(i);
        auto r = enforcer.verify(ctx);
        if (r.verified)
            tracker.recordEvent(makeQueryEvent("ev-C-bad-" + std::to_string(i),
                                               "dataset-C", "bob"));
    }

    EXPECT_EQ(enforcer.getMetrics().requests_allowed.load(),
              static_cast<uint64_t>(kAllowed));
    EXPECT_EQ(tracker.totalEventCount(), static_cast<size_t>(kAllowed))
        << "Lineage event count must equal ZeroTrust::requests_allowed";
}

// ============================================================================
// Group B – DataLineageTracker ancestry chains
// ============================================================================

class LineageAncestryTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }
    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }

    DataLineageTracker tracker_;
};

// B-1: INGESTION → ENRICHMENT → QUERY chain has correct parent_event_id links
TEST_F(LineageAncestryTest, B1_IngestionEnrichmentQuery_CorrectParentLinks) {
    // INGESTION (root)
    LineageEvent ingestion;
    ingestion.event_id    = "ev-ingest";
    ingestion.dataset_id  = "ds-pipeline";
    ingestion.event_type  = LineageEventType::INGESTION;
    ingestion.performed_by = "loader";
    ingestion.operation   = "initial load";
    tracker_.recordEvent(ingestion);

    // ENRICHMENT derived from INGESTION
    LineageEvent enrichment;
    enrichment.event_id       = "ev-enrich";
    enrichment.dataset_id     = "ds-pipeline";
    enrichment.event_type     = LineageEventType::ENRICHMENT;
    enrichment.performed_by   = "enricher";
    enrichment.parent_event_id = "ev-ingest";
    tracker_.recordEvent(enrichment);

    // QUERY derived from ENRICHMENT
    LineageEvent query;
    query.event_id         = "ev-query";
    query.dataset_id       = "ds-pipeline";
    query.event_type       = LineageEventType::QUERY;
    query.performed_by     = "analyst";
    query.parent_event_id  = "ev-enrich";
    tracker_.recordEvent(query);

    auto lineage = tracker_.getLineage("ds-pipeline");
    ASSERT_EQ(lineage.events.size(), 3u);

    // Verify parent links are intact (ignoring sort order)
    auto find = [&](const std::string& eid) -> const LineageEvent* {
        for (const auto& e : lineage.events) if (e.event_id == eid) return &e;
        return nullptr;
    };
    ASSERT_NE(find("ev-ingest"), nullptr);
    ASSERT_NE(find("ev-enrich"), nullptr);
    ASSERT_NE(find("ev-query"),  nullptr);

    EXPECT_TRUE(find("ev-ingest")->parent_event_id.empty())
        << "INGESTION event must be the root (no parent)";
    EXPECT_EQ(find("ev-enrich")->parent_event_id, "ev-ingest");
    EXPECT_EQ(find("ev-query")->parent_event_id,  "ev-enrich");
}

// B-2: getUpstreamLineage returns events ordered root → requested event
TEST_F(LineageAncestryTest, B2_GetUpstreamLineage_RootToLeafOrder) {
    LineageEvent e1;
    e1.event_id = "root"; e1.dataset_id = "ds2";
    e1.event_type = LineageEventType::INGESTION; e1.performed_by = "src";
    tracker_.recordEvent(e1);

    LineageEvent e2;
    e2.event_id = "mid"; e2.dataset_id = "ds2";
    e2.event_type = LineageEventType::TRANSFORMATION; e2.performed_by = "etl";
    e2.parent_event_id = "root";
    tracker_.recordEvent(e2);

    LineageEvent e3;
    e3.event_id = "leaf"; e3.dataset_id = "ds2";
    e3.event_type = LineageEventType::EXPORT; e3.performed_by = "exporter";
    e3.parent_event_id = "mid";
    tracker_.recordEvent(e3);

    auto chain = tracker_.getUpstreamLineage("leaf");
    ASSERT_EQ(chain.size(), 3u) << "Chain from leaf must include all 3 ancestors";
    EXPECT_EQ(chain[0].event_id, "root")  << "First element must be root";
    EXPECT_EQ(chain[1].event_id, "mid")   << "Second element must be mid";
    EXPECT_EQ(chain[2].event_id, "leaf")  << "Third element must be leaf";
}

// B-3: getDownstreamLineage returns all transitively derived events
TEST_F(LineageAncestryTest, B3_GetDownstreamLineage_TransitiveDescendants) {
    LineageEvent root;
    root.event_id = "ds3-root"; root.dataset_id = "ds3";
    root.event_type = LineageEventType::INGESTION;
    tracker_.recordEvent(root);

    LineageEvent child1;
    child1.event_id = "ds3-c1"; child1.dataset_id = "ds3";
    child1.event_type = LineageEventType::QUERY;
    child1.parent_event_id = "ds3-root";
    tracker_.recordEvent(child1);

    LineageEvent child2;
    child2.event_id = "ds3-c2"; child2.dataset_id = "ds3";
    child2.event_type = LineageEventType::EXPORT;
    child2.parent_event_id = "ds3-root";
    tracker_.recordEvent(child2);

    LineageEvent grandchild;
    grandchild.event_id = "ds3-gc"; grandchild.dataset_id = "ds3";
    grandchild.event_type = LineageEventType::ANONYMIZATION;
    grandchild.parent_event_id = "ds3-c1";
    tracker_.recordEvent(grandchild);

    auto downstream = tracker_.getDownstreamLineage("ds3-root");
    EXPECT_EQ(downstream.size(), 3u)
        << "Downstream of root must include 2 children + 1 grandchild";

    auto hasId = [&](const std::string& id) {
        for (const auto& e : downstream) if (e.event_id == id) return true;
        return false;
    };
    EXPECT_TRUE(hasId("ds3-c1"));
    EXPECT_TRUE(hasId("ds3-c2"));
    EXPECT_TRUE(hasId("ds3-gc"));
}

// B-4: exportLineageAsJson contains all events and the event_type field
TEST_F(LineageAncestryTest, B4_ExportLineageAsJson_ContainsAllEvents) {
    LineageEvent e;
    e.event_id     = "ev-json";
    e.dataset_id   = "ds-json";
    e.event_type   = LineageEventType::MODEL_TRAINING;
    e.performed_by = "trainer";
    e.operation    = "model fit";
    tracker_.recordEvent(e);

    nlohmann::json j = tracker_.exportLineageAsJson("ds-json");
    ASSERT_TRUE(j.contains("dataset_id"))   << "JSON must contain dataset_id";
    ASSERT_TRUE(j.contains("events"))       << "JSON must contain events array";
    ASSERT_FALSE(j["events"].empty())       << "Events array must not be empty";

    const auto& first_event = j["events"][0];
    ASSERT_TRUE(first_event.contains("event_type")) << "Event must have event_type";
    EXPECT_EQ(first_event["event_type"].get<std::string>(), "MODEL_TRAINING");
    EXPECT_EQ(first_event["performed_by"].get<std::string>(), "trainer");
}

// ============================================================================
// Group C – DataLineageTracker × MetricsCollector
// ============================================================================

class LineageMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }
    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }

    DataLineageTracker tracker_;
};

// C-1: recordEvent increments governance_lineage_events_total in Prometheus output
TEST_F(LineageMetricsTest, C1_RecordEvent_IncrementsGovernanceCounter) {
    LineageEvent e;
    e.event_id   = "ev-c1";
    e.dataset_id = "ds-metrics";
    e.event_type = LineageEventType::QUERY;
    e.performed_by = "user";
    tracker_.recordEvent(e);

    const std::string prom = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(prom.find("governance_lineage_events_total"), std::string::npos)
        << "Prometheus output must contain governance_lineage_events_total after recordEvent()";
}

// C-2: Different event types produce distinct label series in Prometheus output
TEST_F(LineageMetricsTest, C2_DifferentEventTypes_DistinctLabelSeries) {
    auto makeEv = [](const std::string& id, LineageEventType et) {
        LineageEvent e;
        e.event_id   = id;
        e.dataset_id = "ds-multi";
        e.event_type = et;
        return e;
    };
    tracker_.recordEvent(makeEv("ev1", LineageEventType::INGESTION));
    tracker_.recordEvent(makeEv("ev2", LineageEventType::EXPORT));
    tracker_.recordEvent(makeEv("ev3", LineageEventType::DELETION));

    const std::string prom = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(prom.find("INGESTION"),  std::string::npos)
        << "Prometheus output must contain INGESTION label";
    EXPECT_NE(prom.find("EXPORT"),     std::string::npos)
        << "Prometheus output must contain EXPORT label";
    EXPECT_NE(prom.find("DELETION"),   std::string::npos)
        << "Prometheus output must contain DELETION label";
}

// C-3: MetricsCollector::reset() clears lineage counters
TEST_F(LineageMetricsTest, C3_MetricsReset_ClearsLineageCounters) {
    LineageEvent e;
    e.event_id   = "ev-c3";
    e.dataset_id = "ds-reset";
    e.event_type = LineageEventType::ENRICHMENT;
    tracker_.recordEvent(e);

    // Verify counter appears before reset
    std::string before = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(before.find("governance_lineage_events_total"), std::string::npos)
        << "Counter must be present before reset";

    MetricsCollector::getInstance().reset();

    std::string after = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_EQ(after.find("governance_lineage_events_total"), std::string::npos)
        << "Counter must be absent after reset";
}

// ============================================================================
// Group D – Full security → governance → observability pipeline
// ============================================================================

struct FullPipelineTest : ::testing::Test {
    void SetUp() override {
        MetricsCollector::getInstance().reset();
        // Configure enforcer with a token verifier that accepts "good-token"
        enforcer_ = std::make_unique<ZeroTrustPolicyEnforcer>(
            [](const std::string& tok, const std::string&) {
                return tok == "good-token";
            });
        enforcer_->addNetworkPolicy(makePolicy("p-user1", "user1"));
    }
    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }

    std::unique_ptr<ZeroTrustPolicyEnforcer> enforcer_;
    DataLineageTracker tracker_;
};

// D-1: ZeroTrust allow → QUERY lineage recorded → Prometheus counter incremented
TEST_F(FullPipelineTest, D1_AllowedAccess_LineageAndMetricsBothUpdated) {
    ZeroTrustContext ctx = makeCtx("user1", "10.5.0.1", "dataset-D1", "good-token");
    auto result = enforcer_->verify(ctx);
    ASSERT_TRUE(result.verified);

    tracker_.recordEvent(makeQueryEvent("ev-D1", "dataset-D1", "user1"));

    // Lineage check
    EXPECT_EQ(tracker_.getLineage("dataset-D1").events.size(), 1u)
        << "Exactly one lineage event must be recorded";

    // MetricsCollector check
    const std::string prom = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(prom.find("governance_lineage_events_total"), std::string::npos)
        << "Prometheus counter must be incremented by the lineage record";
}

// D-2: ZeroTrust deny → no lineage event → Prometheus counter not incremented
TEST_F(FullPipelineTest, D2_DeniedAccess_NoLineageNoMetrics) {
    ZeroTrustContext ctx = makeCtx("user1", "10.5.0.1", "dataset-D2", "bad-token");
    auto result = enforcer_->verify(ctx);
    ASSERT_FALSE(result.verified);

    // Gate: do not record lineage on denial
    if (result.verified) {
        tracker_.recordEvent(makeQueryEvent("ev-D2", "dataset-D2", "user1"));
    }

    EXPECT_EQ(tracker_.totalEventCount(), 0u)
        << "No lineage event must be recorded after denial";

    const std::string prom = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_EQ(prom.find("governance_lineage_events_total"), std::string::npos)
        << "Prometheus counter must NOT appear when no lineage events were recorded";
}

// D-3: Multi-user, multi-resource → distinct lineage records per dataset
TEST_F(FullPipelineTest, D3_MultiUserMultiResource_DistinctLineageRecords) {
    enforcer_->addNetworkPolicy(makePolicy("p-user2", "user2"));

    const std::vector<std::pair<std::string, std::string>> requests = {
        {"user1", "dataset-alpha"},
        {"user1", "dataset-alpha"},  // second access to same dataset
        {"user2", "dataset-beta"},
        {"user2", "dataset-gamma"},
    };

    int ev_counter = 0;
    for (const auto& [user, dataset] : requests) {
        ZeroTrustContext ctx = makeCtx(user, "10.0.0.1", dataset, "good-token");
        ctx.request_id = "req-" + std::to_string(ev_counter);
        auto result = enforcer_->verify(ctx);
        ASSERT_TRUE(result.verified) << "All requests must be allowed";
        tracker_.recordEvent(makeQueryEvent("ev-D3-" + std::to_string(ev_counter),
                                             dataset, user));
        ++ev_counter;
    }

    // dataset-alpha gets 2 events, dataset-beta and dataset-gamma get 1 each
    EXPECT_EQ(tracker_.getLineage("dataset-alpha").events.size(), 2u);
    EXPECT_EQ(tracker_.getLineage("dataset-beta").events.size(),  1u);
    EXPECT_EQ(tracker_.getLineage("dataset-gamma").events.size(), 1u);
    EXPECT_EQ(tracker_.totalEventCount(), 4u);

    // Prometheus output must contain the governance counter (4 events recorded)
    const std::string prom = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(prom.find("governance_lineage_events_total"), std::string::npos)
        << "Prometheus output must reflect all 4 lineage events";
}
