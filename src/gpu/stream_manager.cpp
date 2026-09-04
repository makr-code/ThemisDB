/**
 * @file stream_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Stream Manager — named async GPU streams with CPU fallback budget.
 */

#include "themis/gpu/stream_manager.h"

#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include "themis/gpu/rocm_backend.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

// THEMIS_ENABLE_CUDA block intentionally left empty after RAII migration.
// cudaStreamRegistry() and cudaStreamMutex() removed; stream lifetime is now
// managed by CudaStreamGuard in each Stream::cuda_stream_guard field.


namespace themis {
namespace gpu {

// ---------------------------------------------------------------------------
// STUB #77 — CudaStreamBackendFn static bridge (non-CUDA injection)
// ---------------------------------------------------------------------------
namespace {
std::mutex s_cuda_backend_fn_mutex;
GPUStreamManager::CudaStreamBackendFn s_cuda_backend_fn;
} // namespace

void GPUStreamManager::setCudaStreamBackendFn(CudaStreamBackendFn fn) {
    std::lock_guard<std::mutex> lk(s_cuda_backend_fn_mutex);
    s_cuda_backend_fn = std::move(fn);
}

// ============================================================================
// Construction / destruction
// ============================================================================

GPUStreamManager::~GPUStreamManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &kv : streams_) {
#ifdef THEMIS_ENABLE_CUDA
        // cuda_stream_guard destructor calls cudaStreamSynchronize then
        // cudaStreamDestroy automatically when Stream is destroyed below.
        // Nothing to do explicitly here.
        (void)kv;
#endif
        if (kv.second.uses_rocm_stream) {
            ROCmBackend::GetInstance().destroyStream(kv.first);
        }
    }
    streams_.clear();
}

// ============================================================================
// Stream lifecycle
// ============================================================================

bool GPUStreamManager::createStream(const StreamConfig &cfg, GPULauncher::BackendFn backend) {
    if (cfg.name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (streams_.count(cfg.name)) {
        return false; // already exists
    }

    Stream s;
    s.config     = cfg;
    s.stats.name = cfg.name;

    if (backend) {
        s.launcher = std::make_unique<GPULauncher>(std::move(backend));
    } else {
        // When no backend is supplied, create a real HIP stream via the ROCm
        // backend (which transparently falls back to CPU execution when
        // THEMIS_ENABLE_HIP is not defined) and use it as the execution backend.
        ROCmBackend::GetInstance().createStream(cfg.name);
        s.uses_rocm_stream = true;
        s.launcher         = std::make_unique<GPULauncher>(ROCmBackend::GetInstance().createBackendFn());
    }

    streams_.emplace(cfg.name, std::move(s));

#ifdef THEMIS_ENABLE_CUDA
    // Create a real CUDA stream for long-running workloads.  Errors are
    // non-fatal: the logical stream still functions via the ROCm/CPU path.
    {
        auto &entry     = streams_.at(cfg.name);
        cudaStream_t cs = nullptr;
        if (cudaStreamCreate(&cs) == cudaSuccess) {
            // Transfer ownership to the RAII guard; no explicit cudaStreamDestroy needed.
            entry.cuda_stream_guard = CudaStreamGuard::adopt(cs);
        }
    }
#endif

    return true;
}

// ----------------------------------------------------------------------------
// createCudaStream — CUDA stream creation (resolves Stubs: 1)
// ----------------------------------------------------------------------------

bool GPUStreamManager::createCudaStream(const StreamConfig &cfg, int device_index) {
    if (cfg.name.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (streams_.count(cfg.name)) {
            return false;
        }
    }

    GPULauncher::BackendFn backend_fn;
    CudaStreamGuard stream_guard; // holds CUDA stream ownership until transferred to Stream

#ifdef THEMIS_ENABLE_CUDA
    // Create a real CUDA stream on the requested device.
    cudaStream_t cuda_stream = nullptr;
    if (cudaSetDevice(device_index) != cudaSuccess) {
        // Device unavailable — fall back to the ROCm/CPU backend.
        backend_fn = ROCmBackend::GetInstance().createBackendFn(device_index);
    } else if (cudaStreamCreate(&cuda_stream) != cudaSuccess) {
        backend_fn = ROCmBackend::GetInstance().createBackendFn(device_index);
    } else {
        // Transfer ownership to RAII guard.  If a TOCTOU duplicate is detected
        // below, the guard destructs the stream automatically when it goes out
        // of scope at the return false path.
        stream_guard = CudaStreamGuard::adopt(cuda_stream);

        // Build a BackendFn that synchronises the CUDA stream after each work
        // item so the caller receives a well-defined completion signal.
        backend_fn = [cuda_stream](const GPULauncher::WorkItem &) -> bool {
            return cudaStreamSynchronize(cuda_stream) == cudaSuccess;
        };
    }
#else
    // PERMANENT HARDWARE FALLBACK NOTE (CUDA not available for stream manager):
    // Purpose: Allow `createCudaStream()` to succeed on non-CUDA builds.  When
    //   `THEMIS_ENABLE_CUDA` is not defined, no `cudaStream_t` is created; the
    //   stream manager delegates to `ROCmBackend::GetInstance().createBackendFn()`
    //   which in turn falls back to CPU async execution when HIP is also absent
    //   (`THEMIS_ENABLE_HIP` not set).
    // Activation: `THEMIS_ENABLE_CUDA` not defined at compile time.
    // Production Delta: Named CUDA streams are unavailable.  Work items submitted
    //   to a "CUDA stream" actually run on the ROCm backend or on the CPU via
    //   `std::async`.  `GPUStreamManager::streamCount()` still reports the stream
    //   as present; callers cannot distinguish CPU from GPU execution via this API.
    // Hardware requirement: CUDA Toolkit + -DTHEMIS_ENABLE_CUDA=1.

    // CUDA not available — try injected CudaStreamBackendFn, fall back to ROCm/CPU.
    {
        CudaStreamBackendFn fn;
        {
            std::lock_guard<std::mutex> lk(s_cuda_backend_fn_mutex);
            fn = s_cuda_backend_fn;
        }
        if (fn) {
            try {
                backend_fn = fn(device_index);
            } catch (...) {
                backend_fn = ROCmBackend::GetInstance().createBackendFn(device_index);
            }
        } else {
            backend_fn = ROCmBackend::GetInstance().createBackendFn(device_index);
        }
    }
#endif

    std::lock_guard<std::mutex> lock(mutex_);
    // Re-check after acquiring the lock (TOCTOU guard).
    if (streams_.count(cfg.name)) {
        // stream_guard destructs here, releasing any CUDA stream that was
        // created before the duplicate was detected — no explicit cleanup needed.
        return false;
    }

    Stream s;
    s.config                = cfg;
    s.cuda_stream_guard     = std::move(stream_guard); // transfer RAII ownership
    s.launcher              = std::make_unique<GPULauncher>(std::move(backend_fn));
    s.stats.name            = cfg.name;
    streams_.emplace(cfg.name, std::move(s));
    return true;
}

bool GPUStreamManager::destroyStream(const std::string &name) {
    bool uses_rocm_stream = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = streams_.find(name);
        if (it == streams_.end()) {
            return false;
        }
        uses_rocm_stream = it->second.uses_rocm_stream;
        // Erase the Stream; its cuda_stream_guard destructor calls
        // cudaStreamSynchronize + cudaStreamDestroy automatically.
        streams_.erase(it);
    }

    if (uses_rocm_stream) {
        ROCmBackend::GetInstance().destroyStream(name);
    }

    return true;
}

bool GPUStreamManager::hasStream(const std::string &name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return streams_.count(name) > 0;
}

std::vector<std::string> GPUStreamManager::streamNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names = {};

    names.reserve(streams_.size());
    for (const auto &kv : streams_) {
        names.push_back(kv.first);
    }
    return names;
}

size_t GPUStreamManager::streamCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return streams_.size();
}

// ============================================================================
// Work submission
// ============================================================================

std::future<GPULauncher::WorkResult> GPUStreamManager::submit(const std::string &stream_name,
                                                              GPULauncher::WorkItem item) {
    // Hold the mutex for the duration of launcher->submit() — that call uses
    // std::async internally and returns a future immediately (non-blocking),
    // so holding the lock here is safe and prevents a concurrent destroyStream()
    // from invalidating the Stream object while we hold a pointer to it.
    uint32_t budget = 0;
    std::future<GPULauncher::WorkResult> inner_fut;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = streams_.find(stream_name);
        if (it == streams_.end()) {
            std::promise<GPULauncher::WorkResult> p;
            GPULauncher::WorkResult r;
            r.success       = false;
            r.kernel_id     = item.kernel_id;
            r.error_message = "stream '" + stream_name + "' does not exist";
            p.set_value(r);
            return p.get_future();
        }
        it->second.stats.submitted++;
        budget    = it->second.config.cpu_budget_ms;
        inner_fut = it->second.launcher->submit(item);
    }

    // Post-process the result asynchronously (no mutex held here).
    return std::async(std::launch::async,
                      [this, stream_name, budget, f = std::move(inner_fut)]() mutable -> GPULauncher::WorkResult {
                          auto res               = f.get();
                          const uint64_t elapsed = static_cast<uint64_t>(res.elapsed.count());

                          std::lock_guard<std::mutex> lock(mutex_);
                          auto it = streams_.find(stream_name);
                          if (it != streams_.end()) {
                              auto &st = it->second.stats;
                              // submitted was already incremented above; only update
                              // outcome counters here.
                              if (res.success) {
                                  ++st.succeeded;
                              } else {
                                  ++st.failed;
                              }
                              st.total_elapsed_ms += elapsed;
                              if (budget > 0 && elapsed > static_cast<uint64_t>(budget)) {
                                  ++st.budget_exceeded;
                              }
                          }
                          return res;
                      });
}

// ============================================================================
// Statistics
// ============================================================================

GPUStreamManager::StreamStats GPUStreamManager::getStreamStats(const std::string &name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(name);
    if (it == streams_.end()) {
        StreamStats empty;
        empty.name = name;
        return empty;
    }
    return it->second.stats;
}

std::vector<GPUStreamManager::StreamStats> GPUStreamManager::getAllStreamStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<StreamStats> result = {};

    result.reserve(streams_.size());
    for (const auto &kv : streams_) {
        result.push_back(kv.second.stats);
    }
    return result;
}

} // namespace gpu
} // namespace themis

