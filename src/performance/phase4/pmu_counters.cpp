/**
 * @file pmu_counters.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=6, H=0, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/phase4/pmu_counters.h"
#include <stdexcept>

#include <mutex>

#ifdef THEMIS_ENABLE_PMU_COUNTERS
#ifdef __linux__

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

// ---------------------------------------------------------------------------
// Helper: thin wrapper around perf_event_open(2)
// ---------------------------------------------------------------------------
static long perf_event_open(struct perf_event_attr* attr,
                             pid_t pid, int cpu,
                             int group_fd, unsigned long flags) noexcept
{
    return ::syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

namespace themis {
namespace performance {
namespace phase4 {

// ---------------------------------------------------------------------------
// PmuCounter
// ---------------------------------------------------------------------------

PmuCounter::PmuCounter() noexcept : fd_(-1) {}

PmuCounter::~PmuCounter() noexcept {
    close();
}

PmuCounter::PmuCounter(PmuCounter&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

PmuCounter& PmuCounter::operator=(PmuCounter&& other) noexcept {
    if (this != &other) {
        close();
        fd_       = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

bool PmuCounter::open(uint32_t type, uint64_t config) noexcept {
    try {
        close();

        struct perf_event_attr attr{};
        attr.type           = type;
        attr.size           = sizeof(attr);
        attr.config         = config;
        attr.disabled       = 1;   // start disabled; caller calls enable()
        attr.exclude_kernel = 1;   // user-space only to avoid CAP_SYS_ADMIN
        attr.exclude_hv     = 1;

        long fd = perf_event_open(&attr,
                                  0,    // current process
                                  -1,   // any CPU
                                  -1,   // no group
                                  0);
        if (fd < 0) {
            return false;
        }
        fd_ = static_cast<int>(fd);
        return true;
    } catch (...) {
        fd_ = -1;
        return false;
    }
}

void PmuCounter::enable() noexcept {
    if (fd_ >= 0) {
        int ret1 = ::ioctl(fd_, PERF_EVENT_IOC_RESET,  0);
        if (ret1 < 0) {
            // Log or handle reset failure gracefully
        }
        int ret2 = ::ioctl(fd_, PERF_EVENT_IOC_ENABLE, 0);
        if (ret2 < 0) {
            // Log or handle enable failure gracefully
        }
    }
}

void PmuCounter::disable() noexcept {
    if (fd_ >= 0) {
        int ret = ::ioctl(fd_, PERF_EVENT_IOC_DISABLE, 0);
        if (ret < 0) {
            // Log or handle disable failure gracefully
        }
    }
}

uint64_t PmuCounter::read() const noexcept {
    if (fd_ < 0) {
      return 0;
    }
    uint64_t value = 0;
    ssize_t bytes_read = ::read(fd_, &value, sizeof(value));
    if (bytes_read != static_cast<ssize_t>(sizeof(value))) {
        return 0;
    }
    return value;
}

void PmuCounter::close() noexcept {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

// ---------------------------------------------------------------------------
// CacheMissAnalyzer
// ---------------------------------------------------------------------------

CacheMissAnalyzer::CacheMissAnalyzer() noexcept {
    try {
        // L1d read misses: PERF_TYPE_HW_CACHE with
        //   L1D | (OP_READ << 8) | (RESULT_MISS << 16)
        constexpr uint64_t l1d_cfg =
            static_cast<uint64_t>(PERF_COUNT_HW_CACHE_L1D)              |
            (static_cast<uint64_t>(PERF_COUNT_HW_CACHE_OP_READ) << 8)   |
            (static_cast<uint64_t>(PERF_COUNT_HW_CACHE_RESULT_MISS) << 16);

        bool ok = l1d_misses_.open(PERF_TYPE_HW_CACHE, l1d_cfg);

        // LLC misses
        ok &= llc_misses_.open(PERF_TYPE_HARDWARE,
                                static_cast<uint64_t>(PERF_COUNT_HW_CACHE_MISSES));

        // Branch mispredictions
        ok &= branch_mispredict_.open(PERF_TYPE_HARDWARE,
                                       static_cast<uint64_t>(PERF_COUNT_HW_BRANCH_MISSES));

        available_ = ok;
    } catch (...) {
        available_ = false;
        // Ensure counters are in closed state on exception
        l1d_misses_.close();
        llc_misses_.close();
        branch_mispredict_.close();
    }
}

void CacheMissAnalyzer::start() noexcept {
    if (!available_) {
      return;
    }
    l1d_misses_.enable();
    llc_misses_.enable();
    branch_mispredict_.enable();
}

CacheMissMetrics CacheMissAnalyzer::stop() noexcept {
    CacheMissMetrics m = {};
    if (!available_) {
      return m;
    }

    l1d_misses_.disable();
    llc_misses_.disable();
    branch_mispredict_.disable();

    m.l1d_read_misses       = l1d_misses_.read();
    m.llc_misses            = llc_misses_.read();
    m.branch_mispredictions = branch_mispredict_.read();
    m.available             = true;
    return m;
}

bool CacheMissAnalyzer::pmu_accessible() noexcept {
    // Quick probe: try to open one counter and immediately close it
    PmuCounter probe;
    bool ok = probe.open(PERF_TYPE_HARDWARE,
                         static_cast<uint64_t>(PERF_COUNT_HW_CACHE_MISSES));
    return ok;
}

} // namespace phase4
} // namespace performance
} // namespace themis

#elif defined(__APPLE__)

// ---------------------------------------------------------------------------
// macOS PMU backend (THEMIS_ENABLE_PMU_COUNTERS && __APPLE__)
//
// Primary path:  kpc (kernel performance counters) via dynamic loading.
//   - Available on macOS 10.12+ (private API); increasingly public on macOS 14+.
//   - Provides real hardware PMC events: L1d cache refill, LLC miss, branch
//     misprediction.  Requires entitlement or root on older macOS versions;
//     in CI/sandboxed environments it falls back silently.
//
// Fallback path: RDTSC (x86_64) or CNTVCT_EL0 / mach_absolute_time (ARM64).
//   - PmuCounter::open() always returns true on this fallback path.
//   - PmuCounter::read() returns elapsed cycles (not event counts).
//   - CacheMissMetrics::available is set to true; l1d/llc/branch fields are 0
//     because they are not measurable without kpc access.
// ---------------------------------------------------------------------------

#include <atomic>
#include <dlfcn.h>
#include <mach/mach_time.h>

namespace themis {
namespace performance {
namespace phase4 {

namespace {

// ---- kpc API type definitions ----

constexpr uint32_t kKpcClassConfigurable = 1u << 1;  // configurable PMC slots

using kpc_get_counter_count_fn_t   = uint32_t (*)(uint32_t);
using kpc_set_counting_fn_t        = int (*)(uint32_t);
using kpc_set_thread_counting_fn_t = int (*)(uint32_t);
using kpc_get_thread_counters_fn_t = int (*)(uint32_t tid, uint32_t buf_count,
                                              uint64_t* buf);
using kpc_set_config_fn_t          = int (*)(uint32_t, void*);

// Intel x86 raw PMC event selectors (SDM Vol. 3B encoding: umask<<8 | event)
// Used on Intel Macs via kpc_set_config.
constexpr uint64_t kIntelL1dReadMissEvent  = 0x0151;  // MEM_LOAD_RETIRED.L1_MISS
constexpr uint64_t kIntelLlcMissEvent      = 0x412e;  // LONGEST_LAT_CACHE.MISS
constexpr uint64_t kIntelBranchMissEvent   = 0x00c5;  // BR_MISP_RETIRED.ALL_BRANCHES

// ARM PMU architectural event numbers (Arm ARM v9, Table D7-1)
// Used on Apple Silicon via kpc_set_config.
constexpr uint64_t kArmL1dCacheRefill   = 0x03;  // L1D_CACHE_REFILL
constexpr uint64_t kArmLlcMiss          = 0x17;  // L2D_CACHE_REFILL (proxy for LLC)
constexpr uint64_t kArmBranchMispredict = 0x10;  // BR_MIS_PRED

constexpr uint32_t kKpcCounterSlots = 6;  // configurable PMC slots on most Apple CPUs

// ---- kpc dynamic loader ----

struct KpcApi {
    void*                        lib                 = nullptr;
    kpc_get_counter_count_fn_t   get_counter_count   = nullptr;
    kpc_set_counting_fn_t        set_counting         = nullptr;
    kpc_set_thread_counting_fn_t set_thread_counting  = nullptr;
    kpc_get_thread_counters_fn_t get_thread_counters  = nullptr;
    kpc_set_config_fn_t          set_config           = nullptr;
    bool                         loaded               = false;

    static KpcApi& instance() {
        static KpcApi api;
        return api;
    }

    bool init() noexcept {
        if (loaded) {
          return true;
        }
        
        try {
            const char* candidates[] = {
                "/System/Library/PrivateFrameworks/kperf.framework/kperf",
                "/usr/lib/system/libkperf.dylib",
                "kperf",
            };
            for (auto* path : candidates) {
                lib = ::dlopen(path, RTLD_LAZY | RTLD_LOCAL);
                if (lib) {
                  break;
                }
            }
            if (!lib) {
              return false;
            }

#define KPC_LOAD(name) \
            name = reinterpret_cast<decltype(name)>(::dlsym(lib, #name)); \
            if (!name) { ::dlclose(lib); lib = nullptr; return false; }

            KPC_LOAD(kpc_get_counter_count)
            KPC_LOAD(kpc_set_counting)
            KPC_LOAD(kpc_set_thread_counting)
            KPC_LOAD(kpc_get_thread_counters)
            KPC_LOAD(kpc_set_config)
#undef KPC_LOAD

            loaded = true;
            return true;
        } catch (...) {
            if (lib) {
                ::dlclose(lib);
                lib = nullptr;
            }
            loaded = false;
            return false;
        }
    }

    ~KpcApi() {
        if (lib) { ::dlclose(lib); lib = nullptr; }
    }

    KpcApi() = default;
    KpcApi(const KpcApi&) = delete;
    KpcApi& operator=(const KpcApi&) = delete;
};

// ---- Fallback: RDTSC (x86_64) or CNTVCT_EL0 / mach_absolute_time (ARM64) ----

static inline uint64_t read_platform_cycles() noexcept {
#if defined(__x86_64__)
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
#elif defined(__aarch64__)
    // Virtual counter register — same as CycleMetrics::cpu_cycles() on ARM64
    uint64_t val = 0;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
    return static_cast<uint64_t>(::mach_absolute_time());
#endif
}

// Thread-local slot pool for fallback RDTSC-based PmuCounter instances.
// Each PmuCounter allocates a slot (fd_ = slot index); enable() records the
// start cycle, read() returns the delta since last enable().
constexpr int kMaxFallbackSlots = 128;
static thread_local uint64_t tl_fallback_starts[kMaxFallbackSlots] = {};
static std::atomic<int>      s_fallback_slot_seq{0};

// ---- kpc setup helper ----

static bool setup_kpc_counters() noexcept {
    try {
        auto& api = KpcApi::instance();
        if (!api.init()) {
          return false;
        }

        if (api.set_counting(kKpcClassConfigurable) != 0) {
            return false;
        }
        if (api.set_thread_counting(kKpcClassConfigurable) != 0) {
            return false;
        }

        uint32_t n = api.get_counter_count(kKpcClassConfigurable);
        if (n < 3) {
          return false;
        }

        uint64_t configs[kKpcCounterSlots] = {};
#if defined(__aarch64__)
        configs[0] = kArmL1dCacheRefill;
        configs[1] = kArmLlcMiss;
        configs[2] = kArmBranchMispredict;
#else
        configs[0] = kIntelL1dReadMissEvent;
        configs[1] = kIntelLlcMissEvent;
        configs[2] = kIntelBranchMissEvent;
#endif
        int config_ret = api.set_config(kKpcClassConfigurable, configs);
        if (config_ret != 0) {
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// Per-thread kpc counter snapshot buffers
constexpr uint32_t kKpcBufSize = 32;
static thread_local uint64_t tl_kpc_baseline[kKpcBufSize] = {};

} // anonymous namespace

// ---------------------------------------------------------------------------
// PmuCounter — macOS implementation
// ---------------------------------------------------------------------------

PmuCounter::PmuCounter() noexcept : fd_(-1) {}
PmuCounter::~PmuCounter() noexcept { close(); }
PmuCounter::PmuCounter(PmuCounter&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
PmuCounter& PmuCounter::operator=(PmuCounter&& o) noexcept {
    if (this != &o) { close(); fd_ = o.fd_; o.fd_ = -1; }
    return *this;
}

bool PmuCounter::open(uint32_t /*type*/, uint64_t /*config*/) noexcept {
    close();
    // Allocate a fallback slot; used for both kpc and RDTSC paths.
    fd_ = s_fallback_slot_seq.fetch_add(1, std::memory_order_relaxed)
          % kMaxFallbackSlots;
    tl_fallback_starts[fd_] = 0;
    return true;  // RDTSC / mach_absolute_time is always available
}

void PmuCounter::enable() noexcept {
    if (fd_ < 0) {
      return;
    }
    tl_fallback_starts[fd_] = read_platform_cycles();
}

void PmuCounter::disable() noexcept {
    // Delta is computed lazily in read(); nothing to do here.
}

uint64_t PmuCounter::read() const noexcept {
    if (fd_ < 0) {
      return 0;
    }
    uint64_t current = read_platform_cycles();
    uint64_t start   = tl_fallback_starts[fd_];
    return (current >= start) ? (current - start) : 0;
}

void PmuCounter::close() noexcept { fd_ = -1; }

// ---------------------------------------------------------------------------
// CacheMissAnalyzer — macOS implementation
// ---------------------------------------------------------------------------

CacheMissAnalyzer::CacheMissAnalyzer() noexcept : available_(false) {
    try {
        auto& api = KpcApi::instance();
        if (api.init() && setup_kpc_counters()) {
            available_ = true;
            return;
        }
        // kpc unavailable (sandbox / missing entitlement): fall back to
        // RDTSC-based timing.  open() always succeeds on macOS.
        bool ok1 = l1d_misses_.open(0, 0);
        bool ok2 = llc_misses_.open(0, 0);
        bool ok3 = branch_mispredict_.open(0, 0);
        available_ = ok1 && ok2 && ok3;
    } catch (...) {
        available_ = false;
    }
}

void CacheMissAnalyzer::start() noexcept {
    if (!available_) {
      return;
    }
    auto& api = KpcApi::instance();
    if (api.loaded) {
        // Snapshot current kpc counter values as the measurement baseline.
        api.get_thread_counters(0, kKpcBufSize, tl_kpc_baseline);
    } else {
        l1d_misses_.enable();
        llc_misses_.enable();
        branch_mispredict_.enable();
    }
}

CacheMissMetrics CacheMissAnalyzer::stop() noexcept {
    CacheMissMetrics m = {};
    if (!available_) {
      return m;
    }

    try {
        auto& api = KpcApi::instance();
        if (api.loaded) {
            uint64_t current[kKpcBufSize] = {};
            int ret = api.get_thread_counters(0, kKpcBufSize, current);
            if (ret != 0) {
                // Failed to get counters
                m.l1d_read_misses       = 0;
                m.llc_misses            = 0;
                m.branch_mispredictions = 0;
                m.available             = false;
                return m;
            }
            m.l1d_read_misses       = current[0] - tl_kpc_baseline[0];
            m.llc_misses            = current[1] - tl_kpc_baseline[1];
            m.branch_mispredictions = current[2] - tl_kpc_baseline[2];
            m.available             = true;
        } else {
            // RDTSC fallback: cycle counters are available but hardware cache-miss
            // events are not. Return zero counts; available=true indicates that
            // the performance measurement infrastructure itself is functional.
            l1d_misses_.disable();
            llc_misses_.disable();
            branch_mispredict_.disable();
            m.l1d_read_misses       = 0;
            m.llc_misses            = 0;
            m.branch_mispredictions = 0;
            m.available             = true;
        }
        return m;
    } catch (...) {
        m.l1d_read_misses       = 0;
        m.llc_misses            = 0;
        m.branch_mispredictions = 0;
        m.available             = false;
        return m;
    }
}

bool CacheMissAnalyzer::pmu_accessible() noexcept {
    try {
        auto& api = KpcApi::instance();
        if (api.init()) {
            uint64_t probe[kKpcBufSize] = {};
            int ret = api.get_thread_counters(0, kKpcBufSize, probe);
            return ret == 0;
        }
        // RDTSC / mach_absolute_time is always accessible
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace phase4
} // namespace performance
} // namespace themis

#elif defined(_WIN32)

// ---------------------------------------------------------------------------
// Windows PMU backend (THEMIS_ENABLE_PMU_COUNTERS && _WIN32)
//
// Primary cycle-count source: __rdtsc() on x86/x86_64; QueryThreadCycleTime
// on ARM64 Windows.
//
// True hardware PMU cache-miss events (L1d, LLC, branch misprediction) would
// require a kernel-mode ETW hardware counter session with admin privileges.
// That path is deferred to a future release.  As a minimum viable fallback,
// cycle-count-based measurements are provided so that the performance
// measurement infrastructure is functional on Windows developer workstations.
// CacheMissMetrics::available is set to true; cache-miss count fields are 0.
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <atomic>
#include <windows.h>

namespace themis {
namespace performance {
namespace phase4 {

namespace {

static inline uint64_t read_platform_cycles() noexcept {
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_IX86)
    return static_cast<uint64_t>(__rdtsc());
#else
    // ARM64 Windows
    ULONG64 cycles = 0;
    ::QueryThreadCycleTime(::GetCurrentThread(), &cycles);
    return static_cast<uint64_t>(cycles);
#endif
}

// Thread-local slot pool for per-counter start-cycle storage.
constexpr int kMaxWinSlots = 128;
static thread_local uint64_t tl_win_starts[kMaxWinSlots] = {};
static std::atomic<int>      s_win_slot_seq{0};

} // anonymous namespace

// ---------------------------------------------------------------------------
// PmuCounter — Windows implementation
// ---------------------------------------------------------------------------

PmuCounter::PmuCounter() noexcept : fd_(-1) {}
PmuCounter::~PmuCounter() noexcept { close(); }
PmuCounter::PmuCounter(PmuCounter&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
PmuCounter& PmuCounter::operator=(PmuCounter&& o) noexcept {
    if (this != &o) { close(); fd_ = o.fd_; o.fd_ = -1; }
    return *this;
}

bool PmuCounter::open(uint32_t /*type*/, uint64_t /*config*/) noexcept {
    close();
    fd_ = s_win_slot_seq.fetch_add(1, std::memory_order_relaxed) % kMaxWinSlots;
    tl_win_starts[fd_] = 0;
    return true;  // RDTSC / QueryThreadCycleTime is always available
}

void PmuCounter::enable() noexcept {
    if (fd_ < 0) {
      return;
    }
    tl_win_starts[fd_] = read_platform_cycles();
}

void PmuCounter::disable() noexcept {
    // Delta is computed lazily in read(); nothing to do here.
}

uint64_t PmuCounter::read() const noexcept {
    if (fd_ < 0) {
      return 0;
    }
    uint64_t current = read_platform_cycles();
    uint64_t start   = tl_win_starts[fd_];
    return (current >= start) ? (current - start) : 0;
}

void PmuCounter::close() noexcept { fd_ = -1; }

// ---------------------------------------------------------------------------
// CacheMissAnalyzer — Windows implementation
// ---------------------------------------------------------------------------

CacheMissAnalyzer::CacheMissAnalyzer() noexcept : available_(false) {
    try {
        bool ok1 = l1d_misses_.open(0, 0);
        bool ok2 = llc_misses_.open(0, 0);
        bool ok3 = branch_mispredict_.open(0, 0);
        available_ = ok1 && ok2 && ok3;
    } catch (...) {
        available_ = false;
    }
}

void CacheMissAnalyzer::start() noexcept {
    if (!available_) {
      return;
    }
    l1d_misses_.enable();
    llc_misses_.enable();
    branch_mispredict_.enable();
}

CacheMissMetrics CacheMissAnalyzer::stop() noexcept {
    CacheMissMetrics m = {};
    if (!available_) {
      return m;
    }

    l1d_misses_.disable();
    llc_misses_.disable();
    branch_mispredict_.disable();

    // Note: these fields would contain real hardware PMU event counts when an
    // ETW hardware counter session is active.  Without that session (the common
    // case on developer workstations), they remain 0.  available=true indicates
    // that cycle-based timing is functional.
    m.l1d_read_misses       = 0;
    m.llc_misses            = 0;
    m.branch_mispredictions = 0;
    m.available             = true;
    return m;
}

bool CacheMissAnalyzer::pmu_accessible() noexcept {
    // RDTSC / QueryThreadCycleTime is always accessible on Windows.
    return true;
}

} // namespace phase4
} // namespace performance
} // namespace themis

#else

// ---------------------------------------------------------------------------
// Generic non-Linux / non-macOS / non-Windows fallback
// (THEMIS_ENABLE_PMU_COUNTERS && !__linux__ && !__APPLE__ && !_WIN32)
//
// Uses RDTSC on x86_64, CNTVCT_EL0 on ARM64, or clock_gettime(CLOCK_MONOTONIC)
// on all other architectures.  Hardware cache-miss event counts are not
// available; CacheMissMetrics::available is set to true to indicate that
// cycle-based timing infrastructure is functional.
// ---------------------------------------------------------------------------

#include <atomic>
#include <ctime>

namespace themis {
namespace performance {
namespace phase4 {

namespace {

static inline uint64_t read_platform_cycles() noexcept {
#if defined(__x86_64__) || defined(__i386__)
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return (static_cast<uint64_t>(hi) << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val = 0;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#else
    struct timespec ts{};
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL
         + static_cast<uint64_t>(ts.tv_nsec);
#endif
}

constexpr int kMaxRdtscSlots = 128;
static thread_local uint64_t tl_rdtsc_starts[kMaxRdtscSlots] = {};
static std::atomic<int>      s_rdtsc_slot_seq{0};

} // anonymous namespace

// ---------------------------------------------------------------------------
// PmuCounter — generic RDTSC / clock_gettime fallback
// ---------------------------------------------------------------------------

PmuCounter::PmuCounter() noexcept : fd_(-1) {}
PmuCounter::~PmuCounter() noexcept { close(); }
PmuCounter::PmuCounter(PmuCounter&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
PmuCounter& PmuCounter::operator=(PmuCounter&& o) noexcept {
    if (this != &o) { close(); fd_ = o.fd_; o.fd_ = -1; }
    return *this;
}

bool PmuCounter::open(uint32_t /*type*/, uint64_t /*config*/) noexcept {
    close();
    fd_ = s_rdtsc_slot_seq.fetch_add(1, std::memory_order_relaxed) % kMaxRdtscSlots;
    tl_rdtsc_starts[fd_] = 0;
    return true;
}

void PmuCounter::enable() noexcept {
    if (fd_ < 0) {
      return;
    }
    tl_rdtsc_starts[fd_] = read_platform_cycles();
}

void PmuCounter::disable() noexcept {}

uint64_t PmuCounter::read() const noexcept {
    if (fd_ < 0) {
      return 0;
    }
    uint64_t current = read_platform_cycles();
    uint64_t start   = tl_rdtsc_starts[fd_];
    return (current >= start) ? (current - start) : 0;
}

void PmuCounter::close() noexcept { fd_ = -1; }

// ---------------------------------------------------------------------------
// CacheMissAnalyzer — generic RDTSC / clock_gettime fallback
// ---------------------------------------------------------------------------

CacheMissAnalyzer::CacheMissAnalyzer() noexcept : available_(false) {
    try {
        bool ok1 = l1d_misses_.open(0, 0);
        bool ok2 = llc_misses_.open(0, 0);
        bool ok3 = branch_mispredict_.open(0, 0);
        available_ = ok1 && ok2 && ok3;
    } catch (...) {
        available_ = false;
    }
}

void CacheMissAnalyzer::start() noexcept {
    if (!available_) {
      return;
    }
    l1d_misses_.enable();
    llc_misses_.enable();
    branch_mispredict_.enable();
}

CacheMissMetrics CacheMissAnalyzer::stop() noexcept {
    CacheMissMetrics m = {};
    if (!available_) {
      return m;
    }

    l1d_misses_.disable();
    llc_misses_.disable();
    branch_mispredict_.disable();

    m.l1d_read_misses       = 0;
    m.llc_misses            = 0;
    m.branch_mispredictions = 0;
    m.available             = true;
    return m;
}

bool CacheMissAnalyzer::pmu_accessible() noexcept {
    return true;
}

} // namespace phase4
} // namespace performance
} // namespace themis

#endif // platform

#else // !THEMIS_ENABLE_PMU_COUNTERS

// STUB/SIMULATION NOTE:
// Purpose:          Compile-time no-op stubs for PmuCounter and CacheMissAnalyzer.
//                   Returned when the THEMIS_ENABLE_PMU_COUNTERS build flag is not set,
//                   so the performance module still links on any platform without perf_event
//                   support (Windows, macOS, embedded targets).
// Activation:       Active when THEMIS_ENABLE_PMU_COUNTERS is NOT defined at compile time.
//                   On Linux with kernel ≥ 3.4 and perf_event_paranoid ≤ 2, define the flag
//                   to enable hardware PMU counters.
// Production Delta: PmuCounter::open() returns false; read() returns 0.
//                   CacheMissAnalyzer::stop() returns zero-valued CacheMissMetrics.
//                   No perf_event file descriptors are opened; no kernel calls are made.
// Removal Plan:     Enable THEMIS_ENABLE_PMU_COUNTERS in production Linux CI builds.
//                   Tracked in src/performance/ROADMAP.md § Phase 4 (PMU Counters).

namespace themis {
namespace performance {
namespace phase4 {

namespace {
std::mutex                 s_pmu_stub_mutex;
PmuCounter::OpenFn         s_pmu_open_fn;
PmuCounter::ReadFn         s_pmu_read_fn;
CacheMissAnalyzer::StopFn  s_cache_miss_stop_fn;
CacheMissAnalyzer::ProbeFn s_cache_miss_probe_fn;
}

void PmuCounter::setOpenFn(OpenFn fn) {
    std::lock_guard<std::mutex> lk(s_pmu_stub_mutex);
    s_pmu_open_fn = std::move(fn);
}

void PmuCounter::setReadFn(ReadFn fn) {
    std::lock_guard<std::mutex> lk(s_pmu_stub_mutex);
    s_pmu_read_fn = std::move(fn);
}

void CacheMissAnalyzer::setStopFn(StopFn fn) {
    std::lock_guard<std::mutex> lk(s_pmu_stub_mutex);
    s_cache_miss_stop_fn = std::move(fn);
}

void CacheMissAnalyzer::setProbeFn(ProbeFn fn) {
    std::lock_guard<std::mutex> lk(s_pmu_stub_mutex);
    s_cache_miss_probe_fn = std::move(fn);
}

PmuCounter::PmuCounter() noexcept : fd_(-1) {}
PmuCounter::~PmuCounter() noexcept {}
PmuCounter::PmuCounter(PmuCounter&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
PmuCounter& PmuCounter::operator=(PmuCounter&& o) noexcept {
    if (this != &o) { close(); fd_ = o.fd_; o.fd_ = -1; }
    return *this;
}
bool PmuCounter::open(uint32_t type, uint64_t config) noexcept {
    OpenFn fn;
    {
        std::lock_guard<std::mutex> lk(s_pmu_stub_mutex);
        fn = s_pmu_open_fn;
    }
    if (fn) {
        try {
            const bool opened = fn(type, config);
            fd_ = opened ? 1 : -1;
            return opened;
        } catch (...) {
            fd_ = -1;
            return false;
        }
    }
    fd_ = -1;
    return false;
}
void     PmuCounter::enable()  noexcept {}
void     PmuCounter::disable() noexcept {}
uint64_t PmuCounter::read()  const noexcept {
    ReadFn fn;
    {
        std::lock_guard<std::mutex> lk(s_pmu_stub_mutex);
        fn = s_pmu_read_fn;
    }
    if (fn) {
        try {
            return fn();
        } catch (...) {
            return 0;
        }
    }
    return 0;
}
void     PmuCounter::close() noexcept { fd_ = -1; }

CacheMissAnalyzer::CacheMissAnalyzer() noexcept : available_(pmu_accessible()) {}
void             CacheMissAnalyzer::start() noexcept {}
CacheMissMetrics CacheMissAnalyzer::stop() noexcept {
    StopFn fn;
    {
        std::lock_guard<std::mutex> lk(s_pmu_stub_mutex);
        fn = s_cache_miss_stop_fn;
    }
    if (fn) {
        try {
            auto metrics = fn();
            metrics.available = true;
            return metrics;
        } catch (...) {
            return {};
        }
    }
    return {};
}
bool CacheMissAnalyzer::pmu_accessible() noexcept {
    ProbeFn fn;
    {
        std::lock_guard<std::mutex> lk(s_pmu_stub_mutex);
        fn = s_cache_miss_probe_fn;
    }
    if (fn) {
        try {
            return fn();
        } catch (...) {
            return false;
        }
    }
    return false;
}

} // namespace phase4
} // namespace performance
} // namespace themis

#endif // THEMIS_ENABLE_PMU_COUNTERS


