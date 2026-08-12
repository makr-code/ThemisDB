/**
 * @file test_compute_graph_header.cpp
 * @brief Unit tests for the IComputeGraph header interface.
 *
 * Uses an inline mock implementation — no real graph execution backend is required.
 * All tests validate the public contract defined in compute_graph.h.
 */

#include <gtest/gtest.h>
#include "acceleration/compute_graph.h"

#include <stdexcept>
#include <unordered_map>
#include <vector>

using namespace themis::acceleration;

// ============================================================================
// Minimal mock implementation of IComputeGraph for header-contract testing
// ============================================================================

class MockComputeGraph final : public IComputeGraph {
public:
    bool addNode(const ComputeGraphNode& node) override {
        if (nodes_.count(node.node_id)) return false;
        nodes_[node.node_id] = node;
        return true;
    }

    bool addEdge(const ComputeGraphEdge& edge) override {
        if (!nodes_.count(edge.from_node_id) || !nodes_.count(edge.to_node_id)) {
            return false;
        }
        edges_.push_back(edge);
        return true;
    }

    bool removeNode(const std::string& node_id) override {
        return nodes_.erase(node_id) > 0;
    }

    bool compile(const ComputeGraphConfig& config) override {
        config_ = config;
        compiled_ = true;
        return true;
    }

    bool execute() override {
        if (!compiled_) return false;
        stats_.nodes_executed = nodes_.size();
        stats_.kernel_fusions = config_.enable_fusion ? 1 : 0;
        stats_.total_execution_ms = 0.5;
        stats_.peak_memory_bytes = nodes_.size() * 1024;
        return true;
    }

    ComputeGraphStats getStats() const override { return stats_; }

    void reset() override {
        stats_ = {};
    }

    std::string toDot() const override {
        std::string dot = "digraph G {\n";
        for (const auto& e : edges_) {
            dot += "  \"" + e.from_node_id + "\" -> \"" + e.to_node_id + "\";\n";
        }
        dot += "}\n";
        return dot;
    }

    bool isCompiled() const override { return compiled_; }
    size_t nodeCount() const override { return nodes_.size(); }
    size_t edgeCount() const override { return edges_.size(); }

private:
    std::unordered_map<std::string, ComputeGraphNode> nodes_;
    std::vector<ComputeGraphEdge> edges_;
    ComputeGraphConfig config_;
    ComputeGraphStats stats_;
    bool compiled_ = false;
};

// ============================================================================
// Mock factory
// ============================================================================

class MockComputeGraphFactory final : public IComputeGraphFactory {
public:
    std::unique_ptr<IComputeGraph> create(const std::string& /*backend_id*/) override {
        return std::make_unique<MockComputeGraph>();
    }
};

// ============================================================================
// Tests
// ============================================================================

class ComputeGraphHeaderTest : public ::testing::Test {
protected:
    MockComputeGraph graph;

    ComputeGraphNode makeNode(const std::string& id, const std::string& kernel = "relu") {
        ComputeGraphNode n;
        n.node_id = id;
        n.kernel_name = kernel;
        return n;
    }

    ComputeGraphEdge makeEdge(const std::string& from, const std::string& to) {
        ComputeGraphEdge e;
        e.from_node_id = from;
        e.to_node_id = to;
        e.data_buffer_id = from + "_to_" + to;
        return e;
    }
};

TEST_F(ComputeGraphHeaderTest, NodeDefaultDependencyModeIsSequential) {
    ComputeGraphNode node = makeNode("n1");
    EXPECT_EQ(NodeDependencyMode::SEQUENTIAL, node.dependency_mode);
}

TEST_F(ComputeGraphHeaderTest, AddNodeSucceedsAndIncrementsCount) {
    EXPECT_EQ(0u, graph.nodeCount());
    EXPECT_TRUE(graph.addNode(makeNode("n1")));
    EXPECT_EQ(1u, graph.nodeCount());
}

TEST_F(ComputeGraphHeaderTest, AddDuplicateNodeReturnsFalse) {
    EXPECT_TRUE(graph.addNode(makeNode("n1")));
    EXPECT_FALSE(graph.addNode(makeNode("n1")));
    EXPECT_EQ(1u, graph.nodeCount());
}

TEST_F(ComputeGraphHeaderTest, AddEdgeSucceedsWithExistingNodes) {
    graph.addNode(makeNode("a"));
    graph.addNode(makeNode("b"));
    EXPECT_TRUE(graph.addEdge(makeEdge("a", "b")));
    EXPECT_EQ(1u, graph.edgeCount());
}

TEST_F(ComputeGraphHeaderTest, AddEdgeFailsWithMissingNode) {
    graph.addNode(makeNode("a"));
    EXPECT_FALSE(graph.addEdge(makeEdge("a", "missing")));
    EXPECT_EQ(0u, graph.edgeCount());
}

TEST_F(ComputeGraphHeaderTest, CompileSetIsCompiledTrue) {
    graph.addNode(makeNode("n1"));
    EXPECT_FALSE(graph.isCompiled());

    ComputeGraphConfig cfg;
    cfg.graph_id = "test-graph";
    EXPECT_TRUE(graph.compile(cfg));
    EXPECT_TRUE(graph.isCompiled());
}

TEST_F(ComputeGraphHeaderTest, ExecutePopulatesStats) {
    graph.addNode(makeNode("n1"));
    graph.addNode(makeNode("n2"));
    graph.addEdge(makeEdge("n1", "n2"));

    ComputeGraphConfig cfg;
    cfg.enable_fusion = true;
    ASSERT_TRUE(graph.compile(cfg));
    ASSERT_TRUE(graph.execute());

    const auto stats = graph.getStats();
    EXPECT_EQ(2u, stats.nodes_executed);
    EXPECT_GE(stats.total_execution_ms, 0.0);
    EXPECT_GT(stats.peak_memory_bytes, 0u);
}

TEST_F(ComputeGraphHeaderTest, ExecuteFailsWhenNotCompiled) {
    graph.addNode(makeNode("n1"));
    EXPECT_FALSE(graph.execute());
}

TEST_F(ComputeGraphHeaderTest, ToDotContainsEdges) {
    graph.addNode(makeNode("src"));
    graph.addNode(makeNode("dst"));
    graph.addEdge(makeEdge("src", "dst"));

    const std::string dot = graph.toDot();
    EXPECT_NE(std::string::npos, dot.find("src"));
    EXPECT_NE(std::string::npos, dot.find("dst"));
    EXPECT_NE(std::string::npos, dot.find("->"));
}

TEST_F(ComputeGraphHeaderTest, ResetClearsExecutionStats) {
    graph.addNode(makeNode("n1"));
    ComputeGraphConfig cfg;
    ASSERT_TRUE(graph.compile(cfg));
    ASSERT_TRUE(graph.execute());

    graph.reset();
    const auto stats = graph.getStats();
    EXPECT_EQ(0u, stats.nodes_executed);
    EXPECT_EQ(0.0, stats.total_execution_ms);
}

TEST_F(ComputeGraphHeaderTest, FactoryCreatesIndependentGraphInstances) {
    MockComputeGraphFactory factory;
    auto g1 = factory.create("cpu");
    auto g2 = factory.create("cpu");

    ASSERT_NE(nullptr, g1);
    ASSERT_NE(nullptr, g2);

    g1->addNode(makeNode("only_in_g1"));
    EXPECT_EQ(1u, g1->nodeCount());
    EXPECT_EQ(0u, g2->nodeCount());
}
