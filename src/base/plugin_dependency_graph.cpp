/**
 * @file plugin_dependency_graph.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=21, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Plugin dependency graph visualization — implementation.
//
// See include/themis/base/plugin_dependency_graph.h for the public API.

#include "themis/base/plugin_dependency_graph.h"

#include <algorithm>
#include <sstream>

namespace themis {
namespace modules {

// =============================================================================
// Construction helpers
// =============================================================================

void PluginDependencyGraph::addModule(const std::string& name,
                                      const std::string& version)
{
    nodes_[name] = version;
}

void PluginDependencyGraph::addDependency(const std::string& from,
                                          const std::string& to,
                                          bool required,
                                          const std::string& minVersion,
                                          const std::string& maxVersion)
{
    // Ensure both endpoints exist as nodes.
    nodes_.emplace(from, "");
    nodes_.emplace(to,   "");

    Edge e;
    e.from       = from;
    e.to         = to;
    e.required   = required;
    e.minVersion = minVersion;
    e.maxVersion = maxVersion;
    edges_.push_back(std::move(e));
}

void PluginDependencyGraph::buildFromResolver(
    const ModuleDependencyResolver& resolver)
{
    clear();
    // GAP-FIX range_temporary: capture the returned collection to a named
    // local so the vector lifetime is not bound to the range-for expression.
    const auto registered = resolver.getRegisteredModules();
    for (const auto& info : registered) {
        addModule(info.name, info.version);
        for (const auto& dep : info.deps) {
            addDependency(info.name,
                          dep.name,
                          dep.required,
                          dep.minVersion,
                          dep.maxVersion);
        }
    }
}

void PluginDependencyGraph::buildFromRegistry()
{
    clear();
    // GAP-FIX range_temporary: capture the returned collection to a named
    // local so the vector lifetime is not bound to the range-for expression.
    const auto all_mods = ModuleRegistry::instance().getAllModules();
    for (const auto& mod : all_mods) {
        addModule(mod.name, mod.version);
        for (const auto& dep : mod.metadata.dependencies) {
            addDependency(mod.name,
                          dep.name,
                          dep.required,
                          dep.minVersion,
                          dep.maxVersion);
        }
    }
}

void PluginDependencyGraph::clear()
{
    nodes_.clear();
    edges_.clear();
}

// =============================================================================
// Queries
// =============================================================================

std::size_t PluginDependencyGraph::nodeCount() const
{
    return static_cast<int>(nodes_.size());
}

std::size_t PluginDependencyGraph::edgeCount() const
{
    return static_cast<int>(edges_.size());
}

std::vector<PluginDependencyGraph::Node> PluginDependencyGraph::nodes() const
{
    std::vector<Node> result = {};

    result.reserve(nodes_.size());
    for (const auto& kv : nodes_) {
        Node n;
        n.name    = kv.first;
        n.version = kv.second;
        result.push_back(std::move(n));
    }
    return result;
}

const std::vector<PluginDependencyGraph::Edge>&
PluginDependencyGraph::edges() const
{
    return edges_;
}

std::map<std::string, std::set<std::string>>
PluginDependencyGraph::buildAdjacency() const
{
    std::map<std::string, std::set<std::string>> adj;
    for (const auto& kv : nodes_) {
        adj[kv.first]; // ensure all nodes are present
    }
    for (const auto& e : edges_) {
        adj[e.from].insert(e.to);
    }
    return adj;
}

void PluginDependencyGraph::dfsVisit(
    const std::string& node,
    const std::map<std::string, std::set<std::string>>& adj,
    std::map<std::string, int>& color,
    std::vector<std::string>& path,
    std::vector<std::vector<std::string>>& cycles) const
{
    // color: 0 = white (unvisited), 1 = grey (on stack), 2 = black (done)
    color[node] = 1;
    path.push_back(node);

    auto it = adj.find(node);
    if (it != adj.end()) {
        for (const auto& neighbour : it->second) {
            int& nc = color[neighbour];
            if (nc == 1) {
                // Back edge — cycle found. Collect the cycle segment.
                std::vector<std::string> cycle;
                auto start = std::find(path.begin(), path.end(), neighbour);
                if (start != path.end()) {
                    cycle.assign(start, path.end());
                }
                cycles.push_back(cycle);
            } else if (nc == 0) {
                dfsVisit(neighbour, adj, color, path, cycles);
            }
        }
    }

    path.pop_back();
    color[node] = 2;
}

std::vector<std::vector<std::string>>
PluginDependencyGraph::detectCycles() const
{
    auto adj = buildAdjacency();
    std::map<std::string, int> color = {};

    for (const auto& kv : nodes_) {
        color[kv.first] = 0;
    }

    std::vector<std::vector<std::string>> cycles;
    std::vector<std::string> path;

    for (const auto& kv : nodes_) {
        if (color[kv.first] == 0) {
            dfsVisit(kv.first, adj, color, path, cycles);
        }
    }
    return cycles;
}

std::vector<std::string> PluginDependencyGraph::topologicalOrder() const
{
    auto adj = buildAdjacency();

    // Compute in-degrees.
    std::map<std::string, int> inDegree = {};

    for (const auto& kv : nodes_) {
        inDegree[kv.first] = 0;
    }
    for (const auto& e : edges_) {
        inDegree[e.to]++;
    }

    // Kahn's algorithm with deterministic (alphabetical) ordering.
    std::vector<std::string> ready = {};

    for (const auto& kv : inDegree) {
        if (kv.second == 0) {
            ready.push_back(kv.first);
        }
    }
    std::sort(ready.begin(), ready.end());

    std::vector<std::string> order = {};

    while (!ready.empty()) {
        std::string cur = ready.front();
        ready.erase(ready.begin());
        order.push_back(cur);

        auto it = adj.find(cur);
        if (it != adj.end()) {
            for (const auto& dep : it->second) {
                if (--inDegree[dep] == 0) {
                    auto pos = std::lower_bound(ready.begin(), ready.end(), dep);
                    ready.insert(pos, dep);
                }
            }
        }
    }

    if (static_cast<int>(order.size()) != static_cast<int>(nodes_.size())) {
        // Cycle exists — topological order is undefined.
        return std::vector<std::string>{};
    }
    // Our edges are "from → to" where from is the dependent and to is the
    // dependency (inverse of the standard Kahn prerequisite direction).
    // Using inDegree[e.to]++ gives dependencies a high in-degree and
    // top-level consumers an in-degree of 0, so Kahn's processes consumers
    // first and leaf dependencies last — the reverse of the desired load
    // order.  Reversing the result gives the correct order: leaf
    // dependencies first, top-level consumers last.
    std::reverse(order.begin(), order.end());
    return order;
}

// =============================================================================
// Export
// =============================================================================

void PluginDependencyGraph::exportTo(std::ostream& out,
                                     GraphExportFormat format) const
{
    switch (format) {
        case GraphExportFormat::DOT:   renderDot(out);   break;
        case GraphExportFormat::JSON:  renderJson(out);  break;
        case GraphExportFormat::ASCII: renderAscii(out); break;
    }
}

std::string PluginDependencyGraph::toString(GraphExportFormat format) const
{
    std::ostringstream oss = {};
    exportTo(oss, format);
    return oss.str();
}

// =============================================================================
// DOT renderer
// =============================================================================

/*static*/ std::string PluginDependencyGraph::escapeDotId(const std::string& s)
{
    // Wrap in double quotes and escape internal double quotes and backslashes.
    std::string out = {};
    out.reserve(static_cast<int>(s.size()) + 2);
    out += '"';
    for (char c : s) {
        if (c == '"' || c == '\\') {
            out += '\\';
        }
        out += c;
    }
    out += '"';
    return out;
}

void PluginDependencyGraph::renderDot(std::ostream& out) const
{
    out << "digraph plugin_dependencies {\n";
    out << "    rankdir=LR;\n";
    out << "    node [shape=box, style=filled, fillcolor=lightblue];\n";
    out << "\n";

    // Nodes — annotate with version when available.
    for (const auto& kv : nodes_) {
        out << "    " << escapeDotId(kv.first);
        if (!kv.second.empty()) {
            out << " [label=" << escapeDotId(kv.first + "\\n" + kv.second) << "]";
        }
        out << ";\n";
    }

    if (!edges_.empty()) {
        out << "\n";
    }

    // Edges — required deps are solid, optional deps are dashed.
    for (const auto& e : edges_) {
        out << "    " << escapeDotId(e.from) << " -> " << escapeDotId(e.to);

        bool hasAttrs = false;
        std::string attrs = {};
        if (!e.required) {
            attrs += "style=dashed";
            hasAttrs = true;
        }
        // Build version constraint label.
        if (!e.minVersion.empty() || !e.maxVersion.empty()) {
            // GAP-FIX string_concat_loop: use ostringstream instead of +=
            // inside the edge-iteration loop to avoid repeated heap allocations.
            std::ostringstream voss = {};
            if (!e.minVersion.empty()) {
                voss << ">=" << e.minVersion;
            }
            if (!e.maxVersion.empty()) {
                if (!e.minVersion.empty()) { voss << ' '; }
                voss << "<=" << e.maxVersion;
            }
            const std::string vLabel = voss.str();
            if (hasAttrs) {
                attrs += ", ";
            }
            attrs += "label=" + escapeDotId(vLabel);
            hasAttrs = true;
        }
        if (hasAttrs) {
            out << " [" << attrs << "]";
        }
        out << ";\n";
    }

    out << "}\n";
}

// =============================================================================
// JSON renderer
// =============================================================================

/*static*/ std::string PluginDependencyGraph::escapeJson(const std::string& s)
{
    std::string out = {};
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

void PluginDependencyGraph::renderJson(std::ostream& out) const
{
    out << "{\n";

    // Nodes array.
    out << "  \"nodes\": [\n";
    bool firstNode = true;
    for (const auto& kv : nodes_) {
        if (!firstNode) {
            out << ",\n";
        }
        firstNode = false;
        out << "    {\"name\": \"" << escapeJson(kv.first) << "\"";
        if (!kv.second.empty()) {
            out << ", \"version\": \"" << escapeJson(kv.second) << "\"";
        }
        out << "}";
    }
    out << "\n  ],\n";

    // Edges array.
    out << "  \"edges\": [\n";
    bool firstEdge = true;
    for (const auto& e : edges_) {
        if (!firstEdge) {
            out << ",\n";
        }
        firstEdge = false;
        out << "    {"
            << "\"from\": \""     << escapeJson(e.from) << "\""
            << ", \"to\": \""     << escapeJson(e.to)   << "\""
            << ", \"required\": " << (e.required ? "true" : "false");
        if (!e.minVersion.empty()) {
            out << ", \"minVersion\": \"" << escapeJson(e.minVersion) << "\"";
        }
        if (!e.maxVersion.empty()) {
            out << ", \"maxVersion\": \"" << escapeJson(e.maxVersion) << "\"";
        }
        out << "}";
    }
    out << "\n  ]\n";

    out << "}\n";
}

// =============================================================================
// ASCII renderer
// =============================================================================

void PluginDependencyGraph::renderAscii(std::ostream& out) const
{
    // Compute topological order for the display sequence.
    auto order = topologicalOrder();

    // If the graph has cycles, fall back to alphabetical node order.
    if (order.empty() && !nodes_.empty()) {
        for (const auto& kv : nodes_) {
            order.push_back(kv.first);
        }
    }

    // Build adjacency: from → {to, required, version constraint}
    struct DepInfo {
        std::string to = {};
        bool        required = {};
        std::string versionConstraint = {};
    };
    std::map<std::string, std::vector<DepInfo>> deps;
    for (const auto& e : edges_) {
        DepInfo di;
        di.to       = e.to;
        di.required = e.required;
        if (!e.minVersion.empty() || !e.maxVersion.empty()) {
            // GAP-FIX string_concat_loop: use ostringstream instead of +=
            // inside the edge-iteration loop to avoid repeated heap allocations.
            std::ostringstream voss = {};
            if (!e.minVersion.empty()) {
                voss << ">=" << e.minVersion;
            }
            if (!e.maxVersion.empty()) {
                if (!e.minVersion.empty()) { voss << ' '; }
                voss << "<=" << e.maxVersion;
            }
            di.versionConstraint = voss.str();
        }
        deps[e.from].push_back(std::move(di));
    }

    // Detect cycles for labelling.
    auto cycles = detectCycles();
    std::set<std::string> inCycle = {};

    for (const auto& cycle : cycles) {
        for (const auto& n : cycle) {
            inCycle.insert(n);
        }
    }

    // Header.
    out << "Plugin Dependency Graph\n";
    out << "=======================\n";
    if (!nodes_.empty()) {
        out <<static_cast<int>(nodes_.size()) << " module(s), " <<static_cast<int>(edges_.size()) << " edge(s)";
        if (!cycles.empty()) {
            out << "  [WARNING: " <<static_cast<int>(cycles.size()) << " cycle(s) detected]";
        }
        out << "\n";
    }
    out << "\n";

    // Per-module block.
    for (const auto& name : order) {
        const auto& version = nodes_.at(name);

        // Module header line.
        out << "  [" << name;
        if (!version.empty()) {
            out << " v" << version;
        }
        if (inCycle.count(name)) {
            out << " *CYCLE*";
        }
        out << "]\n";

        // Dependency list.
        auto it = deps.find(name);
        if (it == deps.end() || it->second.empty()) {
            out << "    (no dependencies)\n";
        } else {
            for (const auto& di : it->second) {
                out << "    -> " << di.to;
                if (!di.required) {
                    out << " (optional)";
                }
                if (!di.versionConstraint.empty()) {
                    out << " [" << di.versionConstraint << "]";
                }
                out << "\n";
            }
        }
        out << "\n";
    }

    // Load order summary (only if acyclic).
    if (cycles.empty() && !order.empty()) {
        out << "Load order: ";
        for (std::size_t i = 0; i <static_cast<int>(order.size()); ++i) {
            if (i > 0) {
                out << " -> ";
            }
            out << order[i];
        }
        out << "\n";
    }
}

} // namespace modules
} // namespace themis
