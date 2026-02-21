/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_secondary_index.cpp                           ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     202                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>

#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"

using namespace themis;

// ----------------- KeySchema unit tests (existing) -----------------

TEST(KeySchemaTest, MakeRelationalKey) {
    std::string key = KeySchema::makeRelationalKey("users", "123");
    EXPECT_EQ(key, "rel:users:123"); // v1.5.0+ format with prefix
}

TEST(KeySchemaTest, MakeGraphNodeKey) {
    std::string key = KeySchema::makeGraphNodeKey("user/alice");
    EXPECT_EQ(key, "node:user/alice");
}

TEST(KeySchemaTest, MakeGraphEdgeKey) {
    std::string key = KeySchema::makeGraphEdgeKey("edge_1");
    EXPECT_EQ(key, "edge:edge_1");
}

TEST(KeySchemaTest, MakeSecondaryIndexKey) {
    std::string key = KeySchema::makeSecondaryIndexKey("users", "age", "30", "user_123");
    EXPECT_EQ(key, "idx:users:age:30:user_123");
}

TEST(KeySchemaTest, MakeGraphOutdexKey) {
    std::string key = KeySchema::makeGraphOutdexKey("user/alice", "edge_1");
    EXPECT_EQ(key, "graph:out:user/alice:edge_1");
}

TEST(KeySchemaTest, MakeGraphIndexKey) {
    std::string key = KeySchema::makeGraphIndexKey("company/acme", "edge_1");
    EXPECT_EQ(key, "graph:in:company/acme:edge_1");
}

TEST(KeySchemaTest, ExtractPrimaryKey) {
    // v1.5.0+ format with prefixes
    std::string pk = KeySchema::extractPrimaryKey("rel:users:123");
    EXPECT_EQ(pk, "123");
    
    pk = KeySchema::extractPrimaryKey("idx:users:age:30:user_456");
    EXPECT_EQ(pk, "user_456");
}

TEST(KeySchemaTest, ParseKeyType) {
    // v1.5.0+ format tests
    EXPECT_EQ(KeySchema::parseKeyType("rel:users:123"), KeySchema::KeyType::RELATIONAL);
    EXPECT_EQ(KeySchema::parseKeyType("doc:orders:456"), KeySchema::KeyType::DOCUMENT);
    EXPECT_EQ(KeySchema::parseKeyType("idx:users:age:30:pk"), KeySchema::KeyType::SECONDARY_INDEX);
    EXPECT_EQ(KeySchema::parseKeyType("graph:out:alice:e1"), KeySchema::KeyType::GRAPH_OUTDEX);
    EXPECT_EQ(KeySchema::parseKeyType("graph:in:bob:e1"), KeySchema::KeyType::GRAPH_INDEX);
    EXPECT_EQ(KeySchema::parseKeyType("node:alice"), KeySchema::KeyType::GRAPH_NODE);
    EXPECT_EQ(KeySchema::parseKeyType("edge:e1"), KeySchema::KeyType::GRAPH_EDGE);
    
    // Legacy format (pre-1.5.0) - defaults to DOCUMENT for backward compatibility
    EXPECT_EQ(KeySchema::parseKeyType("users:123"), KeySchema::KeyType::DOCUMENT);
}

// ----------------- SecondaryIndex integration tests -----------------

static std::string makeTempDbPath(const std::string& name) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto base = fs::temp_directory_path() / (name + std::to_string(now));
    return base.string();
}

TEST(SecondaryIndexTest, CreatePutScanDelete) {
    // Arrange: RocksDB wrapper
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_secidx_create_put_");
    cfg.enable_blobdb = false; // not needed in tests
    RocksDBWrapper db(cfg);
    
    std::cout << "\n=== TEST DEBUG: DB OPEN ===" << std::endl;
    std::cout << "DB path: " << cfg.db_path << std::endl;
    
    bool opened = db.open();
    ASSERT_TRUE(opened) << "Failed to open RocksDB";
    ASSERT_TRUE(db.isOpen()) << "DB not open after open() returned true";
    
    std::cout << "DB opened successfully, attempting put()..." << std::endl;

    // Test direct put to isolate the problem
    std::vector<uint8_t> test_val{0x01, 0x02};
    bool direct_put_result = db.put("test_key", test_val);
    
    std::cout << "Direct put() result: " << (direct_put_result ? "SUCCESS" : "FAILED") << std::endl;
    
    ASSERT_TRUE(direct_put_result) << "Direct put() to RocksDB failed";
    
    SecondaryIndexManager idx(db);
    auto st = idx.createIndex("users", "age");
    ASSERT_TRUE(st.ok) << "createIndex failed: " << st.message << " (db.isOpen=" << db.isOpen() << ")";

    // Insert entity
    BaseEntity::FieldMap fields1{{"name","Alice"}, {"age", int64_t(30)}, {"city","Berlin"}};
    BaseEntity e1 = BaseEntity::fromFields("u1", fields1);
    st = idx.put("users", e1);
    ASSERT_TRUE(st.ok) << st.message;

    // Scan equals age=30 -> expect u1
    auto [status1, keys] = idx.scanKeysEqual("users", "age", "30");
    ASSERT_TRUE(status1.ok);
    ASSERT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], "u1");

    // Update: change age to 31
    BaseEntity::FieldMap fields2{{"name","Alice"}, {"age", int64_t(31)}, {"city","Berlin"}};
    BaseEntity e2 = BaseEntity::fromFields("u1", fields2);
    st = idx.put("users", e2);
    ASSERT_TRUE(st.ok) << st.message;

    // Old index should be gone
    auto [status2a, keys_old] = idx.scanKeysEqual("users", "age", "30");
    ASSERT_TRUE(status2a.ok);
    EXPECT_TRUE(keys_old.empty());
    // New index
    auto [status2b, keys_new] = idx.scanKeysEqual("users", "age", "31");
    ASSERT_TRUE(status2b.ok);
    ASSERT_EQ(keys_new.size(), 1);
    EXPECT_EQ(keys_new[0], "u1");

    // Delete entity
    st = idx.erase("users", "u1");
    ASSERT_TRUE(st.ok) << st.message;
    auto [status3, keys_post] = idx.scanKeysEqual("users", "age", "31");
    ASSERT_TRUE(status3.ok);
    EXPECT_TRUE(keys_post.empty());

    db.close();
}

TEST(SecondaryIndexTest, EstimateCountAndNoIndex) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = makeTempDbPath("vccdb_secidx_estimate_");
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    SecondaryIndexManager idx(db);

    // No index yet -> scans should error, estimate = 0
    auto [status0, keys0] = idx.scanKeysEqual("users", "age", "30");
    EXPECT_FALSE(status0.ok);
    bool capped=false; 
    EXPECT_EQ(idx.estimateCountEqual("users","age","30", 10, &capped), 0u);
    EXPECT_FALSE(capped);

    // Create index and insert 3 entries with same age
    ASSERT_TRUE(idx.createIndex("users","age").ok);
    for (int i=0; i<3; ++i) {
        BaseEntity::FieldMap f{{"name","N"+std::to_string(i)}, {"age", int64_t(30)}};
        BaseEntity e = BaseEntity::fromFields("u"+std::to_string(i), f);
        ASSERT_TRUE(idx.put("users", e).ok);
    }

    capped = false;
    auto c = idx.estimateCountEqual("users","age","30", 2, &capped);
    EXPECT_EQ(c, 2u);
    EXPECT_TRUE(capped);

    auto [status1, keys] = idx.scanKeysEqual("users","age","30");
    ASSERT_TRUE(status1.ok);
    EXPECT_EQ(keys.size(), 3u);

    db.close();
}
