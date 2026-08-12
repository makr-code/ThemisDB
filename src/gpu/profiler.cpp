/**
 * @file profiler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Profiler — NVIDIA Nsight and AMD ROCm Profiler integration.
 *
 * Real NVTX / rocTX calls are gated behind THEMIS_ENABLE_CUDA and
 * THEMIS_ENABLE_HIP respectively.  When neither is defined (CI / CPU-only
 * builds) the profiler records range events internally and exports them as
 * Chrome trace JSON — identical in structure to ROCm profiler's --sys-trace
 * output — so that the export path is always exercised and tested.
 */

#include "themis/gpu/profiler.h"

#include <chrono>
#include <sstream>

namespace themis {
namespace gpu {

// ============================================================================
// Private helpers
// ============================================================================

uint64_t GPUProfiler::nowNs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

// ============================================================================
// Range markers
// ============================================================================

void GPUProfiler::beginRange(const std::string &name, uint32_t argb_color) {
    std::lock_guard<std::mutex> lock(mutex_);

#ifdef THEMIS_ENABLE_CUDA
    nvtxEventAttributes_t attrs = {};
    attrs.version               = NVTX_VERSION;
    attrs.size                  = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    attrs.colorType             = NVTX_COLOR_ARGB;
    attrs.color                 = argb_color;
    attrs.messageType           = NVTX_MESSAGE_TYPE_ASCII;
    attrs.message.ascii         = name.c_str();
    nvtxRangePushEx(&attrs);
#elif defined(THEMIS_ENABLE_HIP)
    roctxRangePushA(name.c_str());
#else
#endif

    range_stack_.push_back({name, nowNs(), argb_color});
}

void GPUProfiler::endRange() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (range_stack_.empty()) {
        return;
    }

#ifdef THEMIS_ENABLE_CUDA
    nvtxRangePop();
#elif defined(THEMIS_ENABLE_HIP)
    roctxRangePop();
#endif

    const uint64_t now = nowNs();
    const auto &active = range_stack_.back();

    Range completed;
    completed.name     = active.name;
    completed.start_ns = active.start_ns;
    completed.end_ns   = now;
    completed.color    = active.color;
    completed_ranges_.push_back(std::move(completed));
    range_stack_.pop_back();
}

void GPUProfiler::markEvent(const std::string &name) {
    std::lock_guard<std::mutex> lock(mutex_);

#ifdef THEMIS_ENABLE_CUDA
    nvtxMarkA(name.c_str());
#elif defined(THEMIS_ENABLE_HIP)
    roctxMarkA(name.c_str());
#endif

    const uint64_t now = nowNs();
    Range ev;
    ev.name     = name;
    ev.start_ns = now;
    ev.end_ns   = now; // zero duration marks a point event
    completed_ranges_.push_back(std::move(ev));
}

// ============================================================================
// Export — Chrome trace JSON (ROCm profiler compatible)
// ============================================================================

std::string GPUProfiler::rocm_profiler_export() const {
    std::lock_guard<std::mutex> lock(mutex_);

    // Chrome trace format is consumed by:
    //   - AMD ROCm profiler (--sys-trace JSON output)
    //   - Perfetto / chrome://tracing
    //
    // "ph" field values used:
    //   "X"  complete event (duration range)
    //   "i"  instant event  (point marker)
    //
    // Timestamps are in microseconds (Chrome trace convention).
    std::ostringstream oss;

    if (completed_ranges_.empty()) {
        oss << "{\n  \"traceEvents\": []\n}\n";
        return oss.str();
    }

    oss << "{\n  \"traceEvents\": [\n";

    for (std::size_t i = 0; i < completed_ranges_.size(); ++i) {
        const Range &r        = completed_ranges_[i];
        const bool is_instant = (r.start_ns == r.end_ns);

        if (is_instant) {
            oss << "    {\"name\": \"" << r.name << "\", \"ph\": \"i\", "
                << "\"ts\": " << (r.start_ns / 1000) << ", "
                << "\"pid\": 0, \"tid\": " << r.device_id << "}";
        } else {
            oss << "    {\"name\": \"" << r.name << "\", \"ph\": \"X\", "
                << "\"ts\": " << (r.start_ns / 1000) << ", "
                << "\"dur\": " << ((r.end_ns - r.start_ns) / 1000) << ", "
                << "\"pid\": 0, \"tid\": " << r.device_id << "}";
        }
        if (i + 1 < completed_ranges_.size()) {
            oss << ',';
        }
        oss << '\n';
    }

    oss << "  ]\n}\n";
    return oss.str();
}

// ============================================================================
// Accessors
// ============================================================================

std::vector<GPUProfiler::Range> GPUProfiler::getRanges() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return completed_ranges_;
}

void GPUProfiler::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    range_stack_.clear();
    completed_ranges_.clear();
}

// ============================================================================
// ScopedGPURange
// ============================================================================

ScopedGPURange::ScopedGPURange(const std::string &name, uint32_t argb_color) : profiler_(GPUProfiler::GetInstance()) {
    profiler_.beginRange(name, argb_color);
}

ScopedGPURange::~ScopedGPURange() {
    profiler_.endRange();
}

} // namespace gpu
} // namespace themis
