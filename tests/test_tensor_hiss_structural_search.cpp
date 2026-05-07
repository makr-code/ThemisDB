/*
 * @file test_tensor_hiss_structural_search.cpp
 * @brief Phase-6 tensorgraph tests.
 */

#include "tensor/hiss_structural_search.h"

#include "storage/tensor_train_decomposer.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace {

themis::storage::TTTrain makeSmallTrain() {
    themis::storage::TTTrain t;
    t.mode_sizes = {4, 4, 4};
    t.cores.resize(3);

    t.cores[0].r_left = 1;
    t.cores[0].n = 4;
    t.cores[0].r_right = 3;
    t.cores[0].data.assign(1 * 4 * 3, 0.1f);
    t.cores[0].data[0] = 4.0f;

    t.cores[1].r_left = 3;
    t.cores[1].n = 4;
    t.cores[1].r_right = 2;
    t.cores[1].data.assign(3 * 4 * 2, 0.2f);
    t.cores[1].data[5] = 5.0f;

    t.cores[2].r_left = 2;
    t.cores[2].n = 4;
    t.cores[2].r_right = 1;
    t.cores[2].data.assign(2 * 4 * 1, 0.3f);
    t.cores[2].data[3] = 6.0f;
    return t;
}

} // namespace

TEST(TensorHissSearch, GraphAddNodeEdgeAndNeighbors) {
    themis::tensor::TensorNetworkGraph g;
    const auto a = g.addNode({"a", 0, 1, 2, 4, 0.1});
    const auto b = g.addNode({"b", 1, 2, 1, 4, 0.2});
    EXPECT_TRUE(g.addEdge({a, b, 1.0, "chain"}));
    EXPECT_FALSE(g.addEdge({a, b, 1.0, "chain"}));
    EXPECT_EQ(g.nodeCount(), 2u);
    EXPECT_EQ(g.edgeCount(), 1u);
    const auto n = g.neighbors(a);
    ASSERT_EQ(n.size(), 1u);
    EXPECT_EQ(n[0], b);
}

TEST(TensorHissSearch, GraphRerouteEdge) {
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"a", 0, 1, 2, 4, 0.1});
    g.addNode({"b", 1, 2, 1, 4, 0.2});
    ASSERT_TRUE(g.addEdge({0, 1, 1.0, "chain"}));
    EXPECT_TRUE(g.rerouteEdge(0, 1, "reshaped"));
    EXPECT_EQ(g.edges().front().topology, "reshaped");
}

TEST(TensorHissSearch, HissSearchBuildsDeterministicGraph) {
    const auto train = makeSmallTrain();
    themis::tensor::HissConfig cfg;
    cfg.entropy_threshold = 0.1;
    cfg.max_reshape_depth = 2;
    cfg.diversity_budget = 4;
    cfg.random_seed = 1234;

    themis::tensor::HissStructuralSearchEngine engine;
    const auto g1 = engine.search(train, cfg);
    const auto g2 = engine.search(train, cfg);
    EXPECT_EQ(g1.nodeCount(), train.cores.size());
    EXPECT_EQ(g1.edgeCount(), g2.edgeCount());
    ASSERT_EQ(g1.edges().size(), g2.edges().size());
    for (std::size_t i = 0; i < g1.edges().size(); ++i) {
        EXPECT_EQ(g1.edges()[i].from, g2.edges()[i].from);
        EXPECT_EQ(g1.edges()[i].to, g2.edges()[i].to);
        EXPECT_EQ(g1.edges()[i].topology, g2.edges()[i].topology);
        EXPECT_FLOAT_EQ(static_cast<float>(g1.edges()[i].weight), static_cast<float>(g2.edges()[i].weight));
    }
}

TEST(TensorHissSearch, TemplateCatalogRegisterLookup) {
    themis::tensor::TemplateCatalog c;
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n", 0, 1, 1, 8, 0.0});
    c.registerTemplate("finance", g);
    auto found = c.lookup("finance");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->nodeCount(), 1u);
    EXPECT_EQ(c.size(), 1u);
    EXPECT_FALSE(c.lookup("unknown").has_value());
}

TEST(TensorHissSearch, HissReshaperExposeQuanticsPassthrough) {
    const auto train = makeSmallTrain();
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {2, 4, 8});
    EXPECT_EQ(qt.bit_depths.size(), 3u);
    EXPECT_EQ(qt.bit_depths[0], 1u);
    EXPECT_EQ(qt.bit_depths[1], 2u);
    EXPECT_EQ(qt.bit_depths[2], 3u);
    EXPECT_EQ(qt.toTTTrain().cores.size(), train.cores.size());
}

TEST(TensorHissSearch, HissReshaperInfersBitDepthsFromTrainModes) {
    const auto train = makeSmallTrain();
    const auto qt = themis::tensor::HissReshaper::exposeQuantics(train, {});
    ASSERT_EQ(qt.bit_depths.size(), 3u);
    EXPECT_EQ(qt.bit_depths[0], 2u);
    EXPECT_EQ(qt.bit_depths[1], 2u);
    EXPECT_EQ(qt.bit_depths[2], 2u);
}

TEST(TensorHissSearch, HissReshaperRejectsMismatchedGridSizeCount) {
    const auto train = makeSmallTrain();
    EXPECT_THROW(themis::tensor::HissReshaper::exposeQuantics(train, {4, 4}), std::invalid_argument);
}
