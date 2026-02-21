/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_module_sandbox.cpp                            ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     259                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/// @file test_module_sandbox.cpp
/// @brief Unit tests for ModuleSandbox and AbiChecker (Phase 4)

#include <gtest/gtest.h>
#include "themis/base/module_sandbox.h"
#include "themis/base/module_loader.h"

using namespace themis::modules;

// =============================================================================
// AbiChecker – version checks
// =============================================================================

TEST(AbiChecker, DefaultConstruction) {
    EXPECT_NO_THROW({ AbiChecker checker; });
}

TEST(AbiChecker, UseDefaultListsDoesNotThrow) {
    AbiChecker checker;
    EXPECT_NO_THROW(checker.useDefaultLists());
}

TEST(AbiChecker, CompatibleVersions) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = "1.6.0";
    meta.themisMajor  = 1;
    meta.themisMinor  = 5; // module is older minor → compatible
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1 /*host_major*/, 6 /*host_minor*/);
    EXPECT_TRUE(result.compatible);
    EXPECT_TRUE(result.issues.empty()) << result.issues[0];
}

TEST(AbiChecker, IncompatibleMajor) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = "2.0.0";
    meta.themisMajor  = 2;
    meta.themisMinor  = 0;
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1, 6);
    EXPECT_FALSE(result.compatible);
    EXPECT_FALSE(result.issues.empty());
}

TEST(AbiChecker, IncompatibleMinorTooNew) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = "1.9.0";
    meta.themisMajor  = 1;
    meta.themisMinor  = 9; // module requires 1.9, but host is 1.6
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1, 6);
    EXPECT_FALSE(result.compatible);
}

TEST(AbiChecker, ExactVersionMatch) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = "1.6.0";
    meta.themisMajor  = 1;
    meta.themisMinor  = 6;
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1, 6);
    EXPECT_TRUE(result.compatible);
}

TEST(AbiChecker, EmptyVersionIsWarningNotFailure) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.version      = ""; // missing version string
    meta.themisMajor  = 1;
    meta.themisMinor  = 0;
    meta.themisPatch  = 0;

    auto result = checker.checkVersions(meta, 1, 6);
    EXPECT_TRUE(result.compatible); // warning only, not fail
    EXPECT_FALSE(result.issues.empty()) << "Should have at least a warning";
}

TEST(AbiChecker, RequiredSymbolsMissingOnNullHandle) {
    AbiChecker checker;
    checker.addRequiredSymbol("themis_module_init");

    // null handle → cannot resolve any symbol
    auto result = checker.checkRequiredSymbols(nullptr);
    EXPECT_FALSE(result.compatible);
    EXPECT_FALSE(result.issues.empty());
}

TEST(AbiChecker, NoRequiredSymbolsAlwaysPasses) {
    AbiChecker checker;
    // No required symbols added
    auto result = checker.checkRequiredSymbols(nullptr);
    EXPECT_TRUE(result.compatible);
    EXPECT_TRUE(result.issues.empty());
}

TEST(AbiChecker, DeprecatedSymbolsOnNullHandleNoIssues) {
    AbiChecker checker;
    checker.addDeprecatedSymbol("old_symbol");
    // null handle → dlsym/GetProcAddress returns nullptr → no deprecated symbol found
    auto result = checker.checkDeprecatedSymbols(nullptr);
    EXPECT_TRUE(result.compatible); // deprecated is non-fatal
    EXPECT_TRUE(result.issues.empty());
}

TEST(AbiChecker, FullCheckWithNullHandleVersionIncompatible) {
    AbiChecker checker;
    checker.useDefaultLists();

    ModuleMetadata meta;
    meta.version      = "2.0.0";
    meta.themisMajor  = 2;
    meta.themisMinor  = 0;
    meta.themisPatch  = 0;

    auto result = checker.check(nullptr, meta, 1, 6);
    EXPECT_FALSE(result.compatible);
    EXPECT_FALSE(result.summary.empty());
}

TEST(AbiChecker, FullCheckCompatibleVersionsNullHandle) {
    AbiChecker checker;
    // No required symbols → will still check version only
    ModuleMetadata meta;
    meta.version      = "1.5.0";
    meta.themisMajor  = 1;
    meta.themisMinor  = 5;
    meta.themisPatch  = 0;

    auto result = checker.check(nullptr, meta, 1, 6);
    // Version OK, no required symbols → should be compatible
    EXPECT_TRUE(result.compatible);
}

TEST(AbiChecker, SummaryAlwaysNonEmpty) {
    AbiChecker checker;
    ModuleMetadata meta;
    meta.themisMajor = 1; meta.themisMinor = 0;
    auto result = checker.check(nullptr, meta, 1, 0);
    EXPECT_FALSE(result.summary.empty());
}

// =============================================================================
// ModuleSandbox – configuration & construction
// =============================================================================

TEST(ModuleSandbox, DefaultConfig) {
    ModuleSandbox::Config cfg;
    EXPECT_EQ(cfg.max_memory_mb,   256u);
    EXPECT_EQ(cfg.max_cpu_percent, 50);
    EXPECT_FALSE(cfg.allow_network);
    EXPECT_EQ(cfg.fs_access, ModuleSandbox::FilesystemAccess::READ_ONLY);
}

TEST(ModuleSandbox, ConstructWithDefaultConfig) {
    EXPECT_NO_THROW({ ModuleSandbox sb; });
}

TEST(ModuleSandbox, InitiallyNotActive) {
    ModuleSandbox sb;
    EXPECT_FALSE(sb.isActive());
}

TEST(ModuleSandbox, LaunchSucceeds) {
    ModuleSandbox sb;
    bool ok = sb.launch("test_module");
    EXPECT_TRUE(ok) << "launch() must succeed; error: " << sb.lastError();
    EXPECT_TRUE(sb.isActive());
}

TEST(ModuleSandbox, ShutdownMakesInactive) {
    ModuleSandbox sb;
    sb.launch("test_module");
    ASSERT_TRUE(sb.isActive());
    sb.shutdown();
    EXPECT_FALSE(sb.isActive());
}

TEST(ModuleSandbox, StatsAvailableAfterLaunch) {
    ModuleSandbox sb;
    sb.launch("test_module");
    // stats() must not crash
    auto stats = sb.stats();
    (void)stats;
    EXPECT_FALSE(stats.killed);
}

TEST(ModuleSandbox, LaunchWarningsAreStrings) {
    ModuleSandbox sb;
    sb.launch("test_module");
    // Each warning (if any) must be a non-empty string
    for (const auto& w : sb.launchWarnings()) {
        EXPECT_FALSE(w.empty());
    }
}

TEST(ModuleSandbox, ZeroMemoryLimitSkipped) {
    ModuleSandbox::Config cfg;
    cfg.max_memory_mb = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    EXPECT_TRUE(sb.launch("no_limits_module"));
    EXPECT_TRUE(sb.isActive());
}

TEST(ModuleSandbox, DestructorShutdown) {
    // Verify destructor implicitly shuts down an active sandbox
    {
        ModuleSandbox sb;
        sb.launch("temporary_module");
        EXPECT_TRUE(sb.isActive());
    } // ~ModuleSandbox should call shutdown() without crash
    SUCCEED();
}

TEST(ModuleSandbox, NetworkIsolationWarning) {
    ModuleSandbox::Config cfg;
    cfg.allow_network   = false;
    cfg.max_memory_mb   = 0; // no limits
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("network_restricted_module");
    // On most platforms, network isolation produces a warning (not a failure)
    // No assertion needed; just ensure it doesn't crash.
    SUCCEED();
}
