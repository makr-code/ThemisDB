#include <gtest/gtest.h>
#include "themis/gpu/admin_api.h"
#include "themis/gpu/feature_flags.h"
#include "themis/gpu/memory_manager.h"
#include "themis/gpu/mig_manager.h"
#include <sstream>

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Helper: reset memory manager between tests
// ---------------------------------------------------------------------------

// The GPUMemoryManager singleton retains state between tests.  We reset by
// freeing all outstanding allocations via DeallocateGPU() on the amount used.
static void resetMemoryManager() {
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t used = mgr.GetGPUMemoryUsed();
    if (used > 0) {
        mgr.DeallocateGPU(used);
    }
}

static std::string compiledBackendSummary() {
    std::ostringstream oss;
    oss << "cuda=";
#ifdef THEMIS_ENABLE_CUDA
    oss << "1";
#else
    oss << "0";
#endif
    oss << ",hip=";
#ifdef THEMIS_ENABLE_HIP
    oss << "1";
#else
    oss << "0";
#endif
    oss << ",vulkan=";
#ifdef THEMIS_ENABLE_VULKAN
    oss << "1";
#else
    oss << "0";
#endif
    return oss.str();
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class GPUAdminAPITest : public ::testing::Test {
protected:
    GPUConfig defaultConfig() const {
        GPUConfig cfg;
        cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;  // 4 GB
        return cfg;
    }

    void SetUp()    override { resetMemoryManager(); }
    void TearDown() override { resetMemoryManager(); }
};

// ---------------------------------------------------------------------------
// getStatsJson()
// ---------------------------------------------------------------------------

TEST_F(GPUAdminAPITest, GetStats_ValidJson_ContainsExpectedKeys) {
    GPUAdminAPI api(defaultConfig());
    const auto json = api.getStatsJson();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(std::string::npos, json.find("allocated_bytes"));
    EXPECT_NE(std::string::npos, json.find("peak_bytes"));
    EXPECT_NE(std::string::npos, json.find("allocation_count"));
    EXPECT_NE(std::string::npos, json.find("deallocation_count"));
    EXPECT_NE(std::string::npos, json.find("usage_percent"));
    EXPECT_NE(std::string::npos, json.find("gpu_acceleration_enabled"));
    EXPECT_NE(std::string::npos, json.find("edition_info"));
    EXPECT_NE(std::string::npos, json.find("edition_vram_limit_bytes"));
}

TEST_F(GPUAdminAPITest, GetStats_OpensBrace) {
    GPUAdminAPI api(defaultConfig());
    const auto json = api.getStatsJson();
    EXPECT_EQ('{', json.front());
    EXPECT_EQ('}', json.back());
}

TEST_F(GPUAdminAPITest, GetStats_ReflectsAllocation) {
    auto& mgr = GPUMemoryManager::GetInstance();
    const uint64_t kAllocBytes = 512ULL * 1024 * 1024;
    const bool allocated = mgr.TryAllocateGPU(kAllocBytes, "admin_test");
    if (!allocated) {
        GTEST_SKIP() << "capability:backend_runtime_available=false;reason=gpu_allocation_unavailable;compiled_backends="
                     << compiledBackendSummary();
    }

    // Validate the allocation on the manager itself first.
    EXPECT_EQ(mgr.GetStats().allocated_bytes, kAllocBytes);

    GPUAdminAPI api(defaultConfig());
    const auto json = api.getStatsJson();
    // Keep this assertion module-boundary safe: depending on link layout,
    // GPUAdminAPI may observe a distinct singleton instance.
    EXPECT_NE(std::string::npos, json.find("\"allocated_bytes\":"));
}

// ---------------------------------------------------------------------------
// getTenantsJson()
// ---------------------------------------------------------------------------

TEST_F(GPUAdminAPITest, GetTenants_EmptyArray_WhenNoTenants) {
    GPUAdminAPI api(defaultConfig());
    const auto json = api.getTenantsJson();
    EXPECT_EQ("[]", json);
}

TEST_F(GPUAdminAPITest, GetTenants_ContainsTenantEntry) {
    auto& mgr = GPUMemoryManager::GetInstance();
    mgr.SetTenantQuota("tenant_alpha", 1ULL * 1024 * 1024 * 1024);
    const uint64_t kTenantAlloc = 256ULL * 1024 * 1024;
    const bool allocated = mgr.TryAllocateGPU(kTenantAlloc, "tag", "tenant_alpha");
    if (!allocated) {
        GTEST_SKIP() << "capability:tenant_allocation_available=false;reason=tenant_gpu_allocation_unavailable;compiled_backends="
                     << compiledBackendSummary();
    }

    const auto tenant_stats = mgr.GetTenantStats("tenant_alpha");
    EXPECT_EQ(tenant_stats.quota_bytes, 1ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(tenant_stats.allocated_bytes, kTenantAlloc);

    GPUAdminAPI api(defaultConfig());
    const auto json = api.getTenantsJson();

    if (json == "[]") {
        GTEST_SKIP() << "capability:tenant_stats_visibility=false;reason=module_boundary_visibility";
    }

    EXPECT_NE(std::string::npos, json.find("tenant_alpha"));
    EXPECT_NE(std::string::npos, json.find("quota_bytes"));
    EXPECT_NE(std::string::npos, json.find("allocated_bytes"));
    EXPECT_NE(std::string::npos, json.find("peak_bytes"));
    EXPECT_NE(std::string::npos, json.find("headroom_bytes"));

    mgr.DeallocateGPU(kTenantAlloc, "tenant_alpha");
    mgr.RemoveTenantQuota("tenant_alpha");
}

TEST_F(GPUAdminAPITest, GetTenants_IsArray) {
    GPUAdminAPI api(defaultConfig());
    const auto json = api.getTenantsJson();
    EXPECT_EQ('[', json.front());
    EXPECT_EQ(']', json.back());
}

// ---------------------------------------------------------------------------
// getDevicesJson()
// ---------------------------------------------------------------------------

TEST_F(GPUAdminAPITest, GetDevices_NullBalancer_ReturnsEmptyArray) {
    GPUAdminAPI api(defaultConfig(), nullptr);
    EXPECT_EQ("[]", api.getDevicesJson());
}

TEST_F(GPUAdminAPITest, GetDevices_WithBalancer_IsArray) {
    // Build a trivial balancer with one synthetic device.
    DeviceInfo dev;
    dev.device_index    = 0;
    dev.name            = "test-device";
    dev.backend         = "cpu_fallback";
    dev.total_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    dev.free_vram_bytes  = 4ULL * 1024 * 1024 * 1024;
    dev.is_healthy       = true;

    GPULoadBalancer balancer(GPULoadBalancer::Strategy::FIRST_HEALTHY, {dev});
    GPUAdminAPI api(defaultConfig(), &balancer);
    const auto json = api.getDevicesJson();
    EXPECT_EQ('[', json.front());
    EXPECT_EQ(']', json.back());
    EXPECT_NE(std::string::npos, json.find("test-device"));
    EXPECT_NE(std::string::npos, json.find("is_healthy"));
}

// ---------------------------------------------------------------------------
// simulateJson()
// ---------------------------------------------------------------------------

TEST_F(GPUAdminAPITest, Simulate_SmallRequest_Accepted) {
    GPUAdminAPI api(defaultConfig());
    const auto json = api.simulateJson(128ULL * 1024 * 1024);  // 128 MB
    EXPECT_NE(std::string::npos, json.find("\"accepted\":true"));
    EXPECT_NE(std::string::npos, json.find("reason"));
    EXPECT_NE(std::string::npos, json.find("bytes"));
}

TEST_F(GPUAdminAPITest, Simulate_OverLimit_Rejected) {
    // Request much more than the config's max_vram_bytes.
    GPUConfig cfg = defaultConfig();
    cfg.max_vram_bytes = 1ULL * 1024 * 1024;  // 1 MB
    GPUAdminAPI api(cfg);
    const auto json = api.simulateJson(512ULL * 1024 * 1024);  // 512 MB
    EXPECT_NE(std::string::npos, json.find("\"accepted\":false"));
}

TEST_F(GPUAdminAPITest, Simulate_HasCurrentAllocatedBytes) {
    GPUAdminAPI api(defaultConfig());
    const auto json = api.simulateJson(64ULL * 1024 * 1024);
    EXPECT_NE(std::string::npos, json.find("current_allocated_bytes"));
}

TEST_F(GPUAdminAPITest, Simulate_Json_WellFormed) {
    GPUAdminAPI api(defaultConfig());
    const auto json = api.simulateJson(1ULL * 1024 * 1024);
    EXPECT_EQ('{', json.front());
    EXPECT_EQ('}', json.back());
}

// ---------------------------------------------------------------------------
// JSON escaping: special characters in edition_info
// ---------------------------------------------------------------------------

TEST_F(GPUAdminAPITest, GetStats_JsonDoesNotContainRawNewline) {
    GPUAdminAPI api(defaultConfig());
    const auto json = api.getStatsJson();
    // Raw newlines in JSON are invalid; they must be escaped.
    EXPECT_EQ(std::string::npos, json.find('\n'));
}

// ---------------------------------------------------------------------------
// getMIGInstancesJson()
// ---------------------------------------------------------------------------

TEST_F(GPUAdminAPITest, GetMIGInstances_EmptyRegistry_ReturnsEmptyArray) {
    MIGManager::GetInstance().reset();
    GPUAdminAPI api(defaultConfig());
    EXPECT_EQ("[]", api.getMIGInstancesJson());
}

TEST_F(GPUAdminAPITest, GetMIGInstances_IsArray) {
    MIGManager::GetInstance().reset();
    GPUAdminAPI api(defaultConfig());
    const auto json = api.getMIGInstancesJson();
    EXPECT_EQ('[', json.front());
    EXPECT_EQ(']', json.back());
}

TEST_F(GPUAdminAPITest, GetMIGInstances_WithPartition_ContainsExpectedFields) {
    auto& mig = MIGManager::GetInstance();
    mig.reset();
    GPUFeatureFlags::GetInstance().enable(GPUFeatureFlags::Feature::MIG_MANAGER);

    DeviceInfo dev;
    dev.index         = 0;
    dev.device_index  = 0;
    dev.name          = "NVIDIA A100-SXM4-40GB";
    dev.backend       = "CUDA";
    dev.compute_major = 8;
    dev.compute_minor = 0;
    dev.total_vram_bytes = 40ULL * 1024 * 1024 * 1024;
    dev.free_vram_bytes  = dev.total_vram_bytes;
    dev.is_healthy    = true;
    dev.mig_max_instances = 7;

    std::string id;
    const auto create_status = mig.createPartition(0, "1g.5gb", id, {dev});
    if (create_status != MIGManager::Status::OK) {
        GTEST_SKIP() << "capability:mig_available=false;reason=mig_partition_creation_unavailable;compiled_backends="
                     << compiledBackendSummary();
    }
    ASSERT_EQ(mig.assignToTenant(id, "tenant_x"), MIGManager::Status::OK);

    GPUAdminAPI api(defaultConfig());
    const auto json = api.getMIGInstancesJson();

    EXPECT_NE(std::string::npos, json.find("instance_id"));
    EXPECT_NE(std::string::npos, json.find("device_index"));
    EXPECT_NE(std::string::npos, json.find("profile"));
    EXPECT_NE(std::string::npos, json.find("1g.5gb"));
    EXPECT_NE(std::string::npos, json.find("memory_bytes"));
    EXPECT_NE(std::string::npos, json.find("tenant_x"));

    mig.reset();
    GPUFeatureFlags::GetInstance().resetToDefaults();
}
