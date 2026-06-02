/*
 * ThemisDB | File: compute_graph.h | Version: 0.1.0 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 135
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file compute_graph.h
 * @brief IComputeGraph: DAG-based kernel scheduling interface
 *
 * Defines the abstract interface for building and executing directed acyclic
 * graphs (DAGs) of compute kernels. Implementations may apply kernel fusion,
 * memory-buffer aliasing, and parallel scheduling across backends.
 *
 * Target: Q3 2026
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace themis {
namespace acceleration {

/// Controls dependency semantics between a node and its predecessors.
enum class NodeDependencyMode {
    SEQUENTIAL, ///< Node executes after all predecessors complete
    PARALLEL    ///< Node may execute concurrently with eligible siblings
};

/// A single kernel node in the compute graph.
struct ComputeGraphNode {
    std::string node_id;
    std::string kernel_name;
    std::vector<std::string> input_buffer_ids;
    std::vector<std::string> output_buffer_ids;
    std::map<std::string, std::string> attributes;
    NodeDependencyMode dependency_mode = NodeDependencyMode::SEQUENTIAL;
};

/// A directed edge connecting two nodes via a named data buffer.
struct ComputeGraphEdge {
    std::string from_node_id;
    std::string to_node_id;
    std::string data_buffer_id;
};

/// Configuration applied during graph compilation.
struct ComputeGraphConfig {
    std::string graph_id;
    bool enable_fusion = true;           ///< Kernel fusion optimization
    bool enable_memory_reuse = true;     ///< Buffer aliasing for memory efficiency
    int max_parallel_nodes = 4;          ///< Max nodes to run in parallel
    std::chrono::milliseconds timeout{5000};
};

/// Per-execution statistics produced by IComputeGraph::getStats().
struct ComputeGraphStats {
    size_t nodes_executed = 0;
    size_t kernel_fusions = 0;
    double total_execution_ms = 0.0;
    size_t peak_memory_bytes = 0;
};

/**
 * @brief Abstract interface for a compiled, executable compute DAG.
 *
 * Workflow:
 *   1. Call addNode() / addEdge() to build the graph topology.
 *   2. Call compile() to validate, topologically sort, and apply optimisations.
 *   3. Call execute() one or more times.
 *   4. Call reset() to clear execution state for re-use.
 *
 * Thread safety: implementations must document their own thread-safety guarantees.
 */
class IComputeGraph {
public:
    virtual ~IComputeGraph() = default;

    /// Add a kernel node. Returns false if node_id already exists.
    [[nodiscard]] virtual bool addNode(const ComputeGraphNode& node) = 0;

    /// Add a directed edge. Returns false on invalid node references.
    [[nodiscard]] virtual bool addEdge(const ComputeGraphEdge& edge) = 0;

    /// Remove a node and all edges incident to it. Returns false if not found.
    [[nodiscard]] virtual bool removeNode(const std::string& node_id) = 0;

    /// Topologically sort, validate, and apply optimisations (fusion, memory reuse).
    /// Must be called before execute(). Returns false on cycle or invalid topology.
    [[nodiscard]] virtual bool compile(const ComputeGraphConfig& config) = 0;

    /// Execute the compiled graph. Returns false on kernel error or timeout.
    /// @pre isCompiled() == true
    [[nodiscard]] virtual bool execute() = 0;

    /// Return execution statistics from the most recent execute() call.
    [[nodiscard]] virtual ComputeGraphStats getStats() const = 0;

    /// Clear execution state, allowing execute() to be called again.
    /// Does not un-compile the graph.
    virtual void reset() = 0;

    /// Export the graph topology as a Graphviz DOT string for visualisation.
    [[nodiscard]] virtual std::string toDot() const = 0;

    /// Returns true after a successful compile() call.
    [[nodiscard]] virtual bool isCompiled() const = 0;

    /// Number of nodes currently in the graph.
    [[nodiscard]] virtual size_t nodeCount() const = 0;

    /// Number of edges currently in the graph.
    [[nodiscard]] virtual size_t edgeCount() const = 0;
};

/// Factory for creating backend-specific IComputeGraph implementations.
class IComputeGraphFactory {
public:
    virtual ~IComputeGraphFactory() = default;

    /// Create a new IComputeGraph bound to the given backend.
    /// @param backend_id  Identifies the target compute backend (e.g. "cuda", "cpu").
    [[nodiscard]] virtual std::unique_ptr<IComputeGraph> create(const std::string& backend_id) = 0;
};

} // namespace acceleration
} // namespace themis
