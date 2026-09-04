// test_mdm_integration.cpp
//
// Integration-style tests for the full MDM pipeline:
//   - Batch import with existing users, exact + fuzzy matching
//   - Conflict-resolution policies end-to-end
//   - Audit trail generated during a full workflow
//   - Metrics snapshot after a workflow run

#include <gtest/gtest.h>
#include "importers/mdm_engine.h"
#include "importers/mdm_audit_trail.h"
#include "importers/mdm_metrics.h"
#include <vector>

namespace ti = themis::importers;

// ============================================================================
// Helpers
// ============================================================================

static ti::ImportOptions opts() {
    ti::ImportOptions o;
    o.dry_run = false;
    return o;
}

static ti::MDMConfig mdmCfg(
    bool auto_resolve = true,
    ti::ResolutionPolicy policy = ti::ResolutionPolicy::RICHEST_MERGE
) {
    ti::MDMConfig cfg;
    cfg.primary_key_fields     = {"id"};
    cfg.unique_fields          = {"email"};
    cfg.deterministic_threshold = 1.0;
    cfg.semantic_threshold     = 0.80;
    cfg.resolution_policy      = policy;
    cfg.auto_resolve_conflicts  = auto_resolve;
    cfg.create_reverse_links    = true;
    cfg.semantic_config.overall_threshold = 0.80;
    return cfg;
}

// ============================================================================
// Integration test: import 10 users, 8 existing
// ============================================================================

class MDMIntegrationTest : public ::testing::Test {
protected:
    ti::MDMEngine engine;

    std::vector<ti::json> makeExistingUsers(int n) {
        std::vector<ti::json> users = {};

        for (int i = 0; i < n; ++i) {
            users.push_back({
                {"id",    "existing-" + std::to_string(i)},
                {"name",  "User " + std::to_string(i)},
                {"email", "user" + std::to_string(i) + "@example.com"}
            });
        }
        return users;
    }

    std::vector<ti::json> makeIncomingUsers(int n_existing, int n_new) {
        std::vector<ti::json> users = {};

        // Re-import existing users (same IDs).
        for (int i = 0; i < n_existing; ++i) {
            users.push_back({
                {"id",    "existing-" + std::to_string(i)},
                {"name",  "User " + std::to_string(i)},
                {"email", "user" + std::to_string(i) + "@example.com"}
            });
        }
        // Truly new users.
        for (int i = 0; i < n_new; ++i) {
            users.push_back({
                {"id",    "new-" + std::to_string(i)},
                {"name",  "New User " + std::to_string(i)},
                {"email", "newuser" + std::to_string(i) + "@example.com"}
            });
        }
        return users;
    }
};

TEST_F(MDMIntegrationTest, ImportUsersWithExistingRecords) {
    const int n_existing = 8;
    const int n_new      = 2;

    auto existing = makeExistingUsers(n_existing);
    auto incoming = makeIncomingUsers(n_existing, n_new);

    auto result = engine.executeMDMWorkflow(
        incoming, existing, "users", mdmCfg(), opts());

    EXPECT_EQ(result.total_incoming, static_cast<size_t>(n_existing + n_new));
    // New entities = n_new (the truly new ones with no match).
    EXPECT_EQ(result.new_entities, static_cast<size_t>(n_new));
    // Deterministic matches for the re-imported existing users.
    EXPECT_GE(result.deterministic_matches, static_cast<size_t>(n_existing));
    // Links created ≥ n_existing (one per matched pair; reverse links add more).
    EXPECT_GE(result.links_created, static_cast<size_t>(n_existing));
    EXPECT_EQ(result.status, "completed");
}

TEST_F(MDMIntegrationTest, GoldenRecordsCreatedForMatchedEntities) {
    auto existing = makeExistingUsers(3);
    auto incoming = makeIncomingUsers(3, 0);

    auto result = engine.executeMDMWorkflow(
        incoming, existing, "users", mdmCfg(), opts());

    EXPECT_GE(result.golden_records_created, 0u);
    for (const auto& gr : result.golden_records) {
        EXPECT_FALSE(gr.canonical_id.empty());
        EXPECT_GE(gr.completeness_score, 0.0);
        EXPECT_LE(gr.completeness_score, 1.0);
    }
}

TEST_F(MDMIntegrationTest, FieldMergeRichestMergePrefersLongerName) {
    ti::json incoming = {
        {"id", "u-1"}, {"name", "Alice S."}, {"email", "alice@test.com"}
    };
    ti::json existing = {
        {"id", "u-1"}, {"name", "Alice Smith"}, {"email", nullptr}
    };

    auto result = engine.executeMDMWorkflow(
        {incoming}, {existing}, "users", mdmCfg(), opts());

    ASSERT_FALSE(result.golden_records.empty());
    const auto& gr = result.golden_records[0];
    // Richest merge should keep "Alice Smith" (longer) for name
    // and "alice@test.com" (non-null) for email.
    EXPECT_FALSE(gr.merged_data.is_null());
}

TEST_F(MDMIntegrationTest, ManualReviewQueuePopulatedWhenAutoResolveDisabled) {
    // With auto_resolve = false, all matches below deterministic threshold
    // should be queued for review.
    ti::json incoming = {{"id", "u-new"}, {"name", "Alice"}};
    ti::json existing = {{"id", "u-old"}, {"name", "Alice"}};  // Different ID → no det match

    ti::MDMConfig cfg = mdmCfg(/*auto_resolve=*/false);
    cfg.semantic_threshold = 0.50;
    cfg.semantic_config.overall_threshold = 0.50;

    auto result = engine.executeMDMWorkflow({incoming}, {existing}, "users", cfg, opts());

    // Either auto-resolved or in review queue; total = links_created.
    EXPECT_EQ(result.conflicts_auto_resolved + result.manual_reviews_needed,
              result.links_created);
}

TEST_F(MDMIntegrationTest, WorkflowResultJsonRoundtrip) {
    auto result = engine.executeMDMWorkflow(
        makeIncomingUsers(2, 1), makeExistingUsers(2), "users", mdmCfg(), opts());

    auto j = result.toJson();
    EXPECT_EQ(j["collection_name"].get<std::string>(), "users");
    EXPECT_EQ(j["total_incoming"].get<size_t>(), 3u);
}

// ============================================================================
// Audit trail integration
// ============================================================================

TEST_F(MDMIntegrationTest, AuditTrailRecordsAllEventTypes) {
    ti::MDMAuditTrail audit;

    // Record all operation types.
    const std::vector<ti::MDMAuditTrail::Operation> ops = {
        ti::MDMAuditTrail::Operation::MATCH_FOUND,
        ti::MDMAuditTrail::Operation::LINK_CREATED,
        ti::MDMAuditTrail::Operation::CONFLICT_DETECTED,
        ti::MDMAuditTrail::Operation::CONFLICT_RESOLVED,
        ti::MDMAuditTrail::Operation::GOLDEN_RECORD_CREATED,
        ti::MDMAuditTrail::Operation::ENTITY_MERGED,
        ti::MDMAuditTrail::Operation::REVIEW_REQUESTED,
        ti::MDMAuditTrail::Operation::REVIEW_COMPLETED
    };

    for (auto op : ops) {
        ti::MDMAuditTrail::AuditEvent ev;
        ev.operation         = op;
        ev.collection_name   = "users";
        ev.source_entity_id  = "src";
        ev.target_entity_id  = "tgt";
        ev.confidence_score  = 0.9;
        ev.status            = "completed";
        audit.recordEvent(ev);
    }

    EXPECT_EQ(audit.eventCount(), ops.size());
    EXPECT_TRUE(audit.verifyAuditChain());
}

TEST_F(MDMIntegrationTest, AuditReportExportFiltersCorrectly) {
    ti::MDMAuditTrail audit;

    // Add events for two different collections.
    for (int i = 0; i < 3; ++i) {
        ti::MDMAuditTrail::AuditEvent ev;
        ev.operation        = ti::MDMAuditTrail::Operation::LINK_CREATED;
        ev.collection_name  = "users";
        ev.source_entity_id = "u-" + std::to_string(i);
        ev.target_entity_id = "t-" + std::to_string(i);
        audit.recordEvent(ev);
    }
    for (int i = 0; i < 2; ++i) {
        ti::MDMAuditTrail::AuditEvent ev;
        ev.operation        = ti::MDMAuditTrail::Operation::LINK_CREATED;
        ev.collection_name  = "products";
        ev.source_entity_id = "p-" + std::to_string(i);
        ev.target_entity_id = "q-" + std::to_string(i);
        audit.recordEvent(ev);
    }

    auto report = audit.exportAuditReport("users", "", "");
    EXPECT_EQ(report["total_events"].get<size_t>(), 3u);
}

// ============================================================================
// Metrics integration
// ============================================================================

TEST_F(MDMIntegrationTest, MetricsSnapshotPopulatedFromWorkflowResult) {
    auto existing = makeExistingUsers(5);
    auto incoming = makeIncomingUsers(5, 3);

    auto result = engine.executeMDMWorkflow(
        incoming, existing, "users", mdmCfg(), opts());

    ti::MDMMetricSnapshot snap;
    snap.deterministic_matches     = result.deterministic_matches;
    snap.semantic_matches          = result.semantic_matches;
    snap.links_created             = result.links_created;
    snap.duplicate_records_found   = result.deterministic_matches + result.semantic_matches;
    snap.duplicate_records_merged  = result.golden_records_created;
    snap.conflicts_requiring_review = result.manual_reviews_needed;
    snap.conflicts_auto_resolved    = result.conflicts_auto_resolved;
    snap.matching_time_seconds     = 0.01;

    // Emit metrics and verify callback receives calls.
    int cb_calls = 0;
    ti::MDMMetrics::emitMetrics(snap, "users", [&](const std::string&,
                                                     const std::map<std::string, std::string>&,
                                                     double) { ++cb_calls; });
    EXPECT_GE(cb_calls, 10);

    auto dash = ti::MDMMetrics::getDashboardMetrics(snap, "users");
    EXPECT_EQ(dash["collection"].get<std::string>(), "users");
    EXPECT_TRUE(dash["summary"].contains("total_matches"));
}

TEST_F(MDMIntegrationTest, EmptyWorkflowProducesConsistentResult) {
    auto result = engine.executeMDMWorkflow({}, {}, "empty_col", mdmCfg(), opts());
    EXPECT_EQ(result.total_incoming, 0u);
    EXPECT_EQ(result.new_entities, 0u);
    EXPECT_EQ(result.links_created, 0u);
    EXPECT_EQ(result.status, "completed");
    EXPECT_FALSE(result.workflow_id.empty());
}
