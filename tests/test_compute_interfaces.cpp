// test_compute_interfaces.cpp
//
// Tests for the public acceleration interface extensions defined in:
//   - include/acceleration/compute_future.h
//   - include/acceleration/compute_backend.h (new types/interfaces)
//   - include/acceleration/vulkan_backend.h (IVulkanComputeBackend, PODs)
//
// Acceptance criteria (include/acceleration/FUTURE_ENHANCEMENTS.md):
//   AC-1  DeviceCapabilityFlags bitmask operations produce correct results.
//   AC-2  Unknown bits in DeviceCapabilityFlags are outside KNOWN_VALID_MASK.
//   AC-3  DeviceSet holds up to kMaxDevices entries; push() returns false when full.
//   AC-4  ComputeFuture make_ready() returns a valid, fulfillable future.
//   AC-5  ComputeFuture cancel() is safe after completion (no-op).
//   AC-6  ComputeFuture then() continuation is invokable by the dispatcher.
//   AC-7  DispatchStats helper methods compute correct latencies.
//   AC-8  CancellationToken is copyable and shares the same flag.
//   AC-9  WorkloadDescriptor and LatencyClass are default-constructible.
//   AC-10 BatchDescriptor, KernelConfig, KernelDescriptor are default-constructible.
//   AC-11 SimilarityKernelResult carries per-query top-k results.
//   AC-12 IComputeBackend::submitSimilarityKernel() CPU fallback returns correct results.
//   AC-13 Mock IDeviceCapabilityQuery returns NONE for invalid device index.
//   AC-14 Mock IMultiGPUSelector::selectDevices() is callable from 32 concurrent threads.
//   AC-15 Mock IKernelRegistry registers, resolves, and deregisters kernels.
//   AC-16 IAsyncComputeDispatch mock returns a valid ComputeFuture.
//   AC-17 VulkanPipelineHandle valid() is false for id==0, true otherwise.
//   AC-18 VulkanDeviceInfo is default-constructible with expected zero values.
//   AC-19 VulkanPipelineConfig has sensible defaults (local_size_x=64).
//   AC-20 IComputeBackend::submitSimilarityKernel() respects cancellation token.
//
// No GPU hardware required.  All tests exercise host-side code paths.

#include <gtest/gtest.h>
#include "acceleration/compute_future.h"
#include "acceleration/compute_backend.h"
#include "acceleration/vulkan_backend.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <future>
#include <thread>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// DeviceCapabilityFlags
// =============================================================================

TEST(ComputeInterfaces, DeviceCapabilityFlags_NoneIsZero) {
    static_assert(static_cast<uint32_t>(DeviceCapabilityFlags::NONE) == 0u,
                  "NONE must be zero");
    EXPECT_EQ(static_cast<uint32_t>(DeviceCapabilityFlags::NONE), 0u);
}

TEST(ComputeInterfaces, DeviceCapabilityFlags_BitwiseOr) {
    DeviceCapabilityFlags combined =
        DeviceCapabilityFlags::FLOAT32 | DeviceCapabilityFlags::FLOAT16;
    EXPECT_TRUE(hasCapability(combined, DeviceCapabilityFlags::FLOAT32));
    EXPECT_TRUE(hasCapability(combined, DeviceCapabilityFlags::FLOAT16));
    EXPECT_FALSE(hasCapability(combined, DeviceCapabilityFlags::TENSOR_CORES));
}

TEST(ComputeInterfaces, DeviceCapabilityFlags_BitwiseAnd) {
    DeviceCapabilityFlags a =
        DeviceCapabilityFlags::FLOAT32 | DeviceCapabilityFlags::TENSOR_CORES;
    DeviceCapabilityFlags b =
        DeviceCapabilityFlags::TENSOR_CORES | DeviceCapabilityFlags::GRAPH_CAPTURE;
    DeviceCapabilityFlags intersection = a & b;
    EXPECT_TRUE(hasCapability(intersection, DeviceCapabilityFlags::TENSOR_CORES));
    EXPECT_FALSE(hasCapability(intersection, DeviceCapabilityFlags::FLOAT32));
    EXPECT_FALSE(hasCapability(intersection, DeviceCapabilityFlags::GRAPH_CAPTURE));
}

TEST(ComputeInterfaces, DeviceCapabilityFlags_KnownValidMask) {
    // KNOWN_VALID_MASK should cover all defined flags (bits 0-11).
    const uint32_t mask = static_cast<uint32_t>(DeviceCapabilityFlags::KNOWN_VALID_MASK);
    EXPECT_EQ(mask, 0x0FFFu) << "KNOWN_VALID_MASK must cover bits 0..11";

    // All defined flags fit within the mask.
    auto flags = {
        DeviceCapabilityFlags::FLOAT32,
        DeviceCapabilityFlags::FLOAT16,
        DeviceCapabilityFlags::BFLOAT16,
        DeviceCapabilityFlags::INT8,
        DeviceCapabilityFlags::TENSOR_CORES,
        DeviceCapabilityFlags::WARP_PRIMITIVES,
        DeviceCapabilityFlags::DYNAMIC_PARALLELISM,
        DeviceCapabilityFlags::UNIFIED_MEMORY,
        DeviceCapabilityFlags::PEER_ACCESS,
        DeviceCapabilityFlags::COMPUTE_PREEMPTION,
        DeviceCapabilityFlags::COOPERATIVE_GROUPS,
        DeviceCapabilityFlags::GRAPH_CAPTURE,
    };
    for (auto f : flags) {
        EXPECT_TRUE((static_cast<uint32_t>(f) & mask) != 0u)
            << "Flag " << static_cast<uint32_t>(f) << " must be within KNOWN_VALID_MASK";
    }
}

TEST(ComputeInterfaces, DeviceCapabilityFlags_UnknownBitOutsideMask) {
    // Simulate an unknown bit returned by a future driver.
    const uint32_t unknown_bit = 1u << 16;
    EXPECT_FALSE((unknown_bit & static_cast<uint32_t>(
        DeviceCapabilityFlags::KNOWN_VALID_MASK)) != 0u)
        << "Bit 16 must not be in KNOWN_VALID_MASK";
}

// =============================================================================
// DeviceSet
// =============================================================================

TEST(ComputeInterfaces, DeviceSet_DefaultIsEmpty) {
    DeviceSet s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
}

TEST(ComputeInterfaces, DeviceSet_PushAndAccess) {
    DeviceSet s;
    EXPECT_TRUE(s.push(0u));
    EXPECT_TRUE(s.push(2u));
    EXPECT_TRUE(s.push(3u));
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s[0], 0u);
    EXPECT_EQ(s[1], 2u);
    EXPECT_EQ(s[2], 3u);
}

TEST(ComputeInterfaces, DeviceSet_MaxCapacity) {
    DeviceSet s;
    for (uint32_t i = 0; i < DeviceSet::kMaxDevices; ++i) {
        EXPECT_TRUE(s.push(i)) << "Push " << i << " should succeed";
    }
    EXPECT_EQ(s.size(), DeviceSet::kMaxDevices);
    EXPECT_FALSE(s.push(99u)) << "Push beyond kMaxDevices must fail";
}

TEST(ComputeInterfaces, DeviceSet_FrontBack) {
    DeviceSet s;
    s.push(7u);
    s.push(3u);
    EXPECT_EQ(s.front(), 7u);
    EXPECT_EQ(s.back(), 3u);
}

TEST(ComputeInterfaces, DeviceSet_RangeFor) {
    DeviceSet s;
    s.push(10u);
    s.push(20u);
    std::vector<uint32_t> collected(s.begin(), s.end());
    ASSERT_EQ(collected.size(), 2u);
    EXPECT_EQ(collected[0], 10u);
    EXPECT_EQ(collected[1], 20u);
}

// =============================================================================
// CancellationToken
// =============================================================================

TEST(ComputeInterfaces, CancellationToken_DefaultNotCancelled) {
    CancellationToken tok;
    EXPECT_FALSE(tok.is_cancelled());
    EXPECT_TRUE(tok.valid());
}

TEST(ComputeInterfaces, CancellationToken_CancelSetsFlag) {
    CancellationToken tok;
    tok.cancel();
    EXPECT_TRUE(tok.is_cancelled());
}

TEST(ComputeInterfaces, CancellationToken_CopiesShareFlag) {
    CancellationToken tok;
    CancellationToken copy = tok;  // shares the same atomic<bool>
    tok.cancel();
    EXPECT_TRUE(copy.is_cancelled()) << "Copy must observe cancellation set on original";
}

TEST(ComputeInterfaces, CancellationToken_CancelIsIdempotent) {
    CancellationToken tok;
    tok.cancel();
    tok.cancel();  // second call must not throw
    EXPECT_TRUE(tok.is_cancelled());
}

TEST(ComputeInterfaces, CancellationToken_ConcurrentCancelSafe) {
    CancellationToken tok;
    std::vector<std::thread> threads;
    threads.reserve(16);
    for (int i = 0; i < 16; ++i) {
        threads.emplace_back([&tok]() { tok.cancel(); });
    }
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_TRUE(tok.is_cancelled());
}

// =============================================================================
// DispatchStats
// =============================================================================

TEST(ComputeInterfaces, DispatchStats_DefaultZero) {
    DispatchStats s;
    EXPECT_EQ(s.submit_time_ns,  0u);
    EXPECT_EQ(s.start_time_ns,   0u);
    EXPECT_EQ(s.finish_time_ns,  0u);
    EXPECT_EQ(s.queue_depth,     0u);
    EXPECT_FALSE(s.from_cache);
}

TEST(ComputeInterfaces, DispatchStats_KernelLatency) {
    DispatchStats s;
    s.start_time_ns  = 1000u;
    s.finish_time_ns = 4000u;
    EXPECT_EQ(s.kernel_latency_ns(), 3000u);
}

TEST(ComputeInterfaces, DispatchStats_TotalLatency) {
    DispatchStats s;
    s.submit_time_ns = 500u;
    s.finish_time_ns = 4500u;
    EXPECT_EQ(s.total_latency_ns(), 4000u);
}

TEST(ComputeInterfaces, DispatchStats_ZeroWhenTimestampsUnset) {
    DispatchStats s;
    EXPECT_EQ(s.kernel_latency_ns(), 0u);
    EXPECT_EQ(s.total_latency_ns(),  0u);
}

// =============================================================================
// ComputeFuture<T>
// =============================================================================

TEST(ComputeInterfaces, ComputeFuture_MakeReady) {
    int value = 42;
    auto fut = ComputeFuture<int>::make_ready(value);
    ASSERT_TRUE(fut.valid());
    EXPECT_EQ(fut.get(), 42);
}

TEST(ComputeInterfaces, ComputeFuture_DefaultInvalid) {
    ComputeFuture<int> fut;
    EXPECT_FALSE(fut.valid());
}

TEST(ComputeInterfaces, ComputeFuture_CancelAfterReady_IsNoOp) {
    auto fut = ComputeFuture<int>::make_ready(1);
    fut.cancel();  // safe to call after completion
    EXPECT_TRUE(fut.is_cancelled());
    // get() should still return the value (already fulfilled)
    EXPECT_EQ(fut.get(), 1);
}

TEST(ComputeInterfaces, ComputeFuture_ThenContinuation) {
    auto fut = ComputeFuture<int>::make_ready(7);
    int received = 0;
    fut.then([&received](const int& v) { received = v; });
    fut.invoke_then(7);
    EXPECT_EQ(received, 7);
}

TEST(ComputeInterfaces, ComputeFuture_Stats) {
    DispatchStats stats;
    stats.submit_time_ns  = 100u;
    stats.finish_time_ns  = 900u;
    auto fut = ComputeFuture<int>::make_ready(0, stats);
    EXPECT_EQ(fut.stats().submit_time_ns, 100u);
    EXPECT_EQ(fut.stats().finish_time_ns, 900u);
}

TEST(ComputeInterfaces, ComputeFuture_AsyncProducer) {
    std::promise<std::string> prom;
    auto sfut = prom.get_future().share();
    ComputeFuture<std::string> fut(std::move(sfut), CancellationToken{}, DispatchStats{});

    std::thread producer([&prom]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        prom.set_value("hello");
    });
    EXPECT_EQ(fut.get(), "hello");
    producer.join();
}

TEST(ComputeInterfaces, ComputeFuture_MakeExceptional) {
    auto fut = ComputeFuture<int>::make_exceptional(
        std::make_exception_ptr(std::runtime_error("test error")));
    ASSERT_TRUE(fut.valid());
    EXPECT_THROW(fut.get(), std::runtime_error);
}

// =============================================================================
// WorkloadDescriptor and LatencyClass
// =============================================================================

TEST(ComputeInterfaces, WorkloadDescriptor_DefaultValues) {
    WorkloadDescriptor wd;
    EXPECT_EQ(wd.byte_size,     0u);
    EXPECT_EQ(wd.flop_estimate, 0u);
    EXPECT_EQ(wd.latency_class, LatencyClass::BATCH);
    EXPECT_EQ(wd.precision,     PrecisionMode::FP32);
}

TEST(ComputeInterfaces, LatencyClass_Values) {
    EXPECT_EQ(static_cast<uint8_t>(LatencyClass::INTERACTIVE), 0u);
    EXPECT_EQ(static_cast<uint8_t>(LatencyClass::BATCH),       1u);
    EXPECT_EQ(static_cast<uint8_t>(LatencyClass::BACKGROUND),  2u);
}

// =============================================================================
// BatchDescriptor, KernelConfig, KernelDescriptor
// =============================================================================

TEST(ComputeInterfaces, BatchDescriptor_DefaultNullPointers) {
    BatchDescriptor bd;
    EXPECT_EQ(bd.queries,     nullptr);
    EXPECT_EQ(bd.vectors,     nullptr);
    EXPECT_EQ(bd.num_queries, 0u);
    EXPECT_EQ(bd.num_vectors, 0u);
    EXPECT_EQ(bd.dim,         0u);
    EXPECT_EQ(bd.k,           1u);
}

TEST(ComputeInterfaces, KernelConfig_DefaultValues) {
    KernelConfig cfg;
    EXPECT_EQ(cfg.block_size,  256u);
    EXPECT_EQ(cfg.grid_size,   0u);
    EXPECT_EQ(cfg.shared_mem,  0u);
    EXPECT_EQ(cfg.metric,      DistanceMetric::L2);
    EXPECT_EQ(cfg.precision,   PrecisionMode::FP32);
    EXPECT_FALSE(cfg.async_exec);
}

TEST(ComputeInterfaces, KernelDescriptor_DefaultValues) {
    KernelDescriptor kd;
    EXPECT_TRUE(kd.kernel_name.empty());
    EXPECT_EQ(kd.batch.num_queries, 0u);
    EXPECT_EQ(kd.config.block_size, 256u);
}

// =============================================================================
// SimilarityKernelResult
// =============================================================================

TEST(ComputeInterfaces, SimilarityKernelResult_DefaultValues) {
    SimilarityKernelResult r;
    EXPECT_TRUE(r.results.empty());
    EXPECT_EQ(r.metric_used,    DistanceMetric::L2);
    EXPECT_EQ(r.precision_used, PrecisionMode::FP32);
    EXPECT_FALSE(r.used_hw_path);
    EXPECT_NEAR(r.speedup_vs_cpu, 1.0, 1e-9);
}

// =============================================================================
// IComputeBackend::submitSimilarityKernel() — CPU fallback default
// =============================================================================

namespace {

// Minimal concrete IComputeBackend subclass for testing the default
// submitSimilarityKernel() implementation (CPU fallback path).
class TestBackend : public IComputeBackend {
public:
    const char*        name()   const noexcept override { return "Test"; }
    BackendType        type()   const noexcept override { return BackendType::CPU; }
    bool isAvailable() const noexcept override { return true; }

    BackendCapabilities getCapabilities() const override {
        BackendCapabilities caps;
        caps.supportsVectorOps = true;
        caps.supportedPrecisions = PrecisionMode::FP32;
        return caps;
    }

    bool initialize() override { return true; }
    void shutdown()   override {}
};

}  // namespace

TEST(ComputeInterfaces, SubmitSimilarityKernel_EmptyBatch_ReturnsEmptyResults) {
    TestBackend backend;
    BatchDescriptor bd;   // all null / zero
    KernelConfig cfg;
    auto fut = backend.submitSimilarityKernel(bd, cfg);
    ASSERT_TRUE(fut.valid());
    auto result = fut.get();
    EXPECT_TRUE(result.results.empty());
    EXPECT_FALSE(result.used_hw_path);
}

TEST(ComputeInterfaces, SubmitSimilarityKernel_L2_OneQuery) {
    TestBackend backend;

    // 3 corpus vectors in 2D; query is at origin.
    const std::vector<float> vectors = {
        1.0f, 0.0f,   // id 0: dist = 1
        0.0f, 2.0f,   // id 1: dist = 4
        3.0f, 4.0f,   // id 2: dist = 25
    };
    const std::vector<float> queries = {0.0f, 0.0f};

    BatchDescriptor bd;
    bd.queries     = queries.data();
    bd.num_queries = 1u;
    bd.dim         = 2u;
    bd.vectors     = vectors.data();
    bd.num_vectors = 3u;
    bd.k           = 2u;

    KernelConfig cfg;
    cfg.metric = DistanceMetric::L2;

    auto fut = backend.submitSimilarityKernel(bd, cfg);
    ASSERT_TRUE(fut.valid());
    auto result = fut.get();

    ASSERT_EQ(result.results.size(), 1u);
    ASSERT_EQ(result.results[0].size(), 2u);

    // Results should be sorted ascending by L2 distance.
    EXPECT_EQ(result.results[0][0].first, 0u);  // id 0: dist=1 (closest)
    EXPECT_EQ(result.results[0][1].first, 1u);  // id 1: dist=4

    EXPECT_LE(result.results[0][0].second, result.results[0][1].second)
        << "Results must be sorted ascending by distance";
}

TEST(ComputeInterfaces, SubmitSimilarityKernel_KClampedToNumVectors) {
    TestBackend backend;

    const std::vector<float> vectors = {1.0f, 2.0f};  // 2 vectors in 1D
    const std::vector<float> queries = {0.0f};

    BatchDescriptor bd;
    bd.queries     = queries.data();
    bd.num_queries = 1u;
    bd.dim         = 1u;
    bd.vectors     = vectors.data();
    bd.num_vectors = 2u;
    bd.k           = 100u;  // k > num_vectors — should be clamped

    KernelConfig cfg;

    auto fut = backend.submitSimilarityKernel(bd, cfg);
    auto result = fut.get();

    ASSERT_EQ(result.results.size(), 1u);
    EXPECT_LE(result.results[0].size(), 2u)
        << "Result count must not exceed num_vectors";
}

TEST(ComputeInterfaces, SubmitSimilarityKernel_CancellationHonoured) {
    TestBackend backend;

    // Large enough batch that cancellation can kick in before completion.
    constexpr size_t N = 1000u;
    std::vector<float> vectors(N * 4u, 1.0f);
    std::vector<float> queries(4u, 0.0f);

    BatchDescriptor bd;
    bd.queries     = queries.data();
    bd.num_queries = 1u;
    bd.dim         = 4u;
    bd.vectors     = vectors.data();
    bd.num_vectors = N;
    bd.k           = 5u;

    KernelConfig cfg;

    // Pre-cancel the token before submitting.
    CancellationToken tok;
    tok.cancel();

    auto fut = backend.submitSimilarityKernel(bd, cfg, tok);
    ASSERT_TRUE(fut.valid());
    auto result = fut.get();

    // The future should be valid even when cancelled; the CPU path may return
    // a partial or empty result.
    EXPECT_TRUE(result.results.empty() || result.results[0].empty())
        << "Cancelled kernel should return empty result set";
}

// =============================================================================
// Mock IDeviceCapabilityQuery
// =============================================================================

namespace {

class MockDeviceCapabilityQuery : public IDeviceCapabilityQuery {
public:
    DeviceCapabilityFlags queryCapabilities(int device_index) const noexcept override {
        if (device_index < 0 || device_index >= static_cast<int>(capabilities_.size()))
            return DeviceCapabilityFlags::NONE;
        return capabilities_[device_index];
    }

    std::vector<DeviceCapabilityFlags> queryAll() const override {
        return capabilities_;
    }

    void addDevice(DeviceCapabilityFlags flags) {
        capabilities_.push_back(flags);
    }

private:
    std::vector<DeviceCapabilityFlags> capabilities_;
};

}  // namespace

TEST(ComputeInterfaces, MockDeviceCapabilityQuery_InvalidIndexReturnsNone) {
    MockDeviceCapabilityQuery q;
    EXPECT_EQ(q.queryCapabilities(-1), DeviceCapabilityFlags::NONE);
    EXPECT_EQ(q.queryCapabilities(0),  DeviceCapabilityFlags::NONE);
}

TEST(ComputeInterfaces, MockDeviceCapabilityQuery_ValidIndex) {
    MockDeviceCapabilityQuery q;
    q.addDevice(DeviceCapabilityFlags::FLOAT32 | DeviceCapabilityFlags::TENSOR_CORES);
    EXPECT_TRUE(hasCapability(q.queryCapabilities(0), DeviceCapabilityFlags::FLOAT32));
    EXPECT_TRUE(hasCapability(q.queryCapabilities(0), DeviceCapabilityFlags::TENSOR_CORES));
    EXPECT_FALSE(hasCapability(q.queryCapabilities(0), DeviceCapabilityFlags::FLOAT16));
}

TEST(ComputeInterfaces, MockDeviceCapabilityQuery_QueryAll) {
    MockDeviceCapabilityQuery q;
    q.addDevice(DeviceCapabilityFlags::FLOAT32);
    q.addDevice(DeviceCapabilityFlags::FLOAT16 | DeviceCapabilityFlags::BFLOAT16);
    auto all = q.queryAll();
    ASSERT_EQ(all.size(), 2u);
    EXPECT_TRUE(hasCapability(all[1], DeviceCapabilityFlags::FLOAT16));
}

// =============================================================================
// Mock IMultiGPUSelector — thread-safety test (32 concurrent callers)
// =============================================================================

namespace {

class MockMultiGPUSelector : public IMultiGPUSelector {
public:
    explicit MockMultiGPUSelector(uint32_t num_devices)
        : num_devices_(num_devices) {}

    DeviceSet selectDevices(const WorkloadDescriptor& workload) const override {
        // Thread-safe: reads only immutable state.
        DeviceSet s = {};
        if (num_devices_ == 0) {
          return s;
        }
        // For INTERACTIVE: return single best device; otherwise all devices.
        if (workload.latency_class == LatencyClass::INTERACTIVE) {
            s.push(0u);
        } else {
            for (uint32_t i = 0; i < std::min(num_devices_, static_cast<uint32_t>(DeviceSet::kMaxDevices)); ++i)
                s.push(i);
        }
        return s;
    }

    uint32_t deviceCount() const noexcept override { return num_devices_; }

private:
    uint32_t num_devices_;
};

}  // namespace

TEST(ComputeInterfaces, MockMultiGPUSelector_InteractiveSelectsOne) {
    MockMultiGPUSelector sel(4u);
    WorkloadDescriptor wd;
    wd.latency_class = LatencyClass::INTERACTIVE;
    auto devices = sel.selectDevices(wd);
    EXPECT_EQ(devices.size(), 1u);
}

TEST(ComputeInterfaces, MockMultiGPUSelector_BatchSelectsAll) {
    MockMultiGPUSelector sel(3u);
    WorkloadDescriptor wd;
    wd.latency_class = LatencyClass::BATCH;
    auto devices = sel.selectDevices(wd);
    EXPECT_EQ(devices.size(), 3u);
}

TEST(ComputeInterfaces, MockMultiGPUSelector_EmptyWhenNoDevices) {
    MockMultiGPUSelector sel(0u);
    WorkloadDescriptor wd;
    auto devices = sel.selectDevices(wd);
    EXPECT_TRUE(devices.empty());
}

TEST(ComputeInterfaces, MockMultiGPUSelector_ThreadSafety_32Threads) {
    // 32 concurrent threads each call selectDevices() once.
    // The selector is read-only so no lock is needed in the mock, but the
    // test verifies that all threads receive a non-empty DeviceSet.
    MockMultiGPUSelector sel(8u);
    WorkloadDescriptor wd;
    wd.latency_class = LatencyClass::BATCH;

    constexpr int kNumThreads = 32;
    std::vector<DeviceSet> results(kNumThreads);
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([&sel, &wd, &results, i]() {
            results[i] = sel.selectDevices(wd);
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    for (int i = 0; i < kNumThreads; ++i) {
        EXPECT_FALSE(results[i].empty())
            << "Thread " << i << " should receive at least one device";
        EXPECT_EQ(results[i].size(), 8u)
            << "Thread " << i << " should receive all 8 devices";
    }
}

// =============================================================================
// Mock IKernelRegistry
// =============================================================================

namespace {

class MockKernelRegistry : public IKernelRegistry {
public:
    bool registerKernel(std::string name, void* fn_ptr) override {
        if (kernels_.count(name)) {
          return false;
        }
        kernels_[std::move(name)] = fn_ptr;
        return true;
    }

    void* resolveKernel(const std::string& name) const override {
        auto it = kernels_.find(name);
        return (it != kernels_.end()) ? it->second : nullptr;
    }

    bool hasKernel(const std::string& name) const noexcept override {
        return kernels_.count(name) != 0;
    }

    bool deregisterKernel(const std::string& name) noexcept override {
        return kernels_.erase(name) > 0;
    }

private:
    std::map<std::string, void*> kernels_;
};

}  // namespace

TEST(ComputeInterfaces, KernelRegistry_RegisterAndResolve) {
    MockKernelRegistry reg;
    int dummy = 0;
    EXPECT_TRUE(reg.registerKernel("cuda/l2_distance", &dummy));
    EXPECT_EQ(reg.resolveKernel("cuda/l2_distance"), &dummy);
}

TEST(ComputeInterfaces, KernelRegistry_DuplicateRegisterFails) {
    MockKernelRegistry reg;
    int a = 0, b = 0;
    EXPECT_TRUE(reg.registerKernel("test/kernel", &a));
    EXPECT_FALSE(reg.registerKernel("test/kernel", &b));
    // Original pointer still resolved.
    EXPECT_EQ(reg.resolveKernel("test/kernel"), &a);
}

TEST(ComputeInterfaces, KernelRegistry_UnknownNameReturnsNull) {
    MockKernelRegistry reg;
    EXPECT_EQ(reg.resolveKernel("nonexistent"), nullptr);
    EXPECT_FALSE(reg.hasKernel("nonexistent"));
}

TEST(ComputeInterfaces, KernelRegistry_Deregister) {
    MockKernelRegistry reg;
    int dummy = 0;
    reg.registerKernel("k", &dummy);
    EXPECT_TRUE(reg.deregisterKernel("k"));
    EXPECT_FALSE(reg.hasKernel("k"));
    EXPECT_FALSE(reg.deregisterKernel("k"));  // already gone
}

// =============================================================================
// Mock IAsyncComputeDispatch
// =============================================================================

namespace {

class MockAsyncDispatch : public IAsyncComputeDispatch {
public:
    ComputeFuture<SimilarityKernelResult>
    submit(const KernelDescriptor& descriptor,
           CancellationToken        token = {}) override {
        ++submit_count_;
        SimilarityKernelResult res;
        res.used_hw_path   = false;
        res.speedup_vs_cpu = 1.0;
        if (descriptor.batch.num_queries > 0) {
            res.results.resize(descriptor.batch.num_queries);
        }
        std::promise<SimilarityKernelResult> promise;
        promise.set_value(std::move(res));
        return ComputeFuture<SimilarityKernelResult>(
            promise.get_future().share(), std::move(token), {});
    }

    int submit_count_ = 0;
};

}  // namespace

TEST(ComputeInterfaces, AsyncDispatch_SubmitReturnsValidFuture) {
    MockAsyncDispatch dispatcher;
    KernelDescriptor kd;
    kd.batch.num_queries = 3u;
    auto fut = dispatcher.submit(kd);
    ASSERT_TRUE(fut.valid());
    auto result = fut.get();
    EXPECT_EQ(result.results.size(), 3u);
    EXPECT_EQ(dispatcher.submit_count_, 1);
}

TEST(ComputeInterfaces, AsyncDispatch_CancellationTokenPassedThrough) {
    MockAsyncDispatch dispatcher;
    KernelDescriptor kd;
    CancellationToken tok;
    auto fut = dispatcher.submit(kd, tok);
    tok.cancel();
    EXPECT_TRUE(fut.is_cancelled());
}

// =============================================================================
// VulkanPipelineHandle
// =============================================================================

TEST(ComputeInterfaces, VulkanPipelineHandle_DefaultInvalid) {
    VulkanPipelineHandle h;
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(h.id, 0u);
}

TEST(ComputeInterfaces, VulkanPipelineHandle_NonZeroValid) {
    VulkanPipelineHandle h;
    h.id = 42u;
    EXPECT_TRUE(h.valid());
}

TEST(ComputeInterfaces, VulkanPipelineHandle_Equality) {
    VulkanPipelineHandle a, b;
    EXPECT_EQ(a, b);
    a.id = 1u;
    EXPECT_NE(a, b);
}

// =============================================================================
// VulkanPipelineConfig defaults
// =============================================================================

TEST(ComputeInterfaces, VulkanPipelineConfig_DefaultLocalSizes) {
    VulkanPipelineConfig cfg;
    EXPECT_EQ(cfg.local_size_x,    64u);
    EXPECT_EQ(cfg.local_size_y,    1u);
    EXPECT_EQ(cfg.local_size_z,    1u);
    EXPECT_EQ(cfg.push_const_size, 0u);
    EXPECT_FALSE(cfg.enable_fp16);
    EXPECT_FALSE(cfg.enable_int8);
}

// =============================================================================
// VulkanDeviceInfo defaults
// =============================================================================

TEST(ComputeInterfaces, VulkanDeviceInfo_DefaultZero) {
    VulkanDeviceInfo info;
    EXPECT_EQ(info.device_index, 0u);
    EXPECT_EQ(info.vendor_id,    0u);
    EXPECT_EQ(info.vram_bytes,   0u);
    EXPECT_FALSE(info.supports_fp16);
    EXPECT_FALSE(info.supports_int8);
    EXPECT_FALSE(info.is_discrete);
    EXPECT_EQ(info.device_name[0], '\0');
}

// =============================================================================
// Trivially-copyable static assertions (header-only compile test)
// =============================================================================

TEST(ComputeInterfaces, TrivialCopyability_PODTypes) {
    // These structs are used in tight performance loops; verify they are
    // trivially copyable (or explicitly documented as non-trivial).
    static_assert(std::is_trivially_copyable<DeviceSet>::value,
                  "DeviceSet must be trivially copyable");
    static_assert(std::is_trivially_copyable<DispatchStats>::value,
                  "DispatchStats must be trivially copyable");
    static_assert(std::is_trivially_copyable<VulkanPipelineHandle>::value,
                  "VulkanPipelineHandle must be trivially copyable");
    static_assert(std::is_trivially_copyable<VulkanPipelineConfig>::value,
                  "VulkanPipelineConfig must be trivially copyable");
    static_assert(std::is_trivially_copyable<VulkanDeviceInfo>::value,
                  "VulkanDeviceInfo must be trivially copyable");
    static_assert(std::is_trivially_copyable<KernelConfig>::value,
                  "KernelConfig must be trivially copyable");
    // Not trivially copyable (has std::string): BatchDescriptor,
    // WorkloadDescriptor, KernelDescriptor, SimilarityKernelResult —
    // these are documented as non-trivial.
    SUCCEED();
}
