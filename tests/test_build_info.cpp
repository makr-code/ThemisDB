/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_build_info.cpp                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:52:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     244                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/// @file test_build_info.cpp
/// @brief Unit tests for build info and build reproducibility API
///
/// Tests cover:
/// - getBuildConfiguration() completeness
/// - getVersionSummary() format
/// - isModuleCompiledIn() / getCompiledModules() / getDisabledModules()
/// - getReproducibilityInfo() fields
/// - exportBuildManifest() / verifyBuildManifest() round-trip

#include <gtest/gtest.h>
#include "themis/build_info.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace themis::build_info;

namespace {
/// Returns a unique, platform-appropriate temporary file path for each test.
std::string tmpPath(const std::string& suffix) {
    return (std::filesystem::temp_directory_path() / suffix).string();
}
} // namespace

// ===== BuildConfiguration Tests =====

TEST(BuildConfiguration, HasEditionInfo) {
    auto cfg = getBuildConfiguration();
    EXPECT_FALSE(cfg.edition_name.empty());
    EXPECT_FALSE(cfg.edition_type.empty());
    EXPECT_GE(cfg.gpu_max_vram_gb, 1);
}

TEST(BuildConfiguration, HasCompilerInfo) {
    auto cfg = getBuildConfiguration();
    EXPECT_FALSE(cfg.compiler.empty());
    EXPECT_FALSE(cfg.compiler_version.empty());
    EXPECT_FALSE(cfg.build_type.empty());
    EXPECT_FALSE(cfg.build_timestamp.empty());
}

TEST(BuildConfiguration, HasModules) {
    auto cfg = getBuildConfiguration();
    EXPECT_FALSE(cfg.modules.empty()) << "At least core modules must be present";

    // Verify each module entry has required fields
    for (const auto& mod : cfg.modules) {
        EXPECT_FALSE(mod.name.empty()) << "Module must have a name";
        EXPECT_FALSE(mod.description.empty()) << "Module must have a description";
    }
}

TEST(BuildConfiguration, HasCompileFlags) {
    auto cfg = getBuildConfiguration();
    EXPECT_FALSE(cfg.compile_flags.empty()) << "Compile flags must be present";

    // Verify each flag has a name
    for (const auto& [name, value] : cfg.compile_flags) {
        EXPECT_FALSE(name.empty()) << "Flag must have a name";
    }
}

// ===== Format Tests =====

TEST(BuildInfo, FormatBuildInfoContainsEdition) {
    auto cfg = getBuildConfiguration();
    const std::string formatted = formatBuildInfo(cfg);
    EXPECT_NE(formatted.find("EDITION"), std::string::npos);
    EXPECT_NE(formatted.find(cfg.edition_type), std::string::npos);
}

TEST(BuildInfo, FormatBuildInfoContainsCompiler) {
    auto cfg = getBuildConfiguration();
    const std::string formatted = formatBuildInfo(cfg);
    EXPECT_NE(formatted.find(cfg.compiler), std::string::npos);
}

TEST(BuildInfo, VersionSummaryNonEmpty) {
    const std::string summary = getVersionSummary();
    EXPECT_FALSE(summary.empty());
    EXPECT_NE(summary.find("ThemisDB"), std::string::npos);
}

TEST(BuildInfo, VersionSummaryContainsEdition) {
    const std::string summary = getVersionSummary();
    EXPECT_NE(summary.find("Edition"), std::string::npos);
}

// ===== Module Query Tests =====

TEST(ModuleQuery, CompiledModulesNonEmpty) {
    auto compiled = getCompiledModules();
    EXPECT_FALSE(compiled.empty()) << "At least core modules must be compiled in";
}

TEST(ModuleQuery, DisabledPlusCompiledEqualsTotal) {
    auto cfg      = getBuildConfiguration();
    auto compiled = getCompiledModules();
    auto disabled = getDisabledModules();

    EXPECT_EQ(compiled.size() + disabled.size(), cfg.modules.size());
}

TEST(ModuleQuery, IsModuleCompiledInKnownModule) {
    // "Storage Engine" is always compiled in according to build_info.cpp
    EXPECT_TRUE(isModuleCompiledIn("Storage Engine"));
}

TEST(ModuleQuery, IsModuleCompiledInUnknownModule) {
    EXPECT_FALSE(isModuleCompiledIn("__nonexistent_module_xyz__"));
}

// ===== Reproducibility Tests =====

TEST(ReproducibilityInfo, HasToolchain) {
    auto info = getReproducibilityInfo();
    EXPECT_FALSE(info.toolchain.empty());
    // toolchain should be "Compiler/Version"
    EXPECT_NE(info.toolchain.find('/'), std::string::npos);
}

TEST(ReproducibilityInfo, GitCommitFieldPresent) {
    auto info = getReproducibilityInfo();
    // git_commit may be "unknown" in environments without git,
    // but must not be an empty string.
    EXPECT_FALSE(info.git_commit.empty());
}

TEST(ReproducibilityInfo, BuildHostFieldPresent) {
    auto info = getReproducibilityInfo();
    EXPECT_FALSE(info.build_host.empty());
}

// ===== Manifest Export / Verify Tests =====

TEST(BuildManifest, ExportCreatesFile) {
    const std::string path = tmpPath("test_build_manifest.json");
    // Remove any leftover from a previous run
    std::error_code ec;
    std::filesystem::remove(path, ec);

    ASSERT_TRUE(exportBuildManifest(path));
    EXPECT_TRUE(std::filesystem::exists(path));

    // File must not be empty
    auto size = std::filesystem::file_size(path);
    EXPECT_GT(size, 0u);

    std::filesystem::remove(path, ec);
}

TEST(BuildManifest, ExportedManifestContainsGitCommit) {
    const std::string path = tmpPath("test_build_manifest_git.json");
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ASSERT_TRUE(exportBuildManifest(path));

    std::string content;
    {
        std::ifstream f(path);
        ASSERT_TRUE(f.is_open());
        content.assign(std::istreambuf_iterator<char>(f),
                       std::istreambuf_iterator<char>());
    }

    EXPECT_NE(content.find("git_commit"), std::string::npos);
    EXPECT_NE(content.find("toolchain"),  std::string::npos);

    std::filesystem::remove(path, ec);
}

TEST(BuildManifest, ExportedManifestVerifies) {
    const std::string path = tmpPath("test_build_manifest_verify.json");
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ASSERT_TRUE(exportBuildManifest(path));

    // A freshly exported manifest must verify against the current binary
    EXPECT_TRUE(verifyBuildManifest(path));

    std::filesystem::remove(path, ec);
}

TEST(BuildManifest, TamperedManifestFailsVerification) {
    const std::string path = tmpPath("test_build_manifest_tamper.json");
    std::error_code ec;
    std::filesystem::remove(path, ec);
    ASSERT_TRUE(exportBuildManifest(path));

    // Overwrite git_commit with a different value
    {
        std::ifstream fin(path);
        std::string content((std::istreambuf_iterator<char>(fin)),
                             std::istreambuf_iterator<char>());
        fin.close();
        // Replace the real commit hash with a fake one
        auto pos = content.find("\"git_commit\": \"");
        if (pos != std::string::npos) {
            auto start = pos + 15; // skip `"git_commit": "`
            auto end   = content.find('"', start);
            content.replace(start, end - start, "0000000000000000000000000000000000000000");
            std::ofstream fout(path);
            fout << content;
        }
    }

    EXPECT_FALSE(verifyBuildManifest(path));

    std::filesystem::remove(path, ec);
}

TEST(BuildManifest, NonExistentManifestReturnsFalse) {
    EXPECT_FALSE(verifyBuildManifest(
        tmpPath("__no_such_manifest_xyz.json")));
}

TEST(BuildManifest, ExportToInvalidPathReturnsFalse) {
    EXPECT_FALSE(exportBuildManifest(
        "/no/such/directory/manifest.json"));
}
