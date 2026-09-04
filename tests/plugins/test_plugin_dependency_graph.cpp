/// @file test_plugin_dependency_graph.cpp
/// @brief Unit tests for PluginDependencyGraph
///
/// Tests verify:
/// - Empty graph behaviour
/// - Node and edge addition
/// - buildFromResolver() integration
/// - buildFromRegistry() integration
/// - Cycle detection
/// - Topological ordering
/// - DOT export format
/// - JSON export format
/// - ASCII export format
/// - Version constraint annotations in all formats

#include <gtest/gtest.h>
#include "themis/base/plugin_dependency_graph.h"

#include <sstream>
#include <string>

using namespace themis::modules;

// ============================================================================
// Helpers
// ============================================================================

static ModuleDependency requiredDep(const std::string& name,
                                    const std::string& minVer = "",
                                    const std::string& maxVer = "")
{
    ModuleDependency d;
    d.name       = name;
    d.minVersion = minVer;
    d.maxVersion = maxVer;
    d.required   = true;
    return d;
}

static ModuleDependency optionalDep(const std::string& name)
{
    ModuleDependency d;
    d.name     = name;
    d.required = false;
    return d;
}

// ============================================================================
// Empty graph
// ============================================================================

TEST(PluginDependencyGraph, EmptyGraph) {
    PluginDependencyGraph g;
    EXPECT_EQ(g.nodeCount(), 0u);
    EXPECT_EQ(g.edgeCount(), 0u);
    EXPECT_TRUE(g.nodes().empty());
    EXPECT_TRUE(g.edges().empty());
    EXPECT_TRUE(g.detectCycles().empty());
    EXPECT_TRUE(g.topologicalOrder().empty());
}

// ============================================================================
// addModule / addDependency
// ============================================================================

TEST(PluginDependencyGraph, AddSingleModule) {
    PluginDependencyGraph g;
    g.addModule("base", "1.0.0");

    EXPECT_EQ(g.nodeCount(), 1u);
    EXPECT_EQ(g.edgeCount(), 0u);

    auto nodes = g.nodes();
    ASSERT_EQ(nodes.size(), 1u);
    EXPECT_EQ(nodes[0].name,    "base");
    EXPECT_EQ(nodes[0].version, "1.0.0");
}

TEST(PluginDependencyGraph, AddModuleNoVersion) {
    PluginDependencyGraph g;
    g.addModule("base");

    EXPECT_EQ(g.nodeCount(), 1u);
    EXPECT_EQ(g.nodes()[0].version, "");
}

TEST(PluginDependencyGraph, AddModuleIdempotent) {
    // Adding the same module twice replaces the node (no duplicates).
    PluginDependencyGraph g;
    g.addModule("base", "1.0.0");
    g.addModule("base", "2.0.0");

    EXPECT_EQ(g.nodeCount(), 1u);
    EXPECT_EQ(g.nodes()[0].version, "2.0.0");
}

TEST(PluginDependencyGraph, AddDependencyCreatesNodes) {
    PluginDependencyGraph g;
    g.addDependency("storage", "base");

    EXPECT_EQ(g.nodeCount(), 2u);
    EXPECT_EQ(g.edgeCount(), 1u);

    const auto& e = g.edges()[0];
    EXPECT_EQ(e.from,     "storage");
    EXPECT_EQ(e.to,       "base");
    EXPECT_TRUE(e.required);
}

TEST(PluginDependencyGraph, AddOptionalDependency) {
    PluginDependencyGraph g;
    g.addDependency("plugin_a", "plugin_b", /*required=*/false);

    EXPECT_EQ(g.edgeCount(), 1u);
    EXPECT_FALSE(g.edges()[0].required);
}

TEST(PluginDependencyGraph, AddDependencyWithVersionConstraints) {
    PluginDependencyGraph g;
    g.addDependency("storage", "base", true, "1.0.0", "2.0.0");

    const auto& e = g.edges()[0];
    EXPECT_EQ(e.minVersion, "1.0.0");
    EXPECT_EQ(e.maxVersion, "2.0.0");
}

// ============================================================================
// clear()
// ============================================================================

TEST(PluginDependencyGraph, ClearResetsGraph) {
    PluginDependencyGraph g;
    g.addModule("base", "1.0.0");
    g.addDependency("storage", "base");

    g.clear();

    EXPECT_EQ(g.nodeCount(), 0u);
    EXPECT_EQ(g.edgeCount(), 0u);
}

// ============================================================================
// buildFromResolver
// ============================================================================

TEST(PluginDependencyGraph, BuildFromEmptyResolver) {
    ModuleDependencyResolver resolver;
    PluginDependencyGraph g;
    g.buildFromResolver(resolver);

    EXPECT_EQ(g.nodeCount(), 0u);
    EXPECT_EQ(g.edgeCount(), 0u);
}

TEST(PluginDependencyGraph, BuildFromResolver_SingleModule) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("base", "1.0.0", {});

    PluginDependencyGraph g;
    g.buildFromResolver(resolver);

    EXPECT_EQ(g.nodeCount(), 1u);
    EXPECT_EQ(g.edgeCount(), 0u);

    auto nodes = g.nodes();
    ASSERT_EQ(nodes.size(), 1u);
    EXPECT_EQ(nodes[0].name,    "base");
    EXPECT_EQ(nodes[0].version, "1.0.0");
}

TEST(PluginDependencyGraph, BuildFromResolver_LinearChain) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("base",    "1.0.0", {});
    resolver.registerModule("storage", "2.0.0", {requiredDep("base")});
    resolver.registerModule("query",   "3.0.0", {requiredDep("storage")});

    PluginDependencyGraph g;
    g.buildFromResolver(resolver);

    EXPECT_EQ(g.nodeCount(), 3u);
    EXPECT_EQ(g.edgeCount(), 2u);
}

TEST(PluginDependencyGraph, BuildFromResolver_OptionalDep) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("ext", {});
    resolver.registerModule("plugin", {optionalDep("ext")});

    PluginDependencyGraph g;
    g.buildFromResolver(resolver);

    EXPECT_EQ(g.nodeCount(), 2u);
    ASSERT_EQ(g.edgeCount(), 1u);
    EXPECT_FALSE(g.edges()[0].required);
}

TEST(PluginDependencyGraph, BuildFromResolver_VersionConstraints) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("base",    "1.5.0", {});
    resolver.registerModule("storage", "2.0.0", {requiredDep("base", "1.0.0", "2.0.0")});

    PluginDependencyGraph g;
    g.buildFromResolver(resolver);

    ASSERT_EQ(g.edgeCount(), 1u);
    EXPECT_EQ(g.edges()[0].minVersion, "1.0.0");
    EXPECT_EQ(g.edges()[0].maxVersion, "2.0.0");
}

TEST(PluginDependencyGraph, BuildFromResolver_ReplacesExistingGraph) {
    ModuleDependencyResolver r1;
    r1.registerModule("a", {});
    r1.registerModule("b", {requiredDep("a")});

    PluginDependencyGraph g;
    g.buildFromResolver(r1);
    EXPECT_EQ(g.nodeCount(), 2u);

    ModuleDependencyResolver r2;
    r2.registerModule("x", {});
    g.buildFromResolver(r2);
    EXPECT_EQ(g.nodeCount(), 1u);
}

// ============================================================================
// detectCycles
// ============================================================================

TEST(PluginDependencyGraph, NoCycles_Acyclic) {
    PluginDependencyGraph g;
    g.addModule("a");
    g.addDependency("b", "a");
    g.addDependency("c", "b");

    EXPECT_TRUE(g.detectCycles().empty());
}

TEST(PluginDependencyGraph, CycleDetected_TwoNodes) {
    PluginDependencyGraph g;
    g.addDependency("a", "b");
    g.addDependency("b", "a");

    auto cycles = g.detectCycles();
    EXPECT_FALSE(cycles.empty());
    // Both nodes must appear in cycle report.
    bool foundA = false, foundB = false;
    for (const auto& cycle : cycles) {
        for (const auto& n : cycle) {
            if (n == "a") {
              foundA = true;
            }
            if (n == "b") {
              foundB = true;
            }
        }
    }
    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);
}

TEST(PluginDependencyGraph, CycleDetected_ThreeNodes) {
    PluginDependencyGraph g;
    g.addDependency("a", "b");
    g.addDependency("b", "c");
    g.addDependency("c", "a");

    auto cycles = g.detectCycles();
    EXPECT_FALSE(cycles.empty());
}

TEST(PluginDependencyGraph, SelfLoop) {
    PluginDependencyGraph g;
    g.addDependency("a", "a");

    auto cycles = g.detectCycles();
    EXPECT_FALSE(cycles.empty());
}

// ============================================================================
// topologicalOrder
// ============================================================================

TEST(PluginDependencyGraph, TopologicalOrder_Empty) {
    PluginDependencyGraph g;
    EXPECT_TRUE(g.topologicalOrder().empty());
}

TEST(PluginDependencyGraph, TopologicalOrder_SingleNode) {
    PluginDependencyGraph g;
    g.addModule("a");

    auto order = g.topologicalOrder();
    ASSERT_EQ(order.size(), 1u);
    EXPECT_EQ(order[0], "a");
}

TEST(PluginDependencyGraph, TopologicalOrder_LinearChain) {
    PluginDependencyGraph g;
    // c depends on b, b depends on a → load order: a, b, c
    g.addDependency("b", "a");
    g.addDependency("c", "b");

    auto order = g.topologicalOrder();
    ASSERT_EQ(order.size(), 3u);

    auto pos = [&](const std::string& n) {
        return std::find(order.begin(), order.end(), n) - order.begin();
    };
    EXPECT_LT(pos("a"), pos("b"));
    EXPECT_LT(pos("b"), pos("c"));
}

TEST(PluginDependencyGraph, TopologicalOrder_EmptyWhenCyclic) {
    PluginDependencyGraph g;
    g.addDependency("a", "b");
    g.addDependency("b", "a");

    EXPECT_TRUE(g.topologicalOrder().empty());
}

// ============================================================================
// DOT export
// ============================================================================

TEST(PluginDependencyGraph, ExportDot_EmptyGraph) {
    PluginDependencyGraph g;
    auto dot = g.toString(GraphExportFormat::DOT);

    EXPECT_NE(dot.find("digraph"), std::string::npos);
    EXPECT_NE(dot.find('}'),       std::string::npos);
}

TEST(PluginDependencyGraph, ExportDot_SingleNode) {
    PluginDependencyGraph g;
    g.addModule("base", "1.0.0");
    auto dot = g.toString(GraphExportFormat::DOT);

    EXPECT_NE(dot.find("base"),  std::string::npos);
    EXPECT_NE(dot.find("1.0.0"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportDot_RequiredEdge) {
    PluginDependencyGraph g;
    g.addDependency("storage", "base");
    auto dot = g.toString(GraphExportFormat::DOT);

    EXPECT_NE(dot.find("storage"), std::string::npos);
    EXPECT_NE(dot.find("base"),    std::string::npos);
    EXPECT_NE(dot.find("->"),      std::string::npos);
    // Required edges must NOT be dashed.
    EXPECT_EQ(dot.find("dashed"),  std::string::npos);
}

TEST(PluginDependencyGraph, ExportDot_OptionalEdge_IsDashed) {
    PluginDependencyGraph g;
    g.addDependency("plugin_a", "plugin_b", /*required=*/false);
    auto dot = g.toString(GraphExportFormat::DOT);

    EXPECT_NE(dot.find("dashed"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportDot_VersionConstraintLabel) {
    PluginDependencyGraph g;
    g.addDependency("storage", "base", true, "1.0.0", "2.0.0");
    auto dot = g.toString(GraphExportFormat::DOT);

    EXPECT_NE(dot.find("1.0.0"), std::string::npos);
    EXPECT_NE(dot.find("2.0.0"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportDot_SpecialCharactersEscaped) {
    PluginDependencyGraph g;
    g.addModule("my\"module");
    auto dot = g.toString(GraphExportFormat::DOT);

    // The double-quote must be escaped inside the DOT identifier.
    EXPECT_NE(dot.find("\\\""), std::string::npos);
}

// ============================================================================
// JSON export
// ============================================================================

TEST(PluginDependencyGraph, ExportJson_EmptyGraph) {
    PluginDependencyGraph g;
    auto json = g.toString(GraphExportFormat::JSON);

    EXPECT_NE(json.find("\"nodes\""), std::string::npos);
    EXPECT_NE(json.find("\"edges\""), std::string::npos);
}

TEST(PluginDependencyGraph, ExportJson_SingleNode) {
    PluginDependencyGraph g;
    g.addModule("base", "1.0.0");
    auto json = g.toString(GraphExportFormat::JSON);

    EXPECT_NE(json.find("\"base\""),  std::string::npos);
    EXPECT_NE(json.find("\"1.0.0\""), std::string::npos);
}

TEST(PluginDependencyGraph, ExportJson_Edge) {
    PluginDependencyGraph g;
    g.addDependency("storage", "base");
    auto json = g.toString(GraphExportFormat::JSON);

    EXPECT_NE(json.find("\"from\""),    std::string::npos);
    EXPECT_NE(json.find("\"to\""),      std::string::npos);
    EXPECT_NE(json.find("\"storage\""), std::string::npos);
    EXPECT_NE(json.find("\"base\""),    std::string::npos);
    EXPECT_NE(json.find("\"required\""), std::string::npos);
    EXPECT_NE(json.find("true"),        std::string::npos);
}

TEST(PluginDependencyGraph, ExportJson_OptionalEdge) {
    PluginDependencyGraph g;
    g.addDependency("plugin_a", "plugin_b", /*required=*/false);
    auto json = g.toString(GraphExportFormat::JSON);

    EXPECT_NE(json.find("false"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportJson_VersionConstraints) {
    PluginDependencyGraph g;
    g.addDependency("storage", "base", true, "1.0.0", "2.0.0");
    auto json = g.toString(GraphExportFormat::JSON);

    EXPECT_NE(json.find("\"minVersion\""), std::string::npos);
    EXPECT_NE(json.find("\"maxVersion\""), std::string::npos);
    EXPECT_NE(json.find("\"1.0.0\""),      std::string::npos);
    EXPECT_NE(json.find("\"2.0.0\""),      std::string::npos);
}

TEST(PluginDependencyGraph, ExportJson_NoMinVersionFieldWhenAbsent) {
    PluginDependencyGraph g;
    g.addDependency("storage", "base"); // no version constraints
    auto json = g.toString(GraphExportFormat::JSON);

    EXPECT_EQ(json.find("minVersion"), std::string::npos);
    EXPECT_EQ(json.find("maxVersion"), std::string::npos);
}

// ============================================================================
// ASCII export
// ============================================================================

TEST(PluginDependencyGraph, ExportAscii_EmptyGraph) {
    PluginDependencyGraph g;
    auto ascii = g.toString(GraphExportFormat::ASCII);

    EXPECT_NE(ascii.find("Plugin Dependency Graph"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportAscii_SingleModule) {
    PluginDependencyGraph g;
    g.addModule("base", "1.0.0");
    auto ascii = g.toString(GraphExportFormat::ASCII);

    EXPECT_NE(ascii.find("base"),  std::string::npos);
    EXPECT_NE(ascii.find("1.0.0"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportAscii_NoDependencies) {
    PluginDependencyGraph g;
    g.addModule("standalone");
    auto ascii = g.toString(GraphExportFormat::ASCII);

    EXPECT_NE(ascii.find("no dependencies"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportAscii_ShowsDependencyArrow) {
    PluginDependencyGraph g;
    g.addDependency("storage", "base");
    auto ascii = g.toString(GraphExportFormat::ASCII);

    EXPECT_NE(ascii.find("->"),      std::string::npos);
    EXPECT_NE(ascii.find("storage"), std::string::npos);
    EXPECT_NE(ascii.find("base"),    std::string::npos);
}

TEST(PluginDependencyGraph, ExportAscii_OptionalTag) {
    PluginDependencyGraph g;
    g.addDependency("plugin_a", "plugin_b", /*required=*/false);
    auto ascii = g.toString(GraphExportFormat::ASCII);

    EXPECT_NE(ascii.find("optional"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportAscii_LoadOrderShown) {
    PluginDependencyGraph g;
    g.addModule("base");
    g.addDependency("storage", "base");
    auto ascii = g.toString(GraphExportFormat::ASCII);

    EXPECT_NE(ascii.find("Load order"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportAscii_CycleWarning) {
    PluginDependencyGraph g;
    g.addDependency("a", "b");
    g.addDependency("b", "a");
    auto ascii = g.toString(GraphExportFormat::ASCII);

    EXPECT_NE(ascii.find("cycle"), std::string::npos);
}

TEST(PluginDependencyGraph, ExportAscii_VersionConstraint) {
    PluginDependencyGraph g;
    g.addDependency("storage", "base", true, "1.0.0", "2.0.0");
    auto ascii = g.toString(GraphExportFormat::ASCII);

    EXPECT_NE(ascii.find("1.0.0"), std::string::npos);
    EXPECT_NE(ascii.find("2.0.0"), std::string::npos);
}

// ============================================================================
// toString vs exportTo equivalence
// ============================================================================

TEST(PluginDependencyGraph, ToStringEqualsExportTo) {
    PluginDependencyGraph g;
    g.addModule("base", "1.0.0");
    g.addDependency("storage", "base");

    for (auto fmt : {GraphExportFormat::DOT,
                     GraphExportFormat::JSON,
                     GraphExportFormat::ASCII}) {
        std::ostringstream oss;
        g.exportTo(oss, fmt);
        EXPECT_EQ(oss.str(), g.toString(fmt));
    }
}

// ============================================================================
// Realistic ThemisDB module graph
// ============================================================================

TEST(PluginDependencyGraph, ThemisModuleGraph_EndToEnd) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("themis_base",        "1.0.0", {});
    resolver.registerModule("themis_storage",     "1.0.0", {requiredDep("themis_base")});
    resolver.registerModule("themis_security",    "1.0.0", {requiredDep("themis_base")});
    resolver.registerModule("themis_transaction", "1.0.0", {requiredDep("themis_storage")});
    resolver.registerModule("themis_query",       "1.0.0",
                            {requiredDep("themis_storage"),
                             requiredDep("themis_security")});
    resolver.registerModule("themis_server",      "1.0.0",
                            {requiredDep("themis_query"),
                             requiredDep("themis_transaction")});

    PluginDependencyGraph g;
    g.buildFromResolver(resolver);

    EXPECT_EQ(g.nodeCount(), 6u);
    EXPECT_EQ(g.edgeCount(), 7u);

    // Must be acyclic.
    EXPECT_TRUE(g.detectCycles().empty());

    // themis_base first, themis_server last.
    auto order = g.topologicalOrder();
    ASSERT_EQ(order.size(), 6u);

    auto pos = [&](const std::string& n) {
        return std::find(order.begin(), order.end(), n) - order.begin();
    };
    EXPECT_EQ(pos("themis_base"), 0);
    EXPECT_EQ(pos("themis_server"), static_cast<ptrdiff_t>(order.size()) - 1);

    // Validate all three export formats produce non-empty output with
    // expected content.
    auto dot   = g.toString(GraphExportFormat::DOT);
    auto json  = g.toString(GraphExportFormat::JSON);
    auto ascii = g.toString(GraphExportFormat::ASCII);

    EXPECT_NE(dot.find("themis_base"),   std::string::npos);
    EXPECT_NE(json.find("themis_base"),  std::string::npos);
    EXPECT_NE(ascii.find("themis_base"), std::string::npos);

    EXPECT_NE(dot.find("themis_server"),   std::string::npos);
    EXPECT_NE(json.find("themis_server"),  std::string::npos);
    EXPECT_NE(ascii.find("themis_server"), std::string::npos);
}
