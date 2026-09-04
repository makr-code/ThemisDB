/**
 * @file plugin_dependency_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <stdexcept>
#include <algorithm>

namespace themis {
namespace plugins {

// Forward declaration
struct PluginManifest;

/**
 * @brief Plugin Dependency Graph Resolver
 * 
 * Provides dependency graph building, circular dependency detection,
 * and topological sorting for safe plugin load ordering.
 * 
 * Features:
 * - Build dependency graph from plugin manifests
 * - Detect circular dependencies with full path reporting
 * - Compute safe load order via topological sorting
 * - Handle transitive dependencies
 * 
 * Thread-Safety: All methods are thread-safe (stateless)
 */
class PluginDependencyResolver {
public:
    /**
     * @brief Dependency Graph Structure
     * 
     * Contains both forward dependencies (what each plugin depends on)
     * and reverse dependencies (what depends on each plugin)
     */
    struct DependencyGraph {
        // Map: plugin name -> list of dependencies
        std::map<std::string, std::vector<std::string>> dependencies;
        
        // Map: plugin name -> list of dependents (reverse dependencies)
        std::map<std::string, std::vector<std::string>> dependents;
    };
    
    /**
     * @brief Build dependency graph from plugin entries
     * 
     * @tparam MapType Any map-like container (std::map or std::unordered_map)
     *         whose mapped_type has a .manifest field with a
     *         .dependencies field (std::vector<std::string>)
     *         Example: struct PluginEntry { PluginManifest manifest; ... }
     * @param plugins Map of plugin name to plugin entry
     * @return Complete dependency graph
     */
    template<typename MapType>
    static DependencyGraph buildGraph(
        const MapType& plugins
    ) {
        DependencyGraph graph;
        
        // Initialize all nodes (even plugins with no dependencies)
        for (const auto& [name, entry] : plugins) {
            graph.dependencies[name] = entry.manifest.dependencies;
            
            // Build reverse dependency map
            for (const auto& dep : entry.manifest.dependencies) {
                graph.dependents[dep].push_back(name);
            }
        }
        
        return graph;
    }
    
    /**
     * @brief Detect circular dependencies in the graph
     * 
     * Uses depth-first search with a recursion stack to detect cycles.
     * Returns all unique cycles found in the dependency graph.
     * 
     * @param graph Dependency graph
     * @return Vector of cycles, where each cycle is a vector of plugin names
     *         Empty vector if no cycles detected
     */
    static std::vector<std::vector<std::string>> detectCircularDependencies(
        const DependencyGraph& graph
    ) {
        std::vector<std::vector<std::string>> cycles;
        std::set<std::string> visited;
        std::set<std::string> recursion_stack;
        std::vector<std::string> current_path;
        
        // Check each node as potential starting point
        for (const auto& [name, _] : graph.dependencies) {
            if (visited.find(name) == visited.end()) {
                detectCyclesRecursive(
                    name, 
                    graph, 
                    visited, 
                    recursion_stack, 
                    current_path, 
                    cycles
                );
            }
        }
        
        return cycles;
    }
    
    /**
     * @brief Compute safe load order using topological sort
     * 
     * Uses Kahn's algorithm to compute a topological ordering.
     * Ensures dependencies are loaded before dependents.
     * 
     * @param graph Dependency graph
     * @return Vector of plugin names in safe load order
     * @throws std::runtime_error if circular dependencies detected
     */
    static std::vector<std::string> computeLoadOrder(
        const DependencyGraph& graph
    ) {
        std::vector<std::string> load_order;
        std::map<std::string, int> in_degree;
        
        // Calculate in-degree for each node
        for (const auto& [name, deps] : graph.dependencies) {
            in_degree[name] = static_cast<int>(deps.size());
        }
        
        // Find all nodes with no dependencies (in-degree = 0)
        std::queue<std::string> ready = {};

        for (const auto& [name, degree] : in_degree) {
            if (degree == 0) {
                ready.push(name);
            }
        }
        
        // Process nodes in topological order
        while (!ready.empty()) {
            std::string current = ready.front();
            ready.pop();
            
            load_order.push_back(current);
            
            // Reduce in-degree of dependents
            auto dependents_it = graph.dependents.find(current);
            if (dependents_it != graph.dependents.end()) {
                for (const auto& dependent : dependents_it->second) {
                    in_degree[dependent]--;
                    if (in_degree[dependent] == 0) {
                        ready.push(dependent);
                    }
                }
            }
        }
        
        // If not all nodes were processed, there's a cycle
        if (load_order.size() != graph.dependencies.size()) {
            throw std::runtime_error(
                "Circular dependency detected: Cannot compute load order. "
                "Use detectCircularDependencies() to identify cycles."
            );
        }
        
        return load_order;
    }
    
    /**
     * @brief Check if a specific dependency exists
     * 
     * @param graph Dependency graph
     * @param plugin Plugin name
     * @param dependency Dependency name
     * @return true if dependency exists
     */
    static bool hasDependency(
        const DependencyGraph& graph,
        const std::string& plugin,
        const std::string& dependency
    ) {
        auto it = graph.dependencies.find(plugin);
        if (it == graph.dependencies.end()) {
            return false;
        }
        
        const auto& deps = it->second;
        return std::find(deps.begin(), deps.end(), dependency) != deps.end();
    }
    
    /**
     * @brief Validate that all declared dependencies are registered in the graph
     * 
     * Identifies plugins that declare a dependency on a plugin that is not
     * present in the dependency graph (i.e. not registered with the manager).
     * Must be called before computeLoadOrder() to distinguish missing-dependency
     * failures from circular-dependency failures.
     * 
     * @param graph Dependency graph
     * @return Vector of (dependent_plugin, missing_dependency) pairs.
     *         Empty if all dependencies are satisfied.
     */
    static std::vector<std::pair<std::string, std::string>> validateDependencies(
        const DependencyGraph& graph
    ) {
        std::vector<std::pair<std::string, std::string>> missing;
        
        for (const auto& [name, deps] : graph.dependencies) {
            for (const auto& dep : deps) {
                if (graph.dependencies.find(dep) == graph.dependencies.end()) {
                    missing.emplace_back(name, dep);
                }
            }
        }
        
        return missing;
    }
    
    /**
     * @brief Get all transitive dependencies for a plugin
     * 
     * Returns all direct and indirect dependencies in load order.
     * 
     * @param graph Dependency graph
     * @param plugin Plugin name
     * @return Vector of all transitive dependencies
     */
    static std::vector<std::string> getTransitiveDependencies(
        const DependencyGraph& graph,
        const std::string& plugin
    ) {
        std::vector<std::string> result;
        std::set<std::string> visited;
        
        getTransitiveDependenciesRecursive(plugin, graph, visited, result);
        
        return result;
    }
    
private:
    /**
     * @brief Recursive helper for cycle detection
     * 
     * Uses DFS with a recursion stack to detect back edges (cycles).
     * When a back edge is found, extracts the cycle path.
     */
    static void detectCyclesRecursive(
        const std::string& node,
        const DependencyGraph& graph,
        std::set<std::string>& visited,
        std::set<std::string>& recursion_stack,
        std::vector<std::string>& current_path,
        std::vector<std::vector<std::string>>& cycles
    ) {
        visited.insert(node);
        recursion_stack.insert(node);
        current_path.push_back(node);
        
        // Explore dependencies
        auto deps_it = graph.dependencies.find(node);
        if (deps_it != graph.dependencies.end()) {
            for (const auto& dep : deps_it->second) {
                if (recursion_stack.find(dep) != recursion_stack.end()) {
                    // Cycle found: extract the cycle path
                    auto cycle_start = std::find(
                        current_path.begin(), 
                        current_path.end(), 
                        dep
                    );
                    
                    if (cycle_start != current_path.end()) {
                        std::vector<std::string> cycle(
                            cycle_start, 
                            current_path.end()
                        );
                        cycle.push_back(dep);  // Close the cycle
                        cycles.push_back(cycle);
                    }
                } else if (visited.find(dep) == visited.end()) {
                    // Not visited yet, recurse
                    detectCyclesRecursive(
                        dep, 
                        graph, 
                        visited, 
                        recursion_stack, 
                        current_path, 
                        cycles
                    );
                }
            }
        }
        
        current_path.pop_back();
        recursion_stack.erase(node);
    }
    
    /**
     * @brief Recursive helper for transitive dependency collection
     */
    static void getTransitiveDependenciesRecursive(
        const std::string& node,
        const DependencyGraph& graph,
        std::set<std::string>& visited,
        std::vector<std::string>& result
    ) {
        auto deps_it = graph.dependencies.find(node);
        if (deps_it == graph.dependencies.end()) {
            return;
        }
        
        for (const auto& dep : deps_it->second) {
            if (visited.find(dep) == visited.end()) {
                visited.insert(dep);
                
                // Recurse to get transitive dependencies first
                getTransitiveDependenciesRecursive(dep, graph, visited, result);
                
                // Add this dependency
                result.push_back(dep);
            }
        }
    }
};

} // namespace plugins
} // namespace themis

