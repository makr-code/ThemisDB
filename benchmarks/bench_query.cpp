/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_query.cpp                                    ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:48:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     740                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9a52ef6bb1  2026-04-13  perf(query): add 1:1 point-lookup benchmarks and pk_eq fa... ║
    • b55d2d72cc  2026-04-11  perf(index): reduce secondary-index write-path overhead (... ║
    • ef29ea45e7  2026-04-11  feat(bench_query): add benchmarks for simple, complex que... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Query Pagination Benchmarks: Offset vs Cursor (anchor-based)

#include <benchmark/benchmark.h>
#include <algorithm>
#include <cstdio>
#include <chrono>
#include <memory>
#include <filesystem>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "query/query_engine.h"

using themis::RocksDBWrapper;
using themis::SecondaryIndexManager;
using themis::BaseEntity;
using themis::QueryEngine;
using themis::ConjunctiveQuery;
using themis::OrderBy;
using themis::PredicateEq;

namespace {
struct BenchEnv {
    std::shared_ptr<RocksDBWrapper> storage;
    std::shared_ptr<SecondaryIndexManager> secIdx;
    bool ready = false;
    size_t initializedN = 0;

    static BenchEnv& instance() {
        static BenchEnv env; return env;
    }

    static std::string padInt(int v, int width=6) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%0*d", width, v);
        return std::string(buf);
    }

    // Wrapped init with explicit error propagation so benchmark failures are visible.
    // NOTE: Reduced N from 100k to 1k for bench_query to avoid long initialization.
    bool ensureInit(benchmark::State& state, size_t N = 1000) {
        if (ready && initializedN == N) return true;
        try {
            const std::string db_path = std::string("data/themis_bench_query_") + std::to_string(N);
            if (std::filesystem::exists(db_path)) {
                std::filesystem::remove_all(db_path);
            }
            storage.reset();
            secIdx.reset();
            RocksDBWrapper::Config cfg; cfg.db_path = db_path; cfg.memtable_size_mb = 128; cfg.block_cache_size_mb = 256;
            storage = std::make_shared<RocksDBWrapper>(cfg);
            if (!storage->open()) {
                state.SkipWithError("Failed to open RocksDB for benchmark");
                return false;
            }
            secIdx = std::make_shared<SecondaryIndexManager>(*storage);
            // Create indices for pagination and dedicated WHERE/JOIN-style benchmarks.
            auto st = secIdx->createRangeIndex("bench_users", "age");
            if (!st.ok) {
                state.SkipWithError(("Failed to create range index: " + st.message).c_str());
                return false;
            }
            auto city_idx = secIdx->createIndex("bench_users", "city");
            if (!city_idx.ok) {
                state.SkipWithError(("Failed to create city index: " + city_idx.message).c_str());
                return false;
            }
            auto status_idx = secIdx->createIndex("bench_users", "status");
            if (!status_idx.ok) {
                state.SkipWithError(("Failed to create status index: " + status_idx.message).c_str());
                return false;
            }
            auto post_user_idx = secIdx->createIndex("bench_posts", "user_id");
            if (!post_user_idx.ok) {
                state.SkipWithError(("Failed to create post user_id index: " + post_user_idx.message).c_str());
                return false;
            }

            // Populate users with deterministic attributes for stable filtering.
            for (size_t i = 0; i < N; ++i) {
                std::string pk = std::string("u_") + padInt(static_cast<int>(i), 8);
                std::string age = padInt(static_cast<int>(i)); // 000000 .. 099999
                std::string city = std::string("city_") + padInt(static_cast<int>(i % 10), 2);
                std::string status = (i % 3 == 0) ? "active" : "inactive";
                auto e = BaseEntity::fromFields(pk, BaseEntity::FieldMap{{"name", std::string("User ")+std::to_string(i)}, {"age", age}, {"city", city}, {"status", status}});
                auto pst = secIdx->put("bench_users", e);
                if (!pst.ok) {
                    state.SkipWithError(("Put failed at i=" + std::to_string(i) + ": " + pst.message).c_str());
                    return false;
                }

                // Create a few posts per user for JOIN-like benchmarks.
                for (int j = 0; j < 3; ++j) {
                    std::string post_pk = std::string("p_") + padInt(static_cast<int>(i), 8) + "_" + std::to_string(j);
                    auto p = BaseEntity::fromFields(post_pk, BaseEntity::FieldMap{{"user_id", pk}, {"title", std::string("Post ") + std::to_string(j)}, {"tag", (j % 2 == 0) ? "tech" : "news"}});
                    auto post_st = secIdx->put("bench_posts", p);
                    if (!post_st.ok) {
                        state.SkipWithError(("Post put failed at i=" + std::to_string(i) + ": " + post_st.message).c_str());
                        return false;
                    }
                }
            }
            initializedN = N;
            ready = true;
            return true;
        } catch (const std::exception& ex) {
            state.SkipWithError(ex.what());
            return false;
        }
    }
};
} // namespace

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

static void BM_SimpleWhere(benchmark::State& state) {
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    for (auto _ : state) {
        ConjunctiveQuery q;
        q.table = "bench_users";
        q.predicates.push_back(PredicateEq{"city", "city_03"});

        auto result = engine.executeAndEntities(q);
        if (!result) {
            state.SkipWithError(result.error().message().c_str());
            return;
        }

        state.counters["matched_users"] = static_cast<double>(result->size());
    }
}

static void BM_ComplexWhere(benchmark::State& state) {
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    for (auto _ : state) {
        ConjunctiveQuery q;
        q.table = "bench_users";
        q.predicates.push_back(PredicateEq{"city", "city_03"});
        q.predicates.push_back(PredicateEq{"status", "active"});

        auto result = engine.executeAndEntities(q);
        if (!result) {
            state.SkipWithError(result.error().message().c_str());
            return;
        }

        state.counters["matched_users"] = static_cast<double>(result->size());
    }
}

static void BM_JoinUsersPosts(benchmark::State& state) {
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    for (auto _ : state) {
        ConjunctiveQuery users_q;
        users_q.table = "bench_users";
        users_q.predicates.push_back(PredicateEq{"city", "city_03"});
        users_q.predicates.push_back(PredicateEq{"status", "active"});

        auto users = engine.executeAndEntities(users_q);
        if (!users) {
            state.SkipWithError(users.error().message().c_str());
            return;
        }

        size_t joined_rows = 0;
        for (const auto& user : *users) {
            ConjunctiveQuery posts_q;
            posts_q.table = "bench_posts";
            posts_q.predicates.push_back(PredicateEq{"user_id", user.getPrimaryKey()});

            auto posts = engine.executeAndEntities(posts_q);
            if (!posts) {
                state.SkipWithError(posts.error().message().c_str());
                return;
            }
            joined_rows += posts->size();
        }

        state.counters["matched_users"] = static_cast<double>(users->size());
        state.counters["joined_rows"] = static_cast<double>(joined_rows);
    }
}

static bool run_simple_where(QueryEngine& engine, size_t& matched_users, std::string& err);
static bool run_complex_where(QueryEngine& engine, size_t& matched_users, std::string& err);
static bool run_join_users_posts(QueryEngine& engine, size_t& matched_users, size_t& joined_rows, std::string& err);

static void BM_SimpleWhere_Scaled(benchmark::State& state) {
    const size_t N = static_cast<size_t>(state.range(0));
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state, N)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    for (auto _ : state) {
        std::string err;
        size_t matched_users = 0;
        if (!run_simple_where(engine, matched_users, err)) {
            state.SkipWithError(err.c_str());
            return;
        }
        state.counters["matched_users"] = static_cast<double>(matched_users);
        state.counters["dataset_n"] = static_cast<double>(N);
    }
}

static void BM_ComplexWhere_Scaled(benchmark::State& state) {
    const size_t N = static_cast<size_t>(state.range(0));
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state, N)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    for (auto _ : state) {
        std::string err;
        size_t matched_users = 0;
        if (!run_complex_where(engine, matched_users, err)) {
            state.SkipWithError(err.c_str());
            return;
        }
        state.counters["matched_users"] = static_cast<double>(matched_users);
        state.counters["dataset_n"] = static_cast<double>(N);
    }
}

static void BM_JoinUsersPosts_Scaled(benchmark::State& state) {
    const size_t N = static_cast<size_t>(state.range(0));
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state, N)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    for (auto _ : state) {
        std::string err;
        size_t matched_users = 0;
        size_t joined_rows = 0;
        if (!run_join_users_posts(engine, matched_users, joined_rows, err)) {
            state.SkipWithError(err.c_str());
            return;
        }
        state.counters["matched_users"] = static_cast<double>(matched_users);
        state.counters["joined_rows"] = static_cast<double>(joined_rows);
        state.counters["dataset_n"] = static_cast<double>(N);
    }
}

static double percentile_us(std::vector<double>& samples_us, double p) {
    if (samples_us.empty()) {
        return 0.0;
    }
    std::sort(samples_us.begin(), samples_us.end());
    const double rank = (p / 100.0) * static_cast<double>(samples_us.size() - 1);
    const size_t idx = static_cast<size_t>(rank);
    return samples_us[idx];
}

static bool run_simple_where(QueryEngine& engine, size_t& matched_users, std::string& err) {
    ConjunctiveQuery q;
    q.table = "bench_users";
    q.predicates.push_back(PredicateEq{"city", "city_03"});
    auto result = engine.executeAndEntities(q);
    if (!result) {
        err = result.error().message();
        return false;
    }
    matched_users = result->size();
    return true;
}

static bool run_complex_where(QueryEngine& engine, size_t& matched_users, std::string& err) {
    ConjunctiveQuery q;
    q.table = "bench_users";
    q.predicates.push_back(PredicateEq{"city", "city_03"});
    q.predicates.push_back(PredicateEq{"status", "active"});
    auto result = engine.executeAndEntities(q);
    if (!result) {
        err = result.error().message();
        return false;
    }
    matched_users = result->size();
    return true;
}

static bool run_join_users_posts(QueryEngine& engine, size_t& matched_users, size_t& joined_rows, std::string& err) {
    matched_users = 0;
    joined_rows = 0;

    ConjunctiveQuery users_q;
    users_q.table = "bench_users";
    users_q.predicates.push_back(PredicateEq{"city", "city_03"});
    users_q.predicates.push_back(PredicateEq{"status", "active"});

    auto users = engine.executeAndEntities(users_q);
    if (!users) {
        err = users.error().message();
        return false;
    }

    matched_users = users->size();
    for (const auto& user : *users) {
        ConjunctiveQuery posts_q;
        posts_q.table = "bench_posts";
        posts_q.predicates.push_back(PredicateEq{"user_id", user.getPrimaryKey()});

        auto posts = engine.executeAndEntities(posts_q);
        if (!posts) {
            err = posts.error().message();
            return false;
        }
        joined_rows += posts->size();
    }
    return true;
}

static void BM_SimpleWhere_P99(benchmark::State& state) {
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    constexpr size_t kSamples = 300;
    std::vector<double> samples_us;
    samples_us.reserve(kSamples);
    size_t matched_users = 0;

    for (auto _ : state) {
        (void)_;
        samples_us.clear();
        for (size_t i = 0; i < kSamples; ++i) {
            std::string err;
            auto t0 = std::chrono::high_resolution_clock::now();
            bool ok = run_simple_where(engine, matched_users, err);
            auto t1 = std::chrono::high_resolution_clock::now();
            if (!ok) {
                state.SkipWithError(err.c_str());
                return;
            }
            const double us = std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(t1 - t0).count();
            samples_us.push_back(us);
        }

        const double p99_us = percentile_us(samples_us, 99.0);
        const double mean_us = std::accumulate(samples_us.begin(), samples_us.end(), 0.0) / static_cast<double>(samples_us.size());
        state.counters["p99_us"] = p99_us;
        state.counters["mean_us"] = mean_us;
        state.counters["qps_est"] = 1e6 / mean_us;
        state.counters["matched_users"] = static_cast<double>(matched_users);
    }
}

static void BM_ComplexWhere_P99(benchmark::State& state) {
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    constexpr size_t kSamples = 300;
    std::vector<double> samples_us;
    samples_us.reserve(kSamples);
    size_t matched_users = 0;

    for (auto _ : state) {
        (void)_;
        samples_us.clear();
        for (size_t i = 0; i < kSamples; ++i) {
            std::string err;
            auto t0 = std::chrono::high_resolution_clock::now();
            bool ok = run_complex_where(engine, matched_users, err);
            auto t1 = std::chrono::high_resolution_clock::now();
            if (!ok) {
                state.SkipWithError(err.c_str());
                return;
            }
            const double us = std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(t1 - t0).count();
            samples_us.push_back(us);
        }

        const double p99_us = percentile_us(samples_us, 99.0);
        const double mean_us = std::accumulate(samples_us.begin(), samples_us.end(), 0.0) / static_cast<double>(samples_us.size());
        state.counters["p99_us"] = p99_us;
        state.counters["mean_us"] = mean_us;
        state.counters["qps_est"] = 1e6 / mean_us;
        state.counters["matched_users"] = static_cast<double>(matched_users);
    }
}

static void BM_JoinUsersPosts_P99(benchmark::State& state) {
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    constexpr size_t kSamples = 150;
    std::vector<double> samples_us;
    samples_us.reserve(kSamples);
    size_t matched_users = 0;
    size_t joined_rows = 0;

    for (auto _ : state) {
        (void)_;
        samples_us.clear();
        for (size_t i = 0; i < kSamples; ++i) {
            std::string err;
            auto t0 = std::chrono::high_resolution_clock::now();
            bool ok = run_join_users_posts(engine, matched_users, joined_rows, err);
            auto t1 = std::chrono::high_resolution_clock::now();
            if (!ok) {
                state.SkipWithError(err.c_str());
                return;
            }
            const double us = std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(t1 - t0).count();
            samples_us.push_back(us);
        }

        const double p99_us = percentile_us(samples_us, 99.0);
        const double mean_us = std::accumulate(samples_us.begin(), samples_us.end(), 0.0) / static_cast<double>(samples_us.size());
        state.counters["p99_us"] = p99_us;
        state.counters["mean_us"] = mean_us;
        state.counters["qps_est"] = 1e6 / mean_us;
        state.counters["matched_users"] = static_cast<double>(matched_users);
        state.counters["joined_rows"] = static_cast<double>(joined_rows);
    }
}

// ── Historical method approximation ──────────────────────────────────────────
// Mirrors v1.3.4 workload characteristics:
//   • N=10000 dataset (bounded for CI; historical ran 100k+ in nightly)
//   • kWarmupIters=50 warmup queries executed before any timing
//   • Round-robin querymix: 60% SimpleWhere / 30% ComplexWhere / 10% JOIN

static void BM_QueryMix_Historical(benchmark::State& state) {
    constexpr size_t kDatasetN    = 10000;
    constexpr size_t kWarmupIters = 50;
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state, kDatasetN)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    // Warmup: populate index/cache paths before timing starts.
    for (size_t w = 0; w < kWarmupIters; ++w) {
        std::string err;
        size_t du = 0, dj = 0;
        if      (w % 10 == 9)  { run_join_users_posts(engine, du, dj, err); }
        else if (w % 10 >= 7)  { run_complex_where(engine, du, err);        }
        else                   { run_simple_where(engine, du, err);          }
        benchmark::DoNotOptimize(du);
    }

    size_t iter_count = 0;
    for (auto _ : state) {
        std::string err;
        size_t mu = 0, jr = 0;
        bool ok;
        // Deterministic round-robin mix: 6 Simple, 3 Complex, 1 JOIN per 10 iters.
        const size_t slot = iter_count % 10;
        if      (slot < 6) ok = run_simple_where(engine, mu, err);
        else if (slot < 9) ok = run_complex_where(engine, mu, err);
        else               ok = run_join_users_posts(engine, mu, jr, err);
        if (!ok) { state.SkipWithError(err.c_str()); return; }
        state.counters["dataset_n"]    = static_cast<double>(kDatasetN);
        state.counters["warmup_iters"] = static_cast<double>(kWarmupIters);
        ++iter_count;
    }
}

static void BM_QueryMix_Historical_P99(benchmark::State& state) {
    constexpr size_t kDatasetN    = 10000;
    constexpr size_t kWarmupIters = 50;
    constexpr size_t kSamples     = 300;
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state, kDatasetN)) return;
    QueryEngine engine(*env.storage, *env.secIdx);

    for (size_t w = 0; w < kWarmupIters; ++w) {
        std::string err;
        size_t du = 0, dj = 0;
        if      (w % 10 == 9)  { run_join_users_posts(engine, du, dj, err); }
        else if (w % 10 >= 7)  { run_complex_where(engine, du, err);        }
        else                   { run_simple_where(engine, du, err);          }
        benchmark::DoNotOptimize(du);
    }

    std::vector<double> samples_us;
    samples_us.reserve(kSamples);
    for (auto _ : state) {
        (void)_;
        samples_us.clear();
        for (size_t i = 0; i < kSamples; ++i) {
            std::string err;
            size_t mu = 0, jr = 0;
            bool ok;
            const size_t slot = i % 10;
            auto t0 = std::chrono::high_resolution_clock::now();
            if      (slot < 6) ok = run_simple_where(engine, mu, err);
            else if (slot < 9) ok = run_complex_where(engine, mu, err);
            else               ok = run_join_users_posts(engine, mu, jr, err);
            auto t1 = std::chrono::high_resolution_clock::now();
            if (!ok) { state.SkipWithError(err.c_str()); return; }
            samples_us.push_back(
                std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(t1 - t0).count());
        }
        const double p99_us  = percentile_us(samples_us, 99.0);
        const double mean_us = std::accumulate(samples_us.begin(), samples_us.end(), 0.0)
                               / static_cast<double>(samples_us.size());
        state.counters["p99_us"]       = p99_us;
        state.counters["mean_us"]      = mean_us;
        state.counters["qps_est"]      = 1e6 / mean_us;
        state.counters["dataset_n"]    = static_cast<double>(kDatasetN);
        state.counters["warmup_iters"] = static_cast<double>(kWarmupIters);
    }
}

// ── Direct primary-key point-lookup benchmarks ───────────────────────────────
// These are the 1:1 "single-entity read" cases that represent the OLTP hotpath:
// key construction → RocksDB get → optional deserialization.
// They provide the correct baseline for the 900 M/s throughput target and expose
// the per-query overhead relative to the raw storage get speed.

static void BM_PointLookup(benchmark::State& state) {
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;

    // Pre-build the lookup keys for all N entities so the hot loop only does
    // the storage get and no string construction per iteration.
    const size_t N = env.initializedN;
    std::vector<std::string> lookup_keys;
    lookup_keys.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        std::string pk = std::string("u_") + BenchEnv::padInt(static_cast<int>(i), 8);
        lookup_keys.push_back(KeySchema::makeRelationalKey("bench_users", pk));
    }

    size_t idx = 0;
    for (auto _ : state) {
        auto blob = env.storage->get(lookup_keys[idx % N]);
        benchmark::DoNotOptimize(blob);
        ++idx;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["dataset_n"] = static_cast<double>(N);
}

static void BM_PointLookup_WithDeserialize(benchmark::State& state) {
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;

    const size_t N = env.initializedN;
    std::vector<std::pair<std::string, std::string>> lookup_pairs; // (key, pk)
    lookup_pairs.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        std::string pk = std::string("u_") + BenchEnv::padInt(static_cast<int>(i), 8);
        lookup_pairs.emplace_back(KeySchema::makeRelationalKey("bench_users", pk), pk);
    }

    size_t idx = 0;
    for (auto _ : state) {
        const auto& [storage_key, pk] = lookup_pairs[idx % N];
        auto blob = env.storage->get(storage_key);
        if (blob.has_value()) {
            auto entity = BaseEntity::deserialize(pk, *blob);
            benchmark::DoNotOptimize(entity);
        }
        ++idx;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["dataset_n"] = static_cast<double>(N);
}

static void BM_PointLookup_P99(benchmark::State& state) {
    auto& env = BenchEnv::instance();
    if (!env.ensureInit(state)) return;

    const size_t N = env.initializedN;
    std::vector<std::pair<std::string, std::string>> lookup_pairs;
    lookup_pairs.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        std::string pk = std::string("u_") + BenchEnv::padInt(static_cast<int>(i), 8);
        lookup_pairs.emplace_back(KeySchema::makeRelationalKey("bench_users", pk), pk);
    }

    constexpr size_t kSamples = 300;
    std::vector<double> samples_us;
    samples_us.reserve(kSamples);

    for (auto _ : state) {
        (void)_;
        samples_us.clear();
        for (size_t i = 0; i < kSamples; ++i) {
            const auto& [storage_key, pk] = lookup_pairs[i % N];
            auto t0 = std::chrono::high_resolution_clock::now();
            auto blob = env.storage->get(storage_key);
            if (blob.has_value()) {
                auto entity = BaseEntity::deserialize(pk, *blob);
                benchmark::DoNotOptimize(entity);
            }
            auto t1 = std::chrono::high_resolution_clock::now();
            samples_us.push_back(
                std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(t1 - t0).count());
        }

        const double p99_us  = percentile_us(samples_us, 99.0);
        const double mean_us = std::accumulate(samples_us.begin(), samples_us.end(), 0.0)
                               / static_cast<double>(samples_us.size());
        state.counters["p99_us"]    = p99_us;
        state.counters["mean_us"]   = mean_us;
        state.counters["qps_est"]   = 1e6 / mean_us;
        state.counters["dataset_n"] = static_cast<double>(N);
    }
}

// Register with conservative defaults to keep runtime bounded in CI/local runs.
BENCHMARK(BM_Pagination_Offset)->Args({20, 10})->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Pagination_Offset)->Args({50, 50})->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Pagination_Cursor)->Args({20, 10})->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Pagination_Cursor)->Args({50, 50})->Unit(benchmark::kMillisecond);
BENCHMARK(BM_SimpleWhere)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ComplexWhere)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_JoinUsersPosts)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_SimpleWhere_P99)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ComplexWhere_P99)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_JoinUsersPosts_P99)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_SimpleWhere_Scaled)->Args({1000})->Args({10000})->Unit(benchmark::kMillisecond);
BENCHMARK(BM_ComplexWhere_Scaled)->Args({1000})->Args({10000})->Unit(benchmark::kMillisecond);
BENCHMARK(BM_JoinUsersPosts_Scaled)->Args({1000})->Args({10000})->Unit(benchmark::kMillisecond);
BENCHMARK(BM_PointLookup)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_PointLookup_WithDeserialize)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_PointLookup_P99)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_QueryMix_Historical)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_QueryMix_Historical_P99)->Unit(benchmark::kMillisecond);
