/**
 * @file test_rpc_geo_query.cpp
 * @brief Unit tests for RPC geospatial query functionality
 */

#include <gtest/gtest.h>
#include "server/rpc_service_impl.h"
#include "index/spatial_index.h"
#include "storage/rocksdb_wrapper.h"
#include "api/geo_index_hooks.h"
#include "geo/spatial_backend.h"
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace themis;
using namespace themis::server::rpc;
using json = nlohmann::json;

class RPCGeoQueryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary RocksDB instance
        RocksDBWrapper::Config cfg;
        cfg.db_path = "test_rpc_geo_query_db";
        cfg.memtable_size_mb = 16;
        cfg.block_cache_size_mb = 16;
        
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        
        spatial_mgr_ = std::make_unique<index::SpatialIndexManager>(*db_);
        
        // Create spatial index for test collection
        index::RTreeConfig config;
        config.total_bounds = geo::MBR(-180.0, -90.0, 180.0, 90.0);
        auto status = spatial_mgr_->createSpatialIndex("test_collection", "geometry", config);
        ASSERT_TRUE(status.ok);
        
        // Create RPC service with spatial index
        rpc_service_ = std::make_unique<ThemisRPCService>(db_.get(), spatial_mgr_.get());

        // Wire the GPU spatial backend so the exact-check path is exercised.
        // getGpuSpatialBackend() returns a non-null singleton and isAvailable()
        // returns true on CPU-only machines via the CPU fallback path.
        auto* gpu_backend = geo::getGpuSpatialBackend();
        if (gpu_backend) {
            spatial_mgr_->setExactBackend(gpu_backend);
        }
        
        // Insert some test data with geometries
        insertTestData();
    }
    
    void TearDown() override {
        rpc_service_.reset();
        spatial_mgr_.reset();
        db_.reset();
        std::filesystem::remove_all("test_rpc_geo_query_db");
    }
    
    void insertTestData() {
        // Insert entities with different locations
        
        // Entity 1: Berlin (13.4°E, 52.5°N)
        json entity1;
        entity1["id"] = "berlin";
        entity1["name"] = "Berlin";
        entity1["geometry"] = {
            {"type", "Point"},
            {"coordinates", json::array({13.4, 52.5})}
        };
        
        std::string blob_str1 = entity1.dump();
        std::vector<uint8_t> blob1(blob_str1.begin(), blob_str1.end());
        db_->put("entity:test_collection:berlin", blob1);
        api::GeoIndexHooks::onEntityPut(*db_, spatial_mgr_.get(), "test_collection", "berlin", blob1);
        
        // Entity 2: Paris (2.3°E, 48.9°N)
        json entity2;
        entity2["id"] = "paris";
        entity2["name"] = "Paris";
        entity2["geometry"] = {
            {"type", "Point"},
            {"coordinates", json::array({2.3, 48.9})}
        };
        
        std::string blob_str2 = entity2.dump();
        std::vector<uint8_t> blob2(blob_str2.begin(), blob_str2.end());
        db_->put("entity:test_collection:paris", blob2);
        api::GeoIndexHooks::onEntityPut(*db_, spatial_mgr_.get(), "test_collection", "paris", blob2);
        
        // Entity 3: London (0.1°W, 51.5°N)
        json entity3;
        entity3["id"] = "london";
        entity3["name"] = "London";
        entity3["geometry"] = {
            {"type", "Point"},
            {"coordinates", json::array({-0.1, 51.5})}
        };
        
        std::string blob_str3 = entity3.dump();
        std::vector<uint8_t> blob3(blob_str3.begin(), blob_str3.end());
        db_->put("entity:test_collection:london", blob3);
        api::GeoIndexHooks::onEntityPut(*db_, spatial_mgr_.get(), "test_collection", "london", blob3);
    }
    
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<index::SpatialIndexManager> spatial_mgr_;
    std::unique_ptr<ThemisRPCService> rpc_service_;
};

// Test: Intersects query with bounding box
TEST_F(RPCGeoQueryTest, IntersectsQueryWithBbox) {
    json params;
    params["collection"] = "test_collection";
    params["type"] = "intersects";
    params["bbox"] = {
        {"minx", 0.0},
        {"miny", 48.0},
        {"maxx", 15.0},
        {"maxy", 53.0}
    };
    
    json response = rpc_service_->handleGeoQuery(params);
    
    // After refactoring: Response format changed from 'data' to 'result'
    ASSERT_TRUE(response.contains("result"));
    ASSERT_TRUE(response["result"].contains("results"));
    ASSERT_TRUE(response["result"].contains("count"));
    
    auto results = response["result"]["results"];
    EXPECT_GE(results.size(), 1);  // Should find at least Berlin
    
    // Check that results contain expected fields
    for (const auto& result : results) {
        EXPECT_TRUE(result.contains("primary_key"));
        EXPECT_TRUE(result.contains("mbr"));
    }
}

// Test: Within query (alias for intersects)
TEST_F(RPCGeoQueryTest, WithinQueryWithBbox) {
    json params;
    params["collection"] = "test_collection";
    params["type"] = "within";
    params["bbox"] = {
        {"minx", -1.0},
        {"miny", 51.0},
        {"maxx", 1.0},
        {"maxy", 52.0}
    };
    
    json response = rpc_service_->handleGeoQuery(params);
    
    // After refactoring: Response format changed from 'data' to 'result'
    ASSERT_TRUE(response.contains("result"));
    ASSERT_TRUE(response["result"].contains("results"));
    
    auto results = response["result"]["results"];
    EXPECT_GE(results.size(), 1);  // Should find London
}

// Test: Near query with center point and radius
TEST_F(RPCGeoQueryTest, NearQueryWithCenterAndRadius) {
    json params;
    params["collection"] = "test_collection";
    params["type"] = "near";
    params["center"] = {
        {"lon", 13.4},
        {"lat", 52.5}
    };
    params["radius"] = 50000.0;  // 50km radius around Berlin
    
    json response = rpc_service_->handleGeoQuery(params);
    
    // After refactoring: Response format changed from 'data' to 'result'
    ASSERT_TRUE(response.contains("result"));
    ASSERT_TRUE(response["result"].contains("results"));
    
    auto results = response["result"]["results"];
    EXPECT_GE(results.size(), 1);  // Should find Berlin itself
    
    // Check that results contain distance field
    for (const auto& result : results) {
        EXPECT_TRUE(result.contains("primary_key"));
        EXPECT_TRUE(result.contains("distance"));
        
        // Distance should be within radius
        double distance = result["distance"].get<double>();
        EXPECT_LE(distance, 50000.0);
    }
}

// Test: Error handling - missing collection parameter
TEST_F(RPCGeoQueryTest, ErrorMissingCollection) {
    json params;
    params["type"] = "intersects";
    params["bbox"] = {
        {"minx", 0.0},
        {"miny", 0.0},
        {"maxx", 10.0},
        {"maxy", 10.0}
    };
    
    json response = rpc_service_->handleGeoQuery(params);
    
    ASSERT_TRUE(response.contains("error"));
    EXPECT_TRUE(response["error"].contains("message"));
}

// Test: Error handling - collection without spatial index
TEST_F(RPCGeoQueryTest, ErrorNoSpatialIndex) {
    json params;
    params["collection"] = "non_existent_collection";
    params["type"] = "intersects";
    params["bbox"] = {
        {"minx", 0.0},
        {"miny", 0.0},
        {"maxx", 10.0},
        {"maxy", 10.0}
    };
    
    json response = rpc_service_->handleGeoQuery(params);
    
    ASSERT_TRUE(response.contains("error"));
    auto error_msg = response["error"]["message"].get<std::string>();
    EXPECT_TRUE(error_msg.find("spatial index") != std::string::npos);
}

// Test: Error handling - missing query type
TEST_F(RPCGeoQueryTest, ErrorMissingQueryType) {
    json params;
    params["collection"] = "test_collection";
    params["bbox"] = {
        {"minx", 0.0},
        {"miny", 0.0},
        {"maxx", 10.0},
        {"maxy", 10.0}
    };
    
    json response = rpc_service_->handleGeoQuery(params);
    
    ASSERT_TRUE(response.contains("error"));
}

// Test: Error handling - invalid query type
TEST_F(RPCGeoQueryTest, ErrorInvalidQueryType) {
    json params;
    params["collection"] = "test_collection";
    params["type"] = "invalid_type";
    params["bbox"] = {
        {"minx", 0.0},
        {"miny", 0.0},
        {"maxx", 10.0},
        {"maxy", 10.0}
    };
    
    json response = rpc_service_->handleGeoQuery(params);
    
    ASSERT_TRUE(response.contains("error"));
    auto error_msg = response["error"]["message"].get<std::string>();
    EXPECT_TRUE(error_msg.find("Supported types") != std::string::npos);
}

// Test: Error handling - missing bbox for intersects query
TEST_F(RPCGeoQueryTest, ErrorMissingBbox) {
    json params;
    params["collection"] = "test_collection";
    params["type"] = "intersects";
    
    json response = rpc_service_->handleGeoQuery(params);
    
    ASSERT_TRUE(response.contains("error"));
    auto error_msg = response["error"]["message"].get<std::string>();
    EXPECT_TRUE(error_msg.find("bbox") != std::string::npos);
}

// Test: Error handling - missing center for near query
TEST_F(RPCGeoQueryTest, ErrorMissingCenter) {
    json params;
    params["collection"] = "test_collection";
    params["type"] = "near";
    params["radius"] = 1000.0;
    
    json response = rpc_service_->handleGeoQuery(params);
    
    ASSERT_TRUE(response.contains("error"));
    auto error_msg = response["error"]["message"].get<std::string>();
    EXPECT_TRUE(error_msg.find("center") != std::string::npos);
}

// Test: Error handling - missing radius for near query
TEST_F(RPCGeoQueryTest, ErrorMissingRadius) {
    json params;
    params["collection"] = "test_collection";
    params["type"] = "near";
    params["center"] = {
        {"lon", 13.4},
        {"lat", 52.5}
    };
    
    json response = rpc_service_->handleGeoQuery(params);
    
    ASSERT_TRUE(response.contains("error"));
    auto error_msg = response["error"]["message"].get<std::string>();
    EXPECT_TRUE(error_msg.find("radius") != std::string::npos);
}
