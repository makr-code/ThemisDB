/*
 * @file test_tensor_router.cpp
 * @brief TensorRouter unit tests: routing decisions and domain-tag wiring.
 *
 * Test IDs
 * --------
 * TR-01  decide(DataProfile) κ ≥ 1.7 + dim ≥ 256 → TENSOR_TRAIN
 * TR-02  decide(DataProfile) κ ≥ 1.3 → HYBRID
 * TR-03  decide(DataProfile) κ < 1.3 → HNSW
 * TR-04  route() with empty data → KEEP
 * TR-05  route() GEODATA category → LIFT
 * TR-06  route() RELATIONAL category → KEEP
 * TR-07  route() domain_tag + template catalog hit → LIFT
 * TR-08  route() domain_tag + no catalog wired → uses pilot heuristic
 * TR-09  setTemplateCatalog(nullptr) disables promotion
 * TR-10  templateCatalog() returns wired catalog
 * TR-11  template topology apply callback accessor set/clear/get
 * TR-12  callback invoked on domain_tag template hit
 * TR-13  callback false return falls back to heuristic path
 * TR-14  empty template graph → controlled fallback (no crash, no LIFT)
 * TR-15  template node mode_index out of range → topology mismatch fallback
 * TR-16  validateTemplate() API: valid / empty-graph / out-of-range / empty-modes / boundary
 */

#include "storage/tensor_router.h"
#include "tensor/hiss_structural_search.h"
#include "storage/tensor_network_storage_engine.h"

#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <vector>

namespace {

// Build a minimal in-memory engine for TensorRouter construction.
std::shared_ptr<themis::storage::TensorNetworkStorageEngine> makeEngine() {
    auto backend = std::make_shared<themis::storage::InMemoryTensorBackend>();
    themis::storage::TensorStorageConfig cfg;
    cfg.min_compression_ratio = 0.0;
    return std::make_shared<themis::storage::TensorNetworkStorageEngine>(
        backend, cfg);
}

// Minimal 8-element tensor data (constant → high compressibility).
std::vector<float> constantData(std::size_t n = 64, float v = 1.0f) {
    return std::vector<float>(n, v);
}

} // namespace

// ============================================================================
// TR-01  DataProfile routing: κ ≥ 1.7 + dim ≥ 256 → TENSOR_TRAIN
// ============================================================================
TEST(TensorRouterDecide, KappaHighLargeDim) {
    themis::storage::TensorRouter::DataProfile p;
    p.dim            = 512;
    p.num_vectors    = 1000;
    p.kappa_estimate = 2.0;
    EXPECT_EQ(themis::storage::TensorRouter::decide(p),
              themis::storage::TensorRouter::Route::TENSOR_TRAIN);
}

// ============================================================================
// TR-02  DataProfile routing: κ ≥ 1.3 → HYBRID
// ============================================================================
TEST(TensorRouterDecide, KappaMedium) {
    themis::storage::TensorRouter::DataProfile p;
    p.dim            = 128;
    p.kappa_estimate = 1.5;
    EXPECT_EQ(themis::storage::TensorRouter::decide(p),
              themis::storage::TensorRouter::Route::HYBRID);
}

// ============================================================================
// TR-03  DataProfile routing: κ < 1.3 → HNSW
// ============================================================================
TEST(TensorRouterDecide, KappaLow) {
    themis::storage::TensorRouter::DataProfile p;
    p.dim            = 64;
    p.kappa_estimate = 1.1;
    EXPECT_EQ(themis::storage::TensorRouter::decide(p),
              themis::storage::TensorRouter::Route::HNSW);
}

// ============================================================================
// TR-04  route() with empty data → KEEP
// ============================================================================
TEST(TensorRouterRoute, EmptyDataReturnsKeep) {
    themis::storage::TensorRouter router(makeEngine());
    auto d = router.route({}, {4, 4});
    EXPECT_EQ(d, themis::storage::TensorRouteDecision::KEEP);
}

// ============================================================================
// TR-05  route() GEODATA category → LIFT
// ============================================================================
TEST(TensorRouterRoute, GeoDataForceLifts) {
    themis::storage::TensorRouter router(makeEngine());
    themis::storage::TensorRouteHint hint;
    hint.category = themis::storage::TensorRouteHint::DataCategory::GEODATA;
    auto d = router.route(constantData(64), {8, 8}, hint);
    EXPECT_EQ(d, themis::storage::TensorRouteDecision::LIFT);
}

// ============================================================================
// TR-06  route() RELATIONAL category → KEEP
// ============================================================================
TEST(TensorRouterRoute, RelationalForceKeep) {
    themis::storage::TensorRouter router(makeEngine());
    themis::storage::TensorRouteHint hint;
    hint.category = themis::storage::TensorRouteHint::DataCategory::RELATIONAL;
    auto d = router.route(constantData(64), {8, 8}, hint);
    EXPECT_EQ(d, themis::storage::TensorRouteDecision::KEEP);
}

// ============================================================================
// TR-07  route() domain_tag + template catalog hit → LIFT
// ============================================================================
TEST(TensorRouterRoute, DomainTagTemplateCatalogPromotesLift) {
    themis::storage::TensorRouter router(makeEngine());
    std::atomic<int> apply_calls{0};

    // Build and wire a TemplateCatalog with "finance" template
    auto catalog = std::make_shared<themis::tensor::TemplateCatalog>();
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n0", 0, 1, 2, 4, 0.0});
    catalog->registerTemplate("finance", g);
    router.setTemplateCatalog(catalog);
    router.setTemplateTopologyApplyFn(
        [&apply_calls](const std::string&                    domain,
                       const themis::tensor::TensorNetworkGraph& topology,
                       const themis::storage::TensorRouteHint&) {
            if (!domain.empty() && topology.nodeCount() > 0) {
                ++apply_calls;
                return true;
            }
            return false;
        });

    // Provide hint with matching domain_tag and low-compressibility data
    themis::storage::TensorRouteHint hint;
    hint.domain_tag = "finance";
    // Use random data (low κ → would be KEEP without catalog)
    std::vector<float> rand_data(64);
    for (std::size_t i = 0; i < 64; ++i) {
      rand_data[i] = static_cast<float>(i % 7);
    }

    auto d = router.route(rand_data, {8, 8}, hint);
    // Catalog hit should promote to LIFT
    EXPECT_EQ(d, themis::storage::TensorRouteDecision::LIFT);
    EXPECT_EQ(apply_calls.load(), 1);
}

// ============================================================================
// TR-08  route() domain_tag set but no catalog → falls through to heuristic
// ============================================================================
TEST(TensorRouterRoute, DomainTagNoCatalogFallsToHeuristic) {
    themis::storage::TensorRouter router(makeEngine());

    themis::storage::TensorRouteHint hint;
    hint.domain_tag = "finance";
    hint.category   = themis::storage::TensorRouteHint::DataCategory::RELATIONAL;
    // Relational category override is KEEP regardless
    auto d = router.route(constantData(64), {8, 8}, hint);
    EXPECT_EQ(d, themis::storage::TensorRouteDecision::KEEP);
}

// ============================================================================
// TR-09  setTemplateCatalog(nullptr) disables promotion
// ============================================================================
TEST(TensorRouterRoute, NullCatalogDisablesPromotion) {
    themis::storage::TensorRouter router(makeEngine());

    auto catalog = std::make_shared<themis::tensor::TemplateCatalog>();
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n0", 0, 1, 2, 4, 0.0});
    catalog->registerTemplate("finance", g);
    router.setTemplateCatalog(catalog);
    router.setTemplateCatalog(nullptr);  // Disable

    EXPECT_EQ(router.templateCatalog(), nullptr);
}

TEST(TensorRouterRoute, DomainTagWithoutTopologyApplyCallbackDoesNotForceLift) {
    themis::storage::TensorRouter router(makeEngine());

    auto catalog = std::make_shared<themis::tensor::TemplateCatalog>();
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n0", 0, 1, 2, 4, 0.0});
    catalog->registerTemplate("finance", g);
    router.setTemplateCatalog(catalog);

    themis::storage::TensorRouteHint hint;
    hint.domain_tag = "finance";
    hint.min_ratio = 10.0;  // Force heuristic to avoid LIFT/HYBRID fallback.
    std::vector<float> rand_data(64);
    for (std::size_t i = 0; i < 64; ++i) {
        rand_data[i] = static_cast<float>((i * 37) % 101) * 0.01f;
    }

    const auto d = router.route(rand_data, {8, 8}, hint);
    EXPECT_NE(d, themis::storage::TensorRouteDecision::LIFT);
}

// ============================================================================
// TR-10  templateCatalog() returns the wired catalog
// ============================================================================
TEST(TensorRouterRoute, TemplateCatalogAccessor) {
    themis::storage::TensorRouter router(makeEngine());
    EXPECT_EQ(router.templateCatalog(), nullptr);
    EXPECT_FALSE(router.hasTemplateTopologyApplyFn());

    auto catalog = std::make_shared<themis::tensor::TemplateCatalog>();
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n0", 0, 1, 2, 4, 0.0});
    catalog->registerTemplate("template-check", g);
    router.setTemplateCatalog(catalog);
    EXPECT_EQ(router.templateCatalog().get(), catalog.get());
    std::atomic<int> calls{0};
    router.setTemplateTopologyApplyFn(
        [&calls](const std::string&,
                 const themis::tensor::TensorNetworkGraph&,
                 const themis::storage::TensorRouteHint&) {
            ++calls;
            return true;
        });
    EXPECT_TRUE(router.hasTemplateTopologyApplyFn());
    themis::storage::TensorRouteHint hint;
    hint.domain_tag = "template-check";
    (void)router.route(constantData(64), {8, 8}, hint);
    EXPECT_GE(calls.load(), 1);
    router.clearTemplateTopologyApplyFn();
    EXPECT_FALSE(router.hasTemplateTopologyApplyFn());
}

// ============================================================================
// TR-11  template topology apply callback accessor set/clear/get
// ============================================================================
TEST(TensorRouterRoute, TemplateTopologyApplyCallbackAccessor) {
    themis::storage::TensorRouter router(makeEngine());
    router.clearTemplateTopologyApplyFn();
    EXPECT_FALSE(static_cast<bool>(router.getTemplateTopologyApplyFn()));

    router.setTemplateTopologyApplyFn(
        [](const std::string&,
           const themis::tensor::TensorNetworkGraph&,
           const themis::storage::TensorRouteHint&) {
            return true;
        });
    EXPECT_TRUE(static_cast<bool>(router.getTemplateTopologyApplyFn()));
    router.clearTemplateTopologyApplyFn();
    EXPECT_FALSE(static_cast<bool>(router.getTemplateTopologyApplyFn()));
}

// ============================================================================
// TR-12  callback invoked on domain_tag template hit
// ============================================================================
TEST(TensorRouterRoute, TemplateTopologyApplyCallbackInvokedOnHit) {
    themis::storage::TensorRouter router(makeEngine());

    auto catalog = std::make_shared<themis::tensor::TemplateCatalog>();
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n0", 0, 1, 2, 4, 0.0});
    catalog->registerTemplate("finance", g);
    router.setTemplateCatalog(catalog);

    int call_count = 0;
    router.setTemplateTopologyApplyFn(
        [&call_count](const std::string& domain_tag,
                      const themis::tensor::TensorNetworkGraph& graph,
                      const themis::storage::TensorRouteHint&) {
            ++call_count;
            EXPECT_EQ(domain_tag, "finance");
            EXPECT_GE(graph.nodeCount(), 1u);
            return true;
        });

    themis::storage::TensorRouteHint hint;
    hint.domain_tag = "finance";
    hint.category   = themis::storage::TensorRouteHint::DataCategory::EMBEDDING;
    const auto d = router.route(constantData(64), {8, 8}, hint);
    EXPECT_EQ(d, themis::storage::TensorRouteDecision::LIFT);
    EXPECT_EQ(call_count, 1);
}

// ============================================================================
// TR-13  callback false return falls back to heuristic path
// ============================================================================
TEST(TensorRouterRoute, TemplateTopologyApplyCallbackFalseFallsBackToHeuristic) {
    themis::storage::TensorRouter router(makeEngine());

    auto catalog = std::make_shared<themis::tensor::TemplateCatalog>();
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n0", 0, 1, 2, 4, 0.0});
    catalog->registerTemplate("finance", g);
    router.setTemplateCatalog(catalog);

    int call_count = 0;
    router.setTemplateTopologyApplyFn(
        [&call_count](const std::string&,
                      const themis::tensor::TensorNetworkGraph&,
                      const themis::storage::TensorRouteHint&) {
            ++call_count;
            return false;
        });

    // Callback returns false -> no forced LIFT; decision falls back to heuristic.
    themis::storage::TensorRouteHint hint;
    hint.domain_tag = "finance";
    std::vector<float> rand_data(64);
    for (std::size_t i = 0; i < 64; ++i) {
      rand_data[i] = static_cast<float>(i % 7);
    }

    const auto d = router.route(rand_data, {8, 8}, hint);
    EXPECT_NE(d, themis::storage::TensorRouteDecision::LIFT);
    EXPECT_EQ(call_count, 1);
}

// ============================================================================
// TR-14  empty template graph → controlled fallback (no crash, no LIFT)
// ============================================================================
TEST(TensorRouterRoute, EmptyTemplateGraphFallsBackToHeuristic) {
    themis::storage::TensorRouter router(makeEngine());

    // Register an empty graph (no nodes) for the domain "empty-domain".
    auto catalog = std::make_shared<themis::tensor::TemplateCatalog>();
    themis::tensor::TensorNetworkGraph empty_graph;  // 0 nodes
    catalog->registerTemplate("empty-domain", empty_graph);
    router.setTemplateCatalog(catalog);

    // Even with an apply callback that would return true, an invalid (empty)
    // template must not promote to LIFT — validation must reject it first.
    int apply_calls = 0;
    router.setTemplateTopologyApplyFn(
        [&apply_calls](const std::string&,
                       const themis::tensor::TensorNetworkGraph&,
                       const themis::storage::TensorRouteHint&) {
            ++apply_calls;
            return true;
        });

    themis::storage::TensorRouteHint hint;
    hint.domain_tag = "empty-domain";
    hint.min_ratio  = 100.0;  // Force heuristic to KEEP if no template LIFT.

    const auto d = router.route(constantData(64), {8, 8}, hint);

    // The apply_fn must NOT be called for an invalid template.
    EXPECT_EQ(apply_calls, 0);
    // Without a valid template the routing falls through to heuristic → KEEP.
    EXPECT_NE(d, themis::storage::TensorRouteDecision::LIFT);
}

// ============================================================================
// TR-15  template node mode_index out of range → topology mismatch fallback
// ============================================================================
TEST(TensorRouterRoute, TemplateTopologyMismatchFallsBackToHeuristic) {
    themis::storage::TensorRouter router(makeEngine());

    // Graph has a node with mode_index=99, which is out of range for {8, 8}
    // (only 2 modes, indices 0 and 1 are valid).
    auto catalog = std::make_shared<themis::tensor::TemplateCatalog>();
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n0", 99, 1, 2, 4, 0.0});  // mode_index=99 exceeds mode count=2
    catalog->registerTemplate("mismatch-domain", g);
    router.setTemplateCatalog(catalog);

    int apply_calls = 0;
    router.setTemplateTopologyApplyFn(
        [&apply_calls](const std::string&,
                       const themis::tensor::TensorNetworkGraph&,
                       const themis::storage::TensorRouteHint&) {
            ++apply_calls;
            return true;
        });

    themis::storage::TensorRouteHint hint;
    hint.domain_tag = "mismatch-domain";
    hint.min_ratio  = 100.0;  // Force heuristic to KEEP if no template LIFT.

    const auto d = router.route(constantData(64), {8, 8}, hint);

    // apply_fn must NOT be invoked when topology validation rejects the graph.
    EXPECT_EQ(apply_calls, 0);
    EXPECT_NE(d, themis::storage::TensorRouteDecision::LIFT);
}

// ============================================================================
// TR-16  validateTemplate() API: valid template
// ============================================================================
TEST(TensorRouterValidate, ValidTemplateReturnsValid) {
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n0", 0, 1, 2, 4, 0.0});
    g.addNode({"n1", 1, 2, 1, 4, 0.0});

    const auto result =
        themis::storage::TensorRouter::validateTemplate(g, {4, 4});

    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.reason.empty());
}

// ============================================================================
// TR-16b  validateTemplate(): empty graph is invalid
// ============================================================================
TEST(TensorRouterValidate, EmptyGraphIsInvalid) {
    themis::tensor::TensorNetworkGraph g;  // no nodes

    const auto result =
        themis::storage::TensorRouter::validateTemplate(g, {4, 4});

    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.reason.empty());
}

// ============================================================================
// TR-16c  validateTemplate(): mode_index out of range is invalid
// ============================================================================
TEST(TensorRouterValidate, NodeModeIndexOutOfRangeIsInvalid) {
    themis::tensor::TensorNetworkGraph g;
    // mode_index=5, but mode_sizes only has 2 entries → out of range
    g.addNode({"n0", 5, 1, 2, 4, 0.0});

    const auto result =
        themis::storage::TensorRouter::validateTemplate(g, {4, 4});

    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.reason.empty());
}

// ============================================================================
// TR-16d  validateTemplate(): empty mode_sizes → cannot validate
// ============================================================================
TEST(TensorRouterValidate, EmptyModeSizesIsInvalid) {
    themis::tensor::TensorNetworkGraph g;
    g.addNode({"n0", 0, 1, 2, 4, 0.0});

    const auto result =
        themis::storage::TensorRouter::validateTemplate(g, {});

    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.reason.empty());
}

// ============================================================================
// TR-16e  validateTemplate(): valid single-node template (boundary check)
// ============================================================================
TEST(TensorRouterValidate, SingleNodeExactlyAtModeBoundaryIsValid) {
    themis::tensor::TensorNetworkGraph g;
    // mode_index=1 with mode_sizes of size 2 → last valid index
    g.addNode({"n0", 1, 1, 2, 4, 0.0});

    const auto result =
        themis::storage::TensorRouter::validateTemplate(g, {8, 8});

    EXPECT_TRUE(result.valid);
}
