#include <benchmark/benchmark.h>

#include "index/secondary_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr int kCategoryCount = 10;
constexpr int kEdgeFanout = 4;
std::atomic<uint64_t> next_run_id{0};

std::string makeDatabasePath() {
    const auto run_id = next_run_id.fetch_add(1, std::memory_order_relaxed);
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() /
            ("themis_bench_mmdb_" + std::to_string(timestamp) + "_" +
             std::to_string(run_id))).string();
}

class MMDBFixture : public benchmark::Fixture {
public:
    ~MMDBFixture() override {
        release();
    }

    void SetUp(const benchmark::State& state) override {
        product_count_ = static_cast<int>(state.range(0));
        db_path_ = makeDatabasePath();
        std::error_code error;
        std::filesystem::remove_all(db_path_, error);

        themis::RocksDBWrapper::Config config;
        config.db_path = db_path_;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";
        config.block_cache_size_mb = 128;
        config.memtable_size_mb = 64;

        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        if (!db_->open()) {
            release();
            throw std::runtime_error("Failed to open RocksDB for MMDB benchmark");
        }

        secondary_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        requireIndex("products", "product_id", true);
        requireIndex("products", "category_id", false);
        requireIndex("product_docs", "product_id", true);
        requireIndex("edges", "from_id", false);
        loadData();
        warmUp();
    }

    void TearDown(const benchmark::State&) override {
        release();
    }

protected:
    using QueryResult = std::pair<themis::SecondaryIndexManager::Status,
                                  std::vector<themis::BaseEntity>>;

    QueryResult query(std::string_view table, std::string_view column,
                      std::string_view value) const {
        return secondary_->scanEntitiesEqual(table, column, value);
    }

    void loadData() {
        for (int id = 0; id < product_count_; ++id) {
            const auto product_id = std::to_string(id);
            const auto category_id = std::to_string(id % kCategoryCount);

            themis::BaseEntity product("product_" + product_id);
            product.setField("product_id", product_id);
            product.setField("category_id", category_id);
            product.setField("name", "Product " + product_id);
            requirePut("products", product);

            themis::BaseEntity document("document_" + product_id);
            document.setField("product_id", product_id);
            document.setField("description", "Details for product " + product_id);
            requirePut("product_docs", document);

            for (int offset = 1; offset <= kEdgeFanout; ++offset) {
                const auto target_id = std::to_string((id + offset) % product_count_);
                themis::BaseEntity edge("edge_" + product_id + "_" +
                                        std::to_string(offset));
                edge.setField("from_id", product_id);
                edge.setField("to_id", target_id);
                requirePut("edges", edge);
            }
        }
    }

    void warmUp() {
        const auto readRecord = [this](const std::string& id) {
            const auto [product_status, products] =
                query("products", "product_id", id);
            const auto [document_status, documents] =
                query("product_docs", "product_id", id);
            const auto [edge_status, edges] = query("edges", "from_id", id);
            if (!product_status.ok || products.size() != 1 ||
                !document_status.ok || documents.size() != 1 ||
                !edge_status.ok || edges.size() != kEdgeFanout) {
                throw std::runtime_error("MMDB benchmark warmup read failed");
            }
        };

        // Fixture loading performs the cold writes; keep sequential and hot reads
        // outside Google Benchmark's timed iteration window.
        for (int id = 0; id < 100; ++id) {
            readRecord(std::to_string(id));
        }
        for (int index = 0; index < 200; ++index) {
            readRecord(std::to_string((index * 37) % product_count_));
        }
    }

    void requireIndex(std::string_view table, std::string_view column, bool unique) {
        const auto status = secondary_->createIndex(table, column, unique);
        if (!status.ok) {
            throw std::runtime_error("Failed to create MMDB benchmark index: " +
                                     status.message);
        }
    }

    void requirePut(std::string_view table, const themis::BaseEntity& entity) {
        const auto status = secondary_->put(table, entity);
        if (!status.ok) {
            throw std::runtime_error("Failed to load MMDB benchmark data: " +
                                     status.message);
        }
    }

    void release() noexcept {
        secondary_.reset();
        db_.reset();
        if (!db_path_.empty()) {
            std::error_code error;
            std::filesystem::remove_all(db_path_, error);
            db_path_.clear();
        }
    }

    int product_count_ = 0;
    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> secondary_;
};

BENCHMARK_DEFINE_F(MMDBFixture, IndexedProductLookup)(benchmark::State& state) {
    state.SetLabel("component");
    for (auto _ : state) {
        const auto product_id = std::to_string(state.iterations() % product_count_);
        const auto [status, products] = query("products", "product_id", product_id);
        if (!status.ok || products.size() != 1) {
            state.SkipWithError(status.ok ? "Indexed product lookup returned wrong count"
                                          : status.message.c_str());
            break;
        }
        benchmark::DoNotOptimize(products.front());
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(MMDBFixture, CategoryDocumentJoin)(benchmark::State& state) {
    state.SetLabel("component");
    int64_t joined_rows = 0;
    for (auto _ : state) {
        const auto category_id =
            std::to_string(state.iterations() % kCategoryCount);
        const auto [product_status, products] =
            query("products", "category_id", category_id);
        const auto expected_products =
            static_cast<size_t>(product_count_ / kCategoryCount);
        if (!product_status.ok || products.size() != expected_products) {
            state.SkipWithError(product_status.ok ? "Category lookup returned wrong count"
                                                  : product_status.message.c_str());
            break;
        }

        size_t joined_documents = 0;
        for (const auto& product : products) {
            const auto product_id =
                product.getFieldAsString("product_id").value_or("");
            if (product.getFieldAsString("category_id").value_or("") != category_id) {
                state.SkipWithError("Category index returned a product with the wrong category");
                return;
            }
            const auto [document_status, documents] =
                query("product_docs", "product_id", product_id);
            if (!document_status.ok || documents.size() != 1) {
                state.SkipWithError(document_status.ok
                                        ? "Product/document join returned wrong count"
                                        : document_status.message.c_str());
                return;
            }
            if (documents.front().getFieldAsString("product_id").value_or("") != product_id) {
                state.SkipWithError("Product/document join returned a mismatched document");
                return;
            }
            ++joined_documents;
        }
        if (joined_documents != products.size()) {
            state.SkipWithError("Product/document join lost rows");
            break;
        }
        joined_rows += static_cast<int64_t>(joined_documents);
        benchmark::DoNotOptimize(joined_documents);
    }
    state.SetItemsProcessed(joined_rows);
}

BENCHMARK_DEFINE_F(MMDBFixture, GraphOneHop)(benchmark::State& state) {
    state.SetLabel("component");
    for (auto _ : state) {
        const auto source_index =
            static_cast<int>(state.iterations() % product_count_);
        const auto source_id = std::to_string(source_index);
        const auto [status, edges] = query("edges", "from_id", source_id);
        if (!status.ok || edges.size() != kEdgeFanout) {
            state.SkipWithError(status.ok ? "Graph one-hop traversal returned wrong count"
                                          : status.message.c_str());
            break;
        }
        bool valid_edges = true;
        for (int offset = 1; offset <= kEdgeFanout; ++offset) {
            const auto expected_target =
                std::to_string((source_index + offset) % product_count_);
            valid_edges &= std::any_of(edges.begin(), edges.end(),
                [&](const auto& edge) {
                    return edge.getFieldAsString("from_id").value_or("") == source_id &&
                           edge.getFieldAsString("to_id").value_or("") == expected_target;
                });
            if (!valid_edges) {
                state.SkipWithError("Graph index returned an invalid edge");
                break;
            }
        }
        if (!valid_edges) break;
        benchmark::DoNotOptimize(edges);
    }
    state.SetItemsProcessed(state.iterations() * kEdgeFanout);
}

BENCHMARK_DEFINE_F(MMDBFixture, MultiModelContextAssembly)(benchmark::State& state) {
    state.SetLabel("component");
    int64_t records_read = 0;
    for (auto _ : state) {
        const auto product_index =
            static_cast<int>(state.iterations() % product_count_);
        const auto product_id = std::to_string(product_index);
        const auto [product_status, products] =
            query("products", "product_id", product_id);
        const auto [document_status, documents] =
            query("product_docs", "product_id", product_id);
        const auto [edge_status, edges] = query("edges", "from_id", product_id);
        if (!product_status.ok || products.size() != 1 ||
            !document_status.ok || documents.size() != 1 ||
            !edge_status.ok || edges.size() != kEdgeFanout) {
            state.SkipWithError("Multi-model indexed lookup failed correctness checks");
            break;
        }
        if (products.front().getFieldAsString("product_id").value_or("") != product_id ||
            documents.front().getFieldAsString("product_id").value_or("") != product_id) {
            state.SkipWithError("Multi-model lookup returned mismatched records");
            break;
        }

        std::string context =
            products.front().getFieldAsString("name").value_or("") + ": " +
            documents.front().getFieldAsString("description").value_or("");
        context += "\nRelated products:";
        bool valid_edges = true;
        for (int offset = 1; offset <= kEdgeFanout; ++offset) {
            const auto expected_target =
                std::to_string((product_index + offset) % product_count_);
            const auto edge = std::find_if(edges.begin(), edges.end(),
                [&](const auto& candidate) {
                    return candidate.getFieldAsString("from_id").value_or("") ==
                               product_id &&
                           candidate.getFieldAsString("to_id").value_or("") ==
                               expected_target;
                });
            if (edge == edges.end()) {
                state.SkipWithError("Context assembly returned an unexpected edge set");
                valid_edges = false;
                break;
            }
            context += " " + expected_target;
        }
        if (!valid_edges) {
            break;
        }
        records_read += 2 + static_cast<int64_t>(edges.size());
        benchmark::DoNotOptimize(context);
    }
    state.SetItemsProcessed(records_read);
}

BENCHMARK_REGISTER_F(MMDBFixture, IndexedProductLookup)
    ->Arg(1000)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK_REGISTER_F(MMDBFixture, CategoryDocumentJoin)
    ->Arg(1000)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK_REGISTER_F(MMDBFixture, GraphOneHop)
    ->Arg(1000)->Unit(benchmark::kMicrosecond)->UseRealTime();
BENCHMARK_REGISTER_F(MMDBFixture, MultiModelContextAssembly)
    ->Arg(1000)->Unit(benchmark::kMicrosecond)->UseRealTime();

} // namespace

BENCHMARK_MAIN();
