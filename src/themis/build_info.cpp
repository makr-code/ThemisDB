/**
 * @file build_info.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=28, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB Build Information Implementation
 * ==========================================
 * Collects and formats compile-time configuration information.
 *
 * Migrated from src/utils/build_info.cpp to src/themis/build_info.cpp
 * as part of the v1.7.0 Themis core module consolidation.
 */

#include "themis/build_info.h"
#include <stdexcept>
#include "themis/edition.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <map>
#include <mutex>

#ifndef THEMIS_BUILD_UUID
#define THEMIS_BUILD_UUID "unknown"
#endif

#ifndef THEMIS_BUILD_VERSION_STRING
#define THEMIS_BUILD_VERSION_STRING THEMIS_VERSION_STRING " (" THEMIS_BUILD_UUID ")"
#endif

// Platform headers for executable path
#if defined(__linux__)
#  include <unistd.h>
#elif defined(_WIN32)
#  include <windows.h>
#endif

// ── Optional SHA-256 for binary_hash (OpenSSL) ──────────────────────────────
#if __has_include(<openssl/sha.h>)
#  include <openssl/evp.h>
#  define THEMIS_HAVE_OPENSSL_SHA 1
#endif

namespace themis {
namespace build_info {

// ============================================================================
// HSM MODULE STATUS BRIDGE – storage (STUB #95)
// ============================================================================
// Defined before getBuildConfiguration() so the #else HSM branch can call it.

namespace {

std::mutex& hsmStatusFnMutex() {
    static std::mutex m;
    return m;
}

HsmModuleStatusFn& hsmStatusFnStorage() {
    static HsmModuleStatusFn fn;
    return fn;
}

} // anonymous namespace

BuildConfiguration getBuildConfiguration() {
    BuildConfiguration config;
    
    // ========================================================================
    // EDITION INFORMATION
    // ========================================================================
    const auto edition_info = edition::EditionInfo::Get();
    config.edition_name = std::string(edition_info.name);
    
    switch (edition_info.type) {
        case edition::EditionType::MINIMAL:
            config.edition_type = THEMIS_EDITION_STRING;
            break;
        case edition::EditionType::COMMUNITY:
            config.edition_type = THEMIS_EDITION_STRING;
            break;
        case edition::EditionType::ENTERPRISE:
            config.edition_type = THEMIS_EDITION_STRING;
            break;
        case edition::EditionType::MILITARY:
            config.edition_type = THEMIS_EDITION_STRING;
            break;
        case edition::EditionType::HYPERSCALER:
            config.edition_type = THEMIS_EDITION_STRING;
            break;
        default:
            config.edition_type = "Unknown";
            break;
    }
    
    config.gpu_max_vram_gb = edition_info.gpu_max_vram_gb;
    config.sharding_max_nodes = edition_info.sharding_max_nodes;
    
    // ========================================================================
    // COMPILER INFORMATION
    // ========================================================================
#if defined(__clang__)
    config.compiler = "Clang";
    config.compiler_version = std::to_string(__clang_major__) + "." + 
                              std::to_string(__clang_minor__) + "." +
                              std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
    config.compiler = "GCC";
    config.compiler_version = std::to_string(__GNUC__) + "." + 
                              std::to_string(__GNUC_MINOR__) + "." +
                              std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    config.compiler = "MSVC";
    config.compiler_version = std::to_string(_MSC_VER);
#else
    config.compiler = "Unknown";
    config.compiler_version = "Unknown";
#endif

#ifdef NDEBUG
    config.build_type = "Release";
#else
    config.build_type = "Debug";
#endif

#ifdef THEMIS_BUILD_TIMESTAMP
    config.build_timestamp = THEMIS_BUILD_TIMESTAMP;
#else
    config.build_timestamp = __DATE__ " " __TIME__;
#endif

    // ========================================================================
    // MODULE COMPILATION STATUS
    // ========================================================================
    
    // Core Modules
    config.modules.push_back({
        "Storage Engine",
        true, // Always compiled in
        true,
        "RocksDB-based storage with MVCC and transactions"
    });
    
    config.modules.push_back({
        "Vector Index",
        true, // Always compiled in
        true,
        "HNSW vector search with optional GPU acceleration"
    });
    
    config.modules.push_back({
        "Graph Index",
        true, // Always compiled in
        true,
        "Graph analytics and traversal"
    });
    
    config.modules.push_back({
        "Secondary Index",
        true, // Always compiled in
        true,
        "Fast secondary index lookups"
    });
    
    // Optional Modules - GPU Acceleration
#ifdef THEMIS_ENABLE_GPU
    config.modules.push_back({
        "GPU Acceleration (Generic)",
        true,
        true,
        "GPU acceleration for vector operations"
    });
#else
    config.modules.push_back({
        "GPU Acceleration (Generic)",
        false,
        false,
        "GPU acceleration for vector operations"
    });
#endif

#ifdef THEMIS_ENABLE_CUDA
    config.modules.push_back({
        "CUDA Backend",
        true,
        true,
        "NVIDIA CUDA acceleration"
    });
#else
    config.modules.push_back({
        "CUDA Backend",
        false,
        false,
        "NVIDIA CUDA acceleration"
    });
#endif

#ifdef THEMIS_ENABLE_HIP
    config.modules.push_back({
        "HIP Backend",
        true,
        true,
        "AMD HIP/ROCm acceleration"
    });
#else
    config.modules.push_back({
        "HIP Backend",
        false,
        false,
        "AMD HIP/ROCm acceleration"
    });
#endif

#ifdef THEMIS_ENABLE_OPENCL
    config.modules.push_back({
        "OpenCL Backend",
        true,
        true,
        "Cross-platform OpenCL acceleration"
    });
#else
    config.modules.push_back({
        "OpenCL Backend",
        false,
        false,
        "Cross-platform OpenCL acceleration"
    });
#endif

#ifdef THEMIS_ENABLE_VULKAN
    config.modules.push_back({
        "Vulkan Compute",
        true,
        true,
        "Vulkan compute shader acceleration"
    });
#else
    config.modules.push_back({
        "Vulkan Compute",
        false,
        false,
        "Vulkan compute shader acceleration"
    });
#endif

#ifdef THEMIS_ENABLE_DIRECTX
    config.modules.push_back({
        "DirectX Compute",
        true,
        true,
        "DirectX 12 compute shader acceleration (Windows)"
    });
#else
    config.modules.push_back({
        "DirectX Compute",
        false,
        false,
        "DirectX 12 compute shader acceleration (Windows)"
    });
#endif

#ifdef THEMIS_ENABLE_METAL
    config.modules.push_back({
        "Metal Compute",
        true,
        true,
        "Apple Metal acceleration (macOS/iOS)"
    });
#else
    config.modules.push_back({
        "Metal Compute",
        false,
        false,
        "Apple Metal acceleration (macOS/iOS)"
    });
#endif

#ifdef THEMIS_ENABLE_ONEAPI
    config.modules.push_back({
        "OneAPI/SYCL",
        true,
        true,
        "Intel OneAPI/SYCL acceleration"
    });
#else
    config.modules.push_back({
        "OneAPI/SYCL",
        false,
        false,
        "Intel OneAPI/SYCL acceleration"
    });
#endif

    // LLM and Voice
#ifdef THEMIS_ENABLE_LLM
    config.modules.push_back({
        "LLM Integration",
        true,
        true,
        "Large Language Model integration (llama.cpp)"
    });
#else
    config.modules.push_back({
        "LLM Integration",
        false,
        false,
        "Large Language Model integration (llama.cpp)"
    });
#endif

#ifdef THEMIS_ENABLE_VOICE_ASSISTANT
    config.modules.push_back({
        "Voice Assistant",
        true,
        true,
        "Speech-to-Text and Text-to-Speech"
    });
#else
    config.modules.push_back({
        "Voice Assistant",
        false,
        false,
        "Speech-to-Text and Text-to-Speech"
    });
#endif

#ifdef THEMIS_ENABLE_WHISPER
    config.modules.push_back({
        "Whisper STT",
        true,
        true,
        "Whisper.cpp Speech-to-Text engine"
    });
#else
    config.modules.push_back({
        "Whisper STT",
        false,
        false,
        "Whisper.cpp Speech-to-Text engine"
    });
#endif

#ifdef THEMIS_ENABLE_PIPER_TTS
    config.modules.push_back({
        "Piper TTS",
        true,
        true,
        "Piper Text-to-Speech engine"
    });
#else
    config.modules.push_back({
        "Piper TTS",
        false,
        false,
        "Piper Text-to-Speech engine"
    });
#endif

    // Content Processors
#ifdef THEMIS_ENABLE_CONTENT_PROCESSORS
    config.modules.push_back({
        "Content Processors",
        true,
        true,
        "Audio, image, video, geo, CAD file processing"
    });
#else
    config.modules.push_back({
        "Content Processors",
        false,
        false,
        "Audio, image, video, geo, CAD file processing"
    });
#endif

    // Protocols
#ifdef THEMIS_ENABLE_HTTP2
    config.modules.push_back({
        "HTTP/2 Protocol",
        true,
        true,
        "HTTP/2 with server push"
    });
#else
    config.modules.push_back({
        "HTTP/2 Protocol",
        false,
        false,
        "HTTP/2 with server push"
    });
#endif

#ifdef THEMIS_ENABLE_HTTP3
    config.modules.push_back({
        "HTTP/3 Protocol",
        true,
        true,
        "HTTP/3 over QUIC"
    });
#else
    config.modules.push_back({
        "HTTP/3 Protocol",
        false,
        false,
        "HTTP/3 over QUIC"
    });
#endif

#ifdef THEMIS_ENABLE_GRPC
    config.modules.push_back({
        "gRPC Protocol",
        true,
        true,
        "gRPC for inter-shard communication"
    });
#else
    config.modules.push_back({
        "gRPC Protocol",
        false,
        false,
        "gRPC for inter-shard communication"
    });
#endif

#ifdef THEMIS_ENABLE_WEBSOCKET
    config.modules.push_back({
        "WebSocket Protocol",
        true,
        true,
        "WebSocket for real-time streaming"
    });
#else
    config.modules.push_back({
        "WebSocket Protocol",
        false,
        false,
        "WebSocket for real-time streaming"
    });
#endif

    // Performance Optimizations
#ifdef THEMIS_ENABLE_MIMALLOC
    config.modules.push_back({
        "mimalloc Allocator",
        true,
        true,
        "High-performance memory allocator (+20-40% boost)"
    });
#else
    config.modules.push_back({
        "mimalloc Allocator",
        false,
        false,
        "High-performance memory allocator (+20-40% boost)"
    });
#endif

#ifdef THEMIS_ENABLE_HUGE_PAGES
    config.modules.push_back({
        "Huge Pages",
        true,
        true,
        "Large page support for +15-30% memory performance"
    });
#else
    config.modules.push_back({
        "Huge Pages",
        false,
        false,
        "Large page support for +15-30% memory performance"
    });
#endif

#ifdef THEMIS_ENABLE_RCU_INDEX
    config.modules.push_back({
        "RCU Index",
        true,
        true,
        "Read-Copy-Update for lock-free reads (+200-500%)"
    });
#else
    config.modules.push_back({
        "RCU Index",
        false,
        false,
        "Read-Copy-Update for lock-free reads (+200-500%)"
    });
#endif

    // Advanced Storage Features
#ifdef THEMIS_ENABLE_LIRS_CACHE
    config.modules.push_back({
        "LIRS Cache",
        true,
        true,
        "Low Inter-reference Recency Set cache (+30-40% hit rate)"
    });
#else
    config.modules.push_back({
        "LIRS Cache",
        false,
        false,
        "Low Inter-reference Recency Set cache (+30-40% hit rate)"
    });
#endif

#ifdef THEMIS_ENABLE_WISCKEY
    config.modules.push_back({
        "WiscKey Separation",
        true,
        true,
        "Key/value separation for faster writes (+40-60%)"
    });
#else
    config.modules.push_back({
        "WiscKey Separation",
        false,
        false,
        "Key/value separation for faster writes (+40-60%)"
    });
#endif

#ifdef THEMIS_ENABLE_RABITQ
    config.modules.push_back({
        "RaBitQ Quantization",
        true,
        true,
        "Vector quantization for 16x memory reduction"
    });
#else
    config.modules.push_back({
        "RaBitQ Quantization",
        false,
        false,
        "Vector quantization for 16x memory reduction"
    });
#endif

#ifdef THEMIS_ENABLE_DISKANN
    config.modules.push_back({
        "DiskANN",
        true,
        true,
        "Billion-scale disk-based vector search (+300-400%)"
    });
#else
    config.modules.push_back({
        "DiskANN",
        false,
        false,
        "Billion-scale disk-based vector search (+300-400%)"
    });
#endif

    // Security Features
#ifdef THEMIS_ENABLE_HSM_REAL
    config.modules.push_back({
        "HSM PKCS#11",
        true,
        true,
        "Hardware Security Module integration"
    });
#else
    // STUB #95: Consult the runtime bridge when available so the server can
    // report the actual HSM KEK state (stub vs. injected hardware backend).
    // Default (no bridge set): report not-compiled-in with stub annotation.
    {
        bool is_real_hsm = false;
        std::string desc = "Hardware Security Module integration (software stub – dev only)";
        HsmModuleStatusFn fn_copy;
        {
            std::lock_guard<std::mutex> lk(hsmStatusFnMutex());
            fn_copy = hsmStatusFnStorage();
        }
        if (fn_copy) {
            try {
                auto [hsm_active, bridge_desc] = fn_copy();
                is_real_hsm = hsm_active;
                desc = bridge_desc;
            } catch (...) {
                // Bridge failure → keep static defaults
            }
        }
        config.modules.push_back({
            "HSM PKCS#11",
            is_real_hsm,
            is_real_hsm,
            desc
        });
    }
#endif

    // Tracing and Observability
#ifdef THEMIS_ENABLE_TRACING
    config.modules.push_back({
        "OpenTelemetry Tracing",
        true,
        true,
        "Distributed tracing and observability"
    });
#else
    config.modules.push_back({
        "OpenTelemetry Tracing",
        false,
        false,
        "Distributed tracing and observability"
    });
#endif

    // ========================================================================
    // COMPILE FLAGS SUMMARY
    // ========================================================================
    
    // Collect all THEMIS_* compile-time flags
    config.compile_flags = {
#ifdef THEMIS_ENABLE_GPU
        { "THEMIS_ENABLE_GPU", true },
#else
        { "THEMIS_ENABLE_GPU", false },
#endif
#ifdef THEMIS_ENABLE_CUDA
        { "THEMIS_ENABLE_CUDA", true },
#else
        { "THEMIS_ENABLE_CUDA", false },
#endif
#ifdef THEMIS_ENABLE_HIP
        { "THEMIS_ENABLE_HIP", true },
#else
        { "THEMIS_ENABLE_HIP", false },
#endif
#ifdef THEMIS_ENABLE_OPENCL
        { "THEMIS_ENABLE_OPENCL", true },
#else
        { "THEMIS_ENABLE_OPENCL", false },
#endif
#ifdef THEMIS_ENABLE_VULKAN
        { "THEMIS_ENABLE_VULKAN", true },
#else
        { "THEMIS_ENABLE_VULKAN", false },
#endif
#ifdef THEMIS_ENABLE_DIRECTX
        { "THEMIS_ENABLE_DIRECTX", true },
#else
        { "THEMIS_ENABLE_DIRECTX", false },
#endif
#ifdef THEMIS_ENABLE_METAL
        { "THEMIS_ENABLE_METAL", true },
#else
        { "THEMIS_ENABLE_METAL", false },
#endif
#ifdef THEMIS_ENABLE_ONEAPI
        { "THEMIS_ENABLE_ONEAPI", true },
#else
        { "THEMIS_ENABLE_ONEAPI", false },
#endif
#ifdef THEMIS_ENABLE_LLM
        { "THEMIS_ENABLE_LLM", true },
#else
        { "THEMIS_ENABLE_LLM", false },
#endif
#ifdef THEMIS_ENABLE_VOICE_ASSISTANT
        { "THEMIS_ENABLE_VOICE_ASSISTANT", true },
#else
        { "THEMIS_ENABLE_VOICE_ASSISTANT", false },
#endif
#ifdef THEMIS_ENABLE_WHISPER
        { "THEMIS_ENABLE_WHISPER", true },
#else
        { "THEMIS_ENABLE_WHISPER", false },
#endif
#ifdef THEMIS_ENABLE_PIPER_TTS
        { "THEMIS_ENABLE_PIPER_TTS", true },
#else
        { "THEMIS_ENABLE_PIPER_TTS", false },
#endif
#ifdef THEMIS_ENABLE_CONTENT_PROCESSORS
        { "THEMIS_ENABLE_CONTENT_PROCESSORS", true },
#else
        { "THEMIS_ENABLE_CONTENT_PROCESSORS", false },
#endif
#ifdef THEMIS_ENABLE_HTTP2
        { "THEMIS_ENABLE_HTTP2", true },
#else
        { "THEMIS_ENABLE_HTTP2", false },
#endif
#ifdef THEMIS_ENABLE_HTTP3
        { "THEMIS_ENABLE_HTTP3", true },
#else
        { "THEMIS_ENABLE_HTTP3", false },
#endif
#ifdef THEMIS_ENABLE_GRPC
        { "THEMIS_ENABLE_GRPC", true },
#else
        { "THEMIS_ENABLE_GRPC", false },
#endif
#ifdef THEMIS_ENABLE_WEBSOCKET
        { "THEMIS_ENABLE_WEBSOCKET", true },
#else
        { "THEMIS_ENABLE_WEBSOCKET", false },
#endif
#ifdef THEMIS_ENABLE_MIMALLOC
        { "THEMIS_ENABLE_MIMALLOC", true },
#else
        { "THEMIS_ENABLE_MIMALLOC", false },
#endif
#ifdef THEMIS_ENABLE_HUGE_PAGES
        { "THEMIS_ENABLE_HUGE_PAGES", true },
#else
        { "THEMIS_ENABLE_HUGE_PAGES", false },
#endif
#ifdef THEMIS_ENABLE_RCU_INDEX
        { "THEMIS_ENABLE_RCU_INDEX", true },
#else
        { "THEMIS_ENABLE_RCU_INDEX", false },
#endif
#ifdef THEMIS_ENABLE_LIRS_CACHE
        { "THEMIS_ENABLE_LIRS_CACHE", true },
#else
        { "THEMIS_ENABLE_LIRS_CACHE", false },
#endif
#ifdef THEMIS_ENABLE_WISCKEY
        { "THEMIS_ENABLE_WISCKEY", true },
#else
        { "THEMIS_ENABLE_WISCKEY", false },
#endif
#ifdef THEMIS_ENABLE_RABITQ
        { "THEMIS_ENABLE_RABITQ", true },
#else
        { "THEMIS_ENABLE_RABITQ", false },
#endif
#ifdef THEMIS_ENABLE_DISKANN
        { "THEMIS_ENABLE_DISKANN", true },
#else
        { "THEMIS_ENABLE_DISKANN", false },
#endif
#ifdef THEMIS_ENABLE_HSM_REAL
        { "THEMIS_ENABLE_HSM_REAL", true },
#else
        { "THEMIS_ENABLE_HSM_REAL", false },
#endif
#ifdef THEMIS_ENABLE_TRACING
        { "THEMIS_ENABLE_TRACING", true },
#else
        { "THEMIS_ENABLE_TRACING", false },
#endif
#ifdef THEMIS_ENABLE_ASAN
        { "THEMIS_ENABLE_ASAN", true },
#else
        { "THEMIS_ENABLE_ASAN", false },
#endif
#ifdef THEMIS_STRICT_BUILD
        { "THEMIS_STRICT_BUILD", true },
#else
        { "THEMIS_STRICT_BUILD", false },
#endif
#ifdef THEMIS_ENABLE_AVX2
        { "THEMIS_ENABLE_AVX2", true },
#else
        { "THEMIS_ENABLE_AVX2", false },
#endif
#ifdef THEMIS_QNAP_BUILD
        { "THEMIS_QNAP_BUILD", true },
#else
        { "THEMIS_QNAP_BUILD", false },
#endif
#ifdef THEMIS_STATIC_BUILD
        { "THEMIS_STATIC_BUILD", true },
#else
        { "THEMIS_STATIC_BUILD", false },
#endif
    };
    
    return config;
}

std::string formatBuildInfo(const BuildConfiguration& config) {
    std::ostringstream oss;
    
    oss << "\n";
    oss << "===============================================================================\n";
    oss << "                      THEMIS DATABASE BUILD CONFIGURATION                       \n";
    oss << "===============================================================================\n";
    oss << "\n";
    
    // Edition Information
    oss << "EDITION INFORMATION:\n";
    oss << "  Edition:            " << config.edition_type << " (" << config.edition_name << ")\n";
    oss << "  GPU VRAM Limit:     " << config.gpu_max_vram_gb << " GB\n";
    oss << "  Max Shard Nodes:    ";
    if (config.sharding_max_nodes == -1) {
        oss << "Unlimited\n";
    } else {
        oss << config.sharding_max_nodes << "\n";
    }
    oss << "\n";
    
    // Build Information
    oss << "BUILD INFORMATION:\n";
    oss << "  Compiler:           " << config.compiler << " " << config.compiler_version << "\n";
    oss << "  Build Type:         " << config.build_type << "\n";
    oss << "  Build Timestamp:    " << config.build_timestamp << "\n";
    oss << "  Version:            " << THEMIS_BUILD_VERSION_STRING << "\n";
    oss << "\n";
    
    // Module Status
    oss << "COMPILED MODULES:\n";
    
    // Count modules by status
    int compiled_count = 0;
    int disabled_count = 0;
    for (const auto& mod : config.modules) {
        if (mod.compiled_in) compiled_count++;
        else disabled_count++;
    }
    
    oss << "  Total Modules:      " << config.modules.size() << "\n";
    oss << "  Compiled In:        " << compiled_count << "\n";
    oss << "  Not Compiled:       " << disabled_count << "\n";
    oss << "\n";
    
    // List enabled modules
    oss << "  Enabled Modules:\n";
    for (const auto& mod : config.modules) {
        if (mod.compiled_in) {
            oss << "    \u2713 " << std::left << std::setw(28) << mod.name 
                << " - " << mod.description << "\n";
        }
    }
    
    // List disabled modules
    if (disabled_count > 0) {
        oss << "\n";
        oss << "  Disabled Modules:\n";
        for (const auto& mod : config.modules) {
            if (!mod.compiled_in) {
                oss << "    \u2717 " << std::left << std::setw(28) << mod.name 
                    << " - " << mod.description << "\n";
            }
        }
    }
    
    oss << "\n";
    oss << "===============================================================================\n";
    
    return oss.str();
}

std::string getVersionSummary() {
    const auto config = getBuildConfiguration();
    std::ostringstream oss;
    
    oss << "ThemisDB " << THEMIS_BUILD_VERSION_STRING;
    
    oss << " [" << config.edition_type << " Edition]";
    oss << " [" << config.build_type << "]";
    oss << " [" << config.compiler << " " << config.compiler_version << "]";
    
    return oss.str();
}

bool isModuleCompiledIn(const std::string& module_name) {
    const auto config = getBuildConfiguration();
    for (const auto& mod : config.modules) {
        if (mod.name == module_name) {
            return mod.compiled_in;
        }
    }
    return false;
}

std::vector<std::string> getCompiledModules() {
    const auto config = getBuildConfiguration();
    std::vector<std::string> result;
    for (const auto& mod : config.modules) {
        if (mod.compiled_in) {
            result.push_back(mod.name);
        }
    }
    return result;
}

std::vector<std::string> getDisabledModules() {
    const auto config = getBuildConfiguration();
    std::vector<std::string> result;
    for (const auto& mod : config.modules) {
        if (!mod.compiled_in) {
            result.push_back(mod.name);
        }
    }
    return result;
}

// ============================================================================
// BUILD REPRODUCIBILITY IMPLEMENTATION
// ============================================================================

// Compile-time defaults injected by cmake/CMakeLists.txt
#ifndef THEMIS_GIT_COMMIT
#define THEMIS_GIT_COMMIT "unknown"
#endif

#ifndef THEMIS_GIT_COMMIT_DATE
#define THEMIS_GIT_COMMIT_DATE "unknown"
#endif

#ifndef THEMIS_GIT_BRANCH
#define THEMIS_GIT_BRANCH "unknown"
#endif

#ifndef THEMIS_GIT_DIRTY
#define THEMIS_GIT_DIRTY 0
#endif

#ifndef THEMIS_BUILD_HOST
#define THEMIS_BUILD_HOST "unknown"
#endif

#ifndef THEMIS_BUILD_USER
#define THEMIS_BUILD_USER "unknown"
#endif

// ── Helper: SHA-256 hash of the running executable ─────────────────────────
static std::string computeExecutableHash() {
#ifdef THEMIS_HAVE_OPENSSL_SHA
    // Determine path to own executable
    std::string exe_path;
#if defined(__linux__)
    char buf[4096] = {};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) exe_path.assign(buf, static_cast<size_t>(len));
#elif defined(_WIN32)
    char buf[MAX_PATH] = {};
    DWORD len = GetModuleFileNameA(nullptr, buf, sizeof(buf));
    if (len > 0) exe_path.assign(buf, len);
#endif
    if (exe_path.empty()) return "(unavailable)";

    std::ifstream f(exe_path, std::ios::binary);
    if (!f) return "(read-error)";

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "(ctx-error)";
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    char chunk[65536];
    while (f.read(chunk, sizeof(chunk)))
        EVP_DigestUpdate(ctx, chunk, static_cast<size_t>(f.gcount()));
    if (f.gcount() > 0)
        EVP_DigestUpdate(ctx, chunk, static_cast<size_t>(f.gcount()));

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  dlen = 0;
    EVP_DigestFinal_ex(ctx, digest, &dlen);
    EVP_MD_CTX_free(ctx);

    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < dlen; ++i)
        hex << std::setw(2) << static_cast<unsigned>(digest[i]);
    return hex.str();
#else
    return "(openssl-not-available)";
#endif
}

ReproducibilityInfo getReproducibilityInfo() {
    ReproducibilityInfo info;

    info.git_commit      = THEMIS_GIT_COMMIT;
    info.git_commit_date = THEMIS_GIT_COMMIT_DATE;
    info.git_branch      = THEMIS_GIT_BRANCH;
    info.git_dirty       = (THEMIS_GIT_DIRTY != 0);
    info.build_host      = THEMIS_BUILD_HOST;
    info.build_user      = THEMIS_BUILD_USER;

    // Toolchain string: "Compiler/version"
    const auto cfg = getBuildConfiguration();
    info.toolchain = cfg.compiler + "/" + cfg.compiler_version;

    // Key vcpkg / system dependency versions embedded via CMake
#ifdef THEMIS_DEP_ROCKSDB_VERSION
    info.dependencies["rocksdb"] = THEMIS_DEP_ROCKSDB_VERSION;
#endif
#ifdef THEMIS_DEP_OPENSSL_VERSION
    info.dependencies["openssl"] = THEMIS_DEP_OPENSSL_VERSION;
#endif
#ifdef THEMIS_DEP_BOOST_VERSION
    info.dependencies["boost"] = THEMIS_DEP_BOOST_VERSION;
#endif

    info.binary_hash = computeExecutableHash();

    return info;
}

bool exportBuildManifest(const std::string& output_path) {
    const auto repro = getReproducibilityInfo();
    const auto cfg   = getBuildConfiguration();

    std::ofstream out(output_path);
    if (!out) return false;

    // Simple hand-written JSON (avoids adding nlohmann/json as a mandatory dep)
    out << "{\n";
    out << "  \"schema_version\": \"1\",\n";
    out << "  \"git_commit\": \"" << repro.git_commit << "\",\n";
    out << "  \"git_commit_date\": \"" << repro.git_commit_date << "\",\n";
    out << "  \"git_branch\": \"" << repro.git_branch << "\",\n";
    out << "  \"git_dirty\": " << (repro.git_dirty ? "true" : "false") << ",\n";
    out << "  \"build_host\": \"" << repro.build_host << "\",\n";
    out << "  \"build_user\": \"" << repro.build_user << "\",\n";
    out << "  \"build_type\": \"" << cfg.build_type << "\",\n";
    out << "  \"build_timestamp\": \"" << cfg.build_timestamp << "\",\n";
    out << "  \"toolchain\": \"" << repro.toolchain << "\",\n";
    out << "  \"edition\": \"" << cfg.edition_type << "\",\n";
    out << "  \"binary_hash\": \"" << repro.binary_hash << "\",\n";
    out << "  \"dependencies\": {\n";
    bool first_dep = true;
    for (const auto& [name, ver] : repro.dependencies) {
        if (!first_dep) out << ",\n";
        out << "    \"" << name << "\": \"" << ver << "\"";
        first_dep = false;
    }
    out << "\n  }\n";
    out << "}\n";

    return out.good();
}

bool verifyBuildManifest(const std::string& manifest_path) {
    std::ifstream in(manifest_path);
    if (!in) return false;

    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());

    const auto repro = getReproducibilityInfo();

    // Minimal verification: check git_commit field matches embedded value
    auto containsField = [&](const std::string& key, const std::string& value) -> bool {
        const std::string needle = "\"" + key + "\": \"" + value + "\"";
        return content.find(needle) != std::string::npos;
    };

    bool commit_ok    = containsField("git_commit", repro.git_commit);
    bool toolchain_ok = containsField("toolchain",  repro.toolchain);

    return commit_ok && toolchain_ok;
}

void setHsmModuleStatusFn(HsmModuleStatusFn fn) {
    std::lock_guard<std::mutex> lk(hsmStatusFnMutex());
    hsmStatusFnStorage() = std::move(fn);
}

void clearHsmModuleStatusFn() {
    std::lock_guard<std::mutex> lk(hsmStatusFnMutex());
    hsmStatusFnStorage() = nullptr;
}

} // namespace build_info
} // namespace themis

