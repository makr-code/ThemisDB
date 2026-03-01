/// @file test_module_dependency_resolver.cpp
/// @brief Unit tests for ModuleDependencyResolver
///
/// Tests verify:
/// - Empty resolver behaviour
/// - Single-module resolution
/// - Linear dependency chain ordering
/// - Diamond dependency graph
/// - Circular dependency detection
/// - Missing required dependency reporting
/// - Optional dependency handling
/// - Version compatibility checks
/// - resolveFor() subset resolution

#include <gtest/gtest.h>
#include "themis/base/module_loader.h"

using namespace themis::modules;

// ============================================================================
// Helpers
// ============================================================================

/// Build a required ModuleDependency with no version constraints.
static ModuleDependency dep(const std::string& name) {
    ModuleDependency d;
    d.name     = name;
    d.required = true;
    return d;
}

/// Build a required ModuleDependency with version constraints.
static ModuleDependency dep(const std::string& name,
                            const std::string& minVer,
                            const std::string& maxVer) {
    ModuleDependency d;
    d.name       = name;
    d.minVersion = minVer;
    d.maxVersion = maxVer;
    d.required   = true;
    return d;
}

/// Build an optional ModuleDependency.
static ModuleDependency optDep(const std::string& name) {
    ModuleDependency d;
    d.name     = name;
    d.required = false;
    return d;
}

// ============================================================================
// ModuleDependency struct
// ============================================================================

TEST(ModuleDependency, DefaultValues) {
    ModuleDependency d;
    EXPECT_TRUE(d.name.empty());
    EXPECT_TRUE(d.minVersion.empty());
    EXPECT_TRUE(d.maxVersion.empty());
    EXPECT_TRUE(d.required);
}

// ============================================================================
// DependencyResolutionResult struct
// ============================================================================

TEST(DependencyResolutionResult, DefaultValues) {
    DependencyResolutionResult r;
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.loadOrder.empty());
    EXPECT_TRUE(r.missingRequired.empty());
    EXPECT_TRUE(r.cycles.empty());
    EXPECT_TRUE(r.errorMessage.empty());
}

// ============================================================================
// ModuleMetadata has dependencies field
// ============================================================================

TEST(ModuleMetadata, HasDependenciesField) {
    ModuleMetadata meta;
    EXPECT_TRUE(meta.dependencies.empty());

    ModuleDependency d;
    d.name = "themis_base";
    meta.dependencies.push_back(d);
    EXPECT_EQ(meta.dependencies.size(), 1u);
}

// ============================================================================
// ModuleErrorCode new values
// ============================================================================

TEST(ModuleErrorCode, DependencyErrorCodes) {
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::DEPENDENCY_NOT_FOUND),       110);
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::DEPENDENCY_CIRCULAR),         111);
    EXPECT_EQ(static_cast<int>(ModuleErrorCode::DEPENDENCY_VERSION_MISMATCH), 112);
}

// ============================================================================
// isVersionCompatible
// ============================================================================

TEST(ModuleDependencyResolver, VersionCompatible_Unconstrained) {
    // Any version satisfies unconstrained dependency.
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("1.0.0", "", ""));
    EXPECT_TRUE(ModuleDependencyResolver::isVersionCompatible("2.5.3", "", ""));
}

TEST(ModuleDependencyResolver, VersionCompatible_EmptyVersion) {
    // Unversioned module satisfies only unconstrained dependencies.
    EXPECT_TRUE( ModuleDependencyResolver::isVersionCompatible("", "", ""));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("", "1.0.0", ""));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("", "", "2.0.0"));
}

TEST(ModuleDependencyResolver, VersionCompatible_MinConstraint) {
    EXPECT_TRUE( ModuleDependencyResolver::isVersionCompatible("1.2.0", "1.0.0", ""));
    EXPECT_TRUE( ModuleDependencyResolver::isVersionCompatible("1.0.0", "1.0.0", ""));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("0.9.9", "1.0.0", ""));
}

TEST(ModuleDependencyResolver, VersionCompatible_MaxConstraint) {
    EXPECT_TRUE( ModuleDependencyResolver::isVersionCompatible("1.5.0", "", "2.0.0"));
    EXPECT_TRUE( ModuleDependencyResolver::isVersionCompatible("2.0.0", "", "2.0.0"));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("2.1.0", "", "2.0.0"));
}

TEST(ModuleDependencyResolver, VersionCompatible_BothConstraints) {
    EXPECT_TRUE( ModuleDependencyResolver::isVersionCompatible("1.5.0", "1.0.0", "2.0.0"));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("0.9.0", "1.0.0", "2.0.0"));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("3.0.0", "1.0.0", "2.0.0"));
}

TEST(ModuleDependencyResolver, VersionCompatible_PatchLevel) {
    EXPECT_TRUE( ModuleDependencyResolver::isVersionCompatible("1.0.5", "1.0.3", "1.0.9"));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("1.0.2", "1.0.3", "1.0.9"));
    EXPECT_FALSE(ModuleDependencyResolver::isVersionCompatible("1.1.0", "1.0.3", "1.0.9"));
}

// ============================================================================
// resolve() — basic cases
// ============================================================================

TEST(ModuleDependencyResolver, EmptyResolver) {
    ModuleDependencyResolver resolver;
    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.loadOrder.empty());
    EXPECT_TRUE(result.missingRequired.empty());
    EXPECT_TRUE(result.cycles.empty());
}

TEST(ModuleDependencyResolver, SingleModuleNoDeps) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base", {});

    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 1u);
    EXPECT_EQ(result.loadOrder[0], "themis_base");
}

TEST(ModuleDependencyResolver, TwoModulesNoDeps) {
    // Two independent modules — both appear in load order.
    ModuleDependencyResolver resolver;
    resolver.registerModule("module_b", {});
    resolver.registerModule("module_a", {});

    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 2u);
    // Alphabetical ordering for determinism.
    EXPECT_EQ(result.loadOrder[0], "module_a");
    EXPECT_EQ(result.loadOrder[1], "module_b");
}

// ============================================================================
// resolve() — dependency ordering
// ============================================================================

TEST(ModuleDependencyResolver, LinearChain) {
    // C depends on B; B depends on A.
    // Expected load order: A → B → C.
    ModuleDependencyResolver resolver;
    resolver.registerModule("module_a", {});
    resolver.registerModule("module_b", {dep("module_a")});
    resolver.registerModule("module_c", {dep("module_b")});

    auto result = resolver.resolve();

    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 3u);
    EXPECT_EQ(result.loadOrder[0], "module_a");
    EXPECT_EQ(result.loadOrder[1], "module_b");
    EXPECT_EQ(result.loadOrder[2], "module_c");
}

TEST(ModuleDependencyResolver, DiamondDependency) {
    // A depends on B and C; both depend on D.
    // D must be loaded first, A last.
    ModuleDependencyResolver resolver;
    resolver.registerModule("d", {});
    resolver.registerModule("b", {dep("d")});
    resolver.registerModule("c", {dep("d")});
    resolver.registerModule("a", {dep("b"), dep("c")});

    auto result = resolver.resolve();

    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 4u);

    // D must come before B and C.
    auto posD = std::find(result.loadOrder.begin(), result.loadOrder.end(), "d");
    auto posB = std::find(result.loadOrder.begin(), result.loadOrder.end(), "b");
    auto posC = std::find(result.loadOrder.begin(), result.loadOrder.end(), "c");
    auto posA = std::find(result.loadOrder.begin(), result.loadOrder.end(), "a");

    ASSERT_NE(posD, result.loadOrder.end());
    ASSERT_NE(posB, result.loadOrder.end());
    ASSERT_NE(posC, result.loadOrder.end());
    ASSERT_NE(posA, result.loadOrder.end());

    EXPECT_LT(posD, posB);
    EXPECT_LT(posD, posC);
    EXPECT_LT(posB, posA);
    EXPECT_LT(posC, posA);
}

TEST(ModuleDependencyResolver, MultipleRoots) {
    // Two independent roots each with one dependency.
    ModuleDependencyResolver resolver;
    resolver.registerModule("core",    {});
    resolver.registerModule("network", {});
    resolver.registerModule("storage", {dep("core")});
    resolver.registerModule("query",   {dep("network")});

    auto result = resolver.resolve();

    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 4u);

    auto posCore    = std::find(result.loadOrder.begin(), result.loadOrder.end(), "core");
    auto posNet     = std::find(result.loadOrder.begin(), result.loadOrder.end(), "network");
    auto posStorage = std::find(result.loadOrder.begin(), result.loadOrder.end(), "storage");
    auto posQuery   = std::find(result.loadOrder.begin(), result.loadOrder.end(), "query");

    EXPECT_LT(posCore, posStorage);
    EXPECT_LT(posNet,  posQuery);
}

// ============================================================================
// resolve() — error cases
// ============================================================================

TEST(ModuleDependencyResolver, CircularDependencyTwoModules) {
    // A → B → A
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {dep("b")});
    resolver.registerModule("b", {dep("a")});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.cycles.empty());
    EXPECT_FALSE(result.errorMessage.empty());
    EXPECT_EQ(result.errorMessage.find("Circular"), 0u);
}

TEST(ModuleDependencyResolver, CircularDependencyThreeModules) {
    // A → B → C → A
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {dep("b")});
    resolver.registerModule("b", {dep("c")});
    resolver.registerModule("c", {dep("a")});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.cycles.empty());
    // All three should appear in the cycle report.
    ASSERT_EQ(result.cycles[0].size(), 3u);
}

TEST(ModuleDependencyResolver, SelfDependency) {
    // A module declares itself as a dependency.
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {dep("a")});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.cycles.empty());
}

TEST(ModuleDependencyResolver, MissingRequiredDependency) {
    // A depends on B, but B is not registered.
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {dep("b")});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.missingRequired.size(), 1u);
    EXPECT_EQ(result.missingRequired[0], "b");
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST(ModuleDependencyResolver, MultipleMissingDependencies) {
    // A depends on B and C, neither registered.
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {dep("b"), dep("c")});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.missingRequired.size(), 2u);
    // Sorted alphabetically.
    EXPECT_EQ(result.missingRequired[0], "b");
    EXPECT_EQ(result.missingRequired[1], "c");
}

// ============================================================================
// Optional dependencies
// ============================================================================

TEST(ModuleDependencyResolver, OptionalDepMissing) {
    // A has an optional dependency on B (B not registered).
    // Resolution should succeed and load A alone.
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {optDep("b")});

    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 1u);
    EXPECT_EQ(result.loadOrder[0], "a");
    EXPECT_TRUE(result.missingRequired.empty());
}

TEST(ModuleDependencyResolver, OptionalDepPresent) {
    // A has an optional dependency on B (B registered).
    // B should still be loaded before A.
    ModuleDependencyResolver resolver;
    resolver.registerModule("b", {});
    resolver.registerModule("a", {optDep("b")});

    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 2u);
    EXPECT_EQ(result.loadOrder[0], "b");
    EXPECT_EQ(result.loadOrder[1], "a");
}

TEST(ModuleDependencyResolver, MixedRequiredAndOptional) {
    // A requires B (registered) and optionally depends on C (not registered).
    ModuleDependencyResolver resolver;
    resolver.registerModule("b", {});
    resolver.registerModule("a", {dep("b"), optDep("c")});

    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 2u);
    EXPECT_EQ(result.loadOrder[0], "b");
    EXPECT_EQ(result.loadOrder[1], "a");
}

// ============================================================================
// Re-registration
// ============================================================================

TEST(ModuleDependencyResolver, ReRegisterModule) {
    // Re-registering a module should replace its dependency list.
    ModuleDependencyResolver resolver;
    resolver.registerModule("b", {});
    resolver.registerModule("a", {dep("b")});

    // Re-register "a" with no dependencies.
    resolver.registerModule("a", {});

    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 2u);
    // Both appear but "a" has no ordering constraint relative to "b" now.
}

// ============================================================================
// clear()
// ============================================================================

TEST(ModuleDependencyResolver, ClearEmptiesRegistry) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {});
    resolver.registerModule("b", {dep("a")});

    resolver.clear();

    auto result = resolver.resolve();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.loadOrder.empty());
}

// ============================================================================
// resolveFor()
// ============================================================================

TEST(ModuleDependencyResolver, ResolveForSubset) {
    // Register three modules, resolve only one.
    ModuleDependencyResolver resolver;
    resolver.registerModule("core",    {});
    resolver.registerModule("storage", {dep("core")});
    resolver.registerModule("query",   {dep("storage")});

    // Resolve only "storage" — should pull in "core" transitively.
    auto result = resolver.resolveFor({"storage"});

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 2u);
    EXPECT_EQ(result.loadOrder[0], "core");
    EXPECT_EQ(result.loadOrder[1], "storage");
    // "query" should NOT appear.
    EXPECT_EQ(std::find(result.loadOrder.begin(), result.loadOrder.end(), "query"),
              result.loadOrder.end());
}

TEST(ModuleDependencyResolver, ResolveForEmpty) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {});

    auto result = resolver.resolveFor({});

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.loadOrder.empty());
}

TEST(ModuleDependencyResolver, ResolveForUnregisteredModule) {
    // Resolving a module that was never registered — treated as missing dep.
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {dep("b")});

    auto result = resolver.resolveFor({"a"});

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.missingRequired.empty());
}

TEST(ModuleDependencyResolver, ResolveForFullGraph) {
    // resolveFor() with all module names should equal resolve().
    ModuleDependencyResolver resolver;
    resolver.registerModule("c", {});
    resolver.registerModule("b", {dep("c")});
    resolver.registerModule("a", {dep("b")});

    auto r1 = resolver.resolve();
    auto r2 = resolver.resolveFor({"a", "b", "c"});

    EXPECT_EQ(r1.success,   r2.success);
    EXPECT_EQ(r1.loadOrder, r2.loadOrder);
}

// ============================================================================
// Large graph — stress / ordering correctness
// ============================================================================

TEST(ModuleDependencyResolver, LargeLinearChain) {
    // 10-module linear chain.
    ModuleDependencyResolver resolver;
    resolver.registerModule("m0", {});
    for (int i = 1; i < 10; ++i) {
        std::string name = "m" + std::to_string(i);
        std::string prev = "m" + std::to_string(i - 1);
        resolver.registerModule(name, {dep(prev)});
    }

    auto result = resolver.resolve();

    ASSERT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 10u);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(result.loadOrder[static_cast<size_t>(i)], "m" + std::to_string(i));
    }
}

// ============================================================================
// Realistic ThemisDB module graph
// ============================================================================

TEST(ModuleDependencyResolver, ThemisModuleGraph) {
    // Simulate a realistic module dependency graph.
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base",        {});
    resolver.registerModule("themis_storage",     {dep("themis_base")});
    resolver.registerModule("themis_security",    {dep("themis_base")});
    resolver.registerModule("themis_transaction", {dep("themis_storage")});
    resolver.registerModule("themis_query",       {dep("themis_storage"), dep("themis_security")});
    resolver.registerModule("themis_network",     {dep("themis_base")});
    resolver.registerModule("themis_server",      {dep("themis_query"), dep("themis_network"),
                                                   dep("themis_transaction")});

    auto result = resolver.resolve();

    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.loadOrder.size(), 7u);

    auto pos = [&](const std::string& name) {
        return std::find(result.loadOrder.begin(), result.loadOrder.end(), name)
               - result.loadOrder.begin();
    };

    // themis_base must be first.
    EXPECT_EQ(pos("themis_base"), 0);
    // themis_server must be last.
    EXPECT_EQ(pos("themis_server"), static_cast<ptrdiff_t>(result.loadOrder.size()) - 1);

    // Ordering constraints.
    EXPECT_LT(pos("themis_base"),        pos("themis_storage"));
    EXPECT_LT(pos("themis_base"),        pos("themis_security"));
    EXPECT_LT(pos("themis_storage"),     pos("themis_transaction"));
    EXPECT_LT(pos("themis_storage"),     pos("themis_query"));
    EXPECT_LT(pos("themis_security"),    pos("themis_query"));
    EXPECT_LT(pos("themis_query"),       pos("themis_server"));
    EXPECT_LT(pos("themis_network"),     pos("themis_server"));
    EXPECT_LT(pos("themis_transaction"), pos("themis_server"));
}

// ============================================================================
// Version-aware registerModule overload
// ============================================================================

TEST(ModuleDependencyResolver, RegisterWithVersion) {
    // Modules registered with version strings and then queried.
    ModuleDependencyResolver resolver;
    resolver.registerModule("base", "1.0.0", {});
    resolver.registerModule("storage", "2.3.1", {dep("base")});

    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.loadOrder.size(), 2u);
    EXPECT_EQ(result.loadOrder[0], "base");
    EXPECT_EQ(result.loadOrder[1], "storage");
    EXPECT_TRUE(result.versionMismatches.empty());
}

// ============================================================================
// Version constraint enforcement during resolution
// ============================================================================

TEST(ModuleDependencyResolver, VersionConstraintSatisfied) {
    // base v1.2.0 satisfies requirement >=1.0.0 <=2.0.0.
    ModuleDependencyResolver resolver;
    resolver.registerModule("base", "1.2.0", {});
    resolver.registerModule("storage", "2.0.0", {dep("base", "1.0.0", "2.0.0")});

    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.versionMismatches.empty());
    ASSERT_EQ(result.loadOrder.size(), 2u);
    EXPECT_EQ(result.loadOrder[0], "base");
    EXPECT_EQ(result.loadOrder[1], "storage");
}

TEST(ModuleDependencyResolver, VersionConstraintViolated_TooOld) {
    // base v0.9.0 is below the minimum 1.0.0 required by storage.
    ModuleDependencyResolver resolver;
    resolver.registerModule("base", "0.9.0", {});
    resolver.registerModule("storage", "2.0.0", {dep("base", "1.0.0", "")});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.versionMismatches.size(), 1u);
    EXPECT_TRUE(result.errorMessage.find("Version constraint") != std::string::npos);
}

TEST(ModuleDependencyResolver, VersionConstraintViolated_TooNew) {
    // base v3.0.0 exceeds the maximum 2.0.0 allowed by storage.
    ModuleDependencyResolver resolver;
    resolver.registerModule("base", "3.0.0", {});
    resolver.registerModule("storage", "1.0.0", {dep("base", "", "2.0.0")});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.versionMismatches.size(), 1u);
    EXPECT_TRUE(result.errorMessage.find("Version constraint") != std::string::npos);
}

TEST(ModuleDependencyResolver, VersionConstraintUnversionedDepFails) {
    // base registered without a version; storage requires >=1.0.0.
    // Unversioned module cannot satisfy a constrained dependency.
    ModuleDependencyResolver resolver;
    resolver.registerModule("base", {});        // No version
    resolver.registerModule("storage", "1.0.0", {dep("base", "1.0.0", "")});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.versionMismatches.size(), 1u);
}

TEST(ModuleDependencyResolver, VersionConstraintUnconstrainedAlwaysPasses) {
    // Even an unversioned module satisfies an unconstrained dependency.
    ModuleDependencyResolver resolver;
    resolver.registerModule("base", {});        // No version, no constraint
    resolver.registerModule("storage", "1.0.0", {dep("base", "", "")});

    auto result = resolver.resolve();

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.versionMismatches.empty());
}

TEST(ModuleDependencyResolver, MultipleVersionViolations) {
    // Two different dependency version violations in one pass.
    ModuleDependencyResolver resolver;
    resolver.registerModule("libA", "0.5.0", {});
    resolver.registerModule("libB", "3.0.0", {});
    // top requires libA >=1.0 (too old) and libB <=2.0 (too new).
    ModuleDependency dA = dep("libA", "1.0.0", "");
    ModuleDependency dB = dep("libB", "",      "2.0.0");
    resolver.registerModule("top", "1.0.0", {dA, dB});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.versionMismatches.size(), 2u);
}

TEST(ModuleDependencyResolver, VersionConstraintOptionalDepPresent_Violated) {
    // Optional dep present with a version that violates its constraint.
    // Resolution should fail on version mismatch even for optional deps.
    ModuleDependencyResolver resolver;
    resolver.registerModule("ext", "0.8.0", {});
    ModuleDependency od;
    od.name       = "ext";
    od.minVersion = "1.0.0";
    od.required   = false;
    resolver.registerModule("plugin", "1.0.0", {od});

    auto result = resolver.resolve();

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.versionMismatches.size(), 1u);
}

TEST(ModuleDependencyResolver, DependencyResolutionResult_HasVersionMismatchesField) {
    // Verify the new field is accessible and defaults to empty.
    DependencyResolutionResult r;
    EXPECT_TRUE(r.versionMismatches.empty());
}

// ============================================================================
// getRegisteredModules()
// ============================================================================

TEST(ModuleDependencyResolver, GetRegisteredModules_Empty) {
    ModuleDependencyResolver resolver;
    auto modules = resolver.getRegisteredModules();
    EXPECT_TRUE(modules.empty());
}

TEST(ModuleDependencyResolver, GetRegisteredModules_SingleModule) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base", "1.0.0", {});

    auto modules = resolver.getRegisteredModules();

    ASSERT_EQ(modules.size(), 1u);
    EXPECT_EQ(modules[0].name, "themis_base");
    EXPECT_EQ(modules[0].version, "1.0.0");
    EXPECT_TRUE(modules[0].deps.empty());
}

TEST(ModuleDependencyResolver, GetRegisteredModules_SortedByName) {
    // Modules should be returned sorted alphabetically by name.
    ModuleDependencyResolver resolver;
    resolver.registerModule("module_c", {});
    resolver.registerModule("module_a", {});
    resolver.registerModule("module_b", {});

    auto modules = resolver.getRegisteredModules();

    ASSERT_EQ(modules.size(), 3u);
    EXPECT_EQ(modules[0].name, "module_a");
    EXPECT_EQ(modules[1].name, "module_b");
    EXPECT_EQ(modules[2].name, "module_c");
}

TEST(ModuleDependencyResolver, GetRegisteredModules_PreservesDependencies) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("base", "2.1.0", {});
    resolver.registerModule("storage", "1.0.0", {dep("base", "2.0.0", "3.0.0")});

    auto modules = resolver.getRegisteredModules();

    ASSERT_EQ(modules.size(), 2u);
    // Sorted: "base" before "storage"
    EXPECT_EQ(modules[0].name, "base");
    EXPECT_EQ(modules[0].version, "2.1.0");
    EXPECT_TRUE(modules[0].deps.empty());

    EXPECT_EQ(modules[1].name, "storage");
    EXPECT_EQ(modules[1].version, "1.0.0");
    ASSERT_EQ(modules[1].deps.size(), 1u);
    EXPECT_EQ(modules[1].deps[0].name, "base");
    EXPECT_EQ(modules[1].deps[0].minVersion, "2.0.0");
    EXPECT_EQ(modules[1].deps[0].maxVersion, "3.0.0");
    EXPECT_TRUE(modules[1].deps[0].required);
}

TEST(ModuleDependencyResolver, GetRegisteredModules_NoVersionOverload) {
    // registerModule(name, deps) stores an empty version string.
    ModuleDependencyResolver resolver;
    resolver.registerModule("mymod", {});

    auto modules = resolver.getRegisteredModules();

    ASSERT_EQ(modules.size(), 1u);
    EXPECT_EQ(modules[0].name, "mymod");
    EXPECT_TRUE(modules[0].version.empty());
}

TEST(ModuleDependencyResolver, GetRegisteredModules_AfterClear) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", {});
    resolver.registerModule("b", {dep("a")});
    resolver.clear();

    auto modules = resolver.getRegisteredModules();
    EXPECT_TRUE(modules.empty());
}

TEST(ModuleDependencyResolver, GetRegisteredModules_AfterReRegistration) {
    // Re-registering a module should replace its entry; count stays the same.
    ModuleDependencyResolver resolver;
    resolver.registerModule("a", "1.0.0", {});
    resolver.registerModule("a", "2.0.0", {});

    auto modules = resolver.getRegisteredModules();

    ASSERT_EQ(modules.size(), 1u);
    EXPECT_EQ(modules[0].version, "2.0.0");
}
