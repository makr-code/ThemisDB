/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pmu_counters.cpp                                   ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:59:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     242                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3fc507357  2026-02-25  feat(performance/phase4): add PMU hardware counter integr... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "performance/phase4/pmu_counters.h"

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
}

void PmuCounter::enable() noexcept {
    if (fd_ >= 0) {
        ::ioctl(fd_, PERF_EVENT_IOC_RESET,  0);
        ::ioctl(fd_, PERF_EVENT_IOC_ENABLE, 0);
    }
}

void PmuCounter::disable() noexcept {
    if (fd_ >= 0) {
        ::ioctl(fd_, PERF_EVENT_IOC_DISABLE, 0);
    }
}

uint64_t PmuCounter::read() const noexcept {
    if (fd_ < 0) return 0;
    uint64_t value = 0;
    if (::read(fd_, &value, sizeof(value)) != sizeof(value)) {
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
}

void CacheMissAnalyzer::start() noexcept {
    if (!available_) return;
    l1d_misses_.enable();
    llc_misses_.enable();
    branch_mispredict_.enable();
}

CacheMissMetrics CacheMissAnalyzer::stop() noexcept {
    CacheMissMetrics m;
    if (!available_) return m;

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

#else // !__linux__

// Non-Linux stubs – all counters report unavailable

namespace themis {
namespace performance {
namespace phase4 {

PmuCounter::PmuCounter() noexcept : fd_(-1) {}
PmuCounter::~PmuCounter() noexcept {}
PmuCounter::PmuCounter(PmuCounter&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
PmuCounter& PmuCounter::operator=(PmuCounter&& o) noexcept {
    if (this != &o) { close(); fd_ = o.fd_; o.fd_ = -1; }
    return *this;
}
bool     PmuCounter::open(uint32_t, uint64_t) noexcept { return false; }
void     PmuCounter::enable()  noexcept {}
void     PmuCounter::disable() noexcept {}
uint64_t PmuCounter::read()  const noexcept { return 0; }
void     PmuCounter::close() noexcept { fd_ = -1; }

CacheMissAnalyzer::CacheMissAnalyzer() noexcept : available_(false) {}
void             CacheMissAnalyzer::start() noexcept {}
CacheMissMetrics CacheMissAnalyzer::stop() noexcept { return {}; }
bool             CacheMissAnalyzer::pmu_accessible() noexcept { return false; }

} // namespace phase4
} // namespace performance
} // namespace themis

#endif // __linux__

#else // !THEMIS_ENABLE_PMU_COUNTERS

// Stubs when PMU counters are disabled at compile time

namespace themis {
namespace performance {
namespace phase4 {

PmuCounter::PmuCounter() noexcept : fd_(-1) {}
PmuCounter::~PmuCounter() noexcept {}
PmuCounter::PmuCounter(PmuCounter&& o) noexcept : fd_(o.fd_) { o.fd_ = -1; }
PmuCounter& PmuCounter::operator=(PmuCounter&& o) noexcept {
    if (this != &o) { close(); fd_ = o.fd_; o.fd_ = -1; }
    return *this;
}
bool     PmuCounter::open(uint32_t, uint64_t) noexcept { return false; }
void     PmuCounter::enable()  noexcept {}
void     PmuCounter::disable() noexcept {}
uint64_t PmuCounter::read()  const noexcept { return 0; }
void     PmuCounter::close() noexcept { fd_ = -1; }

CacheMissAnalyzer::CacheMissAnalyzer() noexcept : available_(false) {}
void             CacheMissAnalyzer::start() noexcept {}
CacheMissMetrics CacheMissAnalyzer::stop() noexcept { return {}; }
bool             CacheMissAnalyzer::pmu_accessible() noexcept { return false; }

} // namespace phase4
} // namespace performance
} // namespace themis

#endif // THEMIS_ENABLE_PMU_COUNTERS
