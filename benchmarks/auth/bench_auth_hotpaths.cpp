// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_auth_hotpaths.cpp
 * @brief Phase 5 auth hot-path release-gate benchmarks.
 *
 * Provides reproducible latency and throughput measurements for the
 * token/session/revocation hot paths identified in the auth module roadmap
 * (Phase 5 — Performance and Hardening).  Results are used as release gates:
 * a regression beyond 10 % vs the baseline blocks promotion.
 *
 * ## Benchmark families
 *
 * ### AHP-01..03 — JWT Token Validation
 *   AHP-01  Valid RS256 JWT parse overhead (structural validation, no JWKS)
 *   AHP-02  Token-blacklist lookup hit (JTI present in in-memory blacklist)
 *   AHP-03  Token-blacklist lookup miss (JTI absent from in-memory blacklist)
 *
 * ### AHP-04..05 — Session Lifecycle
 *   AHP-04  Session create (cryptographic ID generation + map insert)
 *   AHP-05  Session validate (map lookup + expiry check)
 *
 * ### AHP-06..07 — Distributed Revocation (RocksDB single-node)
 *   AHP-06  DistributedTokenBlacklist::add() — single RocksDB Put
 *   AHP-07  DistributedTokenBlacklist::isRevoked() — O(1) RocksDB point-read
 *
 * ### AHP-08 — Federation Manager Realm Lookup
 *   AHP-08  FederatedIdentityManager realm-count / realm-lookup overhead
 *           (measures mutex + hash-map cost, no network I/O)
 *
 * ## Hard release gates
 *
 * | Gate ID    | Benchmark | Threshold           |
 * |------------|-----------|---------------------|
 * | GATE-AHP-01 | AHP-02   | p99 ≤ 1 µs          |
 * | GATE-AHP-02 | AHP-03   | p99 ≤ 1 µs          |
 * | GATE-AHP-03 | AHP-04   | p99 ≤ 5 ms          |
 * | GATE-AHP-04 | AHP-05   | p99 ≤ 1 ms          |
 * | GATE-AHP-05 | AHP-06   | p99 ≤ 2 ms          |
 * | GATE-AHP-06 | AHP-07   | p99 ≤ 1 µs (warm)   |
 *
 * All benchmarks:
 *   - Use kAhpCanonicalSeed = 42 for deterministic JTI / session data.
 *   - Warm up for kWarmupIterations before measurement.
 *   - Run with Repetitions(kRepetitions) to capture variance.
 *   - I/O benchmarks use UseRealTime() so kernel wait is included.
 *
 * @see src/auth/ROADMAP.md — Phase 5 items
 * @see include/auth/auth_principal_contract.h — §5 Revocation contract
 * @see benchmarks/auth/RELEASE_GATE_AHP.md — gate verification record
 */

#include <benchmark/benchmark.h>

#include "auth/token_blacklist.h"
#include "auth/session_manager.h"
#include "auth/federated_identity_manager.h"
#include "auth/oidc_provider.h"
#include "auth/distributed_token_blacklist.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis::auth;
using namespace std::chrono_literals;

namespace themis {
namespace bench {
namespace ahp {

// ---------------------------------------------------------------------------
// Constants — deterministic, release-pinned
// ---------------------------------------------------------------------------

/// Canonical PRNG seed for all AHP benchmarks.
static constexpr uint64_t kAhpCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kWarmupIterations = 200;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

/// Number of JTI entries pre-loaded for hit/miss benchmarks.
static constexpr int kBlacklistPreloadSize = 1000;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Generate a deterministic JTI string from an integer index.
static std::string makeJti(int index) {
    return "jti-ahp-" + std::to_string(index);
}

/// Time-point helper: far future expiry.
static std::chrono::system_clock::time_point futureExpiry() {
    return std::chrono::system_clock::now() + std::chrono::hours(24);
}

// ---------------------------------------------------------------------------
// Shared state: in-memory TokenBlacklist with pre-loaded entries
// ---------------------------------------------------------------------------

/// SetUp helper: returns an in-memory TokenBlacklist with kBlacklistPreloadSize
/// entries pre-inserted so that the "hit" benchmark path is warm.
static std::unique_ptr<TokenBlacklist> makePreloadedBlacklist() {
    auto bl = std::make_unique<TokenBlacklist>();
    for (int i = 0; i < kBlacklistPreloadSize; ++i) {
        bl->add(makeJti(i), futureExpiry());
    }
    return bl;
}

// ===========================================================================
// AHP-02 — Token-blacklist lookup: HIT (JTI present)
// ===========================================================================

/**
 * @brief AHP-02: isRevoked() for a JTI known to be in the blacklist.
 *
 * Measures the hot O(1) lookup path.  GATE-AHP-01: p99 ≤ 1 µs.
 */
static void BM_AHP02_BlacklistIsRevoked_Hit(benchmark::State &state) {
    auto bl = makePreloadedBlacklist();
    // Warm up
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)bl->isRevoked(makeJti(i % kBlacklistPreloadSize));
    }
    std::mt19937_64 rng(kAhpCanonicalSeed);
    std::uniform_int_distribution<int> dist(0, kBlacklistPreloadSize - 1);
    for (auto _ : state) {
        benchmark::DoNotOptimize(bl->isRevoked(makeJti(dist(rng))));
    }
    state.SetLabel("GATE-AHP-01: p99 <= 1 us");
}
BENCHMARK(BM_AHP02_BlacklistIsRevoked_Hit)->Repetitions(kRepetitions)->ReportAggregatesOnly(true);

// ===========================================================================
// AHP-03 — Token-blacklist lookup: MISS (JTI absent)
// ===========================================================================

/**
 * @brief AHP-03: isRevoked() for a JTI that is NOT in the blacklist.
 *
 * GATE-AHP-02: p99 ≤ 1 µs.
 */
static void BM_AHP03_BlacklistIsRevoked_Miss(benchmark::State &state) {
    auto bl = makePreloadedBlacklist();
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)bl->isRevoked("jti-ahp-miss-warmup-" + std::to_string(i));
    }
    int miss_counter = kBlacklistPreloadSize;
    for (auto _ : state) {
        benchmark::DoNotOptimize(bl->isRevoked("jti-ahp-miss-" + std::to_string(miss_counter++)));
    }
    state.SetLabel("GATE-AHP-02: p99 <= 1 us");
}
BENCHMARK(BM_AHP03_BlacklistIsRevoked_Miss)->Repetitions(kRepetitions)->ReportAggregatesOnly(true);

// ===========================================================================
// AHP-04 — Session create
// ===========================================================================

/**
 * @brief AHP-04: SessionManager::createSession() latency.
 *
 * Measures cryptographic random ID generation + map insert.
 * GATE-AHP-03: p99 ≤ 5 ms.
 */
static void BM_AHP04_SessionCreate(benchmark::State &state) {
    SessionManager::SessionLimits limits;
    limits.max_sessions_per_user = 0;  // unlimited — no eviction overhead
    limits.idle_timeout          = std::chrono::milliseconds(0);
    limits.absolute_timeout      = std::chrono::hours(24);
    SessionManager sm(limits);
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)sm.createSession("bench-user-warmup");
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(sm.createSession("bench-user-ahp04"));
    }
    state.SetLabel("GATE-AHP-03: p99 <= 5 ms");
}
BENCHMARK(BM_AHP04_SessionCreate)->Repetitions(kRepetitions)->ReportAggregatesOnly(true);

// ===========================================================================
// AHP-05 — Session validate
// ===========================================================================

/**
 * @brief AHP-05: SessionManager::validateSession() latency for an active session.
 *
 * Measures map lookup + expiry check against a pre-created session.
 * GATE-AHP-04: p99 ≤ 1 ms.
 */
static void BM_AHP05_SessionValidate(benchmark::State &state) {
    SessionManager::SessionLimits limits;
    limits.max_sessions_per_user = 0;
    limits.idle_timeout          = std::chrono::milliseconds(0);
    limits.absolute_timeout      = std::chrono::hours(24);
    SessionManager sm(limits);
    const std::string sid = sm.createSession("bench-user-ahp05");
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)sm.validateSession(sid);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(sm.validateSession(sid));
    }
    state.SetLabel("GATE-AHP-04: p99 <= 1 ms");
}
BENCHMARK(BM_AHP05_SessionValidate)->Repetitions(kRepetitions)->ReportAggregatesOnly(true);

// ===========================================================================
// AHP-06 — DistributedTokenBlacklist::add() (RocksDB single-node)
// ===========================================================================

/**
 * @brief AHP-06: DistributedTokenBlacklist::add() — single RocksDB Put latency.
 *
 * Uses UseRealTime() because the RocksDB Put includes kernel I/O.
 * GATE-AHP-05: p99 ≤ 2 ms.
 */
class DistributedBlacklistFixture : public benchmark::Fixture {
public:
    fs::path db_path;
    std::unique_ptr<DistributedTokenBlacklist> bl;

    void SetUp(const ::benchmark::State &) override {
        db_path = fs::temp_directory_path()
                  / ("themis_bench_ahp_"
                     + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(db_path);
        DistributedBlacklistConfig cfg;
        cfg.db_path                = db_path.string();
        cfg.enable_cluster_sync    = false;
        cfg.purge_interval_seconds = 3600;
        cfg.sync_interval_seconds  = 3600;
        cfg.local_node.node_id     = "bench-node";
        bl = std::make_unique<DistributedTokenBlacklist>(cfg);
    }

    void TearDown(const ::benchmark::State &) override {
        bl.reset();
        fs::remove_all(db_path);
    }
};

BENCHMARK_DEFINE_F(DistributedBlacklistFixture, AHP06_DistributedAdd)(benchmark::State &state) {
    int counter = 0;
    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        bl->add("warm-" + std::to_string(i), futureExpiry());
    }
    for (auto _ : state) {
        bl->add("bench-" + std::to_string(counter++), futureExpiry());
    }
    state.SetLabel("GATE-AHP-05: p99 <= 2 ms");
}
BENCHMARK_REGISTER_F(DistributedBlacklistFixture, AHP06_DistributedAdd)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// AHP-07 — DistributedTokenBlacklist::isRevoked() (RocksDB warm cache)
// ===========================================================================

/**
 * @brief AHP-07: DistributedTokenBlacklist::isRevoked() — O(1) RocksDB point-read.
 *
 * Pre-warms the RocksDB block-cache before measurement.
 * GATE-AHP-06: p99 ≤ 1 µs (warm cache).
 */
BENCHMARK_DEFINE_F(DistributedBlacklistFixture, AHP07_DistributedIsRevoked)(benchmark::State &state) {
    // Pre-load entries
    for (int i = 0; i < kBlacklistPreloadSize; ++i) {
        bl->add(makeJti(i), futureExpiry());
    }
    // Warm RocksDB block cache
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)bl->isRevoked(makeJti(i % kBlacklistPreloadSize));
    }
    std::mt19937_64 rng(kAhpCanonicalSeed);
    std::uniform_int_distribution<int> dist(0, kBlacklistPreloadSize - 1);
    for (auto _ : state) {
        benchmark::DoNotOptimize(bl->isRevoked(makeJti(dist(rng))));
    }
    state.SetLabel("GATE-AHP-06: p99 <= 1 us warm");
}
BENCHMARK_REGISTER_F(DistributedBlacklistFixture, AHP07_DistributedIsRevoked)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// AHP-08 — Federation realm lookup overhead
// ===========================================================================

/**
 * @brief AHP-08: FederatedIdentityManager::realmCount() — mutex + hash-map overhead.
 *
 * Tests the synchronisation cost of the realm-map fast path with N registered
 * realms.  No network I/O; purely in-process cost.
 */
static void BM_AHP08_FederationRealmLookup(benchmark::State &state) {
    FederatedIdentityManager fed;
    const int n_realms = static_cast<int>(state.range(0));
    for (int i = 0; i < n_realms; ++i) {
        OIDCProviderConfig cfg;
        cfg.issuer_url = "https://idp-bench-" + std::to_string(i) + ".example.com/realm/r";
        cfg.client_id  = "themisdb";
        fed.addRealm(cfg);
    }
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)fed.realmCount();
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(fed.realmCount());
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("federation realm-count overhead");
}
BENCHMARK(BM_AHP08_FederationRealmLookup)->Arg(1)->Arg(5)->Arg(20)
    ->Repetitions(kRepetitions)->ReportAggregatesOnly(true);

} // namespace ahp
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
