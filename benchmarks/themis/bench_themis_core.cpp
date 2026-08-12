#include <benchmark/benchmark.h>

#include "themis/build_info.h"
#include "themis/edition.h"
#include "themis/license_info.h"
#include "themis/module_hash_verifier.h"
#include "themis/network/wire_protocol_server.hpp"

#include <array>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

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
constexpr int kDefaultSocketDispatchChannels = 64;

/**
 * Parse boolean environment variable.
 * Truthy values: "1", "true", "TRUE", "on", "ON"; otherwise false/default.
 */
[[nodiscard]] bool parseBoolEnv(const char* name, bool default_value) {
    const char* raw = std::getenv(name);
    if (raw == nullptr || raw[0] == '\0') {
        return default_value;
    }
    return std::string_view(raw) == "1" || std::string_view(raw) == "true" ||
           std::string_view(raw) == "TRUE" || std::string_view(raw) == "on" ||
           std::string_view(raw) == "ON";
}

/**
 * Parse integer environment variable with clamping to [min_value, max_value].
 * Returns @p default_value for missing or malformed values.
 */
[[nodiscard]] int parseIntEnvBounded(const char* name,
                                     int default_value,
                                     int min_value,
                                     int max_value) {
    const char* raw = std::getenv(name);
    if (raw == nullptr || raw[0] == '\0') {
        return default_value;
    }
    char* end_ptr = nullptr;
    const long parsed = std::strtol(raw, &end_ptr, 10);
    if (end_ptr == raw || *end_ptr != '\0') {
        return default_value;
    }
    if (parsed < min_value) {
        return min_value;
    }
    if (parsed > max_value) {
        return max_value;
    }
    return static_cast<int>(parsed);
}

/**
 * UDP loopback transport harness for benchmark-only opcode dispatch.
 * Uses multiple sender sockets (channel fanout) feeding one local receiver socket.
 */
class LoopbackOpcodeHarness {
public:
    explicit LoopbackOpcodeHarness(int channels)
        : receiver_(io_context_,
                    boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::loopback(), 0)),
          receiver_endpoint_(receiver_.local_endpoint()) {
        senders_.reserve(static_cast<std::size_t>(channels));
        for (int i = 0; i < channels; ++i) {
            senders_.push_back(std::make_unique<boost::asio::ip::udp::socket>(
                io_context_, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)));
        }
    }

    /**
     * Dispatch one opcode byte over loopback.
     * Session index is mapped to sender channel via modulo.
     * Throws on unexpected receive size.
     */
    [[nodiscard]] uint8_t dispatch(uint8_t opcode, std::size_t session_index) {
        const auto sender_count = senders_.size();
        auto& sender = *senders_[session_index % sender_count];
        sender.send_to(boost::asio::buffer(&opcode, 1), receiver_endpoint_);

        std::array<uint8_t, 1> inbound{};
        boost::asio::ip::udp::endpoint remote;
        const auto received = receiver_.receive_from(boost::asio::buffer(inbound), remote);
        if (received != 1) {
            throw std::runtime_error("loopback dispatch expected 1 byte, received " +
                                     std::to_string(received) + " for opcode " +
                                     std::to_string(static_cast<unsigned>(opcode)));
        }
        return inbound[0];
    }

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::udp::socket receiver_;
    boost::asio::ip::udp::endpoint receiver_endpoint_;
    std::vector<std::unique_ptr<boost::asio::ip::udp::socket>> senders_;
};

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
// Purpose: Deterministic benchmark for 10k session opcode dispatch with optional real
//   loopback socket transport to include kernel network-stack overhead.
// Activation: `THEMIS_BENCH_WIRE_USE_SOCKET_HARNESS=1` enables socket-backed mode;
//   synthetic in-process dispatch remains default.
// Production Delta: Socket mode uses loopback UDP channel fanout (`THEMIS_BENCH_WIRE_SOCKET_CHANNELS`)
//   instead of full production session/TLS handshake and request pipeline.
// Removal Plan: Keep as benchmark harness with explicit synthetic/socket modes.
void BM_WireServer_ConcurrentSessions_10k(benchmark::State& state) {
    constexpr int kConcurrentSessions = 10000;
    const bool use_socket_harness = parseBoolEnv("THEMIS_BENCH_WIRE_USE_SOCKET_HARNESS", false);
    const int socket_channels = parseIntEnvBounded("THEMIS_BENCH_WIRE_SOCKET_CHANNELS",
                                                   kDefaultSocketDispatchChannels, 1, 1024);
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

    std::unique_ptr<LoopbackOpcodeHarness> socket_harness;
    if (use_socket_harness) {
        try {
            socket_harness = std::make_unique<LoopbackOpcodeHarness>(socket_channels);
        } catch (const std::exception& e) {
            state.SkipWithError(e.what());
            return;
        }
    }

    std::array<uint64_t, 5> counters{};
    for (auto _ : state) {
        std::uint32_t lcg_state = kOpcodeLcgSeed;
        for (int session = 0; session < kConcurrentSessions; ++session) {
            lcg_state = lcg_state * kOpcodeLcgMultiplier + kOpcodeLcgIncrement;
            const std::size_t opcode_index =
                (static_cast<std::uint64_t>(lcg_state) * opcodes.size()) >> 32;
            uint8_t opcode_byte = static_cast<uint8_t>(opcodes[opcode_index]);
            if (socket_harness) {
                opcode_byte = socket_harness->dispatch(opcode_byte,
                                                       static_cast<std::size_t>(session));
            }
            switch (opcode_byte) {
                case static_cast<uint8_t>(themis::wire::OpCode::OP_GET):
                    ++counters[kGetCounter];
                    break;
                case static_cast<uint8_t>(themis::wire::OpCode::OP_PUT):
                    ++counters[kPutCounter];
                    break;
                case static_cast<uint8_t>(themis::wire::OpCode::OP_QUERY_AQL):
                    ++counters[kQueryCounter];
                    break;
                case static_cast<uint8_t>(themis::wire::OpCode::OP_PING):
                    ++counters[kPingCounter];
                    break;
                default:
                    ++counters[kOtherCounter];
                    break;
            }
        }
        benchmark::DoNotOptimize(counters.data());
    }

    state.SetItemsProcessed(state.iterations() * kConcurrentSessions);
    if (socket_harness) {
        state.SetLabel("mode=socket-loopback, channels=" + std::to_string(socket_channels));
    } else {
        state.SetLabel("mode=synthetic-switch");
    }
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
