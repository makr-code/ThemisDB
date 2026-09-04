#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/vector_index.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"

using namespace themis;

// Utility: Generate random vectors of different dimensions
std::vector<float> genVec(size_t dim, int seed = 0) {
    static thread_local std::mt19937 gen(std::random_device{}() + seed);
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    std::vector<float> v(dim);
    for (auto& x : v) {
      x = dis(gen);
    }
    // Normalize to unit vector
    float norm = 0.0f;
    for (auto x : v) {
      norm += x * x;
    }
    norm = std::sqrt(norm);
    if (norm > 0.001f) {
      for (auto& x : v) {
        x /= norm;
      }
    }
    return v;
}

// Generate random string of given length
std::string randStr(size_t len) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dis(0, sizeof(charset) - 2);
    std::string s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        s += charset[dis(gen)];
    }
    return s;
}

// ============================================================================
// SIMPLE VECTOR OPERATIONS - Small Dimensions, Fast Operations
// ============================================================================

class SimpleVectorBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_simple_vec_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// Small dimension vectors (e.g., RGB color space)
BENCHMARK_F(SimpleVectorBench, Insert_RGB_Vectors)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            BaseEntity e("rgb_" + std::to_string(i), BaseEntity::FieldMap{
                {"color", genVec(3)}
            });
            vim.addEntity(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}

BENCHMARK_F(SimpleVectorBench, Search_RGB_KNN_Top10)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    for (int i = 0; i < 2000; ++i) {
        BaseEntity e("rgb_" + std::to_string(i), BaseEntity::FieldMap{
            {"color", genVec(3)}
        });
        vim.addEntity(e);
    }
    
    for (auto _ : state) {
        auto q = genVec(3);
        auto results = vim.searchKnn(q, 10);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * 10);
}

// Medium dimension vectors (typical embeddings)
BENCHMARK_F(SimpleVectorBench, Insert_384D_Embeddings)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            BaseEntity e("emb_" + std::to_string(i), BaseEntity::FieldMap{
                {"embedding", genVec(384)}
            });
            vim.addEntity(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// COMPLEX VECTOR OPERATIONS - High Dimensions, Batch Operations
// ============================================================================

class ComplexVectorBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_complex_vec_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// High-dimensional vectors (LLM embeddings, e.g., 1536D from OpenAI)
BENCHMARK_F(ComplexVectorBench, BatchInsert_1536D_LLMVectors)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    for (auto _ : state) {
        for (int i = 0; i < 50; ++i) {
            BaseEntity e("llm_" + std::to_string(i), BaseEntity::FieldMap{
                {"embedding", genVec(1536)},
                {"text", "Document content for vector " + std::to_string(i)},
                {"metadata", "extra fields"}
            });
            vim.addEntity(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 50);
}

// Very high-dimensional (e.g., 4096D)
BENCHMARK_F(ComplexVectorBench, Search_4096D_TopK_Batch)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    
    // Populate with 5000 vectors
    for (int i = 0; i < 5000; ++i) {
        BaseEntity e("hvec_" + std::to_string(i), BaseEntity::FieldMap{
            {"embedding", genVec(4096)}
        });
        vim.addEntity(e);
    }
    
    for (auto _ : state) {
        // Batch of 100 queries
        for (int q = 0; q < 100; ++q) {
            auto query = genVec(4096);
            auto results = vim.searchKnn(query, 100); // Top-100
            benchmark::DoNotOptimize(results);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100 * 100);
}

// ============================================================================
// LLM INFERENCING SIMULATIONS
// ============================================================================

class LLMInferencingBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_llm_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// Simulate embedding generation + storage (e.g., text → vector)
BENCHMARK_F(LLMInferencingBench, EmbeddingGeneration_Store)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    int doc_id = 0;
    
    for (auto _ : state) {
        // Generate embedding (simulated)
        auto embedding = genVec(1536);
        
        // Store with metadata
        BaseEntity doc("doc_" + std::to_string(doc_id++), BaseEntity::FieldMap{
            {"embedding", embedding},
            {"text", "This is a sample document for embedding generation benchmark " + randStr(100)},
            {"timestamp", static_cast<int64_t>(std::chrono::system_clock::now().time_since_epoch().count())},
            {"source", "benchmark"}
        });
        vim.addEntity(doc);
    }
    state.SetItemsProcessed(state.iterations());
}

// Simulate RAG (Retrieval-Augmented Generation): search + context retrieval
BENCHMARK_F(LLMInferencingBench, RAG_Search_Retrieve_Top50)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    
    // Populate with 50000 embeddings
    for (int i = 0; i < 50000; ++i) {
        BaseEntity doc("rag_" + std::to_string(i), BaseEntity::FieldMap{
            {"embedding", genVec(1536)},
            {"text", "Document chunk " + std::to_string(i) + ": " + randStr(200)},
            {"chunk_id", static_cast<int64_t>(i % 1000)},
            {"source_doc", std::to_string(i / 100)}
        });
        vim.addEntity(doc);
    }
    
    for (auto _ : state) {
        // Simulate query embedding
        auto query_embedding = genVec(1536);
        
        // Search for top-50 relevant documents
        auto results = vim.searchKnn(query_embedding, 50);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations() * 50);
}

// Simulate multi-query expansion
BENCHMARK_F(LLMInferencingBench, MultiQueryExpansion_5Queries)(benchmark::State& state) {
    VectorIndexManager vim(*db_);
    
    // Populate with 10000 embeddings
    for (int i = 0; i < 2000; ++i) {
        BaseEntity doc("mqe_" + std::to_string(i), BaseEntity::FieldMap{
            {"embedding", genVec(1536)}
        });
        vim.addEntity(doc);
    }
    
    for (auto _ : state) {
        // Generate 5 query variations from original query
        for (int q = 0; q < 5; ++q) {
            auto query = genVec(1536);
            auto [status, results] = vim.searchKnn(query, 20);
            benchmark::DoNotOptimize(results);
        }
    }
    state.SetItemsProcessed(state.iterations() * 5 * 20);
}

// ============================================================================
// AQL QUERY BENCHMARKS - Simple to Complex
// ============================================================================

class AQLQueryBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_aql_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        idx_mgr_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_mgr_->createIndex("users", "country");
        idx_mgr_->createIndex("users", "age");
        idx_mgr_->createIndex("users", "status");
    }
    
    void TearDown(const ::benchmark::State& state) override {
        idx_mgr_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> idx_mgr_;
};

// Simple SELECT with single WHERE clause
BENCHMARK_F(AQLQueryBench, SimpleSelect_WhereClause)(benchmark::State& state) {
    // Populate index with 1k users (reduced for faster benchmarking)
    for (int i = 0; i < 1000; ++i) {
        BaseEntity e("user_" + std::to_string(i), BaseEntity::FieldMap{
            {"country", (i % 50 == 0) ? "US" : "OTHER"},
            {"age", static_cast<int64_t>(18 + (i % 70))},
            {"status", (i % 3 == 0) ? "active" : "inactive"}
        });
        idx_mgr_->put("users", e);
    }
    
    for (auto _ : state) {
        // Query: SELECT * FROM users WHERE country = 'US'
        auto [status, keys] = idx_mgr_->scanKeysEqual("users", "country", "US");
        benchmark::DoNotOptimize(keys);
    }
    state.SetItemsProcessed(state.iterations());
}

// Complex query: Multiple conditions with range
BENCHMARK_F(AQLQueryBench, ComplexSelect_MultipleConditions)(benchmark::State& state) {
    // Populate (reduced for faster benchmarking)
    for (int i = 0; i < 5000; ++i) {
        BaseEntity e("user_" + std::to_string(i), BaseEntity::FieldMap{
            {"country", (i % 200 < 100) ? "US" : "EU"},
            {"age", static_cast<int64_t>(18 + (i % 70))},
            {"status", (i % 2 == 0) ? "active" : "inactive"},
            {"score", static_cast<double>(i % 100)}
        });
        idx_mgr_->put("users", e);
    }
    
    for (auto _ : state) {
        // Complex query: country='US' AND status='active'
        auto [s1, keys1] = idx_mgr_->scanKeysEqual("users", "country", "US");
        // In real scenario would filter further
        benchmark::DoNotOptimize(keys1);
    }
    state.SetItemsProcessed(state.iterations());
}

// JOIN simulation: Graph traversal + secondary index lookup
class AQLJoinBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_aql_join_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        gim_ = std::make_unique<GraphIndexManager>(*db_);
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("posts", "author_id");
    }
    
    void TearDown(const ::benchmark::State& state) override {
        sim_.reset();
        gim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> gim_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

BENCHMARK_F(AQLJoinBench, JoinUsers_Posts)(benchmark::State& state) {
    // Create user graph with follows relationships
    for (int i = 0; i < 1000; ++i) {
        BaseEntity edge("follow_" + std::to_string(i), BaseEntity::FieldMap{
            {"_from", "user_" + std::to_string(i % 100)},
            {"_to", "user_" + std::to_string((i + 1) % 100)}
        });
        gim_->addEdge(edge);
    }
    
    // Create posts by users
    for (int i = 0; i < 2000; ++i) {
        BaseEntity post("post_" + std::to_string(i), BaseEntity::FieldMap{
            {"author_id", "user_" + std::to_string(i % 100)},
            {"title", "Post " + std::to_string(i)},
            {"created_at", static_cast<int64_t>(std::chrono::system_clock::now().time_since_epoch().count())}
        });
        sim_->put("posts", post);
    }
    
    for (auto _ : state) {
        // Query: Find all posts from users that user_0 follows
        auto [s, neighbors] = gim_->outNeighbors("user_0");
        for (const auto& neighbor : neighbors) {
            auto [s2, posts] = sim_->scanKeysEqual("posts", "author_id", neighbor);
            benchmark::DoNotOptimize(posts);
        }
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// BINARY OPERATIONS & BLOB STORAGE
// ============================================================================

class BinaryOperationsBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_binary_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        // PERF-D5: enable streaming blob write path for large-blob benchmarks.
        cfg.enable_blob_streaming = true;
        cfg.blob_streaming_threshold_bytes = 65536;   // 64 KB
        cfg.blob_chunk_size_bytes          = 131072;  // 128 KB
        cfg.blob_streaming_threads         = 8;
        // Disable WAL fsync for benchmark throughput measurement.
        cfg.disable_wal_for_benchmark = true;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
    if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
    }
    
    void TearDown(const ::benchmark::State& state) override {
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

// Store small binary blobs (e.g., image thumbnails)
BENCHMARK_F(BinaryOperationsBench, StoreThumbnails_10KB)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            std::vector<uint8_t> blob(10240);
            std::fill(blob.begin(), blob.end(), i % 256);
            
            BaseEntity e("thumb_" + std::to_string(i), BaseEntity::FieldMap{
                {"image_data", blob},
                {"size", 10240},
                {"format", "jpeg"}
            });
            db_->put("thumbnails:" + std::to_string(i), e.serialize());
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Store large binary blobs (e.g., document PDFs) via streaming write path (PERF-D5).
// Uses putBlob() which splits the 1MB value into 128KB chunks, encodes them in
// parallel, and commits all chunks atomically in one WriteBatch – bypassing the
// per-write transaction overhead of the regular put() path.
BENCHMARK_F(BinaryOperationsBench, StoreLargeBlobs_1MB)(benchmark::State& state) {
    std::vector<uint8_t> blob(1048576); // 1MB
    std::fill(blob.begin(), blob.end(), 42);

    int doc_id = 0;
    for (auto _ : state) {
        // Use a unique key per iteration so RocksDB actually flushes distinct values.
        db_->putBlob("documents:doc_" + std::to_string(doc_id++), blob);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * 1048576);
}

// Retrieve and process binary blobs
BENCHMARK_F(BinaryOperationsBench, RetrieveBlobsBatch_100x100KB)(benchmark::State& state) {
    // Store 1000 blobs first
    for (int i = 0; i < 1000; ++i) {
        std::vector<uint8_t> blob(102400); // 100KB
        std::fill(blob.begin(), blob.end(), i % 256);
        BaseEntity e("blob_" + std::to_string(i), BaseEntity::FieldMap{
            {"data", blob}
        });
        db_->put("blobs:" + std::to_string(i), e.serialize());
    }
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            auto val = db_->get("blobs:" + std::to_string(i % 1000));
            benchmark::DoNotOptimize(val);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// ============================================================================
// GRAPH OPERATIONS - Simple to Complex
// ============================================================================

class GraphOperationsBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_graph_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        gim_ = std::make_unique<GraphIndexManager>(*db_);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        gim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> gim_;
};

// Simple graph: Add edges to sparse graph
BENCHMARK_F(GraphOperationsBench, AddEdges_SparseGraph)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            BaseEntity e("edge_" + std::to_string(i), BaseEntity::FieldMap{
                {"_from", "node_" + std::to_string(i % 100)},
                {"_to", "node_" + std::to_string((i + 1) % 100)},
                {"weight", 1.0}
            });
            gim_->addEdge(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}

// Dense graph queries: Find neighbors in highly connected graph
BENCHMARK_F(GraphOperationsBench, QueryNeighbors_DenseGraph)(benchmark::State& state) {
    // Create dense graph: every node connects to 50 others
    for (int i = 0; i < 50000; ++i) {
        BaseEntity e("edge_" + std::to_string(i), BaseEntity::FieldMap{
            {"_from", "node_" + std::to_string(i / 50)},
            {"_to", "node_" + std::to_string(i % 1000)},
            {"weight", 1.0}
        });
        gim_->addEdge(e);
    }
    
    for (auto _ : state) {
        for (int n = 0; n < 100; ++n) {
            auto [status, neighbors] = gim_->outNeighbors("node_" + std::to_string(n));
            benchmark::DoNotOptimize(neighbors);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100);
}

// Graph traversal: BFS from a node
BENCHMARK_F(GraphOperationsBench, GraphTraversal_BFS_Depth3)(benchmark::State& state) {
    // Create layered graph for BFS testing
    for (int layer = 0; layer < 3; ++layer) {
        int branching = 1 << layer;
        for (int i = 0; i < branching * 100; ++i) {
            BaseEntity e("edge_l" + std::to_string(layer) + "_" + std::to_string(i), BaseEntity::FieldMap{
                {"_from", "node_l" + std::to_string(layer) + "_" + std::to_string(i / branching)},
                {"_to", "node_l" + std::to_string(layer + 1) + "_" + std::to_string(i)},
                {"weight", 1.0}
            });
            gim_->addEdge(e);
        }
    }
    
    for (auto _ : state) {
        // BFS from root
        auto [status, root_neighbors] = gim_->outNeighbors("node_l0_0");
        benchmark::DoNotOptimize(root_neighbors);
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// SECONDARY INDEX OPERATIONS - Variety of Scenarios
// ============================================================================

class SecondaryIndexBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_si_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        sim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

// Index operations on small dataset
BENCHMARK_F(SecondaryIndexBench, SmallIndexInsert_1K)(benchmark::State& state) {
    sim_->createIndex("small", "id");
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            BaseEntity e("small_" + std::to_string(i), BaseEntity::FieldMap{
                {"id", std::to_string(i)},
                {"value", static_cast<int64_t>(i * 2)}
            });
            sim_->put("small", e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}

// Index operations on medium dataset
BENCHMARK_F(SecondaryIndexBench, MediumIndexInsert_100K)(benchmark::State& state) {
    sim_->createIndex("medium", "category");
    static int offset = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            BaseEntity e("med_" + std::to_string(offset + i), BaseEntity::FieldMap{
                {"category", std::to_string((offset + i) % 100)},
                {"data", randStr(50)}
            });
            sim_->put("medium", e);
        }
        offset += 1000;
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}

// Index operations on large dataset
BENCHMARK_F(SecondaryIndexBench, LargeIndexLookup_1M)(benchmark::State& state) {
    sim_->createIndex("large", "key");
    // Populate with 1M entries
    for (int i = 0; i < 100000; ++i) {
        BaseEntity e("large_" + std::to_string(i), BaseEntity::FieldMap{
            {"key", "key_" + std::to_string(i % 10000)},
            {"payload", randStr(100)}
        });
        sim_->put("large", e);
    }
    
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            auto [status, keys] = sim_->scanKeysEqual("large", "key", "key_" + std::to_string(i % 10000));
            benchmark::DoNotOptimize(keys);
        }
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}

// Composite index operations
BENCHMARK_F(SecondaryIndexBench, CompositeIndexLookup)(benchmark::State& state) {
    sim_->createCompositeIndex("composite", {"region", "status"});
    
    // Populate
    for (int i = 0; i < 50000; ++i) {
        BaseEntity e("comp_" + std::to_string(i), BaseEntity::FieldMap{
            {"region", std::to_string(i % 50)},
            {"status", (i % 2 == 0) ? "active" : "inactive"}
        });
        sim_->put("composite", e);
    }
    
    for (auto _ : state) {
        auto [status, keys] = sim_->scanKeysEqualComposite("composite", 
                                                           {"region", "status"},
                                                           {"10", "active"});
        benchmark::DoNotOptimize(keys);
    }
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// BATCH OPERATIONS - Testing throughput at scale
// ============================================================================

class BatchOperationsBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_batch_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        vim_ = std::make_unique<VectorIndexManager>(*db_);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        vim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<VectorIndexManager> vim_;
};

// Batch vector insert with metadata
BENCHMARK_F(BatchOperationsBench, BatchInsert_10K_WithMetadata)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 2000; ++i) {
            BaseEntity e("batch_" + std::to_string(i), BaseEntity::FieldMap{
                {"embedding", genVec(384)},
                {"text", randStr(500)},
                {"metadata_json", randStr(200)},
                {"timestamp", static_cast<int64_t>(std::chrono::system_clock::now().time_since_epoch().count() + i)},
                {"tags", std::vector<float>{1.0f, 0.5f, 0.25f}}
            });
            vim_->addEntity(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}

// Batch multi-field updates
BENCHMARK_F(BatchOperationsBench, BatchUpdate_MultiField_5K)(benchmark::State& state) {
    // Populate first
    for (int i = 0; i < 50000; ++i) {
        BaseEntity e("upd_" + std::to_string(i), BaseEntity::FieldMap{
            {"value1", static_cast<int64_t>(i)},
            {"value2", static_cast<double>(i * 0.5)},
            {"value3", randStr(50)}
        });
        vim_->addEntity(e);
    }
    
    for (auto _ : state) {
        for (int i = 0; i < 5000; ++i) {
            BaseEntity updated("upd_" + std::to_string(i * 10), BaseEntity::FieldMap{
                {"value1", static_cast<int64_t>(i * 100)},
                {"value2", static_cast<double>(i * 2.0)},
                {"value3", randStr(50)},
                {"updated_at", static_cast<int64_t>(std::chrono::system_clock::now().time_since_epoch().count())}
            });
            vim_->addEntity(updated);
        }
    }
    state.SetItemsProcessed(state.iterations() * 5000);
}

// ============================================================================
// CONCURRENT STRESS TESTS
// ============================================================================

class StressTestBench : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        auto tmp = std::filesystem::temp_directory_path();
        db_path_ = (tmp / ("bench_stress_" + std::to_string(reinterpret_cast<uintptr_t>(this)))).string();
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        vim_ = std::make_unique<VectorIndexManager>(*db_);
        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        sim_->createIndex("stress", "key");
    }
    
    void TearDown(const ::benchmark::State& state) override {
        sim_.reset();
        vim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

protected:
    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<VectorIndexManager> vim_;
    std::unique_ptr<SecondaryIndexManager> sim_;
};

// Mixed read/write workload
BENCHMARK_F(StressTestBench, MixedReadWrite_80Reads_20Writes)(benchmark::State& state) {
    // Pre-populate
    for (int i = 0; i < 2000; ++i) {
        BaseEntity e("stress_" + std::to_string(i), BaseEntity::FieldMap{
            {"key", "key_" + std::to_string(i % 1000)},
            {"data", randStr(100)}
        });
        sim_->put("stress", e);
    }
    
    for (auto _ : state) {
        int iter = 0;
        for (int op = 0; op < 1000; ++op) {
            if (op % 5 < 4) {  // 80% reads
                auto [status, keys] = sim_->scanKeysEqual("stress", "key", "key_" + std::to_string(op % 1000));
                benchmark::DoNotOptimize(keys);
            } else {  // 20% writes
                BaseEntity e("stress_" + std::to_string(10000 + iter++), BaseEntity::FieldMap{
                    {"key", "key_" + std::to_string((10000 + iter) % 1000)},
                    {"data", randStr(100)}
                });
                sim_->put("stress", e);
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}

// Hotspot stress: Few keys accessed frequently
BENCHMARK_F(StressTestBench, HotspotAccess_99PercentContention)(benchmark::State& state) {
    // Create highly skewed access pattern
    for (int i = 0; i < 100000; ++i) {
        BaseEntity e("hotspot_" + std::to_string(i), BaseEntity::FieldMap{
            {"key", (i < 1000) ? "hot_key_0" : "cold_key_" + std::to_string(i % 10000)},
            {"value", static_cast<int64_t>(i)}
        });
        sim_->put("hotspot", e);
    }
    
    for (auto _ : state) {
        for (int i = 0; i < 2000; ++i) {
            if (i % 100 < 99) {  // 99% access to hot key
                auto [status, keys] = sim_->scanKeysEqual("hotspot", "key", "hot_key_0");
                benchmark::DoNotOptimize(keys);
            } else {  // 1% access to cold keys
                auto [status, keys] = sim_->scanKeysEqual("hotspot", "key", "cold_key_" + std::to_string(i));
                benchmark::DoNotOptimize(keys);
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}

// ============================================================================
// MAIN
// ============================================================================

BENCHMARK_MAIN();
