/**
 * ThemisDB GeoTopologyApiHandler Tests
 *
 * Tests that the geo topology REST API correctly:
 * - Returns shard topology with region/zone metadata
 * - Aggregates per-region health information
 * - Reports overall geo health (failed/degraded/healthy regions)
 * - Adds and updates shards via POST
 * - Gets and sets geo-replication configuration per collection
 */

#include <gtest/gtest.h>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>

#include "server/geo_topology_api_handler.h"
#include "sharding/shard_topology.h"
#include "sharding/redundancy_strategy.h"

namespace beast = boost::beast;
namespace http  = beast::http;
using json = nlohmann::json;

using namespace themis;
using namespace themis::server;
using namespace themis::sharding;

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class GeoTopologyApiHandlerTest : public ::testing::Test {
protected:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<CollectionRedundancyManager> redundancy_mgr_;
    std::unique_ptr<GeoTopologyApiHandler> handler_;

    void SetUp() override {
        topology_       = std::make_shared<ShardTopology>();
        redundancy_mgr_ = std::make_shared<CollectionRedundancyManager>();

        // Populate topology: 3 us-east + 3 eu-west shards
        auto add = [&](const std::string& id, const std::string& region,
                       const std::string& zone, bool healthy = true) {
            ShardInfo s;
            s.shard_id   = id;
            s.region     = region;
            s.zone       = zone;
            s.datacenter = region;
            s.is_healthy = healthy;
            topology_->addShard(s);
        };
        add("shard-0", "us-east", "us-east-1a");
        add("shard-1", "us-east", "us-east-1b");
        add("shard-2", "us-east", "us-east-1c");
        add("shard-3", "eu-west", "eu-west-1a");
        add("shard-4", "eu-west", "eu-west-1b");
        add("shard-5", "eu-west", "eu-west-1c");

        handler_ = std::make_unique<GeoTopologyApiHandler>(
            topology_, redundancy_mgr_, nullptr /* no auth */);
    }

    // Helper to build a minimal HTTP request
    http::request<http::string_body> makeGet(const std::string& target) {
        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, "localhost");
        return req;
    }

    http::request<http::string_body> makePost(const std::string& target,
                                               const std::string& body) {
        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, "localhost");
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();
        return req;
    }

    http::request<http::string_body> makePut(const std::string& target,
                                              const std::string& body) {
        http::request<http::string_body> req{http::verb::put, target, 11};
        req.set(http::field::host, "localhost");
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();
        return req;
    }

    http::request<http::string_body> makeDelete(const std::string& target) {
        http::request<http::string_body> req{http::verb::delete_, target, 11};
        req.set(http::field::host, "localhost");
        return req;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/geo/topology
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GeoTopologyApiHandlerTest, TopologyGet_ReturnsAllShards) {
    auto req  = makeGet("/api/v1/geo/topology");
    auto resp = handler_->handleTopologyGet(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_TRUE(j.contains("shards"));
    EXPECT_TRUE(j.contains("total"));
    EXPECT_EQ(j["total"].get<int>(), 6);

    const auto& shards = j["shards"];
    ASSERT_EQ(shards.size(), 6u);

    // Find shard-0 and verify region/zone
    auto it = std::find_if(shards.begin(), shards.end(),
                           [](const json& e) { return e["shard_id"] == "shard-0"; });
    ASSERT_NE(it, shards.end());
    EXPECT_EQ((*it)["region"].get<std::string>(), "us-east");
    EXPECT_EQ((*it)["zone"].get<std::string>(), "us-east-1a");
    EXPECT_TRUE((*it)["is_healthy"].get<bool>());
}

TEST_F(GeoTopologyApiHandlerTest, TopologyGet_NoTopology_ServiceUnavailable) {
    GeoTopologyApiHandler no_topo(nullptr, redundancy_mgr_, nullptr);
    auto req  = makeGet("/api/v1/geo/topology");
    auto resp = no_topo.handleTopologyGet(req);
    EXPECT_EQ(resp.result(), http::status::service_unavailable);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/geo/regions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GeoTopologyApiHandlerTest, RegionsGet_ReturnsRegionSummaries) {
    auto req  = makeGet("/api/v1/geo/regions");
    auto resp = handler_->handleRegionsGet(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_EQ(j["total_regions"].get<int>(), 2);

    const auto& regions = j["regions"];
    ASSERT_EQ(regions.size(), 2u);

    for (const auto& r : regions) {
        EXPECT_EQ(r["total_shards"].get<int>(), 3);
        EXPECT_EQ(r["healthy_shards"].get<int>(), 3);
        EXPECT_TRUE(r["has_majority_quorum"].get<bool>());
    }
}

TEST_F(GeoTopologyApiHandlerTest, RegionsGet_DegradedRegion) {
    // Mark 2 eu-west shards unhealthy (minority healthy)
    topology_->updateHealth("shard-3", false);
    topology_->updateHealth("shard-4", false);

    auto req  = makeGet("/api/v1/geo/regions");
    auto resp = handler_->handleRegionsGet(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    const auto& regions = j["regions"];

    for (const auto& r : regions) {
        if (r["region"].get<std::string>() == "eu-west") {
            EXPECT_EQ(r["healthy_shards"].get<int>(), 1);
            EXPECT_FALSE(r["has_majority_quorum"].get<bool>());
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/geo/health
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GeoTopologyApiHandlerTest, HealthGet_AllHealthy) {
    auto req  = makeGet("/api/v1/geo/health");
    auto resp = handler_->handleHealthGet(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_EQ(j["overall_status"].get<std::string>(), "healthy");
    EXPECT_EQ(j["total_shards"].get<int>(), 6);
    EXPECT_EQ(j["healthy_shards"].get<int>(), 6);
    EXPECT_TRUE(j["failed_regions"].empty());
    EXPECT_TRUE(j["degraded_regions"].empty());
}

TEST_F(GeoTopologyApiHandlerTest, HealthGet_FullRegionFailure) {
    // All eu-west shards down
    topology_->updateHealth("shard-3", false);
    topology_->updateHealth("shard-4", false);
    topology_->updateHealth("shard-5", false);

    auto req  = makeGet("/api/v1/geo/health");
    auto resp = handler_->handleHealthGet(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_EQ(j["overall_status"].get<std::string>(), "degraded");
    EXPECT_FALSE(j["failed_regions"].empty());

    bool found_eu_west = false;
    for (const auto& fr : j["failed_regions"]) {
        if (fr.get<std::string>() == "eu-west") found_eu_west = true;
    }
    EXPECT_TRUE(found_eu_west);
}

TEST_F(GeoTopologyApiHandlerTest, HealthGet_PartialRegionFailure) {
    // Only one eu-west shard down → degraded
    topology_->updateHealth("shard-3", false);

    auto req  = makeGet("/api/v1/geo/health");
    auto resp = handler_->handleHealthGet(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_EQ(j["overall_status"].get<std::string>(), "partial");
    EXPECT_FALSE(j["degraded_regions"].empty());
    EXPECT_TRUE(j["failed_regions"].empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /api/v1/geo/topology/shard
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GeoTopologyApiHandlerTest, TopologyShardPost_CreateNewShard) {
    const std::string body = R"({
        "shard_id": "shard-new",
        "region":   "ap-south",
        "zone":     "ap-south-1a",
        "datacenter": "ap-south",
        "is_healthy": true
    })";
    auto req  = makePost("/api/v1/geo/topology/shard", body);
    auto resp = handler_->handleTopologyShardPost(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_TRUE(j["ok"].get<bool>());
    EXPECT_EQ(j["action"].get<std::string>(), "created");
    EXPECT_EQ(j["region"].get<std::string>(), "ap-south");
    EXPECT_EQ(j["zone"].get<std::string>(), "ap-south-1a");

    // Verify topology was updated
    auto info = topology_->getShard("shard-new");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->region, "ap-south");
    EXPECT_EQ(info->zone, "ap-south-1a");
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardPost_UpdateExistingShard) {
    const std::string body = R"({"shard_id": "shard-0", "zone": "us-east-1z"})";
    auto req  = makePost("/api/v1/geo/topology/shard", body);
    auto resp = handler_->handleTopologyShardPost(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_EQ(j["action"].get<std::string>(), "updated");

    auto info = topology_->getShard("shard-0");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->zone, "us-east-1z");
    // Region should be unchanged
    EXPECT_EQ(info->region, "us-east");
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardPost_MissingShardId_BadRequest) {
    const std::string body = R"({"region": "us-east"})";
    auto req  = makePost("/api/v1/geo/topology/shard", body);
    auto resp = handler_->handleTopologyShardPost(req);

    EXPECT_EQ(resp.result(), http::status::bad_request);
    auto j = json::parse(resp.body());
    EXPECT_TRUE(j["error"].get<bool>());
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardPost_InvalidJson_BadRequest) {
    auto req  = makePost("/api/v1/geo/topology/shard", "not-json!");
    auto resp = handler_->handleTopologyShardPost(req);
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardPost_EmptyShardId_BadRequest) {
    const std::string body = R"({"shard_id": "", "region": "us-east"})";
    auto req  = makePost("/api/v1/geo/topology/shard", body);
    auto resp = handler_->handleTopologyShardPost(req);
    EXPECT_EQ(resp.result(), http::status::bad_request);
    auto j = json::parse(resp.body());
    EXPECT_TRUE(j["error"].get<bool>());
    // Error message must mention the validation failure
    EXPECT_NE(j["message"].get<std::string>().find("shard_id"), std::string::npos);
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardPost_InvalidShardId_BadRequest) {
    const std::string body = R"({"shard_id": "../shard-new", "region": "us-east"})";
    auto req  = makePost("/api/v1/geo/topology/shard", body);
    auto resp = handler_->handleTopologyShardPost(req);

    EXPECT_EQ(resp.result(), http::status::bad_request);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/geo/config/{collection}
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GeoTopologyApiHandlerTest, ConfigGet_DefaultConfig) {
    auto req  = makeGet("/api/v1/geo/config/mycollection");
    auto resp = handler_->handleConfigGet(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_EQ(j["collection"].get<std::string>(), "mycollection");
    EXPECT_TRUE(j.contains("replication_factor"));
    EXPECT_TRUE(j.contains("replication_mode"));
    // Mode must be a non-hardcoded reflection of the actual config
    EXPECT_TRUE(j.contains("mode"));
    EXPECT_FALSE(j["mode"].get<std::string>().empty());
}

TEST_F(GeoTopologyApiHandlerTest, ConfigGet_MissingCollection_BadRequest) {
    // Path with no trailing segment
    auto req  = makeGet("/api/v1/geo/config/");
    auto resp = handler_->handleConfigGet(req);
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST_F(GeoTopologyApiHandlerTest, ConfigGet_InvalidCollection_BadRequest) {
    auto req  = makeGet("/api/v1/geo/config/../mycollection");
    auto resp = handler_->handleConfigGet(req);
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

// ─────────────────────────────────────────────────────────────────────────────
// PUT /api/v1/geo/config/{collection}
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GeoTopologyApiHandlerTest, ConfigPut_UpdateGeoConfig) {
    const std::string body = R"({
        "replication_factor": 6,
        "replication_mode": "sync",
        "local_region": "us-east",
        "enable_geo_failover": true,
        "region_failure_threshold": 0.5,
        "region_write_quorums": {"us-east": 2, "eu-west": 1},
        "region_read_quorums":  {"us-east": 1}
    })";
    auto req  = makePut("/api/v1/geo/config/orders", body);
    auto resp = handler_->handleConfigPut(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_TRUE(j["ok"].get<bool>());
    EXPECT_EQ(j["collection"].get<std::string>(), "orders");

    // Verify persisted
    auto cfg = redundancy_mgr_->getConfig("orders");
    EXPECT_EQ(cfg.mode, RedundancyMode::GEO_MIRROR);
    EXPECT_EQ(cfg.replication_factor, 6u);
    EXPECT_EQ(cfg.geo_replication.local_region, "us-east");
    EXPECT_TRUE(cfg.geo_replication.enable_geo_failover);
    EXPECT_EQ(cfg.geo_replication.region_write_quorums.at("us-east"), 2u);
    EXPECT_EQ(cfg.geo_replication.region_write_quorums.at("eu-west"), 1u);
    EXPECT_EQ(cfg.geo_replication.region_read_quorums.at("us-east"), 1u);
}

TEST_F(GeoTopologyApiHandlerTest, ConfigPut_InvalidQuorum_BadRequest) {
    // quorum (10) > replication_factor (3)
    const std::string body = R"({
        "replication_factor": 3,
        "region_write_quorums": {"us-east": 10}
    })";
    auto req  = makePut("/api/v1/geo/config/bad_coll", body);
    auto resp = handler_->handleConfigPut(req);
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST_F(GeoTopologyApiHandlerTest, ConfigPut_InvalidJson_BadRequest) {
    auto req  = makePut("/api/v1/geo/config/badcoll", "{{bad json}}");
    auto resp = handler_->handleConfigPut(req);
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST_F(GeoTopologyApiHandlerTest, ConfigPut_MissingCollection_BadRequest) {
    auto req  = makePut("/api/v1/geo/config/", R"({})");
    auto resp = handler_->handleConfigPut(req);
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST_F(GeoTopologyApiHandlerTest, ConfigGet_AfterConfigPut_Roundtrip) {
    // Write
    const std::string body = R"({
        "replication_factor": 4,
        "local_region": "eu-west",
        "max_staleness_ms": 250,
        "region_write_quorums": {"eu-west": 2}
    })";
    auto put_req  = makePut("/api/v1/geo/config/rt_coll", body);
    auto put_resp = handler_->handleConfigPut(put_req);
    ASSERT_EQ(put_resp.result(), http::status::ok);

    // Read back
    auto get_req  = makeGet("/api/v1/geo/config/rt_coll");
    auto get_resp = handler_->handleConfigGet(get_req);
    ASSERT_EQ(get_resp.result(), http::status::ok);

    auto j = json::parse(get_resp.body());
    EXPECT_EQ(j["collection"].get<std::string>(), "rt_coll");
    EXPECT_EQ(j["replication_factor"].get<int>(), 4);
    EXPECT_EQ(j["local_region"].get<std::string>(), "eu-west");
    EXPECT_EQ(j["max_staleness_ms"].get<int>(), 250);
    EXPECT_EQ(j["region_write_quorums"]["eu-west"].get<int>(), 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// DELETE /api/v1/geo/topology/shard/{shard_id}
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GeoTopologyApiHandlerTest, TopologyShardDelete_RemovesExistingShard) {
    // shard-0 was added in SetUp; verify it exists first
    ASSERT_TRUE(topology_->getShard("shard-0").has_value());

    auto req  = makeDelete("/api/v1/geo/topology/shard/shard-0");
    auto resp = handler_->handleTopologyShardDelete(req);

    EXPECT_EQ(resp.result(), http::status::ok);

    auto j = json::parse(resp.body());
    EXPECT_TRUE(j["ok"].get<bool>());
    EXPECT_TRUE(j["removed"].get<bool>());
    EXPECT_EQ(j["shard_id"].get<std::string>(), "shard-0");

    // Topology should no longer contain shard-0
    EXPECT_FALSE(topology_->getShard("shard-0").has_value());
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardDelete_NotFound_Returns404) {
    auto req  = makeDelete("/api/v1/geo/topology/shard/nonexistent-shard");
    auto resp = handler_->handleTopologyShardDelete(req);
    EXPECT_EQ(resp.result(), http::status::not_found);

    auto j = json::parse(resp.body());
    EXPECT_TRUE(j["error"].get<bool>());
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardDelete_MissingShardIdInPath_BadRequest) {
    // Path ends at the shard/ prefix with no trailing segment
    auto req  = makeDelete("/api/v1/geo/topology/shard/");
    auto resp = handler_->handleTopologyShardDelete(req);
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardDelete_InvalidShardIdInPath_BadRequest) {
    auto req  = makeDelete("/api/v1/geo/topology/shard/../shard-0");
    auto resp = handler_->handleTopologyShardDelete(req);
    EXPECT_EQ(resp.result(), http::status::bad_request);
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardDelete_NoTopology_ServiceUnavailable) {
    GeoTopologyApiHandler no_topo(nullptr, redundancy_mgr_, nullptr);
    auto req  = makeDelete("/api/v1/geo/topology/shard/shard-0");
    auto resp = no_topo.handleTopologyShardDelete(req);
    EXPECT_EQ(resp.result(), http::status::service_unavailable);
}

TEST_F(GeoTopologyApiHandlerTest, TopologyShardDelete_UpdatesRegionCount) {
    // Before: eu-west has 3 shards (shard-3, shard-4, shard-5)
    auto before = handler_->handleRegionsGet(makeGet("/api/v1/geo/regions"));
    auto jb = json::parse(before.body());
    int eu_west_before = 0;
    for (const auto& r : jb["regions"]) {
        if (r["region"].get<std::string>() == "eu-west")
            eu_west_before = r["total_shards"].get<int>();
    }
    EXPECT_EQ(eu_west_before, 3);

    // Delete one eu-west shard
    auto del_resp = handler_->handleTopologyShardDelete(
        makeDelete("/api/v1/geo/topology/shard/shard-3"));
    ASSERT_EQ(del_resp.result(), http::status::ok);

    // After: eu-west should have 2 shards
    auto after = handler_->handleRegionsGet(makeGet("/api/v1/geo/regions"));
    auto ja = json::parse(after.body());
    int eu_west_after = 0;
    for (const auto& r : ja["regions"]) {
        if (r["region"].get<std::string>() == "eu-west")
            eu_west_after = r["total_shards"].get<int>();
    }
    EXPECT_EQ(eu_west_after, 2);
}
