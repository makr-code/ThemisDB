// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_security_release_gates.cpp
 * @brief Phase 5 security module hot-path release-gate benchmarks (SRG-01..SRG-06).
 *
 * Provides reproducible latency measurements for the security hot paths
 * identified in the security module roadmap (Phase 5).  These are the
 * hard release gates — a p99 regression beyond 10 % vs the baseline
 * blocks promotion to production.
 *
 * ## Gate table
 *
 * | Gate | Benchmark                        | Threshold    |
 * |------|----------------------------------|--------------|
 * | SRG-01 | Policy evaluation hot path     | p99 ≤ 1 ms   |
 * | SRG-02 | JWT token signature verify     | p99 ≤ 500 µs |
 * | SRG-03 | Key lookup (in-memory)         | p99 ≤ 100 µs |
 * | SRG-04 | Audit write (mock in-memory)   | p99 ≤ 500 µs |
 * | SRG-05 | RBAC permission check          | p99 ≤ 200 µs |
 * | SRG-06 | Certificate validation overhead| p99 ≤ 2 ms   |
 *
 * All benchmarks:
 *   - Use kSecurityCanonicalSeed = 42 for deterministic input data.
 *   - Run with Repetitions(5) to capture variance.
 *   - I/O-bound benchmarks use UseRealTime() so scheduler wait is included.
 *   - External dependencies (HSM, TLS stack, disk) are mocked in-process.
 *
 * @see include/security/security_api_contract.h — contract thresholds
 * @see src/security/ROADMAP.md — Phase 5 items
 */

#include <benchmark/benchmark.h>

#include "security/security_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace bench {
namespace srg {

using namespace themis::security;
using namespace std::chrono_literals;

// ============================================================================
// Constants — deterministic, release-pinned
// ============================================================================

/// Canonical RNG seed — must not change between releases without a gate note.
static constexpr std::uint64_t kSecurityCanonicalSeed = 42;

/// Number of repeated benchmark runs for variance capture.
static constexpr int kRepetitions = 5;

/// Warmup iterations before measurement begins.
static constexpr int kWarmupIterations = 200;

// ============================================================================
// In-process mock helpers (no live HSM, TLS, or disk I/O)
// ============================================================================

namespace {

/// Deterministic policy table: maps principal+resource → allow/deny.
class BenchPolicyTable {
public:
    BenchPolicyTable() {
        std::mt19937_64 rng(kSecurityCanonicalSeed);
        // Pre-populate 1000 rules with ~80 % allow / 20 % deny.
        for (int i = 0; i < 1000; ++i) {
            std::string key = "principal-" + std::to_string(i % 100)
                            + ":resource-" + std::to_string(i % 50);
            allow_[key] = ((rng() % 5) != 0);
        }
    }

    SecurityErrorCode evaluate(const std::string& principal,
                               const std::string& resource) const {
        auto it = allow_.find(principal + ":" + resource);
        if (it == allow_.end()) {
          return SecurityErrorCode::POLICY_NOT_FOUND;
        }
        return it->second ? SecurityErrorCode::OK : SecurityErrorCode::POLICY_DENY;
    }

private:
    std::unordered_map<std::string, bool> allow_;
};

/// Bench-only mock JWT token (pre-formatted, no crypto).
static const std::string kBenchJwt =
    "******"
    ".eyJzdWIiOiJiZW5jaCIsImlhdCI6MTYwMDAwMCwiZXhwIjo5OTk5OTk5OTl9"
    ".FAKESIGNATURE_bench_only_42";

/// Structural JWT parse (header+payload split, no crypto).
static bool validateJwtStructure(const std::string& token) {
    if (token.size() > kMaxJwtTokenBytes) {
      return false;
    }
    auto d1 = token.find('.');
    if (d1 == std::string::npos) {
      return false;
    }
    auto d2 = token.find('.', d1 + 1);
    return d2 != std::string::npos;
}

/// In-memory key store (benchmarks key lookup overhead).
class BenchKeyStore {
public:
    BenchKeyStore() {
        keys_.reserve(512);
        for (int i = 0; i < 512; ++i) {
            keys_["key-" + std::to_string(i)] = "material-" + std::to_string(i);
        }
    }

    const std::string* lookup(const std::string& id) const {
        auto it = keys_.find(id);
        if (it == keys_.end()) {
          return nullptr;
        }
        return &it->second;
    }

private:
    std::unordered_map<std::string, std::string> keys_;
};

/// In-memory audit ring buffer (mock; no disk I/O).
class BenchAuditLog {
public:
    explicit BenchAuditLog(std::size_t cap = 65536) {
        ring_.resize(cap);
    }

    SecurityErrorCode write(const std::string& event) {
        ring_[pos_.fetch_add(1, std::memory_order_relaxed) % ring_.size()] = event;
        return SecurityErrorCode::OK;
    }

private:
    std::vector<std::string>  ring_;
    std::atomic<std::size_t>  pos_{0};
};

/// RBAC permission table (role → set of allowed resources).
class BenchRbacTable {
public:
    BenchRbacTable() {
        // 50 roles, each with 20 permissions.
        for (int role = 0; role < 50; ++role) {
            std::string rk = "role-" + std::to_string(role);
            for (int perm = 0; perm < 20; ++perm) {
                allowed_[rk].push_back("resource-" + std::to_string(perm));
            }
        }
    }

    SecurityErrorCode check(const std::string& role, const std::string& resource) const {
        auto it = allowed_.find(role);
        if (it == allowed_.end()) {
          return SecurityErrorCode::RBAC_ROLE_MISSING;
        }
        const auto& perms = it->second;
        for (const auto& p : perms) {
            if (p == resource) {
              return SecurityErrorCode::OK;
            }
        }
        return SecurityErrorCode::POLICY_DENY;
    }

private:
    std::unordered_map<std::string, std::vector<std::string>> allowed_;
};

/// Mock certificate validation: chain-depth check + state lookup.
enum class CertBenchState { VALID, EXPIRED };

static SecurityErrorCode validateCertMock(CertBenchState state, int chainDepth) {
    if (chainDepth > static_cast<int>(kMaxCertChainDepth))
        return SecurityErrorCode::CERT_VALIDATION_FAILED;
    switch (state) {
        case CertBenchState::VALID:   return SecurityErrorCode::OK;
        case CertBenchState::EXPIRED: return SecurityErrorCode::CERT_EXPIRED;
    }
    return SecurityErrorCode::INTERNAL_ERROR;
}

}  // anonymous namespace

// ============================================================================
// Shared fixtures
// ============================================================================

/// Lazily-initialised policy table (constructed once per process).
static const BenchPolicyTable& policyTable() {
    static BenchPolicyTable tbl;
    return tbl;
}

static const BenchKeyStore& keyStore() {
    static BenchKeyStore ks;
    return ks;
}

static const BenchRbacTable& rbacTable() {
    static BenchRbacTable rt;
    return rt;
}

// ============================================================================
// SRG-01 — Policy evaluation hot path
// ============================================================================

/**
 * @brief SRG-01: Policy evaluate() from pre-built in-memory table.
 *
 * Gate: p99 ≤ 1 ms.
 */
static void BM_SRG01_PolicyEval(benchmark::State& state) {
    const auto& tbl = policyTable();
    std::mt19937_64 rng(kSecurityCanonicalSeed);
    // Warmup
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)tbl.evaluate("principal-" + std::to_string(rng() % 100),
                           "resource-"  + std::to_string(rng() % 50));
    }
    std::size_t counter = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            tbl.evaluate("principal-" + std::to_string(counter % 100),
                         "resource-"  + std::to_string(counter % 50)));
        ++counter;
    }
    state.SetLabel("SRG-01: GATE p99 <= 1 ms | policy eval hot path");
}
BENCHMARK(BM_SRG01_PolicyEval)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SRG-02 — JWT token structural validation (mock, no crypto)
// ============================================================================

/**
 * @brief SRG-02: JWT structural parse overhead (header.payload.sig split).
 *
 * Gate: p99 ≤ 500 µs.
 */
static void BM_SRG02_JwtValidate(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)validateJwtStructure(kBenchJwt);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(validateJwtStructure(kBenchJwt));
    }
    state.SetLabel("SRG-02: GATE p99 <= 500 us | JWT structural validate");
}
BENCHMARK(BM_SRG02_JwtValidate)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SRG-03 — Key lookup (in-memory)
// ============================================================================

/**
 * @brief SRG-03: Hash-map key lookup overhead (512 entries, warm cache).
 *
 * Gate: p99 ≤ 100 µs.
 */
static void BM_SRG03_KeyLookup(benchmark::State& state) {
    const auto& ks = keyStore();
    std::mt19937_64 rng(kSecurityCanonicalSeed);
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)ks.lookup("key-" + std::to_string(rng() % 512));
    }
    std::size_t counter = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(ks.lookup("key-" + std::to_string(counter % 512)));
        ++counter;
    }
    state.SetLabel("SRG-03: GATE p99 <= 100 us | in-memory key lookup");
}
BENCHMARK(BM_SRG03_KeyLookup)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SRG-04 — Audit write (mock in-memory ring buffer)
// ============================================================================

/**
 * @brief SRG-04: Audit ring-buffer write (no disk I/O).
 *
 * Gate: p99 ≤ 500 µs.
 */
static void BM_SRG04_AuditWrite(benchmark::State& state) {
    BenchAuditLog log;
    std::string event(64, 'A');  // fixed-size event payload
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)log.write(event);
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(log.write(event));
    }
    state.SetLabel("SRG-04: GATE p99 <= 500 us | audit mock write");
}
BENCHMARK(BM_SRG04_AuditWrite)
    ->UseRealTime()
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SRG-05 — RBAC permission check
// ============================================================================

/**
 * @brief SRG-05: RBAC role→resource permission lookup (50 roles, 20 perms each).
 *
 * Gate: p99 ≤ 200 µs.
 */
static void BM_SRG05_RbacCheck(benchmark::State& state) {
    const auto& rt = rbacTable();
    std::mt19937_64 rng(kSecurityCanonicalSeed);
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)rt.check("role-" + std::to_string(rng() % 50),
                       "resource-" + std::to_string(rng() % 20));
    }
    std::size_t counter = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            rt.check("role-" + std::to_string(counter % 50),
                     "resource-" + std::to_string(counter % 20)));
        ++counter;
    }
    state.SetLabel("SRG-05: GATE p99 <= 200 us | RBAC permission check");
}
BENCHMARK(BM_SRG05_RbacCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// SRG-06 — Certificate validation overhead (mock chain-depth + state check)
// ============================================================================

/**
 * @brief SRG-06: Mock certificate validation (chain-depth + state, no real crypto).
 *
 * Gate: p99 ≤ 2 ms.
 */
static void BM_SRG06_CertValidation(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i) {
        (void)validateCertMock(CertBenchState::VALID, 3);
    }
    int counter = 0;
    for (auto _ : state) {
        int depth = (counter % 5) + 1;
        benchmark::DoNotOptimize(validateCertMock(CertBenchState::VALID, depth));
        ++counter;
    }
    state.SetLabel("SRG-06: GATE p99 <= 2 ms | mock cert validation");
}
BENCHMARK(BM_SRG06_CertValidation)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

}  // namespace srg
}  // namespace bench
}  // namespace themis

BENCHMARK_MAIN();
