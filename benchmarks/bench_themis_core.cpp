/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_themis_core.cpp                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-22                                         ║
  Author:          Copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
*/

#include <benchmark/benchmark.h>

#include "themis/build_info.h"
#include "themis/edition.h"
#include "themis/license_info.h"
#include "themis/module_hash_verifier.h"
#include "themis/network/wire_protocol_server.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

#ifndef BENCHMARK_UNIT
#define BENCHMARK_UNIT(unit) ->Unit(unit)
#endif

namespace {

constexpr std::string_view kHotPathFeature = "rbac";
// Source of truth for feature names: include/themis/edition.h::kGatedFeatureNames.
// Base64 for 64 zero-bytes (intentionally invalid signature payload).
// Expected decoded size: 64 bytes.
constexpr std::string_view kZeroByteBenchmarkSignatureBase64 =
    "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==";
constexpr std::uint32_t kOpcodeLcgSeed = 0xC0FFEEu;
constexpr std::uint32_t kOpcodeLcgMultiplier = 1664525u;      // Numerical Recipes LCG
constexpr std::uint32_t kOpcodeLcgIncrement = 1013904223u;    // Numerical Recipes LCG

constexpr std::size_t decodedLengthFromBase64Length(std::size_t encoded_len,
                                                    std::size_t padding_chars) {
    return (encoded_len / 4) * 3 - padding_chars;
}

constexpr std::size_t trailingPaddingCount(std::string_view base64) {
    return (!base64.empty() && base64.back() == '=') +
           (base64.size() > 1 && base64[base64.size() - 2] == '=');
}

static_assert(decodedLengthFromBase64Length(
                  kZeroByteBenchmarkSignatureBase64.size(),
                  trailingPaddingCount(kZeroByteBenchmarkSignatureBase64)) == 64,
              "Benchmark signature payload must decode to 64 bytes");

std::string resolveModulePathForLoadBenchmark() {
#ifdef _WIN32
    return "C:\\Windows\\System32\\kernel32.dll";
#else
    constexpr std::array<const char*, 4> candidates = {
        "/lib/x86_64-linux-gnu/libm.so.6",
        "/usr/lib/x86_64-linux-gnu/libm.so.6",
        "/lib64/libm.so.6",
        "/usr/lib64/libm.so.6"
    };
    for (const auto* path : candidates) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return candidates.front();
#endif
}

void BM_ModuleLoad_WithHashVerify(benchmark::State& state) {
    const std::string module_path = resolveModulePathForLoadBenchmark();
    const std::string expected_hash = themis::modules::ModuleHashVerifier::computeSHA256(module_path);

    if (expected_hash.empty()) {
        state.SkipWithError("Unable to hash benchmark module file");
        return;
    }

    for (auto _ : state) {
        const std::string computed_hash =
            themis::modules::ModuleHashVerifier::computeSHA256(module_path);
        benchmark::DoNotOptimize(computed_hash == expected_hash);

#ifdef _WIN32
        HMODULE handle = LoadLibraryA(module_path.c_str());
        if (handle != nullptr) {
            FreeLibrary(handle);
        }
#else
        void* handle = dlopen(module_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (handle != nullptr) {
            dlclose(handle);
        }
#endif
    }
}

void BM_GetBuildConfiguration_Cold(benchmark::State& state) {
    for (auto _ : state) {
        auto config = themis::build_info::getBuildConfiguration();
        benchmark::DoNotOptimize(config.modules.size());
        benchmark::ClobberMemory();
    }
}

void BM_GetBuildConfiguration_Warm(benchmark::State& state) {
    auto warmup = themis::build_info::getBuildConfiguration();
    benchmark::DoNotOptimize(warmup.modules.size());

    for (auto _ : state) {
        auto config = themis::build_info::getBuildConfiguration();
        benchmark::DoNotOptimize(config.modules.size());
    }
}

void BM_LicenseValidation_Ed25519(benchmark::State& state) {
    themis::license::LicenseData license;
    license.license_key = "THEMIS-BENCH-LICENSE-KEY";
    license.organization_name = "BenchmarkOrg";
    license.organization_id = "bench-org-id";
    license.issued_date = "2026-01-01";
    license.expiry_date = "2027-01-01";
    license.max_nodes = 1;
    license.max_cores = 4;
    license.max_storage_tb = 1;
    license.edition = "COMMUNITY";
    // Deterministic invalid signature payload (base64 for 64 zero-bytes).
    // This benchmark measures Ed25519 signature verification-path overhead
    // (including base64 decode + signature verify) on a non-HSM setup.
    license.signature = std::string(kZeroByteBenchmarkSignatureBase64);

    for (auto _ : state) {
        bool is_valid = themis::license::verifyLicenseSignature(license);
        benchmark::DoNotOptimize(is_valid);
    }
}

// STUB/SIMULATION NOTE:
// Purpose: Benchmark opcode-dispatch hot path for 10k concurrent sessions without opening 10k real sockets.
// Activation: Always active inside benchmark-only binary bench_themis_core.
// Production Delta: Uses synthetic session loop and in-process switch dispatch instead of live network I/O.
// Removal Plan: Replace with socket-backed benchmark once deterministic 10k-session harness is available in CI.
void BM_WireServer_ConcurrentSessions_10k(benchmark::State& state) {
    constexpr int kConcurrentSessions = 10000;
    enum CounterIndex : std::size_t {
        kGetCounter = 0,
        kPutCounter,
        kQueryCounter,
        kPingCounter,
        kOtherCounter,
    };
    const std::array<themis::wire::OpCode, 5> opcodes = {
        themis::wire::OpCode::OP_GET,
        themis::wire::OpCode::OP_PUT,
        themis::wire::OpCode::OP_QUERY_AQL,
        themis::wire::OpCode::OP_PING,
        themis::wire::OpCode::OP_OK
    };

    std::array<uint64_t, 5> counters{};
    for (auto _ : state) {
        std::uint32_t lcg_state = kOpcodeLcgSeed;
        for (int session = 0; session < kConcurrentSessions; ++session) {
            lcg_state = lcg_state * kOpcodeLcgMultiplier + kOpcodeLcgIncrement;
            const std::size_t opcode_index =
                (static_cast<std::uint64_t>(lcg_state) * opcodes.size()) >> 32;
            const auto opcode = opcodes[opcode_index];
            switch (opcode) {
                case themis::wire::OpCode::OP_GET:       ++counters[kGetCounter]; break;
                case themis::wire::OpCode::OP_PUT:       ++counters[kPutCounter]; break;
                case themis::wire::OpCode::OP_QUERY_AQL: ++counters[kQueryCounter]; break;
                case themis::wire::OpCode::OP_PING:      ++counters[kPingCounter]; break;
                default:                                 ++counters[kOtherCounter]; break;
            }
        }
        benchmark::DoNotOptimize(counters.data());
    }

    state.SetItemsProcessed(state.iterations() * kConcurrentSessions);
}

void BM_EditionManager_IsFeatureEnabled_HotPath(benchmark::State& state) {
    // "rbac" is a stable gated feature name in themis::edition::kGatedFeatureNames
    // and represents a typical hot-path feature-gate check.
    // Note: IsFeatureEnabled uses the project API name from include/themis/edition.h.
    for (auto _ : state) {
        const bool enabled = themis::edition::IsFeatureEnabled(kHotPathFeature);
        benchmark::DoNotOptimize(enabled);
    }
}

} // namespace

BENCHMARK(BM_ModuleLoad_WithHashVerify)
    BENCHMARK_UNIT(benchmark::kMicrosecond)
    ->Iterations(300);

BENCHMARK(BM_GetBuildConfiguration_Cold)
    BENCHMARK_UNIT(benchmark::kMicrosecond)
    ->Iterations(2000);

BENCHMARK(BM_GetBuildConfiguration_Warm)
    BENCHMARK_UNIT(benchmark::kMicrosecond)
    ->Iterations(20000);

BENCHMARK(BM_LicenseValidation_Ed25519)
    BENCHMARK_UNIT(benchmark::kMicrosecond)
    ->Iterations(2000);

BENCHMARK(BM_WireServer_ConcurrentSessions_10k)
    BENCHMARK_UNIT(benchmark::kMicrosecond)
    ->Iterations(200);

BENCHMARK(BM_EditionManager_IsFeatureEnabled_HotPath)
    BENCHMARK_UNIT(benchmark::kMicrosecond)
    ->Iterations(2000000);
