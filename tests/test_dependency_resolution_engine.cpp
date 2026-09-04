/**
 * @file test_dependency_resolution_engine.cpp
 * @brief Focused unit tests for DependencyResolver (Issue #216 / v1.6.0)
 *
 * Covers all six acceptance criteria:
 *   AC1 – Dependency graph construction
 *   AC2 – Topological sort for correct update order
 *   AC3 – Cycle detection
 *   AC4 – Minimum version constraints (>=, <=, >, <, ==, !=; compound AND)
 *   AC5 – Conflict resolution
 *   AC6 – Automatic backfill of missing dependencies
 */

#include <gtest/gtest.h>
#include "updates/dependency_resolver.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace themis::updates;

// ============================================================================
// Helpers
// ============================================================================

namespace {

using VersionMap = std::unordered_map<std::string, std::string>;

/// Convenience: find a step for @p pkg in @p steps.
static const UpdateStep* findStep(const std::vector<UpdateStep>& steps,
                                   const std::string& pkg) {
    for (const auto& s : steps) {
        if (s.package == pkg) {
          return &s;
        }
    }
    return nullptr;
}

/// Return position of @p pkg in the steps vector (-1 if not found).
static int stepPos(const std::vector<UpdateStep>& steps, const std::string& pkg) {
    for (int i = 0; i < static_cast<int>(steps.size()); ++i) {
        if (steps[i].package == pkg) {
          return i;
        }
    }
    return -1;
}

} // anonymous namespace

// ============================================================================
// AC4 – satisfiesConstraint (unit tests for the static helper)
// ============================================================================

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_GTE_Pass) {
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("1.4.0", ">=1.4.0"));
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("1.5.0", ">=1.4.0"));
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("2.0.0", ">=1.4.0"));
}

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_GTE_Fail) {
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("1.3.9", ">=1.4.0"));
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("1.3.0", ">=1.4.0"));
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("0.9.0", ">=1.4.0"));
}

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_LT_Pass) {
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("1.9.9", "<2.0.0"));
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("0.0.1", "<2.0.0"));
}

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_LT_Fail) {
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("2.0.0", "<2.0.0"));
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("2.1.0", "<2.0.0"));
}

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_EQ) {
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("1.4.0", "==1.4.0"));
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("1.4.1", "==1.4.0"));
}

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_NEQ) {
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("1.4.1", "!=1.4.0"));
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("1.4.0", "!=1.4.0"));
}

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_GT_Strict) {
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("1.4.1", ">1.4.0"));
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("1.4.0", ">1.4.0"));
}

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_Compound) {
    // ">=1.4.0,<2.0.0"
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("1.4.0", ">=1.4.0,<2.0.0"));
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("1.9.9", ">=1.4.0,<2.0.0"));
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("1.3.9", ">=1.4.0,<2.0.0"));
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("2.0.0", ">=1.4.0,<2.0.0"));
}

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_EmptyConstraintAlwaysTrue) {
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("1.0.0", ""));
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("", ""));
}

// ============================================================================
// AC4 – minVersionFromConstraint (unit tests for the static helper)
// ============================================================================

TEST(DependencyResolutionEngineTest, MinVersion_GTE) {
    EXPECT_EQ("1.4.0", DependencyResolver::minVersionFromConstraint(">=1.4.0"));
}

TEST(DependencyResolutionEngineTest, MinVersion_EQ) {
    EXPECT_EQ("1.4.0", DependencyResolver::minVersionFromConstraint("==1.4.0"));
}

TEST(DependencyResolutionEngineTest, MinVersion_GT_BumpsPatch) {
    EXPECT_EQ("1.4.1", DependencyResolver::minVersionFromConstraint(">1.4.0"));
}

TEST(DependencyResolutionEngineTest, MinVersion_OnlyUpperBound) {
    EXPECT_EQ("", DependencyResolver::minVersionFromConstraint("<2.0.0"));
    EXPECT_EQ("", DependencyResolver::minVersionFromConstraint("<=2.0.0"));
}

TEST(DependencyResolutionEngineTest, MinVersion_Compound_PicksLower) {
    EXPECT_EQ("1.4.0",
              DependencyResolver::minVersionFromConstraint(">=1.4.0,<2.0.0"));
}

TEST(DependencyResolutionEngineTest, MinVersion_Empty) {
    EXPECT_EQ("", DependencyResolver::minVersionFromConstraint(""));
}

// ============================================================================
// AC1 – Dependency graph construction + AC2 – Topological sort
// ============================================================================

TEST(DependencyResolutionEngineTest, SimpleResolution_AlreadySatisfied) {
    // All installed versions already satisfy constraints → no updates needed.
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"themis-storage", ">=1.4.0"});
    resolver.addDependency("1.5.0", {"themis-query",   ">=1.4.5"});

    VersionMap current = {{"themis-storage", "1.5.0"}, {"themis-query", "1.5.0"}};
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.steps.empty());
    EXPECT_TRUE(result.backfilled.empty());
    EXPECT_TRUE(result.error_message.empty());
}

TEST(DependencyResolutionEngineTest, SimpleResolution_OnePackageNeedsUpdate) {
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"themis-storage", ">=1.4.0"});

    VersionMap current = {{"themis-storage", "1.3.0"}};
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(1u, result.steps.size());
    EXPECT_EQ("themis-storage", result.steps[0].package);
    EXPECT_EQ("1.3.0",          result.steps[0].from_version);
    EXPECT_EQ("1.4.0",          result.steps[0].to_version);
}

TEST(DependencyResolutionEngineTest, SimpleResolution_TwoPackagesNeedUpdate) {
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"themis-storage", ">=1.4.0"});
    resolver.addDependency("1.5.0", {"themis-query",   ">=1.4.5"});

    VersionMap current = {{"themis-storage", "1.3.0"}, {"themis-query", "1.3.0"}};
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(2u, result.steps.size());

    const auto* s = findStep(result.steps, "themis-storage");
    ASSERT_NE(nullptr, s);
    EXPECT_EQ("1.3.0", s->from_version);
    EXPECT_EQ("1.4.0", s->to_version);
}

// AC2 – Correct topological order
TEST(DependencyResolutionEngineTest, TopologicalOrder_DepBeforeDependent) {
    // themis-query@1.4.5 depends on themis-storage@>=1.4.0.
    // Both need updating; themis-storage must come first.
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"themis-storage", ">=1.4.0"});
    resolver.addDependency("1.5.0", {"themis-query",   ">=1.4.5"});
    resolver.addPackageDependency("themis-query", "1.4.5",
                                  {"themis-storage", ">=1.4.0"});

    VersionMap current = {{"themis-storage", "1.3.0"}, {"themis-query", "1.3.0"}};
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(2u, result.steps.size());

    const int posStorage = stepPos(result.steps, "themis-storage");
    const int posQuery   = stepPos(result.steps, "themis-query");
    ASSERT_NE(-1, posStorage);
    ASSERT_NE(-1, posQuery);
    EXPECT_LT(posStorage, posQuery)
        << "themis-storage must be updated before themis-query";
}

TEST(DependencyResolutionEngineTest, TopologicalOrder_ThreeLevel) {
    // alpha → beta → gamma (alpha must come first, gamma last)
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"alpha", ">=1.0.0"});
    resolver.addDependency("1.0.0", {"beta",  ">=1.0.0"});
    resolver.addDependency("1.0.0", {"gamma", ">=1.0.0"});
    resolver.addPackageDependency("beta",  "1.0.0", {"alpha", ">=1.0.0"});
    resolver.addPackageDependency("gamma", "1.0.0", {"beta",  ">=1.0.0"});

    VersionMap current = {{"alpha", "0.9.0"}, {"beta", "0.9.0"}, {"gamma", "0.9.0"}};
    auto result = resolver.resolve("1.0.0", current);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(3u, result.steps.size());

    const int posAlpha = stepPos(result.steps, "alpha");
    const int posBeta  = stepPos(result.steps, "beta");
    const int posGamma = stepPos(result.steps, "gamma");
    EXPECT_LT(posAlpha, posBeta);
    EXPECT_LT(posBeta,  posGamma);
}

TEST(DependencyResolutionEngineTest, TopologicalOrder_DeterministicAlphabetical) {
    // No inter-package dependencies → result must be in alphabetical order.
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"zebra",  ">=1.0.0"});
    resolver.addDependency("1.0.0", {"mango",  ">=1.0.0"});
    resolver.addDependency("1.0.0", {"apple",  ">=1.0.0"});

    VersionMap current = {{"zebra", "0.9.0"}, {"mango", "0.9.0"}, {"apple", "0.9.0"}};
    auto result = resolver.resolve("1.0.0", current);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(3u, result.steps.size());
    EXPECT_EQ("apple", result.steps[0].package);
    EXPECT_EQ("mango", result.steps[1].package);
    EXPECT_EQ("zebra", result.steps[2].package);
}

// ============================================================================
// AC3 – Cycle detection
// ============================================================================

TEST(DependencyResolutionEngineTest, CycleDetection_TwoPackages) {
    // A depends on B, B depends on A.
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"pkg-a", ">=1.0.0"});
    resolver.addDependency("1.0.0", {"pkg-b", ">=1.0.0"});
    resolver.addPackageDependency("pkg-a", "1.0.0", {"pkg-b", ">=1.0.0"});
    resolver.addPackageDependency("pkg-b", "1.0.0", {"pkg-a", ">=1.0.0"});

    VersionMap current = {{"pkg-a", "0.9.0"}, {"pkg-b", "0.9.0"}};
    auto result = resolver.resolve("1.0.0", current);

    EXPECT_FALSE(result.success);
    EXPECT_NE(std::string::npos,
              result.error_message.find("Circular dependency"))
        << "Error message should mention circular dependency";
}

TEST(DependencyResolutionEngineTest, CycleDetection_ThreePackages) {
    // A → B → C → A
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"a", ">=1.0.0"});
    resolver.addDependency("1.0.0", {"b", ">=1.0.0"});
    resolver.addDependency("1.0.0", {"c", ">=1.0.0"});
    resolver.addPackageDependency("a", "1.0.0", {"b", ">=1.0.0"});
    resolver.addPackageDependency("b", "1.0.0", {"c", ">=1.0.0"});
    resolver.addPackageDependency("c", "1.0.0", {"a", ">=1.0.0"});

    VersionMap current = {{"a", "0.9.0"}, {"b", "0.9.0"}, {"c", "0.9.0"}};
    auto result = resolver.resolve("1.0.0", current);

    EXPECT_FALSE(result.success);
    EXPECT_NE(std::string::npos, result.error_message.find("Circular dependency"));
}

// ============================================================================
// AC4 – Minimum version constraints
// ============================================================================

TEST(DependencyResolutionEngineTest, VersionConstraint_Compound_RangeUpdates) {
    // ">=1.4.0,<2.0.0" – installed version 1.3.0 must be updated to 1.4.0
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"themis-storage", ">=1.4.0,<2.0.0"});

    VersionMap current = {{"themis-storage", "1.3.0"}};
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(1u, result.steps.size());
    EXPECT_EQ("1.4.0", result.steps[0].to_version);
}

TEST(DependencyResolutionEngineTest, VersionConstraint_EQ_UpdatesExactVersion) {
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"lib", "==1.4.2"});

    VersionMap current = {{"lib", "1.4.0"}};
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(1u, result.steps.size());
    EXPECT_EQ("1.4.2", result.steps[0].to_version);
}

TEST(DependencyResolutionEngineTest, VersionConstraint_GT_BumpsPatch) {
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"lib", ">1.4.0"});

    VersionMap current = {{"lib", "1.3.0"}};
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(1u, result.steps.size());
    EXPECT_EQ("1.4.1", result.steps[0].to_version);
}

TEST(DependencyResolutionEngineTest, VersionConstraint_AlreadySatisfied_NoUpdate) {
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"lib", ">=1.4.0,<2.0.0"});

    // Installed version 1.9.0 satisfies >=1.4.0,<2.0.0 → no update
    VersionMap current = {{"lib", "1.9.0"}};
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.steps.empty());
}

// ============================================================================
// AC5 – Conflict resolution
// ============================================================================

TEST(DependencyResolutionEngineTest, ConflictDetection_ExplicitConflict) {
    // themis-new requires conflicts with themis-old
    DependencyResolver resolver;
    Dependency dep;
    dep.package = "themis-new";
    dep.version_constraint = ">=1.0.0";
    dep.conflicts = {"themis-old"};
    resolver.addDependency("1.5.0", dep);

    // themis-old is currently installed
    VersionMap current = {{"themis-new", "0.9.0"}, {"themis-old", "1.0.0"}};
    auto result = resolver.resolve("1.5.0", current);

    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.conflicts.empty());
    EXPECT_NE(std::string::npos,
              result.error_message.find("conflicts"));
}

TEST(DependencyResolutionEngineTest, ConflictDetection_NoConflict_WhenConflictingPackageAbsent) {
    Dependency dep;
    dep.package = "themis-new";
    dep.version_constraint = ">=1.0.0";
    dep.conflicts = {"themis-old"};

    DependencyResolver resolver;
    resolver.addDependency("1.5.0", dep);

    // themis-old is NOT installed → no conflict
    VersionMap current = {{"themis-new", "0.9.0"}};
    auto result = resolver.resolve("1.5.0", current);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.conflicts.empty());
}

TEST(DependencyResolutionEngineTest, DetectConflicts_VersionConstraintViolation) {
    // themis-query@1.4.0 requires themis-storage >=1.5.1 but 1.5.0 is installed.
    DependencyResolver resolver;
    resolver.addPackageDependency("themis-query", "1.4.0",
                                  {"themis-storage", ">=1.5.1"});

    std::vector<std::pair<std::string, std::string>> installed = {
        {"themis-storage", "1.5.0"},
        {"themis-query",   "1.4.0"}
    };
    auto conflicts = resolver.detectConflicts(installed);

    ASSERT_FALSE(conflicts.empty());
    bool found = false;
    for (const auto& c : conflicts) {
        if ((c.package1 == "themis-query"   && c.package2 == "themis-storage") ||
            (c.package1 == "themis-storage" && c.package2 == "themis-query")) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected a conflict between themis-query and themis-storage";
}

TEST(DependencyResolutionEngineTest, DetectConflicts_NoConflict_WhenVersionSatisfied) {
    DependencyResolver resolver;
    resolver.addPackageDependency("themis-query", "1.4.0",
                                  {"themis-storage", ">=1.5.0"});

    std::vector<std::pair<std::string, std::string>> installed = {
        {"themis-storage", "1.5.0"},
        {"themis-query",   "1.4.0"}
    };
    auto conflicts = resolver.detectConflicts(installed);

    EXPECT_TRUE(conflicts.empty());
}

TEST(DependencyResolutionEngineTest, DetectConflicts_ExplicitConflictList) {
    DependencyResolver resolver;
    Dependency dep;
    dep.package = "pkg-new";
    dep.version_constraint = ">=1.0.0";
    dep.conflicts = {"pkg-old"};
    resolver.addPackageDependency("my-app", "2.0.0", dep);

    std::vector<std::pair<std::string, std::string>> installed = {
        {"pkg-new", "1.0.0"},
        {"pkg-old", "0.5.0"},
        {"my-app",  "2.0.0"}
    };
    auto conflicts = resolver.detectConflicts(installed);

    ASSERT_FALSE(conflicts.empty());
}

// ============================================================================
// AC6 – Automatic backfill of missing dependencies
// ============================================================================

TEST(DependencyResolutionEngineTest, Backfill_MissingRequiredDep) {
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"themis-storage", ">=1.4.0"});

    // themis-storage not installed at all
    VersionMap current; // empty
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(1u, result.steps.size());
    EXPECT_EQ("themis-storage", result.steps[0].package);
    EXPECT_EQ("", result.steps[0].from_version) << "from_version should be empty for backfilled package";
    EXPECT_EQ("1.4.0", result.steps[0].to_version);

    // Should be listed in backfilled
    ASSERT_EQ(1u, result.backfilled.size());
    EXPECT_EQ("themis-storage", result.backfilled[0]);
}

TEST(DependencyResolutionEngineTest, Backfill_MultiplePackagesBackfilled) {
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"alpha", ">=1.0.0"});
    resolver.addDependency("1.5.0", {"beta",  ">=2.0.0"});
    resolver.addDependency("1.5.0", {"gamma", ">=3.0.0"});

    VersionMap current; // nothing installed
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(3u, result.steps.size());
    EXPECT_EQ(3u, result.backfilled.size());

    // backfilled list should be sorted
    EXPECT_EQ("alpha", result.backfilled[0]);
    EXPECT_EQ("beta",  result.backfilled[1]);
    EXPECT_EQ("gamma", result.backfilled[2]);
}

TEST(DependencyResolutionEngineTest, Backfill_OptionalMissingDepNotBackfilled) {
    DependencyResolver resolver;
    Dependency opt_dep;
    opt_dep.package = "optional-lib";
    opt_dep.version_constraint = ">=1.0.0";
    opt_dep.optional = true;
    resolver.addDependency("1.5.0", opt_dep);

    VersionMap current; // nothing installed
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.steps.empty()) << "Optional missing dep should not produce an update step";
    EXPECT_TRUE(result.backfilled.empty());
}

TEST(DependencyResolutionEngineTest, Backfill_TransitiveDependencyBackfilled) {
    // main@1.0.0 → query@>=1.0.0  (not installed)
    // query@1.0.0 → storage@>=1.0.0  (not installed)
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"query", ">=1.0.0"});
    resolver.addPackageDependency("query", "1.0.0", {"storage", ">=1.0.0"});

    VersionMap current;
    auto result = resolver.resolve("1.0.0", current);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(2u, result.steps.size());

    // Both packages should be in backfilled (sorted)
    ASSERT_EQ(2u, result.backfilled.size());

    // storage must come before query in the steps
    const int posStorage = stepPos(result.steps, "storage");
    const int posQuery   = stepPos(result.steps, "query");
    EXPECT_LT(posStorage, posQuery);
}

// ============================================================================
// Additional integration / edge-case tests
// ============================================================================

TEST(DependencyResolutionEngineTest, NoDepsRegistered_EmptyResult) {
    DependencyResolver resolver;
    // No addDependency calls
    VersionMap current = {{"foo", "1.0.0"}};
    auto result = resolver.resolve("1.5.0", current);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.steps.empty());
}

TEST(DependencyResolutionEngineTest, Resolve_WithCompoundConstraint_PartiallyInstalled) {
    DependencyResolver resolver;
    resolver.addDependency("1.5.0", {"lib-a", ">=1.4.0,<2.0.0"});
    resolver.addDependency("1.5.0", {"lib-b", ">=2.0.0"});

    // lib-a needs update, lib-b is fine
    VersionMap current = {{"lib-a", "1.3.0"}, {"lib-b", "2.1.0"}};
    auto result = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(result.success);
    ASSERT_EQ(1u, result.steps.size());
    EXPECT_EQ("lib-a", result.steps[0].package);
}

TEST(DependencyResolutionEngineTest, DetectConflicts_EmptyInstalled_NoConflicts) {
    DependencyResolver resolver;
    resolver.addPackageDependency("pkg", "1.0.0", {"dep", ">=1.0.0"});

    std::vector<std::pair<std::string, std::string>> installed;
    auto conflicts = resolver.detectConflicts(installed);
    EXPECT_TRUE(conflicts.empty());
}

TEST(DependencyResolutionEngineTest, DetectConflicts_NoRegisteredDeps_NoConflicts) {
    DependencyResolver resolver;

    std::vector<std::pair<std::string, std::string>> installed = {
        {"foo", "1.0.0"},
        {"bar", "2.0.0"}
    };
    auto conflicts = resolver.detectConflicts(installed);
    EXPECT_TRUE(conflicts.empty());
}

TEST(DependencyResolutionEngineTest, Resolve_UsageExampleFromIssue) {
    // Reproduces the exact usage example from the issue / FUTURE_ENHANCEMENTS.md
    DependencyResolver resolver;

    resolver.addDependency("1.5.0", {"themis-storage", ">=1.4.0,<2.0.0"});
    resolver.addDependency("1.5.0", {"themis-query",   ">=1.4.5"});

    VersionMap current = {
        {"themis-storage", "1.3.0"},
        {"themis-query",   "1.3.0"}
    };
    auto resolution = resolver.resolve("1.5.0", current);

    ASSERT_TRUE(resolution.success);
    EXPECT_EQ(2u, resolution.steps.size());

    const auto* s_storage = findStep(resolution.steps, "themis-storage");
    ASSERT_NE(nullptr, s_storage);
    EXPECT_EQ("1.3.0", s_storage->from_version);
    EXPECT_EQ("1.4.0", s_storage->to_version);

    const auto* s_query = findStep(resolution.steps, "themis-query");
    ASSERT_NE(nullptr, s_query);
    EXPECT_EQ("1.3.0", s_query->from_version);
    EXPECT_EQ("1.4.5", s_query->to_version);
}

// ============================================================================
// Edge cases: malformed / unusual version strings
// ============================================================================

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_MalformedConstraint_FailsClosed) {
    // Tokens without a recognised operator should fail closed (return false).
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("1.0.0", "1.4.0")); // no operator
}

TEST(DependencyResolutionEngineTest, SatisfiesConstraint_EmptyVersion_WithConstraint) {
    // An empty installed version cannot satisfy a constraint that has a lower bound.
    EXPECT_FALSE(DependencyResolver::satisfiesConstraint("", ">=1.0.0"));
    // But an empty version trivially satisfies an empty constraint.
    EXPECT_TRUE(DependencyResolver::satisfiesConstraint("", ""));
}

TEST(DependencyResolutionEngineTest, MinVersion_MalformedToken_ReturnsEmpty) {
    // A constraint with no recognisable lower-bound operator yields "".
    EXPECT_EQ("", DependencyResolver::minVersionFromConstraint("badtoken"));
    EXPECT_EQ("", DependencyResolver::minVersionFromConstraint("!=1.0.0"));
}

TEST(DependencyResolutionEngineTest, Resolve_EmptyVersionString_NoRegisteredDeps) {
    // Resolving for a version that has no registered deps should succeed cleanly.
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"lib", ">=1.0.0"});

    // Requesting a version with no deps registered (2.0.0)
    VersionMap current = {{"lib", "1.5.0"}};
    auto result = resolver.resolve("2.0.0", current);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.steps.empty());
}

// ============================================================================
// Bug-regression tests (found during code audit)
// ============================================================================

// Bug fix 1: LTE-only constraint violated → must fail with clear error, not
// produce an UpdateStep with to_version="<=2.0.0" (raw constraint string).
TEST(DependencyResolutionEngineTest, Bug_LTEOnlyConstraintViolated_FailsWithError) {
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"lib", "<=2.0.0"});

    // Installed 3.0.0 violates <=2.0.0; no lower bound → cannot auto-resolve
    VersionMap current = {{"lib", "3.0.0"}};
    auto result = resolver.resolve("1.0.0", current);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_NE(std::string::npos,
              result.error_message.find("no lower bound"))
        << "Error should explain the constraint has no lower bound";
    EXPECT_TRUE(result.steps.empty())
        << "No partial steps should be emitted on failure";
}

// Bug fix 1 (variant): NEQ-only constraint violated → same behaviour
TEST(DependencyResolutionEngineTest, Bug_NEQOnlyConstraintViolated_FailsWithError) {
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"lib", "!=1.0.0"});

    VersionMap current = {{"lib", "1.0.0"}};
    auto result = resolver.resolve("1.0.0", current);

    EXPECT_FALSE(result.success);
    EXPECT_NE(std::string::npos, result.error_message.find("no lower bound"));
}

// Bug fix 1 (variant): LTE-only, package not installed → backfill is impossible
TEST(DependencyResolutionEngineTest, Bug_LTEOnlyConstraint_PackageAbsent_FailsWithError) {
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"lib", "<=2.0.0"});

    VersionMap current; // nothing installed
    auto result = resolver.resolve("1.0.0", current);

    EXPECT_FALSE(result.success);
    EXPECT_NE(std::string::npos, result.error_message.find("no lower bound"));
}

// Bug fix 2: Duplicate dep registrations in the DAG phase must not inflate
// in-degree and break topological sort.
TEST(DependencyResolutionEngineTest, Bug_DuplicateDepRegistration_CorrectTopoOrder) {
    // pkg-b depends on pkg-a, registered TWICE (idempotent duplicate)
    DependencyResolver resolver;
    resolver.addDependency("1.0.0", {"pkg-a", ">=1.0.0"});
    resolver.addDependency("1.0.0", {"pkg-b", ">=1.0.0"});
    resolver.addPackageDependency("pkg-b", "1.0.0", {"pkg-a", ">=1.0.0"});
    resolver.addPackageDependency("pkg-b", "1.0.0", {"pkg-a", ">=1.0.0"}); // duplicate

    VersionMap current = {{"pkg-a", "0.5.0"}, {"pkg-b", "0.5.0"}};
    auto result = resolver.resolve("1.0.0", current);

    ASSERT_TRUE(result.success)
        << "Duplicate dep registrations must not cause resolution failure";
    ASSERT_EQ(2u, result.steps.size());

    const int posA = stepPos(result.steps, "pkg-a");
    const int posB = stepPos(result.steps, "pkg-b");
    EXPECT_LT(posA, posB) << "pkg-a must still appear before pkg-b";
}
