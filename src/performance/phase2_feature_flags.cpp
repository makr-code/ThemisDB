/**
 * @file phase2_feature_flags.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: phase2_feature_flags.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 52
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * PR History (last 5): #1223 Reorganize config architect... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "performance/phase2_feature_flags.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <thread>

namespace themis {
namespace performance {

// ============================================================================
// Hardware Capability Detection
// ============================================================================

#if defined(__x86_64__) || defined(_M_X64)
    #define THEMIS_CPUID_SUPPORTED 1
    #include <cpuid.h>
#elif defined(__i386__) || defined(_M_IX86)
    #define THEMIS_CPUID_SUPPORTED 1
    #include <cpuid.h>
#else
    #define THEMIS_CPUID_SUPPORTED 0
#endif

// CPUID detection for x86/x64
static void detect_x86_capabilities(HardwareCapabilities& caps) {
#if THEMIS_CPUID_SUPPORTED
    unsigned int eax, ebx, ecx, edx;
    
    // Check for SSE2 (level 1)
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        caps.has_rdtsc = (edx & (1U << 4)) != 0;       // RDTSC support (bit 4)
        caps.has_sse2 = (edx & (1U << 26)) != 0;       // SSE2 support (bit 26)
        caps.has_cmpxchg16b = (ecx & (1U << 13)) != 0; // CMPXCHG16B (bit 13)
    }
    
    // Check for AVX2 (level 7)
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        caps.has_avx2 = (ebx & (1U << 5)) != 0;  // AVX2 support (bit 5)
    }
#endif
}

// ARM NEON detection
static void detect_arm_capabilities(HardwareCapabilities& caps) {
#if defined(__ARM_NEON) || defined(__aarch64__)
    caps.has_arm_neon = true;
#endif
}

// Storage detection (simplified: check if /dev/sda or similar exists on Linux)
static void detect_storage_capabilities(HardwareCapabilities& caps) {
#if defined(__linux__)
    // Try to detect if at least one block device exists (heuristic)
    std::ifstream dev("/dev/sda");
    if (dev.good()) {
        caps.has_ssd = true;  // Assume SSD if block device exists
    }
#elif defined(_WIN32)
    // On Windows, assume SSD is available
    caps.has_ssd = true;
#elif defined(__APPLE__)
    // On macOS, assume SSD is available
    caps.has_ssd = true;
#endif
}

Phase2FeatureFlags::Phase2FeatureFlags() {
    detect_hardware_capabilities();
}

void Phase2FeatureFlags::detect_hardware_capabilities() {
    // Detect number of cores
    uint32_t cores = std::thread::hardware_concurrency();
    capabilities_.num_cores = (cores > 0) ? cores : 1;
    
    // Detect ISA extensions
    detect_x86_capabilities(capabilities_);
    detect_arm_capabilities(capabilities_);
    
    // Detect storage
    detect_storage_capabilities(capabilities_);
}

// ============================================================================
// Configuration Loading
// ============================================================================

void Phase2FeatureFlags::load_from_config(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            return; // Config file doesn't exist, use defaults
        }
        
        nlohmann::json config;
        file >> config;
        
        if (config.contains("performance") && config["performance"].contains("phase2")) {
            auto phase2 = config["performance"]["phase2"];
            
            // Only enable features if hardware supports them AND config allows
            if (phase2.contains("wisckey_enabled") && wisckey_hardware_supported()) {
                set_wisckey_enabled(phase2["wisckey_enabled"]);
            }
            if (phase2.contains("dostoevsky_enabled") && dostoevsky_hardware_supported()) {
                set_dostoevsky_enabled(phase2["dostoevsky_enabled"]);
            }
            if (phase2.contains("cicada_enabled") && cicada_hardware_supported()) {
                set_cicada_enabled(phase2["cicada_enabled"]);
            }
            if (phase2.contains("ligra_enabled") && ligra_hardware_supported()) {
                set_ligra_enabled(phase2["ligra_enabled"]);
            }
            if (phase2.contains("rabitq_enabled") && rabitq_hardware_supported()) {
                set_rabitq_enabled(phase2["rabitq_enabled"]);
            }
        }
    } catch (...) {
        // Ignore JSON parsing errors, use defaults with hardware detection
    }
}

} // namespace performance
} // namespace themis

