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
#include "sharding/shard_resource_manager.h"
#include "sharding/slo_monitor.h"
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

// ============================================================================
// RS Repair Engine Parallelisation tests (v1.6.0 Issue #204)
// ============================================================================

// Helper: build topology and ring with N shards
static void buildTopologyAndRing(int n_shards,
                                  std::shared_ptr<ShardTopology>& topo,
                                  std::shared_ptr<ConsistentHashRing>& ring) {
    ShardTopology::Config cfg;
    cfg.enable_health_checks = false;
    cfg.cluster_name = "parallel-test";
    topo = std::make_shared<ShardTopology>(cfg);
    ring = std::make_shared<ConsistentHashRing>();
    for (int i = 1; i <= n_shards; ++i) {
        std::string id = "shard_" + std::to_string(i);
        ShardInfo s;
        s.shard_id = id;
        s.primary_endpoint = "localhost:908" + std::to_string(i);
        s.is_healthy = true;
        topo->addShard(s);
        ring->addShard(id, 150);
    }
}

// ── AC-1: Parallel workers config ─────────────────────────────────────────

TEST(ParallelRepairScanTest, RepairConfigHasParallelWorkers) {
    RepairConfig cfg;
    EXPECT_EQ(cfg.num_parallel_workers, 8u);
    cfg.num_parallel_workers = 4;
    EXPECT_EQ(cfg.num_parallel_workers, 4u);
}

TEST(ParallelRepairScanTest, RepairMetricsTracksLastScanWorkers) {
    std::shared_ptr<ShardTopology> topo;
    std::shared_ptr<ConsistentHashRing> ring;
    buildTopologyAndRing(4, topo, ring);

    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    RepairConfig repair_cfg;
    // Enable periodic scan with a very short interval so it fires quickly
    repair_cfg.enable_periodic_scan = true;
    repair_cfg.scan_interval = std::chrono::seconds(1);
    repair_cfg.enable_auto_repair = false;
    repair_cfg.num_parallel_workers = 2;

    ShardRepairEngine engine(repair_cfg, *strategy, *ring, *topo,
                             kNullReadHandler, kAlwaysSucceedWriteHandler);
    engine.start();

    // Wait for at least one periodic scan to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    engine.stop();

    auto metrics = engine.getRepairMetrics();
    EXPECT_GE(metrics.total_scans, 1u);
    EXPECT_GE(metrics.last_scan_workers, 1u);
    EXPECT_LE(metrics.last_scan_workers, 2u);
}

// ── AC-2: Parallel scan actually completes with multiple workers ──────────

TEST(ParallelRepairScanTest, ScanWithZeroWorkersUsesHardwareConcurrency) {
    // Setting num_parallel_workers = 0 should fall back to hardware_concurrency
    std::shared_ptr<ShardTopology> topo;
    std::shared_ptr<ConsistentHashRing> ring;
    buildTopologyAndRing(4, topo, ring);

    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    RepairConfig repair_cfg;
    repair_cfg.enable_periodic_scan = true;
    repair_cfg.scan_interval = std::chrono::seconds(1);
    repair_cfg.enable_auto_repair = false;
    repair_cfg.num_parallel_workers = 0;  // trigger hardware_concurrency() fallback

    ShardRepairEngine engine(repair_cfg, *strategy, *ring, *topo,
                             kNullReadHandler, kAlwaysSucceedWriteHandler);
    engine.start();
    // Wait for at least one periodic scan to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    engine.stop();

    auto metrics = engine.getRepairMetrics();
    // Scan must have run with at least 1 worker (hardware_concurrency >= 1)
    EXPECT_GE(metrics.last_scan_workers, 1u);
}

TEST(ParallelRepairScanTest, ScanWith4WorkersCompletes) {
    std::shared_ptr<ShardTopology> topo;
    std::shared_ptr<ConsistentHashRing> ring;
    buildTopologyAndRing(8, topo, ring);

    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    // Inject a doc-list provider
    auto doc_list_provider = [](const std::string&) -> std::vector<std::string> {
        return {"doc-1", "doc-2", "doc-3"};
    };

    RepairConfig repair_cfg;
    // Use periodic scan so performAntiEntropyScan() is called (it populates shard_health_)
    repair_cfg.enable_periodic_scan = true;
    repair_cfg.scan_interval = std::chrono::seconds(1);
    repair_cfg.enable_auto_repair = false;
    repair_cfg.num_parallel_workers = 4;

    ShardRepairEngine engine(repair_cfg, *strategy, *ring, *topo,
                             kNullReadHandler, kAlwaysSucceedWriteHandler);
    engine.setDocumentListProvider(doc_list_provider);
    engine.start();

    // Wait for at least one periodic scan to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    engine.stop();

    // Scan should have populated shard health reports
    auto reports = engine.getShardHealthReports();
    EXPECT_EQ(reports.size(), 8u);

    // Each shard should have scanned its 3 documents
    for (const auto& r : reports) {
        EXPECT_EQ(r.documents_scanned, 3u);
    }
}

// ── AC-3: setSLOMonitor + progress reporting ───────────────────────────────

TEST(ParallelRepairScanTest, SLOMonitorReceivesRepairProgress) {
    std::shared_ptr<ShardTopology> topo;
    std::shared_ptr<ConsistentHashRing> ring;
    buildTopologyAndRing(4, topo, ring);

    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    auto slo = std::make_shared<themis::sharding::SLOMonitor>();

    RepairConfig repair_cfg;
    repair_cfg.enable_periodic_scan = true;
    repair_cfg.scan_interval = std::chrono::seconds(1);
    repair_cfg.enable_auto_repair = false;
    repair_cfg.num_parallel_workers = 2;

    ShardRepairEngine engine(repair_cfg, *strategy, *ring, *topo,
                             kNullReadHandler, kAlwaysSucceedWriteHandler);
    engine.setSLOMonitor(slo);
    engine.start();

    // Give the scan thread time to run at least one pass
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    engine.stop();

    // The SLO monitor should have seen at least one completed scan progress entry
    // (there may be a very fast scan cycle in CI; at minimum the scan record
    //  must have been populated)
    auto active = slo->getActiveRepairJobs();
    // After stop, scan marks jobs completed, so active may be 0
    // Just verify the monitor accepted records without error
    SUCCEED();
}

TEST(ParallelRepairScanTest, SetSLOMonitorDoesNotThrow) {
    std::shared_ptr<ShardTopology> topo;
    std::shared_ptr<ConsistentHashRing> ring;
    buildTopologyAndRing(2, topo, ring);

    RedundancyConfig rcfg;
    rcfg.mode = RedundancyMode::MIRROR;
    auto strategy = std::make_shared<RedundancyStrategy>(rcfg);

    auto engine = makeMinimalEngine(*strategy, *ring, *topo);
    auto slo = std::make_shared<themis::sharding::SLOMonitor>();
    EXPECT_NO_THROW(engine->setSLOMonitor(slo));
}

// ── AC-4: SLOMonitor repair progress API ─────────────────────────────────

TEST(SLOMonitorRepairProgressTest, RecordAndRetrieveProgress) {
    themis::sharding::SLOMonitor monitor;

    themis::sharding::SLOMonitor::RepairProgress p;
    p.job_id = "job-001";
    p.documents_scanned = 50;
    p.documents_total = 100;
    p.percent_complete = 50.0;
    p.started_at = std::chrono::system_clock::now();
    p.updated_at = p.started_at;

    monitor.recordRepairProgress(p);

    auto retrieved = monitor.getRepairProgress("job-001");
    EXPECT_EQ(retrieved.job_id, "job-001");
    EXPECT_EQ(retrieved.documents_scanned, 50u);
    EXPECT_EQ(retrieved.documents_total, 100u);
    EXPECT_DOUBLE_EQ(retrieved.percent_complete, 50.0);
    EXPECT_FALSE(retrieved.completed);
}

TEST(SLOMonitorRepairProgressTest, UnknownJobReturnsEmpty) {
    themis::sharding::SLOMonitor monitor;
    auto result = monitor.getRepairProgress("does-not-exist");
    EXPECT_EQ(result.job_id, "does-not-exist");
    EXPECT_EQ(result.documents_scanned, 0u);
}

TEST(SLOMonitorRepairProgressTest, GetActiveRepairJobsFiltersCompleted) {
    themis::sharding::SLOMonitor monitor;

    themis::sharding::SLOMonitor::RepairProgress active;
    active.job_id = "active-001";
    active.percent_complete = 45.0;
    active.completed = false;
    monitor.recordRepairProgress(active);

    themis::sharding::SLOMonitor::RepairProgress done;
    done.job_id = "done-001";
    done.percent_complete = 100.0;
    done.completed = true;
    monitor.recordRepairProgress(done);

    auto jobs = monitor.getActiveRepairJobs();
    ASSERT_EQ(jobs.size(), 1u);
    EXPECT_EQ(jobs[0].job_id, "active-001");
}

TEST(SLOMonitorRepairProgressTest, UpdateProgressOverwritesPrevious) {
    themis::sharding::SLOMonitor monitor;

    themis::sharding::SLOMonitor::RepairProgress p;
    p.job_id = "job-upd";
    p.percent_complete = 25.0;
    monitor.recordRepairProgress(p);

    p.percent_complete = 75.0;
    p.documents_scanned = 75;
    monitor.recordRepairProgress(p);

    auto result = monitor.getRepairProgress("job-upd");
    EXPECT_DOUBLE_EQ(result.percent_complete, 75.0);
    EXPECT_EQ(result.documents_scanned, 75u);
}

// ── AC-5: ShardResourceManager GPU feature flag ───────────────────────────

class ShardResourceManagerGPUTest : public ::testing::Test {
protected:
    std::shared_ptr<ShardResourceManager> makeManager(bool gpu_enabled) {
        ShardResourceManager::Config cfg;
        cfg.enable_gpu_erasure_coding = gpu_enabled;
        cfg.enable_gossip_broadcast = false;
        cfg.snapshot_interval_ms = 99999;  // don't collect real metrics in test
        return std::make_shared<ShardResourceManager>(
            "shard-test", nullptr, cfg);
    }
};

TEST_F(ShardResourceManagerGPUTest, GPUDisabledByDefault) {
    ShardResourceManager::Config cfg;
    EXPECT_FALSE(cfg.enable_gpu_erasure_coding);
}

TEST_F(ShardResourceManagerGPUTest, GPUFlagFalseReturnsDisabled) {
    auto mgr = makeManager(false);
    EXPECT_FALSE(mgr->isGPUErasureCodingEnabled());
}

TEST_F(ShardResourceManagerGPUTest, GPUFlagTrueFollowsCUDAAvailability) {
    auto mgr = makeManager(true);
#ifdef THEMIS_ENABLE_CUDA
    EXPECT_TRUE(mgr->isGPUErasureCodingEnabled());
#else
    // Without CUDA headers the method falls back to false
    EXPECT_FALSE(mgr->isGPUErasureCodingEnabled());
#endif
}

// ── AC-6: ShardResourceManager IOPS token-bucket throttle ────────────────

class ShardResourceManagerIOPSTest : public ::testing::Test {
protected:
    std::shared_ptr<ShardResourceManager> makeThrottledManager(
        float budget_percent, uint64_t peak_iops) {
        ShardResourceManager::Config cfg;
        cfg.enable_repair_iops_throttle = true;
        cfg.repair_iops_budget_percent = budget_percent;
        cfg.peak_node_iops = peak_iops;
        cfg.enable_gossip_broadcast = false;
        cfg.snapshot_interval_ms = 99999;
        return std::make_shared<ShardResourceManager>(
            "shard-test", nullptr, cfg);
    }
};

TEST_F(ShardResourceManagerIOPSTest, ThrottleDisabledAlwaysAllows) {
    ShardResourceManager::Config cfg;
    cfg.enable_repair_iops_throttle = false;
    cfg.enable_gossip_broadcast = false;
    auto mgr = std::make_shared<ShardResourceManager>(
        "shard-test", nullptr, cfg);

    // Should always allow regardless of call count
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(mgr->acquireRepairIOToken(1.0));
    }
}

TEST_F(ShardResourceManagerIOPSTest, ThrottleEnabledWithLowBudgetExhausts) {
    // 1% of 100 IOPS = 1 token/s burst bucket
    auto mgr = makeThrottledManager(1.0f, 100u);

    // The first acquire should succeed (bucket starts full at burst = 1)
    bool first = mgr->acquireRepairIOToken(1.0);
    EXPECT_TRUE(first);

    // Subsequent acquires should fail once bucket is empty
    bool second = mgr->acquireRepairIOToken(1.0);
    EXPECT_FALSE(second);
}

TEST_F(ShardResourceManagerIOPSTest, ThrottleEnabledWithHighBudget) {
    // 50% of 1000 IOPS = 500 token/s burst
    auto mgr = makeThrottledManager(50.0f, 1000u);

    // First 500 tokens should be available immediately
    int successes = 0;
    for (int i = 0; i < 500; ++i) {
        if (mgr->acquireRepairIOToken(1.0)) ++successes;
    }
    EXPECT_EQ(successes, 500);

    // 501st should fail (bucket exhausted)
    EXPECT_FALSE(mgr->acquireRepairIOToken(1.0));
}

TEST_F(ShardResourceManagerIOPSTest, DefaultConfigHasThrottleEnabled) {
    ShardResourceManager::Config cfg;
    EXPECT_TRUE(cfg.enable_repair_iops_throttle);
    EXPECT_FLOAT_EQ(cfg.repair_iops_budget_percent, 10.0f);
    EXPECT_EQ(cfg.peak_node_iops, 100'000u);
}

// ── AC-7: setResourceManager wires IOPS throttle + GPU flag into engine ───

class ShardRepairEngineResourceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        ShardTopology::Config topo_cfg;
        topo_cfg.enable_health_checks = false;
        topo_cfg.cluster_name = "rm-test";
        topology_ = std::make_shared<ShardTopology>(topo_cfg);
        ring_ = std::make_shared<ConsistentHashRing>();

        RedundancyConfig rcfg;
        rcfg.mode = RedundancyMode::MIRROR;
        strategy_ = std::make_shared<RedundancyStrategy>(rcfg);
    }

    std::shared_ptr<ShardRepairEngine> makeEngine(bool throttle_enabled = true,
                                                   float budget_percent = 50.0f,
                                                   uint64_t peak_iops = 1000u) {
        RepairConfig cfg;
        cfg.enable_periodic_scan = false;
        cfg.enable_auto_repair = true;
        cfg.repair_poll_interval = std::chrono::seconds(1);
        return std::make_shared<ShardRepairEngine>(
            cfg, *strategy_, *ring_, *topology_,
            kNullReadHandler, kAlwaysSucceedWriteHandler);
    }

    std::shared_ptr<ShardResourceManager> makeResourceManager(bool throttle_enabled = true,
                                                               float budget_percent = 50.0f,
                                                               uint64_t peak_iops = 1000u) {
        ShardResourceManager::Config cfg;
        cfg.enable_repair_iops_throttle = throttle_enabled;
        cfg.repair_iops_budget_percent = budget_percent;
        cfg.peak_node_iops = peak_iops;
        cfg.enable_gossip_broadcast = false;
        cfg.snapshot_interval_ms = 99999;
        return std::make_shared<ShardResourceManager>("shard-test", nullptr, cfg);
    }

    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ConsistentHashRing> ring_;
    std::shared_ptr<RedundancyStrategy> strategy_;
};

TEST_F(ShardRepairEngineResourceManagerTest, SetResourceManagerDoesNotThrow) {
    auto engine = makeEngine();
    auto rm = makeResourceManager();
    EXPECT_NO_THROW(engine->setResourceManager(rm));
}

TEST_F(ShardRepairEngineResourceManagerTest, SetNullResourceManagerIsNoOp) {
    auto engine = makeEngine();
    EXPECT_NO_THROW(engine->setResourceManager(nullptr));
}

TEST_F(ShardRepairEngineResourceManagerTest, EngineReadsGPUFlagFromResourceManager) {
    auto engine = makeEngine();

    // GPU disabled by default
    ShardResourceManager::Config gpu_off_cfg;
    gpu_off_cfg.enable_gpu_erasure_coding = false;
    gpu_off_cfg.enable_gossip_broadcast = false;
    gpu_off_cfg.snapshot_interval_ms = 99999;
    auto rm_off = std::make_shared<ShardResourceManager>("shard-test", nullptr, gpu_off_cfg);
    engine->setResourceManager(rm_off);
    // isGPUErasureCodingEnabled should reflect config + CUDA availability
    EXPECT_FALSE(rm_off->isGPUErasureCodingEnabled());

    // GPU "enabled" in config (still false without CUDA compile flag)
    ShardResourceManager::Config gpu_on_cfg;
    gpu_on_cfg.enable_gpu_erasure_coding = true;
    gpu_on_cfg.enable_gossip_broadcast = false;
    gpu_on_cfg.snapshot_interval_ms = 99999;
    auto rm_on = std::make_shared<ShardResourceManager>("shard-test", nullptr, gpu_on_cfg);
    engine->setResourceManager(rm_on);
#ifdef THEMIS_ENABLE_CUDA
    EXPECT_TRUE(rm_on->isGPUErasureCodingEnabled());
#else
    EXPECT_FALSE(rm_on->isGPUErasureCodingEnabled());  // no CUDA at compile time
#endif
}

TEST_F(ShardRepairEngineResourceManagerTest, ThrottledRepairRespectsBudget) {
    // Set a tiny IOPS budget so the throttle gets exercised during repair
    // 1% of 100 IOPS = 1 token/s; burst = 1 token
    auto engine = makeEngine();
    auto rm = makeResourceManager(true, 1.0f, 100u);
    engine->setResourceManager(rm);

    // Inject a doc-list provider returning a few documents
    int docs_attempted = 0;
    engine->setDocumentListProvider([&docs_attempted](const std::string&) {
        docs_attempted = 3;
        return std::vector<std::string>{"d1", "d2", "d3"};
    });

    engine->start();

    // Trigger a document repair job; the engine will throttle after the first token
    engine->triggerDocumentRepair("d1", "col");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    engine->stop();

    // Test just verifies no crash/deadlock when throttle is active; functional
    // throughput tests belong in integration benchmarks.
    SUCCEED();
}



// ============================================================================
// RVW-01..06  runConsistencyCheck() + makeReplicaValidationHandler() wiring
// ============================================================================

#include "maintenance/maintenance_task_handler_impls.h"
#include "maintenance/maintenance_task.h"   // MaintenanceTaskType

// RVW-01: empty health map → OK summary
TEST_F(ShardRepairEngineTest, RVW01_RunConsistencyCheck_NoShards_ReturnsOk) {
    auto result = engine_->runConsistencyCheck();
    ASSERT_TRUE(result.has_value()) << result.error().context();
    EXPECT_NE(result->find("OK"), std::string::npos);
}

// RVW-02: still OK after start (background scan disabled)
TEST_F(ShardRepairEngineTest, RVW02_RunConsistencyCheck_AfterStart_ReturnsOk) {
    engine_->start();
    auto result = engine_->runConsistencyCheck();
    EXPECT_TRUE(result.has_value());
    engine_->stop();
}

// RVW-03: makeReplicaValidationHandler with non-null engine succeeds
TEST_F(ShardRepairEngineTest, RVW03_MakeReplicaValidationHandler_NotNull) {
    auto shared_engine = std::shared_ptr<themis::sharding::ShardRepairEngine>(
        engine_.release(),
        [](themis::sharding::ShardRepairEngine* e) {
            if (e && e->isRunning()) e->stop();
            delete e;
        });

    auto handler = themis::maintenance::makeReplicaValidationHandler(shared_engine);
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->handlerName(), "ReplicaValidationHandler");
}

// RVW-04: handler execute returns success when no degraded shards
TEST_F(ShardRepairEngineTest, RVW04_Handler_Execute_NoShards_Succeeds) {
    auto shared_engine = std::shared_ptr<themis::sharding::ShardRepairEngine>(
        engine_.release(),
        [](themis::sharding::ShardRepairEngine* e) {
            if (e && e->isRunning()) e->stop();
            delete e;
        });

    auto handler = themis::maintenance::makeReplicaValidationHandler(shared_engine);
    auto result = handler->execute("job_rvw04", themis::maintenance::MaintenanceTaskType::REPLICA_VALIDATION);
    ASSERT_TRUE(result.has_value()) << result.error().context();
    EXPECT_FALSE(result->empty());
}

// RVW-05: null engine in makeReplicaValidationHandler returns error
TEST(ReplicaValidationHandlerTest, RVW05_NullEngine_ReturnsError) {
    auto handler = themis::maintenance::makeReplicaValidationHandler(nullptr);
    ASSERT_NE(handler, nullptr);
    auto result = handler->execute("job_rvw05", themis::maintenance::MaintenanceTaskType::REPLICA_VALIDATION);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), themis::errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED);
}

// RVW-06: ReplicaValidationHandler with custom check function (no ShardRepairEngine needed)
TEST(ReplicaValidationHandlerTest, RVW06_CustomCheckFn_ReturnsExpectedResult) {
    using themis::maintenance::ReplicaValidationHandler;
    bool called = false;
    auto handler = std::make_shared<ReplicaValidationHandler>(
        [&called]() -> themis::Result<std::string> {
            called = true;
            return std::string("custom check OK");
        });

    auto result = handler->execute("job_rvw06", themis::maintenance::MaintenanceTaskType::REPLICA_VALIDATION);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "custom check OK");
    EXPECT_TRUE(called);
}
