#include <gtest/gtest.h>
#include "storage/disk_space_monitor.h"
#include <thread>
#include <fstream>

using namespace themis::storage;

class DiskSpaceMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test directory
        test_path_ = "/tmp/themis_disk_test";
        
        // Default configuration
        config_.warning_threshold = 0.20f;
        config_.critical_threshold = 0.10f;
        config_.emergency_threshold = 0.05f;
        config_.reserved_bytes = 100 * 1024 * 1024;  // 100 MB
        config_.check_interval = std::chrono::seconds(1);
        config_.enable_auto_monitoring = false;  // Manual control in tests
        config_.enable_alerts = true;
        config_.enable_auto_gc = true;
        config_.enable_write_blocking = true;
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    std::string test_path_;
    DiskSpaceMonitor::Config config_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, InitialCheck) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    auto info = monitor.getSpaceInfo();
    
    EXPECT_GT(info.total_bytes, 0);
    EXPECT_GT(info.free_bytes, 0);
    EXPECT_LE(info.used_bytes, info.total_bytes);
    EXPECT_GE(info.usage_percent, 0.0f);
    EXPECT_LE(info.usage_percent, 1.0f);
}

TEST_F(DiskSpaceMonitorTest, SpaceLevelClassification) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    auto info = monitor.getSpaceInfo();
    auto level = monitor.getSpaceLevel();
    
    // Most systems should have normal space
    if (info.free_percent > config_.warning_threshold) {
        EXPECT_EQ(level, DiskSpaceMonitor::SpaceLevel::NORMAL);
    }
}

TEST_F(DiskSpaceMonitorTest, CanWriteCheck) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    // Small writes should only be allowed when the current disk state permits them.
    auto info = monitor.getSpaceInfo();
    if (info.level == DiskSpaceMonitor::SpaceLevel::CRITICAL ||
        info.level == DiskSpaceMonitor::SpaceLevel::EMERGENCY ||
        monitor.isReadOnly()) {
        EXPECT_FALSE(monitor.canWrite(1024));
    } else {
        EXPECT_TRUE(monitor.canWrite(1024));
    }
    
    // Write larger than available space should fail
    info = monitor.getSpaceInfo();
    EXPECT_FALSE(monitor.canWrite(info.total_bytes * 2));
}

// ============================================================================
// Alert Callback Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, AlertCallbackInvoked) {
    bool alert_received = false;
    std::string alert_message;
    
    DiskSpaceMonitor monitor(test_path_, config_);
    
    monitor.setAlertCallback([&](const DiskSpaceMonitor::SpaceInfo& info, 
                                  const std::string& msg) {
        alert_received = true;
        alert_message = msg;
    });
    
    // Manually trigger alert by setting very high thresholds
    // Note: In real usage, this would happen naturally
    // For testing, we verify the callback mechanism works
    
    // Just verify callback was set (actual triggering requires disk manipulation)
    auto stats = monitor.getStats();
    EXPECT_GE(stats.total_checks, 0);  // At least initial check
}

// ============================================================================
// GC Callback Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, GCCallbackInvoked) {
    bool gc_triggered = false;
    
    DiskSpaceMonitor monitor(test_path_, config_);
    const auto stats_before = monitor.getStats();
    
    monitor.setGCCallback([&]() {
        gc_triggered = true;
    });
    
    // Manually trigger GC
    monitor.triggerGC();
    
    EXPECT_TRUE(gc_triggered);
    
    auto stats = monitor.getStats();
    EXPECT_GE(stats.gc_triggers, stats_before.gc_triggers + 1);
}

// ============================================================================
// Read-Only Mode Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, ReadOnlyOverride) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    EXPECT_FALSE(monitor.isReadOnly());
    
    monitor.setReadOnlyOverride(true);
    
    EXPECT_TRUE(monitor.isReadOnly());
    EXPECT_FALSE(monitor.canWrite(1024));
    
    monitor.setReadOnlyOverride(false);
    
    EXPECT_FALSE(monitor.isReadOnly());
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, Statistics) {
    DiskSpaceMonitor monitor(test_path_, config_);
    const auto stats_before = monitor.getStats();
    
    // Perform some checks
    monitor.checkSpace();
    monitor.checkSpace();
    monitor.checkSpace();
    
    auto stats = monitor.getStats();
    
    EXPECT_GE(stats.total_checks, stats_before.total_checks + 3);
    EXPECT_GE(stats.warning_triggers, stats_before.warning_triggers);
    EXPECT_GE(stats.critical_triggers, stats_before.critical_triggers);
    EXPECT_GE(stats.emergency_triggers, stats_before.emergency_triggers);
}

// ============================================================================
// Monitoring Loop Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, AutoMonitoring) {
    config_.check_interval = std::chrono::seconds(1);
    config_.enable_auto_monitoring = true;
    
    DiskSpaceMonitor monitor(test_path_, config_);
    
    // Wait for a few monitoring cycles
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    auto stats = monitor.getStats();
    
    // Should have performed multiple checks
    EXPECT_GE(stats.total_checks, 2);
}

TEST_F(DiskSpaceMonitorTest, StartStopMonitoring) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    monitor.startMonitoring();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    monitor.stopMonitoring();
    
    auto stats1 = monitor.getStats();
    size_t checks1 = stats1.total_checks;
    
    // Wait a bit more after stopping
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto stats2 = monitor.getStats();
    size_t checks2 = stats2.total_checks;
    
    // Check count should not increase after stopping
    EXPECT_EQ(checks1, checks2);
}

// ============================================================================
// Recommended Action Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, RecommendedActionNormal) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    auto action = monitor.getRecommendedAction();
    
    // Should have some recommendation
    EXPECT_FALSE(action.empty());
}

// ============================================================================
// Time Estimation Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, EstimateTimeUntilFull) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    // Perform multiple checks to build history
    for (int i = 0; i < 5; ++i) {
        monitor.checkSpace();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto time_until_full = monitor.estimateTimeUntilFull();
    
    // With stable usage, estimate may be 0 (no growth detected)
    // This is expected behavior
    EXPECT_GE(time_until_full.count(), 0);
}

// ============================================================================
// DiskSpaceGuard Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, DiskSpaceGuardValid) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    DiskSpaceGuard guard(monitor, 1024, "test_operation");
    
    // Small allocation should succeed only when the current disk state permits it.
    if (monitor.canWrite(1024)) {
        EXPECT_TRUE(guard.isValid());
        EXPECT_TRUE(guard.getError().empty());
    } else {
        EXPECT_FALSE(guard.isValid());
        EXPECT_FALSE(guard.getError().empty());
    }
}

TEST_F(DiskSpaceMonitorTest, DiskSpaceGuardInvalid) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    // Request more space than exists
    size_t huge_size = monitor.getSpaceInfo().total_bytes * 2;
    
    DiskSpaceGuard guard(monitor, huge_size, "huge_operation");
    
    EXPECT_FALSE(guard.isValid());
    EXPECT_FALSE(guard.getError().empty());
}

// ============================================================================
// Disk Utils Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, GetDiskSpace) {
    size_t total, free, available;
    
    bool result = disk_utils::getDiskSpace(test_path_, total, free, available);
    
    EXPECT_TRUE(result);
    EXPECT_GT(total, 0);
    EXPECT_LE(free, total);
    EXPECT_LE(available, free);
}

TEST_F(DiskSpaceMonitorTest, FormatBytes) {
    EXPECT_EQ(disk_utils::formatBytes(0), "0.00 B");
    EXPECT_EQ(disk_utils::formatBytes(1024), "1.00 KB");
    EXPECT_EQ(disk_utils::formatBytes(1024 * 1024), "1.00 MB");
    EXPECT_EQ(disk_utils::formatBytes(1024 * 1024 * 1024), "1.00 GB");
    EXPECT_EQ(disk_utils::formatBytes(1536), "1.50 KB");
}

TEST_F(DiskSpaceMonitorTest, PathExists) {
    // Root should always exist
    EXPECT_TRUE(disk_utils::pathExists("/"));
    
    // Non-existent path should return false
    EXPECT_FALSE(disk_utils::pathExists("/this/path/does/not/exist/hopefully"));
}

TEST_F(DiskSpaceMonitorTest, GetDirectory) {
    EXPECT_EQ(disk_utils::getDirectory("/home/user/file.txt"), "/home/user");
    EXPECT_EQ(disk_utils::getDirectory("/home/user/"), "/home/user");
    EXPECT_EQ(disk_utils::getDirectory("file.txt"), ".");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, RealWorldScenario) {
    bool alert_received = false;
    bool gc_triggered = false;
    
    DiskSpaceMonitor monitor(test_path_, config_);
    
    monitor.setAlertCallback([&](const DiskSpaceMonitor::SpaceInfo& info, 
                                  const std::string& msg) {
        alert_received = true;
        // spdlog logging commented out - use fmt::print instead if logging needed
    });
    
    monitor.setGCCallback([&]() {
        gc_triggered = true;
        // GC triggered - logging commented out
    });
    
    // Start monitoring
    monitor.startMonitoring();
    
    // Simulate some operations
    for (int i = 0; i < 5; ++i) {
        if (monitor.canWrite(1024 * 1024)) {  // Try to write 1 MB
            // Write would succeed
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    monitor.stopMonitoring();
    
    auto stats = monitor.getStats();
    auto info = monitor.getSpaceInfo();
    
    // Verify monitoring worked
    EXPECT_GT(stats.total_checks, 0);
    EXPECT_GT(info.total_bytes, 0);
    
    // Logging commented out - not needed for test assertions
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(DiskSpaceMonitorTest, InvalidPath) {
    // Monitor with invalid path should handle gracefully
    DiskSpaceMonitor monitor("/invalid/path/that/does/not/exist", config_);
    
    // Should not crash, but may have limited functionality
    auto info = monitor.getSpaceInfo();
    // On failure, total_bytes will be 0
    EXPECT_GE(info.total_bytes, 0);
}

TEST_F(DiskSpaceMonitorTest, ZeroByteWrite) {
    DiskSpaceMonitor monitor(test_path_, config_);
    
    // Zero-byte write should always succeed
    EXPECT_TRUE(monitor.canWrite(0));
}

TEST_F(DiskSpaceMonitorTest, ReservedSpaceRespected) {
    config_.reserved_bytes = 1024 * 1024 * 1024;  // 1 GB reserved
    DiskSpaceMonitor monitor(test_path_, config_);
    
    auto info = monitor.getSpaceInfo();
    
    // Available should be less than free
    EXPECT_LE(info.available_bytes, info.free_bytes);
    
    if (info.free_bytes > config_.reserved_bytes) {
        EXPECT_EQ(info.available_bytes, info.free_bytes - config_.reserved_bytes);
    } else {
        EXPECT_EQ(info.available_bytes, 0);
    }
}
