// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include "metadata/distributed_catalog.h"
#include "metadata/schema_manager.h"
#include "sharding/metadata_shard.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themis;
using namespace themisdb::sharding;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string makeTempDbPath(const std::string& name) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (name + std::to_string(now))).string();
}

// Build a 3-shard MetadataShardRouter backed by in-memory MetadataShards.
static std::pair<
    std::unique_ptr<MetadataShardRouter>,
    std::vector<std::shared_ptr<MetadataShard>>>
makeRouter(size_t num_shards = 3)
{
    auto router = std::make_unique<MetadataShardRouter>(num_shards);
    std::vector<std::shared_ptr<MetadataShard>> shards;

    for (size_t i = 0; i < num_shards; ++i) {
        MetadataShardConfig cfg;
        cfg.shard_id         = "shard_" + std::to_string(i);
        cfg.partitions       = {MetadataPartitionKey::SCHEMA};
        cfg.enable_cache     = true;
        cfg.cache_size       = 512;
        cfg.enforce_strong_consistency = false;

        auto shard = std::make_shared<MetadataShard>(cfg, nullptr);
        shard->initialize();
        shard->start();

        router->addShard(cfg.shard_id, shard);
        shards.push_back(std::move(shard));
    }

    return {std::move(router), std::move(shards)};
}

// Build a minimal TableSchema for testing.
static SchemaManager::TableSchema makeSchema(const std::string& name,
                                             const std::string& type = "relational")
{
    SchemaManager::TableSchema s;
    s.name = name;
    s.type = type;
    s.estimated_row_count = 42;

    SchemaManager::PropertyInfo col;
    col.name       = "id";
    col.type       = "integer";
    col.indexed    = true;
    col.nullable   = false;
    col.index_type = "regular";
    s.properties.push_back(col);

    return s;
}

// ---------------------------------------------------------------------------
// DistributedMetadataCatalogTest
// ---------------------------------------------------------------------------

class DistributedMetadataCatalogTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto [r, s] = makeRouter();
        router_  = std::move(r);
        shards_  = std::move(s);
        catalog_ = std::make_unique<DistributedMetadataCatalog>(*router_);
    }

    void TearDown() override {
        catalog_.reset();
        for (auto& shard : shards_) {
            shard->stop();
        }
    }

    std::unique_ptr<MetadataShardRouter>            router_;
    std::vector<std::shared_ptr<MetadataShard>>     shards_;
    std::unique_ptr<DistributedMetadataCatalog>     catalog_;
};

// ---------------------------------------------------------------------------
// Basic write / read round-trip
// ---------------------------------------------------------------------------

TEST_F(DistributedMetadataCatalogTest, PublishAndFetch) {
    auto schema = makeSchema("users");
    EXPECT_TRUE(catalog_->publishSchema(schema));

    auto fetched = catalog_->fetchSchema("users");
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->name, "users");
    EXPECT_EQ(fetched->type, "relational");
    ASSERT_FALSE(fetched->properties.empty());
    EXPECT_EQ(fetched->properties[0].name, "id");
}

TEST_F(DistributedMetadataCatalogTest, FetchNonExistent) {
    auto fetched = catalog_->fetchSchema("does_not_exist");
    EXPECT_FALSE(fetched.has_value());
}

TEST_F(DistributedMetadataCatalogTest, PublishEmptyNameFails) {
    SchemaManager::TableSchema empty;  // name is ""
    EXPECT_FALSE(catalog_->publishSchema(empty));
}

// ---------------------------------------------------------------------------
// Update (re-publish overwrites)
// ---------------------------------------------------------------------------

TEST_F(DistributedMetadataCatalogTest, PublishOverwriteSchema) {
    auto schema_v1 = makeSchema("orders", "relational");
    auto schema_v2 = makeSchema("orders", "document");  // same name, different type

    EXPECT_TRUE(catalog_->publishSchema(schema_v1));
    EXPECT_TRUE(catalog_->publishSchema(schema_v2));

    auto fetched = catalog_->fetchSchema("orders");
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->type, "document");
}

// ---------------------------------------------------------------------------
// Remove
// ---------------------------------------------------------------------------

TEST_F(DistributedMetadataCatalogTest, RemoveSchema) {
    EXPECT_TRUE(catalog_->publishSchema(makeSchema("sessions")));
    EXPECT_TRUE(catalog_->fetchSchema("sessions").has_value());

    EXPECT_TRUE(catalog_->removeSchema("sessions"));
    EXPECT_FALSE(catalog_->fetchSchema("sessions").has_value());
}

TEST_F(DistributedMetadataCatalogTest, RemoveNonExistentReturnsFalse) {
    EXPECT_FALSE(catalog_->removeSchema("phantom_table"));
}

// ---------------------------------------------------------------------------
// ListTableNames (scatter-gather across shards)
// ---------------------------------------------------------------------------

TEST_F(DistributedMetadataCatalogTest, ListTableNamesEmpty) {
    auto names = catalog_->listTableNames();
    EXPECT_TRUE(names.empty());
}

TEST_F(DistributedMetadataCatalogTest, ListTableNamesAfterPublish) {
    catalog_->publishSchema(makeSchema("alpha"));
    catalog_->publishSchema(makeSchema("beta"));
    catalog_->publishSchema(makeSchema("gamma"));

    auto names = catalog_->listTableNames();
    EXPECT_EQ(names.size(), 3u);

    // listTableNames returns a sorted vector
    EXPECT_TRUE(std::is_sorted(names.begin(), names.end()));
    EXPECT_NE(std::find(names.begin(), names.end(), "alpha"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "beta"),  names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "gamma"), names.end());
}

TEST_F(DistributedMetadataCatalogTest, ListTableNamesAfterRemove) {
    catalog_->publishSchema(makeSchema("t1"));
    catalog_->publishSchema(makeSchema("t2"));
    catalog_->removeSchema("t1");

    auto names = catalog_->listTableNames();
    EXPECT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "t2");
}

// ---------------------------------------------------------------------------
// SyncFromSchemaManager
// ---------------------------------------------------------------------------

class DistributedCatalogSyncTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up a real RocksDB-backed SchemaManager
        RocksDBWrapper::Config cfg;
        cfg.db_path      = makeTempDbPath("test_dist_catalog_");
        cfg.enable_blobdb = false;

        db_        = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        index_mgr_ = std::make_unique<SecondaryIndexManager>(*db_);

        // Inject two custom schemas into the SchemaManager
        schema_mgr_ = std::make_unique<SchemaManager>(*db_, index_mgr_.get());
        schema_mgr_->setTableSchema("products", makeSchema("products"));
        schema_mgr_->setTableSchema("reviews",  makeSchema("reviews", "document"));

        // Set up router and catalog
        auto [r, s] = makeRouter();
        router_ = std::move(r);
        shards_ = std::move(s);
        catalog_ = std::make_unique<DistributedMetadataCatalog>(*router_);
    }

    void TearDown() override {
        catalog_.reset();
        schema_mgr_.reset();
        index_mgr_.reset();
        if (db_) db_->close();
        for (auto& shard : shards_) shard->stop();
    }

    std::unique_ptr<RocksDBWrapper>               db_;
    std::unique_ptr<SecondaryIndexManager>        index_mgr_;
    std::unique_ptr<SchemaManager>                schema_mgr_;
    std::unique_ptr<MetadataShardRouter>          router_;
    std::vector<std::shared_ptr<MetadataShard>>   shards_;
    std::unique_ptr<DistributedMetadataCatalog>   catalog_;
};

TEST_F(DistributedCatalogSyncTest, SyncPublishesAllSchemas) {
    size_t synced = catalog_->syncFromSchemaManager(*schema_mgr_);
    EXPECT_EQ(synced, 2u);  // exactly "products" and "reviews"

    auto names = catalog_->listTableNames();
    EXPECT_NE(std::find(names.begin(), names.end(), "products"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "reviews"),  names.end());
}

TEST_F(DistributedCatalogSyncTest, SyncedSchemaRoundTrips) {
    catalog_->syncFromSchemaManager(*schema_mgr_);

    auto fetched = catalog_->fetchSchema("reviews");
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->name, "reviews");
    EXPECT_EQ(fetched->type, "document");
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST_F(DistributedMetadataCatalogTest, Statistics) {
    catalog_->publishSchema(makeSchema("s1"));
    catalog_->publishSchema(makeSchema("s2"));
    catalog_->fetchSchema("s1");
    catalog_->fetchSchema("s1");
    catalog_->removeSchema("s2");

    auto stats = catalog_->getStatistics();
    EXPECT_EQ(stats["publish_count"], 2);
    EXPECT_EQ(stats["fetch_count"],   2);
    EXPECT_EQ(stats["remove_count"],  1);
    EXPECT_EQ(stats["sync_count"],    0);
}

// ---------------------------------------------------------------------------
// Thread safety
// ---------------------------------------------------------------------------

TEST_F(DistributedMetadataCatalogTest, ThreadSafety) {
    const int num_threads = 8;
    const int ops_per_thread = 20;

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                std::string name = "table_" + std::to_string(t) + "_" + std::to_string(i);
                auto schema = makeSchema(name);
                catalog_->publishSchema(schema);
                catalog_->fetchSchema(name);
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    auto stats = catalog_->getStatistics();
    EXPECT_EQ(stats["publish_count"], num_threads * ops_per_thread);
    EXPECT_EQ(stats["fetch_count"],   num_threads * ops_per_thread);
}
