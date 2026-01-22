---
name: "📦 Plugin Dependency Management"
about: Implement automatic dependency resolution and load ordering
title: "[Plugin] Implement Dependency Graph Resolver"
labels: 
  - type:feature
  - area:plugins
  - priority:P2
  - effort:large
assignees: ''

---

## 📋 Problem / Motivation

Plugin dependencies are currently defined in manifests but not actively managed:

**Current State:**
- ✅ Dependencies are parsed from `plugin.json`
- ❌ No verification that dependencies are available
- ❌ No automatic load ordering
- ❌ No circular dependency detection
- ❌ No topological sorting

**Impact:**
- Plugins may fail to load due to missing dependencies
- Load order is arbitrary (by priority only)
- Circular dependencies cause undefined behavior

## 🎯 Proposed Solution

Implement a dependency graph resolver with:

### 1. Dependency Graph
- Build graph from plugin manifests
- Track both dependencies and dependents

### 2. Circular Dependency Detection
- Detect cycles in dependency graph
- Report circular dependencies with full path

### 3. Topological Sort
- Compute safe load order
- Ensure dependencies are loaded before dependents

### 4. Automatic Resolution
- Load dependencies automatically
- Handle transitive dependencies

## 📝 Implementation Details

### PluginDependencyResolver Class

```cpp
class PluginDependencyResolver {
public:
    struct DependencyGraph {
        std::map<std::string, std::vector<std::string>> dependencies;
        std::map<std::string, std::vector<std::string>> dependents;
    };
    
    /**
     * @brief Build dependency graph from manifests
     */
    static DependencyGraph buildGraph(
        const std::map<std::string, PluginEntry>& plugins
    );
    
    /**
     * @brief Detect circular dependencies
     * @return Vector of cycles (each cycle is a vector of plugin names)
     */
    static std::vector<std::vector<std::string>> detectCircularDependencies(
        const DependencyGraph& graph
    );
    
    /**
     * @brief Compute load order (topological sort)
     * @return Vector of plugin names in load order
     * @throws std::runtime_error if circular dependencies detected
     */
    static std::vector<std::string> computeLoadOrder(
        const DependencyGraph& graph
    );
};
```

### Integration with autoLoadPlugins

```cpp
size_t PluginManager::autoLoadPlugins() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 1. Build dependency graph
    auto graph = PluginDependencyResolver::buildGraph(plugins_);
    
    // 2. Check for circular dependencies
    auto cycles = PluginDependencyResolver::detectCircularDependencies(graph);
    if (!cycles.empty()) {
        // Log and report cycles
        return 0;
    }
    
    // 3. Compute load order
    auto load_order = PluginDependencyResolver::computeLoadOrder(graph);
    
    // 4. Load in dependency order
    for (const auto& name : load_order) {
        if (plugins_[name].manifest.auto_load) {
            loadPlugin(name);
        }
    }
}
```

See detailed implementation in: `docs/de/plugins/PLUGIN_SYSTEM_CONSISTENCY_ANALYSIS.md` (lines 539-760)

## ✅ Acceptance Criteria

- [ ] `PluginDependencyResolver` class implemented
- [ ] Dependency graph building from manifests
- [ ] Circular dependency detection with reporting
- [ ] Topological sort for load order
- [ ] Integration with `autoLoadPlugins()`
- [ ] Integration with `loadPlugin()` for automatic dependency loading
- [ ] Error messages include full dependency paths
- [ ] Unit tests for all graph algorithms
- [ ] Unit tests for circular dependency detection
- [ ] Integration tests with complex dependency scenarios
- [ ] Documentation with examples

## 🔗 Related

- Documentation: `docs/de/plugins/PLUGIN_SYSTEM_CONSISTENCY_ANALYSIS.md`
- Related: Enhanced hot-reload (#TBD)
- Related: Plugin metrics (#TBD)

## 📊 Impact

**Benefits:**
- Reliable plugin loading
- Clear error messages for dependency issues
- Prevents circular dependency bugs
- Better plugin developer experience

**Risks:**
- Performance impact for large plugin sets (mitigated by caching)
- Breaking change if existing plugins have circular dependencies

## 🧪 Testing Strategy

1. **Unit Tests:**
   - Empty graph
   - Single plugin (no dependencies)
   - Linear dependency chain (A→B→C)
   - Diamond dependency (A→B, A→C, B→D, C→D)
   - Direct circular dependency (A→B, B→A)
   - Transitive circular dependency (A→B→C→A)

2. **Integration Tests:**
   - Load 10+ plugins with complex dependencies
   - Verify load order matches dependency requirements
   - Test error handling for missing dependencies

3. **Performance Tests:**
   - Measure graph building time for 100 plugins
   - Measure topological sort time

## 📚 Additional Context

This feature was identified during the plugin system consistency analysis (2026-01-20).

**Priority Justification:** P2 (High) - Critical for reliable plugin loading in production.

**Effort Estimate:** Large (1-2 weeks) - Requires graph algorithms, integration, and comprehensive testing.

**Algorithm Complexity:**
- Graph building: O(n) where n = number of plugins
- Circular detection: O(n + e) where e = number of dependency edges
- Topological sort: O(n + e)
