// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


// Copyright 2026 ThemisDB — Licensed under MIT License
// IMPL-B8 / S-7: WorkloadFingerprintEngine unit tests
//
// Tests:
//   WFE-01  OLTP stats → pattern=OLTP, confidence > 0.8
//   WFE-02  OLAP stats → pattern=OLAP, confidence > 0.8
//   WFE-03  BATCH stats → pattern=BATCH, confidence > 0.8
//   WFE-04  similarityTo() two OLTP profiles → similarity > 0.9
//   WFE-05  similarityTo() OLTP vs BATCH → similarity < 0.3
//   WFE-06  recommended_policy has sensible values per pattern
//   WFE-07  fingerprint vector has dimension 4
//   WFE-08  fingerprint vector sums to ≈ 1.0

#include <gtest/gtest.h>
#include "server/workload_fingerprint_engine.h"

#include <cmath>
#include <numeric>

using namespace themis::server;
using WP = WorkloadFingerprintEngine::WorkloadPattern;

namespace {

WorkloadFingerprintEngine engine;

/// Build typical OLTP stats: many short read-heavy queries.
TenantWorkloadStats makeOltpStats(const std::string& tid = "tenant-oltp") {
    TenantWorkloadStats s;
    s.tenant_id           = tid;
    s.query_count         = 5000;
    s.write_ratio         = 0.10;
    s.avg_p99_ms          = 5.0;
    s.avg_rows_per_query  = 20;
    s.bulk_insert_count   = 0;
    s.window_seconds      = 60;
    return s;
}

/// Build typical OLAP stats: few, very heavy analytical queries.
TenantWorkloadStats makeOlapStats(const std::string& tid = "tenant-olap") {
    TenantWorkloadStats s;
    s.tenant_id           = tid;
    s.query_count         = 5;
    s.write_ratio         = 0.05;
    s.avg_p99_ms          = 8000.0;
    s.avg_rows_per_query  = 500000;
    s.bulk_insert_count   = 0;
    s.window_seconds      = 60;
    return s;
}

/// Build typical BATCH stats: periodic bulk inserts.
TenantWorkloadStats makeBatchStats(const std::string& tid = "tenant-batch") {
    TenantWorkloadStats s;
    s.tenant_id           = tid;
    s.query_count         = 3;
    s.write_ratio         = 0.90;
    s.avg_p99_ms          = 15000.0;
    s.avg_rows_per_query  = 100000;
    s.bulk_insert_count   = 10;
    s.window_seconds      = 3600;
    return s;
}

} // namespace

// ---------------------------------------------------------------------------
// WFE-01  OLTP stats → OLTP pattern, confidence > 0.8
// ---------------------------------------------------------------------------
TEST(WorkloadFingerprintEngineTest, OltpClassification) {
    const auto fp = engine.classify("t-oltp", makeOltpStats());
    EXPECT_EQ(fp.pattern, WP::OLTP);
    EXPECT_GT(fp.confidence, 0.8);
}

// ---------------------------------------------------------------------------
// WFE-02  OLAP stats → OLAP pattern, confidence > 0.8
// ---------------------------------------------------------------------------
TEST(WorkloadFingerprintEngineTest, OlapClassification) {
    const auto fp = engine.classify("t-olap", makeOlapStats());
    EXPECT_EQ(fp.pattern, WP::OLAP);
    EXPECT_GT(fp.confidence, 0.8);
}

// ---------------------------------------------------------------------------
// WFE-03  BATCH stats → BATCH pattern, confidence > 0.8
// ---------------------------------------------------------------------------
TEST(WorkloadFingerprintEngineTest, BatchClassification) {
    const auto fp = engine.classify("t-batch", makeBatchStats());
    EXPECT_EQ(fp.pattern, WP::BATCH);
    EXPECT_GT(fp.confidence, 0.8);
}

// ---------------------------------------------------------------------------
// WFE-04  Two OLTP profiles → similarity > 0.9
// ---------------------------------------------------------------------------
TEST(WorkloadFingerprintEngineTest, SimilarityOltpVsOltp) {
    const auto fp1 = engine.classify("t1", makeOltpStats());
    const auto fp2 = engine.classify("t2", makeOltpStats("tenant-oltp-2"));
    const double sim = engine.similarityTo(fp1, fp2);
    EXPECT_GT(sim, 0.9);
}

// ---------------------------------------------------------------------------
// WFE-05  OLTP vs BATCH → similarity < 0.3
// ---------------------------------------------------------------------------
TEST(WorkloadFingerprintEngineTest, SimilarityOltpVsBatch) {
    const auto fpOltp  = engine.classify("t-oltp",  makeOltpStats());
    const auto fpBatch = engine.classify("t-batch", makeBatchStats());
    const double sim = engine.similarityTo(fpOltp, fpBatch);
    EXPECT_LT(sim, 0.3);
}

// ---------------------------------------------------------------------------
// WFE-06  recommended_policy has sensible values per pattern
// ---------------------------------------------------------------------------
TEST(WorkloadFingerprintEngineTest, PolicyRecommendation) {
    const auto fpOltp  = engine.classify("t", makeOltpStats());
    const auto fpOlap  = engine.classify("t", makeOlapStats());
    const auto fpBatch = engine.classify("t", makeBatchStats());

    // OLTP: high connection count, high priority
    EXPECT_GE(fpOltp.recommended_policy.max_connections, 100u);
    EXPECT_EQ(fpOltp.recommended_policy.priority, "HIGH");

    // OLAP: fewer connections, read replica recommended
    EXPECT_LT(fpOlap.recommended_policy.max_connections, 50u);
    EXPECT_TRUE(fpOlap.recommended_policy.suggest_read_replica);

    // BATCH: low priority
    EXPECT_EQ(fpBatch.recommended_policy.priority, "LOW");
}

// ---------------------------------------------------------------------------
// WFE-07  fingerprint vector has dimension 4
// ---------------------------------------------------------------------------
TEST(WorkloadFingerprintEngineTest, VectorDimension4) {
    const auto fp = engine.classify("t", makeOltpStats());
    EXPECT_EQ(fp.vector.size(), 4u);
}

// ---------------------------------------------------------------------------
// WFE-08  fingerprint vector sums to ≈ 1.0
// ---------------------------------------------------------------------------
TEST(WorkloadFingerprintEngineTest, VectorSumsToOne) {
    for (const auto& stats : {makeOltpStats(), makeOlapStats(), makeBatchStats()}) {
        const auto fp = engine.classify("t", stats);
        const double sum = std::accumulate(fp.vector.begin(), fp.vector.end(), 0.0);
        EXPECT_NEAR(sum, 1.0, 1e-9);
    }
}
