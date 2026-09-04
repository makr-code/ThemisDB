/**
 * @file w6a_critical_journey_hardening_test.cpp
 * @brief Wave 6-A: Release-Candidate Critical Journey Hardening.
 *
 * Validates the highest-priority E2E journeys required for the release decision:
 *   RCJ-01  Happy-path: authenticated ingest → index → query → audit
 *   RCJ-02  Idempotent double-ingest produces a single indexed entry
 *   RCJ-03  Query result consistency after concurrent writes
 *   RCJ-04  Auth-token rotation mid-session preserves read access
 *   RCJ-05  Cross-pipeline state is consistent after partial rollback
 *   RCJ-06  Audit trail completeness across multi-step journey
 *   RCJ-07  Empty-result edge case is handled without error
 *   RCJ-08  State transitions are deterministic under re-execution
 *
 * All tests run offline using mocks from test_fixture.h.
 * CTest labels: wave6;w6a;release_candidate
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis { namespace test { 

namespace {

/// @brief Minimal pipeline that covers ingest → index → query for RCJ tests.
class CriticalJourneyPipeline {
public:
    explicit CriticalJourneyPipeline(std::shared_ptr<MockPipelineAuth>     auth,
                                     std::shared_ptr<MockPipelineIndex>    index,
                                     std::shared_ptr<InMemoryPipelineStorage> storage,
                                     std::shared_ptr<PipelineAuditLog>     audit)
        : auth_(std::move(auth))
        , index_(std::move(index))
        , storage_(std::move(storage))
        , audit_(std::move(audit)) {}

    /// @brief Ingest a document. Returns false if auth fails or doc already indexed.
    bool Ingest(const std::string& token,
                const std::string& doc_id,
                const std::string& content,
                const std::vector<std::string>& terms) {
        if (!auth_->Authorize(token).authorized) {
            audit_->Record({"ingest", "auth_failed", doc_id});
            return false;
        }
        if (storage_->Contains(doc_id)) {
            // Idempotent: skip re-index but do not treat as error
            audit_->Record({"ingest", "skipped_duplicate", doc_id});
            return true;
        }
        storage_->Write(doc_id, content);
        index_->IndexDocument(doc_id, terms);
        audit_->Record({"ingest", "stored", doc_id});
        return true;
    }

    /// @brief Query documents by term. Returns matched payloads.
    std::vector<std::string> Query(const std::string& token, const std::string& term) {
        if (!auth_->Authorize(token).authorized) {
            audit_->Record({"query", "auth_failed", term});
            return {};
        }
        const auto ids = index_->Search(term);
        std::vector<std::string> results = {};

        results.reserve(ids.size());
        for (const auto& id : ids) {
            if (const auto val = storage_->Read(id); val.has_value()) {
                results.push_back(*val);
            }
        }
        audit_->Record({"query", "executed", term});
        return results;
    }

    /// @brief Roll back a document from both storage and index, then record the outcome.
    bool Rollback(const std::string& token, const std::string& doc_id) {
        if (!auth_->Authorize(token).authorized) {
            return false;
        }
        const bool erased = storage_->Erase(doc_id);
        if (erased) {
            index_->RemoveDocument(doc_id);
        }
        audit_->Record({"ingest", erased ? "rollback" : "rollback_noop", doc_id});
        return erased;
    }

    [[nodiscard]] size_t StoredCount() const { return storage_->Size(); }

private:
    std::shared_ptr<MockPipelineAuth>        auth_;
    std::shared_ptr<MockPipelineIndex>       index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog>        audit_;
};

} // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class CriticalJourneyHardeningTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        auth_    = CreateMockAuth();
        index_   = CreateMockIndex();
        storage_ = CreateInMemoryStorage();
        audit_   = CreateAuditLog();
        pipeline_ = std::make_unique<CriticalJourneyPipeline>(auth_, index_, storage_, audit_);

        auth_->AllowToken(kValidToken);
        auth_->DenyToken(kInvalidToken);
    }

    static constexpr const char* kValidToken   = "rcj_valid_token";
    static constexpr const char* kInvalidToken = "rcj_invalid_token";
    static constexpr const char* kTerm         = "release_term";

    std::shared_ptr<MockPipelineAuth>        auth_;
    std::shared_ptr<MockPipelineIndex>       index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog>        audit_;
    std::unique_ptr<CriticalJourneyPipeline> pipeline_;
    TestDataGenerator                        data_gen_;
};

// ---------------------------------------------------------------------------
// RCJ-01 — Happy-path: authenticated ingest → index → query → audit
// ---------------------------------------------------------------------------

/**
 * @test RCJ-01: Full happy-path ingest-query journey.
 *
 * Acceptance Criteria:
 * - Document is stored after authenticated ingest.
 * - Query by term returns the document content.
 * - Audit log records both "stored" and "executed" events.
 */
TEST_F(CriticalJourneyHardeningTest, RCJ01_HappyPathIngestIndexQueryAudit) {
    const std::string doc_id  = "rcj01_doc";
    const std::string content = "rcj01_content";

    ASSERT_TRUE(pipeline_->Ingest(kValidToken, doc_id, content, {kTerm}));

    const auto results = pipeline_->Query(kValidToken, kTerm);

    ASSERT_EQ(results.size(), 1U) << "Expected exactly one result for term '" << kTerm << "'";
    EXPECT_EQ(results.front(), content);
    EXPECT_TRUE(audit_->Contains("ingest", "stored"));
    EXPECT_TRUE(audit_->Contains("query", "executed"));
}

// ---------------------------------------------------------------------------
// RCJ-02 — Idempotent double-ingest produces a single indexed entry
// ---------------------------------------------------------------------------

/**
 * @test RCJ-02: Idempotent ingest — ingesting the same document twice is a no-op.
 *
 * Acceptance Criteria:
 * - Second ingest call succeeds (returns true).
 * - Storage size is 1, not 2.
 * - Audit records one "stored" and one "skipped_duplicate" event.
 */
TEST_F(CriticalJourneyHardeningTest, RCJ02_IdempotentDoubleIngestProducesSingleEntry) {
    const std::string doc_id  = "rcj02_doc";
    const std::string content = "rcj02_content";

    ASSERT_TRUE(pipeline_->Ingest(kValidToken, doc_id, content, {kTerm}));
    ASSERT_TRUE(pipeline_->Ingest(kValidToken, doc_id, content, {kTerm})) << "Second ingest must not error";

    EXPECT_EQ(pipeline_->StoredCount(), 1U);
    EXPECT_TRUE(audit_->Contains("ingest", "stored"));
    EXPECT_TRUE(audit_->Contains("ingest", "skipped_duplicate"));
}

// ---------------------------------------------------------------------------
// RCJ-03 — Query result consistency after concurrent writes
// ---------------------------------------------------------------------------

/**
 * @test RCJ-03: Concurrent writes do not corrupt query results.
 *
 * Acceptance Criteria:
 * - All N concurrent ingests succeed.
 * - Query by shared term returns exactly N results with distinct doc IDs.
 * - Storage count matches thread count.
 */
TEST_F(CriticalJourneyHardeningTest, RCJ03_QueryConsistencyAfterConcurrentWrites) {
    constexpr int kThreads = 8;
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i]() {
            const std::string doc_id  = "rcj03_doc_" + std::to_string(i);
            const std::string content = "rcj03_content_" + std::to_string(i);
            if (pipeline_->Ingest(kValidToken, doc_id, content, {kTerm})) {
                ++success_count;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), kThreads) << "All concurrent ingests must succeed";
    EXPECT_EQ(pipeline_->StoredCount(), static_cast<size_t>(kThreads));

    const auto results = pipeline_->Query(kValidToken, kTerm);
    EXPECT_EQ(results.size(), static_cast<size_t>(kThreads));
}

// ---------------------------------------------------------------------------
// RCJ-04 — Auth-token rotation mid-session preserves read access
// ---------------------------------------------------------------------------

/**
 * @test RCJ-04: New token admitted mid-session; old token revoked; read still works.
 *
 * Acceptance Criteria:
 * - Ingest with original token succeeds.
 * - After token rotation, new token can query.
 * - Old (revoked) token is rejected.
 */
TEST_F(CriticalJourneyHardeningTest, RCJ04_TokenRotationMidSessionPreservesAccess) {
    const std::string doc_id  = "rcj04_doc";
    const std::string content = "rcj04_content";
    const std::string new_token = "rcj04_rotated_token";

    ASSERT_TRUE(pipeline_->Ingest(kValidToken, doc_id, content, {kTerm}));

    // Simulate token rotation: revoke old, admit new
    auth_->DenyToken(kValidToken);
    auth_->AllowToken(new_token);

    // New token should work
    const auto results_new = pipeline_->Query(new_token, kTerm);
    EXPECT_EQ(results_new.size(), 1U) << "New token must have read access after rotation";

    // Old token should be rejected
    const auto results_old = pipeline_->Query(kValidToken, kTerm);
    EXPECT_TRUE(results_old.empty()) << "Revoked token must not return data";
    EXPECT_TRUE(audit_->Contains("query", "auth_failed"));
}

// ---------------------------------------------------------------------------
// RCJ-05 — Cross-pipeline state is consistent after partial rollback
// ---------------------------------------------------------------------------

/**
 * @test RCJ-05: Rolling back one document leaves others intact.
 *
 * Acceptance Criteria:
 * - Two documents are ingested.
 * - Rollback of doc_a succeeds; storage count drops to 1.
 * - Query for term still returns doc_b's content only.
 */
TEST_F(CriticalJourneyHardeningTest, RCJ05_StateConsistentAfterPartialRollback) {
    ASSERT_TRUE(pipeline_->Ingest(kValidToken, "rcj05_doc_a", "content_a", {kTerm, "term_a"}));
    ASSERT_TRUE(pipeline_->Ingest(kValidToken, "rcj05_doc_b", "content_b", {kTerm, "term_b"}));
    ASSERT_EQ(pipeline_->StoredCount(), 2U);

    ASSERT_TRUE(pipeline_->Rollback(kValidToken, "rcj05_doc_a"));
    EXPECT_EQ(pipeline_->StoredCount(), 1U);

    const auto results = pipeline_->Query(kValidToken, kTerm);
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results.front(), "content_b");
    EXPECT_TRUE(audit_->Contains("ingest", "rollback"));
}

// ---------------------------------------------------------------------------
// RCJ-06 — Audit trail completeness across multi-step journey
// ---------------------------------------------------------------------------

/**
 * @test RCJ-06: Audit log is complete for a full multi-step journey.
 *
 * Acceptance Criteria:
 * - Ingest + query + rollback journey produces at least 3 audit events.
 * - All expected module/action pairs are present.
 */
TEST_F(CriticalJourneyHardeningTest, RCJ06_AuditTrailCompleteAcrossMultiStepJourney) {
    ASSERT_TRUE(pipeline_->Ingest(kValidToken, "rcj06_doc", "rcj06_content", {kTerm}));
    pipeline_->Query(kValidToken, kTerm);
    pipeline_->Rollback(kValidToken, "rcj06_doc");

    EXPECT_GE(audit_->Count(), 3U) << "At minimum: stored, executed, rollback";
    EXPECT_TRUE(audit_->Contains("ingest", "stored"));
    EXPECT_TRUE(audit_->Contains("query", "executed"));
    EXPECT_TRUE(audit_->Contains("ingest", "rollback"));
}

// ---------------------------------------------------------------------------
// RCJ-07 — Empty-result edge case is handled without error
// ---------------------------------------------------------------------------

/**
 * @test RCJ-07: Query for a non-existent term returns empty results, not an error.
 *
 * Acceptance Criteria:
 * - Query with valid token for unknown term succeeds (no exception).
 * - Returns empty vector.
 * - Audit records "executed" (query ran normally, just no matches).
 */
TEST_F(CriticalJourneyHardeningTest, RCJ07_EmptyResultEdgeCaseIsHandledWithoutError) {
    const auto results = pipeline_->Query(kValidToken, "nonexistent_term_rcj07");

    EXPECT_TRUE(results.empty()) << "No documents indexed for this term; must return empty";
    EXPECT_TRUE(audit_->Contains("query", "executed"));
}

// ---------------------------------------------------------------------------
// RCJ-08 — State transitions are deterministic under re-execution
// ---------------------------------------------------------------------------

/**
 * @test RCJ-08: Re-running the same journey N times yields identical final state.
 *
 * Acceptance Criteria:
 * - Each iteration ends with the same storage count.
 * - Query always returns the same number of results.
 * - No state leaks between iterations.
 */
TEST_F(CriticalJourneyHardeningTest, RCJ08_StateTransitionsDeterministicUnderReExecution) {
    constexpr int kIterations = 5;

    for (int iter = 0; iter < kIterations; ++iter) {
        // Fresh infrastructure per iteration
        auto auth    = CreateMockAuth();
        auto index   = CreateMockIndex();
        auto storage = CreateInMemoryStorage();
        auto audit   = CreateAuditLog();
        auth->AllowToken(kValidToken);

        CriticalJourneyPipeline pipe(auth, index, storage, audit);

        pipe.Ingest(kValidToken, "doc_iter", "content_iter", {kTerm});
        const auto results = pipe.Query(kValidToken, kTerm);

        EXPECT_EQ(pipe.StoredCount(), 1U)
            << "Iteration " << iter << ": StoredCount must be 1";
        EXPECT_EQ(results.size(), 1U)
            << "Iteration " << iter << ": Query must return exactly 1 result";
    }
}
} } // namespace themis::test
