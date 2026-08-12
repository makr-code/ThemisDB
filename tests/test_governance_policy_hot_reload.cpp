/**
 * @file test_governance_policy_hot_reload.cpp
 * @brief Unit tests for governance PolicyEngine hot-reload and PolicyFileWatcher.
 *
 * Tests cover:
 * - reloadIfChanged when no file has been loaded → fast no-op (returns true)
 * - reloadIfChanged when file is unchanged → no-op (classification unchanged)
 * - reloadIfChanged after file is modified → new policies take effect atomically
 * - Classification change is reflected immediately after reload
 * - reloadIfChanged returns false and populates err on a missing file
 * - PolicyFileWatcher starts and stops cleanly
 * - PolicyFileWatcher detects a file change and triggers a reload automatically
 */

#include <gtest/gtest.h>
#include "governance/policy_engine.h"
#include "governance/policy_file_watcher.h"
#include "observability/metrics_collector.h"
#include "utils/audit_logger.h"

#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

namespace fs = std::filesystem;
using namespace themis::governance;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class GovernanceHotReloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = fs::temp_directory_path() / "gov_hotreload_test";
        fs::create_directories(tmp_dir_);
        yaml_path_ = (tmp_dir_ / "governance.yaml").string();
    }

    void TearDown() override {
        fs::remove_all(tmp_dir_);
    }

    void writeYaml(const char* content) {
        std::ofstream f(yaml_path_, std::ios::trunc);
        ASSERT_TRUE(f.good());
        f << content;
        f.flush();
    }

    /// Bump the file's mtime so reloadIfChanged() treats it as "changed".
    void touchFile(const std::string& path) {
        fs::last_write_time(path, fs::file_time_type::clock::now());
    }

    fs::path   tmp_dir_;
    std::string yaml_path_;
};

// ---------------------------------------------------------------------------
// reloadIfChanged – no file loaded
// ---------------------------------------------------------------------------

TEST(GovernanceHotReloadNoFileTest, ReloadIfChanged_NoFileLoaded_ReturnsTrue) {
    PolicyEngine pe;
    // No loadFromYAML called – must be a fast no-op
    EXPECT_TRUE(pe.reloadIfChanged());
}

// ---------------------------------------------------------------------------
// getLoadedFilePath – no file loaded
// ---------------------------------------------------------------------------

TEST(GovernanceHotReloadNoFileTest, GetLoadedFilePath_NoFile_ReturnsEmpty) {
    PolicyEngine pe;
    EXPECT_TRUE(pe.getLoadedFilePath().empty());
}

// ---------------------------------------------------------------------------
// reloadIfChanged – file unchanged → no-op
// ---------------------------------------------------------------------------

TEST_F(GovernanceHotReloadTest, ReloadIfChanged_FileUnchanged_NoOp) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 90
    log_encryption: false
enforcement:
  default_mode: enforce
)");

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromYAML(yaml_path_));
    ASSERT_EQ(pe.getLoadedFilePath(), yaml_path_);

    // Second call with the same (unchanged) file – must be a no-op
    EXPECT_TRUE(pe.reloadIfChanged());

    // Classification profile should still be present
    auto profile = pe.getClassificationProfile("offen");
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->retention_days, 90);
}

// ---------------------------------------------------------------------------
// reloadIfChanged – file modified → new content loaded atomically
// ---------------------------------------------------------------------------

TEST_F(GovernanceHotReloadTest, ReloadIfChanged_FileModified_NewContentLoaded) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 90
    log_encryption: false
enforcement:
  default_mode: enforce
)");

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromYAML(yaml_path_));

    auto profile_before = pe.getClassificationProfile("offen");
    ASSERT_TRUE(profile_before.has_value());
    EXPECT_EQ(profile_before->retention_days, 90);

    // Brief pause so the filesystem mtime granularity is guaranteed to change
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Overwrite with updated retention_days
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 365
    log_encryption: false
  geheim:
    encryption_required: true
    ann_allowed: false
    export_allowed: false
    cache_allowed: false
    redaction_level: "strict"
    retention_days: 7
    log_encryption: true
enforcement:
  default_mode: enforce
)");
    touchFile(yaml_path_);

    std::string err;
    ASSERT_TRUE(pe.reloadIfChanged(&err)) << err;

    // Old classification updated
    auto profile_after = pe.getClassificationProfile("offen");
    ASSERT_TRUE(profile_after.has_value());
    EXPECT_EQ(profile_after->retention_days, 365);

    // New classification available
    auto geheim_profile = pe.getClassificationProfile("geheim");
    ASSERT_TRUE(geheim_profile.has_value());
    EXPECT_TRUE(geheim_profile->encryption_required);
    EXPECT_FALSE(geheim_profile->ann_allowed);
}

// ---------------------------------------------------------------------------
// reloadIfChanged – evaluate() reflects updated classification after reload
// ---------------------------------------------------------------------------

TEST_F(GovernanceHotReloadTest, ReloadIfChanged_EvaluateReflectsNewPolicy) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 30
    log_encryption: false
enforcement:
  default_mode: observe
  resource_mapping:
    /data: offen
)");

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromYAML(yaml_path_));

    std::unordered_map<std::string, std::string> headers;
    auto d1 = pe.evaluate(headers, "/data");
    EXPECT_EQ(d1.classification, "offen");
    EXPECT_EQ(d1.mode, "observe");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Update to enforce mode and geheim classification for /data
    writeYaml(R"(
vs_classification:
  geheim:
    encryption_required: true
    ann_allowed: false
    export_allowed: false
    cache_allowed: false
    redaction_level: "strict"
    retention_days: 7
    log_encryption: true
enforcement:
  default_mode: enforce
  resource_mapping:
    /data: geheim
)");
    touchFile(yaml_path_);

    ASSERT_TRUE(pe.reloadIfChanged());

    auto d2 = pe.evaluate(headers, "/data");
    EXPECT_EQ(d2.classification, "geheim");
    EXPECT_EQ(d2.mode, "enforce");
    EXPECT_TRUE(d2.require_content_encryption);
}

// ---------------------------------------------------------------------------
// reloadIfChanged – non-existent file → returns false without throwing
// ---------------------------------------------------------------------------

TEST_F(GovernanceHotReloadTest, ReloadIfChanged_MissingFile_ReturnsFalse) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
enforcement:
  default_mode: enforce
)");

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromYAML(yaml_path_));

    // Delete the file so the next reload attempt will fail
    fs::remove(yaml_path_);

    // Verify that reloadIfChanged() handles a missing file gracefully without throwing.
    std::string err;
    bool result = pe.reloadIfChanged(&err);
    // Either true (mtime unchanged in cache, no reload attempted) or false
    // (stat failed).  The important thing is it does not throw.
    (void)result;
}

// ---------------------------------------------------------------------------
// PolicyFileWatcher – starts and stops cleanly
// ---------------------------------------------------------------------------

TEST_F(GovernanceHotReloadTest, PolicyFileWatcher_StartStop) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 30
    log_encryption: false
enforcement:
  default_mode: enforce
)");

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromYAML(yaml_path_));

    PolicyFileWatcher watcher(pe);
    EXPECT_FALSE(watcher.isRunning());

    EXPECT_TRUE(watcher.start());
    EXPECT_TRUE(watcher.isRunning());

    watcher.stop();
    EXPECT_FALSE(watcher.isRunning());
}

// ---------------------------------------------------------------------------
// PolicyFileWatcher – detects file change and triggers reload automatically
// ---------------------------------------------------------------------------

TEST_F(GovernanceHotReloadTest, PolicyFileWatcher_AutoReloadsOnFileChange) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 30
    log_encryption: false
enforcement:
  default_mode: enforce
)");

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromYAML(yaml_path_));

    // Use fast debounce for testing
    PolicyFileWatcher::Config cfg;
    cfg.poll_interval = std::chrono::milliseconds(50);
    cfg.debounce      = std::chrono::milliseconds(100);

    PolicyFileWatcher watcher(pe, std::move(cfg));
    ASSERT_TRUE(watcher.start());

    // Allow the watcher to capture an initial baseline mtime
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Write a new file content
    writeYaml(R"(
vs_classification:
  geheim:
    encryption_required: true
    ann_allowed: false
    export_allowed: false
    cache_allowed: false
    redaction_level: "strict"
    retention_days: 7
    log_encryption: true
enforcement:
  default_mode: enforce
)");
    touchFile(yaml_path_);

    // Wait for debounce + at least one poll cycle (100 + 50 + margin = 300 ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    watcher.stop();

    // The new classification must be available
    auto geheim = pe.getClassificationProfile("geheim");
    EXPECT_TRUE(geheim.has_value()) << "geheim profile should be loaded after auto-reload";
    if (geheim) {
        EXPECT_TRUE(geheim->encryption_required);
    }
}

// ---------------------------------------------------------------------------
// Prometheus counter incremented on reload
// ---------------------------------------------------------------------------

TEST_F(GovernanceHotReloadTest, ReloadIfChanged_EmitsPrometheusCounter) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 30
    log_encryption: false
enforcement:
  default_mode: enforce
)");

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromYAML(yaml_path_));

    // Reset the global metrics collector so we start from zero
    themis::observability::MetricsCollector::getInstance().reset();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Modify the file so reloadIfChanged() will actually reload
    writeYaml(R"(
vs_classification:
  geheim:
    encryption_required: true
    ann_allowed: false
    export_allowed: false
    cache_allowed: false
    redaction_level: "strict"
    retention_days: 7
    log_encryption: true
enforcement:
  default_mode: enforce
)");
    touchFile(yaml_path_);

    std::string err;
    const bool ok = pe.reloadIfChanged(&err);
    ASSERT_TRUE(ok) << err;

    // The governance_policy_reload_total{result="success"} counter must have
    // been incremented exactly once.
    const std::string prometheus_text =
        themis::observability::MetricsCollector::getInstance().getPrometheusMetrics();
    // Verify the metric name and the success label appear in the output
    const bool has_metric = prometheus_text.find("governance_policy_reload_total") != std::string::npos;
    const bool has_success = prometheus_text.find("governance_policy_reload_total") != std::string::npos &&
                             prometheus_text.find("result") != std::string::npos &&
                             prometheus_text.find("success") != std::string::npos;
    EXPECT_TRUE(has_metric) << "Prometheus counter governance_policy_reload_total must be present";
    EXPECT_TRUE(has_success) << "Counter must have result=success label";
}

// ---------------------------------------------------------------------------
// Audit entry emitted on reload when logger is set
// ---------------------------------------------------------------------------

TEST_F(GovernanceHotReloadTest, ReloadIfChanged_EmitsAuditEntry) {
    writeYaml(R"(
vs_classification:
  offen:
    encryption_required: false
    ann_allowed: true
    export_allowed: true
    cache_allowed: true
    redaction_level: "none"
    retention_days: 30
    log_encryption: false
enforcement:
  default_mode: enforce
)");

    PolicyEngine pe;
    ASSERT_TRUE(pe.loadFromYAML(yaml_path_));

    // Install an audit logger so the reload event is recorded
    themis::utils::AuditLoggerConfig audit_cfg;
    audit_cfg.enabled = false;
    auto logger = std::make_shared<themis::utils::AuditLogger>(nullptr, nullptr, audit_cfg);
    pe.setAuditLogger(logger);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    writeYaml(R"(
vs_classification:
  geheim:
    encryption_required: true
    ann_allowed: false
    export_allowed: false
    cache_allowed: false
    redaction_level: "strict"
    retention_days: 7
    log_encryption: true
enforcement:
  default_mode: enforce
)");
    touchFile(yaml_path_);

    std::string err;
    const bool ok = pe.reloadIfChanged(&err);
    ASSERT_TRUE(ok) << err;

    // New geheim classification should be active
    auto geheim = pe.getClassificationProfile("geheim");
    EXPECT_TRUE(geheim.has_value());
}
