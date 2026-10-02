#include <chrono>
#include <cassert>
#include "themis/gpu/profiler.h"
#include "themis/gpu/gpu_timeout.h"
#include "themis/gpu/rocm_backend.h"

int main() {
    {
        themis::gpu::ScopedGPURange scoped("scoped_op");
    }
    const auto ranges = themis::gpu::GPUProfiler::GetInstance().getRanges();
    assert(ranges.size() == 1u);
    assert(ranges[0].name == "scoped_op");
    assert(ranges[0].start_ns < ranges[0].end_ns);

    themis::gpu::KernelSLAGuard guard;
    const auto duration = guard.getSLADuration();
    assert(duration == std::chrono::seconds(5));
    assert(duration.count() == 5);

    auto rec = themis::gpu::ROCmBackend::GetInstance().allocate(128, "probe");
    assert(rec.is_valid());
    const auto stats_before = themis::gpu::ROCmBackend::GetInstance().getStats();
    assert(stats_before.alloc_count == 1u);
    assert(stats_before.bytes_allocated == 128u);
    auto result = themis::gpu::ROCmBackend::GetInstance().deallocate(rec);
    assert(result.ok);
    const auto stats_after = themis::gpu::ROCmBackend::GetInstance().getStats();
    assert(stats_after.dealloc_count == 1u);
    assert(stats_after.bytes_allocated == 0u);
    return 0;
}
