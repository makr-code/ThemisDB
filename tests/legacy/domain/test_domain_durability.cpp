// Domain-specific durability tests: Graph, Vector, Geo, Timeseries, LLM
// Tests persistence and recovery of specialized data structures across crashes

#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <cmath>
#include <algorithm>

namespace fs = std::filesystem;

class DomainDurabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = fs::temp_directory_path() / ("themis_domain_durability_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count()));
        fs::create_directories(db_path_);
    }

    void TearDown() override {
        closeDB();
        std::error_code ec = {};
        fs::remove_all(db_path_, ec);
    }

    void openDB() {
        if (db_) {
          return;
        }

        rocksdb::TransactionDBOptions txn_db_opts;
        rocksdb::Options options;
        options.create_if_missing = true;
        options.max_open_files = 256;

        rocksdb::Status s = rocksdb::TransactionDB::Open(
            options, txn_db_opts, db_path_.string(), &db_);
        ASSERT_TRUE(s.ok()) << "Failed to open TransactionDB: " << s.ToString();
    }

    void closeDB() {
        if (db_) {
            delete db_;
            db_ = nullptr;
        }
    }

    fs::path db_path_;
    rocksdb::TransactionDB* db_ = nullptr;
};

// ==================== GRAPH TESTS ====================

TEST_F(DomainDurabilityTest, GraphNodePersistence) {
    openDB();

    // Create graph nodes
    std::vector<std::pair<std::string, nlohmann::json>> nodes;
    for (int i = 0; i < 10; ++i) {
        std::string node_id = "node_" + std::to_string(i);
        nlohmann::json node{
            {"id", node_id},
            {"label", "Node " + std::to_string(i)},
            {"properties", {{"color", "red"}, {"size", i * 10}}}
        };
        nodes.emplace_back(node_id, node);
    }

    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());
    ASSERT_TRUE(txn);

    for (const auto& [id, node_data] : nodes) {
        rocksdb::Status s = txn->Put("graph:node:" + id, node_data.dump());
        ASSERT_TRUE(s.ok());
    }

    rocksdb::Status commit_s = txn->Commit();
    ASSERT_TRUE(commit_s.ok());
    delete txn;

    closeDB();

    // Reopen and verify all nodes
    openDB();
    for (const auto& [id, expected_node] : nodes) {
        std::string value = {};
        rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), "graph:node:" + id, &value);
        ASSERT_TRUE(s.ok()) << "Failed to read node: " << id;

        auto stored_node = nlohmann::json::parse(value);
        EXPECT_EQ(stored_node["id"], expected_node["id"]);
        EXPECT_EQ(stored_node["label"], expected_node["label"]);
    }

    closeDB();
}

TEST_F(DomainDurabilityTest, GraphEdgePersistence) {
    openDB();

    // Create nodes first
    for (int i = 0; i < 5; ++i) {
        nlohmann::json node{{"id", "n" + std::to_string(i)}, {"type", "node"}};
        rocksdb::Transaction* txn = db_->BeginTransaction(
            rocksdb::WriteOptions(), rocksdb::TransactionOptions());
        txn->Put("graph:node:n" + std::to_string(i), node.dump());
        txn->Commit();
        delete txn;
    }

    // Create edges
    std::vector<std::tuple<int, int, std::string>> edges;
    rocksdb::Transaction* edge_txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());

    for (int i = 0; i < 5; ++i) {
        for (int j = i + 1; j < 5; ++j) {
            std::string edge_id = "e_" + std::to_string(i) + "_" + std::to_string(j);
            nlohmann::json edge{
                {"from", "n" + std::to_string(i)},
                {"to", "n" + std::to_string(j)},
                {"weight", static_cast<double>(i + j) / 10.0}
            };
            edge_txn->Put("graph:edge:" + edge_id, edge.dump());
            edges.emplace_back(i, j, edge_id);
        }
    }

    rocksdb::Status s = edge_txn->Commit();
    ASSERT_TRUE(s.ok());
    delete edge_txn;

    closeDB();

    // Verify edges persisted
    openDB();
    for (const auto& [from, to, edge_id] : edges) {
        std::string value = {};
        rocksdb::Status read_s = db_->Get(rocksdb::ReadOptions(), "graph:edge:" + edge_id, &value);
        ASSERT_TRUE(read_s.ok());

        auto edge = nlohmann::json::parse(value);
        EXPECT_EQ(edge["from"], "n" + std::to_string(from));
        EXPECT_EQ(edge["to"], "n" + std::to_string(to));
    }

    closeDB();
}

// ==================== VECTOR TESTS ====================

TEST_F(DomainDurabilityTest, VectorEmbeddingPersistence) {
    openDB();

    // Create vector embeddings (simulated)
    std::vector<std::string> vector_ids;
    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());

    for (int i = 0; i < 20; ++i) {
        std::string vec_id = "vec_" + std::to_string(i);
        vector_ids.push_back(vec_id);

        // Simulate 128-dim embedding
        nlohmann::json embedding = nlohmann::json::array();
        for (int d = 0; d < 128; ++d) {
            embedding.push_back(static_cast<float>(std::sin(i + d)) / 100.0f);
        }

        nlohmann::json doc{
            {"id", vec_id},
            {"embedding", embedding},
            {"metadata", {{"text", "Document " + std::to_string(i)}}}
        };

        txn->Put("vector:embedding:" + vec_id, doc.dump());
    }

    rocksdb::Status s = txn->Commit();
    ASSERT_TRUE(s.ok());
    delete txn;

    closeDB();

    // Verify embeddings
    openDB();
    for (const auto& vec_id : vector_ids) {
        std::string value = {};
        rocksdb::Status read_s = db_->Get(rocksdb::ReadOptions(), "vector:embedding:" + vec_id, &value);
        ASSERT_TRUE(read_s.ok());

        auto doc = nlohmann::json::parse(value);
        ASSERT_TRUE(doc.contains("embedding"));
        EXPECT_EQ(doc["embedding"].size(), 128);
    }

    closeDB();
}

TEST_F(DomainDurabilityTest, VectorIndexMetadata) {
    openDB();

    // Store HNSW index metadata
    nlohmann::json index_meta{
        {"type", "hnsw"},
        {"dimension", 384},
        {"ef_construction", 200},
        {"max_m", 16},
        {"entry_point", 0},
        {"num_nodes", 1000}
    };

    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());
    txn->Put("vector:index_meta", index_meta.dump());
    rocksdb::Status s = txn->Commit();
    ASSERT_TRUE(s.ok());
    delete txn;

    closeDB();

    // Verify metadata
    openDB();
    std::string meta_value = {};
    rocksdb::Status read_s = db_->Get(rocksdb::ReadOptions(), "vector:index_meta", &meta_value);
    ASSERT_TRUE(read_s.ok());

    auto meta = nlohmann::json::parse(meta_value);
    EXPECT_EQ(meta["type"], "hnsw");
    EXPECT_EQ(meta["dimension"], 384);

    closeDB();
}

// ==================== GEOSPATIAL TESTS ====================

TEST_F(DomainDurabilityTest, GeoPointPersistence) {
    openDB();

    // Store geospatial points
    std::vector<std::tuple<double, double, std::string>> points;
    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());

    // Create points in Berlin area
    double base_lat = 52.5200, base_lon = 13.4050;
    for (int i = 0; i < 15; ++i) {
        double lat = base_lat + (i * 0.01);
        double lon = base_lon + (i * 0.01);
        std::string poi_id = "poi_" + std::to_string(i);

        nlohmann::json geo_point{
            {"id", poi_id},
            {"type", "Point"},
            {"coordinates", nlohmann::json::array({lon, lat})},
            {"name", "POI " + std::to_string(i)}
        };

        txn->Put("geo:point:" + poi_id, geo_point.dump());
        points.emplace_back(lat, lon, poi_id);
    }

    rocksdb::Status s = txn->Commit();
    ASSERT_TRUE(s.ok());
    delete txn;

    closeDB();

    // Verify points
    openDB();
    for (const auto& [lat, lon, poi_id] : points) {
        std::string value = {};
        rocksdb::Status read_s = db_->Get(rocksdb::ReadOptions(), "geo:point:" + poi_id, &value);
        ASSERT_TRUE(read_s.ok());

        auto point = nlohmann::json::parse(value);
        EXPECT_EQ(point["type"], "Point");
        EXPECT_EQ(point["coordinates"][1].get<double>(), lat);
        EXPECT_EQ(point["coordinates"][0].get<double>(), lon);
    }

    closeDB();
}

TEST_F(DomainDurabilityTest, GeoPolygonPersistence) {
    openDB();

    // Store polygon (e.g., city boundary)
    nlohmann::json polygon{
        {"id", "berlin_boundary"},
        {"type", "Polygon"},
        {"coordinates", nlohmann::json::array({
            nlohmann::json::array({13.0, 52.0}),
            nlohmann::json::array({14.0, 52.0}),
            nlohmann::json::array({14.0, 53.0}),
            nlohmann::json::array({13.0, 53.0}),
            nlohmann::json::array({13.0, 52.0})  // Close polygon
        })},
        {"area_km2", 891.68}
    };

    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());
    txn->Put("geo:polygon:berlin", polygon.dump());
    rocksdb::Status s = txn->Commit();
    ASSERT_TRUE(s.ok());
    delete txn;

    closeDB();

    // Verify polygon
    openDB();
    std::string value = {};
    rocksdb::Status read_s = db_->Get(rocksdb::ReadOptions(), "geo:polygon:berlin", &value);
    ASSERT_TRUE(read_s.ok());

    auto stored_poly = nlohmann::json::parse(value);
    EXPECT_EQ(stored_poly["type"], "Polygon");
    EXPECT_EQ(stored_poly["coordinates"].size(), 5);

    closeDB();
}

// ==================== TIMESERIES TESTS ====================

TEST_F(DomainDurabilityTest, TimeseriesDatapointPersistence) {
    openDB();

    // Store time-series data points
    auto now = std::chrono::system_clock::now();
    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());

    for (int i = 0; i < 100; ++i) {
        auto timestamp = now - std::chrono::seconds(100 - i);
        int64_t ts_millis = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();

        nlohmann::json datapoint{
            {"timestamp", ts_millis},
            {"metric", "cpu_usage"},
            {"value", 30.0 + (i % 50)},
            {"tags", {{"host", "server1"}, {"region", "eu"}}}
        };

        std::string key = "ts:cpu_usage:server1:" + std::to_string(ts_millis);
        txn->Put(key, datapoint.dump());
    }

    rocksdb::Status s = txn->Commit();
    ASSERT_TRUE(s.ok());
    delete txn;

    closeDB();

    // Verify datapoints exist
    openDB();
    int count = 0;
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    it->Seek("ts:cpu_usage:server1:");
    while (it->Valid() && it->key().starts_with("ts:cpu_usage:server1:")) {
        auto data = nlohmann::json::parse(it->value().ToString());
        EXPECT_EQ(data["metric"], "cpu_usage");
        count++;
        it->Next();
    }
    delete it;

    EXPECT_EQ(count, 100) << "Not all timeseries datapoints found";
    closeDB();
}

TEST_F(DomainDurabilityTest, TimeseriesRetentionPolicy) {
    openDB();

    // Store retention policy
    nlohmann::json retention{
        {"metric", "cpu_usage"},
        {"policies", {
            {
                {"name", "raw"},
                {"retention_days", 7},
                {"aggregation", "none"}
            },
            {
                {"name", "hourly"},
                {"retention_days", 30},
                {"aggregation", "avg"}
            },
            {
                {"name", "daily"},
                {"retention_days", 365},
                {"aggregation", "avg"}
            }
        }}
    };

    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());
    txn->Put("ts:retention_policy:cpu_usage", retention.dump());
    rocksdb::Status s = txn->Commit();
    ASSERT_TRUE(s.ok());
    delete txn;

    closeDB();

    // Verify policy persists
    openDB();
    std::string value = {};
    rocksdb::Status read_s = db_->Get(rocksdb::ReadOptions(), "ts:retention_policy:cpu_usage", &value);
    ASSERT_TRUE(read_s.ok());

    auto policy = nlohmann::json::parse(value);
    EXPECT_EQ(policy["policies"].size(), 3);

    closeDB();
}

// ==================== LLM TESTS ====================

#ifdef THEMIS_ENABLE_LLM

TEST_F(DomainDurabilityTest, LLMModelCachePersistence) {
    openDB();

    // Store LLM model metadata and cache
    nlohmann::json model_meta{
        {"model_name", "gpt-2"},
        {"model_size_mb", 548},
        {"vocab_size", 50257},
        {"max_context", 1024},
        {"quantization", "int8"},
        {"cached_at", 1704198000}
    };

    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());
    txn->Put("llm:model_meta:gpt2", model_meta.dump());

    // Store some cached embeddings
    for (int i = 0; i < 50; ++i) {
        nlohmann::json cached_embedding{
            {"text_hash", "hash_" + std::to_string(i)},
            {"embedding_dim", 768},
            {"cached_value", i * 0.5}
        };
        txn->Put("llm:embedding_cache:" + std::to_string(i), cached_embedding.dump());
    }

    rocksdb::Status s = txn->Commit();
    ASSERT_TRUE(s.ok());
    delete txn;

    closeDB();

    // Verify model and cache
    openDB();
    std::string meta_value = {};
    rocksdb::Status meta_s = db_->Get(rocksdb::ReadOptions(), "llm:model_meta:gpt2", &meta_value);
    ASSERT_TRUE(meta_s.ok());

    auto meta = nlohmann::json::parse(meta_value);
    EXPECT_EQ(meta["model_name"], "gpt-2");
    EXPECT_EQ(meta["vocab_size"], 50257);

    // Verify cache entries
    int cache_count = 0;
    for (int i = 0; i < 50; ++i) {
        std::string cache_value = {};
        rocksdb::Status cache_s = db_->Get(rocksdb::ReadOptions(),
            "llm:embedding_cache:" + std::to_string(i), &cache_value);
        if (cache_s.ok()) {
            cache_count++;
        }
    }

    EXPECT_EQ(cache_count, 50);
    closeDB();
}

TEST_F(DomainDurabilityTest, LLMPromptHistoryPersistence) {
    openDB();

    // Store prompt history for few-shot learning
    rocksdb::Transaction* txn = db_->BeginTransaction(
        rocksdb::WriteOptions(), rocksdb::TransactionOptions());

    for (int i = 0; i < 20; ++i) {
        nlohmann::json prompt_record{
            {"id", "prompt_" + std::to_string(i)},
            {"input", "Example input " + std::to_string(i)},
            {"output", "Example output " + std::to_string(i)},
            {"model", "gpt-2"},
            {"timestamp", 1704198000 + i},
            {"quality_score", 0.85 + (i * 0.01)}
        };

        txn->Put("llm:prompt_history:" + std::to_string(i), prompt_record.dump());
    }

    rocksdb::Status s = txn->Commit();
    ASSERT_TRUE(s.ok());
    delete txn;

    closeDB();

    // Verify prompt history
    openDB();
    int count = 0;
    for (int i = 0; i < 20; ++i) {
        std::string value = {};
        rocksdb::Status read_s = db_->Get(rocksdb::ReadOptions(),
            "llm:prompt_history:" + std::to_string(i), &value);
        if (read_s.ok()) {
            auto record = nlohmann::json::parse(value);
            EXPECT_TRUE(record.contains("quality_score"));
            count++;
        }
    }

    EXPECT_EQ(count, 20);
    closeDB();
}

#endif  // THEMIS_ENABLE_LLM
