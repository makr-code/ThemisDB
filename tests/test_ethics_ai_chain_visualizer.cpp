#include <gtest/gtest.h>

#include "ethics_ai/argument_store.h"
#include "ethics_ai/chain_visualizer.h"
#include "ethics_ai/ethics_ai_types.h"

#include <memory>
#include <string>
#include <variant>

using namespace themis::plugins::ethics;

// =============================================================================
// Fixture
// =============================================================================

class ChainVisualizerTests : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_shared<ArgumentStore>();
        Status s = store_->initialize(nullptr, nullptr);
        ASSERT_TRUE(s.isOK()) << s.message;

        // arg_a  -- supports --> arg_b
        // arg_b  -- counters --> arg_c
        arg_a_.id                = "arg_a";
        arg_a_.philosophy_school = "kantian";
        arg_a_.argument_type     = ArgumentType::PRO;
        arg_a_.strength          = ArgumentStrength::MODERATE;
        arg_a_.content           = "Kantian PRO argument";
        arg_a_.supports.push_back("arg_b");

        arg_b_.id                = "arg_b";
        arg_b_.philosophy_school = "utilitarianism";
        arg_b_.argument_type     = ArgumentType::CONTRA;
        arg_b_.strength          = ArgumentStrength::STRONG;
        arg_b_.content           = "Utilitarian CONTRA argument";
        arg_b_.counterarguments.push_back("arg_c");

        arg_c_.id                = "arg_c";
        arg_c_.philosophy_school = "virtue_ethics";
        arg_c_.argument_type     = ArgumentType::SYNTHESIS;
        arg_c_.strength          = ArgumentStrength::DECISIVE;
        arg_c_.content           = "Virtue-ethics SYNTHESIS argument";

        store_->storeArgument(arg_a_, false);
        store_->storeArgument(arg_b_, false);
        store_->storeArgument(arg_c_, false);
    }

    std::shared_ptr<ArgumentStore> store_;
    EthicalArgument arg_a_, arg_b_, arg_c_;
};

// =============================================================================
// CV-01  exportDot with empty list returns minimal valid DOT
// =============================================================================

TEST_F(ChainVisualizerTests, CV01_ExportDotEmptyListReturnsMinimalGraph) {
    std::string dot = ChainVisualizer::exportDot({}, *store_);

    // Must start with "digraph" and end with "}"
    EXPECT_NE(dot.find("digraph"), std::string::npos);
    EXPECT_NE(dot.rfind('}'), std::string::npos);
    // No node lines (no quotes around node ids expected)
    EXPECT_EQ(dot.find("arg_a"), std::string::npos);
}

// =============================================================================
// CV-02  exportDot with single argument produces correct node
// =============================================================================

TEST_F(ChainVisualizerTests, CV02_ExportDotSingleNodePresent) {
    std::string dot = ChainVisualizer::exportDot({"arg_a"}, *store_);

    EXPECT_NE(dot.find("arg_a"), std::string::npos);
    // Node must have a label
    EXPECT_NE(dot.find("label="), std::string::npos);
    // No edge (only one node, no edges within the set)
    EXPECT_EQ(dot.find("->"), std::string::npos);
}

// =============================================================================
// CV-03  exportDot with two linked arguments produces an edge
// =============================================================================

TEST_F(ChainVisualizerTests, CV03_ExportDotEdgeBetweenLinkedNodes) {
    std::string dot = ChainVisualizer::exportDot({"arg_a", "arg_b"}, *store_);

    EXPECT_NE(dot.find("arg_a"), std::string::npos);
    EXPECT_NE(dot.find("arg_b"), std::string::npos);
    // Edge: arg_a supports arg_b
    EXPECT_NE(dot.find("->"), std::string::npos);
    EXPECT_NE(dot.find("supports"), std::string::npos);
}

// =============================================================================
// CV-04  exportDot node labels contain school name + type + strength
// =============================================================================

TEST_F(ChainVisualizerTests, CV04_ExportDotNodeLabelContainsMetadata) {
    std::string dot = ChainVisualizer::exportDot({"arg_a"}, *store_);

    // School name
    EXPECT_NE(dot.find("kantian"), std::string::npos);
    // Argument type token (PRO is typically "PRO" or "pro")
    EXPECT_TRUE(dot.find("PRO") != std::string::npos ||
                dot.find("pro") != std::string::npos);
    // Strength token
    EXPECT_TRUE(dot.find("MODERATE") != std::string::npos ||
                dot.find("moderate") != std::string::npos);
}

// =============================================================================
// CV-05  exportMermaid with empty list returns flowchart header
// =============================================================================

TEST_F(ChainVisualizerTests, CV05_ExportMermaidEmptyListReturnsHeader) {
    std::string mm = ChainVisualizer::exportMermaid({}, *store_);

    EXPECT_NE(mm.find("flowchart"), std::string::npos);
    // No node entries
    EXPECT_EQ(mm.find("arg_a"), std::string::npos);
}

// =============================================================================
// CV-06  exportMermaid with two linked arguments produces an edge
// =============================================================================

TEST_F(ChainVisualizerTests, CV06_ExportMermaidEdgeBetweenLinkedNodes) {
    std::string mm = ChainVisualizer::exportMermaid({"arg_a", "arg_b"}, *store_);

    EXPECT_NE(mm.find("arg_a"), std::string::npos);
    EXPECT_NE(mm.find("arg_b"), std::string::npos);
    // Edge arrow
    EXPECT_NE(mm.find("-->"), std::string::npos);
    EXPECT_NE(mm.find("supports"), std::string::npos);
}

// =============================================================================
// CV-07  chainToDot uses chain.argument_ids ordering
// =============================================================================

TEST_F(ChainVisualizerTests, CV07_ChainToDotUsesChainOrdering) {
    ArgumentChain chain;
    chain.id           = "chain_test";
    chain.argument_ids = {"arg_a", "arg_b", "arg_c"};
    chain.chain_type   = "pro";

    std::string dot = ChainVisualizer::chainToDot(chain, *store_);

    // All three nodes present
    EXPECT_NE(dot.find("arg_a"), std::string::npos);
    EXPECT_NE(dot.find("arg_b"), std::string::npos);
    EXPECT_NE(dot.find("arg_c"), std::string::npos);

    // arg_a -> arg_b (supports)
    EXPECT_NE(dot.find("supports"), std::string::npos);
    // arg_b -> arg_c (counters)
    EXPECT_NE(dot.find("counters"), std::string::npos);
}

// =============================================================================
// CV-08  Both exportDot and exportMermaid are deterministic
// =============================================================================

TEST_F(ChainVisualizerTests, CV08_ExportsDeterministic) {
    std::vector<std::string> ids = {"arg_a", "arg_b", "arg_c"};

    std::string dot1 = ChainVisualizer::exportDot(ids, *store_);
    std::string dot2 = ChainVisualizer::exportDot(ids, *store_);
    EXPECT_EQ(dot1, dot2);

    std::string mm1 = ChainVisualizer::exportMermaid(ids, *store_);
    std::string mm2 = ChainVisualizer::exportMermaid(ids, *store_);
    EXPECT_EQ(mm1, mm2);
}
