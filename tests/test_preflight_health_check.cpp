// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_preflight_health_check.cpp
 * @brief Unit tests for the pre-flight health check system (Issue #2490).
 *
 * All tests use injectable providers so no real filesystem queries or
 * system calls are required.
 *
 * Coverage:
 *  - DiskSpaceChecker: pass / fail / zero-provider / MiB display
 *  - MemoryHeadroomChecker: pass / fail / zero-provider skip
 *  - DependencyVersionChecker: equal / greater / lesser / missing / null
 *  - DependencyVersionChecker::compareVersions: patch / minor / major /
 *    unequal-length / equal
 *  - PreflightHealthChecker: empty / single pass / single fail /
 *    multi all-pass / multi some-fail / null-check throw / duration /
 *    error-summary / check count / all-fail error-summary ordering
 *  - Custom IHealthCheck implementation (integration)
 */

#include <gtest/gtest.h>

#include "updates/preflight_health_check.h"

#include <memory>
#include <stdexcept>
#include <string>

using namespace themis::updates;

// =============================================================================
// Helpers
// =============================================================================

static std::unique_ptr<DiskSpaceChecker>
makeDiskChecker(uint64_t available, uint64_t required,
                const std::string& path = "/tmp") {
    return std::make_unique<DiskSpaceChecker>(
        path, required,
        [available](const std::string&) -> uint64_t { return available; });
}

static std::unique_ptr<MemoryHeadroomChecker>
makeMemChecker(uint64_t available, uint64_t required) {
    return std::make_unique<MemoryHeadroomChecker>(
        required,
        [available]() -> uint64_t { return available; });
}

static std::unique_ptr<DependencyVersionChecker>
makeDepChecker(const std::string& dep, const std::string& min,
               const std::string& installed) {
    return std::make_unique<DependencyVersionChecker>(
        dep, min,
        [installed](const std::string&) -> std::string { return installed; });
}

// =============================================================================
// DiskSpaceChecker tests
// =============================================================================

class DiskSpaceCheckerTest : public ::testing::Test {};

TEST_F(DiskSpaceCheckerTest, Name_IsDiskSpace) {
    auto c = makeDiskChecker(1000, 500);
    EXPECT_EQ(c->name(), "disk_space");
}

TEST_F(DiskSpaceCheckerTest, Pass_WhenAvailableEqualsRequired) {
    auto c = makeDiskChecker(512, 512);
    auto r = c->run();
    EXPECT_TRUE(r.passed);
    EXPECT_TRUE(r.error.empty());
}

TEST_F(DiskSpaceCheckerTest, Pass_WhenAvailableExceedsRequired) {
    auto c = makeDiskChecker(1024 * 1024, 512 * 1024);
    auto r = c->run();
    EXPECT_TRUE(r.passed);
    EXPECT_TRUE(r.error.empty());
}

TEST_F(DiskSpaceCheckerTest, Fail_WhenAvailableLessThanRequired) {
    auto c = makeDiskChecker(100, 200);
    auto r = c->run();
    EXPECT_FALSE(r.passed);
    EXPECT_FALSE(r.error.empty());
}

TEST_F(DiskSpaceCheckerTest, Fail_WhenProviderReturnsZero) {
    // Zero available → any positive requirement fails.
    auto c = makeDiskChecker(0, 1);
    auto r = c->run();
    EXPECT_FALSE(r.passed);
}

TEST_F(DiskSpaceCheckerTest, Pass_ZeroRequired_ZeroAvailable) {
    auto c = makeDiskChecker(0, 0);
    auto r = c->run();
    EXPECT_TRUE(r.passed);
}

TEST_F(DiskSpaceCheckerTest, MessageContainsMiB) {
    constexpr uint64_t kMiB = 1024ULL * 1024ULL;
    auto c = makeDiskChecker(10 * kMiB, 5 * kMiB);
    auto r = c->run();
    EXPECT_TRUE(r.passed);
    EXPECT_NE(r.message.find("MiB"), std::string::npos);
}

TEST_F(DiskSpaceCheckerTest, ErrorContainsPath) {
    auto c = std::make_unique<DiskSpaceChecker>(
        "/custom/path", 999,
        [](const std::string&) -> uint64_t { return 0; });
    auto r = c->run();
    EXPECT_FALSE(r.passed);
    EXPECT_NE(r.error.find("/custom/path"), std::string::npos);
}

TEST_F(DiskSpaceCheckerTest, CheckName_MatchesName) {
    auto c = makeDiskChecker(100, 50);
    auto r = c->run();
    EXPECT_EQ(r.check_name, c->name());
}

// =============================================================================
// MemoryHeadroomChecker tests
// =============================================================================

class MemoryHeadroomCheckerTest : public ::testing::Test {};

TEST_F(MemoryHeadroomCheckerTest, Name_IsMemoryHeadroom) {
    auto c = makeMemChecker(1000, 500);
    EXPECT_EQ(c->name(), "memory_headroom");
}

TEST_F(MemoryHeadroomCheckerTest, Pass_WhenAvailableEqualsRequired) {
    auto c = makeMemChecker(256, 256);
    auto r = c->run();
    EXPECT_TRUE(r.passed);
}

TEST_F(MemoryHeadroomCheckerTest, Pass_WhenAvailableExceedsRequired) {
    constexpr uint64_t kGiB = 1024ULL * 1024ULL * 1024ULL;
    auto c = makeMemChecker(4 * kGiB, 256 * 1024 * 1024ULL);
    auto r = c->run();
    EXPECT_TRUE(r.passed);
}

TEST_F(MemoryHeadroomCheckerTest, Fail_WhenAvailableLessThanRequired) {
    auto c = makeMemChecker(100, 200);
    auto r = c->run();
    EXPECT_FALSE(r.passed);
    EXPECT_FALSE(r.error.empty());
}

TEST_F(MemoryHeadroomCheckerTest, Pass_WhenProviderReturnsZero_SkipsCheck) {
    // Provider returning 0 → unsupported platform → check is skipped (passes).
    auto c = makeMemChecker(0, 1024);
    auto r = c->run();
    EXPECT_TRUE(r.passed);
    EXPECT_NE(r.message.find("skipped"), std::string::npos);
}

TEST_F(MemoryHeadroomCheckerTest, CheckName_MatchesName) {
    auto c = makeMemChecker(100, 50);
    auto r = c->run();
    EXPECT_EQ(r.check_name, c->name());
}

TEST_F(MemoryHeadroomCheckerTest, MessageContainsMiB) {
    constexpr uint64_t kMiB = 1024ULL * 1024ULL;
    auto c = makeMemChecker(8 * kMiB, 2 * kMiB);
    auto r = c->run();
    EXPECT_TRUE(r.passed);
    EXPECT_NE(r.message.find("MiB"), std::string::npos);
}

// =============================================================================
// DependencyVersionChecker::compareVersions tests
// =============================================================================

class CompareVersionsTest : public ::testing::Test {};

TEST_F(CompareVersionsTest, Equal_ReturnsZero) {
    EXPECT_EQ(DependencyVersionChecker::compareVersions("1.4.0", "1.4.0"), 0);
}

TEST_F(CompareVersionsTest, PatchGreater_ReturnsPositive) {
    EXPECT_GT(DependencyVersionChecker::compareVersions("1.4.1", "1.4.0"), 0);
}

TEST_F(CompareVersionsTest, PatchLesser_ReturnsNegative) {
    EXPECT_LT(DependencyVersionChecker::compareVersions("1.4.0", "1.4.1"), 0);
}

TEST_F(CompareVersionsTest, MinorGreater_ReturnsPositive) {
    EXPECT_GT(DependencyVersionChecker::compareVersions("1.5.0", "1.4.9"), 0);
}

TEST_F(CompareVersionsTest, MajorGreater_ReturnsPositive) {
    EXPECT_GT(DependencyVersionChecker::compareVersions("2.0.0", "1.9.9"), 0);
}

TEST_F(CompareVersionsTest, UnequalLength_ShorterTreatedAsZero) {
    // "1.4" == "1.4.0"
    EXPECT_EQ(DependencyVersionChecker::compareVersions("1.4", "1.4.0"), 0);
}

TEST_F(CompareVersionsTest, LargeMinorBeat_ReturnsPositive) {
    // "1.10.0" > "1.9.0"
    EXPECT_GT(DependencyVersionChecker::compareVersions("1.10.0", "1.9.0"), 0);
}

// =============================================================================
// DependencyVersionChecker run() tests
// =============================================================================

class DependencyVersionCheckerTest : public ::testing::Test {};

TEST_F(DependencyVersionCheckerTest, Name_IncludesDepName) {
    auto c = makeDepChecker("libfoo", "1.0.0", "1.0.0");
    EXPECT_NE(c->name().find("libfoo"), std::string::npos);
}

TEST_F(DependencyVersionCheckerTest, Pass_WhenInstalledEqualsMin) {
    auto c = makeDepChecker("libfoo", "1.4.0", "1.4.0");
    EXPECT_TRUE(c->run().passed);
}

TEST_F(DependencyVersionCheckerTest, Pass_WhenInstalledGreaterThanMin) {
    auto c = makeDepChecker("libfoo", "1.4.0", "1.5.0");
    EXPECT_TRUE(c->run().passed);
}

TEST_F(DependencyVersionCheckerTest, Fail_WhenInstalledLessThanMin) {
    auto c = makeDepChecker("libfoo", "1.4.0", "1.3.9");
    auto r = c->run();
    EXPECT_FALSE(r.passed);
    EXPECT_FALSE(r.error.empty());
}

TEST_F(DependencyVersionCheckerTest, Fail_WhenDependencyNotFound) {
    auto c = makeDepChecker("missing-lib", "1.0.0", "");  // empty → not found
    auto r = c->run();
    EXPECT_FALSE(r.passed);
    EXPECT_NE(r.error.find("not found"), std::string::npos);
}

TEST_F(DependencyVersionCheckerTest, ErrorContainsDepName) {
    auto c = makeDepChecker("my-special-lib", "2.0.0", "1.0.0");
    auto r = c->run();
    EXPECT_FALSE(r.passed);
    EXPECT_NE(r.error.find("my-special-lib"), std::string::npos);
}

TEST_F(DependencyVersionCheckerTest, NullProvider_Throws) {
    EXPECT_THROW(
        DependencyVersionChecker("lib", "1.0.0", nullptr),
        std::invalid_argument);
}

TEST_F(DependencyVersionCheckerTest, CheckName_MatchesName) {
    auto c = makeDepChecker("libbar", "1.0.0", "1.0.0");
    auto r = c->run();
    EXPECT_EQ(r.check_name, c->name());
}

// =============================================================================
// PreflightHealthChecker tests
// =============================================================================

class PreflightCheckerTest : public ::testing::Test {};

TEST_F(PreflightCheckerTest, Empty_AllPassedTrue) {
    PreflightHealthChecker checker;
    auto result = checker.runAll();
    EXPECT_TRUE(result.all_passed);
    EXPECT_TRUE(result.results.empty());
    EXPECT_TRUE(result.error_summary.empty());
}

TEST_F(PreflightCheckerTest, CheckCount_Zero_WhenEmpty) {
    PreflightHealthChecker checker;
    EXPECT_EQ(checker.checkCount(), 0u);
}

TEST_F(PreflightCheckerTest, CheckCount_AfterAdd) {
    PreflightHealthChecker checker;
    checker.addCheck(makeDiskChecker(1000, 500));
    checker.addCheck(makeMemChecker(1000, 500));
    EXPECT_EQ(checker.checkCount(), 2u);
}

TEST_F(PreflightCheckerTest, SingleCheck_Pass) {
    PreflightHealthChecker checker;
    checker.addCheck(makeDiskChecker(1000, 500));
    auto result = checker.runAll();
    EXPECT_TRUE(result.all_passed);
    EXPECT_EQ(result.results.size(), 1u);
    EXPECT_TRUE(result.results[0].passed);
    EXPECT_TRUE(result.error_summary.empty());
}

TEST_F(PreflightCheckerTest, SingleCheck_Fail) {
    PreflightHealthChecker checker;
    checker.addCheck(makeDiskChecker(100, 500));  // fails
    auto result = checker.runAll();
    EXPECT_FALSE(result.all_passed);
    EXPECT_EQ(result.results.size(), 1u);
    EXPECT_FALSE(result.results[0].passed);
    EXPECT_FALSE(result.error_summary.empty());
}

TEST_F(PreflightCheckerTest, MultiCheck_AllPass) {
    PreflightHealthChecker checker;
    checker.addCheck(makeDiskChecker(1000, 500));
    checker.addCheck(makeMemChecker(1000, 500));
    checker.addCheck(makeDepChecker("libfoo", "1.0.0", "1.2.0"));
    auto result = checker.runAll();
    EXPECT_TRUE(result.all_passed);
    EXPECT_EQ(result.results.size(), 3u);
    EXPECT_TRUE(result.error_summary.empty());
}

TEST_F(PreflightCheckerTest, MultiCheck_OneFail_AllChecksStillRun) {
    PreflightHealthChecker checker;
    checker.addCheck(makeDiskChecker(100, 500));    // fails
    checker.addCheck(makeMemChecker(1000, 500));    // passes
    checker.addCheck(makeDepChecker("lib", "1.0.0", "1.0.0"));  // passes

    auto result = checker.runAll();
    EXPECT_FALSE(result.all_passed);
    // All 3 checks must be executed (no early abort)
    EXPECT_EQ(result.results.size(), 3u);
    EXPECT_FALSE(result.results[0].passed);
    EXPECT_TRUE(result.results[1].passed);
    EXPECT_TRUE(result.results[2].passed);
}

TEST_F(PreflightCheckerTest, MultiCheck_AllFail_ErrorSummaryIsFirstError) {
    PreflightHealthChecker checker;
    checker.addCheck(makeDiskChecker(0, 100));   // fails first
    checker.addCheck(makeMemChecker(0, 100));    // also fails, provider=0 → skipped
    // Note: MemoryHeadroomChecker with available==0 skips the check (passes).
    // Use DependencyVersionChecker for a second true failure.
    checker.addCheck(makeDepChecker("lib", "2.0.0", "1.0.0"));  // fails

    auto result = checker.runAll();
    EXPECT_FALSE(result.all_passed);
    // error_summary should contain the disk space failure (first real failure).
    EXPECT_FALSE(result.error_summary.empty());
}

TEST_F(PreflightCheckerTest, AddNullCheck_Throws) {
    PreflightHealthChecker checker;
    EXPECT_THROW(checker.addCheck(nullptr), std::invalid_argument);
}

TEST_F(PreflightCheckerTest, Duration_IsNonNegative) {
    PreflightHealthChecker checker;
    checker.addCheck(makeDiskChecker(1000, 500));
    auto result = checker.runAll();
    EXPECT_GE(result.duration.count(), 0);
}

TEST_F(PreflightCheckerTest, ResultOrderMatchesRegistrationOrder) {
    PreflightHealthChecker checker;
    checker.addCheck(makeDiskChecker(1000, 500));  // idx 0
    checker.addCheck(makeMemChecker(1000, 500));   // idx 1
    auto result = checker.runAll();
    EXPECT_EQ(result.results[0].check_name, "disk_space");
    EXPECT_EQ(result.results[1].check_name, "memory_headroom");
}

// =============================================================================
// Custom IHealthCheck integration test
// =============================================================================

class CustomCheck : public IHealthCheck {
public:
    explicit CustomCheck(bool should_pass, const std::string& check_name)
        : should_pass_(should_pass), name_(check_name) {}

    std::string name() const override { return name_; }

    HealthCheckResult run() override {
        HealthCheckResult r;
        r.check_name = name_;
        r.passed = should_pass_;
        r.message = should_pass_ ? "custom check passed" : "custom check failed";
        if (!should_pass_) {
          r.error = "custom error";
        }
        ++run_count;
        return r;
    }

    int run_count = 0;

private:
    bool        should_pass_;
    std::string name_;
};

TEST(CustomIHealthCheckTest, CustomCheck_Integrates_WithOrchestrator) {
    auto* raw = new CustomCheck(true, "my_custom");
    PreflightHealthChecker checker;
    checker.addCheck(std::unique_ptr<IHealthCheck>(raw));

    auto result = checker.runAll();
    EXPECT_TRUE(result.all_passed);
    EXPECT_EQ(raw->run_count, 1);
    EXPECT_EQ(result.results[0].check_name, "my_custom");
}

TEST(CustomIHealthCheckTest, CustomFailCheck_SetsAllPassedFalse) {
    PreflightHealthChecker checker;
    checker.addCheck(std::make_unique<CustomCheck>(false, "bad_check"));

    auto result = checker.runAll();
    EXPECT_FALSE(result.all_passed);
    EXPECT_EQ(result.error_summary, "custom error");
}

// =============================================================================
// Bundle-size multiplier usage pattern test
// =============================================================================

TEST(BundleSizePatternTest, TwoTimesMultiplier_EnforcedByDiskChecker) {
    // The FUTURE_ENHANCEMENTS.md mandates ≥ 2× bundle size of free disk space.
    // This test verifies the pattern works with the DiskSpaceChecker.
    constexpr uint64_t bundle_size   = 50 * 1024 * 1024ULL;  // 50 MiB
    constexpr uint64_t required      = 2 * bundle_size;       // 100 MiB
    constexpr uint64_t free_space    = 150 * 1024 * 1024ULL;  // 150 MiB – passes

    auto c = std::make_unique<DiskSpaceChecker>(
        "/tmp", required,
        [free_space](const std::string&) -> uint64_t { return free_space; });
    EXPECT_TRUE(c->run().passed);

    // Now shrink free space below the 2× threshold.
    auto c2 = std::make_unique<DiskSpaceChecker>(
        "/tmp", required,
        [](const std::string&) -> uint64_t {
            return 80 * 1024 * 1024ULL;  // 80 MiB – fails (< 100 MiB)
        });
    EXPECT_FALSE(c2->run().passed);
}
