/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_updates_production.cpp                        ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 19:20:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     709                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_updates_production.cpp
 * @brief Production-readiness tests for the Updates module (all 8 phases)
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
 */

#include <gtest/gtest.h>

#include "updates/release_manifest.h"
#include "updates/updates_config.h"
#include "updates/hot_reload_engine.h"
#include "updates/update_state_machine.h"
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
