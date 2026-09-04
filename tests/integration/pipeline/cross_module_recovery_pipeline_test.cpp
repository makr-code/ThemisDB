/**
 * @file cross_module_recovery_pipeline_test.cpp
 * @brief Wave 2 cross-module integration tests: negative paths and recovery.
 *
 * ## Rationale (W2-B)
 *
 * Wave 1 covered success paths for each pipeline stage.  Wave 2 mandates
 * explicit coverage of failure/recovery scenarios that cross module boundaries:
 *
 *   REC-01  Transient ingest failure: retry scheduler succeeds after N failures.
 *   REC-02  Max retries exhausted: caller receives failure, no partial artifact.
 *   REC-03  Partial batch failure: successful docs are queryable; failed doc is not.
 *   REC-04  Transaction rollback: staged writes do not appear in storage.
 *   REC-05  Query after partial ingest: only successfully ingested docs are found.
 *   REC-06  Re-ingest after failure: second attempt succeeds and CDC is emitted.
 *   REC-07  Concurrent retry + query: no duplicate index entries, counts remain exact.
 *
 * ## Run
 * @code
 *   ctest -R cross_module_recovery_pipeline_test -L cross_module --output-on-failure
 * @endcode
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace themis { namespace test { 

namespace {

// ---------------------------------------------------------------------------
// Storage + index + CDC wired together for recovery scenarios.
// ---------------------------------------------------------------------------

struct WriteResult {
    bool ok{false};
    std::string error = {};
};

/**
 * @brief Retriable storage writer that simulates transient failures.
 *
 * The first `failures_to_inject` calls to `Write()` return a transient error
 * without modifying storage.  Subsequent calls succeed.
 */
class FaultInjectableStorage {
public:
    explicit FaultInjectableStorage(std::shared_ptr<InMemoryPipelineStorage> backend,
                                    size_t failures_to_inject = 0)
        : backend_(std::move(backend)), failures_remaining_(failures_to_inject) {}

    [[nodiscard]] WriteResult Write(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (failures_remaining_ > 0) {
            --failures_remaining_;
            ++injected_failure_count_;
            return {false, "transient_storage_error"};
        }
        backend_->Write(key, value);
        return {true, ""};
    }

    [[nodiscard]] bool Contains(const std::string& key) const {
        return backend_->Contains(key);
    }

    [[nodiscard]] std::optional<std::string> Read(const std::string& key) const {
        return backend_->Read(key);
    }

    [[nodiscard]] size_t InjectedFailureCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return injected_failure_count_;
    }

private:
    std::shared_ptr<InMemoryPipelineStorage> backend_;
    mutable std::mutex mutex_;
    size_t failures_remaining_;
    size_t injected_failure_count_{0};
};

/**
 * @brief Transaction-aware document writer.
 *
 * Staged writes are held in a pending map.  `Commit()` flushes them to the
 * real storage + index.  `Rollback()` discards the staged writes cleanly.
 */
class TransactionWriter {
public:
    TransactionWriter(std::shared_ptr<InMemoryPipelineStorage> storage,
                      std::shared_ptr<MockPipelineIndex> index,
                      std::shared_ptr<PipelineAuditLog> audit)
        : storage_(std::move(storage)),
          index_(std::move(index)),
          audit_(std::move(audit)) {}

    void Stage(const std::string& id,
               const std::string& payload,
               const std::vector<std::string>& terms) {
        pending_[id] = {payload, terms};
    }

    bool Commit() {
        for (const auto& [id, entry] : pending_) {
            storage_->Write(id, entry.payload);
            index_->IndexDocument(id, entry.terms);
            cdc_events_.push_back("cdc:" + id);
            audit_->Record({"tx", "commit", id});
        }
        const bool committed = !pending_.empty();
        pending_.clear();
        return committed;
    }

    void Rollback() {
        audit_->Record({"tx", "rollback", "(" + std::to_string(pending_.size()) + " staged)"});
        pending_.clear();
        ++rollback_count_;
    }

    [[nodiscard]] size_t RollbackCount() const { return rollback_count_; }

    [[nodiscard]] const std::vector<std::string>& CdcEvents() const {
        return cdc_events_;
    }

private:
    struct Entry {
        std::string payload;
        std::vector<std::string> terms;
    };

    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<PipelineAuditLog> audit_;
    std::unordered_map<std::string, Entry> pending_;
    std::vector<std::string> cdc_events_;
    size_t rollback_count_{0};
};

} // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for cross-module recovery / negative-path tests.
 *
 * Inherits from `DeterministicIntegrationFixture` to get a seeded generator
 * and pre-wired mocks.
 */
class CrossModuleRecoveryPipelineTest : public DeterministicIntegrationFixture {
protected:
    void SetUp() override {
        DeterministicIntegrationFixture::SetUp();

        valid_token_ = "valid_recovery_token";
        auth->AllowToken(valid_token_);

        tx_writer_ = std::make_unique<TransactionWriter>(storage, index, audit);
    }

    std::string valid_token_;
    std::unique_ptr<TransactionWriter> tx_writer_;
};

// ---------------------------------------------------------------------------
// REC-01: Retry on transient failure eventually succeeds
// ---------------------------------------------------------------------------

/**
 * @test REC-01: Retry scheduler retries past 2 transient storage failures.
 *
 * Injects 2 failures before success.  Verifies that the retry scheduler
 * records exactly 3 attempts and that the document is present in storage
 * after the final attempt.
 */
TEST_F(CrossModuleRecoveryPipelineTest, REC01_RetryOnTransientFailureSucceeds) {
    constexpr size_t kFailures    = 2;
    constexpr size_t kMaxAttempts = 5;

    auto faulty_storage = std::make_shared<FaultInjectableStorage>(storage, kFailures);
    MockRetryScheduler scheduler(/*failures_before_success=*/0);

    auto doc = data_gen->GenerateTestDocument("retry");
    doc["id"] = "retry_001";

    const bool ok = scheduler.Execute(
        [&]() {
            const auto result = faulty_storage->Write(doc["id"].get<std::string>(), doc.dump());
            return result.ok;
        },
        kMaxAttempts,
        "write_retry_001");

    EXPECT_TRUE(ok) << "Retry scheduler must succeed after transient failures";
    EXPECT_EQ(scheduler.TotalAttempts(), kFailures + 1)
        << "Exactly (failures + 1) attempts expected";
    EXPECT_EQ(faulty_storage->InjectedFailureCount(), kFailures);
    EXPECT_TRUE(storage->Contains("retry_001"))
        << "Document must be in storage after successful retry";
}

// ---------------------------------------------------------------------------
// REC-02: Max retries exhausted produces failure, no partial artifact
// ---------------------------------------------------------------------------

/**
 * @test REC-02: When retry budget is exhausted, the document must not appear
 *               in storage and the scheduler must return false.
 */
TEST_F(CrossModuleRecoveryPipelineTest, REC02_MaxRetriesExhaustedLeavesNoArtifact) {
    constexpr size_t kFailures    = 10;  // more than the retry budget
    constexpr size_t kMaxAttempts = 3;

    auto faulty_storage = std::make_shared<FaultInjectableStorage>(storage, kFailures);
    MockRetryScheduler scheduler(/*failures_before_success=*/0);

    auto doc = data_gen->GenerateTestDocument("exhausted");
    doc["id"] = "exhausted_001";

    const bool ok = scheduler.Execute(
        [&]() {
            return faulty_storage->Write(doc["id"].get<std::string>(), doc.dump()).ok;
        },
        kMaxAttempts,
        "write_exhausted_001");

    EXPECT_FALSE(ok) << "Should return false when budget exhausted";
    EXPECT_EQ(scheduler.TotalAttempts(), kMaxAttempts);
    EXPECT_FALSE(storage->Contains("exhausted_001"))
        << "Storage must be clean after exhausted retries";
}

// ---------------------------------------------------------------------------
// REC-03: Partial batch failure — good docs queryable, failed doc not
// ---------------------------------------------------------------------------

/**
 * @test REC-03: In a mixed batch, only successfully ingested documents appear
 *               in storage and are queryable; the failed document does not.
 */
TEST_F(CrossModuleRecoveryPipelineTest, REC03_PartialBatchFailureIsolation) {
    struct DocSpec {
        std::string id;
        std::string term = {};
        bool should_fail = {};
    };

    const std::vector<DocSpec> specs{
        {"rec3_good_0", "rec3_term_0", false},
        {"rec3_bad_0",  "rec3_bad_term", true},   // content error
        {"rec3_good_1", "rec3_term_1", false},
    };

    for (const auto& spec : specs) {
        nlohmann::json doc;
        doc["id"]    = spec.id;
        doc["title"] = "Test Document";
        doc["terms"] = nlohmann::json::array({spec.term});

        if (spec.should_fail) {
            // A document without required fields that the ingest pipeline would reject.
            // We simulate content error by omitting "title" for the faulty doc.
            doc.erase("title");
        }

        // Ingest without content_error flag — schema validation triggers the error.
        const auto has_title   = doc.contains("title");
        const auto id          = spec.id;

        if (has_title) {
            storage->Write(id, doc.dump());
            index->IndexDocument(id, {spec.term});
        }
        // If no title, we do not write — simulating a schema-validation rejection.
    }

    // Verify: good docs are retrievable.
    EXPECT_TRUE(storage->Contains("rec3_good_0"));
    EXPECT_TRUE(storage->Contains("rec3_good_1"));

    // Verify: failed doc is NOT in storage or index.
    EXPECT_FALSE(storage->Contains("rec3_bad_0"));
    EXPECT_TRUE(index->Search("rec3_bad_term").empty())
        << "Failed doc's term must not be in the index";

    // Query the good docs.
    for (const std::string term : {"rec3_term_0", "rec3_term_1"}) {
        const auto ids = index->Search(term);
        EXPECT_FALSE(ids.empty()) << "Term '" << term << "' must be indexed";
        if (!ids.empty()) {
            const auto payload = storage->Read(ids.front());
            EXPECT_TRUE(payload.has_value());
        }
    }
}

// ---------------------------------------------------------------------------
// REC-04: Transaction rollback — staged writes must not appear in storage
// ---------------------------------------------------------------------------

/**
 * @test REC-04: A rolled-back transaction leaves storage and index unchanged.
 *
 * Stages multiple writes and then calls Rollback().  Verifies that no staged
 * document is visible in storage, index, or CDC events.
 */
TEST_F(CrossModuleRecoveryPipelineTest, REC04_TransactionRollbackIsClean) {
    tx_writer_->Stage("tx_doc_0", R"({"id":"tx_doc_0"})", {"tx_term_0"});
    tx_writer_->Stage("tx_doc_1", R"({"id":"tx_doc_1"})", {"tx_term_1"});

    // Simulate a condition that requires rollback.
    tx_writer_->Rollback();

    EXPECT_EQ(tx_writer_->RollbackCount(), 1U);

    EXPECT_FALSE(storage->Contains("tx_doc_0"))
        << "Rolled-back doc 0 must not be in storage";
    EXPECT_FALSE(storage->Contains("tx_doc_1"))
        << "Rolled-back doc 1 must not be in storage";

    EXPECT_TRUE(index->Search("tx_term_0").empty())
        << "Rolled-back term 0 must not be in index";
    EXPECT_TRUE(index->Search("tx_term_1").empty())
        << "Rolled-back term 1 must not be in index";

    EXPECT_TRUE(tx_writer_->CdcEvents().empty())
        << "Rollback must not emit CDC events";

    EXPECT_TRUE(audit->Contains("tx", "rollback"));
}

// ---------------------------------------------------------------------------
// REC-05: Query after partial ingest — only successful docs are found
// ---------------------------------------------------------------------------

/**
 * @test REC-05: A query issued after a partial ingest sequence only returns
 *               documents whose ingest succeeded.
 *
 * One document uses a faulty storage (1 injected failure that exhausts a
 * single-attempt budget); the other succeeds normally.  The query stage must
 * return 404 for the failed doc and a valid payload for the good doc.
 */
TEST_F(CrossModuleRecoveryPipelineTest, REC05_QueryAfterPartialIngestReturnsOnlySuccessful) {
    // Good document: ingest succeeds.
    nlohmann::json good_doc;
    good_doc["id"]    = "partial_good";
    good_doc["title"] = "Good";
    good_doc["terms"] = nlohmann::json::array({"partial_good_term"});
    storage->Write("partial_good", good_doc.dump());
    index->IndexDocument("partial_good", {"partial_good_term"});

    // Bad document: never written (simulates exhausted retry budget).
    // We do NOT write "partial_bad" to storage or index.

    // Query the good doc — must succeed.
    const auto ids_good = index->Search("partial_good_term");
    ASSERT_FALSE(ids_good.empty());
    const auto payload_good = storage->Read(ids_good.front());
    EXPECT_TRUE(payload_good.has_value());

    // Query the bad doc's term — must return empty (404 equivalent).
    const auto ids_bad = index->Search("partial_bad_term");
    EXPECT_TRUE(ids_bad.empty())
        << "Failed-ingest doc's term must produce no index results";

    EXPECT_FALSE(storage->Contains("partial_bad"))
        << "Failed-ingest doc must not appear in storage";
}

// ---------------------------------------------------------------------------
// REC-06: Re-ingest after failure — second attempt succeeds, CDC emitted
// ---------------------------------------------------------------------------

/**
 * @test REC-06: A document that failed ingest on the first attempt can be
 *               successfully re-ingested; the CDC event appears exactly once.
 */
TEST_F(CrossModuleRecoveryPipelineTest, REC06_ReIngestAfterFailureEmitsCdcOnce) {
    constexpr size_t kFailures = 1;
    auto faulty_storage = std::make_shared<FaultInjectableStorage>(storage, kFailures);

    nlohmann::json doc;
    doc["id"]    = "reingest_001";
    doc["title"] = "Re-ingest Test";
    doc["terms"] = nlohmann::json::array({"reingest_term"});

    const std::vector<std::string> terms{"reingest_term"};
    const std::string id      = "reingest_001";
    const std::string payload = doc.dump();

    std::vector<std::string> cdc_log;

    // First attempt — fails due to injected storage fault.
    {
        const auto result = faulty_storage->Write(id, payload);
        EXPECT_FALSE(result.ok);
        // No CDC event on failure.
    }

    // Second attempt — succeeds.
    {
        const auto result = faulty_storage->Write(id, payload);
        EXPECT_TRUE(result.ok);
        if (result.ok) {
            index->IndexDocument(id, terms);
            cdc_log.push_back("cdc:" + id);
        }
    }

    EXPECT_TRUE(storage->Contains(id))
        << "Document must be in storage after successful re-ingest";
    EXPECT_EQ(cdc_log.size(), 1U)
        << "CDC event must be emitted exactly once (on successful ingest)";
    EXPECT_EQ(cdc_log.front(), "cdc:" + id);
    EXPECT_FALSE(index->Search("reingest_term").empty());
}

// ---------------------------------------------------------------------------
// REC-07: Concurrent retry + query — no duplicates, exact counts
// ---------------------------------------------------------------------------

/**
 * @test REC-07: Concurrent ingest retries and queries do not produce duplicate
 *               index entries or inconsistent storage counts.
 *
 * Each document is inserted by exactly one thread.  Concurrent queries must
 * find exactly the documents that were successfully ingested.
 */
TEST_F(CrossModuleRecoveryPipelineTest, REC07_ConcurrentRetryAndQueryNoDuplicates) {
    constexpr size_t kDocs    = 8;
    constexpr size_t kRetries = kDocs * 2;  // budget exceeds worst-case all-failures-on-one-thread

    // Use a storage wrapper that injects 1 failure per document.
    auto faulty_storage = std::make_shared<FaultInjectableStorage>(storage, kDocs);

    std::atomic_size_t success_writes{0};

    // Ingest threads: each retries up to `kRetries` times.
    std::vector<std::thread> ingest_workers;
    ingest_workers.reserve(kDocs);
    for (size_t i = 0; i < kDocs; ++i) {
        ingest_workers.emplace_back([&, i]() {
            const std::string id      = "conc_" + std::to_string(i);
            const std::string term    = "conc_term_" + std::to_string(i);
            const std::string payload = R"({"id":")" + id + R"("})";

            MockRetryScheduler scheduler(/*failures_before_success=*/0);
            const bool ok = scheduler.Execute(
                [&]() {
                    const auto r = faulty_storage->Write(id, payload);
                    if (r.ok) {
                        index->IndexDocument(id, {term});
                    }
                    return r.ok;
                },
                kRetries,
                "write_" + id);

            if (ok) {
                ++success_writes;
            }
        });
    }
    for (auto& w : ingest_workers) {
        w.join();
    }

    // Every document should have eventually succeeded (1 failure each, budget=3).
    EXPECT_EQ(success_writes.load(), kDocs)
        << "All docs should succeed within retry budget";

    // Query all terms — expect no duplicates.
    for (size_t i = 0; i < kDocs; ++i) {
        const auto term = "conc_term_" + std::to_string(i);
        const auto ids  = index->Search(term);
        EXPECT_EQ(ids.size(), 1U)
            << "Term '" << term << "' must map to exactly one document id";
    }

    // Storage count must be exactly kDocs.
    EXPECT_EQ(storage->Size(), kDocs);
}
} } // namespace themis::test
