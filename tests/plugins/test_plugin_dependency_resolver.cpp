// Test: Plugin Dependency Graph Resolver
// Comprehensive tests for dependency graph building, cycle detection, and topological sorting

#include <gtest/gtest.h>
#include "plugins/plugin_dependency_resolver.h"
#include "plugins/plugin_interface.h"
#include <string>
#include <vector>
#include <map>

using namespace themis::plugins;

// ============================================================================
// Test Fixtures
// ============================================================================

class PluginDependencyResolverTest : public ::testing::Test {
protected:
    // Helper structure for test plugins
    struct TestPluginEntry {
        std::string name;
        PluginManifest manifest;
    };
    
    // Helper function to create a test plugin entry
    TestPluginEntry createPlugin(
        const std::string& name,
        const std::vector<std::string>& dependencies = {}
    ) {
        TestPluginEntry entry;
        entry.name = name;
        entry.manifest.name = name;
        entry.manifest.version = "1.0.0";
        entry.manifest.dependencies = dependencies;
        entry.manifest.type = PluginType::CUSTOM;
        return entry;
    }
    
    // Helper to convert entries to map
    std::map<std::string, TestPluginEntry> toMap(
        const std::vector<TestPluginEntry>& entries
    ) {
        std::map<std::string, TestPluginEntry> result;
        for (const auto& entry : entries) {
            result.emplace(entry.name, entry);
        }
        return result;
    }
};

// ============================================================================
// Graph Building Tests
// ============================================================================

TEST_F(PluginDependencyResolverTest, BuildGraph_EmptyPlugins) {
    std::map<std::string, TestPluginEntry> plugins;
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    EXPECT_TRUE(graph.dependencies.empty());
    EXPECT_TRUE(graph.dependents.empty());
}

TEST_F(PluginDependencyResolverTest, BuildGraph_SinglePluginNoDependencies) {
    auto plugins = toMap({
        createPlugin("PluginA")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    ASSERT_EQ(graph.dependencies.size(), 1);
    EXPECT_TRUE(graph.dependencies["PluginA"].empty());
    EXPECT_TRUE(graph.dependents["PluginA"].empty());
}

TEST_F(PluginDependencyResolverTest, BuildGraph_LinearChain) {
    // A -> B -> C
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB", {"PluginC"}),
        createPlugin("PluginC")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    ASSERT_EQ(graph.dependencies.size(), 3);
    
    // Check dependencies
    EXPECT_EQ(graph.dependencies["PluginA"].size(), 1);
    EXPECT_EQ(graph.dependencies["PluginA"][0], "PluginB");
    
    EXPECT_EQ(graph.dependencies["PluginB"].size(), 1);
    EXPECT_EQ(graph.dependencies["PluginB"][0], "PluginC");
    
    EXPECT_TRUE(graph.dependencies["PluginC"].empty());
    
    // Check reverse dependencies
    EXPECT_EQ(graph.dependents["PluginB"].size(), 1);
    EXPECT_EQ(graph.dependents["PluginB"][0], "PluginA");
    
    EXPECT_EQ(graph.dependents["PluginC"].size(), 1);
    EXPECT_EQ(graph.dependents["PluginC"][0], "PluginB");
}

TEST_F(PluginDependencyResolverTest, BuildGraph_DiamondDependency) {
    // Diamond: A -> B, A -> C, B -> D, C -> D
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB", "PluginC"}),
        createPlugin("PluginB", {"PluginD"}),
        createPlugin("PluginC", {"PluginD"}),
        createPlugin("PluginD")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    ASSERT_EQ(graph.dependencies.size(), 4);
    
    // A depends on B and C
    EXPECT_EQ(graph.dependencies["PluginA"].size(), 2);
    
    // D has two dependents: B and C
    EXPECT_EQ(graph.dependents["PluginD"].size(), 2);
    EXPECT_TRUE(
        std::find(graph.dependents["PluginD"].begin(), 
                  graph.dependents["PluginD"].end(), 
                  "PluginB") != graph.dependents["PluginD"].end()
    );
    EXPECT_TRUE(
        std::find(graph.dependents["PluginD"].begin(), 
                  graph.dependents["PluginD"].end(), 
                  "PluginC") != graph.dependents["PluginD"].end()
    );
}

TEST_F(PluginDependencyResolverTest, BuildGraph_MultipleDependencies) {
    // A -> B, C, D
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB", "PluginC", "PluginD"}),
        createPlugin("PluginB"),
        createPlugin("PluginC"),
        createPlugin("PluginD")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    EXPECT_EQ(graph.dependencies["PluginA"].size(), 3);
    EXPECT_EQ(graph.dependents["PluginB"].size(), 1);
    EXPECT_EQ(graph.dependents["PluginC"].size(), 1);
    EXPECT_EQ(graph.dependents["PluginD"].size(), 1);
}

// ============================================================================
// Circular Dependency Detection Tests
// ============================================================================

TEST_F(PluginDependencyResolverTest, DetectCircular_NoCycles) {
    // Linear: A -> B -> C
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB", {"PluginC"}),
        createPlugin("PluginC")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
    
    EXPECT_TRUE(cycles.empty());
}

TEST_F(PluginDependencyResolverTest, DetectCircular_DirectCycle) {
    // Direct cycle: A -> B, B -> A
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB", {"PluginA"})
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
    
    ASSERT_EQ(cycles.size(), 1);
    EXPECT_EQ(cycles[0].size(), 3);  // A -> B -> A
}

TEST_F(PluginDependencyResolverTest, DetectCircular_TransitiveCycle) {
    // Transitive cycle: A -> B -> C -> A
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB", {"PluginC"}),
        createPlugin("PluginC", {"PluginA"})
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
    
    ASSERT_EQ(cycles.size(), 1);
    EXPECT_EQ(cycles[0].size(), 4);  // A -> B -> C -> A
}

TEST_F(PluginDependencyResolverTest, DetectCircular_SelfCycle) {
    // Self-cycle: A -> A
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginA"})
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
    
    ASSERT_EQ(cycles.size(), 1);
    EXPECT_EQ(cycles[0].size(), 2);  // A -> A
}

TEST_F(PluginDependencyResolverTest, DetectCircular_MultipleCycles) {
    // Multiple independent cycles: A -> B -> A, C -> D -> C
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB", {"PluginA"}),
        createPlugin("PluginC", {"PluginD"}),
        createPlugin("PluginD", {"PluginC"})
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
    
    // Should detect both cycles
    EXPECT_GE(cycles.size(), 2);
}

TEST_F(PluginDependencyResolverTest, DetectCircular_DiamondNoCycle) {
    // Diamond without cycle: A -> B, A -> C, B -> D, C -> D
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB", "PluginC"}),
        createPlugin("PluginB", {"PluginD"}),
        createPlugin("PluginC", {"PluginD"}),
        createPlugin("PluginD")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
    
    EXPECT_TRUE(cycles.empty());
}

// ============================================================================
// Topological Sort / Load Order Tests
// ============================================================================

TEST_F(PluginDependencyResolverTest, ComputeLoadOrder_EmptyGraph) {
    std::map<std::string, TestPluginEntry> plugins;
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    auto load_order = PluginDependencyResolver::computeLoadOrder(graph);
    
    EXPECT_TRUE(load_order.empty());
}

TEST_F(PluginDependencyResolverTest, ComputeLoadOrder_SinglePlugin) {
    auto plugins = toMap({
        createPlugin("PluginA")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto load_order = PluginDependencyResolver::computeLoadOrder(graph);
    
    ASSERT_EQ(load_order.size(), 1);
    EXPECT_EQ(load_order[0], "PluginA");
}

TEST_F(PluginDependencyResolverTest, ComputeLoadOrder_LinearChain) {
    // A -> B -> C
    // Load order should be: C, B, A
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB", {"PluginC"}),
        createPlugin("PluginC")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto load_order = PluginDependencyResolver::computeLoadOrder(graph);
    
    ASSERT_EQ(load_order.size(), 3);
    
    // C has no dependencies, should be first
    EXPECT_EQ(load_order[0], "PluginC");
    
    // B depends on C, should be after C
    auto b_pos = std::find(load_order.begin(), load_order.end(), "PluginB");
    auto c_pos = std::find(load_order.begin(), load_order.end(), "PluginC");
    EXPECT_LT(c_pos, b_pos);
    
    // A depends on B, should be after B
    auto a_pos = std::find(load_order.begin(), load_order.end(), "PluginA");
    EXPECT_LT(b_pos, a_pos);
}

TEST_F(PluginDependencyResolverTest, ComputeLoadOrder_DiamondDependency) {
    // Diamond: A -> B, A -> C, B -> D, C -> D
    // D should be first, B and C next (order between them doesn't matter), A last
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB", "PluginC"}),
        createPlugin("PluginB", {"PluginD"}),
        createPlugin("PluginC", {"PluginD"}),
        createPlugin("PluginD")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto load_order = PluginDependencyResolver::computeLoadOrder(graph);
    
    ASSERT_EQ(load_order.size(), 4);
    
    // D should be first (no dependencies)
    EXPECT_EQ(load_order[0], "PluginD");
    
    // B and C should be after D
    auto d_pos = std::find(load_order.begin(), load_order.end(), "PluginD");
    auto b_pos = std::find(load_order.begin(), load_order.end(), "PluginB");
    auto c_pos = std::find(load_order.begin(), load_order.end(), "PluginC");
    EXPECT_LT(d_pos, b_pos);
    EXPECT_LT(d_pos, c_pos);
    
    // A should be last (depends on B and C)
    auto a_pos = std::find(load_order.begin(), load_order.end(), "PluginA");
    EXPECT_LT(b_pos, a_pos);
    EXPECT_LT(c_pos, a_pos);
    EXPECT_EQ(load_order[3], "PluginA");
}

TEST_F(PluginDependencyResolverTest, ComputeLoadOrder_IndependentPlugins) {
    // Three independent plugins
    auto plugins = toMap({
        createPlugin("PluginA"),
        createPlugin("PluginB"),
        createPlugin("PluginC")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto load_order = PluginDependencyResolver::computeLoadOrder(graph);
    
    ASSERT_EQ(load_order.size(), 3);
    // All plugins should be in the order (order doesn't matter)
    EXPECT_TRUE(std::find(load_order.begin(), load_order.end(), "PluginA") != load_order.end());
    EXPECT_TRUE(std::find(load_order.begin(), load_order.end(), "PluginB") != load_order.end());
    EXPECT_TRUE(std::find(load_order.begin(), load_order.end(), "PluginC") != load_order.end());
}

TEST_F(PluginDependencyResolverTest, ComputeLoadOrder_ThrowsOnCircularDependency) {
    // Circular: A -> B -> A
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB", {"PluginA"})
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    EXPECT_THROW(
        PluginDependencyResolver::computeLoadOrder(graph),
        std::runtime_error
    );
}

TEST_F(PluginDependencyResolverTest, ComputeLoadOrder_ThrowsOnUnregisteredDependency) {
    // Plugin A depends on X, but X is not registered.
    // detectCircularDependencies() finds no cycle (X is merely absent),
    // but computeLoadOrder() cannot satisfy A's in-degree so it throws.
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginX_NotRegistered"})
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    // No cycle — X is simply missing
    EXPECT_TRUE(PluginDependencyResolver::detectCircularDependencies(graph).empty());
    
    // computeLoadOrder cannot resolve the missing dep → must throw
    EXPECT_THROW(
        PluginDependencyResolver::computeLoadOrder(graph),
        std::runtime_error
    );
}

// ============================================================================
// validateDependencies Tests
// ============================================================================

TEST_F(PluginDependencyResolverTest, ValidateDependencies_AllPresent) {
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB", {"PluginC"}),
        createPlugin("PluginC")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto missing = PluginDependencyResolver::validateDependencies(graph);
    
    EXPECT_TRUE(missing.empty());
}

TEST_F(PluginDependencyResolverTest, ValidateDependencies_MissingDirect) {
    // A depends on X (not registered)
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginX_NotRegistered"})
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto missing = PluginDependencyResolver::validateDependencies(graph);
    
    ASSERT_EQ(missing.size(), 1);
    EXPECT_EQ(missing[0].first, "PluginA");
    EXPECT_EQ(missing[0].second, "PluginX_NotRegistered");
}

TEST_F(PluginDependencyResolverTest, ValidateDependencies_MultipleMissing) {
    // A depends on X and Y (neither registered)
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginX", "PluginY"}),
        createPlugin("PluginB", {"PluginZ"})
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto missing = PluginDependencyResolver::validateDependencies(graph);
    
    ASSERT_EQ(missing.size(), 3);  // A->X, A->Y, B->Z
}

TEST_F(PluginDependencyResolverTest, ValidateDependencies_Empty) {
    std::map<std::string, TestPluginEntry> plugins;
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto missing = PluginDependencyResolver::validateDependencies(graph);
    
    EXPECT_TRUE(missing.empty());
}

TEST_F(PluginDependencyResolverTest, ComputeLoadOrder_ComplexGraph) {
    // Complex: A -> B, C; B -> D; C -> D, E; D -> F; E -> F
    // Load order should be: F, then D/E, then B/C, then A
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB", "PluginC"}),
        createPlugin("PluginB", {"PluginD"}),
        createPlugin("PluginC", {"PluginD", "PluginE"}),
        createPlugin("PluginD", {"PluginF"}),
        createPlugin("PluginE", {"PluginF"}),
        createPlugin("PluginF")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto load_order = PluginDependencyResolver::computeLoadOrder(graph);
    
    ASSERT_EQ(load_order.size(), 6);
    
    // F should be first
    EXPECT_EQ(load_order[0], "PluginF");
    
    // Verify dependency constraints are satisfied
    // Build position map once for efficiency
    std::map<std::string, size_t> positions;
    for (size_t i = 0; i < load_order.size(); ++i) {
        positions.emplace(load_order[i], i);
    }
    
    EXPECT_LT(positions["PluginF"], positions["PluginD"]);
    EXPECT_LT(positions["PluginF"], positions["PluginE"]);
    EXPECT_LT(positions["PluginD"], positions["PluginB"]);
    EXPECT_LT(positions["PluginD"], positions["PluginC"]);
    EXPECT_LT(positions["PluginE"], positions["PluginC"]);
    EXPECT_LT(positions["PluginB"], positions["PluginA"]);
    EXPECT_LT(positions["PluginC"], positions["PluginA"]);
    
    // A should be last
    EXPECT_EQ(load_order[5], "PluginA");
}

// ============================================================================
// Helper Method Tests
// ============================================================================

TEST_F(PluginDependencyResolverTest, HasDependency_Exists) {
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    EXPECT_TRUE(PluginDependencyResolver::hasDependency(graph, "PluginA", "PluginB"));
    EXPECT_FALSE(PluginDependencyResolver::hasDependency(graph, "PluginB", "PluginA"));
}

TEST_F(PluginDependencyResolverTest, HasDependency_NonExistent) {
    auto plugins = toMap({
        createPlugin("PluginA"),
        createPlugin("PluginB")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    
    EXPECT_FALSE(PluginDependencyResolver::hasDependency(graph, "PluginA", "PluginB"));
    EXPECT_FALSE(PluginDependencyResolver::hasDependency(graph, "PluginC", "PluginA"));
}

TEST_F(PluginDependencyResolverTest, GetTransitiveDependencies_NoDependencies) {
    auto plugins = toMap({
        createPlugin("PluginA")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto deps = PluginDependencyResolver::getTransitiveDependencies(graph, "PluginA");
    
    EXPECT_TRUE(deps.empty());
}

TEST_F(PluginDependencyResolverTest, GetTransitiveDependencies_DirectOnly) {
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto deps = PluginDependencyResolver::getTransitiveDependencies(graph, "PluginA");
    
    ASSERT_EQ(deps.size(), 1);
    EXPECT_EQ(deps[0], "PluginB");
}

TEST_F(PluginDependencyResolverTest, GetTransitiveDependencies_Transitive) {
    // A -> B -> C
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB"}),
        createPlugin("PluginB", {"PluginC"}),
        createPlugin("PluginC")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto deps = PluginDependencyResolver::getTransitiveDependencies(graph, "PluginA");
    
    ASSERT_EQ(deps.size(), 2);
    // Should be in load order: C first, then B
    EXPECT_EQ(deps[0], "PluginC");
    EXPECT_EQ(deps[1], "PluginB");
}

TEST_F(PluginDependencyResolverTest, GetTransitiveDependencies_Diamond) {
    // A -> B, C; B -> D; C -> D
    auto plugins = toMap({
        createPlugin("PluginA", {"PluginB", "PluginC"}),
        createPlugin("PluginB", {"PluginD"}),
        createPlugin("PluginC", {"PluginD"}),
        createPlugin("PluginD")
    });
    
    auto graph = PluginDependencyResolver::buildGraph(plugins);
    auto deps = PluginDependencyResolver::getTransitiveDependencies(graph, "PluginA");
    
    ASSERT_EQ(deps.size(), 3);
    // D should appear only once despite being a common dependency
    size_t d_count = std::count(deps.begin(), deps.end(), "PluginD");
    EXPECT_EQ(d_count, 1);
    
    // D should be first in the list (no dependencies)
    EXPECT_EQ(deps[0], "PluginD");
}

// ============================================================================
// Main
// ============================================================================


