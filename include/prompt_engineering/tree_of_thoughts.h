/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tree_of_thoughts.h                                 ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 07:08:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     341                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 696d2d349b  2026-03-24  fix: address 7 Copilot review comments (docs, beam_width ... ║
    • b87706b26d  2026-03-24  feat(prompt_engineering): implement ToT reasoner, ProTeGi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tree_of_thoughts.h
 * @brief Tree-of-Thoughts (ToT) multi-path reasoning framework.
 *
 * Implements the Tree-of-Thoughts reasoning strategy introduced by
 * Yao et al. (2023): instead of a single linear chain of thought, the model
 * explores multiple intermediate reasoning steps ("thoughts") organised as a
 * tree, evaluates each node, and selects the best path to a final answer.
 *
 * Three search strategies are supported:
 *  - **BFS** – breadth-first expansion, level by level.
 *  - **DFS** – depth-first expansion with backtracking.
 *  - **BEAM** – beam search retaining only the top-k nodes per level.
 *
 * Reference:
 *   S. Yao et al., "Tree of Thoughts: Deliberate Problem Solving with Large
 *   Language Models," in Proc. NeurIPS, vol. 36, 2023.
 *   Available: https://arxiv.org/abs/2305.10601
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace themis {
namespace prompt_engineering {

// ---------------------------------------------------------------------------
// Enumerations
// ---------------------------------------------------------------------------

/**
 * @brief Search strategy for tree exploration.
 */
enum class ToTSearchStrategy {
    BFS,   ///< Breadth-first search – explores all nodes at each depth before going deeper.
    DFS,   ///< Depth-first search with backtracking – follows one path to max depth.
    BEAM   ///< Beam search – retains only the top-k nodes at each level.
};

/**
 * @brief Node evaluation verdict returned by IToTEvaluator.
 */
enum class ToTVerdict {
    SURE,    ///< This thought is accepted as a valid solution; stop expanding from this node.
    MAYBE,   ///< This thought might lead to a solution; worth exploring.
    IMPOSSIBLE ///< This thought cannot lead to a valid solution; prune it.
};

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

/**
 * @brief A single node in the thought tree.
 */
struct ToTNode {
    std::string              thought;    ///< The reasoning step text at this node.
    double                   score;      ///< Numeric quality score assigned by the evaluator.
    ToTVerdict               verdict;    ///< Categorical verdict for pruning decisions.
    size_t                   depth;      ///< Depth in the tree (root = 0).
    std::vector<std::string> path;       ///< Ordered thoughts from root to this node.
};

/**
 * @brief Configuration for the TreeOfThoughtsBuilder.
 */
struct ToTConfig {
    size_t           max_depth        = 3;    ///< Maximum tree depth before forcing a final answer.
    size_t           branching_factor = 3;    ///< Number of child thoughts generated per node.
    size_t           beam_width       = 2;    ///< Beam width used when strategy == BEAM.
    ToTSearchStrategy strategy        = ToTSearchStrategy::BFS; ///< Default search strategy.
    double           prune_threshold  = 0.0;  ///< Prune nodes with score <= this value.
    bool             verbose          = false;///< Emit intermediate thoughts to result log.
};

/**
 * @brief Complete result of a tree-of-thoughts reasoning session.
 */
struct ToTResult {
    bool                     success   = false;  ///< True when a conclusive answer was found.
    std::string              answer;             ///< The final synthesised answer.
    std::vector<std::string> best_path;          ///< Thought chain of the winning path.
    double                   best_score = 0.0;   ///< Score of the winning path.
    size_t                   nodes_explored = 0; ///< Total nodes evaluated during search.
    std::vector<std::string> log;                ///< Verbose per-step log (populated when ToTConfig::verbose).
};

// ---------------------------------------------------------------------------
// Interfaces
// ---------------------------------------------------------------------------

/**
 * @brief Interface for generating candidate child thoughts from a parent node.
 *
 * Callers inject an implementation backed by an LLM to produce `k` diverse
 * next reasoning steps from the accumulated thought path.
 */
class IToTThoughtGenerator {
public:
    virtual ~IToTThoughtGenerator() = default;

    /**
     * @brief Generate candidate thoughts from the current reasoning path.
     *
     * @param problem   The original problem statement.
     * @param path      Reasoning steps accumulated so far (root → current node).
     * @param k         Number of candidate thoughts to generate.
     * @return Vector of exactly @p k thought strings (or fewer on failure).
     */
    virtual std::vector<std::string> generate(
        const std::string& problem,
        const std::vector<std::string>& path,
        size_t k) = 0;
};

/**
 * @brief Interface for evaluating and scoring a candidate thought node.
 *
 * Implementors score the plausibility of a partial reasoning path and assign
 * a verdict used for pruning.
 */
class IToTEvaluator {
public:
    virtual ~IToTEvaluator() = default;

    /**
     * @brief Score a thought node.
     *
     * @param problem  The original problem statement.
     * @param node     The node to evaluate (path and thought are populated).
     * @return Pair of (score in [0, 1], verdict).
     */
    virtual std::pair<double, ToTVerdict> evaluate(
        const std::string& problem,
        const ToTNode& node) = 0;
};

// ---------------------------------------------------------------------------
// Built-in heuristic implementations (no LLM required for unit tests)
// ---------------------------------------------------------------------------

/**
 * @brief Heuristic thought generator that appends numbered reasoning steps.
 *
 * Produces deterministic outputs useful for testing without an LLM.
 * Each generated thought has the form:
 *   "Consider approach <depth>.<i>: <problem excerpt>"
 */
class HeuristicThoughtGenerator : public IToTThoughtGenerator {
public:
    std::vector<std::string> generate(
        const std::string& problem,
        const std::vector<std::string>& path,
        size_t k) override;
};

/**
 * @brief Length-based heuristic evaluator.
 *
 * Scores a node by the cumulative length of its thought path relative to the
 * problem length — longer paths score higher (proxy for thoroughness).
 * Always returns MAYBE for internal nodes and SURE for nodes at max depth.
 */
class HeuristicToTEvaluator : public IToTEvaluator {
public:
    explicit HeuristicToTEvaluator(size_t max_depth = 3);

    std::pair<double, ToTVerdict> evaluate(
        const std::string& problem,
        const ToTNode& node) override;

private:
    size_t max_depth_;
};

// ---------------------------------------------------------------------------
// Main builder
// ---------------------------------------------------------------------------

/**
 * @brief Drives Tree-of-Thoughts reasoning over a problem statement.
 *
 * Usage:
 * @code
 * TreeOfThoughtsBuilder tot;
 * tot.setThoughtGenerator(std::make_shared<MyLLMGenerator>())
 *    .setEvaluator(std::make_shared<MyLLMEvaluator>());
 * ToTResult result = tot.solve("What caused the fall of the Roman Empire?");
 * std::cout << result.answer << "\n";
 * for (const auto& thought : result.best_path) {
 *     std::cout << " • " << thought << "\n";
 * }
 * @endcode
 */
class TreeOfThoughtsBuilder {
public:
    /**
     * @brief Construct with default configuration.
     */
    explicit TreeOfThoughtsBuilder(const ToTConfig& config = ToTConfig{});

    /**
     * @brief Set the thought generator.
     * @return Reference to @c *this for chaining.
     */
    TreeOfThoughtsBuilder& setThoughtGenerator(
        std::shared_ptr<IToTThoughtGenerator> generator);

    /**
     * @brief Set the node evaluator.
     * @return Reference to @c *this for chaining.
     */
    TreeOfThoughtsBuilder& setEvaluator(
        std::shared_ptr<IToTEvaluator> evaluator);

    /**
     * @brief Update the search configuration.
     * @return Reference to @c *this for chaining.
     */
    TreeOfThoughtsBuilder& setConfig(const ToTConfig& config);

    /**
     * @brief Return a read-only reference to the current configuration.
     */
    const ToTConfig& getConfig() const;

    /**
     * @brief Run tree-of-thoughts search to solve @p problem.
     *
     * If no generator or evaluator was injected, built-in heuristic
     * implementations are used automatically so callers do not need to
     * provide custom components for basic usage.
     *
     * If @p problem is empty, the search is not executed and a default /
     * empty ToTResult is returned (e.g., with no best_path and no answer).
     *
     * Even for non-empty problems, the returned ToTResult may have an
     * empty best_path and/or answer if no satisfactory solution can be
     * found under the current configuration (e.g., depth limits, pruning).
     * Callers SHOULD NOT assume that best_path or answer are always set and
     * SHOULD check these fields before use.
     *
     * @param problem  Natural-language problem statement.
     * @return ToTResult containing the best reasoning path (if any) and
     *         final answer (if available).
     */
    ToTResult solve(const std::string& problem);

    // -------------------------------------------------------------------------
    // Static helpers for prompt construction
    // -------------------------------------------------------------------------

    /**
     * @brief Build a thought-generation prompt for a given path.
     *
     * Formats the accumulated path as context and instructs the LLM to
     * generate @p k next reasoning steps.
     *
     * @param problem   The original problem.
     * @param path      Current reasoning chain.
     * @param k         Number of thoughts to request.
     * @return Formatted prompt string.
     */
    static std::string buildGenerationPrompt(
        const std::string& problem,
        const std::vector<std::string>& path,
        size_t k);

    /**
     * @brief Build a node-evaluation prompt for a given thought node.
     *
     * Instructs the LLM to rate the promising-ness of the partial path
     * on a scale of 0–10 and assign a sure/maybe/impossible verdict.
     *
     * @param problem  The original problem.
     * @param node     The node to evaluate.
     * @return Formatted prompt string.
     */
    static std::string buildEvaluationPrompt(
        const std::string& problem,
        const ToTNode& node);

    /**
     * @brief Build a synthesis prompt that condenses the best path to an answer.
     *
     * @param problem    The original problem.
     * @param best_path  Winning reasoning chain.
     * @return Formatted synthesis prompt.
     */
    static std::string buildSynthesisPrompt(
        const std::string& problem,
        const std::vector<std::string>& best_path);

private:
    ToTConfig                            config_;
    std::shared_ptr<IToTThoughtGenerator> generator_;
    std::shared_ptr<IToTEvaluator>        evaluator_;

    // Internal search implementations
    ToTResult solveBFS(const std::string& problem);
    ToTResult solveDFS(const std::string& problem);
    ToTResult solveBeam(const std::string& problem);

    // Shared helpers
    ToTNode makeNode(const std::string& thought,
                     const std::vector<std::string>& parent_path,
                     size_t depth) const;
    std::string synthesiseAnswer(const std::string& problem,
                                 const std::vector<std::string>& best_path) const;
};

} // namespace prompt_engineering
} // namespace themis
