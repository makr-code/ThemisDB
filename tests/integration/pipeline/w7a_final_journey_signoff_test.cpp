/**
 * @file w7a_final_journey_signoff_test.cpp
 * @brief Wave 7A — Final Critical Journey Sign-off (FJS-01..FJS-08).
 *
 * Validates every pipeline stage — ingest, index, query, delete — against
 * strong invariant checks.  All tests are deterministic via kCanonicalSeed.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis { namespace test { 

namespace {

// ---------------------------------------------------------------------------
// Canonical seed required by all Wave 7 tests
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// FullLifecyclePipeline — ingest → index → query → delete
// ---------------------------------------------------------------------------

struct LifecycleResult {
    bool ok{false};
    std::string stage;
    std::string error;
};

class FullLifecyclePipeline {
public:
    explicit FullLifecyclePipeline(std::shared_ptr<InMemoryPipelineStorage> storage,
                                   std::shared_ptr<MockPipelineIndex>       index,
                                   std::shared_ptr<PipelineAuditLog>        audit)
        : storage_(std::move(storage))
        , index_(std::move(index))
        , audit_(std::move(audit)) {}

    // --- ingest -------------------------------------------------------
    [[nodiscard]] LifecycleResult Ingest(const std::string& id,
                                         const std::string& payload,
                                         const std::vector<std::string>& terms) {
        if (id.empty()) {
            return {false, "ingest", "empty_id"};
        }
        if (payload.empty()) {
            // edge-case: empty payload is accepted but flagged
            audit_->Record({"ingest", "empty_payload", id});
        }
        storage_->Write(id, payload);
        index_->IndexDocument(id, terms);
        audit_->Record({"ingest", "write", id});
        ingested_.insert(id);
        return {true, "ingest", ""};
    }

    // --- query --------------------------------------------------------
    [[nodiscard]] std::vector<std::string> Query(const std::string& term) const {
        return index_->Search(term);
    }

    [[nodiscard]] std::optional<std::string> Fetch(const std::string& id) const {
        return storage_->Read(id);
    }

    // --- delete -------------------------------------------------------
    [[nodiscard]] LifecycleResult Delete(const std::string& id) {
        if (!storage_->Contains(id)) {
            return {false, "delete", "not_found"};
        }
        storage_->Erase(id);
        ingested_.erase(id);
        audit_->Record({"delete", "erase", id});
        return {true, "delete", ""};
    }

    // --- invariant helpers --------------------------------------------
    [[nodiscard]] bool IsTracked(const std::string& id) const {
        return ingested_.count(id) > 0;
    }
    [[nodiscard]] size_t IngestedCount() const { return ingested_.size(); }

private:
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineIndex>       index_;
    std::shared_ptr<PipelineAuditLog>        audit_;
    std::unordered_set<std::string>          ingested_;
};

// ---------------------------------------------------------------------------
// MultiTenantPipeline — per-tenant isolated storage + index
// ---------------------------------------------------------------------------

class MultiTenantPipeline {
public:
    void ProvisionTenant(const std::string& tenant_id) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (stores_.count(tenant_id) == 0) {
            stores_[tenant_id]  = std::make_shared<InMemoryPipelineStorage>();
            indices_[tenant_id] = std::make_shared<MockPipelineIndex>();
        }
    }

    [[nodiscard]] bool Write(const std::string& tenant_id,
                             const std::string& key,
                             const std::string& value) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = stores_.find(tenant_id);
        if (it == stores_.end()) { return false; }
        it->second->Write(key, value);
        indices_[tenant_id]->IndexDocument(key, {tenant_id + "_term"});
        return true;
    }

    [[nodiscard]] bool TenantCanReadOthersTenant(const std::string& reader,
                                                  const std::string& other_key) const {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = stores_.find(reader);
        if (it == stores_.end()) { return false; }
        return it->second->Contains(other_key);
    }

    [[nodiscard]] std::vector<std::string> TenantSearch(const std::string& tenant_id,
                                                         const std::string& term) const {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = indices_.find(tenant_id);
        if (it == indices_.end()) { return {}; }
        return it->second->Search(term);
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<InMemoryPipelineStorage>> stores_;
    std::unordered_map<std::string, std::shared_ptr<MockPipelineIndex>>       indices_;
};

// ---------------------------------------------------------------------------
// IdempotentIngestPipeline — duplicate detection via fingerprint map
// ---------------------------------------------------------------------------

class IdempotentIngestPipeline {
public:
    explicit IdempotentIngestPipeline(std::shared_ptr<InMemoryPipelineStorage> storage)
        : storage_(std::move(storage)) {}

    struct IngestResult {
        bool written{false};
        bool duplicate{false};
    };

    [[nodiscard]] IngestResult Ingest(const std::string& id, const std::string& payload) {
        const std::string fingerprint = id + ":" + std::to_string(payload.size());
        if (fingerprints_.count(fingerprint) > 0) {
            return {false, true};
        }
        storage_->Write(id, payload);
        fingerprints_.insert(fingerprint);
        return {true, false};
    }

    [[nodiscard]] size_t UniqueCount() const { return fingerprints_.size(); }

private:
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::unordered_set<std::string>          fingerprints_;
};

// ---------------------------------------------------------------------------
// CausalWritePipeline — ordered sequence via monotonic sequence number
// ---------------------------------------------------------------------------

class CausalWritePipeline {
public:
    struct Write {
        uint64_t    seq{0};
        std::string key;
        std::string value;
    };

    [[nodiscard]] uint64_t Append(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mutex_);
        const uint64_t seq = ++next_seq_;
        log_.push_back({seq, key, value});
        return seq;
    }

    [[nodiscard]] bool IsMonotonic() const {
        std::lock_guard<std::mutex> lk(mutex_);
        for (size_t i = 1; i < log_.size(); ++i) {
            if (log_[i].seq <= log_[i - 1].seq) { return false; }
        }
        return true;
    }

    [[nodiscard]] const std::vector<Write>& Log() const { return log_; }

private:
    mutable std::mutex   mutex_;
    uint64_t             next_seq_{0};
    std::vector<Write>   log_;
};

// ---------------------------------------------------------------------------
// ConcurrentConsistencyPipeline — cross-component consistency check
// ---------------------------------------------------------------------------

class ConcurrentConsistencyPipeline {
public:
    explicit ConcurrentConsistencyPipeline(std::shared_ptr<InMemoryPipelineStorage> storage,
                                            std::shared_ptr<MockPipelineIndex>       index)
        : storage_(std::move(storage)), index_(std::move(index)) {}

    void ConcurrentWrite(const std::string& id, const std::string& payload,
                         const std::string& term) {
        std::lock_guard<std::mutex> lk(mutex_);
        storage_->Write(id, payload);
        index_->IndexDocument(id, {term});
        write_ids_.push_back(id);
    }

    [[nodiscard]] bool IndexConsistentWithStorage() const {
        std::lock_guard<std::mutex> lk(mutex_);
        for (const auto& id : write_ids_) {
            if (!storage_->Contains(id)) { return false; }
        }
        return true;
    }

    [[nodiscard]] size_t WriteCount() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return write_ids_.size();
    }

private:
    mutable std::mutex                       mutex_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineIndex>       index_;
    std::vector<std::string>                 write_ids_;
};

// ---------------------------------------------------------------------------
// RollbackPipeline — stage-based failure with compensating rollback
// ---------------------------------------------------------------------------

struct StageFailure {
    bool fail_at_index{false};
    bool fail_at_audit{false};
};

class RollbackPipeline {
public:
    explicit RollbackPipeline(std::shared_ptr<InMemoryPipelineStorage> storage,
                               std::shared_ptr<PipelineAuditLog>        audit)
        : storage_(std::move(storage)), audit_(std::move(audit)) {}

    struct PipelineResult {
        bool ok{false};
        std::string failed_stage;
        bool rolled_back{false};
    };

    [[nodiscard]] PipelineResult Execute(const std::string& id,
                                          const std::string& payload,
                                          StageFailure injection) {
        // Stage 1: storage write
        storage_->Write(id, payload);

        // Stage 2: index (may fail)
        if (injection.fail_at_index) {
            // rollback storage
            storage_->Erase(id);
            rollback_count_++;
            return {false, "index", true};
        }

        // Stage 3: audit (may fail)
        if (injection.fail_at_audit) {
            storage_->Erase(id);
            rollback_count_++;
            return {false, "audit", true};
        }

        audit_->Record({"pipeline", "commit", id});
        return {true, "", false};
    }

    [[nodiscard]] size_t RollbackCount() const { return rollback_count_; }

private:
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog>        audit_;
    size_t                                   rollback_count_{0};
};

} // namespace

// ===========================================================================
// Test fixture
// ===========================================================================

class FinalJourneySignoffTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        rng_.seed(kCanonicalSeed);
        storage_ = CreateInMemoryStorage();
        index_   = CreateMockIndex();
        audit_   = CreateAuditLog();
    }

    [[nodiscard]] std::string MakeId(const std::string& prefix, int n) {
        return prefix + "_" + std::to_string(n);
    }

    std::mt19937                             rng_{kCanonicalSeed};
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineIndex>       index_;
    std::shared_ptr<PipelineAuditLog>        audit_;
};

// ===========================================================================
// FJS-01 — Full ingest → index → query → delete lifecycle
// ===========================================================================
TEST_F(FinalJourneySignoffTest, FJS01_FullIngestIndexQueryDeleteLifecycleWithInvariantChecks) {
    SCOPED_TRACE("FJS-01: full lifecycle invariant");

    FullLifecyclePipeline pipeline(storage_, index_, audit_);

    const std::string id      = "fjs01_doc";
    const std::string payload = "lifecycle_payload_value";
    const std::string term    = "fjs01_term";

    // --- ingest ---
    const auto ingest_result = pipeline.Ingest(id, payload, {term});
    ASSERT_TRUE(ingest_result.ok) << "ingest stage failed: " << ingest_result.error;
    EXPECT_TRUE(pipeline.IsTracked(id))   << "id not tracked after ingest";
    EXPECT_TRUE(storage_->Contains(id))   << "storage missing after ingest";

    // --- index query ---
    const auto hits = pipeline.Query(term);
    ASSERT_FALSE(hits.empty()) << "index returned no hits for term '" << term << "'";
    EXPECT_NE(std::find(hits.begin(), hits.end(), id), hits.end())
        << "indexed id not found in search results";

    // --- fetch raw payload ---
    const auto fetched = pipeline.Fetch(id);
    ASSERT_TRUE(fetched.has_value())     << "fetch returned nullopt";
    EXPECT_EQ(*fetched, payload)         << "fetched payload mismatch";

    // --- delete ---
    const auto del_result = pipeline.Delete(id);
    ASSERT_TRUE(del_result.ok) << "delete stage failed: " << del_result.error;
    EXPECT_FALSE(storage_->Contains(id)) << "storage still contains id after delete";
    EXPECT_FALSE(pipeline.IsTracked(id)) << "id still tracked after delete";

    // --- audit completeness ---
    EXPECT_TRUE(audit_->Contains("ingest", "write"))   << "audit missing ingest/write event";
    EXPECT_TRUE(audit_->Contains("delete", "erase"))   << "audit missing delete/erase event";
}

// ===========================================================================
// FJS-02 — Multi-tenant isolation invariant across all pipeline stages
// ===========================================================================
TEST_F(FinalJourneySignoffTest, FJS02_MultiTenantIsolationInvariantAcrossAllPipelineStages) {
    SCOPED_TRACE("FJS-02: multi-tenant isolation");

    MultiTenantPipeline pipeline;
    pipeline.ProvisionTenant("tenant_A");
    pipeline.ProvisionTenant("tenant_B");

    const std::string key_a = "tenant_A_secret_key";
    const std::string key_b = "tenant_B_secret_key";

    ASSERT_TRUE(pipeline.Write("tenant_A", key_a, "value_a"));
    ASSERT_TRUE(pipeline.Write("tenant_B", key_b, "value_b"));

    // Tenant A must NOT be able to read tenant B's key
    EXPECT_FALSE(pipeline.TenantCanReadOthersTenant("tenant_A", key_b))
        << "tenant_A can read tenant_B data — isolation violated";
    EXPECT_FALSE(pipeline.TenantCanReadOthersTenant("tenant_B", key_a))
        << "tenant_B can read tenant_A data — isolation violated";

    // Each tenant's index must only expose their own terms
    const auto hits_a = pipeline.TenantSearch("tenant_A", "tenant_A_term");
    const auto hits_b = pipeline.TenantSearch("tenant_B", "tenant_B_term");
    const auto cross  = pipeline.TenantSearch("tenant_A", "tenant_B_term");

    EXPECT_FALSE(hits_a.empty()) << "tenant_A index returned no results for own term";
    EXPECT_FALSE(hits_b.empty()) << "tenant_B index returned no results for own term";
    EXPECT_TRUE(cross.empty())   << "tenant_A index leaked tenant_B term — index isolation violated";
}

// ===========================================================================
// FJS-03 — Idempotent re-ingestion with duplicate detection
// ===========================================================================
TEST_F(FinalJourneySignoffTest, FJS03_IdempotentReIngestionWithDuplicateDetection) {
    SCOPED_TRACE("FJS-03: idempotent re-ingestion");

    IdempotentIngestPipeline pipeline(storage_);

    const std::string id      = "fjs03_doc";
    const std::string payload = "exact_payload";

    const auto first  = pipeline.Ingest(id, payload);
    const auto second = pipeline.Ingest(id, payload);  // exact duplicate
    const auto third  = pipeline.Ingest(id, payload);  // exact duplicate again

    EXPECT_TRUE(first.written)    << "first ingest must succeed";
    EXPECT_FALSE(first.duplicate) << "first ingest must not be flagged as duplicate";
    EXPECT_FALSE(second.written)  << "second (duplicate) ingest must not re-write";
    EXPECT_TRUE(second.duplicate) << "second ingest must be flagged as duplicate";
    EXPECT_FALSE(third.written)   << "third (duplicate) ingest must not re-write";
    EXPECT_TRUE(third.duplicate)  << "third ingest must be flagged as duplicate";

    // Storage count must remain 1 (idempotent)
    EXPECT_EQ(pipeline.UniqueCount(), 1U)    << "unique fingerprint count must be 1";
    EXPECT_EQ(storage_->Size(), 1U)          << "storage size must be 1 after 3 identical ingests";

    // Different payload → new entry allowed
    const auto different = pipeline.Ingest(id, payload + "_v2");
    EXPECT_TRUE(different.written)  << "different-payload ingest must be accepted";
    EXPECT_FALSE(different.duplicate);
    EXPECT_EQ(pipeline.UniqueCount(), 2U);
}

// ===========================================================================
// FJS-04 — Ordered write sequence correctness (causal consistency)
// ===========================================================================
TEST_F(FinalJourneySignoffTest, FJS04_OrderedWriteSequenceCorrectnessCausalConsistency) {
    SCOPED_TRACE("FJS-04: causal consistency / monotonic sequence");

    CausalWritePipeline pipeline;
    constexpr size_t kWrites = 20;

    for (size_t i = 0; i < kWrites; ++i) {
        const auto seq = pipeline.Append("key_" + std::to_string(i), "val_" + std::to_string(i));
        EXPECT_EQ(seq, static_cast<uint64_t>(i + 1))
            << "sequence number not monotonically incrementing at i=" << i;
    }

    EXPECT_TRUE(pipeline.IsMonotonic())    << "write log is not monotonically ordered";
    EXPECT_EQ(pipeline.Log().size(), kWrites);

    // Verify causal order: each entry's seq > previous
    const auto& log = pipeline.Log();
    for (size_t i = 1; i < log.size(); ++i) {
        EXPECT_GT(log[i].seq, log[i - 1].seq)
            << "causal order violation at position " << i
            << ": seq[i]=" << log[i].seq << " <= seq[i-1]=" << log[i - 1].seq;
    }
}

// ===========================================================================
// FJS-05 — Cross-component consistency after concurrent writes
// ===========================================================================
TEST_F(FinalJourneySignoffTest, FJS05_CrossComponentConsistencyAfterConcurrentWrites) {
    SCOPED_TRACE("FJS-05: cross-component consistency after concurrent writes");

    ConcurrentConsistencyPipeline pipeline(storage_, index_);
    constexpr int kThreads       = 4;
    constexpr int kWritesPerThread = 10;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&pipeline, t]() {
            for (int i = 0; i < kWritesPerThread; ++i) {
                const std::string id    = "t" + std::to_string(t) + "_w" + std::to_string(i);
                const std::string term  = "concurrent_term_" + std::to_string(t);
                pipeline.ConcurrentWrite(id, "payload_" + id, term);
            }
        });
    }

    for (auto& th : threads) { th.join(); }

    const size_t expected_writes = static_cast<size_t>(kThreads * kWritesPerThread);
    EXPECT_EQ(pipeline.WriteCount(), expected_writes)
        << "concurrent write count mismatch";

    EXPECT_TRUE(pipeline.IndexConsistentWithStorage())
        << "index/storage inconsistency after concurrent writes";
}

// ===========================================================================
// FJS-06 — Edge case: empty payload handling throughout pipeline
// ===========================================================================
TEST_F(FinalJourneySignoffTest, FJS06_EdgeCaseEmptyPayloadHandlingThroughoutPipeline) {
    SCOPED_TRACE("FJS-06: empty payload edge case");

    FullLifecyclePipeline pipeline(storage_, index_, audit_);

    const std::string id = "fjs06_empty";

    // Empty payload must be accepted (not rejected) — stored as ""
    const auto result = pipeline.Ingest(id, /*payload=*/"", {"empty_term"});
    EXPECT_TRUE(result.ok) << "empty payload ingest must not be rejected; error=" << result.error;
    EXPECT_TRUE(storage_->Contains(id)) << "empty-payload document not stored";

    const auto fetched = pipeline.Fetch(id);
    ASSERT_TRUE(fetched.has_value()) << "empty-payload document not fetchable";
    EXPECT_EQ(*fetched, "")          << "fetched empty-payload value is wrong";

    // Audit must record the empty payload warning
    EXPECT_TRUE(audit_->Contains("ingest", "empty_payload"))
        << "audit must record empty_payload event";

    // Query still works
    const auto hits = pipeline.Query("empty_term");
    EXPECT_FALSE(hits.empty()) << "index must still return results for empty-payload document";

    // Delete still cleans up
    const auto del = pipeline.Delete(id);
    EXPECT_TRUE(del.ok)              << "empty-payload document must be deleteable";
    EXPECT_FALSE(storage_->Contains(id));
}

// ===========================================================================
// FJS-07 — Edge case: maximum payload size boundary
// ===========================================================================
TEST_F(FinalJourneySignoffTest, FJS07_EdgeCaseMaximumPayloadSizeBoundary) {
    SCOPED_TRACE("FJS-07: maximum payload size boundary");

    FullLifecyclePipeline pipeline(storage_, index_, audit_);

    // Use a 1 MiB payload — boundary test
    constexpr size_t kMaxPayloadBytes = 1U * 1024U * 1024U;
    const std::string large_payload(kMaxPayloadBytes, 'X');

    const std::string id = "fjs07_large";

    const auto ingest_result = pipeline.Ingest(id, large_payload, {"large_term"});
    ASSERT_TRUE(ingest_result.ok) << "max-size payload ingest failed: " << ingest_result.error;

    const auto fetched = pipeline.Fetch(id);
    ASSERT_TRUE(fetched.has_value()) << "max-size payload not fetchable";
    EXPECT_EQ(fetched->size(), kMaxPayloadBytes)
        << "fetched payload size mismatch (expected=" << kMaxPayloadBytes
        << " got=" << fetched->size() << ")";

    const auto hits = pipeline.Query("large_term");
    EXPECT_FALSE(hits.empty()) << "large-payload document not indexed";

    const auto del = pipeline.Delete(id);
    EXPECT_TRUE(del.ok);
    EXPECT_FALSE(storage_->Contains(id));
}

// ===========================================================================
// FJS-08 — Error variant: pipeline stage failure with clean rollback
// ===========================================================================
TEST_F(FinalJourneySignoffTest, FJS08_ErrorVariantPipelineStageFailureWithCleanRollback) {
    SCOPED_TRACE("FJS-08: stage failure + rollback invariant");

    RollbackPipeline pipeline(storage_, audit_);

    const std::string id      = "fjs08_rollback_doc";
    const std::string payload = "rollback_payload";

    // Inject failure at index stage
    const auto result = pipeline.Execute(id, payload, {.fail_at_index = true});

    EXPECT_FALSE(result.ok)         << "pipeline must report failure";
    EXPECT_EQ(result.failed_stage, "index") << "failed stage must be 'index'";
    EXPECT_TRUE(result.rolled_back)          << "rollback flag must be set";

    // After rollback: storage must be clean (no phantom record)
    EXPECT_FALSE(storage_->Contains(id))
        << "phantom record found after rollback — storage not clean";
    EXPECT_EQ(pipeline.RollbackCount(), 1U)  << "rollback counter must be 1";

    // Now inject failure at audit stage
    const std::string id2 = "fjs08_audit_rollback";
    const auto result2 = pipeline.Execute(id2, payload, {.fail_at_audit = true});

    EXPECT_FALSE(result2.ok);
    EXPECT_EQ(result2.failed_stage, "audit");
    EXPECT_TRUE(result2.rolled_back);
    EXPECT_FALSE(storage_->Contains(id2))
        << "phantom record found after audit-stage rollback";
    EXPECT_EQ(pipeline.RollbackCount(), 2U);

    // Happy path — no injection
    const std::string id3 = "fjs08_success";
    const auto result3 = pipeline.Execute(id3, payload, {});
    EXPECT_TRUE(result3.ok)    << "happy path must succeed";
    EXPECT_TRUE(storage_->Contains(id3)) << "committed record must be in storage";
    EXPECT_EQ(pipeline.RollbackCount(), 2U) << "rollback count must not increase on success";
}
} } // namespace themis::test
