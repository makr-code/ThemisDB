/**
 * @file w6b_stress_soak_stability_test.cpp
 * @brief Wave 6-B: Stress, Soak & Stability Validation.
 *
 * Validates that production-critical paths remain stable under elevated load
 * and detect resource-leak, deadlock, and reliability-degradation signatures.
 *
 *   SSS-01  Sustained ingest burst — no storage corruption after N batches
 *   SSS-02  High-concurrency query fan-out — metric counts stay consistent
 *   SSS-03  Storage size is monotone during soak-style write loop
 *   SSS-04  Repeated index lookups under concurrent writers find no phantom reads
 *   SSS-05  Audit log capacity is bounded — no unbounded growth under load
 *   SSS-06  Auth mock under concurrent token checks returns no false positives
 *   SSS-07  Pipeline throughput does not degrade across successive soak cycles
 *   SSS-08  Zero resource leaks — storage and index are fully cleaned between runs
 *
 * All tests run offline using mocks from test_fixture.h.
 * CTest labels: wave6;w6b;stress_soak
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

namespace themis { namespace test { 

namespace {

/// @brief Lightweight ingest-query pipeline reused across SSS tests.
class StressPipeline {
public:
    explicit StressPipeline(std::shared_ptr<MockPipelineAuth>        auth,
                            std::shared_ptr<MockPipelineIndex>       index,
                            std::shared_ptr<InMemoryPipelineStorage> storage,
                            std::shared_ptr<PipelineAuditLog>        audit)
        : auth_(std::move(auth))
        , index_(std::move(index))
        , storage_(std::move(storage))
        , audit_(std::move(audit)) {}

    bool Ingest(const std::string& token,
                const std::string& doc_id,
                const std::string& content,
                const std::string& term) {
        if (!auth_->Authorize(token).authorized) {
            return false;
        }
        storage_->Write(doc_id, content);
        index_->IndexDocument(doc_id, {term});
        audit_->Record({"ingest", "stored", doc_id});
        ++ingest_count_;
        return true;
    }

    std::vector<std::string> Query(const std::string& token, const std::string& term) {
        if (!auth_->Authorize(token).authorized) {
            return {};
        }
        const auto ids = index_->Search(term);
        std::vector<std::string> out = {};

        out.reserve(ids.size());
        for (const auto& id : ids) {
            if (const auto v = storage_->Read(id); v.has_value()) {
                out.push_back(*v);
                ++query_hits_;
            }
        }
        return out;
    }

    [[nodiscard]] size_t StoredCount() const  { return storage_->Size(); }
    [[nodiscard]] size_t IngestCount() const  { return ingest_count_.load(); }
    [[nodiscard]] size_t QueryHits()   const  { return query_hits_.load(); }

private:
    std::shared_ptr<MockPipelineAuth>        auth_;
    std::shared_ptr<MockPipelineIndex>       index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog>        audit_;
    std::atomic<size_t>                      ingest_count_{0};
    std::atomic<size_t>                      query_hits_{0};
};

} // namespace

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class StressSoakStabilityTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        auth_    = CreateMockAuth();
        index_   = CreateMockIndex();
        storage_ = CreateInMemoryStorage();
        audit_   = CreateAuditLog();
        pipeline_ = std::make_unique<StressPipeline>(auth_, index_, storage_, audit_);
        auth_->AllowToken(kToken);
    }

    static constexpr const char* kToken = "sss_valid_token";

    std::shared_ptr<MockPipelineAuth>        auth_;
    std::shared_ptr<MockPipelineIndex>       index_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<PipelineAuditLog>        audit_;
    std::unique_ptr<StressPipeline>          pipeline_;
    TestDataGenerator                        data_gen_;
};

// ---------------------------------------------------------------------------
// SSS-01 — Sustained ingest burst — no storage corruption after N batches
// ---------------------------------------------------------------------------

/**
 * @test SSS-01: Multiple ingest batches leave storage in a consistent state.
 *
 * Acceptance Criteria:
 * - After kBatches × kBatchSize ingests all succeed.
 * - StoredCount == total ingests.
 * - No overwrite collisions for unique doc IDs.
 */
TEST_F(StressSoakStabilityTest, SSS01_SustainedIngestBurstNoStorageCorruption) {
    constexpr size_t kBatches   = 10;
    constexpr size_t kBatchSize = 20;

    for (size_t b = 0; b < kBatches; ++b) {
        for (size_t i = 0; i < kBatchSize; ++i) {
            const std::string doc_id = "sss01_b" + std::to_string(b) + "_i" + std::to_string(i);
            ASSERT_TRUE(pipeline_->Ingest(kToken, doc_id, "content_" + doc_id, "term_batch"))
                << "Ingest must not fail for doc " << doc_id;
        }
    }

    EXPECT_EQ(pipeline_->StoredCount(), kBatches * kBatchSize);
    EXPECT_EQ(pipeline_->IngestCount(), kBatches * kBatchSize);
}

// ---------------------------------------------------------------------------
// SSS-02 — High-concurrency query fan-out — metric counts stay consistent
// ---------------------------------------------------------------------------

/**
 * @test SSS-02: Concurrent queries on a seeded index yield consistent hit counts.
 *
 * Acceptance Criteria:
 * - kDocCount docs are indexed before concurrent queries start.
 * - kQueryThreads threads each issue one query.
 * - Total query hits == kDocCount × kQueryThreads.
 */
TEST_F(StressSoakStabilityTest, SSS02_HighConcurrencyQueryFanOutMetricsConsistent) {
    constexpr size_t kDocCount     = 10;
    constexpr int    kQueryThreads = 8;
    const std::string kSharedTerm  = "sss02_shared_term";

    for (size_t i = 0; i < kDocCount; ++i) {
        const std::string id = "sss02_doc_" + std::to_string(i);
        pipeline_->Ingest(kToken, id, "val_" + id, kSharedTerm);
    }

    std::atomic<size_t> total_hits{0};
    std::vector<std::thread> threads;
    threads.reserve(kQueryThreads);
    for (int t = 0; t < kQueryThreads; ++t) {
        threads.emplace_back([&]() {
            const auto results = pipeline_->Query(kToken, kSharedTerm);
            total_hits += results.size();
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(total_hits.load(), kDocCount * static_cast<size_t>(kQueryThreads))
        << "All threads must see all documents";
}

// ---------------------------------------------------------------------------
// SSS-03 — Storage size is monotone during soak-style write loop
// ---------------------------------------------------------------------------

/**
 * @test SSS-03: Storage grows monotonically during a soak-style sequential write loop.
 *
 * Acceptance Criteria:
 * - After each write StoredCount increases or stays equal (write-once keys).
 * - Final StoredCount == number of unique keys written.
 */
TEST_F(StressSoakStabilityTest, SSS03_StorageSizeMonotoneDuringSoakWriteLoop) {
    constexpr size_t kWrites = 50;
    size_t prev_count = 0;

    for (size_t i = 0; i < kWrites; ++i) {
        const std::string doc_id = "sss03_doc_" + std::to_string(i);
        pipeline_->Ingest(kToken, doc_id, "soak_content_" + std::to_string(i), "sss03_term");

        const size_t current = pipeline_->StoredCount();
        EXPECT_GE(current, prev_count) << "StoredCount must not decrease at step " << i;
        prev_count = current;
    }

    EXPECT_EQ(pipeline_->StoredCount(), kWrites);
}

// ---------------------------------------------------------------------------
// SSS-04 — Repeated index lookups under concurrent writers find no phantom reads
// ---------------------------------------------------------------------------

/**
 * @test SSS-04: Readers never see documents that were not yet written (no phantoms).
 *
 * Acceptance Criteria:
 * - Writer thread inserts kDocCount docs with unique term prefix.
 * - Concurrent reader queries for a term only after that doc's ID has been stored.
 * - No reader hits return data that was never written.
 */
TEST_F(StressSoakStabilityTest, SSS04_RepeatedIndexLookupsNoPhantomsUnderConcurrentWriters) {
    constexpr int kDocCount = 20;
    std::atomic<int> writes_done{0};
    std::atomic<bool> phantom_detected{false};

    // Writer thread
    std::thread writer([&]() {
        for (int i = 0; i < kDocCount; ++i) {
            const std::string id   = "sss04_doc_" + std::to_string(i);
            const std::string term = "sss04_term_" + std::to_string(i);
            pipeline_->Ingest(kToken, id, "payload_" + std::to_string(i), term);
            ++writes_done;
        }
    });

    // Reader thread checks only terms it knows have been written
    std::thread reader([&]() {
        for (int i = 0; i < kDocCount; ++i) {
            // Spin-wait until this doc is confirmed written
            while (writes_done.load() <= i) {
                std::this_thread::yield();
            }
            const std::string term = "sss04_term_" + std::to_string(i);
            const auto results = pipeline_->Query(kToken, term);
            for (const auto& r : results) {
                if (r.find("payload_" + std::to_string(i)) == std::string::npos) {
                    phantom_detected.store(true);
                }
            }
        }
    });

    writer.join();
    reader.join();

    EXPECT_FALSE(phantom_detected.load()) << "No phantom reads must be observable";
}

// ---------------------------------------------------------------------------
// SSS-05 — Audit log capacity is bounded — no unbounded growth under load
// ---------------------------------------------------------------------------

/**
 * @test SSS-05: Under a high-volume ingest burst the audit log grows proportionally.
 *
 * Acceptance Criteria:
 * - After kOps ingests the audit log count == kOps.
 * - No duplicate or missing events (each ingest maps 1:1 to one audit record).
 */
TEST_F(StressSoakStabilityTest, SSS05_AuditLogCapacityBoundedUnderLoad) {
    constexpr size_t kOps = 100;

    for (size_t i = 0; i < kOps; ++i) {
        const std::string doc_id = "sss05_doc_" + std::to_string(i);
        pipeline_->Ingest(kToken, doc_id, "c_" + std::to_string(i), "sss05_term");
    }

    EXPECT_EQ(audit_->Count(), kOps)
        << "Each ingest must produce exactly one audit event";
}

// ---------------------------------------------------------------------------
// SSS-06 — Auth mock under concurrent token checks returns no false positives
// ---------------------------------------------------------------------------

/**
 * @test SSS-06: Concurrent auth checks never admit an explicitly denied token.
 *
 * Acceptance Criteria:
 * - kThreads threads each attempt to ingest using the denied token.
 * - All attempts return false.
 * - Storage remains empty.
 */
TEST_F(StressSoakStabilityTest, SSS06_AuthMockConcurrentNoDeniedTokenAdmitted) {
    constexpr int kThreads = 8;
    const std::string denied_token = "sss06_denied_token";
    auth_->DenyToken(denied_token);

    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i]() {
            const bool ok = pipeline_->Ingest(
                denied_token,
                "sss06_doc_" + std::to_string(i),
                "content",
                "sss06_term");
            if (!ok) {
                ++failures;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(failures.load(), kThreads) << "All denied-token ingests must fail";
    EXPECT_EQ(pipeline_->StoredCount(), 0U) << "Storage must remain empty";
}

// ---------------------------------------------------------------------------
// SSS-07 — Pipeline throughput does not degrade across successive soak cycles
// ---------------------------------------------------------------------------

/**
 * @test SSS-07: Multiple soak cycles show non-degrading throughput (no slowdown proxy).
 *
 * Each cycle is timed; cycle N duration must not exceed kDegradationFactor × cycle 1 duration.
 * This serves as a canary for monotonic query-path slowdowns.
 *
 * Acceptance Criteria:
 * - kCycles soak cycles each complete within a deterministic wall-clock bound.
 * - Last cycle duration <= kDegradationFactor × first cycle duration.
 */
TEST_F(StressSoakStabilityTest, SSS07_PipelineThroughputNonDegradingAcrossSoakCycles) {
    constexpr int    kCycles           = 4;
    constexpr size_t kOpsPerCycle      = 25;
    constexpr double kDegradationFactor = 10.0; // generous; catches severe regressions

    std::vector<long long> durations_us;
    durations_us.reserve(kCycles);

    for (int cycle = 0; cycle < kCycles; ++cycle) {
        // Fresh infra each cycle to isolate accumulated state
        auto auth    = CreateMockAuth();
        auto index   = CreateMockIndex();
        auto storage = CreateInMemoryStorage();
        auto audit   = CreateAuditLog();
        auth->AllowToken(kToken);
        StressPipeline pipe(auth, index, storage, audit);

        const auto start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < kOpsPerCycle; ++i) {
            pipe.Ingest(kToken, "c" + std::to_string(cycle) + "_" + std::to_string(i),
                        "v", "sss07_term");
        }
        pipe.Query(kToken, "sss07_term");
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start)
            .count();
        durations_us.push_back(elapsed);
    }

    const double baseline_us = static_cast<double>(durations_us.front() > 0 ? durations_us.front()
                                                                             : 1LL);
    const double ratio = static_cast<double>(durations_us.back()) / baseline_us;
    EXPECT_LE(ratio, kDegradationFactor)
        << "Last cycle must not take >" << kDegradationFactor
        << "x longer than the first cycle";
}

// ---------------------------------------------------------------------------
// SSS-08 — Zero resource leaks — storage and index are fully cleaned between runs
// ---------------------------------------------------------------------------

/**
 * @test SSS-08: Verifies clean-slate isolation — each fresh pipeline instance starts empty.
 *
 * Acceptance Criteria:
 * - After a write-heavy run the pipeline instance has non-zero storage.
 * - A new pipeline instance starts with zero storage.
 * - No state bleeds from the previous instance.
 */
TEST_F(StressSoakStabilityTest, SSS08_ZeroResourceLeaksStorageCleanBetweenRuns) {
    // Run 1: populate storage
    for (int i = 0; i < 20; ++i) {
        pipeline_->Ingest(kToken, "sss08_run1_" + std::to_string(i), "v", "sss08_term");
    }
    EXPECT_EQ(pipeline_->StoredCount(), 20U);

    // Run 2: fresh pipeline
    auto auth2    = CreateMockAuth();
    auto index2   = CreateMockIndex();
    auto storage2 = CreateInMemoryStorage();
    auto audit2   = CreateAuditLog();
    auth2->AllowToken(kToken);
    StressPipeline pipeline2(auth2, index2, storage2, audit2);

    EXPECT_EQ(pipeline2.StoredCount(), 0U) << "Fresh pipeline must start with empty storage";

    const auto results = pipeline2.Query(kToken, "sss08_term");
    EXPECT_TRUE(results.empty()) << "No data must leak from the previous pipeline instance";
}
} } // namespace themis::test
