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

// Use the appropriate CPUID mechanism per platform:
//   - MSVC (_M_X64 / _M_IX86): <intrin.h> + __cpuidex
//   - GCC/Clang (__x86_64__ / __i386__): <cpuid.h> + __get_cpuid / __get_cpuid_count
#if defined(_M_X64) || defined(_M_IX86)
    // MSVC on x86/x64
    #define THEMIS_CPUID_SUPPORTED 1
    #define THEMIS_CPUID_MSVC 1
    #include <intrin.h>
#elif defined(__x86_64__) || defined(__i386__)
    // GCC/Clang on x86/x64
    #define THEMIS_CPUID_SUPPORTED 1
    #define THEMIS_CPUID_MSVC 0
    #include <cpuid.h>
#else
    #define THEMIS_CPUID_SUPPORTED 0
    #define THEMIS_CPUID_MSVC 0
#endif

// CPUID detection for x86/x64
static void detect_x86_capabilities(HardwareCapabilities& caps) {
#if THEMIS_CPUID_SUPPORTED
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;

#if THEMIS_CPUID_MSVC
    // MSVC path: __cpuidex takes an int array [eax,ebx,ecx,edx]
    int cpu_info[4] = {};
    __cpuidex(cpu_info, 1, 0);
    eax = static_cast<unsigned int>(cpu_info[0]);
    ebx = static_cast<unsigned int>(cpu_info[1]);
    ecx = static_cast<unsigned int>(cpu_info[2]);
    edx = static_cast<unsigned int>(cpu_info[3]);
    caps.has_rdtsc      = (edx & (1U << 4))  != 0;   // RDTSC (bit 4)
    caps.has_sse2       = (edx & (1U << 26)) != 0;   // SSE2 (bit 26)
    caps.has_cmpxchg16b = (ecx & (1U << 13)) != 0;   // CMPXCHG16B (bit 13)

    __cpuidex(cpu_info, 7, 0);
    ebx = static_cast<unsigned int>(cpu_info[1]);
    caps.has_avx2 = (ebx & (1U << 5)) != 0;  // AVX2 (bit 5)
#else
    // GCC/Clang path
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        caps.has_rdtsc      = (edx & (1U << 4))  != 0;   // RDTSC (bit 4)
        caps.has_sse2       = (edx & (1U << 26)) != 0;   // SSE2 (bit 26)
        caps.has_cmpxchg16b = (ecx & (1U << 13)) != 0;   // CMPXCHG16B (bit 13)
    }
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        caps.has_avx2 = (ebx & (1U << 5)) != 0;  // AVX2 (bit 5)
    }
#endif  // THEMIS_CPUID_MSVC
#endif  // THEMIS_CPUID_SUPPORTED
}

// ARM NEON detection
static void detect_arm_capabilities(HardwareCapabilities& caps) {
    (void)caps;
#if defined(__ARM_NEON) || defined(__aarch64__)
    caps.has_arm_neon = true;
#endif
}

// Storage detection for Linux: probe sysfs rotational flag to determine
// whether any candidate block device (NVMe, virtio, SATA/SSD) is present.
// Falls back to true on Windows/macOS where SSD is the common case.
static void detect_storage_capabilities(HardwareCapabilities& caps) {
#if defined(__linux__)
    // Ordered candidate list: NVMe, virtio, and conventional SATA block devices.
    // For each, check the sysfs "rotational" flag: 0 = non-rotational (SSD/NVMe).
    // If the sysfs entry is absent, also accept the device existing at all
    // (some virtual environments don't expose the rotational flag).
    static const char* const kCandidates[] = {
        "/dev/nvme0n1", "/dev/nvme1n1",
        "/dev/vda",     "/dev/vdb",
        "/dev/sda",     "/dev/sdb",
    };
    static const char* const kRotationalFmt[] = {
        "/sys/block/nvme0n1/queue/rotational",
        "/sys/block/nvme1n1/queue/rotational",
        "/sys/block/vda/queue/rotational",
        "/sys/block/vdb/queue/rotational",
        "/sys/block/sda/queue/rotational",
        "/sys/block/sdb/queue/rotational",
    };
    static_assert(sizeof(kCandidates) == sizeof(kRotationalFmt),
                  "candidate and rotational arrays must be the same length");

    constexpr int kN = static_cast<int>(sizeof(kCandidates) / sizeof(kCandidates[0]));
    for (int i = 0; i < kN; ++i) {
        // Check sysfs rotational flag first (preferred)
        std::ifstream rot(kRotationalFmt[i]);
        if (rot.good()) {
            int val = -1;
            rot >> val;
            if (val == 0) {
                caps.has_ssd = true;  // Non-rotational device confirmed
                return;
            }
            // val == 1 means rotational (HDD); keep searching
            continue;
        }
        // No sysfs entry — fall back to device node existence
        std::ifstream dev(kCandidates[i]);
        if (dev.good()) {
            caps.has_ssd = true;  // Device present; assume SSD-capable
            return;
        }
    }
    // No block device found; has_ssd stays false (fail-closed in callers)
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

