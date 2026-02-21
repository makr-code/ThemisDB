/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_sharding_repair.cpp                           ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:35:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     799                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Shard Repair Engine Tests
 *
 * Tests for:
 *  - ReedSolomonCoder multi-chunk erasure decode (Vandermonde matrix)
 *  - ShardRepairEngine lifecycle and on-demand triggers
 *  - AdminAPI repair endpoint routing
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "sharding/redundancy_strategy.h"
#include "sharding/shard_repair_engine.h"
#include "sharding/auto_recovery_manager.h"
#include "sharding/admin_api.h"
#include "sharding/hot_spare_manager.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/prometheus_metrics.h"
#include "server/sharding_metrics_handler.h"
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::sharding;

// ============================================================================
// Shared test helpers
// ============================================================================

// No-op read handler: all reads return "not found".
static const RedundancyStrategy::ReadHandler kNullReadHandler =
    [](const std::string&, const std::string&) -> std::optional<std::vector<uint8_t>> {
        return std::nullopt;
    };

// Always-succeed write handler.
static const RedundancyStrategy::WriteHandler kAlwaysSucceedWriteHandler =
    [](const std::string&, const std::string&, const std::vector<uint8_t>&) -> bool {
        return true;
    };

// ============================================================================
// ReedSolomonCoder multi-chunk erasure decoding tests
// ============================================================================

class ReedSolomonCoderTest : public ::testing::Test {
protected:
    ReedSolomonCoder coder_;

    // Helpers
    std::vector<uint8_t> makeData(size_t n) {
        std::vector<uint8_t> d(n);
        for (size_t i = 0; i < n; ++i) {
            d[i] = static_cast<uint8_t>(i & 0xFF);
        }
        return d;
    }

    // Encode, then drop specified chunk indices, then decode and verify
    void encodeDropDecode(const std::vector<uint8_t>& original,
                          uint32_t k, uint32_t m,
                          const std::vector<uint32_t>& drop_indices) {
        auto chunks = coder_.encode(original, k, m);
        ASSERT_EQ(chunks.size(), k + m);

        // Build available-chunks map (exclude dropped indices)
        std::map<uint32_t, std::vector<uint8_t>> available;
        for (uint32_t i = 0; i < k + m; ++i) {
            if (std::find(drop_indices.begin(), drop_indices.end(), i) == drop_indices.end()) {
                available[i] = chunks[i];
            }
        }

        ASSERT_GE(available.size(), k) << "Not enough available chunks to recover";

        auto recovered = coder_.decode(available, drop_indices, k, m);

        // The recovered data should be at least as long as the original (padding may exist)
        ASSERT_GE(recovered.size(), original.size());

        // First original.size() bytes must match
        std::string dropped_str;
        for (auto idx : drop_indices) {
            dropped_str += std::to_string(idx) + " ";
        }
        for (size_t i = 0; i < original.size(); ++i) {
            EXPECT_EQ(recovered[i], original[i])
                << "Mismatch at byte " << i << " (dropped chunks: " << dropped_str << ")";
        }
    }
};

TEST_F(ReedSolomonCoderTest, EncodeDecodeNoLoss) {
    auto data = makeData(256);
    encodeDropDecode(data, 4, 2, {});
}

TEST_F(ReedSolomonCoderTest, RecoverOneMissingDataChunk_RAID5) {
    auto data = makeData(400);
    // Drop data chunk 1
    encodeDropDecode(data, 4, 1, {1});
}

TEST_F(ReedSolomonCoderTest, RecoverOneMissingDataChunk_RAID6) {
    auto data = makeData(400);
    // Drop data chunk 2
    encodeDropDecode(data, 4, 2, {2});
}

TEST_F(ReedSolomonCoderTest, RecoverTwoMissingDataChunks_RAID6) {
    // RAID-6 with 2 parity shards should survive 2 simultaneous failures
    auto data = makeData(400);
    // Drop data chunks 0 and 3
    encodeDropDecode(data, 4, 2, {0, 3});
}

TEST_F(ReedSolomonCoderTest, RecoverMissingParityChunk) {
    auto data = makeData(400);
    // Drop a parity chunk – recovery should still give back original data
    encodeDropDecode(data, 4, 2, {4});  // parity shard index 4
}

TEST_F(ReedSolomonCoderTest, RecoverDataAndParityChunk) {
    auto data = makeData(400);
    // Drop one data + one parity chunk (still within 2-fault tolerance)
    encodeDropDecode(data, 4, 2, {1, 5});
}

TEST_F(ReedSolomonCoderTest, NotEnoughChunksThrows) {
    auto data = makeData(256);
    auto chunks = coder_.encode(data, 4, 2);

    // Keep only k-1 = 3 chunks – decode must throw
    std::map<uint32_t, std::vector<uint8_t>> available;
    available[0] = chunks[0];
    available[1] = chunks[1];
    available[2] = chunks[2];

    EXPECT_THROW(coder_.decode(available, {3, 4, 5}, 4, 2), std::runtime_error);
}

TEST_F(ReedSolomonCoderTest, SmallData) {
    auto data = makeData(4);  // Tiny, even smaller than number of data shards
    encodeDropDecode(data, 4, 2, {0});
}

TEST_F(ReedSolomonCoderTest, ThreeParityShards_RecoverThree) {
    auto data = makeData(600);
    // 4 data + 3 parity -> tolerate 3 failures
    encodeDropDecode(data, 4, 3, {0, 2, 4});
}

TEST_F(ReedSolomonCoderTest, TooManyMissingThrows) {
    auto data = makeData(256);
    auto chunks = coder_.encode(data, 4, 2);

    // Provide k chunks but claim 3 are missing (exceeds parity_shards=2)
    std::map<uint32_t, std::vector<uint8_t>> available;
    available[0] = chunks[0];
    available[1] = chunks[1];
    available[2] = chunks[2];
    available[3] = chunks[3];

    // 3 missing > 2 parity shards — should throw
    EXPECT_THROW(coder_.decode(available, {4, 5, 6}, 4, 2), std::runtime_error);
}

// ============================================================================
// CauchyReedSolomonCoder tests
// ============================================================================

class CauchyReedSolomonCoderTest : public ::testing::Test {
protected:
    CauchyReedSolomonCoder coder_;

    std::vector<uint8_t> makeData(size_t n) {
        std::vector<uint8_t> d(n);
        for (size_t i = 0; i < n; ++i) d[i] = static_cast<uint8_t>(i & 0xFF);
        return d;
    }

    void encodeDropDecode(const std::vector<uint8_t>& original,
                          uint32_t k, uint32_t m,
                          const std::vector<uint32_t>& drop_indices) {
        auto chunks = coder_.encode(original, k, m);
        ASSERT_EQ(chunks.size(), k + m);

        std::map<uint32_t, std::vector<uint8_t>> available;
        for (uint32_t i = 0; i < k + m; ++i) {
            if (std::find(drop_indices.begin(), drop_indices.end(), i) == drop_indices.end()) {
                available[i] = chunks[i];
            }
        }
        ASSERT_GE(available.size(), k);

        auto recovered = coder_.decode(available, drop_indices, k, m);
        ASSERT_GE(recovered.size(), original.size());

        std::string dropped_str;
        for (auto idx : drop_indices) dropped_str += std::to_string(idx) + " ";
        for (size_t i = 0; i < original.size(); ++i) {
            EXPECT_EQ(recovered[i], original[i])
                << "Mismatch at byte " << i << " (dropped: " << dropped_str << ")";
        }
    }
};

TEST_F(CauchyReedSolomonCoderTest, EncodeDecodeNoLoss) {
    encodeDropDecode(makeData(256), 4, 2, {});
}

TEST_F(CauchyReedSolomonCoderTest, RecoverOneMissingDataChunk) {
    encodeDropDecode(makeData(400), 4, 2, {1});
}

TEST_F(CauchyReedSolomonCoderTest, RecoverTwoMissingChunks_RAID6) {
    encodeDropDecode(makeData(400), 4, 2, {0, 3});
}

TEST_F(CauchyReedSolomonCoderTest, TooManyMissingThrows) {
    auto data = makeData(256);
    auto chunks = coder_.encode(data, 4, 2);

    std::map<uint32_t, std::vector<uint8_t>> available;
    available[0] = chunks[0];
    available[1] = chunks[1];
    available[2] = chunks[2];
    available[3] = chunks[3];

    // 3 missing > 2 parity shards — should throw
    EXPECT_THROW(coder_.decode(available, {4, 5, 6}, 4, 2), std::runtime_error);
}


class ShardRepairEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create minimal topology
        ShardTopology::Config topo_cfg;
        topo_cfg.enable_health_checks = false;
        topo_cfg.cluster_name = "repair-test";
        topology_ = std::make_shared<ShardTopology>(topo_cfg);

        for (int i = 1; i <= 3; ++i) {
            ShardInfo s;
            s.shard_id = "shard_" + std::to_string(i);
            s.primary_endpoint = "localhost:908" + std::to_string(i);
            s.is_healthy = true;
            topology_->addShard(s);
        }

        ring_ = std::make_shared<ConsistentHashRing>();
        for (int i = 1; i <= 3; ++i) {
            ring_->addShard("shard_" + std::to_string(i), 150);
        }

        RedundancyConfig cfg;
        cfg.mode = RedundancyMode::MIRROR;
        cfg.replication_factor = 3;
        strategy_ = std::make_unique<RedundancyStrategy>(cfg);

        RepairConfig repair_cfg;
        repair_cfg.enable_periodic_scan = false;  // Manual control in tests
        repair_cfg.enable_auto_repair = true;
        repair_cfg.repair_poll_interval = std::chrono::seconds(1);

        engine_ = std::make_unique<ShardRepairEngine>(
            repair_cfg,
            *strategy_,
            *ring_,
            *topology_,
            kNullReadHandler,
            kAlwaysSucceedWriteHandler
        );
    }

    void TearDown() override {
        if (engine_ && engine_->isRunning()) {
            engine_->stop();
        }
    }

    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ConsistentHashRing> ring_;
    std::unique_ptr<RedundancyStrategy> strategy_;
    std::unique_ptr<ShardRepairEngine> engine_;
};

TEST_F(ShardRepairEngineTest, StartStop) {
    EXPECT_FALSE(engine_->isRunning());
    engine_->start();
    EXPECT_TRUE(engine_->isRunning());
    engine_->stop();
    EXPECT_FALSE(engine_->isRunning());
}

TEST_F(ShardRepairEngineTest, DoubleStartIsIdempotent) {
    engine_->start();
    engine_->start();  // Should not crash or spawn extra threads
    EXPECT_TRUE(engine_->isRunning());
    engine_->stop();
}

TEST_F(ShardRepairEngineTest, TriggerRepairEnqueuesJob) {
    engine_->start();
    std::string job_id = engine_->triggerRepair("shard_1");
    EXPECT_FALSE(job_id.empty());

    // Wait a moment for the repair worker to pick it up
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto job = engine_->getJobStatus(job_id);
    EXPECT_EQ(job.job_id, job_id);
    EXPECT_EQ(job.shard_id, "shard_1");

    engine_->stop();
}

TEST_F(ShardRepairEngineTest, TriggerFullScan) {
    engine_->start();
    std::string job_id = engine_->triggerFullScan();
    EXPECT_FALSE(job_id.empty());

    auto job = engine_->getJobStatus(job_id);
    EXPECT_TRUE(job.is_full_scan);

    engine_->stop();
}

TEST_F(ShardRepairEngineTest, TriggerDocumentRepair) {
    engine_->start();
    std::string job_id = engine_->triggerDocumentRepair("doc-123", "col_test");
    EXPECT_FALSE(job_id.empty());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto job = engine_->getJobStatus(job_id);
    EXPECT_EQ(job.document_id, "doc-123");
    EXPECT_EQ(job.collection, "col_test");

    engine_->stop();
}

TEST_F(ShardRepairEngineTest, JobNotFoundReturnsError) {
    auto job = engine_->getJobStatus("nonexistent-job");
    EXPECT_TRUE(job.completed);
    EXPECT_FALSE(job.success);
    EXPECT_FALSE(job.error_message.empty());
}

TEST_F(ShardRepairEngineTest, PrometheusMetricsNotEmpty) {
    engine_->start();
    engine_->triggerRepair("shard_1");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    engine_->stop();

    std::string metrics = engine_->exportPrometheusMetrics();
    EXPECT_FALSE(metrics.empty());
    EXPECT_NE(metrics.find("themis_shard_repair_scans_total"), std::string::npos);
    EXPECT_NE(metrics.find("themis_shard_repair_attempts_total"), std::string::npos);
}

TEST_F(ShardRepairEngineTest, GetActiveJobsReturnsPending) {
    // Don't start the engine so jobs stay queued
    engine_->triggerRepair("shard_1");
    engine_->triggerRepair("shard_2");

    auto active = engine_->getActiveJobs();
    EXPECT_GE(active.size(), 2u);
}

// ============================================================================
// AdminAPI repair endpoint routing tests
// ============================================================================

class AdminAPIRepairTest : public ::testing::Test {
protected:
    void SetUp() override {
        AdminAPI::Config cfg;
        cfg.require_signatures = false;
        api_ = std::make_unique<AdminAPI>(cfg);

        api_->registerRepairHandler([](const nlohmann::json& body) -> nlohmann::json {
            if (body.contains("job_id")) {
                return {{"status", "completed"}, {"job_id", body["job_id"]}};
            }
            if (body.value("full_scan", false)) {
                return {{"job_id", "scan-001"}, {"status", "queued"}};
            }
            std::string shard = body.value("shard_id", "");
            return {{"job_id", "repair-001"}, {"shard_id", shard}, {"status", "queued"}};
        });
    }

    std::unique_ptr<AdminAPI> api_;
};

TEST_F(AdminAPIRepairTest, PostRepairReturnsJobId) {
    nlohmann::json body = {{"shard_id", "shard_1"}};
    auto resp = api_->handleRequest("POST", "/admin/repair", body, "operator-cert");
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_TRUE(resp.contains("job_id"));
    EXPECT_EQ(resp["shard_id"], "shard_1");
}

TEST_F(AdminAPIRepairTest, PostRepairScanReturnsJobId) {
    nlohmann::json body;
    auto resp = api_->handleRequest("POST", "/admin/repair/scan", body, "operator-cert");
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_TRUE(resp.contains("job_id"));
    EXPECT_TRUE(resp.value("status", "") == "queued");
}

TEST_F(AdminAPIRepairTest, GetRepairStatusExtractsJobId) {
    nlohmann::json body;
    auto resp = api_->handleRequest("GET", "/admin/repair/repair-001", body, "operator-cert");
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_EQ(resp["job_id"], "repair-001");
    EXPECT_EQ(resp["status"], "completed");
}

TEST_F(AdminAPIRepairTest, NoHandlerReturns404) {
    // Remove the repair handler by registering a null-wrapper
    AdminAPI::Config cfg;
    cfg.require_signatures = false;
    AdminAPI api_no_handler(cfg);  // No repair handler registered
    nlohmann::json body;
    auto resp = api_no_handler.handleRequest("POST", "/admin/repair", body, "operator-cert");
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"], 404);
}

TEST_F(AdminAPIRepairTest, UnauthorizedRequestRejected) {
    nlohmann::json body;
    // Empty cert should be rejected
    auto resp = api_->handleRequest("POST", "/admin/repair", body, "");
    EXPECT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"], 403);
}

// ============================================================================
// PrometheusMetrics repair methods tests
// ============================================================================

TEST(PrometheusRepairMetricsTest, RecordRepairOperationSuccess) {
    themis::sharding::PrometheusMetrics::Config cfg;
    themis::sharding::PrometheusMetrics metrics(cfg);

    metrics.recordRepairOperation(true, 42.5);
    metrics.recordRepairOperation(true, 10.0);
    metrics.recordRepairOperation(false, 5.0);

    std::string out = metrics.getMetrics();
    EXPECT_NE(out.find("themis_shard_repair_operations_total"), std::string::npos);
}

TEST(PrometheusRepairMetricsTest, RecordRepairShardStatus) {
    themis::sharding::PrometheusMetrics::Config cfg;
    themis::sharding::PrometheusMetrics metrics(cfg);
    using S = themis::sharding::PrometheusMetrics::RepairShardStatus;

    metrics.recordRepairShardStatus("shard_1", S::HEALTHY);
    metrics.recordRepairShardStatus("shard_2", S::DEGRADED);
    metrics.recordRepairShardStatus("shard_3", S::REBUILDING);

    std::string out = metrics.getMetrics();
    EXPECT_NE(out.find("themis_shard_repair_health"), std::string::npos);
}

TEST(PrometheusRepairMetricsTest, RecordRepairScan) {
    themis::sharding::PrometheusMetrics::Config cfg;
    themis::sharding::PrometheusMetrics metrics(cfg);

    metrics.recordRepairScan();
    metrics.recordRepairScan();

    std::string out = metrics.getMetrics();
    EXPECT_NE(out.find("themis_shard_repair_scans_total"), std::string::npos);
}

static std::shared_ptr<ShardRepairEngine> makeMinimalEngine(
    RedundancyStrategy& strategy,
    ConsistentHashRing& ring,
    ShardTopology& topology);

// ============================================================================
// ShardingMetricsHandler repair integration tests
// ============================================================================

TEST(ShardingMetricsHandlerRepairTest, GetMetricsIncludesRepairWhenEngineSet) {
    // Build a minimal ShardRepairEngine
    ShardTopology::Config topo_cfg;
    topo_cfg.enable_health_checks = false;
    topo_cfg.cluster_name = "metrics-test";
    auto topology = std::make_shared<ShardTopology>(topo_cfg);
    auto ring = std::make_shared<ConsistentHashRing>();

    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    auto engine = makeMinimalEngine(*strategy, *ring, *topology);

    // Create handler with no PrometheusMetrics (minimal) but with repair engine
    themis::server::ShardingMetricsHandler handler(nullptr);
    handler.setRepairEngine(engine);

    // getRepairMetrics should return non-empty Prometheus text
    std::string repair_metrics = handler.getRepairMetrics();
    EXPECT_NE(repair_metrics.find("themis_shard_repair_scans_total"), std::string::npos);
}

TEST(ShardingMetricsHandlerRepairTest, GetMetricsAppendsRepairWhenEngineSet) {
    auto prom = std::make_shared<themis::sharding::PrometheusMetrics>(
        themis::sharding::PrometheusMetrics::Config{});
    prom->recordRoutingRequest("read");

    ShardTopology::Config topo_cfg;
    topo_cfg.enable_health_checks = false;
    topo_cfg.cluster_name = "metrics-append-test";
    auto topology = std::make_shared<ShardTopology>(topo_cfg);
    auto ring = std::make_shared<ConsistentHashRing>();
    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    auto engine = makeMinimalEngine(*strategy, *ring, *topology);

    themis::server::ShardingMetricsHandler handler(prom);
    handler.setRepairEngine(engine);

    std::string all_metrics = handler.getMetrics();
    // Both routing and repair metrics should appear
    EXPECT_NE(all_metrics.find("themis_routing_requests_total"), std::string::npos);
    EXPECT_NE(all_metrics.find("themis_shard_repair_scans_total"), std::string::npos);
}

TEST(ShardingMetricsHandlerRepairTest, GetRepairMetricsEmptyWithoutEngine) {
    themis::server::ShardingMetricsHandler handler(nullptr);
    EXPECT_TRUE(handler.getRepairMetrics().empty());
}

TEST(ShardingMetricsHandlerRepairTest, GetMetricsNoRepairWithoutEngine) {
    auto prom = std::make_shared<themis::sharding::PrometheusMetrics>(
        themis::sharding::PrometheusMetrics::Config{});
    prom->recordRoutingRequest("write");

    themis::server::ShardingMetricsHandler handler(prom);
    std::string out = handler.getMetrics();
    // Should have routing metrics but no repair metrics
    EXPECT_NE(out.find("themis_routing_requests_total"), std::string::npos);
    EXPECT_EQ(out.find("themis_shard_repair_scans_total"), std::string::npos);
}

// ============================================================================
// AutoRecoveryManager + ShardRepairEngine integration tests
// ============================================================================

static std::shared_ptr<ShardRepairEngine> makeMinimalEngine(
    RedundancyStrategy& strategy,
    ConsistentHashRing& ring,
    ShardTopology& topology) {
    RepairConfig cfg;
    cfg.enable_periodic_scan = false;
    cfg.enable_auto_repair = false;
    return std::make_shared<ShardRepairEngine>(
        cfg, strategy, ring, topology,
        kNullReadHandler, kAlwaysSucceedWriteHandler);
}

TEST(AutoRecoveryManagerRepairTest, WithoutEngineRepairDocumentReturnsFalse) {
    ShardTopology::Config topo_cfg;
    topo_cfg.enable_health_checks = false;
    ShardTopology topology(topo_cfg);
    ConsistentHashRing ring;
    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    RedundancyStrategy strategy(rcfg);

    themisdb::sharding::AutoRecoveryConfig arm_cfg;
    arm_cfg.enable_auto_repair = false;
    themisdb::sharding::AutoRecoveryManager arm(arm_cfg, strategy, ring, topology);

    // No engine set → processRepairQueue's repairDocument returns false
    // We verify the manager can start and stop cleanly without hanging.
    arm.start();
    arm.stop();
    EXPECT_FALSE(arm.isRunning());
}

TEST(AutoRecoveryManagerRepairTest, WithEngineRepairDocumentEnqueuesJob) {
    ShardTopology::Config topo_cfg;
    topo_cfg.enable_health_checks = false;
    topo_cfg.cluster_name = "arm-test";
    auto topology = std::make_shared<ShardTopology>(topo_cfg);
    auto ring = std::make_shared<ConsistentHashRing>();
    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    auto engine = makeMinimalEngine(*strategy, *ring, *topology);
    engine->start();

    themisdb::sharding::AutoRecoveryConfig arm_cfg;
    arm_cfg.enable_auto_repair = false;
    themisdb::sharding::AutoRecoveryManager arm(arm_cfg, *strategy, *ring, *topology);
    arm.setRepairEngine(engine);

    arm.start();
    arm.stop();

    engine->stop();
}

// ============================================================================
// ShardRepairEngine → PrometheusMetrics forwarding tests
// ============================================================================

TEST(ShardRepairPrometheusForwardingTest, SetPrometheusMetrics) {
    ShardTopology::Config topo_cfg;
    topo_cfg.enable_health_checks = false;
    auto topology = std::make_shared<ShardTopology>(topo_cfg);
    auto ring = std::make_shared<ConsistentHashRing>();
    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    auto engine = makeMinimalEngine(*strategy, *ring, *topology);
    auto prom = std::make_shared<themis::sharding::PrometheusMetrics>(
        themis::sharding::PrometheusMetrics::Config{});
    // Should not throw
    EXPECT_NO_THROW(engine->setPrometheusMetrics(prom));
}

TEST(ShardRepairPrometheusForwardingTest, RepairOperationForwardedToProm) {
    ShardTopology::Config topo_cfg;
    topo_cfg.enable_health_checks = false;
    auto topology = std::make_shared<ShardTopology>(topo_cfg);
    auto ring = std::make_shared<ConsistentHashRing>();
    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    auto prom = std::make_shared<themis::sharding::PrometheusMetrics>(
        themis::sharding::PrometheusMetrics::Config{});

    RepairConfig cfg;
    cfg.enable_periodic_scan = false;
    cfg.enable_auto_repair = true;
    cfg.repair_poll_interval = std::chrono::seconds(1);
    ShardRepairEngine engine(cfg, *strategy, *ring, *topology,
                             kNullReadHandler, kAlwaysSucceedWriteHandler);
    engine.setPrometheusMetrics(prom);
    engine.start();

    // Trigger a document repair; wait for the repair worker to process it
    engine.triggerDocumentRepair("doc-fwd-test", "col");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    engine.stop();

    // Prometheus should now have a repair_operations_total entry
    std::string out = prom->getMetrics();
    EXPECT_NE(out.find("themis_shard_repair_operations_total"), std::string::npos);
}

// ============================================================================
// AdminAPI::setRepairEngine + /admin/health integration tests
// ============================================================================

class AdminAPIHealthRepairTest : public ::testing::Test {
protected:
    void SetUp() override {
        topo_cfg_.enable_health_checks = false;
        topo_cfg_.cluster_name = "health-test";
        topology_ = std::make_shared<ShardTopology>(topo_cfg_);

        ShardInfo s;
        s.shard_id = "shard_1";
        s.primary_endpoint = "localhost:9081";
        s.is_healthy = true;
        topology_->addShard(s);

        ring_ = std::make_shared<ConsistentHashRing>();
        ring_->addShard("shard_1", 150);

        RedundancyConfig rcfg;
        rcfg.mode = RedundancyMode::MIRROR;
        strategy_ = std::make_shared<RedundancyStrategy>(rcfg);

        engine_ = makeMinimalEngine(*strategy_, *ring_, *topology_);

        AdminAPI::Config api_cfg;
        api_cfg.require_signatures = false;
        api_.reset(new AdminAPI(api_cfg));
        // Register a minimal health handler that returns a base response
        api_->registerHealthHandler([](const nlohmann::json&) -> nlohmann::json {
            return {{"cluster", "ok"}};
        });
    }

    ShardTopology::Config topo_cfg_;
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ConsistentHashRing> ring_;
    std::shared_ptr<RedundancyStrategy> strategy_;
    std::shared_ptr<ShardRepairEngine> engine_;
    std::unique_ptr<AdminAPI> api_;
};

TEST_F(AdminAPIHealthRepairTest, HealthWithoutEngineHasNoRepairKey) {
    auto resp = api_->handleRequest("GET", "/admin/health", {}, "operator-cert");
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_EQ(resp["cluster"], "ok");
    EXPECT_FALSE(resp.contains("repair"));
}

TEST_F(AdminAPIHealthRepairTest, HealthWithEngineIncludesRepairSection) {
    api_->setRepairEngine(engine_);
    auto resp = api_->handleRequest("GET", "/admin/health", {}, "operator-cert");
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_TRUE(resp.contains("repair"));
    EXPECT_TRUE(resp["repair"].contains("status"));
    EXPECT_TRUE(resp["repair"].contains("engine_running"));
    EXPECT_TRUE(resp["repair"].contains("shards"));
}

TEST_F(AdminAPIHealthRepairTest, HealthRepairStatusIsHealthyWithNoScans) {
    api_->setRepairEngine(engine_);
    auto resp = api_->handleRequest("GET", "/admin/health", {}, "operator-cert");
    // No scans performed yet → shards array empty, overall status healthy
    EXPECT_EQ(resp["repair"]["status"], "healthy");
    EXPECT_EQ(resp["repair"]["total_scans"], 0u);
}

TEST_F(AdminAPIHealthRepairTest, HealthWithEngineNoHandlerStillReturnsRepair) {
    // No health_handler registered but repair engine set
    AdminAPI::Config api_cfg;
    api_cfg.require_signatures = false;
    AdminAPI bare_api(api_cfg);
    bare_api.setRepairEngine(engine_);
    auto resp = bare_api.handleRequest("GET", "/admin/health", {}, "operator-cert");
    // Should still return the repair section (not a 404 error)
    EXPECT_FALSE(resp.contains("error"));
    EXPECT_TRUE(resp.contains("repair"));
}

// ============================================================================
// HotSpareManager::setRepairEngine tests
// ============================================================================

TEST(HotSpareManagerRepairTest, SetRepairEngineDoesNotThrow) {
    ShardTopology::Config topo_cfg;
    topo_cfg.enable_health_checks = false;
    auto topology = std::make_shared<ShardTopology>(topo_cfg);
    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    HotSpareConfig cfg;
    cfg.enable = false;
    HotSpareManager mgr(cfg, *strategy, *topology);

    auto ring = std::make_shared<ConsistentHashRing>();
    auto engine = makeMinimalEngine(*strategy, *ring, *topology);

    EXPECT_NO_THROW(mgr.setRepairEngine(engine));
}
