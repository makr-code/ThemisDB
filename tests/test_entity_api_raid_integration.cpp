/**
 * ThemisDB Entity API Handler RAID Integration Test
 * 
 * Tests that RedundancyStrategy is properly integrated into EntityApiHandler
 * and that RAID modes are applied at runtime when feature_raid is enabled.
 */

#include <gtest/gtest.h>
#include "server/entity_api_handler.h"
#include "sharding/redundancy_strategy.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "server/auth_middleware.h"
#include "storage/key_schema.h"
#include <memory>
#include <string>
#include <filesystem>
#include <ctime>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

using namespace themis;
using namespace themis::server;
using namespace themis::sharding;

class EntityApiRaidIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping unstable entity RAID integration tests on Windows";
#endif
        // Create temporary directory for test database with unique suffix
        test_db_path_ = std::filesystem::temp_directory_path() / 
                       ("themis_raid_test_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(getpid()));
        std::filesystem::create_directories(test_db_path_);
        
        // Initialize storage
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_.string();
        storage_ = std::make_shared<RocksDBWrapper>(config);
        storage_->open();
        
        // Initialize indexes (they take RocksDBWrapper& not shared_ptr)
        secondary_index_ = std::make_shared<SecondaryIndexManager>(*storage_);
        graph_index_ = std::make_shared<GraphIndexManager>(*storage_);
        vector_index_ = std::make_shared<VectorIndexManager>(*storage_);
        
        // Initialize transaction manager
        tx_manager_ = std::make_shared<TransactionManager>(*storage_, *secondary_index_, *graph_index_, *vector_index_);
        
        // Initialize encryption components
        key_provider_ = std::make_shared<themis::MockKeyProvider>();
        field_encryption_ = std::make_shared<FieldEncryption>(key_provider_);
        
        // Initialize auth middleware (default constructor)
        auth_ = std::make_shared<themis::AuthMiddleware>();
        
        // Initialize RAID components
        hash_ring_ = std::make_shared<ConsistentHashRing>(100);
        hash_ring_->addNode("shard-0");
        hash_ring_->addNode("shard-1");
        hash_ring_->addNode("shard-2");
        
        shard_topology_ = std::make_shared<ShardTopology>();
        
        redundancy_manager_ = std::make_shared<CollectionRedundancyManager>();
        
        // Configure MIRROR mode for "users" collection
        RedundancyConfig raid_config;
        raid_config.mode = RedundancyMode::MIRROR;
        raid_config.replication_factor = 1;
        raid_config.write_concern = WriteConcern::MAJORITY;
        redundancy_manager_->setCollectionConfig("users", raid_config);
    }
    
    void TearDown() override {
        // Destroy managers that may hold references into storage_ before resetting storage_
        secondary_index_.reset();
        graph_index_.reset();
        vector_index_.reset();
        tx_manager_.reset();
        storage_.reset();
        
        std::filesystem::remove_all(test_db_path_);
    }
    
    std::filesystem::path test_db_path_;
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<TransactionManager> tx_manager_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    std::shared_ptr<themis::KeyProvider> key_provider_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    std::shared_ptr<ConsistentHashRing> hash_ring_;
    std::shared_ptr<ShardTopology> shard_topology_;
    std::shared_ptr<CollectionRedundancyManager> redundancy_manager_;
};

TEST_F(EntityApiRaidIntegrationTest, RaidDisabledByDefault) {
    // Create handler with RAID components but feature disabled
    EntityApiConfig config;
    config.feature_raid = false;  // Disabled
    
    EntityApiHandler handler(
        storage_,
        secondary_index_,
        graph_index_,
        tx_manager_,
        field_encryption_,
        key_provider_,
        auth_,
        config,
        nullptr,  // spatial_index
        nullptr,  // changefeed
        nullptr,  // wal_manager
        nullptr,  // replication_coordinator
        nullptr,  // multi_primary_coordinator
        redundancy_manager_,
        hash_ring_,
        shard_topology_
    );
    
    // Create a mock HTTP request
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::put);
    req.target("/entities/users:alice");
    req.set(boost::beast::http::field::content_type, "application/json");
    req.body() = R"({"key":"users:alice","blob":"{\"name\":\"Alice\",\"age\":30}"})";
    req.prepare_payload();
    
    // Execute request
    auto response = handler.handlePut(req);
    
    // Should succeed without RAID (feature disabled)
    EXPECT_EQ(response.result(), boost::beast::http::status::created);
    
    // Verify data was written to primary storage
    auto value = storage_->get(KeySchema::makeRelationalKey("users", "alice"));
    EXPECT_TRUE(value.has_value());
}

TEST_F(EntityApiRaidIntegrationTest, RaidEnabledWithComponents) {
#ifdef _WIN32
    GTEST_SKIP() << "Known hang in RAID-enabled put path on Windows in current build; other RAID integration paths remain covered.";
#endif

    // Create handler with RAID enabled
    EntityApiConfig config;
    config.feature_raid = true;  // Enabled
    
    EntityApiHandler handler(
        storage_,
        secondary_index_,
        graph_index_,
        tx_manager_,
        field_encryption_,
        key_provider_,
        auth_,
        config,
        nullptr,  // spatial_index
        nullptr,  // changefeed
        nullptr,  // wal_manager
        nullptr,  // replication_coordinator
        nullptr,  // multi_primary_coordinator
        redundancy_manager_,
        hash_ring_,
        shard_topology_
    );
    
    // Create a mock HTTP request
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::put);
    req.target("/entities/users:bob");
    req.set(boost::beast::http::field::content_type, "application/json");
    req.body() = R"({"key":"users:bob","blob":"{\"name\":\"Bob\",\"age\":25}"})";
    req.prepare_payload();
    
    // Execute request
    auto response = handler.handlePut(req);
    
    // Should succeed with RAID
    EXPECT_EQ(response.result(), boost::beast::http::status::created);
    
    // Verify data was written to primary storage
    auto value = storage_->get(KeySchema::makeRelationalKey("users", "bob"));
    EXPECT_TRUE(value.has_value());
    
    // Verify RAID replicas were written (with shard prefix)
    // Note: In the current implementation, replicas are written to local storage with shard prefix
    auto shard_0_value = storage_->get("shard-0:users:bob");
    auto shard_1_value = storage_->get("shard-1:users:bob");
    auto shard_2_value = storage_->get("shard-2:users:bob");
    
    // At least some replicas should exist (depending on write concern)
    int replica_count = 0;
    if (shard_0_value.has_value()) {
      replica_count++;
    }
    if (shard_1_value.has_value()) {
      replica_count++;
    }
    if (shard_2_value.has_value()) {
      replica_count++;
    }
    
    EXPECT_GT(replica_count, 0) << "Expected at least one RAID replica to be written";
}

TEST_F(EntityApiRaidIntegrationTest, RaidDisabledWhenComponentsMissing) {
    // Create handler with feature enabled but no RAID components
    EntityApiConfig config;
    config.feature_raid = true;  // Enabled but will be skipped
    
    EntityApiHandler handler(
        storage_,
        secondary_index_,
        graph_index_,
        tx_manager_,
        field_encryption_,
        key_provider_,
        auth_,
        config,
        nullptr,  // spatial_index
        nullptr,  // changefeed
        nullptr,  // wal_manager
        nullptr,  // replication_coordinator
        nullptr,  // multi_primary_coordinator
        nullptr,  // redundancy_manager - missing!
        nullptr,  // hash_ring - missing!
        nullptr   // shard_topology - missing!
    );
    
    // Create a mock HTTP request
    boost::beast::http::request<boost::beast::http::string_body> req;
    req.method(boost::beast::http::verb::put);
    req.target("/entities/users:charlie");
    req.set(boost::beast::http::field::content_type, "application/json");
    req.body() = R"({"key":"users:charlie","blob":"{\"name\":\"Charlie\",\"age\":35}"})";
    req.prepare_payload();
    
    // Execute request
    auto response = handler.handlePut(req);
    
    // Should still succeed (RAID skipped gracefully)
    EXPECT_EQ(response.result(), boost::beast::http::status::created);
    
    // Verify data was written to primary storage
    auto value = storage_->get(KeySchema::makeRelationalKey("users", "charlie"));
    EXPECT_TRUE(value.has_value());
}