/**
 * @file plugin_dependency_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Plugin dependency graph visualization for ThemisDB.
//
// Builds a dependency graph from a ModuleDependencyResolver or
// ModuleRegistry snapshot and exports it to DOT (Graphviz), JSON,
// or ASCII text representations.
//
// See src/base/ROADMAP.md – Long-term: Plugin dependency graph visualization

#pragma once

#include "themis/base/module_loader.h"

#include <string>
#include <vector>
#include <map>
#include <set>
#include <ostream>

namespace themis {
namespace modules {

// =============================================================================
// GraphExportFormat
// =============================================================================

/**
 * @brief Output format for PluginDependencyGraph::exportTo().
 */
enum class GraphExportFormat {
    DOT,    ///< Graphviz DOT language (suitable for dot/neato/fdp rendering)
    JSON,   ///< JSON object with "nodes" and "edges" arrays
    ASCII   ///< Human-readable ASCII dependency tree
};

// =============================================================================
// PluginDependencyGraph
// =============================================================================

/**
 * @brief In-memory representation of a plugin dependency graph.
 *
 * Build a graph from a @c ModuleDependencyResolver (static registry) or
 * from a snapshot of loaded modules provided directly via @c addModule().
 * Then export to DOT, JSON, or ASCII for visualisation in tools like
 * Graphviz, D3.js, or a terminal.
 *
 * Usage:
 * @code
 *   ModuleDependencyResolver resolver;
 *   resolver.registerModule("base",    "1.0.0", {});
 *   resolver.registerModule("storage", "2.0.0", {{"base"}});
 *
 *   PluginDependencyGraph graph;
 *   graph.buildFromResolver(resolver);
 *
 *   // Write a Graphviz DOT file
 *   std::ofstream out("plugins.dot");
 *   graph.exportTo(out, GraphExportFormat::DOT);
 *
 *   // Print ASCII tree to stdout
 *   graph.exportTo(std::cout, GraphExportFormat::ASCII);
 * @endcode
 */
class PluginDependencyGraph {
public:
    // -------------------------------------------------------------------------
    // Inner types
    // -------------------------------------------------------------------------

    /**
     * @brief A single node in the dependency graph.
     */
    struct Node {
        std::string name;       ///< Module/plugin name
        std::string version;    ///< Semantic version string (may be empty)
    };

    /**
     * @brief A directed edge: @c from depends on @c to.
     */
    struct Edge {
        std::string from;       ///< Dependent module
        std::string to;         ///< Dependency module
        bool        required;   ///< False → optional dependency
        std::string minVersion; ///< Lower version bound declared by @c from
        std::string maxVersion; ///< Upper version bound declared by @c from
    };

    // -------------------------------------------------------------------------
    // Construction helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Add a module node with an optional version string.
     *
     * Registering the same name twice replaces the previous node.
     *
     * @param name    Module name.
     * @param version Semantic version string (may be "").
     */
    void addModule(const std::string& name, const std::string& version = "");

    /**
     * @brief Add a directed dependency edge.
     *
     * Both @p from and @p to are added as nodes if not already present.
     *
     * @param from       Dependent module name.
     * @param to         Dependency module name.
     * @param required   True if the dependency is required, false if optional.
     * @param minVersion Minimum version of @p to required by @p from (may be "").
     * @param maxVersion Maximum version of @p to required by @p from (may be "").
     */
    void addDependency(const std::string& from,
                       const std::string& to,
                       bool required       = true,
                       const std::string& minVersion = "",
                       const std::string& maxVersion = "");

    /**
     * @brief Populate the graph from a @c ModuleDependencyResolver.
     *
     * Replaces any previously added nodes and edges.
     *
     * @param resolver Source resolver (must already have modules registered).
     */
    void buildFromResolver(const ModuleDependencyResolver& resolver);

    /**
     * @brief Populate the graph from the @c ModuleRegistry singleton.
     *
     * Uses the currently registered (loaded) modules and the dependency
     * metadata stored in each module's @c LoadedModule::metadata field.
     * Replaces any previously added nodes and edges.
     */
    void buildFromRegistry();

    /**
     * @brief Remove all nodes and edges.
     */
    void clear();

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /** @brief Number of nodes in the graph. */
    std::size_t nodeCount() const;

    /** @brief Number of edges in the graph. */
    std::size_t edgeCount() const;

    /** @brief All nodes, sorted by name. */
    std::vector<Node> nodes() const;

    /** @brief All edges. */
    const std::vector<Edge>& edges() const;

    /**
     * @brief Detect cycles in the dependency graph.
     *
     * @return Each inner vector is one cycle (list of module names).
     *         Empty if the graph is acyclic.
     */
    std::vector<std::vector<std::string>> detectCycles() const;

    /**
     * @brief Compute a topological load order (leaves first).
     *
     * @return Ordered module names, or empty if cycles prevent ordering.
     */
    std::vector<std::string> topologicalOrder() const;

    // -------------------------------------------------------------------------
    // Export
    // -------------------------------------------------------------------------

    /**
     * @brief Export the graph in the requested format to an output stream.
     *
     * @param out    Destination stream (file, std::cout, std::ostringstream…).
     * @param format One of DOT, JSON, or ASCII.
     */
    void exportTo(std::ostream& out, GraphExportFormat format) const;

    /**
     * @brief Export the graph and return it as a string.
     *
     * Convenience wrapper around @c exportTo().
     *
     * @param format One of DOT, JSON, or ASCII.
     * @return Serialised graph.
     */
    std::string toString(GraphExportFormat format) const;

private:
    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------

    std::map<std::string, std::string> nodes_;  // name → version
    std::vector<Edge>                  edges_;

    // -------------------------------------------------------------------------
    // Format-specific render helpers
    // -------------------------------------------------------------------------

    void renderDot(std::ostream& out) const;
    void renderJson(std::ostream& out) const;
    void renderAscii(std::ostream& out) const;

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    /// Escape a string for use as a DOT identifier / JSON string value.
    static std::string escapeDotId(const std::string& s);
    static std::string escapeJson(const std::string& s);

    /// Build an adjacency list (name → set of direct dependency names).
    std::map<std::string, std::set<std::string>> buildAdjacency() const;

    /// DFS helper used by detectCycles().
    void dfsVisit(const std::string& node,
                  const std::map<std::string, std::set<std::string>>& adj,
                  std::map<std::string, int>& color,
                  std::vector<std::string>& path,
                  std::vector<std::vector<std::string>>& cycles) const;
};

} // namespace modules
} // namespace themis
