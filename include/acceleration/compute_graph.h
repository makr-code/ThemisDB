/**
 * @file compute_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace themis {
namespace acceleration {

enum class NodeDependencyMode {
    SEQUENTIAL,  ///< Node executes only after all predecessors complete (blocking)
    PARALLEL     ///< Node may execute concurrently with eligible sibling nodes
};

struct ComputeGraphNode {
    std::string node_id;                      ///< Unique node identifier (within graph scope)
    std::string kernel_name;                  ///< Name of the kernel to execute (e.g. "l2_distance")
    std::vector<std::string> input_buffer_ids;   ///< IDs of input buffers for this kernel
    std::vector<std::string> output_buffer_ids;  ///< IDs of output buffers produced by this kernel
    std::map<std::string, std::string> attributes; ///< Optional kernel attributes (e.g. {"metric": "L2"})
    NodeDependencyMode dependency_mode = NodeDependencyMode::SEQUENTIAL; ///< Dependency semantics
};

struct ComputeGraphEdge {
    std::string from_node_id;    ///< Source node ID
    std::string to_node_id;      ///< Destination node ID
    std::string data_buffer_id;  ///< Shared buffer connecting these nodes
};

struct ComputeGraphConfig {
    std::string graph_id;                    ///< Unique identifier for this graph
    bool enable_fusion = true;               ///< Enable kernel fusion optimization (combines compatible ops)
    bool enable_memory_reuse = true;         ///< Enable buffer aliasing for memory efficiency
    int max_parallel_nodes = 4;              ///< Maximum number of nodes to run in parallel
    std::chrono::milliseconds timeout{5000}; ///< Execution timeout for the entire graph
};

struct ComputeGraphStats {
    size_t nodes_executed = 0;      ///< Number of nodes that executed (excluding fused nodes)
    size_t kernel_fusions = 0;      ///< Number of kernel fusion operations performed
    double total_execution_ms = 0.0; ///< Total execution time (milliseconds)
    size_t peak_memory_bytes = 0;   ///< Peak memory usage during execution (bytes)
};

class IComputeGraph {
public:
    /**
     * @brief ICompute Graph.
     * @return Return value.
     */
    virtual ~IComputeGraph() = default;

    [[nodiscard]] virtual bool addNode(const ComputeGraphNode& node) = 0;

    [[nodiscard]] virtual bool addEdge(const ComputeGraphEdge& edge) = 0;

    [[nodiscard]] virtual bool removeNode(const std::string& node_id) = 0;

    [[nodiscard]] virtual bool compile(const ComputeGraphConfig& config) = 0;

    [[nodiscard]] virtual bool execute() = 0;

    [[nodiscard]] virtual ComputeGraphStats getStats() const = 0;

    /**
     * @brief Reset the modification detection flag.
     */
    virtual void reset() = 0;

    [[nodiscard]] virtual std::string toDot() const = 0;

    [[nodiscard]] virtual bool isCompiled() const = 0;

    [[nodiscard]] virtual size_t nodeCount() const = 0;

    [[nodiscard]] virtual size_t edgeCount() const = 0;
};

class IComputeGraphFactory {
public:
    /**
     * @brief ICompute Graph Factory.
     * @return Return value.
     */
    virtual ~IComputeGraphFactory() = default;

    [[nodiscard]] virtual std::unique_ptr<IComputeGraph> create(const std::string& backend_id) = 0;
};

} // namespace acceleration
} // namespace themis
