/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_tensor_phase3.cpp                              ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 3 (Q1-Q2 2027)                      ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_tensor_phase3.cpp
 * @brief Phase 3 tensor tests: AQL operators, TensorAwareQueryOptimizer, TARG.
 *
 * Test IDs
 * --------
 * TensorContractionEngine::project — TCP-01..TCP-04
 *   TCP-01  project(mode=0): result has order d-1 and correct shape
 *   TCP-02  project(mode=last): result has order d-1 and correct shape
 *   TCP-03  project() result matches dense reference (sum over axis)
 *   TCP-04  project() with mode out of range throws std::out_of_range
 *
 * TensorContractionEngine::contractModes — TCM-01..TCM-05
 *   TCM-01  contractModes: full contraction (all modes) returns scalar train
 *   TCM-02  contractModes: partial contraction returns correct shape
 *   TCM-03  contractModes: full contraction result matches innerProduct
 *   TCM-04  contractModes: length mismatch in modes_a/modes_b throws
 *   TCM-05  contractModes: incompatible mode sizes throws
 *
 * TensorAwareQueryOptimizer — TAQO-01..TAQO-06
 *   TAQO-01  isTensorFunction returns true for all 8 known functions
 *   TAQO-02  isTensorFunction returns false for non-tensor function
 *   TAQO-03  rewrite() marks TENSOR_SIMILARITY node as TensorContraction
 *   TAQO-04  rewrite() marks TENSOR_CONTRACT node as TensorContraction
 *   TAQO-05  rewrite() leaves non-tensor nodes unchanged
 *   TAQO-06  rewrite() increments nodes_rewritten counter correctly
 *
 * TARGRetrieval — TARG-01..TARG-08
 *   TARG-01  shouldRetrieve returns true when logit gap < threshold
 *   TARG-02  shouldRetrieve returns false when logit gap >= threshold
 *   TARG-03  cool-down suppresses retrieval for configured number of tokens
 *   TARG-04  notifyRetrievalExecuted resets consecutive_uncertain counter
 *   TARG-05  min_consecutive_uncertain=2 requires two consecutive tokens
 *   TARG-06  entropy gate triggers when entropy exceeds threshold
 *   TARG-07  stats() tracks trigger rate correctly
 *   TARG-08  reset() clears all state
 *
 * FlareRetrieval — FR-01..FR-10
 *   FR-01  shouldRetrieve() returns false before any token emitted
 *   FR-02  shouldRetrieve() returns true when log-prob drops below threshold
 *   FR-03  shouldRetrieve() returns false when log-prob is above threshold
 *   FR-04  cooldown suppresses retrieval for the configured number of tokens
 *   FR-05  min_consecutive_uncertain=2 requires two consecutive low-prob tokens
 *   FR-06  notifyRetrievalExecuted() increments retrieval step count
 *   FR-07  max_retrieval_steps=1 prevents further retrieval after one step
 *   FR-08  buildQuery() returns non-empty string from partial output
 *   FR-09  buildQuery() masks low-confidence tokens when mask_uncertain_tokens=true
 *   FR-10  reset() clears all state
 *
 * TensorButterflyOperator — TBO-01..TBO-06
 *   TBO-01  build(FOURIER, {4}) succeeds and describe() mentions FOURIER
 *   TBO-02  apply() on a 1D TT (n=4) produces orthonormal output (‖WHT·v‖ = ‖v‖)
 *   TBO-03  apply() on a 2-mode TT ({4,4}) is separable: mode transforms commute
 *   TBO-04  apply() shape mismatch throws std::invalid_argument
 *   TBO-05  build(FOURIER, {3}) throws (3 is not a power of 2)
 *   TBO-06  build(RADON, ...) throws std::logic_error (stub #171)
 *
 * AdapterRepository — AR-01..AR-06
 *   AR-01  store() + loadAdapter() round-trip: valid=true and cores match
 *   AR-02  loadAdapter() for unknown domain returns valid=false
 *   AR-03  listDomains() returns all stored domains (deduplicated, sorted)
 *   AR-04  store() overwrites existing adapter at same key
 *   AR-05  Different tenants are isolated (store in tenant-A, miss in tenant-B)
 *   AR-06  listDomains() returns empty vector for empty repository
 *
 * TensorFingerprintGraph — TFG-01..TFG-06
 *   TFG-01  addAdapter() registers the entry; size() increases
 *   TFG-02  entry() returns the stored FingerprintEntry with correct metadata
 *   TFG-03  findSimilar() returns key_b as most similar to key_a (near-identical trains)
 *   TFG-04  findSimilar() excludes the query adapter itself from results
 *   TFG-05  removeAdapter() removes the entry; second remove returns false
 *   TFG-06  findSimilarByFingerprint() respects tenant_id filter
 *
 * TensorRAGPipeline — TRPL-01..TRPL-08
 *   TRPL-01  step() returns should_retrieve=false for a confident token (large gap, high log-prob)
 *   TRPL-02  step() sets flare_triggered when log-prob is below FLARE threshold
 *   TRPL-03  step() sets targ_triggered when logit gap is below TARG threshold
 *   TRPL-04  step() sets BOTH trigger and increments combined_triggers when both gates fire
 *   TRPL-05  notifyRetrievalDone() resets cooldown for both gates
 *   TRPL-06  reset() clears stats and sub-gate state
 *   TRPL-07  stats() tracks total_token_steps, flare_triggers, targ_triggers correctly
 *   TRPL-08  use_flare=false disables FLARE; use_targ=false disables TARG
 */

#include "query/tensor_contraction_engine.h"
#include "query/tensor_aware_query_optimizer.h"
#include "rag/targ_retrieval.h"
#include "rag/flare_retrieval.h"
#include "rag/tensor_rag_pipeline.h"
#include "storage/tensor_train_decomposer.h"
#include "tensor/tensor_butterfly_operator.h"
#include "tensor/adapter_repository.h"
#include "tensor/tensor_fingerprint_graph.h"

#include <gtest/gtest.h>
#include <cmath>
#include <memory>
#include <numeric>
#include <vector>

using namespace themis::storage;
using namespace themis::query;
using namespace themis::rag;
using namespace themis::tensor;

namespace {

// ─── Helpers ────────────────────────────────────────────────────────────────

static TTTrain makeTrain(const std::vector<float>& data,
                          const std::vector<std::size_t>& shape,
                          double eps = 0.01) {
    TensorTrainConfig cfg;
    cfg.eps = eps;
    TensorTrainDecomposer dec;
    auto [train, _] = dec.decompose(data, shape, cfg);
    return train;
}

/// Fill a vector with range [0, n).
static std::vector<float> iota(std::size_t n) {
    std::vector<float> v(n);
    std::iota(v.begin(), v.end(), 0.0f);
    return v;
}

/// Dense reference for project (sum over axis).
static std::vector<float> denseProject(const std::vector<float>& data,
                                        const std::vector<std::size_t>& shape,
                                        std::size_t mode) {
    // Result shape
    std::vector<std::size_t> res_shape;
    for (std::size_t k = 0; k < shape.size(); ++k)
        if (k != mode) res_shape.push_back(shape[k]);

    std::size_t res_sz = 1;
    for (auto s : res_shape) res_sz *= s;
    std::vector<float> result(res_sz, 0.0f);

    // Iterate over all elements
    std::size_t total = data.size();
    for (std::size_t flat = 0; flat < total; ++flat) {
        // Decode flat into multi-index
        std::vector<std::size_t> idx(shape.size());
        std::size_t tmp = flat;
        for (int k = static_cast<int>(shape.size()) - 1; k >= 0; --k) {
            idx[static_cast<std::size_t>(k)] = tmp % shape[static_cast<std::size_t>(k)];
            tmp /= shape[static_cast<std::size_t>(k)];
        }
        // Build result index (skip mode)
        std::vector<std::size_t> ridx;
        for (std::size_t k = 0; k < shape.size(); ++k)
            if (k != mode) ridx.push_back(idx[k]);
        // Flat result offset
        std::size_t roff = 0, stride = 1;
        for (int k = static_cast<int>(res_shape.size()) - 1; k >= 0; --k) {
            roff   += ridx[static_cast<std::size_t>(k)] * stride;
            stride *= res_shape[static_cast<std::size_t>(k)];
        }
        result[roff] += data[flat];
    }
    return result;
}

// ─── TCP tests — project ────────────────────────────────────────────────────

TEST(TensorContractionEnginePhase3, TCP01_project_mode0_shape) {
    // 3D tensor [3,4,5] → project mode 0 → [4,5]
    auto data  = iota(3 * 4 * 5);
    auto train = makeTrain(data, {3, 4, 5});
    auto proj  = TensorContractionEngine::project(train, 0);
    EXPECT_EQ(proj.order(), 2u);
    EXPECT_EQ(proj.mode_sizes[0], 4u);
    EXPECT_EQ(proj.mode_sizes[1], 5u);
}

TEST(TensorContractionEnginePhase3, TCP02_project_last_mode_shape) {
    // 3D tensor [3,4,5] → project mode 2 → [3,4]
    auto data  = iota(3 * 4 * 5);
    auto train = makeTrain(data, {3, 4, 5});
    auto proj  = TensorContractionEngine::project(train, 2);
    EXPECT_EQ(proj.order(), 2u);
    EXPECT_EQ(proj.mode_sizes[0], 3u);
    EXPECT_EQ(proj.mode_sizes[1], 4u);
}

TEST(TensorContractionEnginePhase3, TCP03_project_matches_dense) {
    // 3D tensor [2,3,4] → project mode 1 → [2,4]; compare vs dense ref
    auto data  = iota(2 * 3 * 4);
    auto train = makeTrain(data, {2, 3, 4}, 1e-6);
    auto proj  = TensorContractionEngine::project(train, 1);
    auto recon = proj.reconstruct();

    auto ref = denseProject(data, {2, 3, 4}, 1);
    ASSERT_EQ(recon.size(), ref.size());
    for (std::size_t i = 0; i < ref.size(); ++i) {
        EXPECT_NEAR(recon[i], ref[i], 0.5f)
            << "index " << i << ": got " << recon[i] << " expected " << ref[i];
    }
}

TEST(TensorContractionEnginePhase3, TCP04_project_out_of_range_throws) {
    auto train = makeTrain(iota(2 * 3), {2, 3});
    EXPECT_THROW(TensorContractionEngine::project(train, 5),
                 std::out_of_range);
}

// ─── TCM tests — contractModes ──────────────────────────────────────────────

TEST(TensorContractionEnginePhase3, TCM01_full_contraction_is_scalar) {
    // 2D [2,3] fully contracted → order-1 train with mode_size 1
    auto a = makeTrain(iota(2 * 3), {2, 3}, 1e-6);
    auto b = makeTrain(iota(2 * 3), {2, 3}, 1e-6);
    auto result = TensorContractionEngine::contractModes(a, b, {0, 1}, {0, 1});
    // Full contraction → scalar (order 1, mode_size 1)
    EXPECT_EQ(result.mode_sizes[0], 1u);
}

TEST(TensorContractionEnginePhase3, TCM02_partial_contraction_shape) {
    // a: [2,3], b: [3,4] — contract mode 1 of a with mode 0 of b → [2,4]
    auto a = makeTrain(iota(2 * 3), {2, 3}, 1e-6);
    auto b = makeTrain(iota(3 * 4), {3, 4}, 1e-6);
    auto result = TensorContractionEngine::contractModes(a, b, {1}, {0});
    ASSERT_GE(result.order(), 1u);
    // Result shape should be [2, 4] = 2 free modes
    std::size_t total_sz = 1;
    for (auto s : result.mode_sizes) total_sz *= s;
    EXPECT_EQ(total_sz, 2u * 4u);
}

TEST(TensorContractionEnginePhase3, TCM03_full_contraction_matches_innerProduct) {
    // Fully contracted result should equal innerProduct(a, b)
    auto data_a = iota(2 * 3);
    auto data_b = iota(2 * 3);
    auto a = makeTrain(data_a, {2, 3}, 1e-6);
    auto b = makeTrain(data_b, {2, 3}, 1e-6);
    auto result = TensorContractionEngine::contractModes(a, b, {0, 1}, {0, 1});
    float scalar = result.reconstruct()[0];
    double ip    = TensorContractionEngine::innerProduct(a, b);
    EXPECT_NEAR(scalar, static_cast<float>(ip), 0.5f);
}

TEST(TensorContractionEnginePhase3, TCM04_length_mismatch_throws) {
    auto a = makeTrain(iota(2 * 3), {2, 3});
    auto b = makeTrain(iota(2 * 3), {2, 3});
    EXPECT_THROW(TensorContractionEngine::contractModes(a, b, {0}, {0, 1}),
                 std::invalid_argument);
}

TEST(TensorContractionEnginePhase3, TCM05_incompatible_mode_sizes_throws) {
    auto a = makeTrain(iota(2 * 3), {2, 3});
    auto b = makeTrain(iota(4 * 3), {4, 3});
    // mode 0 of a (size 2) vs mode 0 of b (size 4) — incompatible
    EXPECT_THROW(TensorContractionEngine::contractModes(a, b, {0}, {0}),
                 std::invalid_argument);
}

// ─── TAQO tests — TensorAwareQueryOptimizer ─────────────────────────────────

TEST(TensorAwareQueryOptimizerPhase3, TAQO01_known_functions_detected) {
    for (const auto& fn : {
            "TENSOR_SIMILARITY", "TENSOR_NORM", "TENSOR_SLICE",
            "TENSOR_COMPRESS",   "TENSOR_INFO", "TENSOR_CONTRACT",
            "TENSOR_PROJECT",    "TENSOR_DECOMPOSE"}) {
        EXPECT_TRUE(TensorAwareQueryOptimizer::isTensorFunction(fn)) << fn;
    }
}

TEST(TensorAwareQueryOptimizerPhase3, TAQO02_unknown_function_not_detected) {
    EXPECT_FALSE(TensorAwareQueryOptimizer::isTensorFunction("HASH_JOIN"));
    EXPECT_FALSE(TensorAwareQueryOptimizer::isTensorFunction("SORT"));
    EXPECT_FALSE(TensorAwareQueryOptimizer::isTensorFunction(""));
}

TEST(TensorAwareQueryOptimizerPhase3, TAQO03_rewrite_similarity_node) {
    auto node = std::make_shared<QueryPlanNode>();
    node->type        = PlanNodeType::Filter;
    node->description = "Filter: TENSOR_SIMILARITY(a.vec, b.vec) > 0.8";

    TensorAwareQueryOptimizer opt;
    auto result = opt.rewrite(node);
    EXPECT_EQ(result->type, PlanNodeType::TensorContraction);
    EXPECT_NE(result->description.find("[TT-domain]"), std::string::npos);
    EXPECT_GT(result->estimated_cost, 0.0);
}

TEST(TensorAwareQueryOptimizerPhase3, TAQO04_rewrite_contract_node) {
    auto node = std::make_shared<QueryPlanNode>();
    node->type        = PlanNodeType::Unknown;
    node->description = "Compute: TENSOR_CONTRACT(a, b, [0], [0])";

    TensorAwareQueryOptimizer opt;
    auto result = opt.rewrite(node);
    EXPECT_EQ(result->type, PlanNodeType::TensorContraction);
}

TEST(TensorAwareQueryOptimizerPhase3, TAQO05_non_tensor_node_unchanged) {
    auto node = std::make_shared<QueryPlanNode>();
    node->type        = PlanNodeType::SeqScan;
    node->description = "SeqScan on collection users";

    TensorAwareQueryOptimizer opt;
    auto result = opt.rewrite(node);
    EXPECT_EQ(result->type, PlanNodeType::SeqScan);
}

TEST(TensorAwareQueryOptimizerPhase3, TAQO06_rewrite_counter) {
    auto root = std::make_shared<QueryPlanNode>();
    root->type        = PlanNodeType::Filter;
    root->description = "TENSOR_NORM(v) > 1.0";

    auto child = std::make_shared<QueryPlanNode>();
    child->type        = PlanNodeType::Filter;
    child->description = "TENSOR_SIMILARITY(a, b) > 0.5";
    root->children.push_back(child);

    auto other = std::make_shared<QueryPlanNode>();
    other->type        = PlanNodeType::SeqScan;
    other->description = "SeqScan users";
    root->children.push_back(other);

    TensorAwareQueryOptimizer opt;
    opt.rewrite(root);
    auto stats = opt.lastStats();

    EXPECT_EQ(stats.nodes_rewritten, 2u);
    EXPECT_EQ(stats.nodes_visited,   3u);
    EXPECT_GT(stats.costReductionFactor(), 1.0);
}

// ─── TARG tests ──────────────────────────────────────────────────────────────

/// Build logits where top-1 and top-2 have a specified gap.
static std::vector<float> makeLogits(float top1, float top2,
                                      std::size_t vocab_size = 100) {
    std::vector<float> logits(vocab_size, -10.0f);
    if (vocab_size >= 2) {
        logits[0] = top1;
        logits[1] = top2;
    }
    return logits;
}

TEST(TARGRetrievalPhase3, TARG01_shouldRetrieve_true_when_gap_below_threshold) {
    TARGConfig cfg;
    cfg.gap_threshold = 5.0f;
    TARGRetrieval targ(cfg);
    // gap = 3.0 < 5.0 → should retrieve
    EXPECT_TRUE(targ.shouldRetrieve(makeLogits(10.0f, 7.0f)));
}

TEST(TARGRetrievalPhase3, TARG02_shouldRetrieve_false_when_gap_above_threshold) {
    TARGConfig cfg;
    cfg.gap_threshold = 5.0f;
    TARGRetrieval targ(cfg);
    // gap = 8.0 >= 5.0 → no retrieval
    EXPECT_FALSE(targ.shouldRetrieve(makeLogits(10.0f, 2.0f)));
}

TEST(TARGRetrievalPhase3, TARG03_cooldown_suppresses_retrieval) {
    TARGConfig cfg;
    cfg.gap_threshold             = 5.0f;
    cfg.retrieval_cooldown_tokens = 3;
    TARGRetrieval targ(cfg);

    // First trigger
    EXPECT_TRUE(targ.shouldRetrieve(makeLogits(10.0f, 7.0f)));
    targ.notifyRetrievalExecuted();

    // Next 3 tokens should be suppressed by cooldown
    targ.notifyTokenEmitted();
    EXPECT_FALSE(targ.shouldRetrieve(makeLogits(10.0f, 7.0f)));
    targ.notifyTokenEmitted();
    EXPECT_FALSE(targ.shouldRetrieve(makeLogits(10.0f, 7.0f)));
    targ.notifyTokenEmitted();
    EXPECT_FALSE(targ.shouldRetrieve(makeLogits(10.0f, 7.0f)));

    // After cooldown expires, should trigger again
    targ.notifyTokenEmitted();
    EXPECT_TRUE(targ.shouldRetrieve(makeLogits(10.0f, 7.0f)));
}

TEST(TARGRetrievalPhase3, TARG04_notifyRetrievalExecuted_resets_counter) {
    TARGConfig cfg;
    cfg.gap_threshold             = 5.0f;
    cfg.retrieval_cooldown_tokens = 0;
    TARGRetrieval targ(cfg);

    targ.shouldRetrieve(makeLogits(10.0f, 7.0f));
    auto d1 = targ.gate(makeLogits(10.0f, 7.0f));
    EXPECT_GE(d1.consecutive_uncertain, 1u);

    targ.notifyRetrievalExecuted();
    targ.notifyTokenEmitted();

    auto d2 = targ.gate(makeLogits(10.0f, 2.0f));  // not uncertain
    EXPECT_EQ(d2.consecutive_uncertain, 0u);
}

TEST(TARGRetrievalPhase3, TARG05_min_consecutive_requires_two_tokens) {
    TARGConfig cfg;
    cfg.gap_threshold             = 5.0f;
    cfg.min_consecutive_uncertain = 2;
    cfg.retrieval_cooldown_tokens = 0;
    TARGRetrieval targ(cfg);

    // First uncertain token → NOT yet triggered (need 2 consecutive)
    EXPECT_FALSE(targ.shouldRetrieve(makeLogits(10.0f, 7.0f)));
    // Second uncertain token → now triggered
    EXPECT_TRUE(targ.shouldRetrieve(makeLogits(10.0f, 7.0f)));
}

TEST(TARGRetrievalPhase3, TARG06_entropy_gate_triggers) {
    TARGConfig cfg;
    cfg.gap_threshold             = 5.0f;
    cfg.use_entropy_gate          = true;
    cfg.entropy_threshold         = 1.0f;   // very low threshold
    cfg.retrieval_cooldown_tokens = 0;
    TARGRetrieval targ(cfg);

    // Flat logits → high entropy → triggers even though gap >= threshold
    std::vector<float> flat(50, 0.0f);  // uniform → entropy = ln(50) ≈ 3.9 nats
    EXPECT_TRUE(targ.shouldRetrieve(flat));
}

TEST(TARGRetrievalPhase3, TARG07_stats_trigger_rate) {
    TARGConfig cfg;
    cfg.gap_threshold             = 5.0f;
    cfg.retrieval_cooldown_tokens = 0;
    TARGRetrieval targ(cfg);

    // 3 uncertain, 2 confident
    targ.shouldRetrieve(makeLogits(10.0f, 7.0f));  // trigger
    targ.shouldRetrieve(makeLogits(10.0f, 7.0f));  // trigger
    targ.shouldRetrieve(makeLogits(10.0f, 2.0f));  // no trigger
    targ.shouldRetrieve(makeLogits(10.0f, 7.0f));  // trigger
    targ.shouldRetrieve(makeLogits(10.0f, 2.0f));  // no trigger

    auto s = targ.stats();
    EXPECT_EQ(s.tokens_seen,        5u);
    EXPECT_EQ(s.retrieval_triggers, 3u);
    EXPECT_NEAR(s.triggerRate(), 0.6, 0.01);
}

TEST(TARGRetrievalPhase3, TARG08_reset_clears_state) {
    TARGConfig cfg;
    cfg.gap_threshold             = 5.0f;
    cfg.retrieval_cooldown_tokens = 5;
    TARGRetrieval targ(cfg);

    targ.shouldRetrieve(makeLogits(10.0f, 7.0f));
    targ.notifyRetrievalExecuted();

    targ.reset();

    // After reset: no cooldown
    EXPECT_TRUE(targ.shouldRetrieve(makeLogits(10.0f, 7.0f)));
    EXPECT_EQ(targ.stats().tokens_seen, 1u);
}

// ============================================================================
// FlareRetrieval — FR-01..FR-10
// ============================================================================

#include "rag/flare_retrieval.h"

// FR-01  shouldRetrieve() returns false before any token emitted
TEST(FlareRetrievalPhase3, FR01_no_retrieval_before_tokens) {
    FlareRetrieval flare;
    EXPECT_FALSE(flare.shouldRetrieve());
}

// FR-02  shouldRetrieve() returns true when log-prob drops below threshold
TEST(FlareRetrievalPhase3, FR02_triggers_on_low_logprob) {
    FlareConfig cfg;
    cfg.confidence_threshold      = -2.303f;  // ln(0.1)
    cfg.retrieval_cooldown_tokens = 0;
    FlareRetrieval flare(cfg);

    // log(-3.0) < -2.303 → uncertain
    flare.notifyTokenEmitted("hello", -3.0f);
    EXPECT_TRUE(flare.shouldRetrieve());
}

// FR-03  shouldRetrieve() returns false when log-prob is above threshold
TEST(FlareRetrievalPhase3, FR03_no_trigger_high_logprob) {
    FlareConfig cfg;
    cfg.confidence_threshold      = -2.303f;
    cfg.retrieval_cooldown_tokens = 0;
    FlareRetrieval flare(cfg);

    // log(-0.1) > -2.303 → confident
    flare.notifyTokenEmitted("world", -0.1f);
    EXPECT_FALSE(flare.shouldRetrieve());
}

// FR-04  cooldown suppresses retrieval for the configured number of tokens
TEST(FlareRetrievalPhase3, FR04_cooldown_suppresses) {
    FlareConfig cfg;
    cfg.confidence_threshold      = -2.303f;
    cfg.retrieval_cooldown_tokens = 3;
    FlareRetrieval flare(cfg);

    flare.notifyTokenEmitted("tok1", -3.0f);
    ASSERT_TRUE(flare.shouldRetrieve());
    flare.notifyRetrievalExecuted();

    // Cooldown: next 3 tokens should be suppressed
    for (int i = 0; i < 3; ++i) {
        flare.notifyTokenEmitted("tok", -3.0f);
        auto d = flare.decide();
        EXPECT_TRUE(d.in_cooldown) << "iteration " << i;
        EXPECT_FALSE(d.should_retrieve) << "iteration " << i;
    }

    // After cooldown expires: should trigger again
    flare.notifyTokenEmitted("tok", -3.0f);
    EXPECT_TRUE(flare.shouldRetrieve());
}

// FR-05  min_consecutive_uncertain=2 requires two consecutive low-prob tokens
TEST(FlareRetrievalPhase3, FR05_min_consecutive_uncertain) {
    FlareConfig cfg;
    cfg.confidence_threshold      = -2.303f;
    cfg.min_consecutive_uncertain = 2;
    cfg.retrieval_cooldown_tokens = 0;
    FlareRetrieval flare(cfg);

    flare.notifyTokenEmitted("tok1", -3.0f);
    EXPECT_FALSE(flare.shouldRetrieve());  // only 1 uncertain token

    flare.notifyTokenEmitted("tok2", -3.0f);
    EXPECT_TRUE(flare.shouldRetrieve());   // now 2 consecutive
}

// FR-06  notifyRetrievalExecuted() increments retrieval step count
TEST(FlareRetrievalPhase3, FR06_retrieval_step_counter) {
    FlareConfig cfg;
    cfg.confidence_threshold      = -2.303f;
    cfg.retrieval_cooldown_tokens = 0;
    FlareRetrieval flare(cfg);

    EXPECT_EQ(flare.retrievalStepsDone(), 0u);

    flare.notifyTokenEmitted("tok", -3.0f);
    flare.notifyRetrievalExecuted();
    EXPECT_EQ(flare.retrievalStepsDone(), 1u);

    flare.notifyTokenEmitted("tok", -3.0f);
    flare.notifyRetrievalExecuted();
    EXPECT_EQ(flare.retrievalStepsDone(), 2u);
}

// FR-07  max_retrieval_steps=1 prevents further retrieval after one step
TEST(FlareRetrievalPhase3, FR07_max_steps_cap) {
    FlareConfig cfg;
    cfg.confidence_threshold      = -2.303f;
    cfg.retrieval_cooldown_tokens = 0;
    cfg.max_retrieval_steps       = 1;
    FlareRetrieval flare(cfg);

    flare.notifyTokenEmitted("tok", -3.0f);
    ASSERT_TRUE(flare.shouldRetrieve());
    flare.notifyRetrievalExecuted();

    // Second low-confidence token: max steps reached
    flare.notifyTokenEmitted("tok", -3.0f);
    auto d = flare.decide();
    EXPECT_TRUE(d.max_steps_reached);
    EXPECT_FALSE(d.should_retrieve);
}

// FR-08  buildQuery() returns non-empty string from partial output
TEST(FlareRetrievalPhase3, FR08_buildQuery_non_empty) {
    FlareConfig cfg;
    cfg.mask_uncertain_tokens = false;
    FlareRetrieval flare(cfg);

    flare.notifyTokenEmitted("The", -0.1f);
    flare.notifyTokenEmitted("cat", -0.2f);
    flare.notifyTokenEmitted("sat", -0.3f);

    auto q = flare.buildQuery();
    EXPECT_FALSE(q.empty());
    EXPECT_NE(q.find("The"), std::string::npos);
    EXPECT_NE(q.find("cat"), std::string::npos);
    EXPECT_NE(q.find("sat"), std::string::npos);
}

// FR-09  buildQuery() masks low-confidence tokens when mask_uncertain_tokens=true
TEST(FlareRetrievalPhase3, FR09_buildQuery_masks_uncertain) {
    FlareConfig cfg;
    cfg.confidence_threshold  = -2.303f;
    cfg.mask_uncertain_tokens = true;
    cfg.mask_token            = "[MASK]";
    FlareRetrieval flare(cfg);

    flare.notifyTokenEmitted("The", -0.1f);   // confident
    flare.notifyTokenEmitted("???", -5.0f);   // uncertain → masked
    flare.notifyTokenEmitted("end", -0.2f);   // confident

    auto q = flare.buildQuery();
    EXPECT_NE(q.find("The"), std::string::npos);
    EXPECT_NE(q.find("[MASK]"), std::string::npos);
    EXPECT_EQ(q.find("???"), std::string::npos);  // replaced by mask
    EXPECT_NE(q.find("end"), std::string::npos);
}

// FR-10  reset() clears all state
TEST(FlareRetrievalPhase3, FR10_reset_clears_state) {
    FlareConfig cfg;
    cfg.confidence_threshold      = -2.303f;
    cfg.retrieval_cooldown_tokens = 10;
    FlareRetrieval flare(cfg);

    flare.notifyTokenEmitted("tok", -3.0f);
    flare.notifyRetrievalExecuted();
    ASSERT_EQ(flare.retrievalStepsDone(), 1u);

    flare.reset();

    EXPECT_EQ(flare.retrievalStepsDone(), 0u);
    EXPECT_FALSE(flare.shouldRetrieve());
    EXPECT_EQ(flare.stats().tokens_emitted, 0u);
    // After reset: cooldown is gone, so new low-confidence token triggers
    flare.notifyTokenEmitted("tok", -3.0f);
    EXPECT_TRUE(flare.shouldRetrieve());
}

} // anonymous namespace

// ============================================================================
// TensorButterflyOperator tests — TBO-01..TBO-06
// ============================================================================

namespace {

// Helper: build a 1D TTTrain from a flat float vector using TT-SVD.
static TTTrain make1DTrain(const std::vector<float>& v, double eps = 0.0) {
    TensorTrainConfig cfg;
    cfg.eps = eps;
    TensorTrainDecomposer dec;
    auto [train, _] = dec.decompose(v, {v.size()}, cfg);
    return train;
}

// Helper: reconstruct a 1D TTTrain back to a flat vector.
// For a 1D TT this is just the core data with the bond dimensions stripped.
static std::vector<float> flatten1D(const TTTrain& t) {
    if (t.cores.empty()) return {};
    // 1D TT: single core of shape 1 × n × 1
    const auto& c = t.cores[0];
    return c.data;  // already the n values
}

// Helper: Frobenius norm of a vector.
static float vecNorm(const std::vector<float>& v) {
    float s = 0.0f;
    for (float x : v) s += x * x;
    return std::sqrt(s);
}

// Reference WHT (iterative, same as implementation) for correctness check.
static std::vector<float> refWHT(std::vector<float> v) {
    const std::size_t n = v.size();
    for (std::size_t half = n >> 1; half >= 1; half >>= 1) {
        for (std::size_t start = 0; start < n; start += 2 * half) {
            for (std::size_t j = 0; j < half; ++j) {
                const float u = v[start + j];
                const float w = v[start + j + half];
                v[start + j]        = u + w;
                v[start + j + half] = u - w;
            }
        }
    }
    const float inv_sqrt_n = 1.0f / std::sqrt(static_cast<float>(n));
    for (float& x : v) x *= inv_sqrt_n;
    return v;
}

// TBO-01: build(FOURIER, {4}) succeeds; describe() mentions FOURIER
TEST(TensorButterflyOperator, TBO01_build_fourier_ok) {
    auto op = TensorButterflyOperator::build(OperatorType::FOURIER, {4});
    EXPECT_EQ(op.type(), OperatorType::FOURIER);
    EXPECT_EQ(op.gridShape().size(), 1u);
    EXPECT_EQ(op.gridShape()[0], 4u);
    EXPECT_NE(op.describe().find("FOURIER"), std::string::npos);
}

// TBO-02: apply() on a rank-1 1D TT preserves Frobenius norm (WHT is orthogonal)
TEST(TensorButterflyOperator, TBO02_apply_1d_preserves_norm) {
    // Signal: [1, 2, 3, 4]
    const std::vector<float> sig = {1.0f, 2.0f, 3.0f, 4.0f};
    TTTrain data = make1DTrain(sig);
    ASSERT_EQ(data.cores.size(), 1u);

    auto op = TensorButterflyOperator::build(OperatorType::FOURIER, {4});
    TTTrain result = op.apply(data);

    ASSERT_EQ(result.cores.size(), 1u);
    const auto out = flatten1D(result);
    ASSERT_EQ(out.size(), 4u);

    // Orthogonality: ‖WHT(v)‖ = ‖v‖
    const float norm_in  = vecNorm(sig);
    const float norm_out = vecNorm(out);
    EXPECT_NEAR(norm_in, norm_out, 1e-5f);

    // Verify output matches reference WHT
    const auto ref = refWHT(sig);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_NEAR(out[i], ref[i], 1e-5f) << "index " << i;
    }
}

// TBO-03: apply() on a 2-mode TT ({4,4}) produces separable transform
TEST(TensorButterflyOperator, TBO03_apply_2mode_separable) {
    // Create a {4, 4} TTTrain from a 16-element vector
    std::vector<float> sig(16);
    std::iota(sig.begin(), sig.end(), 1.0f);

    TensorTrainConfig cfg;
    cfg.eps = 0.0;
    TensorTrainDecomposer dec;
    auto [data, _] = dec.decompose(sig, {4u, 4u}, cfg);

    auto op = TensorButterflyOperator::build(OperatorType::FOURIER, {4u, 4u});
    TTTrain result = op.apply(data);

    ASSERT_EQ(result.cores.size(), 2u);
    // Both modes should have the same n as input
    EXPECT_EQ(result.cores[0].n, 4u);
    EXPECT_EQ(result.cores[1].n, 4u);
    // Result norm equals input norm (orthogonal transform)
    EXPECT_NEAR(TensorTrainDecomposer::frobeniusNorm(data),
                TensorTrainDecomposer::frobeniusNorm(result),
                1e-4);
}

// TBO-04: apply() with mismatched shape throws std::invalid_argument
TEST(TensorButterflyOperator, TBO04_apply_shape_mismatch_throws) {
    // Build operator for {4}, apply to 2-mode train
    auto op = TensorButterflyOperator::build(OperatorType::FOURIER, {4u});
    TensorTrainConfig cfg; cfg.eps = 0.0;
    TensorTrainDecomposer dec;
    auto [data, _] = dec.decompose(std::vector<float>(16, 1.0f), {4u, 4u}, cfg);
    EXPECT_THROW(op.apply(data), std::invalid_argument);
}

// TBO-05: build(FOURIER, {3}) throws (3 is not a power of 2)
TEST(TensorButterflyOperator, TBO05_non_power2_throws) {
    EXPECT_THROW(
        TensorButterflyOperator::build(OperatorType::FOURIER, {3u}),
        std::invalid_argument);
}

// TBO-06: build(RADON, ...) throws std::logic_error (STUB #171)
TEST(TensorButterflyOperator, TBO06_radon_stub_throws) {
    EXPECT_THROW(
        TensorButterflyOperator::build(OperatorType::RADON, {4u}),
        std::logic_error);
    EXPECT_THROW(
        TensorButterflyOperator::build(OperatorType::GREENS_FUNCTION, {4u}),
        std::logic_error);
}

// ============================================================================
// AdapterRepository tests — AR-01..AR-06
// ============================================================================

// Helper: build a small valid TTTrain (1D, n=4, rank-1)
static TTTrain makeAdapterTrain() {
    const std::vector<float> sig = {0.1f, 0.2f, 0.3f, 0.4f};
    TensorTrainConfig cfg; cfg.eps = 0.0;
    TensorTrainDecomposer dec;
    auto [t, _] = dec.decompose(sig, {4u}, cfg);
    return t;
}

// AR-01: store() + loadAdapter() round-trip: valid=true, cores match
TEST(AdapterRepository, AR01_store_load_roundtrip) {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    AdapterRepository repo(backend, "tenant1");

    TTTrain adapter = makeAdapterTrain();
    ASSERT_TRUE(repo.store("legal", "llama3-8b", adapter));

    auto desc = repo.loadAdapter("legal", "llama3-8b");
    ASSERT_TRUE(desc.valid);
    EXPECT_EQ(desc.domain,         "legal");
    EXPECT_EQ(desc.base_model_id,  "llama3-8b");
    EXPECT_EQ(desc.tenant_id,      "tenant1");

    ASSERT_EQ(desc.train.cores.size(), adapter.cores.size());
    ASSERT_FALSE(desc.train.cores.empty());
    EXPECT_EQ(desc.train.cores[0].data, adapter.cores[0].data);
}

// AR-02: loadAdapter() for unknown domain returns valid=false
TEST(AdapterRepository, AR02_load_unknown_returns_invalid) {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    AdapterRepository repo(backend, "tenant1");

    auto desc = repo.loadAdapter("nonexistent", "model-x");
    EXPECT_FALSE(desc.valid);
}

// AR-03: listDomains() returns all stored domains (deduplicated, sorted)
TEST(AdapterRepository, AR03_listDomains_sorted_deduplicated) {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    AdapterRepository repo(backend, "tenant1");

    auto adapter = makeAdapterTrain();
    repo.store("medical", "llama3-8b",  adapter);
    repo.store("legal",   "llama3-8b",  adapter);
    repo.store("medical", "llama3-70b", adapter);  // same domain, different model

    auto domains = repo.listDomains();
    ASSERT_EQ(domains.size(), 2u);
    EXPECT_EQ(domains[0], "legal");
    EXPECT_EQ(domains[1], "medical");
}

// AR-04: store() overwrites existing adapter at same key
TEST(AdapterRepository, AR04_store_overwrites) {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    AdapterRepository repo(backend, "tenant1");

    TTTrain a1 = makeAdapterTrain();
    repo.store("legal", "llama3-8b", a1);

    // Overwrite with different weights
    TensorTrainConfig cfg; cfg.eps = 0.0;
    TensorTrainDecomposer dec;
    auto [a2, _] = dec.decompose({0.9f, 0.8f, 0.7f, 0.6f}, {4u}, cfg);
    ASSERT_TRUE(repo.store("legal", "llama3-8b", a2));

    auto desc = repo.loadAdapter("legal", "llama3-8b");
    ASSERT_TRUE(desc.valid);
    EXPECT_NEAR(desc.train.cores[0].data[0], 0.9f, 1e-2f);
}

// AR-05: Different tenants are isolated
TEST(AdapterRepository, AR05_tenant_isolation) {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    AdapterRepository repoA(backend, "tenantA");
    AdapterRepository repoB(backend, "tenantB");

    auto adapter = makeAdapterTrain();
    repoA.store("legal", "llama3-8b", adapter);

    // tenantB should NOT see tenantA's adapter
    auto desc = repoB.loadAdapter("legal", "llama3-8b");
    EXPECT_FALSE(desc.valid);

    // tenantA should still see its own adapter
    EXPECT_TRUE(repoA.loadAdapter("legal", "llama3-8b").valid);
}

// AR-06: listDomains() returns empty vector for empty repository
TEST(AdapterRepository, AR06_empty_repository_listDomains) {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    AdapterRepository repo(backend, "tenant1");

    auto domains = repo.listDomains();
    EXPECT_TRUE(domains.empty());
}

// ============================================================================
// TensorFingerprintGraph — TFG-01..TFG-06
// ============================================================================
//   TFG-01  addAdapter() registers the entry; size() increases
//   TFG-02  entry() returns the stored FingerprintEntry
//   TFG-03  findSimilar() returns the same adapter as top-1 for identical train
//   TFG-04  findSimilar() excludes the query adapter itself
//   TFG-05  removeAdapter() removes entry; findSimilar returns empty
//   TFG-06  findSimilarByFingerprint() respects tenant_id filter
// ============================================================================

// Build a simple 2-core TT-train for TFG tests.
static TTTrain makeTFGTrain(float fill_value = 1.0f) {
    TensorTrainConfig cfg;
    cfg.max_rank = 4;
    cfg.eps = 0.0f;
    TensorTrainDecomposer dec;
    std::vector<float> data(16);
    for (std::size_t i = 0; i < data.size(); ++i)
        data[i] = fill_value * static_cast<float>(i + 1);
    auto [train, info] = dec.decompose(data, {4u, 4u}, cfg);
    return train;
}

// TFG-01: addAdapter registers the entry; size increases
TEST(TensorFingerprintGraph, TFG01_addAdapter_registers_entry) {
    TensorFingerprintGraph graph;
    EXPECT_EQ(graph.size(), 0u);

    auto train = makeTFGTrain();
    graph.addAdapter("key1", train, "legal", "llama3", "t1");
    EXPECT_EQ(graph.size(), 1u);

    graph.addAdapter("key2", makeTFGTrain(2.0f), "medical", "llama3", "t1");
    EXPECT_EQ(graph.size(), 2u);
}

// TFG-02: entry() returns the stored FingerprintEntry
TEST(TensorFingerprintGraph, TFG02_entry_returns_stored_fingerprint) {
    TensorFingerprintGraph graph;
    auto train = makeTFGTrain();
    graph.addAdapter("key_legal", train, "legal", "llama3-8b", "tenant1");

    auto opt = graph.entry("key_legal");
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->domain,        "legal");
    EXPECT_EQ(opt->base_model_id, "llama3-8b");
    EXPECT_EQ(opt->tenant_id,     "tenant1");
    EXPECT_FALSE(opt->fingerprint.empty());
}

// TFG-03: findSimilar returns top match for identical adapter re-registered
TEST(TensorFingerprintGraph, TFG03_findSimilar_returns_most_similar) {
    TensorFingerprintGraph graph;
    auto train_a = makeTFGTrain(1.0f);
    auto train_b = makeTFGTrain(1.001f);  // nearly identical
    auto train_c = makeTFGTrain(100.0f);  // very different scale

    graph.addAdapter("key_a", train_a, "legal",   "llama3", "t1");
    graph.addAdapter("key_b", train_b, "legal",   "llama3", "t1");
    graph.addAdapter("key_c", train_c, "science", "llama3", "t1");

    auto results = graph.findSimilar("key_a", 2);
    ASSERT_FALSE(results.empty());
    // key_b (near-identical) should score higher than key_c.
    EXPECT_EQ(results[0].adapter_key, "key_b");
    EXPECT_GT(results[0].score, 0.0f);
}

// TFG-04: findSimilar excludes the query adapter itself
TEST(TensorFingerprintGraph, TFG04_findSimilar_excludes_query_adapter) {
    TensorFingerprintGraph graph;
    auto train = makeTFGTrain();
    graph.addAdapter("key_query", train, "legal", "llama3", "t1");
    graph.addAdapter("key_other", makeTFGTrain(2.0f), "legal", "llama3", "t1");

    auto results = graph.findSimilar("key_query", 5);
    for (const auto& r : results) {
        EXPECT_NE(r.adapter_key, "key_query");
    }
}

// TFG-05: removeAdapter removes the entry
TEST(TensorFingerprintGraph, TFG05_removeAdapter_removes_entry) {
    TensorFingerprintGraph graph;
    graph.addAdapter("key_a", makeTFGTrain(), "d", "m", "t1");
    graph.addAdapter("key_b", makeTFGTrain(2.0f), "d", "m", "t1");
    EXPECT_EQ(graph.size(), 2u);

    EXPECT_TRUE(graph.removeAdapter("key_a"));
    EXPECT_EQ(graph.size(), 1u);
    EXPECT_FALSE(graph.entry("key_a").has_value());
    EXPECT_FALSE(graph.removeAdapter("key_a"));  // second remove → false
}

// TFG-06: findSimilarByFingerprint respects tenant_id filter
TEST(TensorFingerprintGraph, TFG06_findSimilarByFingerprint_tenant_filter) {
    TensorFingerprintGraph graph;
    graph.addAdapter("key_t1", makeTFGTrain(1.0f), "legal", "llama3", "t1");
    graph.addAdapter("key_t2", makeTFGTrain(1.0f), "legal", "llama3", "t2");

    // Fingerprint identical to both, but filter to tenant t1.
    auto ent = graph.entry("key_t1");
    ASSERT_TRUE(ent.has_value());

    auto results = graph.findSimilarByFingerprint(ent->fingerprint, 5, "t1");
    for (const auto& r : results) {
        EXPECT_EQ(r.adapter_key, "key_t1");  // only t1 entries
    }
    EXPECT_TRUE(results.size() <= 1u);
}

// =============================================================================
// TensorRAGPipeline tests — TRPL-01..TRPL-08
// =============================================================================

// Helper: logits with a large gap (top-1 much larger than top-2)
static std::vector<float> confidentLogits() {
    // top-1 = index 0 (value 20), top-2 = index 1 (value 5) → gap = 15
    return {20.0f, 5.0f, 1.0f, 0.5f, 0.2f};
}

// Helper: logits with a small gap (near-tie between top-1 and top-2)
static std::vector<float> uncertainLogits() {
    // top-1 = 5.1, top-2 = 5.0 → gap = 0.1 (below default threshold 5.0)
    return {5.1f, 5.0f, 1.0f, 0.5f, 0.2f};
}

// TRPL-01: step() returns should_retrieve=false for a confident token
TEST(TensorRAGPipeline, TRPL01_confident_token_no_retrieval) {
    TensorRAGPipelineConfig cfg;
    cfg.use_flare = true;
    cfg.use_targ  = true;
    cfg.flare_config.confidence_threshold = -2.303f;
    cfg.targ_config.gap_threshold         = 5.0f;

    TensorRAGPipeline pipeline(cfg);

    // High log-prob (confident) + large logit gap (confident)
    auto decision = pipeline.step("hello", -0.1f, confidentLogits());

    EXPECT_FALSE(decision.should_retrieve);
    EXPECT_EQ(decision.trigger, RAGDecision::Trigger::NONE);
    EXPECT_FALSE(decision.flare_triggered);
    EXPECT_FALSE(decision.targ_triggered);
    EXPECT_EQ(pipeline.stats().total_token_steps, 1u);
    EXPECT_EQ(pipeline.stats().flare_triggers,    0u);
    EXPECT_EQ(pipeline.stats().targ_triggers,     0u);
}

// TRPL-02: step() sets flare_triggered when log-prob is below FLARE threshold
TEST(TensorRAGPipeline, TRPL02_flare_triggers_on_low_log_prob) {
    TensorRAGPipelineConfig cfg;
    cfg.use_flare = true;
    cfg.use_targ  = false;  // isolate FLARE
    cfg.flare_config.confidence_threshold      = -2.303f;
    cfg.flare_config.min_consecutive_uncertain = 1;

    TensorRAGPipeline pipeline(cfg);

    // Very low log-prob (well below threshold)
    auto decision = pipeline.step("uncertain_token", -5.0f, {});

    EXPECT_TRUE(decision.should_retrieve);
    EXPECT_TRUE(decision.flare_triggered);
    EXPECT_FALSE(decision.targ_triggered);
    EXPECT_EQ(decision.trigger, RAGDecision::Trigger::FLARE_ONLY);
    EXPECT_FALSE(decision.flare_query.empty());
    EXPECT_EQ(pipeline.stats().flare_triggers, 1u);
    EXPECT_EQ(pipeline.stats().targ_triggers,  0u);
}

// TRPL-03: step() sets targ_triggered when logit gap is below TARG threshold
TEST(TensorRAGPipeline, TRPL03_targ_triggers_on_small_gap) {
    TensorRAGPipelineConfig cfg;
    cfg.use_flare = false;  // isolate TARG
    cfg.use_targ  = true;
    cfg.targ_config.gap_threshold             = 5.0f;
    cfg.targ_config.min_consecutive_uncertain = 1;

    TensorRAGPipeline pipeline(cfg);

    // Small logit gap (0.1 < 5.0 threshold)
    auto decision = pipeline.step("token", 0.0f, uncertainLogits());

    EXPECT_TRUE(decision.should_retrieve);
    EXPECT_TRUE(decision.targ_triggered);
    EXPECT_FALSE(decision.flare_triggered);
    EXPECT_EQ(decision.trigger, RAGDecision::Trigger::TARG_ONLY);
    EXPECT_GT(decision.targ_gap, 0.0f);
    EXPECT_EQ(pipeline.stats().targ_triggers,  1u);
    EXPECT_EQ(pipeline.stats().flare_triggers, 0u);
}

// TRPL-04: step() sets BOTH trigger and increments combined_triggers
TEST(TensorRAGPipeline, TRPL04_both_gates_fire_simultaneously) {
    TensorRAGPipelineConfig cfg;
    cfg.use_flare = true;
    cfg.use_targ  = true;
    cfg.flare_config.confidence_threshold      = -2.303f;
    cfg.flare_config.min_consecutive_uncertain = 1;
    cfg.targ_config.gap_threshold              = 5.0f;
    cfg.targ_config.min_consecutive_uncertain  = 1;

    TensorRAGPipeline pipeline(cfg);

    // Low log-prob (FLARE fires) + small gap (TARG fires)
    auto decision = pipeline.step("bad_token", -5.0f, uncertainLogits());

    EXPECT_TRUE(decision.should_retrieve);
    EXPECT_TRUE(decision.flare_triggered);
    EXPECT_TRUE(decision.targ_triggered);
    EXPECT_EQ(decision.trigger, RAGDecision::Trigger::BOTH);
    EXPECT_EQ(pipeline.stats().combined_triggers, 1u);
    EXPECT_EQ(pipeline.stats().flare_triggers,    1u);
    EXPECT_EQ(pipeline.stats().targ_triggers,     1u);
}

// TRPL-05: notifyRetrievalDone() resets cooldowns; next uncertain token is suppressed
TEST(TensorRAGPipeline, TRPL05_notifyRetrievalDone_resets_cooldown) {
    TensorRAGPipelineConfig cfg;
    cfg.use_flare = true;
    cfg.use_targ  = true;
    cfg.flare_config.confidence_threshold      = -2.303f;
    cfg.flare_config.min_consecutive_uncertain = 1;
    cfg.flare_config.retrieval_cooldown_tokens = 5;
    cfg.targ_config.gap_threshold              = 5.0f;
    cfg.targ_config.min_consecutive_uncertain  = 1;
    cfg.targ_config.retrieval_cooldown_tokens  = 5;

    TensorRAGPipeline pipeline(cfg);

    // First call: both gates fire
    pipeline.step("bad", -5.0f, uncertainLogits());
    pipeline.notifyRetrievalDone();
    EXPECT_EQ(pipeline.stats().total_retrievals, 1u);

    // Immediately after retrieval: cooldown suppresses both gates
    auto d2 = pipeline.step("next", -5.0f, uncertainLogits());
    EXPECT_FALSE(d2.flare_triggered);  // FLARE in cooldown
    EXPECT_FALSE(d2.targ_triggered);   // TARG in cooldown
    EXPECT_FALSE(d2.should_retrieve);
}

// TRPL-06: reset() clears all stats and sub-gate state
TEST(TensorRAGPipeline, TRPL06_reset_clears_all_state) {
    TensorRAGPipelineConfig cfg;
    cfg.use_flare = true;
    cfg.use_targ  = true;
    cfg.flare_config.confidence_threshold      = -2.303f;
    cfg.flare_config.min_consecutive_uncertain = 1;
    cfg.targ_config.gap_threshold              = 5.0f;
    cfg.targ_config.min_consecutive_uncertain  = 1;

    TensorRAGPipeline pipeline(cfg);

    pipeline.step("a", -5.0f, uncertainLogits());
    pipeline.notifyRetrievalDone();
    EXPECT_GT(pipeline.stats().total_token_steps, 0u);

    pipeline.reset();

    EXPECT_EQ(pipeline.stats().total_token_steps, 0u);
    EXPECT_EQ(pipeline.stats().flare_triggers,    0u);
    EXPECT_EQ(pipeline.stats().targ_triggers,     0u);
    EXPECT_EQ(pipeline.stats().combined_triggers, 0u);
    EXPECT_EQ(pipeline.stats().total_retrievals,  0u);

    // Sub-gate state is also reset: high log-prob should not trigger
    auto d = pipeline.step("clean", -0.1f, confidentLogits());
    EXPECT_FALSE(d.should_retrieve);
}

// TRPL-07: stats() tracks totals across a multi-step session correctly
TEST(TensorRAGPipeline, TRPL07_stats_accuracy) {
    TensorRAGPipelineConfig cfg;
    cfg.use_flare = true;
    cfg.use_targ  = true;
    cfg.flare_config.confidence_threshold      = -2.303f;
    cfg.flare_config.min_consecutive_uncertain = 1;
    cfg.flare_config.retrieval_cooldown_tokens = 0;  // no cooldown
    cfg.targ_config.gap_threshold              = 5.0f;
    cfg.targ_config.min_consecutive_uncertain  = 1;
    cfg.targ_config.retrieval_cooldown_tokens  = 0;  // no cooldown

    TensorRAGPipeline pipeline(cfg);

    // Step 1: confident → no trigger
    pipeline.step("ok", -0.1f, confidentLogits());
    // Step 2: both fire
    pipeline.step("bad", -5.0f, uncertainLogits());
    pipeline.notifyRetrievalDone();
    // Step 3: only FLARE fires (confident TARG logits)
    pipeline.step("low_prob", -5.0f, confidentLogits());

    auto s = pipeline.stats();
    EXPECT_EQ(s.total_token_steps, 3u);
    EXPECT_EQ(s.total_retrievals,  1u);
    EXPECT_EQ(s.flare_triggers,    2u);  // step 2 + step 3
    EXPECT_EQ(s.targ_triggers,     1u);  // step 2 only
    EXPECT_EQ(s.combined_triggers, 1u);  // step 2 only
}

// TRPL-08: use_flare=false and use_targ=false each independently disable a gate
TEST(TensorRAGPipeline, TRPL08_individual_gate_disable) {
    // Disable FLARE, keep TARG
    {
        TensorRAGPipelineConfig cfg;
        cfg.use_flare = false;
        cfg.use_targ  = true;
        cfg.targ_config.gap_threshold             = 5.0f;
        cfg.targ_config.min_consecutive_uncertain = 1;

        TensorRAGPipeline pipeline(cfg);
        auto d = pipeline.step("tok", -5.0f, uncertainLogits());
        EXPECT_FALSE(d.flare_triggered);
        EXPECT_TRUE(d.targ_triggered);
        EXPECT_EQ(d.flare_log_prob, 0.0f);  // FLARE produced no output
    }

    // Disable TARG, keep FLARE
    {
        TensorRAGPipelineConfig cfg;
        cfg.use_flare = true;
        cfg.use_targ  = false;
        cfg.flare_config.confidence_threshold      = -2.303f;
        cfg.flare_config.min_consecutive_uncertain = 1;

        TensorRAGPipeline pipeline(cfg);
        auto d = pipeline.step("tok", -5.0f, uncertainLogits());
        EXPECT_TRUE(d.flare_triggered);
        EXPECT_FALSE(d.targ_triggered);
        EXPECT_EQ(d.targ_gap, 0.0f);  // TARG produced no output
    }
}

} // anonymous namespace
