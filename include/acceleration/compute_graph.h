/**
 * @file compute_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// @brief Controls dependency semantics between a node and its predecessors.
///
/// Determines whether a node waits for all predecessors to finish (SEQUENTIAL)
/// or may execute concurrently with eligible siblings (PARALLEL).
enum class NodeDependencyMode {
    SEQUENTIAL,  ///< Node executes only after all predecessors complete (blocking)
    PARALLEL     ///< Node may execute concurrently with eligible sibling nodes
};

/// @brief A single kernel node in the compute graph (DAG).
///
/// Represents a computational task within the compute DAG. Each node has an
/// associated kernel operation, input/output buffers, and optional runtime
/// attributes. Nodes are connected via edges, forming a directed acyclic graph
/// (DAG) of dependencies.
struct ComputeGraphNode {
    std::string node_id;                      ///< Unique node identifier (within graph scope)
    std::string kernel_name;                  ///< Name of the kernel to execute (e.g. "l2_distance")
    std::vector<std::string> input_buffer_ids;   ///< IDs of input buffers for this kernel
    std::vector<std::string> output_buffer_ids;  ///< IDs of output buffers produced by this kernel
    std::map<std::string, std::string> attributes; ///< Optional kernel attributes (e.g. {"metric": "L2"})
    NodeDependencyMode dependency_mode = NodeDependencyMode::SEQUENTIAL; ///< Dependency semantics
};

/// @brief A directed edge connecting two nodes via a named data buffer.
///
/// Edges define the data flow between nodes. The buffer transfers output from
/// one node to input of another node in the DAG.
struct ComputeGraphEdge {
    std::string from_node_id;    ///< Source node ID
    std::string to_node_id;      ///< Destination node ID
    std::string data_buffer_id;  ///< Shared buffer connecting these nodes
};

/// @brief Configuration applied during graph compilation and optimization.
///
/// Specifies optimization and execution parameters for the compute graph.
/// Some parameters control optimizations (fusion, memory reuse); others control
/// execution limits (timeout, parallelism).
struct ComputeGraphConfig {
    std::string graph_id;                    ///< Unique identifier for this graph
    bool enable_fusion = true;               ///< Enable kernel fusion optimization (combines compatible ops)
    bool enable_memory_reuse = true;         ///< Enable buffer aliasing for memory efficiency
    int max_parallel_nodes = 4;              ///< Maximum number of nodes to run in parallel
    std::chrono::milliseconds timeout{5000}; ///< Execution timeout for the entire graph
};

/// @brief Per-execution statistics produced by IComputeGraph::getStats().
///
/// Contains metrics collected during graph execution. Useful for profiling,
/// optimization decisions, and performance diagnostics.
struct ComputeGraphStats {
    size_t nodes_executed = 0;      ///< Number of nodes that executed (excluding fused nodes)
    size_t kernel_fusions = 0;      ///< Number of kernel fusion operations performed
    double total_execution_ms = 0.0; ///< Total execution time (milliseconds)
    size_t peak_memory_bytes = 0;   ///< Peak memory usage during execution (bytes)
};

/// @brief Abstract interface for a compiled, executable compute DAG (directed acyclic graph).
///
/// Represents a data-flow graph of kernel operations that can be compiled, optimized,
/// and executed. Supports both construction (add/remove nodes and edges) and execution
/// workflows. Provides topological sort, optimization passes, and statistics collection.
///
/// ## Workflow
/// 1. Call addNode() / addEdge() to build the graph topology
/// 2. Call compile() to validate, topologically sort, and apply optimizations
/// 3. Call execute() one or more times
/// 4. Call reset() to clear execution state for re-use (graph remains compiled)
///
/// ## Thread safety
/// Implementations must document their own thread-safety guarantees. By default,
/// assume instances are not thread-safe; synchronize externally if used from
/// multiple threads.
///
/// ## Example Usage
/// @code
///   auto graph = factory->create("cuda");
///   
///   ComputeGraphNode n1{.node_id="q_load", .kernel_name="load_queries"};
///   ComputeGraphNode n2{.node_id="l2_dist", .kernel_name="l2_distance"};
///   graph->addNode(n1);
///   graph->addNode(n2);
///   graph->addEdge({"q_load", "l2_dist", "queries"});
///   
///   ComputeGraphConfig cfg{.graph_id="demo", .enable_fusion=true};
///   graph->compile(cfg);
///   graph->execute();
///   
///   auto stats = graph->getStats();
///   std::cout << "Executed " << stats.nodes_executed << " nodes in "
///             << stats.total_execution_ms << "ms\n";
/// @endcode
class IComputeGraph {
public:
    virtual ~IComputeGraph() = default;

    /// @brief Add a kernel node to the graph.
    ///
    /// Inserts a node into the graph topology. Fails if a node with the same
    /// node_id already exists.
    ///
    /// @param node ComputeGraphNode with unique node_id, kernel_name, and buffers
    /// @return true on success; false if node_id already exists
    /// @pre !node.node_id.empty() && !node.kernel_name.empty()
    /// @pre isCompiled() == false (graph must not be compiled yet)
    [[nodiscard]] virtual bool addNode(const ComputeGraphNode& node) = 0;

    /// @brief Add a directed edge between two nodes.
    ///
    /// Connects a source node to a destination node via a named data buffer.
    /// The buffer transfers output from source to input of destination.
    ///
    /// @param edge ComputeGraphEdge with valid from_node_id and to_node_id
    /// @return false if node references are invalid (nodes don't exist)
    /// @pre Both from_node_id and to_node_id must exist in the graph
    /// @pre Adding this edge does not create a cycle (DAG property must hold)
    [[nodiscard]] virtual bool addEdge(const ComputeGraphEdge& edge) = 0;

    /// @brief Remove a node and all edges incident to it.
    ///
    /// Removes a node from the graph topology. All incoming and outgoing edges
    /// are also removed to maintain graph consistency.
    ///
    /// @param node_id Node identifier to remove
    /// @return true if the node was found and removed; false otherwise
    /// @pre isCompiled() == false (graph must not be compiled yet)
    [[nodiscard]] virtual bool removeNode(const std::string& node_id) = 0;

    /// @brief Validate, topologically sort, and apply optimizations.
    ///
    /// Prepares the graph for execution: checks for cycles, topologically sorts
    /// nodes, and applies optimization passes (fusion, memory reuse) based on config.
    /// Must be called before execute(). After successful compile(), execute() and
    /// reset() may be called multiple times.
    ///
    /// @param config ComputeGraphConfig with optimization and execution parameters
    /// @return true on success (graph is now compiled and ready to execute)
    /// @return false on error (cycle detected, invalid topology, missing nodes, etc.)
    /// @post isCompiled() == true on success
    /// @note Safe to call multiple times; subsequent calls may re-optimize
    [[nodiscard]] virtual bool compile(const ComputeGraphConfig& config) = 0;

    /// @brief Execute the compiled graph.
    ///
    /// Dispatches all nodes according to the topological order established by
    /// compile(). Respects dependency_mode for each node (SEQUENTIAL vs. PARALLEL).
    /// If any node fails, execution stops and returns false.
    ///
    /// @return true if all nodes executed successfully; false on kernel error or timeout
    /// @pre isCompiled() == true (must call compile() first)
    /// @post Statistics are available via getStats()
    /// @throws std::runtime_error if execution timeout is exceeded
    [[nodiscard]] virtual bool execute() = 0;

    /// @brief Return execution statistics from the most recent execute() call.
    ///
    /// Provides metrics about the last execution: number of nodes executed,
    /// fusion operations, execution time, and peak memory usage.
    ///
    /// @return ComputeGraphStats with execution metrics
    /// @note Valid only after a successful execute() call
    [[nodiscard]] virtual ComputeGraphStats getStats() const = 0;

    /// @brief Clear execution state, allowing execute() to be called again.
    ///
    /// Resets execution state (statistics, buffers) but keeps the compiled DAG
    /// and optimization decisions. Next execute() will re-run the same compiled graph.
    ///
    /// @note This does not un-compile the graph; isCompiled() remains true
    virtual void reset() = 0;

    /// @brief Export the graph topology as a Graphviz DOT string for visualization.
    ///
    /// Generates a text representation of the graph suitable for rendering with
    /// Graphviz tools (dot, neato, etc.). Useful for debugging and visualization.
    ///
    /// @return Valid DOT format string describing all nodes and edges
    /// @note Graph structure remains unchanged; this is a read-only operation
    [[nodiscard]] virtual std::string toDot() const = 0;

    /// @brief Returns true after a successful compile() call.
    ///
    /// Indicates whether the graph has been compiled and is ready for execution.
    ///
    /// @return true if compile() succeeded; false if compile() was never called
    ///         or failed, or if the graph has been modified since compilation
    [[nodiscard]] virtual bool isCompiled() const = 0;

    /// @brief Number of nodes currently in the graph (including fused nodes).
    ///
    /// Returns the count of distinct nodes. Fused nodes still count as individual nodes.
    ///
    /// @return Number of nodes in the graph topology
    [[nodiscard]] virtual size_t nodeCount() const = 0;

    /// @brief Number of edges currently in the graph.
    ///
    /// Returns the count of directed edges (node-to-node connections).
    ///
    /// @return Number of edges in the graph topology
    [[nodiscard]] virtual size_t edgeCount() const = 0;
};

/// @brief Factory interface for creating backend-specific IComputeGraph implementations.
///
/// Provides a factory pattern for creating graphs bound to specific compute backends.
/// Different backends (CUDA, Vulkan, CPU) may have different optimization strategies
/// and execution models; the factory encapsulates these differences.
class IComputeGraphFactory {
public:
    virtual ~IComputeGraphFactory() = default;

    /// @brief Create a new IComputeGraph bound to the given backend.
    ///
    /// Instantiates a new, empty compute graph bound to a specific backend.
    /// The returned graph can then be populated with nodes and edges.
    ///
    /// @param backend_id Identifies the target compute backend
    ///                   Examples: "cuda", "cpu", "vulkan", "auto"
    /// @return Unique pointer to a new IComputeGraph instance; nullptr if backend
    ///         is not available or unrecognized
    /// @throws std::runtime_error if backend initialization fails
    [[nodiscard]] virtual std::unique_ptr<IComputeGraph> create(const std::string& backend_id) = 0;
};

} // namespace acceleration
} // namespace themis
