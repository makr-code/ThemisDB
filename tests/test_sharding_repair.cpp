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
#include "sharding/admin_api.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::sharding;

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
        for (size_t i = 0; i < original.size(); ++i) {
            EXPECT_EQ(recovered[i], original[i])
                << "Mismatch at byte " << i << " (dropped chunks: ";
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

// ============================================================================
// ShardRepairEngine lifecycle tests
// ============================================================================

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
            [](const std::string&, const std::string&) -> std::optional<std::vector<uint8_t>> {
                return std::nullopt;  // Mock: all reads fail (simulates missing data)
            },
            [](const std::string&, const std::string&, const std::vector<uint8_t>&) -> bool {
                return true;  // Mock: all writes succeed
            }
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
