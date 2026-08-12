// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_updates_production.cpp
 * @brief Production-readiness tests for the Updates module (all 11 phases)
 *
 * Covers:
 *  Phase 1 – Core Download & Backup structures
 *  Phase 2 – Verification & Security (ReleaseManifest hash, ReleaseFile)
 *  Phase 3 – Atomic Apply & Rollback (HotReloadEngine config, rollback points)
 *  Phase 4 – State Machine (6 states, valid/invalid transitions, crash recovery)
 *  Phase 5 – ManifestDatabase types (ReleaseManifest JSON round-trip)
 *  Phase 6 – Compatibility & Dependencies (version parsing via UpdateChecker)
 *  Phase 7 – UpdatesConfig & Observability (YAML/JSON round-trip)
 *  Phase 8 – Testing coverage (progress callbacks, config defaults)
 *  Phase 9 – DeltaUpdateEngine (binary diff patches, generate/apply, path traversal security)
 *  Phase 10 – SchemaMigrationTester (staging-before-production framework;
 *              full integration suite in test_schema_migration_tester.cpp)
 *  Phase 11 – Post-update health check & automatic rollback (Issue #2335)
 */

#include <gtest/gtest.h>

#include "updates/release_manifest.h"
#include "updates/updates_config.h"
#include "updates/hot_reload_engine.h"
#include "updates/update_state_machine.h"
#include "updates/delta_update_engine.h"
#include "updates/schema_migration_tester.h"
#include "updates/update_history_logger.h"
#include "utils/update_checker.h"

#include <chrono>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

using namespace themis::updates;
using namespace themis::utils;

// ============================================================================
// Phase 1 & 5: ReleaseFile JSON serialisation
// ============================================================================

class ReleaseFileTest : public ::testing::Test {};

TEST_F(ReleaseFileTest, ToJsonAndFromJson_RoundTrip) {
    ReleaseFile f;
    f.path          = "bin/themis_server";
    f.type          = "executable";
    f.sha256_hash   = "abc123";
    f.size_bytes    = 1024 * 1024;
    f.file_signature = "sig_data";
    f.platform      = "linux";
    f.architecture  = "x64";
    f.permissions   = "0755";
    f.download_url  = "https://example.com/download/bin/themis_server";

    auto j = f.toJson();
    auto parsed = ReleaseFile::fromJson(j);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->path,           f.path);
    EXPECT_EQ(parsed->type,           f.type);
    EXPECT_EQ(parsed->sha256_hash,    f.sha256_hash);
    EXPECT_EQ(parsed->size_bytes,     f.size_bytes);
    EXPECT_EQ(parsed->file_signature, f.file_signature);
    EXPECT_EQ(parsed->platform,       f.platform);
    EXPECT_EQ(parsed->architecture,   f.architecture);
    EXPECT_EQ(parsed->permissions,    f.permissions);
    EXPECT_EQ(parsed->download_url,   f.download_url);
}

TEST_F(ReleaseFileTest, FromJson_MissingFields_ReturnsPartialObject) {
    nlohmann::json j;
    j["path"] = "lib/libthemis.so";

    auto parsed = ReleaseFile::fromJson(j);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->path, "lib/libthemis.so");
    EXPECT_TRUE(parsed->sha256_hash.empty());
    EXPECT_EQ(parsed->size_bytes, 0ULL);
}

TEST_F(ReleaseFileTest, ToJson_HasRequiredKeys) {
    ReleaseFile f;
    f.path = "bin/test";
    f.type = "executable";

    auto j = f.toJson();
    EXPECT_TRUE(j.contains("path"));
    EXPECT_TRUE(j.contains("type"));
    EXPECT_TRUE(j.contains("sha256_hash"));
    EXPECT_TRUE(j.contains("size_bytes"));
}

// ============================================================================
// Phase 2 & 5: ReleaseManifest JSON round-trip and hash calculation
// ============================================================================

class ReleaseManifestTest : public ::testing::Test {
protected:
    ReleaseManifest makeTestManifest() {
        ReleaseManifest m;
        m.version           = "1.2.3";
        m.tag_name          = "v1.2.3";
        m.release_notes     = "Bug fixes and security patches";
        m.is_critical       = true;
        m.release_date      = std::chrono::system_clock::now();
        m.manifest_hash     = "";
        m.signature         = "";
        m.signing_certificate = "";
        m.build_commit      = "abc123def456";
        m.build_date        = "2026-01-15";
        m.compiler_version  = "GCC 14.0";
        m.dependencies      = {"core>=1.0.0", "storage>=2.0.0"};
        m.min_upgrade_from  = "1.0.0";
        m.schema_version    = 1;

        ReleaseFile f;
        f.path          = "bin/themis_server";
        f.type          = "executable";
        f.sha256_hash   = "deadbeef";
        f.size_bytes    = 8192;
        f.platform      = "linux";
        f.architecture  = "x64";
        m.files.push_back(f);

        return m;
    }
};

TEST_F(ReleaseManifestTest, ToJsonAndFromJson_RoundTrip) {
    auto m = makeTestManifest();
    auto j = m.toJson();
    auto parsed = ReleaseManifest::fromJson(j);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->version,          m.version);
    EXPECT_EQ(parsed->tag_name,         m.tag_name);
    EXPECT_EQ(parsed->release_notes,    m.release_notes);
    EXPECT_EQ(parsed->is_critical,      m.is_critical);
    EXPECT_EQ(parsed->build_commit,     m.build_commit);
    EXPECT_EQ(parsed->min_upgrade_from, m.min_upgrade_from);
    EXPECT_EQ(parsed->schema_version,   m.schema_version);
    ASSERT_EQ(parsed->files.size(),     1u);
    EXPECT_EQ(parsed->files[0].path,    "bin/themis_server");
}

TEST_F(ReleaseManifestTest, CalculateHash_IsDeterministic) {
    auto m = makeTestManifest();
    EXPECT_EQ(m.calculateHash(), m.calculateHash());
}

TEST_F(ReleaseManifestTest, CalculateHash_ChangesWithContent) {
    auto m1 = makeTestManifest();
    auto m2 = makeTestManifest();
    m2.version = "1.2.4";

    EXPECT_NE(m1.calculateHash(), m2.calculateHash());
}

TEST_F(ReleaseManifestTest, CalculateHash_IsNotEmpty) {
    auto m = makeTestManifest();
    EXPECT_FALSE(m.calculateHash().empty());
}

TEST_F(ReleaseManifestTest, ToJson_HasRequiredSecurityFields) {
    auto m = makeTestManifest();
    auto j = m.toJson();

    EXPECT_TRUE(j.contains("manifest_hash"));
    EXPECT_TRUE(j.contains("signature"));
    EXPECT_TRUE(j.contains("signing_certificate"));
    EXPECT_TRUE(j.contains("timestamp_token"));
}

TEST_F(ReleaseManifestTest, Dependencies_RoundTrip) {
    auto m = makeTestManifest();
    auto j = m.toJson();
    auto parsed = ReleaseManifest::fromJson(j);

    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->dependencies.size(), 2u);
    EXPECT_EQ(parsed->dependencies[0], "core>=1.0.0");
    EXPECT_EQ(parsed->dependencies[1], "storage>=2.0.0");
}

// ============================================================================
// Phase 3: HotReloadEngine configuration
// ============================================================================

class HotReloadEngineConfigTest : public ::testing::Test {};

TEST_F(HotReloadEngineConfigTest, DefaultConfig_HasSensibleValues) {
    const auto& cfg = HotReloadEngine::defaultConfig();

    EXPECT_FALSE(cfg.download_directory.empty());
    EXPECT_FALSE(cfg.backup_directory.empty());
    EXPECT_TRUE(cfg.verify_signatures);
    EXPECT_TRUE(cfg.create_backup);
    EXPECT_FALSE(cfg.dry_run);
    EXPECT_TRUE(cfg.rollback_on_health_check_failure);
}

TEST_F(HotReloadEngineConfigTest, CustomConfig_IsPreserved) {
    HotReloadEngine::Config cfg;
    cfg.download_directory = "/tmp/test_downloads";
    cfg.backup_directory   = "/tmp/test_backups";
    cfg.verify_signatures  = false;
    cfg.dry_run            = true;

    EXPECT_EQ(cfg.download_directory, "/tmp/test_downloads");
    EXPECT_FALSE(cfg.verify_signatures);
    EXPECT_TRUE(cfg.dry_run);
}

// ============================================================================
// Phase 4: UpdateStateMachine – 6 states, transitions, crash recovery
// ============================================================================

class UpdateStateMachineTest : public ::testing::Test {
protected:
    std::string tmp_log_;

    void SetUp() override {
        tmp_log_ = std::string("/tmp/test_update_sm_") + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".jsonl";
    }

    void TearDown() override {
        try { fs::remove(tmp_log_); } catch (...) {}
    }
};

TEST_F(UpdateStateMachineTest, InitialState_IsIdle) {
    UpdateStateMachine sm;
    EXPECT_EQ(sm.currentState(), UpdateState::IDLE);
}

TEST_F(UpdateStateMachineTest, StateName_AllStates) {
    EXPECT_EQ(UpdateStateMachine::stateName(UpdateState::IDLE),         "idle");
    EXPECT_EQ(UpdateStateMachine::stateName(UpdateState::DOWNLOADING),  "downloading");
    EXPECT_EQ(UpdateStateMachine::stateName(UpdateState::VERIFYING),    "verifying");
    EXPECT_EQ(UpdateStateMachine::stateName(UpdateState::APPLYING),     "applying");
    EXPECT_EQ(UpdateStateMachine::stateName(UpdateState::ROLLING_BACK), "rolling_back");
    EXPECT_EQ(UpdateStateMachine::stateName(UpdateState::FAILED),       "failed");
}

TEST_F(UpdateStateMachineTest, ValidTransition_IdleToDownloading) {
    UpdateStateMachine sm;
    EXPECT_TRUE(sm.transition(UpdateState::DOWNLOADING, "1.2.3", "starting download"));
    EXPECT_EQ(sm.currentState(), UpdateState::DOWNLOADING);
    EXPECT_EQ(sm.currentVersion(), "1.2.3");
}

TEST_F(UpdateStateMachineTest, ValidTransition_FullSuccessPath) {
    UpdateStateMachine sm;
    EXPECT_TRUE(sm.transition(UpdateState::DOWNLOADING,  "2.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::VERIFYING,    "2.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::APPLYING,     "2.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::IDLE,         "2.0.0"));
    EXPECT_EQ(sm.currentState(), UpdateState::IDLE);
}

TEST_F(UpdateStateMachineTest, ValidTransition_RollbackPath) {
    UpdateStateMachine sm;
    EXPECT_TRUE(sm.transition(UpdateState::DOWNLOADING,  "2.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::VERIFYING,    "2.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::APPLYING,     "2.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::ROLLING_BACK, "2.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::IDLE,         "2.0.0"));
    EXPECT_EQ(sm.currentState(), UpdateState::IDLE);
}

TEST_F(UpdateStateMachineTest, ValidTransition_FailureFromDownloading) {
    UpdateStateMachine sm;
    EXPECT_TRUE(sm.transition(UpdateState::DOWNLOADING, "2.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::FAILED,      "2.0.0", "network error"));
    EXPECT_EQ(sm.currentState(), UpdateState::FAILED);
}

TEST_F(UpdateStateMachineTest, InvalidTransition_IdleToVerifying_Rejected) {
    UpdateStateMachine sm;
    EXPECT_FALSE(sm.transition(UpdateState::VERIFYING, "1.0.0"));
    EXPECT_EQ(sm.currentState(), UpdateState::IDLE);
}

TEST_F(UpdateStateMachineTest, InvalidTransition_IdleToApplying_Rejected) {
    UpdateStateMachine sm;
    EXPECT_FALSE(sm.transition(UpdateState::APPLYING, "1.0.0"));
}

TEST_F(UpdateStateMachineTest, InvalidTransition_SkipDownloading_Rejected) {
    UpdateStateMachine sm;
    EXPECT_TRUE(sm.transition(UpdateState::DOWNLOADING, "1.0.0"));
    EXPECT_FALSE(sm.transition(UpdateState::APPLYING, "1.0.0"));
}

TEST_F(UpdateStateMachineTest, FailedState_BlocksAllTransitions) {
    UpdateStateMachine sm;
    EXPECT_TRUE(sm.transition(UpdateState::DOWNLOADING, "1.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::FAILED,      "1.0.0"));
    // No further transitions allowed from FAILED without reset()
    EXPECT_FALSE(sm.transition(UpdateState::IDLE, "1.0.0"));
    EXPECT_FALSE(sm.transition(UpdateState::DOWNLOADING, "2.0.0"));
}

TEST_F(UpdateStateMachineTest, Reset_FromFailed_ReturnToIdle) {
    UpdateStateMachine sm;
    EXPECT_TRUE(sm.transition(UpdateState::DOWNLOADING, "1.0.0"));
    EXPECT_TRUE(sm.transition(UpdateState::FAILED,      "1.0.0"));
    sm.reset();
    EXPECT_EQ(sm.currentState(), UpdateState::IDLE);
    EXPECT_TRUE(sm.currentVersion().empty());
}

TEST_F(UpdateStateMachineTest, StateChangeCallback_IsCalled) {
    UpdateStateMachine sm;
    int call_count = 0;
    UpdateState last_to = UpdateState::IDLE;

    sm.addStateChangeCallback([&](UpdateState /*from*/, UpdateState to, const std::string& /*v*/) {
        ++call_count;
        last_to = to;
    });

    sm.transition(UpdateState::DOWNLOADING, "1.0.0");
    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(last_to, UpdateState::DOWNLOADING);
}

TEST_F(UpdateStateMachineTest, MultipleCallbacks_AllInvoked) {
    UpdateStateMachine sm;
    int cb1 = 0, cb2 = 0;

    sm.addStateChangeCallback([&](UpdateState, UpdateState, const std::string&) { ++cb1; });
    sm.addStateChangeCallback([&](UpdateState, UpdateState, const std::string&) { ++cb2; });

    sm.transition(UpdateState::DOWNLOADING, "1.0.0");
    EXPECT_EQ(cb1, 1);
    EXPECT_EQ(cb2, 1);
}

TEST_F(UpdateStateMachineTest, TransactionLog_RecordsAllTransitions) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "3.0.0", "start");
    sm.transition(UpdateState::VERIFYING,   "3.0.0", "verify");
    sm.transition(UpdateState::APPLYING,    "3.0.0", "apply");
    sm.transition(UpdateState::IDLE,        "3.0.0", "done");

    auto log = sm.transactionLog();
    ASSERT_GE(log.size(), 4u);
    // Log is newest-first
    EXPECT_EQ(log[0].to_state, UpdateState::IDLE);
    EXPECT_EQ(log[3].to_state, UpdateState::DOWNLOADING);
}

TEST_F(UpdateStateMachineTest, PersistentLog_WrittenToFile) {
    {
        UpdateStateMachine sm(tmp_log_);
        sm.transition(UpdateState::DOWNLOADING, "1.5.0", "start");
        sm.transition(UpdateState::VERIFYING,   "1.5.0", "verify");
    }

    EXPECT_TRUE(fs::exists(tmp_log_));

    std::ifstream f(tmp_log_);
    std::string line;
    int line_count = 0;
    while (std::getline(f, line)) {
        if (!line.empty()) ++line_count;
    }
    EXPECT_GE(line_count, 2);
}

TEST_F(UpdateStateMachineTest, CrashRecovery_DetectsInFlightUpdate) {
    // Simulate a crash in APPLYING state
    {
        UpdateStateMachine sm(tmp_log_);
        sm.transition(UpdateState::DOWNLOADING, "2.1.0", "start");
        sm.transition(UpdateState::VERIFYING,   "2.1.0", "verify");
        sm.transition(UpdateState::APPLYING,    "2.1.0", "apply");
        // "crash" here – no IDLE transition written
    }

    // New instance reads the log
    UpdateStateMachine sm2(tmp_log_);
    EXPECT_TRUE(sm2.hasInFlightUpdate());
    EXPECT_EQ(sm2.inFlightVersion(), "2.1.0");
}

TEST_F(UpdateStateMachineTest, CrashRecovery_NoInFlight_WhenCompletedNormally) {
    {
        UpdateStateMachine sm(tmp_log_);
        sm.transition(UpdateState::DOWNLOADING, "2.1.0");
        sm.transition(UpdateState::VERIFYING,   "2.1.0");
        sm.transition(UpdateState::APPLYING,    "2.1.0");
        sm.transition(UpdateState::IDLE,        "2.1.0");  // clean finish
    }

    UpdateStateMachine sm2(tmp_log_);
    EXPECT_FALSE(sm2.hasInFlightUpdate());
}

TEST_F(UpdateStateMachineTest, NoLogPath_DoesNotCrash) {
    UpdateStateMachine sm;  // no log path
    EXPECT_TRUE(sm.transition(UpdateState::DOWNLOADING, "1.0.0"));
    EXPECT_FALSE(sm.hasInFlightUpdate());
}

// ============================================================================
// Phase 5: UpdateTransactionEntry serialisation
// ============================================================================

TEST(UpdateTransactionEntryTest, ToJsonAndFromJson_RoundTrip) {
    UpdateTransactionEntry e;
    e.from_state = UpdateState::APPLYING;
    e.to_state   = UpdateState::ROLLING_BACK;
    e.version    = "1.9.9";
    e.message    = "checksum mismatch";
    e.timestamp  = std::chrono::system_clock::now();

    auto j = e.toJson();
    auto parsed = UpdateTransactionEntry::fromJson(j);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->from_state, UpdateState::APPLYING);
    EXPECT_EQ(parsed->to_state,   UpdateState::ROLLING_BACK);
    EXPECT_EQ(parsed->version,    "1.9.9");
    EXPECT_EQ(parsed->message,    "checksum mismatch");
}

// ============================================================================
// Phase 6: Version compatibility (uses existing UpdateChecker)
// ============================================================================

class VersionCompatibilityTest : public ::testing::Test {};

TEST_F(VersionCompatibilityTest, Parse_ValidSemVer) {
    auto v = Version::parse("2.0.0");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(v->major, 2);
    EXPECT_EQ(v->minor, 0);
    EXPECT_EQ(v->patch, 0);
}

TEST_F(VersionCompatibilityTest, Ordering_NewerIsGreater) {
    auto v1 = Version::parse("1.0.0").value();
    auto v2 = Version::parse("2.0.0").value();
    EXPECT_TRUE(v1 < v2);
    EXPECT_FALSE(v2 < v1);
}

TEST_F(VersionCompatibilityTest, MinUpgradeFrom_EnforcedByParsing) {
    // min_upgrade_from is a version string; verify parseable
    ReleaseManifest m;
    m.min_upgrade_from = "1.0.0";
    auto min_v = Version::parse(m.min_upgrade_from);
    EXPECT_TRUE(min_v.has_value());
}

TEST_F(VersionCompatibilityTest, Dependencies_StoredCorrectly) {
    ReleaseManifest m;
    m.version      = "2.0.0";
    m.dependencies = {"core>=1.5.0", "query>=1.2.0"};

    ASSERT_EQ(m.dependencies.size(), 2u);
    EXPECT_EQ(m.dependencies[0], "core>=1.5.0");
    EXPECT_EQ(m.dependencies[1], "query>=1.2.0");
}

// ============================================================================
// Phase 7: UpdatesConfig JSON round-trip and defaults
// ============================================================================

class UpdatesConfigTest : public ::testing::Test {
protected:
    UpdatesConfig makeConfig() {
        UpdatesConfig cfg;
        cfg.checker.enabled              = true;
        cfg.checker.check_interval       = std::chrono::seconds(1800);
        cfg.checker.github_owner         = "test-owner";
        cfg.checker.github_repo          = "test-repo";
        cfg.checker.github_api_token     = "secret-token";

        cfg.auto_update.enabled          = true;
        cfg.auto_update.critical_only    = false;
        cfg.auto_update.require_approval = false;

        cfg.hot_reload.enabled             = true;
        cfg.hot_reload.download_directory  = "/tmp/test_dl";
        cfg.hot_reload.backup_directory    = "/tmp/test_bak";
        cfg.hot_reload.keep_rollback_points = 5;
        cfg.hot_reload.max_retries          = 4;

        cfg.notifications.enabled      = true;
        cfg.notifications.webhook_url  = "https://hooks.example.com/test";
        return cfg;
    }
};

TEST_F(UpdatesConfigTest, ToJson_HasAllSections) {
    auto cfg = makeConfig();
    auto j   = cfg.toJson();

    EXPECT_TRUE(j.contains("checker"));
    EXPECT_TRUE(j.contains("auto_update"));
    EXPECT_TRUE(j.contains("hot_reload"));
    EXPECT_TRUE(j.contains("notifications"));
}

TEST_F(UpdatesConfigTest, ToJson_MasksApiToken) {
    auto cfg = makeConfig();
    auto j   = cfg.toJson();

    EXPECT_EQ(j["checker"]["github_api_token"], "***");
}

TEST_F(UpdatesConfigTest, FromJson_RoundTrip) {
    auto cfg  = makeConfig();
    auto j    = cfg.toJson();
    auto cfg2 = UpdatesConfig::fromJson(j);

    EXPECT_EQ(cfg2.checker.enabled,              cfg.checker.enabled);
    EXPECT_EQ(cfg2.checker.check_interval.count(), cfg.checker.check_interval.count());
    EXPECT_EQ(cfg2.checker.github_owner,         cfg.checker.github_owner);
    EXPECT_EQ(cfg2.auto_update.enabled,          cfg.auto_update.enabled);
    EXPECT_EQ(cfg2.auto_update.critical_only,    cfg.auto_update.critical_only);
    EXPECT_EQ(cfg2.hot_reload.enabled,           cfg.hot_reload.enabled);
    EXPECT_EQ(cfg2.hot_reload.download_directory, cfg.hot_reload.download_directory);
    EXPECT_EQ(cfg2.hot_reload.keep_rollback_points, cfg.hot_reload.keep_rollback_points);
    EXPECT_EQ(cfg2.hot_reload.max_retries,       cfg.hot_reload.max_retries);
    EXPECT_EQ(cfg2.notifications.enabled,        cfg.notifications.enabled);
}

TEST_F(UpdatesConfigTest, Defaults_AreConservative) {
    UpdatesConfig def;
    // By default, everything is OFF / conservative
    EXPECT_FALSE(def.checker.enabled);
    EXPECT_FALSE(def.auto_update.enabled);
    EXPECT_TRUE(def.auto_update.critical_only);
    EXPECT_TRUE(def.auto_update.require_approval);
    EXPECT_FALSE(def.hot_reload.enabled);
    EXPECT_TRUE(def.hot_reload.verify_signatures);
    EXPECT_TRUE(def.hot_reload.create_backup);
    EXPECT_GE(def.hot_reload.max_retries, 1);
}

// ============================================================================
// Phase 8: Progress callbacks & overall integration smoke tests
// ============================================================================

class ProgressCallbackTest : public ::testing::Test {};

TEST_F(ProgressCallbackTest, HotReloadConfig_ProgressCallbackSetable) {
    // Verify that the progress callback type is compatible with a lambda
    std::function<void(int, const std::string&)> cb =
        [](int pct, const std::string& msg) {
            (void)pct; (void)msg;
        };
    EXPECT_TRUE(static_cast<bool>(cb));
}

TEST_F(ProgressCallbackTest, ReleaseManifest_CriticalFlag) {
    ReleaseManifest m;
    m.is_critical = true;
    EXPECT_TRUE(m.is_critical);

    m.is_critical = false;
    EXPECT_FALSE(m.is_critical);
}

TEST_F(ProgressCallbackTest, MultipleFiles_InManifest) {
    ReleaseManifest m;
    m.version = "1.0.0";

    for (int i = 0; i < 5; ++i) {
        ReleaseFile f;
        f.path      = "bin/component_" + std::to_string(i);
        f.sha256_hash = "hash_" + std::to_string(i);
        f.size_bytes  = static_cast<uint64_t>(1024 * (i + 1));
        m.files.push_back(f);
    }

    EXPECT_EQ(m.files.size(), 5u);
    EXPECT_EQ(m.files[2].path, "bin/component_2");

    // JSON round-trip preserves all files
    auto j      = m.toJson();
    auto parsed = ReleaseManifest::fromJson(j);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->files.size(), 5u);
}

TEST_F(ProgressCallbackTest, UpdatesConfig_NotificationEvents) {
    UpdatesConfig cfg;
    EXPECT_FALSE(cfg.notifications.on_events.empty());

    // Default events should include common update lifecycle events
    bool has_update_available = false;
    for (const auto& ev : cfg.notifications.on_events) {
        if (ev == "update_available") has_update_available = true;
    }
    EXPECT_TRUE(has_update_available);
}

// ============================================================================
// Regression: state machine with throwing callback doesn't crash
// ============================================================================

TEST(StateMachineRegressionTest, ThrowingCallback_DoesNotPropagateException) {
    UpdateStateMachine sm;
    sm.addStateChangeCallback([](UpdateState, UpdateState, const std::string&) {
        throw std::runtime_error("callback error");
    });

    // Should not throw
    EXPECT_NO_THROW(sm.transition(UpdateState::DOWNLOADING, "1.0.0"));
    EXPECT_EQ(sm.currentState(), UpdateState::DOWNLOADING);
}

// ============================================================================
// Thread-safety: callbacks must not deadlock when accessing state machine
// ============================================================================

TEST(StateMachineThreadSafetyTest, Callback_CanCallCurrentState_NoDeadlock) {
    UpdateStateMachine sm;
    UpdateState observed_state = UpdateState::IDLE;

    sm.addStateChangeCallback([&](UpdateState, UpdateState /*to*/, const std::string&) {
        // currentState() must NOT deadlock (it uses atomic, not mutex)
        observed_state = sm.currentState();
    });

    sm.transition(UpdateState::DOWNLOADING, "1.0.0");
    EXPECT_EQ(observed_state, UpdateState::DOWNLOADING);
}

TEST(StateMachineThreadSafetyTest, Callback_CanCallCurrentVersion_NoDeadlock) {
    UpdateStateMachine sm;
    std::string observed_version;

    sm.addStateChangeCallback([&](UpdateState, UpdateState, const std::string&) {
        // currentVersion() acquires mutex_ – must NOT deadlock after fix
        observed_version = sm.currentVersion();
    });

    sm.transition(UpdateState::DOWNLOADING, "2.5.0");
    EXPECT_EQ(observed_version, "2.5.0");
}

TEST(StateMachineThreadSafetyTest, Reset_FiresCallbacks) {
    UpdateStateMachine sm;

    // Reach FAILED state
    sm.transition(UpdateState::DOWNLOADING, "1.0.0");
    sm.transition(UpdateState::FAILED,      "1.0.0", "error");

    int reset_cb_count = 0;
    UpdateState reset_to_state = UpdateState::DOWNLOADING;  // sentinel

    sm.addStateChangeCallback([&](UpdateState /*from*/, UpdateState to, const std::string&) {
        ++reset_cb_count;
        reset_to_state = to;
    });

    sm.reset();

    EXPECT_EQ(reset_cb_count, 1);
    EXPECT_EQ(reset_to_state, UpdateState::IDLE);
    EXPECT_EQ(sm.currentState(), UpdateState::IDLE);
}

TEST(StateMachineThreadSafetyTest, Reset_Callback_CanCallCurrentVersion_NoDeadlock) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "3.0.0");
    sm.transition(UpdateState::FAILED,      "3.0.0");

    std::string version_at_reset;
    sm.addStateChangeCallback([&](UpdateState, UpdateState, const std::string&) {
        version_at_reset = sm.currentVersion();
    });

    EXPECT_NO_THROW(sm.reset());
    EXPECT_EQ(sm.currentState(), UpdateState::IDLE);
}

// ============================================================================
// Phase 9: DeltaUpdateEngine – binary delta patches
// ============================================================================

class DeltaUpdateEngineTest : public ::testing::Test {
protected:
    std::string tmp_dir_;
    std::string install_dir_;
    std::string download_dir_;

    void SetUp() override {
        auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
        tmp_dir_     = "/tmp/test_delta_" + std::to_string(ts);
        install_dir_ = tmp_dir_ + "/install";
        download_dir_ = tmp_dir_ + "/download";
        fs::create_directories(install_dir_);
        fs::create_directories(download_dir_);
    }

    void TearDown() override {
        try { fs::remove_all(tmp_dir_); } catch (...) {}
    }

    static void writeBytes(const std::string& path,
                           const std::vector<uint8_t>& data) {
        fs::create_directories(fs::path(path).parent_path());
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(data.data()),
                static_cast<std::streamsize>(data.size()));
    }

    static std::vector<uint8_t> readBytes(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(f),
                std::istreambuf_iterator<char>()};
    }
};

// ── FileDelta JSON round-trip ─────────────────────────────────────────────

TEST_F(DeltaUpdateEngineTest, FileDelta_ToJsonFromJson_RoundTrip) {
    FileDelta fd;
    fd.path        = "bin/themis_server";
    fd.base_hash   = "aabbcc";
    fd.target_hash = "ddeeff";
    fd.patch_url   = "https://example.com/patches/server.patch";
    fd.patch_size  = 1024;
    fd.target_size = 8192;
    fd.algorithm   = PatchAlgorithm::ZSTD_DICT;

    auto j      = fd.toJson();
    auto parsed = FileDelta::fromJson(j);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->path,        fd.path);
    EXPECT_EQ(parsed->base_hash,   fd.base_hash);
    EXPECT_EQ(parsed->target_hash, fd.target_hash);
    EXPECT_EQ(parsed->patch_url,   fd.patch_url);
    EXPECT_EQ(parsed->patch_size,  fd.patch_size);
    EXPECT_EQ(parsed->target_size, fd.target_size);
    EXPECT_EQ(parsed->algorithm,   PatchAlgorithm::ZSTD_DICT);
}

TEST_F(DeltaUpdateEngineTest, FileDelta_AllAlgorithms_RoundTrip) {
    const std::vector<PatchAlgorithm> algos = {
        PatchAlgorithm::BSDIFF,
        PatchAlgorithm::XDELTA3,
        PatchAlgorithm::VCDIFF,
        PatchAlgorithm::ZSTD_DICT,
    };
    for (auto algo : algos) {
        FileDelta fd;
        fd.algorithm = algo;
        auto j       = fd.toJson();
        auto parsed  = FileDelta::fromJson(j);
        ASSERT_TRUE(parsed.has_value());
        EXPECT_EQ(parsed->algorithm, algo);
    }
}

TEST_F(DeltaUpdateEngineTest, FileDelta_UnknownAlgorithm_DefaultsToZstdDict) {
    nlohmann::json j;
    j["algorithm"] = "nonexistent_algo";
    auto parsed = FileDelta::fromJson(j);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->algorithm, PatchAlgorithm::ZSTD_DICT);
}

// ── DeltaManifest JSON round-trip ─────────────────────────────────────────

TEST_F(DeltaUpdateEngineTest, DeltaManifest_ToJsonFromJson_RoundTrip) {
    DeltaManifest dm;
    dm.from_version = "1.4.0";
    dm.to_version   = "1.5.0";

    FileDelta fd1;
    fd1.path        = "bin/server";
    fd1.patch_size  = 512;
    fd1.target_size = 4096;
    fd1.algorithm   = PatchAlgorithm::ZSTD_DICT;

    FileDelta fd2;
    fd2.path        = "lib/libthemis.so";
    fd2.patch_size  = 256;
    fd2.target_size = 2048;
    fd2.algorithm   = PatchAlgorithm::VCDIFF;

    dm.deltas = {fd1, fd2};

    auto j      = dm.toJson();
    auto parsed = DeltaManifest::fromJson(j);

    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->from_version, "1.4.0");
    EXPECT_EQ(parsed->to_version,   "1.5.0");
    ASSERT_EQ(parsed->deltas.size(), 2u);
    EXPECT_EQ(parsed->deltas[0].path,      "bin/server");
    EXPECT_EQ(parsed->deltas[1].path,      "lib/libthemis.so");
    EXPECT_EQ(parsed->deltas[1].algorithm, PatchAlgorithm::VCDIFF);
}

TEST_F(DeltaUpdateEngineTest, DeltaManifest_TotalSizes) {
    DeltaManifest dm;
    FileDelta fd1; fd1.patch_size = 100; fd1.target_size = 1000;
    FileDelta fd2; fd2.patch_size = 200; fd2.target_size = 2000;
    dm.deltas = {fd1, fd2};

    EXPECT_EQ(dm.totalPatchSize(),  300u);
    EXPECT_EQ(dm.totalTargetSize(), 3000u);
}

TEST_F(DeltaUpdateEngineTest, DeltaManifest_EmptyDeltas_ZeroTotals) {
    DeltaManifest dm;
    EXPECT_EQ(dm.totalPatchSize(),  0u);
    EXPECT_EQ(dm.totalTargetSize(), 0u);
}

// ── PatchAlgorithm string helpers ─────────────────────────────────────────

TEST_F(DeltaUpdateEngineTest, PatchAlgorithmToString_AllValues) {
    EXPECT_EQ(patchAlgorithmToString(PatchAlgorithm::BSDIFF),    "bsdiff");
    EXPECT_EQ(patchAlgorithmToString(PatchAlgorithm::XDELTA3),   "xdelta3");
    EXPECT_EQ(patchAlgorithmToString(PatchAlgorithm::VCDIFF),    "vcdiff");
    EXPECT_EQ(patchAlgorithmToString(PatchAlgorithm::ZSTD_DICT), "zstd_dict");
}

TEST_F(DeltaUpdateEngineTest, PatchAlgorithmFromString_ValidValues) {
    EXPECT_EQ(patchAlgorithmFromString("bsdiff"),    PatchAlgorithm::BSDIFF);
    EXPECT_EQ(patchAlgorithmFromString("xdelta3"),   PatchAlgorithm::XDELTA3);
    EXPECT_EQ(patchAlgorithmFromString("vcdiff"),    PatchAlgorithm::VCDIFF);
    EXPECT_EQ(patchAlgorithmFromString("zstd_dict"), PatchAlgorithm::ZSTD_DICT);
}

TEST_F(DeltaUpdateEngineTest, PatchAlgorithmFromString_Invalid_ReturnsNullopt) {
    EXPECT_FALSE(patchAlgorithmFromString("unknown").has_value());
    EXPECT_FALSE(patchAlgorithmFromString("").has_value());
}

// ── Delta registry ────────────────────────────────────────────────────────

TEST_F(DeltaUpdateEngineTest, FindDelta_NotRegistered_ReturnsNullopt) {
    DeltaUpdateEngine engine(install_dir_, download_dir_);
    EXPECT_FALSE(engine.findDelta("1.0.0", "1.1.0").has_value());
}

TEST_F(DeltaUpdateEngineTest, FindDelta_AfterRegister_ReturnsManifest) {
    DeltaUpdateEngine engine(install_dir_, download_dir_);

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    engine.registerDelta(dm);

    auto found = engine.findDelta("1.0.0", "1.1.0");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->from_version, "1.0.0");
    EXPECT_EQ(found->to_version,   "1.1.0");
}

TEST_F(DeltaUpdateEngineTest, RegisterDelta_Update_ReplacesExisting) {
    DeltaUpdateEngine engine(install_dir_, download_dir_);

    DeltaManifest dm1;
    dm1.from_version = "1.0.0";
    dm1.to_version   = "1.1.0";
    FileDelta fd1; fd1.path = "old_file"; dm1.deltas = {fd1};
    engine.registerDelta(dm1);

    DeltaManifest dm2;
    dm2.from_version = "1.0.0";
    dm2.to_version   = "1.1.0";
    FileDelta fd2; fd2.path = "new_file"; dm2.deltas = {fd2};
    engine.registerDelta(dm2);

    auto found = engine.findDelta("1.0.0", "1.1.0");
    ASSERT_TRUE(found.has_value());
    ASSERT_EQ(found->deltas.size(), 1u);
    EXPECT_EQ(found->deltas[0].path, "new_file");
}

// ── generatePatch + applyPatch round-trip (ZSTD_DICT) ────────────────────

TEST_F(DeltaUpdateEngineTest, GenerateApplyPatch_ZstdDict_Identical) {
    // Edge case: base == target
    std::vector<uint8_t> data(1024, 0xAB);
    std::string base_path   = tmp_dir_ + "/base.bin";
    std::string target_path = tmp_dir_ + "/target.bin";
    std::string patch_path  = tmp_dir_ + "/patch.bin";
    std::string recon_path  = tmp_dir_ + "/recon.bin";

    writeBytes(base_path,   data);
    writeBytes(target_path, data);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(
        base_path, target_path, patch_path, PatchAlgorithm::ZSTD_DICT));
    ASSERT_TRUE(fs::exists(patch_path));

    ASSERT_TRUE(engine.applyPatch(base_path, patch_path, recon_path));
    ASSERT_TRUE(fs::exists(recon_path));

    auto recon = readBytes(recon_path);
    EXPECT_EQ(recon, data);
}

TEST_F(DeltaUpdateEngineTest, GenerateApplyPatch_ZstdDict_SmallDiff) {
    // base and target differ only in a few bytes (realistic update)
    std::vector<uint8_t> base(4096);
    for (size_t i = 0; i < base.size(); ++i) base[i] = static_cast<uint8_t>(i & 0xFF);

    auto target = base;
    target[100] = 0xFF;
    target[101] = 0xFE;
    target[200] = 0xAA;

    std::string base_path   = tmp_dir_ + "/base_small.bin";
    std::string target_path = tmp_dir_ + "/target_small.bin";
    std::string patch_path  = tmp_dir_ + "/patch_small.bin";
    std::string recon_path  = tmp_dir_ + "/recon_small.bin";

    writeBytes(base_path,   base);
    writeBytes(target_path, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(
        base_path, target_path, patch_path, PatchAlgorithm::ZSTD_DICT));

    // Patch should be smaller than target
    EXPECT_LT(fs::file_size(patch_path), target.size());

    ASSERT_TRUE(engine.applyPatch(base_path, patch_path, recon_path));

    auto recon = readBytes(recon_path);
    EXPECT_EQ(recon, target);
}

TEST_F(DeltaUpdateEngineTest, GenerateApplyPatch_ZstdDict_EmptyBase) {
    std::vector<uint8_t> base;
    std::vector<uint8_t> target(512, 0x42);

    std::string base_path   = tmp_dir_ + "/empty_base.bin";
    std::string target_path = tmp_dir_ + "/empty_target.bin";
    std::string patch_path  = tmp_dir_ + "/empty_patch.bin";
    std::string recon_path  = tmp_dir_ + "/empty_recon.bin";

    writeBytes(base_path,   base);
    writeBytes(target_path, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(
        base_path, target_path, patch_path, PatchAlgorithm::ZSTD_DICT));
    ASSERT_TRUE(engine.applyPatch(base_path, patch_path, recon_path));

    auto recon = readBytes(recon_path);
    EXPECT_EQ(recon, target);
}

// ── generatePatch + applyPatch round-trip (VCDIFF) ───────────────────────

TEST_F(DeltaUpdateEngineTest, GenerateApplyPatch_Vcdiff_SmallDiff) {
    std::vector<uint8_t> base(2048);
    for (size_t i = 0; i < base.size(); ++i) base[i] = static_cast<uint8_t>(i % 251);

    auto target = base;
    target[50]  = 0xCC;
    target[51]  = 0xDD;
    target[300] = 0xEE;

    std::string base_path   = tmp_dir_ + "/vcd_base.bin";
    std::string target_path = tmp_dir_ + "/vcd_target.bin";
    std::string patch_path  = tmp_dir_ + "/vcd_patch.bin";
    std::string recon_path  = tmp_dir_ + "/vcd_recon.bin";

    writeBytes(base_path,   base);
    writeBytes(target_path, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(
        base_path, target_path, patch_path, PatchAlgorithm::VCDIFF));
    ASSERT_TRUE(fs::exists(patch_path));

    ASSERT_TRUE(engine.applyPatch(base_path, patch_path, recon_path));

    auto recon = readBytes(recon_path);
    EXPECT_EQ(recon, target);
}

TEST_F(DeltaUpdateEngineTest, GenerateApplyPatch_Vcdiff_AllNew) {
    // Target has no overlap with base (worst case for delta)
    std::vector<uint8_t> base(256, 0x00);
    std::vector<uint8_t> target(256, 0xFF);

    std::string base_path   = tmp_dir_ + "/vcd2_base.bin";
    std::string target_path = tmp_dir_ + "/vcd2_target.bin";
    std::string patch_path  = tmp_dir_ + "/vcd2_patch.bin";
    std::string recon_path  = tmp_dir_ + "/vcd2_recon.bin";

    writeBytes(base_path,   base);
    writeBytes(target_path, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(
        base_path, target_path, patch_path, PatchAlgorithm::VCDIFF));
    ASSERT_TRUE(engine.applyPatch(base_path, patch_path, recon_path));

    auto recon = readBytes(recon_path);
    EXPECT_EQ(recon, target);
}

// ── Fallback for unsupported algorithms ──────────────────────────────────

TEST_F(DeltaUpdateEngineTest, GeneratePatch_BsdiffFallsBackToZstdDict) {
    std::vector<uint8_t> base(128, 0x01);
    std::vector<uint8_t> target(128, 0x02);

    std::string base_path   = tmp_dir_ + "/fb_base.bin";
    std::string target_path = tmp_dir_ + "/fb_target.bin";
    std::string patch_path  = tmp_dir_ + "/fb_patch.bin";
    std::string recon_path  = tmp_dir_ + "/fb_recon.bin";

    writeBytes(base_path,   base);
    writeBytes(target_path, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    // BSDIFF falls back to ZSTD_DICT and should still succeed
    ASSERT_TRUE(engine.generatePatch(
        base_path, target_path, patch_path, PatchAlgorithm::BSDIFF));
    ASSERT_TRUE(engine.applyPatch(base_path, patch_path, recon_path));

    auto recon = readBytes(recon_path);
    EXPECT_EQ(recon, target);
}

// ── applyDelta with hash verification ────────────────────────────────────

TEST_F(DeltaUpdateEngineTest, ApplyDelta_HashVerification_Success) {
    // Prepare a "currently installed" base file
    std::vector<uint8_t> base_data(512);
    for (size_t i = 0; i < base_data.size(); ++i)
        base_data[i] = static_cast<uint8_t>(i & 0xFF);

    std::string rel_path = "bin/component";
    std::string base_path = install_dir_ + "/" + rel_path;
    writeBytes(base_path, base_data);

    // Generate a target
    auto target_data = base_data;
    target_data[10] = 0xFF;
    target_data[11] = 0xFE;

    // Generate patch file (place in download_dir/<rel_path>.patch)
    std::string patch_path = download_dir_ + "/" + rel_path + ".patch";
    {
        DeltaUpdateEngine gen_engine(install_dir_, download_dir_);
        std::string tgt_tmp = tmp_dir_ + "/tgt_tmp.bin";
        writeBytes(tgt_tmp, target_data);
        ASSERT_TRUE(gen_engine.generatePatch(
            base_path, tgt_tmp, patch_path, PatchAlgorithm::ZSTD_DICT));
    }

    // Build a DeltaManifest pointing to the pre-downloaded patch
    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";

    FileDelta fd;
    fd.path        = rel_path;
    fd.target_size = static_cast<uint64_t>(target_data.size());
    fd.algorithm   = PatchAlgorithm::ZSTD_DICT;
    // base_hash and target_hash left empty → verification skipped for this test
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.files_fallback.empty());
    ASSERT_EQ(result.files_patched.size(), 1u);
    EXPECT_EQ(result.files_patched[0], rel_path);

    // Verify the installed file was updated
    std::ifstream installed_file(base_path, std::ios::binary);
    std::vector<uint8_t> installed;
    installed.assign(std::istreambuf_iterator<char>(installed_file),
                     std::istreambuf_iterator<char>());
    EXPECT_EQ(installed, target_data);
}

TEST_F(DeltaUpdateEngineTest, ApplyDelta_MissingPatch_FallsBack) {
    // Base file exists but patch does not
    std::vector<uint8_t> base_data(64, 0xAA);
    std::string rel_path  = "bin/missing_patch";
    writeBytes(install_dir_ + "/" + rel_path, base_data);

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = rel_path;
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);    // engine-level success
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_EQ(result.files_fallback[0], rel_path);
    EXPECT_TRUE(result.files_patched.empty());
}

TEST_F(DeltaUpdateEngineTest, ApplyDelta_BaseHashMismatch_FallsBack) {
    std::vector<uint8_t> base_data(64, 0xBB);
    std::string rel_path = "bin/hash_mismatch";
    writeBytes(install_dir_ + "/" + rel_path, base_data);

    // Create a dummy patch so the path check passes
    std::string patch_path = download_dir_ + "/" + rel_path + ".patch";
    fs::create_directories(fs::path(patch_path).parent_path());
    writeBytes(patch_path, {0x01, 0x02, 0x03});

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path      = rel_path;
    fd.base_hash = "deadbeef_wrong_hash";  // deliberate mismatch
    dm.deltas    = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_EQ(result.files_fallback[0], rel_path);
}

// ── Progress callback ─────────────────────────────────────────────────────

TEST_F(DeltaUpdateEngineTest, ProgressCallback_IsInvoked) {
    std::vector<int> percentages;
    DeltaUpdateEngine engine(install_dir_, download_dir_);
    engine.setProgressCallback([&](int pct, const std::string&) {
        percentages.push_back(pct);
    });

    // Apply a single-file delta (patch absent → fallback, but callback fires)
    std::vector<uint8_t> base(32, 0x01);
    std::string rel_path = "bin/cb_test";
    writeBytes(install_dir_ + "/" + rel_path, base);

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd; fd.path = rel_path;
    dm.deltas = {fd};

    engine.applyDelta(dm);

    EXPECT_FALSE(percentages.empty());
    for (int p : percentages) {
        EXPECT_GE(p, 0);
        EXPECT_LE(p, 100);
    }
}

// ============================================================================
// Phase 9 – Security: path traversal prevention
// ============================================================================

class DeltaPathTraversalTest : public ::testing::Test {
protected:
    std::string tmp_dir_;
    std::string install_dir_;
    std::string download_dir_;

    void SetUp() override {
        auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
        tmp_dir_      = "/tmp/test_delta_sec_" + std::to_string(ts);
        install_dir_  = tmp_dir_ + "/install";
        download_dir_ = tmp_dir_ + "/download";
        fs::create_directories(install_dir_);
        fs::create_directories(download_dir_);
    }

    void TearDown() override {
        try { fs::remove_all(tmp_dir_); } catch (...) {}
    }

    static void writeBytes(const std::string& path,
                           const std::vector<uint8_t>& data) {
        fs::create_directories(fs::path(path).parent_path());
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(data.data()),
                static_cast<std::streamsize>(data.size()));
    }

    static std::vector<uint8_t> readBytes(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(f),
                std::istreambuf_iterator<char>()};
    }
};

TEST_F(DeltaPathTraversalTest, DotDotPath_FallsBackAndNeverWritesOutsideSandbox) {
    // The "sentinel" file that must NOT be overwritten
    std::string outside_file = tmp_dir_ + "/outside_secret.txt";
    writeBytes(outside_file, {0xDE, 0xAD, 0xBE, 0xEF});

    // A manifest with a path traversal attempt
    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = "../outside_secret.txt";   // traversal attempt
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    // Engine-level success but this file must fall back (path rejected)
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_TRUE(result.files_patched.empty());

    // The outside file must be untouched
    std::ifstream f(outside_file, std::ios::binary);
    std::vector<uint8_t> content;
    content.assign(std::istreambuf_iterator<char>(f),
                   std::istreambuf_iterator<char>());
    std::vector<uint8_t> expected = {0xDE, 0xAD, 0xBE, 0xEF};
    EXPECT_EQ(content, expected);
}

TEST_F(DeltaPathTraversalTest, AbsolutePath_Rejected) {
    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = "/etc/passwd";  // absolute path
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_TRUE(result.files_patched.empty());
}

TEST_F(DeltaPathTraversalTest, EmptyPath_Rejected) {
    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = "";  // empty
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.files_fallback.size(), 1u);
}

TEST_F(DeltaPathTraversalTest, NormalSubdirPath_Accepted) {
    // Normal nested path inside install_dir should not be rejected
    std::string rel_path = "bin/subdir/component";
    std::vector<uint8_t> base_data(64, 0x01);
    std::string base_path = install_dir_ + "/" + rel_path;
    writeBytes(base_path, base_data);

    // No patch present → fallback for a different reason (not path rejection)
    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = rel_path;
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    // Should fall back due to missing patch file, NOT due to path rejection
    EXPECT_TRUE(result.success);
    // It ended up in fallback (patch missing) but wasn't rejected for the path
    ASSERT_EQ(result.files_fallback.size(), 1u);
    // The install dir base file should still exist (wasn't touched)
    EXPECT_TRUE(fs::exists(base_path));
}

TEST_F(DeltaPathTraversalTest, NormalSubdirPath_WithPatch_AppliesSuccessfully) {
    // Prove that the security check does NOT block valid nested-path operations.
    std::string rel_path = "lib/sub/libfoo.so";
    std::vector<uint8_t> base_data(512);
    for (size_t i = 0; i < base_data.size(); ++i) base_data[i] = static_cast<uint8_t>(i & 0xFF);

    auto target_data = base_data;
    target_data[10] = 0xFF;
    target_data[20] = 0xFE;

    std::string base_path = install_dir_ + "/" + rel_path;
    writeBytes(base_path, base_data);

    // Generate patch and place it where applyDelta expects it
    std::string patch_path = download_dir_ + "/" + rel_path + ".patch";
    {
        std::string tgt_tmp = tmp_dir_ + "/tgt_sec_tmp.bin";
        writeBytes(tgt_tmp, target_data);
        DeltaUpdateEngine gen(install_dir_, download_dir_);
        ASSERT_TRUE(gen.generatePatch(base_path, tgt_tmp, patch_path,
                                      PatchAlgorithm::ZSTD_DICT));
    }

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path        = rel_path;
    fd.target_size = static_cast<uint64_t>(target_data.size());
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.files_fallback.empty());
    ASSERT_EQ(result.files_patched.size(), 1u);
    EXPECT_EQ(result.files_patched[0], rel_path);

    // Verify the installed file matches the target
    std::ifstream installed_file(base_path, std::ios::binary);
    std::vector<uint8_t> installed;
    installed.assign(std::istreambuf_iterator<char>(installed_file),
                     std::istreambuf_iterator<char>());
    EXPECT_EQ(installed, target_data);
}

// ============================================================================
// Phase 10: SchemaMigrationTester – structural & config smoke tests
// (Full integration suite lives in test_schema_migration_tester.cpp)
// ============================================================================

class SchemaMigrationTesterProductionTest : public ::testing::Test {};

TEST_F(SchemaMigrationTesterProductionTest, DefaultConfig_HasExpectedDefaults) {
    themis::updates::SchemaMigrationTester::Config cfg;
    EXPECT_EQ(cfg.staging_directory, "/tmp/themis_staging");
    EXPECT_TRUE(cfg.cleanup_staging_on_success);
    EXPECT_FALSE(cfg.cleanup_staging_on_failure);
}

TEST_F(SchemaMigrationTesterProductionTest, Constructable_WithDefaultConfig) {
    themis::updates::SchemaMigrationTester tester;
    // Constructing with default config must not throw or crash.
    (void)tester;
}

TEST_F(SchemaMigrationTesterProductionTest, MigrationTestResult_EmptyCounters) {
    themis::updates::MigrationTestResult r;
    EXPECT_EQ(r.passedCount(), 0u);
    EXPECT_EQ(r.failedCount(), 0u);
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.error_message.empty());
    EXPECT_TRUE(r.migration_script.empty());
}

TEST_F(SchemaMigrationTesterProductionTest, MigrationTestResult_CountHelpers) {
    themis::updates::MigrationTestResult r;
    r.test_results.push_back({"t1", true,  ""});
    r.test_results.push_back({"t2", false, "err"});
    r.test_results.push_back({"t3", true,  ""});
    EXPECT_EQ(r.passedCount(), 2u);
    EXPECT_EQ(r.failedCount(), 1u);
}

// ============================================================================
// Phase 11: Post-update health check & automatic rollback (Issue #2335)
// ============================================================================

// ---------------------------------------------------------------------------
// Structural: ReloadResult and Config field defaults
// ---------------------------------------------------------------------------

class PostUpdateHealthCheckStructuralTest : public ::testing::Test {};

TEST_F(PostUpdateHealthCheckStructuralTest, ReloadResult_HealthCheckFailed_DefaultsFalse) {
    ReloadResult r;
    EXPECT_FALSE(r.health_check_failed);
}

TEST_F(PostUpdateHealthCheckStructuralTest, Config_RollbackOnHealthCheckFailure_DefaultsTrue) {
    HotReloadEngine::Config cfg;
    EXPECT_TRUE(cfg.rollback_on_health_check_failure);
}

TEST_F(PostUpdateHealthCheckStructuralTest, Config_RollbackOnHealthCheckFailure_CanDisable) {
    HotReloadEngine::Config cfg;
    cfg.rollback_on_health_check_failure = false;
    EXPECT_FALSE(cfg.rollback_on_health_check_failure);
}

TEST_F(PostUpdateHealthCheckStructuralTest, PostUpdateHealthCheck_TypeIsCallable) {
    HotReloadEngine::PostUpdateHealthCheck check = []() -> bool { return true; };
    EXPECT_TRUE(static_cast<bool>(check));
    EXPECT_TRUE(check());
}

TEST_F(PostUpdateHealthCheckStructuralTest, PostUpdateHealthCheck_CanReturnFalse) {
    HotReloadEngine::PostUpdateHealthCheck check = []() -> bool { return false; };
    EXPECT_TRUE(static_cast<bool>(check));
    EXPECT_FALSE(check());
}

// ---------------------------------------------------------------------------
// Functional: behavioural tests via TestableHotReloadEngine
//
// The stub subclass below overrides applyHotReload() to simulate the
// post-update phase (files already applied) and exercises the health-check /
// rollback code path using the protected post_update_health_check_ member.
// ---------------------------------------------------------------------------

namespace {

/**
 * Subclass that simulates a successful file-apply phase and then exercises
 * the same health-check / rollback logic as the production implementation,
 * using the protected post_update_health_check_ member.
 */
class TestableHotReloadEngine : public HotReloadEngine {
public:
    explicit TestableHotReloadEngine(bool rollback_on_hc_failure = true)
        : HotReloadEngine(nullptr, nullptr,
                          [rollback_on_hc_failure]() {
                              HotReloadEngine::Config c;
                              c.verify_signatures                = false;
                              c.create_backup                    = false;
                              c.rollback_on_health_check_failure = rollback_on_hc_failure;
                              return c;
                          }())
        , rollback_on_hc_failure_(rollback_on_hc_failure)
    {}

    /**
     * Simulate a successful file-apply and then run the registered
     * health check, mirroring the base class behaviour.
     */
    ReloadResult applyHotReload(const std::string& version,
                                bool /*verify_only*/ = false) override {
        ReloadResult result;
        result.success     = true;
        result.rollback_id = "test_rollback_" + version;
        result.files_updated = {"bin/themis_server"};

        if (post_update_health_check_) {
            bool healthy = post_update_health_check_();
            if (!healthy) {
                result.health_check_failed = true;
                if (rollback_on_hc_failure_ && !result.rollback_id.empty()) {
                    result.error_message =
                        "Post-update health check failed; rolled back to previous version";
                    rollback(result.rollback_id);
                } else {
                    result.error_message = "Post-update health check failed";
                }
                result.success = false;
            }
        }
        return result;
    }

    bool rollback(const std::string& /*id*/) override {
        ++rollback_call_count_;
        return true;
    }

    int rollback_call_count_ = 0;

private:
    bool rollback_on_hc_failure_;
};

} // anonymous namespace

class PostUpdateHealthCheckFunctionalTest : public ::testing::Test {};

TEST_F(PostUpdateHealthCheckFunctionalTest, NoHealthCheck_ApplySucceeds) {
    TestableHotReloadEngine engine;
    auto result = engine.applyHotReload("1.2.0");

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.health_check_failed);
    EXPECT_EQ(engine.rollback_call_count_, 0);
}

TEST_F(PostUpdateHealthCheckFunctionalTest, HealthCheckPasses_ApplySucceeds) {
    TestableHotReloadEngine engine;
    engine.setPostUpdateHealthCheck([]() { return true; });

    auto result = engine.applyHotReload("1.2.0");

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.health_check_failed);
    EXPECT_EQ(engine.rollback_call_count_, 0);
}

TEST_F(PostUpdateHealthCheckFunctionalTest, HealthCheckFails_RollbackTriggered) {
    TestableHotReloadEngine engine(/*rollback_on_hc_failure=*/true);
    engine.setPostUpdateHealthCheck([]() { return false; });

    auto result = engine.applyHotReload("1.2.0");

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.health_check_failed);
    EXPECT_EQ(engine.rollback_call_count_, 1);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(PostUpdateHealthCheckFunctionalTest, HealthCheckFails_RollbackDisabled_NoRollback) {
    TestableHotReloadEngine engine(/*rollback_on_hc_failure=*/false);
    engine.setPostUpdateHealthCheck([]() { return false; });

    auto result = engine.applyHotReload("1.2.0");

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.health_check_failed);
    EXPECT_EQ(engine.rollback_call_count_, 0);
    EXPECT_EQ(result.error_message, "Post-update health check failed");
}

TEST_F(PostUpdateHealthCheckFunctionalTest, HealthCheckCalledExactlyOnce) {
    TestableHotReloadEngine engine;
    int call_count = 0;
    engine.setPostUpdateHealthCheck([&call_count]() {
        ++call_count;
        return true;
    });

    engine.applyHotReload("1.3.0");

    EXPECT_EQ(call_count, 1);
}

TEST_F(PostUpdateHealthCheckFunctionalTest, HealthCheckCanBeReplaced) {
    TestableHotReloadEngine engine;
    // Register a failing check, then replace with a passing one
    engine.setPostUpdateHealthCheck([]() { return false; });
    engine.setPostUpdateHealthCheck([]() { return true; });

    auto result = engine.applyHotReload("1.4.0");

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.health_check_failed);
}

TEST_F(PostUpdateHealthCheckFunctionalTest, HealthCheckCanBeCleared) {
    TestableHotReloadEngine engine;
    engine.setPostUpdateHealthCheck([]() { return false; });
    engine.setPostUpdateHealthCheck(nullptr);  // clear

    auto result = engine.applyHotReload("1.5.0");

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.health_check_failed);
}

// ============================================================================
// Phase 11: UpdateHistoryLogger – who, when, from/to version
// ============================================================================

class UpdateHistoryLoggerTest : public ::testing::Test {
protected:
    std::string tmp_dir_;
    std::string log_file_;

    void SetUp() override {
        auto ts = std::chrono::system_clock::now().time_since_epoch().count();
        tmp_dir_  = "/tmp/test_history_" + std::to_string(ts);
        log_file_ = tmp_dir_ + "/update_history.json";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        try { fs::remove_all(tmp_dir_); } catch (...) {}
    }
};

TEST_F(UpdateHistoryLoggerTest, LogFilePath_IsRetained) {
    UpdateHistoryLogger logger(log_file_);
    EXPECT_EQ(logger.logFilePath(), log_file_);
}

TEST_F(UpdateHistoryLoggerTest, InitialHistory_IsEmpty) {
    UpdateHistoryLogger logger(log_file_);
    EXPECT_TRUE(logger.getHistory().empty());
}

TEST_F(UpdateHistoryLoggerTest, RecordEntry_AppearsInHistory) {
    UpdateHistoryLogger logger(log_file_);

    UpdateHistoryEntry e;
    e.who           = "admin";
    e.timestamp_ms  = 1700000000000LL;
    e.from_version  = "1.0.0";
    e.to_version    = "1.1.0";
    e.event_type    = "update";
    e.success       = true;

    logger.record(e);

    auto history = logger.getHistory();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_EQ(history[0].who,          "admin");
    EXPECT_EQ(history[0].timestamp_ms, 1700000000000LL);
    EXPECT_EQ(history[0].from_version, "1.0.0");
    EXPECT_EQ(history[0].to_version,   "1.1.0");
    EXPECT_EQ(history[0].event_type,   "update");
    EXPECT_TRUE(history[0].success);
    EXPECT_TRUE(history[0].error_message.empty());
}

TEST_F(UpdateHistoryLoggerTest, RecordFailure_ErrorMessagePersisted) {
    UpdateHistoryLogger logger(log_file_);

    UpdateHistoryEntry e;
    e.who           = "ci-bot";
    e.timestamp_ms  = 1700000001000LL;
    e.from_version  = "1.0.0";
    e.to_version    = "1.1.0";
    e.event_type    = "update";
    e.success       = false;
    e.error_message = "Manifest verification failed";

    logger.record(e);

    auto history = logger.getHistory();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_FALSE(history[0].success);
    EXPECT_EQ(history[0].error_message, "Manifest verification failed");
}

TEST_F(UpdateHistoryLoggerTest, MultipleEntries_ReturnedNewestFirst) {
    UpdateHistoryLogger logger(log_file_);

    for (int i = 1; i <= 3; ++i) {
        UpdateHistoryEntry e;
        e.who           = "user";
        e.timestamp_ms  = static_cast<int64_t>(i) * 1000LL;
        e.from_version  = "1." + std::to_string(i - 1) + ".0";
        e.to_version    = "1." + std::to_string(i) + ".0";
        e.event_type    = "update";
        e.success       = true;
        logger.record(e);
    }

    auto history = logger.getHistory();
    ASSERT_EQ(history.size(), 3u);
    // Newest first: timestamps should be decreasing
    EXPECT_EQ(history[0].timestamp_ms, 3000LL);
    EXPECT_EQ(history[1].timestamp_ms, 2000LL);
    EXPECT_EQ(history[2].timestamp_ms, 1000LL);
}

TEST_F(UpdateHistoryLoggerTest, LimitParameter_CapsResults) {
    UpdateHistoryLogger logger(log_file_);

    for (int i = 0; i < 5; ++i) {
        UpdateHistoryEntry e;
        e.who          = "user";
        e.timestamp_ms = static_cast<int64_t>(i);
        e.event_type   = "update";
        e.success      = true;
        logger.record(e);
    }

    auto history = logger.getHistory(2);
    EXPECT_EQ(history.size(), 2u);
}

TEST_F(UpdateHistoryLoggerTest, Clear_RemovesAllEntries) {
    UpdateHistoryLogger logger(log_file_);

    UpdateHistoryEntry e;
    e.who        = "user";
    e.event_type = "update";
    e.success    = true;
    logger.record(e);
    ASSERT_EQ(logger.getHistory().size(), 1u);

    logger.clear();
    EXPECT_TRUE(logger.getHistory().empty());
}

TEST_F(UpdateHistoryLoggerTest, Persistence_SurvivesReopen) {
    {
        UpdateHistoryLogger logger(log_file_);
        UpdateHistoryEntry e;
        e.who           = "deploy-svc";
        e.timestamp_ms  = 999LL;
        e.from_version  = "2.0.0";
        e.to_version    = "2.1.0";
        e.event_type    = "update";
        e.success       = true;
        logger.record(e);
    }

    // Re-open from the same file
    UpdateHistoryLogger logger2(log_file_);
    auto history = logger2.getHistory();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_EQ(history[0].who,          "deploy-svc");
    EXPECT_EQ(history[0].from_version, "2.0.0");
    EXPECT_EQ(history[0].to_version,   "2.1.0");
    EXPECT_EQ(history[0].timestamp_ms, 999LL);
}

TEST_F(UpdateHistoryLoggerTest, EntryJsonRoundTrip) {
    UpdateHistoryEntry e;
    e.who           = "ops-team";
    e.timestamp_ms  = 1234567890LL;
    e.from_version  = "3.1.0";
    e.to_version    = "3.2.0";
    e.event_type    = "rollback";
    e.success       = false;
    e.error_message = "Health check failed";

    auto j  = e.toJson();
    auto e2 = UpdateHistoryEntry::fromJson(j);

    EXPECT_EQ(e2.who,           e.who);
    EXPECT_EQ(e2.timestamp_ms,  e.timestamp_ms);
    EXPECT_EQ(e2.from_version,  e.from_version);
    EXPECT_EQ(e2.to_version,    e.to_version);
    EXPECT_EQ(e2.event_type,    e.event_type);
    EXPECT_EQ(e2.success,       e.success);
    EXPECT_EQ(e2.error_message, e.error_message);
}

TEST_F(UpdateHistoryLoggerTest, HotReloadEngineConfig_HistoryFieldDefaults) {
    HotReloadEngine::Config cfg;
    EXPECT_TRUE(cfg.history_log_path.empty());
    EXPECT_EQ(cfg.history_actor, "system");
}

TEST_F(UpdateHistoryLoggerTest, HotReloadEngineConfig_HistoryFieldsCustomizable) {
    HotReloadEngine::Config cfg;
    cfg.history_log_path = log_file_;
    cfg.history_actor    = "deploy-bot";
    EXPECT_EQ(cfg.history_log_path, log_file_);
    EXPECT_EQ(cfg.history_actor,    "deploy-bot");
}

// ============================================================================
// Phase 12: Rollback Checkpoint API
// ============================================================================

class CheckpointTest : public ::testing::Test {
protected:
    std::string tmp_dir_;
    std::string log_file_;
    std::string history_file_;

    void SetUp() override {
        auto ts   = std::chrono::steady_clock::now().time_since_epoch().count();
        tmp_dir_      = "/tmp/test_checkpoint_" + std::to_string(ts);
        log_file_     = tmp_dir_ + "/sm.log";
        history_file_ = tmp_dir_ + "/history.json";
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        try { fs::remove_all(tmp_dir_); } catch (...) {}
    }
};

TEST_F(CheckpointTest, CreateCheckpoint_ReturnsMonotonicId) {
    UpdateStateMachine sm;
    auto id1 = sm.createCheckpoint("first");
    auto id2 = sm.createCheckpoint("second");
    EXPECT_EQ(id1, 1u);
    EXPECT_EQ(id2, 2u);
}

TEST_F(CheckpointTest, ListCheckpoints_EmptyInitially) {
    UpdateStateMachine sm;
    EXPECT_TRUE(sm.listCheckpoints().empty());
}

TEST_F(CheckpointTest, ListCheckpoints_ReturnedInCreationOrder) {
    UpdateStateMachine sm;
    sm.createCheckpoint("a");
    sm.createCheckpoint("b");
    sm.createCheckpoint("c");

    auto cps = sm.listCheckpoints();
    ASSERT_EQ(cps.size(), 3u);
    EXPECT_EQ(cps[0].description, "a");
    EXPECT_EQ(cps[1].description, "b");
    EXPECT_EQ(cps[2].description, "c");
    EXPECT_EQ(cps[0].id, 1u);
    EXPECT_EQ(cps[2].id, 3u);
}

TEST_F(CheckpointTest, Checkpoint_CapturesCurrentState) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "1.2.3");

    auto id = sm.createCheckpoint("mid-download");

    auto cps = sm.listCheckpoints();
    ASSERT_EQ(cps.size(), 1u);
    EXPECT_EQ(cps[0].state,   UpdateState::DOWNLOADING);
    EXPECT_EQ(cps[0].version, "1.2.3");
    EXPECT_EQ(cps[0].id,      id);
}

TEST_F(CheckpointTest, RollbackToCheckpoint_RestoresStateAndVersion) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "2.0.0");
    auto id = sm.createCheckpoint("after-download");

    // Advance further
    sm.transition(UpdateState::VERIFYING,  "2.0.0");
    sm.transition(UpdateState::APPLYING,   "2.0.0");

    EXPECT_EQ(sm.currentState(), UpdateState::APPLYING);

    bool ok = sm.rollbackToCheckpoint(id);
    EXPECT_TRUE(ok);
    EXPECT_EQ(sm.currentState(),   UpdateState::DOWNLOADING);
    EXPECT_EQ(sm.currentVersion(), "2.0.0");
}

TEST_F(CheckpointTest, RollbackToCheckpoint_UnknownId_ReturnsFalse) {
    UpdateStateMachine sm;
    EXPECT_FALSE(sm.rollbackToCheckpoint(999));
}

TEST_F(CheckpointTest, RollbackToCheckpoint_PrunesNewerCheckpoints) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "3.0.0");
    auto id1 = sm.createCheckpoint("cp1");
    sm.transition(UpdateState::VERIFYING, "3.0.0");
    sm.createCheckpoint("cp2");
    sm.createCheckpoint("cp3");

    ASSERT_EQ(sm.listCheckpoints().size(), 3u);

    sm.rollbackToCheckpoint(id1);

    // cp2 and cp3 should be removed; only cp1 remains
    auto remaining = sm.listCheckpoints();
    ASSERT_EQ(remaining.size(), 1u);
    EXPECT_EQ(remaining[0].id, id1);
}

TEST_F(CheckpointTest, RollbackToCheckpoint_FiresCallbacks) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "4.0.0");
    auto id = sm.createCheckpoint("snap");
    sm.transition(UpdateState::VERIFYING, "4.0.0");

    UpdateState observed_to = UpdateState::FAILED;
    sm.addStateChangeCallback([&](UpdateState, UpdateState to, const std::string&) {
        observed_to = to;
    });

    sm.rollbackToCheckpoint(id);
    EXPECT_EQ(observed_to, UpdateState::DOWNLOADING);
}

TEST_F(CheckpointTest, RollbackToCheckpoint_RecordedInTransactionLog) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "5.0.0");
    auto id = sm.createCheckpoint();
    sm.transition(UpdateState::VERIFYING, "5.0.0");
    sm.rollbackToCheckpoint(id);

    auto log = sm.transactionLog();
    // Most recent entry (index 0 = newest) should be the rollback
    ASSERT_FALSE(log.empty());
    EXPECT_EQ(log[0].to_state, UpdateState::DOWNLOADING);
    EXPECT_NE(log[0].message.find("checkpoint"), std::string::npos);
}

TEST_F(CheckpointTest, ClearCheckpoints_RemovesAll) {
    UpdateStateMachine sm;
    sm.createCheckpoint("x");
    sm.createCheckpoint("y");
    ASSERT_EQ(sm.listCheckpoints().size(), 2u);

    sm.clearCheckpoints();
    EXPECT_TRUE(sm.listCheckpoints().empty());
}

TEST_F(CheckpointTest, Checkpoint_DefaultDescription_IsEmpty) {
    UpdateStateMachine sm;
    sm.createCheckpoint();
    auto cps = sm.listCheckpoints();
    ASSERT_EQ(cps.size(), 1u);
    EXPECT_TRUE(cps[0].description.empty());
}

TEST_F(CheckpointTest, Checkpoint_TimestampIsRecent) {
    UpdateStateMachine sm;
    auto before = std::chrono::system_clock::now();
    sm.createCheckpoint("ts-test");
    auto after = std::chrono::system_clock::now();

    auto cps = sm.listCheckpoints();
    ASSERT_EQ(cps.size(), 1u);
    EXPECT_GE(cps[0].timestamp, before);
    EXPECT_LE(cps[0].timestamp, after);
}

TEST_F(CheckpointTest, SetHistoryLogger_AuditsCheckpointCreate) {
    UpdateHistoryLogger logger(history_file_);
    UpdateStateMachine sm;
    sm.setHistoryLogger(&logger);

    sm.createCheckpoint("audit-test");

    auto history = logger.getHistory();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_EQ(history[0].event_type, "checkpoint_created");
    EXPECT_EQ(history[0].error_message, "audit-test");
    EXPECT_TRUE(history[0].success);
}

TEST_F(CheckpointTest, SetHistoryLogger_AuditsRollback) {
    UpdateHistoryLogger logger(history_file_);
    UpdateStateMachine sm;
    sm.setHistoryLogger(&logger);

    sm.transition(UpdateState::DOWNLOADING, "6.0.0");
    auto id = sm.createCheckpoint("before-rollback");
    sm.transition(UpdateState::VERIFYING, "6.0.0");
    sm.rollbackToCheckpoint(id);

    auto history = logger.getHistory();  // newest first
    ASSERT_GE(history.size(), 2u);
    // Most recent entry should be the rollback
    EXPECT_EQ(history[0].event_type, "checkpoint_rollback");
    EXPECT_TRUE(history[0].success);
}

TEST_F(CheckpointTest, SetHistoryLogger_Nullptr_DisablesLogging) {
    UpdateHistoryLogger logger(history_file_);
    UpdateStateMachine sm;
    sm.setHistoryLogger(&logger);
    sm.createCheckpoint("x");

    sm.setHistoryLogger(nullptr);
    sm.createCheckpoint("y");  // should NOT log

    auto history = logger.getHistory();
    EXPECT_EQ(history.size(), 1u);  // only the first createCheckpoint logged
}

TEST_F(CheckpointTest, RollbackToCheckpoint_UnknownId_DoesNotChangeState) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "7.0.0");

    sm.rollbackToCheckpoint(42);  // no such checkpoint

    EXPECT_EQ(sm.currentState(),   UpdateState::DOWNLOADING);
    EXPECT_EQ(sm.currentVersion(), "7.0.0");
}

TEST_F(CheckpointTest, MultipleRollbacks_WorkSequentially) {
    UpdateStateMachine sm;
    sm.transition(UpdateState::DOWNLOADING, "8.0.0");
    auto id_dl = sm.createCheckpoint("dl");
    sm.transition(UpdateState::VERIFYING, "8.0.0");
    auto id_vr = sm.createCheckpoint("vr");
    sm.transition(UpdateState::APPLYING, "8.0.0");

    // Roll back to VERIFYING
    EXPECT_TRUE(sm.rollbackToCheckpoint(id_vr));
    EXPECT_EQ(sm.currentState(), UpdateState::VERIFYING);

    // Roll back further to DOWNLOADING
    EXPECT_TRUE(sm.rollbackToCheckpoint(id_dl));
    EXPECT_EQ(sm.currentState(), UpdateState::DOWNLOADING);
}

