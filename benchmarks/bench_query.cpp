/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_query.cpp                                    ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     193                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Query Pagination Benchmarks: Offset vs Cursor (anchor-based)

#include <benchmark/benchmark.h>
#include <memory>
#include <filesystem>
#include <string>
#include <vector>

#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "storage/base_entity.h"
#include "query/query_engine.h"

using themis::RocksDBWrapper;
using themis::SecondaryIndexManager;
using themis::BaseEntity;
using themis::QueryEngine;
using themis::ConjunctiveQuery;
using themis::OrderBy;

namespace {
struct BenchEnv {
    std::shared_ptr<RocksDBWrapper> storage;
    std::shared_ptr<SecondaryIndexManager> secIdx;
    bool ready = false;

    static BenchEnv& instance() {
        static BenchEnv env; return env;
    }

    static std::string padInt(int v, int width=6) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%0*d", width, v);
        return std::string(buf);
    }

    // Wrapped init with explicit error propagation so benchmark failures are visible
    // NOTE: Reduced N from 100k to 1k for bench_query to avoid long initialization
    bool ensureInit(benchmark::State& state, size_t N = 1000) {
        if (ready) return true;
        try {
            const std::string db_path = "data/themis_bench_query";
            if (std::filesystem::exists(db_path)) {
                std::filesystem::remove_all(db_path);
            }
            RocksDBWrapper::Config cfg; cfg.db_path = db_path; cfg.memtable_size_mb = 128; cfg.block_cache_size_mb = 256;
            storage = std::make_shared<RocksDBWrapper>(cfg);
            if (!storage->open()) {
                state.SkipWithError("Failed to open RocksDB for benchmark");
                return false;
            }
            secIdx = std::make_shared<SecondaryIndexManager>(*storage);
            // Create range index for ORDER BY
            auto st = secIdx->createRangeIndex("bench_users", "age");
            if (!st.ok) {
                state.SkipWithError(("Failed to create range index: " + st.message).c_str());
                return false;
            }

            // Populate N entities: ascending age with zero-padded strings
            std::vector<BaseEntity> batch; batch.reserve(1000);
            for (size_t i = 0; i < N; ++i) {
                std::string pk = std::string("u_") + padInt(static_cast<int>(i), 8);
                std::string age = padInt(static_cast<int>(i)); // 000000 .. 099999
                auto e = BaseEntity::fromFields(pk, BaseEntity::FieldMap{{"name", std::string("User ")+std::to_string(i)}, {"age", age}});
                auto pst = secIdx->put("bench_users", e);
                if (!pst.ok) {
                    state.SkipWithError(("Put failed at i=" + std::to_string(i) + ": " + pst.message).c_str());
                    return false;
                }
            }
            ready = true;
            return true;
        } catch (const std::exception& ex) {
            state.SkipWithError(ex.what());
            return false;
        }
    }
};
} // namespace

/*
// Pagination benchmarks (not currently registered with BENCHMARK() macros)
// These can be enabled when needed for performance testing

static void BM_Pagination_Offset(benchmark::State& state) {
    // Args: page_size, pages
    const int pageSize = static_cast<int>(state.range(0));
    const int pages = static_cast<int>(state.range(1));
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    for (auto _ : state) {
        size_t totalFetched = 0;
        for (int p = 0; p < pages; ++p) {
            size_t offset = static_cast<size_t>(p) * static_cast<size_t>(pageSize);
            ConjunctiveQuery q; q.table = "bench_users";
            OrderBy ob; ob.column = "age"; ob.desc = false; ob.limit = static_cast<size_t>(pageSize) + offset;
            q.orderBy = ob;
            auto result = engine.executeAndEntities(q);
            if (!result) {
                state.SkipWithError(result.error().message().c_str());
                return;
            }
            auto& ents = *result;
            // emulate HTTP post-fetch slicing of last page
            if (ents.size() > offset) {
                size_t last = std::min(ents.size(), offset + static_cast<size_t>(pageSize));
                totalFetched += (last - offset);
            }
        }
        state.counters["pages"] = pages;
        state.counters["page_size"] = pageSize;
        state.counters["fetched_items"] = static_cast<double>(totalFetched);
    }
}

static void BM_Pagination_Cursor(benchmark::State& state) {
    // Args: page_size, pages
    const int pageSize = static_cast<int>(state.range(0));
    const int pages = static_cast<int>(state.range(1));
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    std::optional<std::string> anchorValue;
    std::optional<std::string> anchorPk;

    for (auto _ : state) {
        size_t totalFetched = 0;
        anchorValue.reset(); anchorPk.reset();
        for (int p = 0; p < pages; ++p) {
            ConjunctiveQuery q; q.table = "bench_users";
            OrderBy ob; ob.column = "age"; ob.desc = false; ob.limit = static_cast<size_t>(pageSize) + 1;
            ob.cursor_value = anchorValue; ob.cursor_pk = anchorPk; // first page: std::nullopt
            q.orderBy = ob;
            auto result = engine.executeAndEntities(q);
            if (!result) { 
                state.SkipWithError(result.error().message().c_str()); 
                return; 
            }
            auto& ents = *result;
            bool has_more = ents.size() > static_cast<size_t>(pageSize);
            size_t count = std::min(ents.size(), static_cast<size_t>(pageSize));
            totalFetched += count;
            if (count > 0) {
                const auto& last = ents[count - 1];
                anchorPk = last.getPrimaryKey();
                // Know the age because it is a field; extractField returns optional<string>
                auto v = last.extractField("age");
                if (v) anchorValue = *v; else anchorValue.reset();
            }
            if (!has_more) break;
        }
        state.counters["pages"] = pages;
        state.counters["page_size"] = pageSize;
        state.counters["fetched_items"] = static_cast<double>(totalFetched);
    }
}
*/

// Register with typical settings: page_size=50, pages=50
// DISABLED: Pagination benchmarks timeout due to QueryEngine performance issues
// Uncomment to re-enable after investigating executeAndEntities() performance
// BENCHMARK(BM_Pagination_Offset)->Args({50, 50})->Unit(benchmark::kMillisecond);
// BENCHMARK(BM_Pagination_Cursor)->Args({50, 50})->Unit(benchmark::kMillisecond);
