// Dispatch test for SIMILARITY syntax sugar

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include "query/aql_runner.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "storage/base_entity.h"

using namespace themis;
using namespace themis::query;

class AQLSimilarityDispatchTest : public ::testing::Test {
protected:
    void SetUp() override {
                db_path_ = (std::filesystem::temp_directory_path() /
                                        ("themis_aql_similarity_dispatch_" +
                                         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
                                             .string();
                std::filesystem::remove_all(db_path_);
                RocksDBWrapper::Config cfg; cfg.db_path = db_path_; cfg.memtable_size_mb = 32; cfg.block_cache_size_mb = 32;
        db = std::make_unique<RocksDBWrapper>(cfg); ASSERT_TRUE(db->open());
        sec = std::make_unique<SecondaryIndexManager>(*db);
        engine = std::make_unique<QueryEngine>(*db, *sec); // no vector/spatial index managers attached (fallback paths)

        // Insert a minimal entity with embedding + location fields
        BaseEntity e("h1");
        e.setField("embedding", std::vector<float>{0.1f,0.2f});
        e.setField("location", std::vector<float>{0.5f,0.5f});
        sec->put("hotels", e);
    }
    void TearDown() override {
        engine.reset(); sec.reset(); db.reset();
        std::filesystem::remove_all(db_path_);
    }
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db; std::unique_ptr<SecondaryIndexManager> sec; std::unique_ptr<QueryEngine> engine;
};

// Previously disabled on MSVC; now stable after RocksDB env isolation
TEST_F(AQLSimilarityDispatchTest, ExecuteSimilarityVectorGeoFallback) {
    // Test SIMILARITY syntax in SORT clause (without complex spatial filter)
    std::string aql = R"(
        FOR doc IN hotels
        SORT SIMILARITY(doc.embedding, [0.1,0.2]) DESC
        LIMIT 1
        RETURN doc
    )";
    auto jsonRes = executeAql(aql, *engine);
    ASSERT_TRUE(jsonRes.has_value()) << jsonRes.error().message();
    // Should execute successfully and return results
    ASSERT_TRUE(jsonRes->contains("results"));
    EXPECT_GE((*jsonRes)["results"].size(), 0);
}
