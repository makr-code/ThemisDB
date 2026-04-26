/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_http_aql_debug.cpp                            ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     83                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Debug test to isolate HTTP AQL data visibility issue
#include <gtest/gtest.h>
#include <filesystem>
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"

using namespace themis;

TEST(HttpAqlDebug, DirectIndexAccess) {
    const std::string db_path = "data/themis_http_aql_debug_test";
    if (std::filesystem::exists(db_path)) {
        std::filesystem::remove_all(db_path);
    }
    
    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    
    SecondaryIndexManager idx(db);
    
    // Create index
    auto st1 = idx.createIndex("users", "city", false);
    ASSERT_TRUE(st1.ok) << st1.message;
    
    // Insert test entities
    BaseEntity alice = BaseEntity::fromFields("alice", {
        {"name", std::string("Alice")},
        {"age", int64_t(25)},
        {"city", std::string("Berlin")}
    });
    
    BaseEntity diana = BaseEntity::fromFields("diana", {
        {"name", std::string("Diana")},
        {"age", int64_t(28)},
        {"city", std::string("Berlin")}
    });
    
    auto st2 = idx.put("users", alice);
    ASSERT_TRUE(st2.ok) << "Failed to insert alice: " << st2.message;
    
    auto st3 = idx.put("users", diana);
    ASSERT_TRUE(st3.ok) << "Failed to insert diana: " << st3.message;
    
    // Now query via index
    auto [status, pks] = idx.scanKeysEqual("users", "city", "Berlin");
    ASSERT_TRUE(status.ok) << "Index scan failed: " << status.message;
    EXPECT_EQ(pks.size(), 2) << "Expected 2 users in Berlin, found " << pks.size();
    
    // Query via QueryEngine
    QueryEngine engine(db, idx);
    ConjunctiveQuery q{"users", {{"city", "Berlin"}}};
    auto result = engine.executeAndKeys(q);
    ASSERT_TRUE(result.has_value()) << "Query failed: " << result.error().message();
    EXPECT_EQ(result->size(), 2) << "QueryEngine returned " << result->size() << " results";
    
    db.close();
    std::filesystem::remove_all(db_path);
}
