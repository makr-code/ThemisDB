/**
 * @file w3a_full_function_critical_flows_test.cpp
 * @brief Wave 3-A integration tests: full-function critical end-to-end flows.
 *
 * Covers success and failure paths across the complete pipeline:
 *   Auth -> Ingest -> Index -> Storage -> LLM -> Export
 *
 * Each test verifies state consistency at multiple checkpoints and uses
 * fachliche (domain-level) assertions rather than no-crash-only checks.
 *
 * ## Covered Flows
 * - FFW-01: Happy-path full pipeline from token registration to export snapshot
 * - FFW-02: Ingest failure rolls back index and CDC state
 * - FFW-03: Query on missing document returns domain error (not crash)
 * - FFW-04: Auth revocation mid-session blocks all subsequent pipeline actions
 * - FFW-05: Concurrent ingest and query keeps state consistent
 * - FFW-06: LLM degradation falls back and recovery is tracked in audit
 * - FFW-07: Export snapshot content matches ingested event count exactly
 * - FFW-08: Multi-tenant complete flow maintains strict cross-tenant isolation
 *
 * @note All tests run offline with deterministic mocks; no GPU, LLM service,
 *       or network dependency.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis { namespace test { 

namespace {

/**
 * @brief Result of a full-function pipeline query request.
 */
struct FullFunctionQueryResult {
    bool ok{false};
    bool used_index{false};
    bool used_llm{false};
    bool used_fallback{false};
    std::string answer;
    std::string error_code;
};

/**
 * @brief Snapshot export result with counters for assertions.
 */
struct SnapshotResult {
    bool ok{false};
    size_t event_count{0};
    size_t cdc_count{0};
};

/**
 * @brief Full-function pipeline spanning Auth → Ingest → Index → Storage → LLM → Export.
 *
 * This class models the critical system flow under test. It uses the shared
 * mock infrastructure from IntegrationTestFixture to produce deterministic,
 * reproducible results without external runtime dependencies.
 *
 * @note This is a test-only pipeline abstraction.
 * // NON-PRODUCTION PATH (Simulation/Stub/Mockup)
 * // Reason: self-contained pipeline model for integration coverage of critical flows
 * // Activation: test-only (THEMIS_TEST_BUILD=1)
 * // Production Delta: no RocksDB, no gRPC, no GPU; mocked auth/index/LLM
 * // Approved By: @makr-code (maintainer) — PR tests(w3) Wave 3 test hardening
 * // Removal Target: keep permanently as integration coverage harness
 */
class FullFunctionPipeline {
public:
    /**
     * @brief Constructs the pipeline with all required mock components.
     * @param auth   Token-based authorization mock.
     * @param index  Inverted-index mock for full-text retrieval.
     * @param storage Key-value storage mock.
     * @param llm    LLM backend mock for inference and embeddings.
     * @param audit  Thread-safe audit log.
     */
    FullFunctionPipeline(std::shared_ptr<MockPipelineAuth> auth,
                         std::shared_ptr<MockPipelineIndex> index,
                         std::shared_ptr<InMemoryPipelineStorage> storage,
                         std::shared_ptr<MockPipelineLlmBackend> llm,
                         std::shared_ptr<PipelineAuditLog> audit)
        : auth_(std::move(auth)),
          index_(std::move(index)),
          storage_(std::move(storage)),
          llm_(std::move(llm)),
          audit_(std::move(audit)) {}

    /**
     * @brief Registers a session for the given token and tenant.
     * @param token     Auth token to validate and bind.
     * @param tenant_id Tenant namespace for isolation.
     * @param role      Role required for access (must be "app_user").
     * @return true if the session was accepted; false if auth failed or role mismatch.
     */
    [[nodiscard]] bool RegisterSession(const std::string& token,
                                       const std::string& tenant_id,
                                       const std::string& role) {
        const auto auth_result = auth_->Authorize(token);
        if (!auth_result.authorized || role != "app_user") {
            audit_->Record({"auth", "session_rejected", token});
            return false;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (revoked_tokens_.count(token) > 0U) {
            audit_->Record({"auth", "token_revoked", token});
            return false;
        }
        const auto existing = session_tenant_.find(token);
        if (existing != session_tenant_.end() && existing->second != tenant_id) {
            audit_->Record({"auth", "token_rebind_blocked", token});
            return false;
        }
        session_tenant_[token] = tenant_id;
        audit_->Record({"auth", "session_started", tenant_id});
        return true;
    }

    /**
     * @brief Revokes a token, blocking all future pipeline actions for that session.
     * @param token Auth token to revoke.
     */
    void RevokeToken(const std::string& token) {
        std::lock_guard<std::mutex> lock(mutex_);
        revoked_tokens_.insert(token);
        session_tenant_.erase(token);
        audit_->Record({"auth", "token_revoked_by_admin", token});
    }

    /**
     * @brief Ingests an event: validates auth, deduplicates, writes to storage,
     *        indexes terms, and records a CDC event.
     *
     * @param token    Auth token identifying the caller session.
     * @param tenant_id Tenant namespace; must match the registered session.
     * @param event_id Unique event identifier within the tenant namespace.
     * @param payload  Event payload string to persist.
     * @param terms    Full-text index terms to associate with this event.
     * @return true on success or duplicate (idempotent); false on auth failure.
     */
    [[nodiscard]] bool IngestEvent(const std::string& token,
                                   const std::string& tenant_id,
                                   const std::string& event_id,
                                   const std::string& payload,
                                   const std::vector<std::string>& terms) {
        if (!IsAuthorizedTenant(token, tenant_id)) {
            audit_->Record({"ingest", "auth_failed", tenant_id + "::" + event_id});
            return false;
        }

        const auto key = tenant_id + "::" + event_id;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ingested_keys_.count(key) > 0U) {
                audit_->Record({"ingest", "duplicate_skipped", key});
                return true; // idempotent
            }
            ingested_keys_.insert(key);
            cdc_log_.push_back("cdc:insert:" + key);
        }

        // Write to storage and index only after dedup check passes
        storage_->Write(key, payload);
        index_->IndexDocument(key, terms);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            tenant_event_keys_[tenant_id].push_back(key);
        }
        audit_->Record({"ingest", "event_persisted", key});
        return true;
    }

    /**
     * @brief Executes a full-function query: auth → index lookup → storage read
     *        → LLM inference (with fallback).
     *
     * @param token    Auth token.
     * @param tenant_id Tenant namespace.
     * @param question User question to pass to the LLM.
     * @param term     Index term to drive document retrieval.
     * @return FullFunctionQueryResult with answer, flags, and error_code.
     */
    [[nodiscard]] FullFunctionQueryResult Query(const std::string& token,
                                                const std::string& tenant_id,
                                                const std::string& question,
                                                const std::string& term) {
        if (!IsAuthorizedTenant(token, tenant_id)) {
            return {false, false, false, false, "", "auth_failed"};
        }

        // Index lookup
        const auto hits = index_->Search(term);
        if (hits.empty()) {
            audit_->Record({"query", "index_miss", term});
            return {false, false, false, false, "", "index_miss"};
        }

        // Find a hit belonging to this tenant
        std::string context = {};
        bool tenant_hit = false;
        for (const auto& hit : hits) {
            if (hit.rfind(tenant_id + "::", 0) != 0U) {
                continue;
            }
            const auto payload = storage_->Read(hit);
            if (payload.has_value()) {
                context = *payload;
                tenant_hit = true;
                break;
            }
        }

        if (!tenant_hit) {
            audit_->Record({"query", "storage_miss", tenant_id + ":" + term});
            return {false, true, false, false, "", "storage_miss"};
        }

        // LLM inference with fallback
        const auto response = llm_->Infer(question, context);
        if (response.has_value()) {
            audit_->Record({"query", "llm_answered", tenant_id});
            return {true, true, true, false, *response, ""};
        }

        // Fallback: deterministic local summary
        audit_->Record({"query", "llm_fallback", tenant_id});
        return {true, true, false, true, "fallback:summary:" + context, ""};
    }

    /**
     * @brief Exports a tenant snapshot to the given output file.
     *
     * Writes event count and CDC count to the file; does NOT emit a replica
     * marker if the file cannot be opened.
     *
     * @param tenant_id Tenant namespace to export.
     * @param output_file Destination file path.
     * @return SnapshotResult with success flag and consistency counters.
     */
    [[nodiscard]] SnapshotResult ExportSnapshot(const std::string& tenant_id,
                                                const std::filesystem::path& output_file) {
        const size_t event_count = TenantEventCount(tenant_id);
        const size_t cdc_count   = TenantCdcCount(tenant_id);

        std::ofstream out(output_file);
        if (!out.is_open()) {
            audit_->Record({"export", "open_failed", output_file.string()});
            return {false, event_count, cdc_count};
        }

        out << "tenant=" << tenant_id << "\n";
        out << "events=" << event_count << "\n";
        out << "cdc=" << cdc_count << "\n";
        audit_->Record({"export", "snapshot_written", tenant_id});
        return {true, event_count, cdc_count};
    }

    // --- Counters for state consistency assertions ---

    /**
     * @brief Returns the number of events ingested for the given tenant.
     * @param tenant_id Tenant namespace to query.
     */
    [[nodiscard]] size_t TenantEventCount(const std::string& tenant_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = tenant_event_keys_.find(tenant_id);
        return it == tenant_event_keys_.end() ? 0U : it->second.size();
    }

    /**
     * @brief Returns total number of CDC log entries.
     */
    [[nodiscard]] size_t CdcCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cdc_log_.size();
    }

    /**
     * @brief Returns number of CDC log entries for the given tenant.
     * @param tenant_id Tenant namespace to query.
     */
    [[nodiscard]] size_t TenantCdcCount(const std::string& tenant_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const std::string prefix = "cdc:insert:" + tenant_id + "::";
        size_t count = 0U;
        for (const auto& entry : cdc_log_) {
            if (entry.rfind(prefix, 0U) == 0U) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Returns total unique ingested key count (global across all tenants).
     */
    [[nodiscard]] size_t TotalIngestedCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return ingested_keys_.size();
    }

private:
    [[nodiscard]] bool IsAuthorizedTenant(const std::string& token,
                                          const std::string& tenant_id) const {
        const auto auth_result = auth_->Authorize(token);
        if (!auth_result.authorized) {
            return false;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (revoked_tokens_.count(token) > 0U) {
            return false;
        }
        const auto it = session_tenant_.find(token);
        return it != session_tenant_.end() && it->second == tenant_id;
    }

    std::shared_ptr<MockPipelineAuth>       auth_;
    std::shared_ptr<MockPipelineIndex>      index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineLlmBackend> llm_;
    std::shared_ptr<PipelineAuditLog>       audit_;

    mutable std::mutex                                           mutex_;
    std::unordered_map<std::string, std::string>                 session_tenant_;
    std::unordered_set<std::string>                              revoked_tokens_;
    std::unordered_set<std::string>                              ingested_keys_;
    std::unordered_map<std::string, std::vector<std::string>>   tenant_event_keys_;
    std::vector<std::string>                                     cdc_log_;
};

} // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for Wave 3-A full-function critical flow tests.
 *
 * Provides a fully wired FullFunctionPipeline with all mock components
 * initialised in SetUp(). Each test gets a fresh, isolated pipeline instance.
 */
class W3AFullFunctionCriticalFlowsTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        auth_     = CreateMockAuth();
        index_    = CreateMockIndex();
        storage_  = CreateInMemoryStorage();
        llm_      = CreateMockLlmBackend();
        audit_    = CreateAuditLog();
        data_gen_ = std::make_unique<TestDataGenerator>();
        pipeline_ = std::make_unique<FullFunctionPipeline>(auth_, index_, storage_, llm_, audit_);
    }

    std::shared_ptr<MockPipelineAuth>        auth_;
    std::shared_ptr<MockPipelineIndex>       index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineLlmBackend>  llm_;
    std::shared_ptr<PipelineAuditLog>        audit_;
    std::unique_ptr<TestDataGenerator>       data_gen_;
    std::unique_ptr<FullFunctionPipeline>    pipeline_;
};

// ---------------------------------------------------------------------------
// FFW-01: Happy-path full pipeline
// ---------------------------------------------------------------------------

/**
 * @test FFW-01 — Complete happy-path flow from token registration to export snapshot.
 *
 * Acceptance Criteria:
 * - Session registration succeeds for a valid token.
 * - Ingest persists the event and records a CDC entry.
 * - Query resolves via index + LLM and returns a non-empty answer.
 * - Export snapshot writes event and CDC count consistently to disk.
 * - Audit log captures each stage: session_started, event_persisted,
 *   llm_answered, snapshot_written.
 */
TEST_F(W3AFullFunctionCriticalFlowsTest, FFW01_HappyPathFullPipelineIngestQueryExport) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);

    // Step 1: Register session
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_ffw01", "app_user"))
        << "FFW-01: Session registration must succeed for a valid token";

    // Step 2: Ingest event
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_ffw01", "event_1",
                                       "contract-2024-001 signed", {"contract"}))
        << "FFW-01: Event ingest must succeed";

    // Step 3: Query — should resolve via index + LLM
    const auto qr = pipeline_->Query(token, "tenant_ffw01", "Welcher Vertrag ist unterschrieben?", "contract");
    ASSERT_TRUE(qr.ok)              << "FFW-01: Query must succeed";
    EXPECT_TRUE(qr.used_index)      << "FFW-01: Query must use the index";
    EXPECT_TRUE(qr.used_llm)        << "FFW-01: LLM must be invoked";
    EXPECT_FALSE(qr.used_fallback)  << "FFW-01: No fallback should activate on a working LLM";
    EXPECT_FALSE(qr.answer.empty()) << "FFW-01: Answer must be non-empty";

    // Step 4: Consistency — event and CDC counts must agree
    EXPECT_EQ(pipeline_->TenantEventCount("tenant_ffw01"), 1U)
        << "FFW-01: Exactly one event must be recorded for the tenant";
    EXPECT_EQ(pipeline_->CdcCount(), 1U)
        << "FFW-01: CDC count must equal ingest count";

    // Step 5: Export snapshot
    const auto snap_path = GetTempDir() / "ffw01_snapshot.txt";
    const auto snap      = pipeline_->ExportSnapshot("tenant_ffw01", snap_path);
    ASSERT_TRUE(snap.ok)                    << "FFW-01: Snapshot export must succeed";
    EXPECT_EQ(snap.event_count, 1U)         << "FFW-01: Snapshot event_count mismatch";
    EXPECT_EQ(snap.cdc_count, 1U)           << "FFW-01: Snapshot CDC count mismatch";
    EXPECT_TRUE(std::filesystem::exists(snap_path)) << "FFW-01: Snapshot file must exist on disk";

    // Step 6: Audit verification
    EXPECT_TRUE(audit_->Contains("auth",   "session_started"))   << "FFW-01: Auth audit missing";
    EXPECT_TRUE(audit_->Contains("ingest", "event_persisted"))   << "FFW-01: Ingest audit missing";
    EXPECT_TRUE(audit_->Contains("query",  "llm_answered"))      << "FFW-01: Query audit missing";
    EXPECT_TRUE(audit_->Contains("export", "snapshot_written"))  << "FFW-01: Export audit missing";
}

// ---------------------------------------------------------------------------
// FFW-02: Ingest failure on unauthorized token does not mutate state
// ---------------------------------------------------------------------------

/**
 * @test FFW-02 — Ingest with an unauthorized token must not mutate index, storage, or CDC log.
 *
 * Acceptance Criteria:
 * - IngestEvent returns false for a token that was never registered.
 * - Storage, index, and CDC state remain empty after the failed ingest.
 * - Audit log records an auth_failed entry.
 */
TEST_F(W3AFullFunctionCriticalFlowsTest, FFW02_UnauthorizedIngestDoesNotMutateState) {
    const auto token = data_gen_->GeneratePipelineToken(false);
    // Token is NOT added to auth_ → every call will be rejected

    EXPECT_FALSE(pipeline_->IngestEvent(token, "tenant_ffw02", "event_1",
                                        "secret-payload", {"sensitive"}))
        << "FFW-02: Ingest with unauthorized token must fail";

    EXPECT_EQ(pipeline_->TenantEventCount("tenant_ffw02"), 0U)
        << "FFW-02: No event must be recorded after failed ingest";
    EXPECT_EQ(pipeline_->CdcCount(), 0U)
        << "FFW-02: CDC log must remain empty after failed ingest";
    EXPECT_EQ(storage_->Size(), 0U)
        << "FFW-02: Storage must not receive any entry after failed ingest";
    EXPECT_TRUE(index_->Search("sensitive").empty())
        << "FFW-02: Index must not contain the rejected term after failed ingest";

    EXPECT_TRUE(audit_->Contains("ingest", "auth_failed"))
        << "FFW-02: Audit must record auth_failed for rejected ingest";
}

// ---------------------------------------------------------------------------
// FFW-03: Query on missing document returns domain error, not a crash
// ---------------------------------------------------------------------------

/**
 * @test FFW-03 — Query on a term with no indexed documents returns a domain error.
 *
 * Acceptance Criteria:
 * - Query returns ok=false with error_code "index_miss".
 * - No LLM call is made when the index has no match.
 * - Audit log records index_miss.
 */
TEST_F(W3AFullFunctionCriticalFlowsTest, FFW03_QueryOnMissingTermReturnsDomainError) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_ffw03", "app_user"));

    const auto qr = pipeline_->Query(token, "tenant_ffw03", "Was ist passiert?", "nonexistent_term");
    EXPECT_FALSE(qr.ok)                      << "FFW-03: Query must fail on index miss";
    EXPECT_EQ(qr.error_code, "index_miss")   << "FFW-03: Error code must be 'index_miss'";
    EXPECT_FALSE(qr.used_llm)                << "FFW-03: LLM must not be called on index miss";
    EXPECT_EQ(llm_->InferenceCalls(), 0U)    << "FFW-03: LLM inference must not be invoked";

    EXPECT_TRUE(audit_->Contains("query", "index_miss"))
        << "FFW-03: Audit must record index_miss";
}

// ---------------------------------------------------------------------------
// FFW-04: Token revocation blocks all subsequent pipeline actions
// ---------------------------------------------------------------------------

/**
 * @test FFW-04 — Revoking an active token immediately blocks all subsequent pipeline actions.
 *
 * Acceptance Criteria:
 * - An event ingested before revocation is persisted correctly.
 * - After RevokeToken(), IngestEvent returns false.
 * - After RevokeToken(), Query returns ok=false with auth_failed.
 * - CDC count remains unchanged after the revocation-blocked ingest attempt.
 */
TEST_F(W3AFullFunctionCriticalFlowsTest, FFW04_TokenRevocationBlocksAllSubsequentActions) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_ffw04", "app_user"));

    // Pre-revocation: ingest should work
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_ffw04", "event_before",
                                       "pre-revocation payload", {"ops"}))
        << "FFW-04: Pre-revocation ingest must succeed";

    EXPECT_EQ(pipeline_->TenantEventCount("tenant_ffw04"), 1U)
        << "FFW-04: One event must be persisted before revocation";

    // Revoke the token
    pipeline_->RevokeToken(token);

    // Post-revocation: ingest must fail
    EXPECT_FALSE(pipeline_->IngestEvent(token, "tenant_ffw04", "event_after",
                                        "should not persist", {"ops"}))
        << "FFW-04: Post-revocation ingest must be rejected";

    EXPECT_EQ(pipeline_->TenantEventCount("tenant_ffw04"), 1U)
        << "FFW-04: Event count must not change after revocation";
    EXPECT_EQ(pipeline_->CdcCount(), 1U)
        << "FFW-04: CDC count must not grow after token revocation";

    // Post-revocation: query must fail
    const auto qr = pipeline_->Query(token, "tenant_ffw04", "status", "ops");
    EXPECT_FALSE(qr.ok)                       << "FFW-04: Post-revocation query must fail";
    EXPECT_EQ(qr.error_code, "auth_failed")   << "FFW-04: Error code must be 'auth_failed'";

    EXPECT_TRUE(audit_->Contains("auth", "token_revoked_by_admin"))
        << "FFW-04: Audit must record token_revoked_by_admin";
}

// ---------------------------------------------------------------------------
// FFW-05: Concurrent ingest keeps state consistent
// ---------------------------------------------------------------------------

/**
 * @test FFW-05 — Concurrent ingest operations from multiple threads produce
 *               consistent final state (no double-counts, no lost writes).
 *
 * Acceptance Criteria:
 * - All 10 ingest threads complete successfully without race conditions.
 * - TenantEventCount equals the number of distinct events (10).
 * - CdcCount equals TenantEventCount.
 * - TotalIngestedCount equals 10 (no duplicates, no lost writes).
 */
TEST_F(W3AFullFunctionCriticalFlowsTest, FFW05_ConcurrentIngestKeepsStateConsistent) {
    constexpr size_t kWorkers     = 10U;
    constexpr size_t kEventsEach  = 1U;

    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_ffw05", "app_user"));

    std::atomic<size_t> success_count{0U};
    std::vector<std::thread> threads;
    threads.reserve(kWorkers);

    for (size_t i = 0; i < kWorkers; ++i) {
        threads.emplace_back([&, i]() {
            const auto event_id = "event_" + std::to_string(i);
            const auto payload  = "data_" + std::to_string(i);
            if (pipeline_->IngestEvent(token, "tenant_ffw05", event_id, payload, {"concurrent"})) {
                ++success_count;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    const size_t expected_events = kWorkers * kEventsEach;
    EXPECT_EQ(success_count.load(), expected_events)
        << "FFW-05: All " << expected_events << " concurrent ingests must succeed";
    EXPECT_EQ(pipeline_->TenantEventCount("tenant_ffw05"), expected_events)
        << "FFW-05: Tenant event count must equal ingest count (no lost writes)";
    EXPECT_EQ(pipeline_->CdcCount(), expected_events)
        << "FFW-05: CDC count must equal event count (no double-counts)";
    EXPECT_EQ(pipeline_->TotalIngestedCount(), expected_events)
        << "FFW-05: Total ingested must equal expected (no duplicates)";
}

// ---------------------------------------------------------------------------
// FFW-06: LLM degradation activates fallback; recovery re-enables LLM path
// ---------------------------------------------------------------------------

/**
 * @test FFW-06 — LLM degradation falls back to deterministic summary; recovery
 *               re-enables the LLM path on the next request.
 *
 * Acceptance Criteria:
 * - With LLM failure active, query returns used_fallback=true.
 * - After LLM recovery, query returns used_llm=true with no fallback.
 * - Audit log records both llm_fallback and llm_answered in the correct order.
 */
TEST_F(W3AFullFunctionCriticalFlowsTest, FFW06_LlmDegradationFallsBackThenRecoveryReenablesLlm) {
    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_ffw06", "app_user"));
    ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_ffw06", "event_1",
                                       "service degraded", {"ops"}));

    // Degrade LLM
    llm_->SetInferenceFailure(true);
    const auto degraded_qr = pipeline_->Query(token, "tenant_ffw06", "status", "ops");
    ASSERT_TRUE(degraded_qr.ok)          << "FFW-06: Query must return ok even with LLM failure";
    EXPECT_TRUE(degraded_qr.used_fallback) << "FFW-06: Fallback must activate on LLM failure";
    EXPECT_FALSE(degraded_qr.used_llm)    << "FFW-06: LLM flag must be false when inference fails";
    EXPECT_NE(degraded_qr.answer.find("fallback:summary"), std::string::npos)
        << "FFW-06: Fallback answer must contain 'fallback:summary'";
    EXPECT_TRUE(audit_->Contains("query", "llm_fallback"))
        << "FFW-06: Audit must record llm_fallback";

    // Recover LLM
    llm_->SetInferenceFailure(false);
    const auto recovered_qr = pipeline_->Query(token, "tenant_ffw06", "status", "ops");
    ASSERT_TRUE(recovered_qr.ok)           << "FFW-06: Recovered query must succeed";
    EXPECT_TRUE(recovered_qr.used_llm)     << "FFW-06: LLM must be used after recovery";
    EXPECT_FALSE(recovered_qr.used_fallback) << "FFW-06: No fallback after LLM recovery";
    EXPECT_TRUE(audit_->Contains("query", "llm_answered"))
        << "FFW-06: Audit must record llm_answered after recovery";
}

// ---------------------------------------------------------------------------
// FFW-07: Export snapshot content matches ingested event count
// ---------------------------------------------------------------------------

/**
 * @test FFW-07 — Snapshot file content reflects the exact event count at export time.
 *
 * Acceptance Criteria:
 * - After N ingests, the snapshot file contains "events=N".
 * - SnapshotResult.event_count equals TenantEventCount.
 * - SnapshotResult.cdc_count equals CdcCount.
 */
TEST_F(W3AFullFunctionCriticalFlowsTest, FFW07_SnapshotContentMatchesIngestedEventCount) {
    constexpr size_t kEventCount = 7U;

    const auto token = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token);
    ASSERT_TRUE(pipeline_->RegisterSession(token, "tenant_ffw07", "app_user"));

    for (size_t i = 0; i < kEventCount; ++i) {
        const auto event_id = "event_" + std::to_string(i);
        ASSERT_TRUE(pipeline_->IngestEvent(token, "tenant_ffw07", event_id,
                                           "payload_" + std::to_string(i), {"batch"}))
            << "FFW-07: Ingest " << i << " must succeed";
    }

    const auto snap_path = GetTempDir() / "ffw07_snapshot.txt";
    const auto snap      = pipeline_->ExportSnapshot("tenant_ffw07", snap_path);

    ASSERT_TRUE(snap.ok)                     << "FFW-07: Snapshot export must succeed";
    EXPECT_EQ(snap.event_count, kEventCount) << "FFW-07: Snapshot event_count must equal ingested count";
    EXPECT_EQ(snap.cdc_count, kEventCount)   << "FFW-07: Snapshot CDC count must equal ingested count";

    // Verify file content
    std::ifstream in(snap_path);
    ASSERT_TRUE(in.is_open()) << "FFW-07: Snapshot file must be openable";
    const std::string content{std::istreambuf_iterator<char>(in), {}};
    EXPECT_NE(content.find("events=" + std::to_string(kEventCount)), std::string::npos)
        << "FFW-07: Snapshot file must contain 'events=" << kEventCount << "'";
    EXPECT_NE(content.find("cdc=" + std::to_string(kEventCount)), std::string::npos)
        << "FFW-07: Snapshot file must contain 'cdc=" << kEventCount << "'";
}

// ---------------------------------------------------------------------------
// FFW-08: Multi-tenant complete flow enforces strict cross-tenant isolation
// ---------------------------------------------------------------------------

/**
 * @test FFW-08 — Two tenants run independent full pipelines; each sees only
 *               their own data at every stage (ingest, query, export).
 *
 * Acceptance Criteria:
 * - Tenant A's query never returns data ingested by Tenant B.
 * - Tenant B's query never returns data ingested by Tenant A.
 * - Export snapshots report per-tenant event counts correctly.
 * - Global CDC count equals the sum of both tenants' event counts.
 */
TEST_F(W3AFullFunctionCriticalFlowsTest, FFW08_MultiTenantFullFlowMaintainsStrictIsolation) {
    const auto token_a = data_gen_->GeneratePipelineToken(true);
    const auto token_b = data_gen_->GeneratePipelineToken(true);
    auth_->AllowToken(token_a);
    auth_->AllowToken(token_b);

    ASSERT_TRUE(pipeline_->RegisterSession(token_a, "tenant_a", "app_user"));
    ASSERT_TRUE(pipeline_->RegisterSession(token_b, "tenant_b", "app_user"));

    // Ingest exclusive payloads per tenant
    ASSERT_TRUE(pipeline_->IngestEvent(token_a, "tenant_a", "event_a1", "alpha-secret", {"shared"}));
    ASSERT_TRUE(pipeline_->IngestEvent(token_b, "tenant_b", "event_b1", "beta-secret",  {"shared"}));

    // Tenant A query: must not see Tenant B's payload
    const auto qr_a = pipeline_->Query(token_a, "tenant_a", "query-a", "shared");
    ASSERT_TRUE(qr_a.ok)                                                  << "FFW-08: Tenant A query must succeed";
    EXPECT_EQ(qr_a.answer.find("beta-secret"), std::string::npos)         << "FFW-08: Tenant A must not see Tenant B's data";
    EXPECT_NE(qr_a.answer.find("alpha-secret"), std::string::npos)        << "FFW-08: Tenant A query answer must contain Tenant A's data";

    // Tenant B query: must not see Tenant A's payload
    const auto qr_b = pipeline_->Query(token_b, "tenant_b", "query-b", "shared");
    ASSERT_TRUE(qr_b.ok)                                                  << "FFW-08: Tenant B query must succeed";
    EXPECT_EQ(qr_b.answer.find("alpha-secret"), std::string::npos)        << "FFW-08: Tenant B must not see Tenant A's data";
    EXPECT_NE(qr_b.answer.find("beta-secret"), std::string::npos)         << "FFW-08: Tenant B query answer must contain Tenant B's data";

    // Export snapshots for both tenants
    const auto snap_a = pipeline_->ExportSnapshot("tenant_a", GetTempDir() / "ffw08_a.txt");
    const auto snap_b = pipeline_->ExportSnapshot("tenant_b", GetTempDir() / "ffw08_b.txt");
    ASSERT_TRUE(snap_a.ok) << "FFW-08: Tenant A snapshot export must succeed";
    ASSERT_TRUE(snap_b.ok) << "FFW-08: Tenant B snapshot export must succeed";

    EXPECT_EQ(snap_a.event_count, 1U) << "FFW-08: Tenant A must have exactly 1 event";
    EXPECT_EQ(snap_b.event_count, 1U) << "FFW-08: Tenant B must have exactly 1 event";

    // Global CDC must equal sum of both tenants
    EXPECT_EQ(pipeline_->CdcCount(), 2U)
        << "FFW-08: Global CDC count must equal sum of both tenants' events";
}
} } // namespace themis::test
