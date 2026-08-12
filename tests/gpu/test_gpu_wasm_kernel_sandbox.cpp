/*
 * Unit tests for WASMKernelSandbox.
 *
 * All tests run on CI without GPU hardware.  The sandbox's CPU simulation
 * path is exercised end-to-end: kernel-validator integration, feature-gate
 * enforcement, memory-limit rejection, timeout enforcement, statistics
 * tracking, and concurrent safety.
 */

#include <gtest/gtest.h>
#include "themis/gpu/wasm_kernel_sandbox.h"
#include "themis/gpu/feature_flags.h"
#include "themis/gpu/kernel_validator.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::gpu;
using Status = WASMKernelSandbox::Status;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<uint8_t> makeBlob(const std::string& s) {
    return {s.begin(), s.end()};
}

// ---------------------------------------------------------------------------
// Fixture
//
// Each test gets a fresh local WASMKernelSandbox and a GPUKernelValidator
// that is reset before and after the test to prevent state leakage through
// the validator singleton.
// ---------------------------------------------------------------------------
class WASMKernelSandboxTest : public ::testing::Test {
protected:
    void SetUp() override {
        GPUKernelValidator::GetInstance().reset();
        // Enable WASM_SANDBOX for all tests (override edition default).
        GPUFeatureFlags::GetInstance().enable(
            GPUFeatureFlags::Feature::WASM_SANDBOX);
    }

    void TearDown() override {
        GPUKernelValidator::GetInstance().reset();
        GPUFeatureFlags::GetInstance().resetToDefaults();
    }

    WASMKernelSandbox sandbox;
};

// ===========================================================================
// Feature flag gate
// ===========================================================================

TEST_F(WASMKernelSandboxTest, FeatureDisabled_RejectsExecution) {
    GPUFeatureFlags::GetInstance().disable(
        GPUFeatureFlags::Feature::WASM_SANDBOX);

    const auto blob = makeBlob("kernel_bytes");
    GPUKernelValidator::GetInstance().registerKernel("k1", blob);

    auto r = sandbox.execute("k1", blob);
    EXPECT_EQ(r.status, Status::REJECTED_FEATURE_DISABLED);
    EXPECT_FALSE(r.ok());

    const auto s = sandbox.getStats();
    EXPECT_EQ(s.total_submitted, 1u);
    EXPECT_EQ(s.rejected_feature_disabled, 1u);
}

TEST_F(WASMKernelSandboxTest, FeatureEnabled_AllowsValidKernel) {
    const auto blob = makeBlob("valid_kernel");
    GPUKernelValidator::GetInstance().registerKernel("valid", blob);

    auto r = sandbox.execute("valid", blob);
    EXPECT_EQ(r.status, Status::OK);
    EXPECT_TRUE(r.ok());
}

// ===========================================================================
// Empty blob rejection
// ===========================================================================

TEST_F(WASMKernelSandboxTest, EmptyBlob_Rejected) {
    GPUKernelValidator::GetInstance().registerKernel("k_empty", 42ULL);

    auto r = sandbox.execute("k_empty", {});
    EXPECT_EQ(r.status, Status::REJECTED_EMPTY_BLOB);
    EXPECT_FALSE(r.ok());

    EXPECT_EQ(sandbox.getStats().rejected_empty, 1u);
}

// ===========================================================================
// Kernel whitelist enforcement
// ===========================================================================

TEST_F(WASMKernelSandboxTest, UnknownKernel_Rejected) {
    const auto blob = makeBlob("some_kernel");
    // Do NOT register "unknown_kernel" with the validator.

    auto r = sandbox.execute("unknown_kernel", blob);
    EXPECT_EQ(r.status, Status::REJECTED_NOT_WHITELISTED);
    EXPECT_EQ(sandbox.getStats().rejected_not_whitelisted, 1u);
}

TEST_F(WASMKernelSandboxTest, ChecksumMismatch_Rejected) {
    const auto canonical = makeBlob("original_kernel");
    GPUKernelValidator::GetInstance().registerKernel("k_cs", canonical);

    const auto tampered = makeBlob("tampered_kernel");
    auto r = sandbox.execute("k_cs", tampered);
    EXPECT_EQ(r.status, Status::REJECTED_CHECKSUM_MISMATCH);
    EXPECT_EQ(sandbox.getStats().rejected_checksum, 1u);
}

// ===========================================================================
// Memory limit enforcement
// ===========================================================================

TEST_F(WASMKernelSandboxTest, BlobExceedsMemoryLimit_Rejected) {
    WASMKernelSandbox::SandboxConfig cfg;
    cfg.memory_limit_bytes = 4;  // Tiny limit so any realistic blob exceeds it.
    cfg.max_execution_ms   = 0;
    sandbox.setConfig(cfg);

    const auto blob = makeBlob("kernel_bigger_than_limit");
    GPUKernelValidator::GetInstance().registerKernel("k_mem", blob);

    auto r = sandbox.execute("k_mem", blob);
    EXPECT_EQ(r.status, Status::REJECTED_MEMORY_LIMIT);
    EXPECT_EQ(sandbox.getStats().rejected_memory_limit, 1u);
}

TEST_F(WASMKernelSandboxTest, ZeroMemoryLimit_DisablesCheck) {
    WASMKernelSandbox::SandboxConfig cfg;
    cfg.memory_limit_bytes = 0;  // Disabled.
    cfg.max_execution_ms   = 0;
    sandbox.setConfig(cfg);

    const auto blob = makeBlob("large_enough_blob");
    GPUKernelValidator::GetInstance().registerKernel("k_nolimit", blob);

    auto r = sandbox.execute("k_nolimit", blob);
    EXPECT_EQ(r.status, Status::OK);
}

// ===========================================================================
// Execution with custom backend
// ===========================================================================

TEST_F(WASMKernelSandboxTest, CustomBackend_SuccessPath) {
    const auto blob = makeBlob("kernel_v1");
    GPUKernelValidator::GetInstance().registerKernel("kb", blob);

    bool called = false;
    GPULauncher::BackendFn fn = [&called](const GPULauncher::WorkItem& item) -> bool {
        called = true;
        return item.kernel_id == "kb";
    };

    auto r = sandbox.execute("kb", blob, fn);
    EXPECT_EQ(r.status, Status::OK);
    EXPECT_TRUE(called);
}

TEST_F(WASMKernelSandboxTest, CustomBackend_FailurePath) {
    const auto blob = makeBlob("kernel_fail");
    GPUKernelValidator::GetInstance().registerKernel("kfail", blob);

    GPULauncher::BackendFn fn = [](const GPULauncher::WorkItem&) -> bool {
        return false;
    };

    auto r = sandbox.execute("kfail", blob, fn);
    EXPECT_EQ(r.status, Status::EXECUTION_ERROR);
    EXPECT_FALSE(r.ok());
    EXPECT_EQ(sandbox.getStats().execution_errors, 1u);
}

// ===========================================================================
// Timeout enforcement
// ===========================================================================

TEST_F(WASMKernelSandboxTest, SlowBackend_TimesOut) {
    WASMKernelSandbox::SandboxConfig cfg;
    cfg.max_execution_ms   = 50;   // 50 ms deadline.
    cfg.memory_limit_bytes = 0;
    sandbox.setConfig(cfg);

    const auto blob = makeBlob("slow_kernel");
    GPUKernelValidator::GetInstance().registerKernel("k_slow", blob);

    GPULauncher::BackendFn fn = [](const GPULauncher::WorkItem&) -> bool {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return true;
    };

    auto r = sandbox.execute("k_slow", blob, fn);
    EXPECT_EQ(r.status, Status::REJECTED_TIMEOUT);
    EXPECT_EQ(sandbox.getStats().rejected_timeout, 1u);
}

TEST_F(WASMKernelSandboxTest, FastBackend_DoesNotTimeout) {
    WASMKernelSandbox::SandboxConfig cfg;
    cfg.max_execution_ms   = 2000;  // 2 s deadline — fast backend won't hit it.
    cfg.memory_limit_bytes = 0;
    sandbox.setConfig(cfg);

    const auto blob = makeBlob("fast_kernel");
    GPUKernelValidator::GetInstance().registerKernel("k_fast", blob);

    auto r = sandbox.execute("k_fast", blob);
    EXPECT_EQ(r.status, Status::OK);
    EXPECT_EQ(sandbox.getStats().rejected_timeout, 0u);
}

// ===========================================================================
// Statistics tracking
// ===========================================================================

TEST_F(WASMKernelSandboxTest, Stats_CountAllOutcomes) {
    const auto blob = makeBlob("stats_kernel");
    GPUKernelValidator::GetInstance().registerKernel("sk", blob);

    sandbox.execute("sk", blob);                             // OK
    sandbox.execute("sk", {});                               // EMPTY
    sandbox.execute("unknown_k", blob);                      // NOT_WHITELISTED
    sandbox.execute("sk", makeBlob("tampered"));             // CHECKSUM_MISMATCH

    const auto s = sandbox.getStats();
    EXPECT_EQ(s.total_submitted, 4u);
    EXPECT_EQ(s.ok_count, 1u);
    EXPECT_EQ(s.rejected_empty, 1u);
    EXPECT_EQ(s.rejected_not_whitelisted, 1u);
    EXPECT_EQ(s.rejected_checksum, 1u);
}

TEST_F(WASMKernelSandboxTest, ResetStats_ClearsCounters) {
    const auto blob = makeBlob("reset_kernel");
    GPUKernelValidator::GetInstance().registerKernel("rk", blob);
    sandbox.execute("rk", blob);

    ASSERT_EQ(sandbox.getStats().total_submitted, 1u);

    sandbox.resetStats();

    const auto s = sandbox.getStats();
    EXPECT_EQ(s.total_submitted, 0u);
    EXPECT_EQ(s.ok_count, 0u);
    EXPECT_EQ(s.total_elapsed_ms, 0u);
}

// ===========================================================================
// Configuration
// ===========================================================================

TEST_F(WASMKernelSandboxTest, SetConfig_UpdatesLimits) {
    WASMKernelSandbox::SandboxConfig cfg;
    cfg.memory_limit_bytes = 1024;
    cfg.max_execution_ms   = 100;
    cfg.allow_host_calls   = true;
    sandbox.setConfig(cfg);

    auto got = sandbox.getConfig();
    EXPECT_EQ(got.memory_limit_bytes, 1024u);
    EXPECT_EQ(got.max_execution_ms, 100u);
    EXPECT_TRUE(got.allow_host_calls);
}

// ===========================================================================
// isWASMSupported
// ===========================================================================

TEST_F(WASMKernelSandboxTest, IsWASMSupported_ReturnsFalseWithoutRuntime) {
    // Without THEMIS_ENABLE_WASM the CPU simulation path is active.
#ifndef THEMIS_ENABLE_WASM
    EXPECT_FALSE(sandbox.isWASMSupported());
#else
    EXPECT_TRUE(sandbox.isWASMSupported());
#endif
}

// ===========================================================================
// sandboxStatusName helper
// ===========================================================================

TEST(WASMKernelSandboxStatusNameTest, AllStatusesHaveNames) {
    using S = WASMKernelSandbox::Status;
    const S all[] = {
        S::OK,
        S::REJECTED_NOT_WHITELISTED,
        S::REJECTED_CHECKSUM_MISMATCH,
        S::REJECTED_EMPTY_BLOB,
        S::REJECTED_FEATURE_DISABLED,
        S::REJECTED_MEMORY_LIMIT,
        S::REJECTED_TIMEOUT,
        S::EXECUTION_ERROR,
    };
    for (auto s : all) {
        const char* name = sandboxStatusName(s);
        EXPECT_NE(name, nullptr);
        EXPECT_STRNE(name, "UNKNOWN");
    }
}

// ===========================================================================
// Feature flags: WASM_SANDBOX in getAll()
// ===========================================================================

TEST(WASMKernelSandboxFeatureFlagTest, WASMSandboxAppearsInGetAll) {
    const auto flags = GPUFeatureFlags::GetInstance().getAll();
    bool found = false;
    for (const auto& fs : flags) {
        if (fs.name == std::string("WASM_SANDBOX")) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "WASM_SANDBOX must appear in GPUFeatureFlags::getAll()";
}

// ===========================================================================
// Thread safety
// ===========================================================================

TEST_F(WASMKernelSandboxTest, ConcurrentExecute_NoDataRace) {
    const auto blob = makeBlob("concurrent_kernel");
    GPUKernelValidator::GetInstance().registerKernel("ck", blob);

    constexpr int THREADS = 8;
    constexpr int OPS     = 10;
    std::atomic<int> ok_count{0};

    auto worker = [&]() {
        for (int i = 0; i < OPS; ++i) {
            auto r = sandbox.execute("ck", blob);
            if (r.ok()) ok_count.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back(worker);
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(ok_count.load(), THREADS * OPS);
    EXPECT_EQ(sandbox.getStats().total_submitted,
              static_cast<size_t>(THREADS * OPS));
}
