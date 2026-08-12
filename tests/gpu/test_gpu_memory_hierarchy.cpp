#include <gtest/gtest.h>
#include <string>
#include "themis/gpu/ivram_policy.h"
#include "themis/gpu/memory_manager.h"
#include "llm/gpu_memory_manager.h"

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void DrainCanonicalManager() {
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t used = mgr.GetGPUMemoryUsed();
    if (used > 0) {
        mgr.DeallocateGPU(used);
    }
    for (const auto& ts : mgr.GetAllTenantStats()) {
        mgr.RemoveTenantQuota(ts.tenant_id);
    }
}

// ---------------------------------------------------------------------------
// IVRAMPolicy interface contract tests
// ---------------------------------------------------------------------------

class IVRAMPolicyTest : public ::testing::Test {
protected:
    void SetUp() override { DrainCanonicalManager(); }
    void TearDown() override { DrainCanonicalManager(); }
};

TEST_F(IVRAMPolicyTest, CanonicalManagerImplementsInterface) {
    // GPUMemoryManager must be usable through the IVRAMPolicy interface pointer.
    IVRAMPolicy* policy = &GPUMemoryManager::GetInstance();
    ASSERT_NE(policy, nullptr);
}

TEST_F(IVRAMPolicyTest, IsGPUEnabled_MatchesEditionConstant) {
    IVRAMPolicy& policy = GPUMemoryManager::GetInstance();
    EXPECT_EQ(policy.isGPUEnabled(), GPUMemoryManager::GetMaxGPUVRAMGB() > 0);
}

TEST_F(IVRAMPolicyTest, UsedBytes_InitiallyZero) {
    IVRAMPolicy& policy = GPUMemoryManager::GetInstance();
    EXPECT_EQ(policy.usedBytes(), 0u);
}

TEST_F(IVRAMPolicyTest, OnAllocate_IncreasesUsedBytes) {
    if (!GPUMemoryManager::GetInstance().isGPUEnabled()) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=edition_limit_zero";
    }
    IVRAMPolicy& policy = GPUMemoryManager::GetInstance();
    const uint64_t kOneMB = 1024ULL * 1024ULL;
    policy.onAllocate(kOneMB, "test-tag");
    EXPECT_EQ(policy.usedBytes(), kOneMB);
}

TEST_F(IVRAMPolicyTest, OnDeallocate_DecreasesUsedBytes) {
    if (!GPUMemoryManager::GetInstance().isGPUEnabled()) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=edition_limit_zero";
    }
    IVRAMPolicy& policy = GPUMemoryManager::GetInstance();
    const uint64_t kOneMB = 1024ULL * 1024ULL;
    policy.onAllocate(kOneMB, "test-tag");
    ASSERT_EQ(policy.usedBytes(), kOneMB);
    policy.onDeallocate(kOneMB);
    EXPECT_EQ(policy.usedBytes(), 0u);
}

TEST_F(IVRAMPolicyTest, CanAllocate_ReturnsTrueWhenBelowLimit) {
    if (!GPUMemoryManager::GetInstance().isGPUEnabled()) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=edition_limit_zero";
    }
    IVRAMPolicy& policy = GPUMemoryManager::GetInstance();
    EXPECT_TRUE(policy.canAllocate(1024ULL * 1024ULL));
}

TEST_F(IVRAMPolicyTest, CanAllocate_ReturnsFalseWhenAboveLimit) {
    IVRAMPolicy& policy = GPUMemoryManager::GetInstance();
    // Requesting UINT64_MAX bytes must always fail regardless of edition.
    EXPECT_FALSE(policy.canAllocate(std::numeric_limits<uint64_t>::max()));
}

TEST_F(IVRAMPolicyTest, TenantIsolation_ViaInterface) {
    if (!GPUMemoryManager::GetInstance().isGPUEnabled()) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=edition_limit_zero";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    IVRAMPolicy& policy = mgr;

    const std::string tenant = "test-tenant-hierarchy";
    const uint64_t kQuota = 4ULL * 1024ULL * 1024ULL;  // 4 MB
    mgr.SetTenantQuota(tenant, kQuota);

    // Within quota: canAllocate should return true.
    EXPECT_TRUE(policy.canAllocate(kQuota, tenant));

    // Exceeds quota: canAllocate should return false.
    EXPECT_FALSE(policy.canAllocate(kQuota + 1, tenant));

    mgr.RemoveTenantQuota(tenant);
}

// ---------------------------------------------------------------------------
// Cross-namespace accounting: LLM manager registers with canonical
// ---------------------------------------------------------------------------

class LLMToCanonicalDelegationTest : public ::testing::Test {
protected:
    void SetUp() override {
        DrainCanonicalManager();
    }
    void TearDown() override {
        DrainCanonicalManager();
    }
};

TEST_F(LLMToCanonicalDelegationTest, CanonicalManagerIsGPUEnabledConsistency) {
    // The canonical manager's isGPUEnabled() result should be usable as a
    // prerequisite gate in tests that exercise cross-namespace delegation.
    auto& canonical = GPUMemoryManager::GetInstance();
    // Just validate the getter returns a consistent value.
    bool first  = canonical.isGPUEnabled();
    bool second = canonical.isGPUEnabled();
    EXPECT_EQ(first, second);
}

TEST_F(LLMToCanonicalDelegationTest, LLMManagerAllocate_GatedByCanonical) {
    // When the canonical manager has the global VRAM budget exhausted,
    // the LLM manager must reject new allocations.
    auto& canonical = GPUMemoryManager::GetInstance();
    if (!canonical.isGPUEnabled()) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=edition_limit_zero";
    }

    // Fill the canonical budget completely.
    const uint64_t limit = GPUMemoryManager::GetMaxGPUVRAMBytes();
    ASSERT_TRUE(canonical.TryAllocateGPU(limit, "fill-budget"));
    EXPECT_EQ(canonical.GetGPUMemoryUsed(), limit);

    // Now attempt an LLM-manager allocation; it should be rejected because
    // the canonical policy has no headroom left.
    themis::llm::GPUMemoryManager::Config cfg;
    cfg.max_vram_bytes = 64ULL * 1024 * 1024 * 1024;  // plenty of local room
    themis::llm::GPUMemoryManager llm_mgr(cfg);

    void* ptr = llm_mgr.allocateGPU("model-x", 1024 * 1024);
    EXPECT_EQ(ptr, nullptr) << "Expected nullptr when canonical budget is exhausted";

    // Clean up.
    canonical.DeallocateGPU(limit);
}

// ---------------------------------------------------------------------------
// Tenant isolation via canonical manager quota propagation
// ---------------------------------------------------------------------------

TEST_F(LLMToCanonicalDelegationTest, TenantQuota_EnforcedAcrossHierarchy) {
    auto& canonical = GPUMemoryManager::GetInstance();
    if (!canonical.isGPUEnabled()) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=edition_limit_zero";
    }

    // Register a tiny per-tenant quota on the canonical manager.
    const std::string tenant = "llm-hierarchy-tenant";
    const uint64_t kQuotaBytes = 2ULL * 1024 * 1024;  // 2 MB
    canonical.SetTenantQuota(tenant, kQuotaBytes);

    // The canonical manager must reject any request that would exceed the quota.
    EXPECT_TRUE(canonical.TryAllocateGPU(kQuotaBytes, "alloc-ok", tenant));
    EXPECT_FALSE(canonical.TryAllocateGPU(1, "alloc-denied", tenant))
        << "Second allocation should exceed tenant quota";

    // Cleanup.
    canonical.DeallocateGPU(kQuotaBytes, tenant);
    canonical.RemoveTenantQuota(tenant);
}

// ---------------------------------------------------------------------------
// IVRAMPolicy::onAllocate/onDeallocate consistency
// ---------------------------------------------------------------------------

TEST_F(IVRAMPolicyTest, OnAllocate_WithTenant_UpdatesTenantStats) {
    if (!GPUMemoryManager::GetInstance().isGPUEnabled()) {
        GTEST_SKIP() << "capability:gpu_edition_enabled=false;reason=edition_limit_zero";
    }
    auto& mgr = GPUMemoryManager::GetInstance();
    IVRAMPolicy& policy = mgr;

    const std::string tenant = "policy-tenant";
    const uint64_t kBytes = 512ULL * 1024;  // 512 KB
    mgr.SetTenantQuota(tenant, kBytes * 4);  // enough room

    policy.onAllocate(kBytes, "lora-layer", tenant);
    EXPECT_EQ(policy.usedBytes(), kBytes);

    auto ts = mgr.GetTenantStats(tenant);
    EXPECT_EQ(ts.allocated_bytes, kBytes);

    policy.onDeallocate(kBytes, tenant);
    EXPECT_EQ(policy.usedBytes(), 0u);

    ts = mgr.GetTenantStats(tenant);
    EXPECT_EQ(ts.allocated_bytes, 0u);

    mgr.RemoveTenantQuota(tenant);
}
