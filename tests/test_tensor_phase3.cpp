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
 */

#include "query/tensor_contraction_engine.h"
#include "query/tensor_aware_query_optimizer.h"
#include "rag/targ_retrieval.h"
#include "storage/tensor_train_decomposer.h"

#include <gtest/gtest.h>
#include <cmath>
#include <memory>
#include <numeric>
#include <vector>

using namespace themis::storage;
using namespace themis::query;
using namespace themis::rag;

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
