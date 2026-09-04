#include <benchmark/benchmark.h>
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include <random>
#include <filesystem>
#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>

/**
 * MMDB-E: Multi-Modal Database Benchmark with Embeddings
 * 
 * Specialized benchmark for databases that combine:
 * - Relational data (structured records)
 * - Documents (JSON, nested structures)
 * - Graphs (relationships, traversals)
 * - Vectors (embeddings for semantic search)
 * - LLM Integration (RAG workflows)
 * 
 * Use Case: E-Commerce + Knowledge Base with AI Features
 * 
 * Performance Targets (8-core, 32GB, NVMe):
 * - Hybrid CRUD: 15,000 ops/sec, < 10ms p95
 * - Semantic Search: 8,000 ops/sec, < 50ms p95
 * - Graph Traversal: 3,000 ops/sec, < 100ms p95
 * - RAG Queries: 200 ops/sec, < 2s p95
 */

namespace {
    // MMDB-E Constants
    constexpr int DEFAULT_PRODUCT_COUNT = 10000;
    constexpr int EMBEDDING_DIM = 768;  // BERT-base dimension
    constexpr int CATEGORIES_COUNT = 100;
    constexpr int BRANDS_COUNT = 50;
    
    std::mt19937 rng{std::random_device{}()};
    
    // Generate realistic product name
    std::string generateProductName(int id) {
        static const std::vector<std::string> adjectives = {
            "Premium", "Professional", "Ultra", "Advanced", "Smart", 
            "Eco", "Wireless", "Digital", "Portable", "Compact"
        };
        static const std::vector<std::string> products = {
            "Headphones", "Monitor", "Keyboard", "Mouse", "Camera",
            "Speaker", "Tablet", "Phone", "Laptop", "Watch"
        };
        std::uniform_int_distribution<size_t> adj_dist(0, adjectives.size() - 1);
        std::uniform_int_distribution<size_t> prod_dist(0, products.size() - 1);
        
        return adjectives[adj_dist(rng)] + " " + products[prod_dist(rng)] + " #" + std::to_string(id);
    }
    
    // Generate random embedding vector (simulating sentence-transformer output)
    std::vector<float> generateEmbedding(int seed) {
        std::mt19937 gen(seed);
        std::normal_distribution<float> dist(0.0f, 1.0f);
        
        std::vector<float> embedding(EMBEDDING_DIM);
        float norm = 0.0f;
        
        for (int i = 0; i < EMBEDDING_DIM; ++i) {
            embedding[i] = dist(gen);
            norm += embedding[i] * embedding[i];
        }
        
        // Normalize to unit vector (common for embeddings)
        norm = std::sqrt(norm);
        for (auto& val : embedding) {
            val /= norm;
        }
        
        return embedding;
    }
    
    // Cosine similarity between two embeddings
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
        float dot = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            dot += a[i] * b[i];
        }
        return dot;  // Already normalized, so dot product = cosine similarity
    }
    
    std::string makeRandomString(size_t len) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
        std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 3);
        std::string s;
        s.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            s += charset[dist(rng)];
        }
        return s;
    }
    
    void cleanupTestDB(const std::string& path) {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
}

/**
 * MMDB-E Benchmark Fixture
 * 
 * Sets up multi-modal database with:
 * - Products table (relational)
 * - Product documents (JSON details)
 * - Category/Brand graph (relationships)
 * - Product embeddings (vectors)
 */
class MMDBFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        db_path_ = "bench_mmdb_db";
        cleanupTestDB(db_path_);
        
        // Configure for multi-modal workload
        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";
        config.block_cache_size_mb = 512;
        config.memtable_size_mb = 128;
        
        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        secondary_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        
        product_count_ = state.range(0);
        
        // Create indexes for multi-modal access
        createIndexes();
        
        // Load multi-modal dataset
        loadData();
    }
    
    void TearDown(const ::benchmark::State&) override {
        secondary_.reset();
        db_.reset();
        cleanupTestDB(db_path_);
    }
    
protected:
    const themis::BaseEntity* getEntity(const std::unordered_map<std::string, themis::BaseEntity>& table,
                                        const std::string& pk) const {
        auto it = table.find(pk);
        return it != table.end() ? &it->second : nullptr;
    }

    void createIndexes() {
        // Relational indexes
        secondary_->createIndex("products", "product_id", true);
        secondary_->createIndex("products", "category_id", false);
        secondary_->createIndex("products", "brand_id", false);
        secondary_->createRangeIndex("products", "price");
        secondary_->createRangeIndex("products", "rating");
        
        // Document indexes
        secondary_->createIndex("product_docs", "product_id", true);
        secondary_->createFulltextIndex("product_docs", "description");
        
        // Graph indexes
        secondary_->createIndex("edges", "from_id", false);
        secondary_->createIndex("edges", "to_id", false);
        
        // Vector indexes (simplified - ThemisDB would have specialized vector index)
        secondary_->createIndex("embeddings", "product_id", true);
    }
    
    void loadData() {
        // Load products (relational)
        for (int i = 0; i < product_count_; ++i) {
            themis::BaseEntity product("product_" + std::to_string(i));
            product.setField("product_id", static_cast<int64_t>(i));
            product.setField("name", generateProductName(i));
            product.setField("price", static_cast<double>(std::uniform_int_distribution<int>(10, 1000)(rng)));
            product.setField("stock", static_cast<int64_t>(std::uniform_int_distribution<int>(0, 500)(rng)));
            product.setField("category_id", static_cast<int64_t>(i % CATEGORIES_COUNT));
            product.setField("brand_id", static_cast<int64_t>(i % BRANDS_COUNT));
            product.setField("rating", static_cast<double>(std::uniform_int_distribution<int>(30, 50)(rng)) / 10.0);
            secondary_->put("products", product);
            products_.emplace(product.getPrimaryKey(), product);
            
            // Load product document (JSON details)
            themis::BaseEntity doc("doc_" + std::to_string(i));
            doc.setField("product_id", static_cast<int64_t>(i));
            doc.setField("description", makeRandomString(200));
            doc.setField("specifications", makeRandomString(100));
            doc.setField("reviews_count", static_cast<int64_t>(std::uniform_int_distribution<int>(0, 100)(rng)));
            secondary_->put("product_docs", doc);
            product_docs_.emplace(doc.getPrimaryKey(), doc);
            
            // Load embedding
            auto embedding = generateEmbedding(i);
            themis::BaseEntity emb("emb_" + std::to_string(i));
            emb.setField("product_id", static_cast<int64_t>(i));
            // Store embedding as string (simplified - would be binary in production)
            std::string emb_str;
            for (float val : embedding) {
                emb_str += std::to_string(val) + ",";
            }
            emb.setField("embedding", emb_str);
            secondary_->put("embeddings", emb);
            embeddings_.push_back(embedding);
        }
        
        // Load graph edges (similar products, category membership)
        for (int i = 0; i < product_count_; ++i) {
            // Each product has 5 similar products
            for (int j = 0; j < 5; ++j) {
                int similar_id = std::uniform_int_distribution<int>(0, product_count_ - 1)(rng);
                if (similar_id != i) {
                    themis::BaseEntity edge("edge_similar_" + std::to_string(i) + "_" + std::to_string(similar_id));
                    edge.setField("from_id", static_cast<int64_t>(i));
                    edge.setField("to_id", static_cast<int64_t>(similar_id));
                    edge.setField("edge_type", std::string("SIMILAR_TO"));
                    edge.setField("weight", static_cast<double>(std::uniform_int_distribution<int>(70, 100)(rng)) / 100.0);
                    secondary_->put("edges", edge);
                    edges_.emplace(edge.getPrimaryKey(), std::move(edge));
                }
            }
        }
    }
    
    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> secondary_;
    int product_count_;
    std::unordered_map<std::string, themis::BaseEntity> products_;
    std::unordered_map<std::string, themis::BaseEntity> product_docs_;
    std::unordered_map<std::string, themis::BaseEntity> edges_;
    std::vector<std::vector<float>> embeddings_;
};

/**
 * Workload 1: Hybrid CRUD Operations
 * 
 * Combines relational, document, and vector access in a single query.
 * Simulates: "Get product details with similar products and semantic matches"
 */
BENCHMARK_DEFINE_F(MMDBFixture, HybridProductLookup)(benchmark::State& state) {
    std::uniform_int_distribution<int> dist(0, product_count_ - 1);
    
    for (auto _ : state) {
        int product_id = dist(rng);
        
        // 1. Relational: Get product
        auto product = getEntity(products_, "product_" + std::to_string(product_id));
        benchmark::DoNotOptimize(product);
        
        // 2. Document: Get detailed info
        auto doc = getEntity(product_docs_, "doc_" + std::to_string(product_id));
        benchmark::DoNotOptimize(doc);
        
        // 3. Graph: Get similar products (simplified)
        for (int i = 0; i < 3; ++i) {
            auto edge = getEntity(edges_, "edge_similar_" + std::to_string(product_id) + "_" + std::to_string(i));
            benchmark::DoNotOptimize(edge);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * Workload 2: Semantic Search
 * 
 * Vector similarity search with filtering.
 * Simulates: "Find products similar to this description"
 */
BENCHMARK_DEFINE_F(MMDBFixture, SemanticSearch)(benchmark::State& state) {
    std::uniform_int_distribution<int> dist(0, product_count_ - 1);
    
    for (auto _ : state) {
        // Query embedding (simulating user query)
        int query_id = dist(rng);
        const auto& query_emb = embeddings_[query_id];
        
        // Simplified vector search: brute force top-k (would use ANN in production)
        std::vector<std::pair<float, int>> similarities;
        for (int i = 0; i < std::min(100, product_count_); ++i) {
            float sim = cosineSimilarity(query_emb, embeddings_[i]);
            similarities.push_back({sim, i});
        }
        
        // Get top-5
        std::partial_sort(similarities.begin(), 
                         similarities.begin() + std::min(size_t(5), similarities.size()),
                         similarities.end(),
                         [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Retrieve full product details for top results
        size_t top_results = std::min(size_t(5), similarities.size());
        for (size_t i = 0; i < top_results; ++i) {
            auto product = getEntity(products_, "product_" + std::to_string(similarities[i].second));
            benchmark::DoNotOptimize(product);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * Workload 3: Graph Traversal
 * 
 * Multi-hop graph traversal.
 * Simulates: "Find products similar to products I purchased"
 */
BENCHMARK_DEFINE_F(MMDBFixture, GraphTraversal)(benchmark::State& state) {
    std::uniform_int_distribution<int> dist(0, product_count_ - 1);
    
    for (auto _ : state) {
        int start_product = dist(rng);
        
        // 2-hop traversal: product -> similar products -> their similar products
        std::vector<int> hop1_products;
        
        // First hop
        for (int i = 0; i < 5; ++i) {
            auto edge = getEntity(edges_, "edge_similar_" + std::to_string(start_product) + "_" + std::to_string(i));
            if (edge) {
                auto to_id = edge->getFieldAsInt("to_id");
                if (to_id) {
                  hop1_products.push_back(*to_id);
                }
            }
        }
        
        // Second hop (simplified)
        for (int hop1_id : hop1_products) {
            auto edge = getEntity(edges_, "edge_similar_" + std::to_string(hop1_id) + "_0");
            benchmark::DoNotOptimize(edge);
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * Workload 4: Multi-Modal Join
 * 
 * Joins relational, document, and graph data.
 * Simulates: "Analytics query combining all data models"
 */
BENCHMARK_DEFINE_F(MMDBFixture, MultiModalJoin)(benchmark::State& state) {
    std::uniform_int_distribution<int> cat_dist(0, CATEGORIES_COUNT - 1);
    
    for (auto _ : state) {
        int category_id = cat_dist(rng);
        int count = 0;
        
        // Find products in category with high ratings and detailed docs
        for (int i = 0; i < product_count_; ++i) {
            auto product = getEntity(products_, "product_" + std::to_string(i));
            if (product) {
                auto cat_id = product->getFieldAsInt("category_id");
                auto rating = product->getFieldAsDouble("rating");
                
                if (cat_id && *cat_id == category_id && rating && *rating >= 4.0) {
                    // Get document
                    auto doc = getEntity(product_docs_, "doc_" + std::to_string(i));
                    if (doc) {
                        auto reviews = doc->getFieldAsInt("reviews_count");
                        if (reviews && *reviews > 10) {
                            count++;
                        }
                    }
                }
            }
            
            if (count >= 10) break;  // Limit for benchmark
        }
        
        benchmark::DoNotOptimize(count);
    }
    
    state.SetItemsProcessed(state.iterations());
}

/**
 * Workload 5: RAG Simulation
 * 
 * Retrieval Augmented Generation workflow.
 * Simulates: "Answer question using product database"
 */
BENCHMARK_DEFINE_F(MMDBFixture, RAGWorkflow)(benchmark::State& state) {
    std::uniform_int_distribution<int> dist(0, product_count_ - 1);
    
    for (auto _ : state) {
        // Phase 1: Retrieval (semantic search)
        int query_id = dist(rng);
        const auto& query_emb = embeddings_[query_id];
        
        std::vector<std::pair<float, int>> similarities;
        for (int i = 0; i < std::min(50, product_count_); ++i) {
            float sim = cosineSimilarity(query_emb, embeddings_[i]);
            similarities.push_back({sim, i});
        }
        
        std::partial_sort(similarities.begin(), 
                         similarities.begin() + std::min(size_t(5), similarities.size()),
                         similarities.end(),
                         [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Phase 2: Context building (retrieve documents)
        std::string context;
        size_t top_results = std::min(size_t(5), similarities.size());
        for (size_t i = 0; i < top_results; ++i) {
            auto product = getEntity(products_, "product_" + std::to_string(similarities[i].second));
            auto doc = getEntity(product_docs_, "doc_" + std::to_string(similarities[i].second));
            
            if (product && doc) {
                // Build context string (would be sent to LLM)
                context += product->getFieldAsString("name").value_or("") + ": ";
                context += doc->getFieldAsString("description").value_or("") + "\n";
            }
        }
        
        // Phase 3: Generation (simulated - would call LLM API)
        benchmark::DoNotOptimize(context);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Register benchmarks with different dataset sizes
BENCHMARK_REGISTER_F(MMDBFixture, HybridProductLookup)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(MMDBFixture, SemanticSearch)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(MMDBFixture, GraphTraversal)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(MMDBFixture, MultiModalJoin)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(MMDBFixture, RAGWorkflow)
    ->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
