/**
 * @file tree_of_thoughts.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/tree_of_thoughts.h"
#include "utils/logger.h"

#include <algorithm>
#include <queue>
#include <sstream>
#include <stack>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// HeuristicThoughtGenerator
// ============================================================================

std::vector<std::string> HeuristicThoughtGenerator::generate(
    const std::string& problem,
    const std::vector<std::string>& path,
    size_t k)
{
    std::vector<std::string> thoughts;
    thoughts.reserve(k);

    // Derive a short excerpt from the problem for labelling
    std::string excerpt = problem.substr(0, std::min(problem.size(), size_t(30)));
    size_t depth = path.size();

    for (size_t i = 0; i < k; ++i) {
        std::ostringstream oss;
        oss << "Consider approach " << depth << "." << (i + 1)
            << ": " << excerpt
            << " (variant " << (i + 1) << ")";
        thoughts.push_back(oss.str());
    }
    return thoughts;
}

// ============================================================================
// HeuristicToTEvaluator
// ============================================================================

HeuristicToTEvaluator::HeuristicToTEvaluator(size_t max_depth)
    : max_depth_(max_depth)
{}

std::pair<double, ToTVerdict> HeuristicToTEvaluator::evaluate(
    const std::string& problem,
    const ToTNode& node)
{
    // Score = total path length / problem length capped at 1.0
    size_t path_len = 0;
    for (const auto& t : node.path) {
        path_len += t.size();
    }
    path_len += node.thought.size();

    double score = (problem.empty())
                   ? 0.5
                   : std::min(1.0, static_cast<double>(path_len) /
                                   static_cast<double>(problem.size() * (max_depth_ + 1)));

    // Nodes at max depth are considered conclusive
    ToTVerdict verdict = (node.depth >= max_depth_)
                         ? ToTVerdict::SURE
                         : ToTVerdict::MAYBE;

    return {score, verdict};
}

// ============================================================================
// TreeOfThoughtsBuilder – construction
// ============================================================================

TreeOfThoughtsBuilder::TreeOfThoughtsBuilder(const ToTConfig& config)
    : config_(config)
{}

TreeOfThoughtsBuilder& TreeOfThoughtsBuilder::setThoughtGenerator(
    std::shared_ptr<IToTThoughtGenerator> generator)
{
    std::lock_guard<std::mutex> lock(mutex_);
    generator_ = std::move(generator);
    return *this;
}

TreeOfThoughtsBuilder& TreeOfThoughtsBuilder::setEvaluator(
    std::shared_ptr<IToTEvaluator> evaluator)
{
    std::lock_guard<std::mutex> lock(mutex_);
    evaluator_ = std::move(evaluator);
    return *this;
}

TreeOfThoughtsBuilder& TreeOfThoughtsBuilder::setConfig(const ToTConfig& config)
{
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    return *this;
}

const ToTConfig& TreeOfThoughtsBuilder::getConfig() const
{
    return config_;
}

// ============================================================================
// TreeOfThoughtsBuilder – solve()
// ============================================================================

ToTResult TreeOfThoughtsBuilder::solve(const std::string& problem)
{
    if (problem.empty()) {
        THEMIS_WARN("TreeOfThoughtsBuilder::solve called with empty problem");
        return ToTResult{};
    }

    std::shared_ptr<IToTThoughtGenerator> generator;
    std::shared_ptr<IToTEvaluator> evaluator;
    ToTConfig config;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Snapshot injected collaborators once so concurrent reconfiguration
        // cannot race with generation/evaluation during this solve() call.
        if (!generator_) {
            generator_ = std::make_shared<HeuristicThoughtGenerator>();
        }
        if (!evaluator_) {
            evaluator_ = std::make_shared<HeuristicToTEvaluator>(config_.max_depth);
        }

        generator = generator_;
        evaluator = evaluator_;
        config = config_;
    }

    THEMIS_INFO("ToT solve: strategy={}, max_depth={}, branching={}",
                static_cast<int>(config.strategy),
                config.max_depth,
                config.branching_factor);

    switch (config.strategy) {
        case ToTSearchStrategy::DFS:  return solveDFS(problem, config, generator, evaluator);
        case ToTSearchStrategy::BEAM: return solveBeam(problem, config, generator, evaluator);
        case ToTSearchStrategy::BFS:  // fallthrough
        [[fallthrough]];\n        default:                      return solveBFS(problem, config, generator, evaluator);
    }
}

// ============================================================================
// Internal search – BFS
// ============================================================================

ToTResult TreeOfThoughtsBuilder::solveBFS(
    const std::string& problem,
    const ToTConfig& config,
    const std::shared_ptr<IToTThoughtGenerator>& generator,
    const std::shared_ptr<IToTEvaluator>& evaluator)
{
    ToTResult result;
    std::queue<ToTNode> frontier;

    // Use the solve()-scoped snapshots so pointer updates on the builder do not
    // race with this traversal. Implementations are responsible for their own
    // internal thread-safety when shared across multiple solve() calls.
    auto root_thoughts = generator->generate(problem, {}, config.branching_factor);
    for (const auto& t : root_thoughts) {
        frontier.push(makeNode(t, {}, 0));
    }

    ToTNode best_node;
    best_node.score = -1.0;

    while (!frontier.empty()) {
        ToTNode node = frontier.front();
        frontier.pop();

        auto [score, verdict] = evaluator->evaluate(problem, node);
        node.score   = score;
        node.verdict = verdict;
        ++result.nodes_explored;

        if (config.verbose) {
            std::ostringstream msg;
            msg << "BFS depth=" << node.depth
                << " score=" << score
                << " verdict=" << static_cast<int>(verdict)
                << " thought=\"" << node.thought.substr(0, 40) << "\"";
            result.log.push_back(msg.str());
        }

        if (score > best_node.score) {
            best_node = node;
        }

        if (verdict == ToTVerdict::SURE) {
            result.success    = true;
            result.best_path  = node.path;
            result.best_path.push_back(node.thought);
            result.best_score = score;
            result.answer     = synthesiseAnswer(problem, result.best_path);
            return result;
        }

        if (verdict == ToTVerdict::IMPOSSIBLE || score <= config.prune_threshold) {
            continue; // prune
        }

        if (node.depth < config.max_depth) {
            std::vector<std::string> child_path = node.path;
            child_path.push_back(node.thought);
            auto children = generator->generate(problem, child_path, config.branching_factor);
            for (const auto& child_thought : children) {
                frontier.push(makeNode(child_thought, child_path, node.depth + 1));
            }
        }
    }

    // No SURE node found – return best discovered
    if (best_node.score >= 0.0) {
        result.best_path = best_node.path;
        result.best_path.push_back(best_node.thought);
        result.best_score = best_node.score;
        result.answer     = synthesiseAnswer(problem, result.best_path);
    }
    return result;
}

// ============================================================================
// Internal search – DFS
// ============================================================================

ToTResult TreeOfThoughtsBuilder::solveDFS(
    const std::string& problem,
    const ToTConfig& config,
    const std::shared_ptr<IToTThoughtGenerator>& generator,
    const std::shared_ptr<IToTEvaluator>& evaluator)
{
    ToTResult result;

    // Use an explicit stack of nodes
    std::stack<ToTNode> frontier;

    auto root_thoughts = generator->generate(problem, {}, config.branching_factor);
    // Push in reverse so we pop highest-index first (matches BFS order)
    for (int i = static_cast<int>(root_thoughts.size()) - 1; i >= 0; --i) {
        frontier.push(makeNode(root_thoughts[i], {}, 0));
    }

    ToTNode best_node;
    best_node.score = -1.0;

    while (!frontier.empty()) {
        ToTNode node = frontier.top();
        frontier.pop();

        auto [score, verdict] = evaluator->evaluate(problem, node);
        node.score   = score;
        node.verdict = verdict;
        ++result.nodes_explored;

        if (config.verbose) {
            std::ostringstream msg;
            msg << "DFS depth=" << node.depth << " score=" << score;
            result.log.push_back(msg.str());
        }

        if (score > best_node.score) {
            best_node = node;
        }

        if (verdict == ToTVerdict::SURE) {
            result.success    = true;
            result.best_path  = node.path;
            result.best_path.push_back(node.thought);
            result.best_score = score;
            result.answer     = synthesiseAnswer(problem, result.best_path);
            return result;
        }

        if (verdict == ToTVerdict::IMPOSSIBLE || score <= config.prune_threshold) {
            continue;
        }

        if (node.depth < config.max_depth) {
            std::vector<std::string> child_path = node.path;
            child_path.push_back(node.thought);
            auto children = generator->generate(problem, child_path, config.branching_factor);
            for (int i = static_cast<int>(children.size()) - 1; i >= 0; --i) {
                frontier.push(makeNode(children[i], child_path, node.depth + 1));
            }
        }
    }

    if (best_node.score >= 0.0) {
        result.best_path = best_node.path;
        result.best_path.push_back(best_node.thought);
        result.best_score = best_node.score;
        result.answer     = synthesiseAnswer(problem, result.best_path);
    }
    return result;
}

// ============================================================================
// Internal search – Beam
// ============================================================================

ToTResult TreeOfThoughtsBuilder::solveBeam(
    const std::string& problem,
    const ToTConfig& config,
    const std::shared_ptr<IToTThoughtGenerator>& generator,
    const std::shared_ptr<IToTEvaluator>& evaluator)
{
    ToTResult result;

    // Each "beam" entry is an evaluated node
    std::vector<ToTNode> beam;

    // Initialise beam from root thoughts
    auto root_thoughts = generator->generate(problem, {}, config.branching_factor);
    for (const auto& t : root_thoughts) {
        ToTNode n = makeNode(t, {}, 0);
        auto [score, verdict] = evaluator->evaluate(problem, n);
        n.score   = score;
        n.verdict = verdict;
        ++result.nodes_explored;
        if (verdict != ToTVerdict::IMPOSSIBLE && score > config.prune_threshold) {
            beam.push_back(std::move(n));
        }
    }

    // Sort and trim beam to beam_width
    auto sort_beam = [](std::vector<ToTNode>& b, size_t width) {
        std::sort(b.begin(), b.end(),
                  [](const ToTNode& a, const ToTNode& x) { return a.score > x.score; });
        if (b.size() > width) {
            b.resize(width);
        }
    };
    sort_beam(beam, config.beam_width);

    for (size_t depth = 1; depth <= config.max_depth; ++depth) {
        std::vector<ToTNode> next_beam;

        for (const auto& node : beam) {
            if (node.verdict == ToTVerdict::SURE) {
                result.success    = true;
                result.best_path  = node.path;
                result.best_path.push_back(node.thought);
                result.best_score = node.score;
                result.answer     = synthesiseAnswer(problem, result.best_path);
                return result;
            }

            std::vector<std::string> child_path = node.path;
            child_path.push_back(node.thought);

            auto children = generator->generate(problem, child_path, config.branching_factor);
            for (const auto& child_t : children) {
                ToTNode child = makeNode(child_t, child_path, depth);
                auto [score, verdict] = evaluator->evaluate(problem, child);
                child.score   = score;
                child.verdict = verdict;
                ++result.nodes_explored;

                if (config.verbose) {
                    std::ostringstream msg;
                    msg << "Beam depth=" << depth << " score=" << score;
                    result.log.push_back(msg.str());
                }

                if (verdict != ToTVerdict::IMPOSSIBLE && score > config.prune_threshold) {
                    next_beam.push_back(std::move(child));
                }
            }
        }

        if (next_beam.empty()) {
            break;
        }
        sort_beam(next_beam, config.beam_width);
        beam = std::move(next_beam);
    }

    if (!beam.empty()) {
        const ToTNode& best = beam.front();
        result.best_path = best.path;
        result.best_path.push_back(best.thought);
        result.best_score = best.score;
        result.answer     = synthesiseAnswer(problem, result.best_path);
    }
    return result;
}

// ============================================================================
// Helpers
// ============================================================================

ToTNode TreeOfThoughtsBuilder::makeNode(const std::string& thought,
                                        const std::vector<std::string>& parent_path,
                                        size_t depth) const
{
    ToTNode node;
    node.thought = thought;
    node.score   = 0.0;
    node.verdict = ToTVerdict::MAYBE;
    node.depth   = depth;
    node.path    = parent_path;
    return node;
}

std::string TreeOfThoughtsBuilder::synthesiseAnswer(
    const std::string& problem,
    const std::vector<std::string>& best_path) const
{
    std::ostringstream ans;
    ans << "Based on the following reasoning chain:\n\n";
    for (size_t i = 0; i < best_path.size(); ++i) {
        ans << "Step " << (i + 1) << ": " << best_path[i] << "\n";
    }
    ans << "\nConclusion: The best path through the problem \""
        << problem.substr(0, std::min(problem.size(), size_t(60)))
        << "\" has been identified through "
        << best_path.size() << " reasoning step(s).";
    return ans.str();
}

// ============================================================================
// Static prompt builders
// ============================================================================

std::string TreeOfThoughtsBuilder::buildGenerationPrompt(
    const std::string& problem,
    const std::vector<std::string>& path,
    size_t k)
{
    std::ostringstream out;
    out << "You are solving the following problem step by step.\n\n";
    out << "Problem:\n" << problem << "\n\n";

    if (!path.empty()) {
        out << "Reasoning so far:\n";
        for (size_t i = 0; i < path.size(); ++i) {
            out << "Step " << (i + 1) << ": " << path[i] << "\n";
        }
        out << "\n";
    }

    out << "Generate " << k << " different possible next reasoning steps.\n";
    out << "Each step should be a distinct approach or sub-problem decomposition.\n";
    out << "Format: one step per line, prefixed with a number.\n";
    return out.str();
}

std::string TreeOfThoughtsBuilder::buildEvaluationPrompt(
    const std::string& problem,
    const ToTNode& node)
{
    std::ostringstream out;
    out << "Evaluate the following partial reasoning path for the problem below.\n\n";
    out << "Problem:\n" << problem << "\n\n";

    if (!node.path.empty()) {
        out << "Reasoning path:\n";
        for (size_t i = 0; i < node.path.size(); ++i) {
            out << "Step " << (i + 1) << ": " << node.path[i] << "\n";
        }
    }
    out << "Current step: " << node.thought << "\n\n";

    out << "Rate this reasoning path on a scale of 0–10.\n";
    out << "Then assign a verdict: sure / maybe / impossible.\n";
    out << "Format:\n  Score: <number>\n  Verdict: <sure|maybe|impossible>\n";
    return out.str();
}

std::string TreeOfThoughtsBuilder::buildSynthesisPrompt(
    const std::string& problem,
    const std::vector<std::string>& best_path)
{
    std::ostringstream out;
    out << "You have completed a reasoning process for the problem below.\n\n";
    out << "Problem:\n" << problem << "\n\n";
    out << "Best reasoning chain:\n";
    for (size_t i = 0; i < best_path.size(); ++i) {
        out << "Step " << (i + 1) << ": " << best_path[i] << "\n";
    }
    out << "\nSynthesize a concise, accurate final answer based on the reasoning above.\n";
    return out.str();
}

} // namespace prompt_engineering
} // namespace themis

