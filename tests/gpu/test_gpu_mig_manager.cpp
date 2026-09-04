/*
 * Unit tests for MIGManager.
 *
 * All tests run on CI without GPU hardware.  The CPU simulation path is
 * exercised end-to-end: feature-gate enforcement, device capability checks,
 * profile validation, partition lifecycle, tenant assignment, statistics
 * tracking, and concurrent safety.
 *
 * Tests that require a MIG-capable device inject a synthetic DeviceInfo via
 * the createPartition(device_index, profile, out_id, devices) overload so
 * that the full partition lifecycle can be verified on any machine without
 * CUDA hardware.
 */

#include <gtest/gtest.h>
#include "themis/gpu/mig_manager.h"
#include "themis/gpu/feature_flags.h"
#include "themis/gpu/device_discovery.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace themis::gpu;
using Status = MIGManager::Status;

// ---------------------------------------------------------------------------
// Helper: build a synthetic CUDA Ampere device for testing
// ---------------------------------------------------------------------------
static DeviceInfo makeFakeA100(int index = 0) {
    DeviceInfo d;
    d.index         = index;
    d.device_index  = index;
    d.name          = "NVIDIA A100-SXM4-40GB";
    d.backend       = "CUDA";
    d.compute_major = 8;
    d.compute_minor = 0;
    d.total_vram_bytes = 40ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = d.total_vram_bytes;
    d.is_healthy    = true;
    d.mig_enabled   = false;
    d.mig_max_instances = 7;
    return d;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------
class MIGManagerTest : public ::testing::Test {
protected:
    MIGManager mgr;

    void SetUp() override {
        mgr.reset();
        GPUFeatureFlags::GetInstance().enable(
            GPUFeatureFlags::Feature::MIG_MANAGER);
    }
    void TearDown() override {
        mgr.reset();
        GPUFeatureFlags::GetInstance().resetToDefaults();
    }

    // Convenience: create a partition on the fake A100 at device index 0.
    Status create(const std::string& profile, std::string& out_id) {
        return mgr.createPartition(0, profile, out_id, {makeFakeA100(0)});
    }
};

// ===========================================================================
// deviceSupportsMIG — unit tests (no hardware required)
// ===========================================================================

TEST(MIGManagerCapabilityTest, CUDAAmpereSupportsMIG) {
    EXPECT_TRUE(MIGManager::deviceSupportsMIG(makeFakeA100(0)));
}

TEST(MIGManagerCapabilityTest, CUDAHopperSupportsMIG) {
    DeviceInfo d = makeFakeA100(0);
    d.compute_major = 9;
    EXPECT_TRUE(MIGManager::deviceSupportsMIG(d));
}

TEST(MIGManagerCapabilityTest, CUDAVoltaDoesNotSupportMIG) {
    DeviceInfo d = makeFakeA100(0);
    d.compute_major = 7;
    EXPECT_FALSE(MIGManager::deviceSupportsMIG(d));
}

TEST(MIGManagerCapabilityTest, ROCmDoesNotSupportMIG) {
    DeviceInfo d = makeFakeA100(0);
    d.backend = "ROCm";
    EXPECT_FALSE(MIGManager::deviceSupportsMIG(d));
}

TEST(MIGManagerCapabilityTest, CPUFallbackDoesNotSupportMIG) {
    DeviceInfo d;
    d.backend       = "CPU_FALLBACK";
    d.compute_major = 0;
    EXPECT_FALSE(MIGManager::deviceSupportsMIG(d));
}

// ===========================================================================
// isKnownProfile
// ===========================================================================

TEST(MIGManagerProfileTest, KnownProfilesAreRecognised) {
    EXPECT_TRUE(MIGManager::isKnownProfile("1g.5gb"));
    EXPECT_TRUE(MIGManager::isKnownProfile("2g.10gb"));
    EXPECT_TRUE(MIGManager::isKnownProfile("3g.20gb"));
    EXPECT_TRUE(MIGManager::isKnownProfile("4g.20gb"));
    EXPECT_TRUE(MIGManager::isKnownProfile("7g.40gb"));
    EXPECT_TRUE(MIGManager::isKnownProfile("1g.10gb"));
    EXPECT_TRUE(MIGManager::isKnownProfile("1g.12gb"));
    EXPECT_TRUE(MIGManager::isKnownProfile("7g.80gb"));
}

TEST(MIGManagerProfileTest, UnknownProfilesRejected) {
    EXPECT_FALSE(MIGManager::isKnownProfile(""));
    EXPECT_FALSE(MIGManager::isKnownProfile("0g.0gb"));
    EXPECT_FALSE(MIGManager::isKnownProfile("1g.5gb "));  // trailing space
    EXPECT_FALSE(MIGManager::isKnownProfile("unknown"));
}

// ===========================================================================
// profileMemoryBytes
// ===========================================================================

TEST(MIGManagerProfileTest, MemoryBytesCorrectForKnownProfiles) {
    EXPECT_EQ(MIGManager::profileMemoryBytes("1g.5gb"),
              5ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(MIGManager::profileMemoryBytes("7g.40gb"),
              40ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(MIGManager::profileMemoryBytes("7g.80gb"),
              80ULL * 1024 * 1024 * 1024);
}

TEST(MIGManagerProfileTest, UnknownProfileReturnsZeroBytes) {
    EXPECT_EQ(MIGManager::profileMemoryBytes("unknown"), 0ULL);
}

// ===========================================================================
// makeInstanceId
// ===========================================================================

TEST(MIGManagerInstanceIdTest, MakeInstanceId_FormatIsConsistent) {
    const std::string id = MIGManager::makeInstanceId(0, 0);
    EXPECT_EQ(id, "dev0_gi0");
    EXPECT_EQ(MIGManager::makeInstanceId(2, 5), "dev2_gi5");
}

// ===========================================================================
// Feature flag gate
// ===========================================================================

TEST_F(MIGManagerTest, FeatureDisabled_CreateRejectsWithFeatureDisabled) {
    GPUFeatureFlags::GetInstance().disable(
        GPUFeatureFlags::Feature::MIG_MANAGER);

    std::string id;
    auto s = create("1g.5gb", id);
    EXPECT_EQ(s, Status::MIG_FEATURE_DISABLED);
    EXPECT_TRUE(id.empty());
}

TEST_F(MIGManagerTest, FeatureDisabled_DestroyRejectsWithFeatureDisabled) {
    // Pre-create while feature is enabled.
    std::string id;
    ASSERT_EQ(create("1g.5gb", id), Status::OK);

    GPUFeatureFlags::GetInstance().disable(
        GPUFeatureFlags::Feature::MIG_MANAGER);

    EXPECT_EQ(mgr.destroyPartition(id), Status::MIG_FEATURE_DISABLED);
}

// ===========================================================================
// Profile validation
// ===========================================================================

TEST_F(MIGManagerTest, InvalidProfile_Rejected) {
    std::string id;
    auto s = mgr.createPartition(0, "bad_profile", id, {makeFakeA100(0)});
    EXPECT_EQ(s, Status::INVALID_PROFILE);
    EXPECT_TRUE(id.empty());
}

// ===========================================================================
// Device validation
// ===========================================================================

TEST_F(MIGManagerTest, DeviceNotInList_ReturnsDeviceNotFound) {
    std::string id;
    auto s = mgr.createPartition(99, "1g.5gb", id, {makeFakeA100(0)});
    EXPECT_EQ(s, Status::DEVICE_NOT_FOUND);
}

TEST_F(MIGManagerTest, NonMIGDevice_ReturnsMIGNotSupported) {
    DeviceInfo gpu = makeFakeA100(0);
    gpu.compute_major = 7;  // Volta — no MIG support.
    std::string id;
    auto s = mgr.createPartition(0, "1g.5gb", id, {gpu});
    EXPECT_EQ(s, Status::MIG_NOT_SUPPORTED);
}

TEST_F(MIGManagerTest, NoGPUHardware_CreateViaNativeEnumReturnsDeviceNotFound) {
    // The native createPartition(3-arg) calls DeviceDiscovery::Enumerate().
    // On CI without CUDA hardware no real device index 99 exists.
    std::string id;
    auto s = mgr.createPartition(99, "1g.5gb", id);
    EXPECT_TRUE(s == Status::DEVICE_NOT_FOUND ||
                s == Status::MIG_NOT_SUPPORTED);
}

// ===========================================================================
// Create partition — success paths
// ===========================================================================

TEST_F(MIGManagerTest, CreatePartition_ReturnsOKAndPopulatesId) {
    std::string id;
    EXPECT_EQ(create("1g.5gb", id), Status::OK);
    EXPECT_FALSE(id.empty());

    const auto stats = mgr.getStats();
    EXPECT_EQ(stats.total_created, 1u);
    EXPECT_EQ(stats.active_instances, 1u);
}

TEST_F(MIGManagerTest, CreateMultiplePartitions_UniqueIds) {
    std::string id0, id1, id2;
    EXPECT_EQ(create("1g.5gb",  id0), Status::OK);
    EXPECT_EQ(create("2g.10gb", id1), Status::OK);
    EXPECT_EQ(create("3g.20gb", id2), Status::OK);
    EXPECT_NE(id0, id1);
    EXPECT_NE(id1, id2);
    EXPECT_EQ(mgr.getStats().active_instances, 3u);
}

TEST_F(MIGManagerTest, CreatePartition_MemoryBytesMatchProfile) {
    std::string id;
    ASSERT_EQ(create("7g.40gb", id), Status::OK);

    MIGManager::MIGInstance inst;
    ASSERT_TRUE(mgr.getInstance(id, inst));
    EXPECT_EQ(inst.memory_bytes, 40ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(inst.profile, "7g.40gb");
    EXPECT_EQ(inst.device_index, 0);
    EXPECT_TRUE(inst.tenant_id.empty());
}

TEST_F(MIGManagerTest, CreatePartition_IdMatchesMakeInstanceId) {
    std::string id;
    ASSERT_EQ(create("1g.5gb", id), Status::OK);
    // First partition on device 0 must have gi_id=0.
    EXPECT_EQ(id, MIGManager::makeInstanceId(0, 0));
}

// ===========================================================================
// Create partition — rejection paths
// ===========================================================================

TEST_F(MIGManagerTest, CreatePartition_ExceedsMaxInstances) {
    for (int i = 0; i < 7; ++i) {
        std::string id;
        ASSERT_EQ(create("1g.5gb", id), Status::OK)
            << "Failed at iteration " << i;
    }
    std::string id;
    EXPECT_EQ(create("1g.5gb", id), Status::PARTITION_LIMIT_EXCEEDED);
}

// ===========================================================================
// Destroy partition
// ===========================================================================

TEST_F(MIGManagerTest, DestroyPartition_ExistingInstance_OK) {
    std::string id;
    ASSERT_EQ(create("1g.5gb", id), Status::OK);

    EXPECT_EQ(mgr.destroyPartition(id), Status::OK);
    EXPECT_EQ(mgr.getStats().active_instances, 0u);
    EXPECT_EQ(mgr.getStats().total_destroyed, 1u);
}

TEST_F(MIGManagerTest, DestroyPartition_NonExistent_NotFound) {
    EXPECT_EQ(mgr.destroyPartition("ghost"), Status::INSTANCE_NOT_FOUND);
}

TEST_F(MIGManagerTest, DestroyPartition_FreesSlotForNewPartition) {
    std::string ids[7];
    for (int i = 0; i < 7; ++i) {
        ASSERT_EQ(create("1g.5gb", ids[i]), Status::OK);
    }
    ASSERT_EQ(mgr.destroyPartition(ids[3]), Status::OK);
    std::string new_id;
    EXPECT_EQ(create("1g.5gb", new_id), Status::OK);
    EXPECT_EQ(mgr.getStats().active_instances, 7u);
}

// ===========================================================================
// Tenant assignment — success paths
// ===========================================================================

TEST_F(MIGManagerTest, AssignToTenant_OK) {
    std::string id;
    ASSERT_EQ(create("2g.10gb", id), Status::OK);

    EXPECT_EQ(mgr.assignToTenant(id, "tenant_A"), Status::OK);

    MIGManager::MIGInstance inst;
    ASSERT_TRUE(mgr.getInstance(id, inst));
    EXPECT_EQ(inst.tenant_id, "tenant_A");
    EXPECT_EQ(mgr.getStats().total_assigned, 1u);
}

TEST_F(MIGManagerTest, UnassignFromTenant_OK) {
    std::string id;
    ASSERT_EQ(create("2g.10gb", id), Status::OK);
    ASSERT_EQ(mgr.assignToTenant(id, "tenant_A"), Status::OK);

    EXPECT_EQ(mgr.unassignFromTenant(id), Status::OK);

    MIGManager::MIGInstance inst;
    ASSERT_TRUE(mgr.getInstance(id, inst));
    EXPECT_TRUE(inst.tenant_id.empty());
    EXPECT_EQ(mgr.getStats().total_unassigned, 1u);
}

TEST_F(MIGManagerTest, AssignAndUnassign_AllowsReassign) {
    std::string id;
    ASSERT_EQ(create("1g.5gb", id), Status::OK);
    ASSERT_EQ(mgr.assignToTenant(id, "tenant_A"), Status::OK);
    ASSERT_EQ(mgr.unassignFromTenant(id), Status::OK);
    EXPECT_EQ(mgr.assignToTenant(id, "tenant_B"), Status::OK);

    MIGManager::MIGInstance inst;
    ASSERT_TRUE(mgr.getInstance(id, inst));
    EXPECT_EQ(inst.tenant_id, "tenant_B");
}

// ===========================================================================
// Tenant assignment — rejection paths
// ===========================================================================

TEST_F(MIGManagerTest, AssignAlreadyAssigned_Rejected) {
    std::string id;
    ASSERT_EQ(create("1g.5gb", id), Status::OK);
    ASSERT_EQ(mgr.assignToTenant(id, "tenant_A"), Status::OK);
    EXPECT_EQ(mgr.assignToTenant(id, "tenant_B"), Status::ALREADY_ASSIGNED);
}

TEST_F(MIGManagerTest, UnassignNotAssigned_Rejected) {
    std::string id;
    ASSERT_EQ(create("1g.5gb", id), Status::OK);
    EXPECT_EQ(mgr.unassignFromTenant(id), Status::NOT_ASSIGNED);
}

TEST_F(MIGManagerTest, AssignNonExistentInstance_ReturnsNotFound) {
    EXPECT_EQ(mgr.assignToTenant("nonexistent", "t1"), Status::INSTANCE_NOT_FOUND);
}

TEST_F(MIGManagerTest, UnassignNonExistentInstance_ReturnsNotFound) {
    EXPECT_EQ(mgr.unassignFromTenant("nonexistent"), Status::INSTANCE_NOT_FOUND);
}

TEST_F(MIGManagerTest, DestroyNonExistentInstance_ReturnsNotFound) {
    EXPECT_EQ(mgr.destroyPartition("nonexistent"), Status::INSTANCE_NOT_FOUND);
}

// ===========================================================================
// Query methods
// ===========================================================================

TEST_F(MIGManagerTest, GetInstances_ReturnsAllActive) {
    std::string id0, id1;
    ASSERT_EQ(create("1g.5gb", id0), Status::OK);
    ASSERT_EQ(create("2g.10gb", id1), Status::OK);

    EXPECT_EQ(mgr.getInstances().size(), 2u);
}

TEST_F(MIGManagerTest, GetInstancesForDevice_FiltersCorrectly) {
    // Device 0 gets two instances; device 1 gets one.
    const DeviceInfo dev0 = makeFakeA100(0);
    const DeviceInfo dev1 = makeFakeA100(1);

    std::string id0, id1, id2;
    ASSERT_EQ(mgr.createPartition(0, "1g.5gb",  id0, {dev0, dev1}), Status::OK);
    ASSERT_EQ(mgr.createPartition(0, "2g.10gb", id1, {dev0, dev1}), Status::OK);
    ASSERT_EQ(mgr.createPartition(1, "1g.5gb",  id2, {dev0, dev1}), Status::OK);

    EXPECT_EQ(mgr.getInstancesForDevice(0).size(), 2u);
    EXPECT_EQ(mgr.getInstancesForDevice(1).size(), 1u);
    EXPECT_TRUE(mgr.getInstancesForDevice(9).empty());
}

TEST_F(MIGManagerTest, GetInstancesForTenant_FiltersCorrectly) {
    std::string id0, id1, id2;
    ASSERT_EQ(create("1g.5gb",  id0), Status::OK);
    ASSERT_EQ(create("1g.5gb",  id1), Status::OK);
    ASSERT_EQ(create("2g.10gb", id2), Status::OK);
    ASSERT_EQ(mgr.assignToTenant(id0, "tA"), Status::OK);
    ASSERT_EQ(mgr.assignToTenant(id1, "tA"), Status::OK);
    ASSERT_EQ(mgr.assignToTenant(id2, "tB"), Status::OK);

    EXPECT_EQ(mgr.getInstancesForTenant("tA").size(), 2u);
    EXPECT_EQ(mgr.getInstancesForTenant("tB").size(), 1u);
    EXPECT_TRUE(mgr.getInstancesForTenant("tC").empty());
}

TEST_F(MIGManagerTest, GetInstance_ReturnsCorrectDescriptor) {
    std::string id;
    ASSERT_EQ(create("3g.20gb", id), Status::OK);

    MIGManager::MIGInstance inst;
    EXPECT_TRUE(mgr.getInstance(id, inst));
    EXPECT_EQ(inst.instance_id, id);
    EXPECT_EQ(inst.profile, "3g.20gb");
    EXPECT_EQ(inst.device_index, 0);
}

TEST_F(MIGManagerTest, GetInstance_NotFound_ReturnsFalse) {
    MIGManager::MIGInstance inst;
    EXPECT_FALSE(mgr.getInstance("ghost", inst));
}

// ===========================================================================
// Statistics
// ===========================================================================

TEST_F(MIGManagerTest, Stats_CountAllOperations) {
    std::string id0, id1;
    create("1g.5gb", id0);                      // +1 create
    create("2g.10gb", id1);                      // +1 create
    mgr.assignToTenant(id0, "tA");               // +1 assign
    mgr.unassignFromTenant(id0);                 // +1 unassign
    mgr.destroyPartition(id1);                   // +1 destroy

    const auto s = mgr.getStats();
    EXPECT_EQ(s.total_created,    2u);
    EXPECT_EQ(s.total_destroyed,  1u);
    EXPECT_EQ(s.total_assigned,   1u);
    EXPECT_EQ(s.total_unassigned, 1u);
    EXPECT_EQ(s.active_instances, 1u);
}

TEST_F(MIGManagerTest, Reset_ClearsAllState) {
    std::string id;
    create("1g.5gb", id);
    mgr.reset();

    const auto s = mgr.getStats();
    EXPECT_EQ(s.total_created,    0u);
    EXPECT_EQ(s.active_instances, 0u);
    EXPECT_TRUE(mgr.getInstances().empty());
}

// ===========================================================================
// migStatusName helper
// ===========================================================================

TEST(MIGManagerStatusNameTest, AllStatusesHaveNames) {
    using S = MIGManager::Status;
    const S all[] = {
        S::OK,
        S::MIG_NOT_SUPPORTED,
        S::MIG_FEATURE_DISABLED,
        S::INVALID_PROFILE,
        S::DEVICE_NOT_FOUND,
        S::PARTITION_LIMIT_EXCEEDED,
        S::INSTANCE_NOT_FOUND,
        S::ALREADY_ASSIGNED,
        S::NOT_ASSIGNED,
    };
    for (auto s : all) {
        const char* name = migStatusName(s);
        EXPECT_NE(name, nullptr);
        EXPECT_STRNE(name, "UNKNOWN");
    }
}

// ===========================================================================
// Feature flag: MIG_MANAGER in getAll()
// ===========================================================================

TEST(MIGManagerFeatureFlagTest, MIGManagerAppearsInGetAll) {
    const auto flags = GPUFeatureFlags::GetInstance().getAll();
    bool found = false;
    for (const auto& fs : flags) {
        if (fs.name == std::string("MIG_MANAGER")) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "MIG_MANAGER must appear in GPUFeatureFlags::getAll()";
}

// ===========================================================================
// DeviceInfo MIG field validation
// ===========================================================================

TEST(MIGManagerDeviceInfoTest, AmpereFakeDevice_HasMIGMaxInstances) {
    // A synthetic Ampere device should indicate 7 MIG instances max.
    DeviceInfo a100 = makeFakeA100(0);
    EXPECT_EQ(a100.mig_max_instances, 7);
    EXPECT_EQ(a100.compute_major, 8);
    EXPECT_EQ(a100.backend, std::string("CUDA"));
}

TEST(MIGManagerDeviceInfoTest, HopperFakeDevice_SupportsMIG) {
    DeviceInfo h100;
    h100.index          = 0;
    h100.device_index   = 0;
    h100.name           = "NVIDIA H100 SXM5 80GB";
    h100.backend        = "CUDA";
    h100.compute_major  = 9;
    h100.compute_minor  = 0;
    h100.total_vram_bytes = 80ULL * 1024 * 1024 * 1024;
    h100.free_vram_bytes  = h100.total_vram_bytes;
    h100.is_healthy     = true;
    h100.mig_max_instances = 7;
    EXPECT_TRUE(MIGManager::deviceSupportsMIG(h100));
    EXPECT_EQ(h100.mig_max_instances, 7);
}

TEST(MIGManagerDeviceInfoTest, VoltaDevice_NoMIGMaxInstances) {
    // Volta (major=7) does not support MIG — mig_max_instances should be 0.
    DeviceInfo volta;
    volta.index         = 0;
    volta.device_index  = 0;
    volta.name          = "NVIDIA V100";
    volta.backend       = "CUDA";
    volta.compute_major = 7;
    volta.compute_minor = 0;
    volta.is_healthy    = true;
    volta.mig_max_instances = 0;  // Not Ampere/Hopper — no MIG support.
    EXPECT_FALSE(MIGManager::deviceSupportsMIG(volta));
    EXPECT_EQ(volta.mig_max_instances, 0);
}

TEST(MIGManagerDeviceInfoTest, CPUFallback_NoMIGMaxInstances) {
    // CPU_FALLBACK sentinel should report no MIG support.
    DeviceInfo cpu;
    cpu.index         = -1;
    cpu.device_index  = -1;
    cpu.name          = "CPU Fallback";
    cpu.backend       = "CPU_FALLBACK";
    cpu.compute_major = 0;
    cpu.mig_max_instances = 0;
    EXPECT_FALSE(MIGManager::deviceSupportsMIG(cpu));
    EXPECT_EQ(cpu.mig_max_instances, 0);
}

// ===========================================================================
// Thread safety
// ===========================================================================

TEST_F(MIGManagerTest, ConcurrentCreate_NoDataRace) {
    constexpr int THREADS = 4;
    // Each thread creates one instance; 4 <= 7 so all should succeed.
    std::atomic<int> ok_count{0};
    std::atomic<int> fail_count{0};
    const std::vector<DeviceInfo> devices = {makeFakeA100(0)};

    auto worker = [&]() {
        std::string id;
        auto s = mgr.createPartition(0, "1g.5gb", id, devices);
        if (s == Status::OK) {
            ok_count.fetch_add(1, std::memory_order_relaxed);
        } else {
            fail_count.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(ok_count.load(), THREADS);
    EXPECT_EQ(mgr.getStats().active_instances,
              static_cast<size_t>(THREADS));
}
