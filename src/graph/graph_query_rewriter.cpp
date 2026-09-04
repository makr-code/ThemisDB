/**
 * @file graph_query_rewriter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// GraphQueryRewriter – automatic query rewriting for graph optimization.

#include "graph/graph_query_rewriter.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// Anonymous helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Return true when `node` has a "type" key with the given string value.
bool hasType(const nlohmann::json &node, std::string_view t) {
    auto it = node.find("type");
    return it != node.end() && it->is_string() && it->get<std::string>() == t;
}

/// Count JSON nodes satisfying a predicate (recursive).
template <typename Pred> size_t countNodes(const nlohmann::json &node, Pred pred) {
    size_t count = pred(node) ? 1 : 0;
    if (node.is_object()) {
        for (const auto &[k, v] : node.items()) {
            count += countNodes(v, pred);
        }
    } else if (node.is_array()) {
        for (const auto &elem : node) {
            count += countNodes(elem, pred);
        }
    }
    return count;
}

/// Transform JSON nodes satisfying a predicate in-place (recursive).
template <typename Pred, typename Xform> size_t transformNodes(nlohmann::json &node, Pred pred, Xform xform) {
    size_t changes = 0;
    if (pred(node)) {
        xform(node);
        ++changes;
    }
    if (node.is_object()) {
        for (auto &[k, v] : node.items()) {
            changes += transformNodes(v, pred, xform);
        }
    } else if (node.is_array()) {
        for (auto &elem : node) {
            changes += transformNodes(elem, pred, xform);
        }
    }
    return changes;
}

/// Heuristic cardinality estimate for a plan node.
/// Lower value = smaller expected result set = more selective.
double estimateCardinalityImpl(const nlohmann::json &node) {
    if (!node.is_object()) {
        return 1.0;
    }

    auto type_it = node.find("type");
    if (type_it == node.end()) {
        return 1.0;
    }
    const std::string t = type_it->get<std::string>();

    if (t == "graph_traversal" || t == "multi_traversal") {
        double base = 1.0;
        // Deeper traversal = more nodes visited
        auto depth_it = node.find("max_depth");
        if (depth_it != node.end() && depth_it->is_number()) {
            base *= std::pow(2.0, depth_it->get<double>());
        }
        // Each filter reduces expected result set
        auto filters_it = node.find("vertex_filters");
        if (filters_it != node.end() && filters_it->is_array()) {
            base /= std::max(1.0, static_cast<double>(filters_it->size()) * 2.0);
        }
        // Prune conditions also reduce result size
        auto prune_it = node.find("prune_conditions");
        if (prune_it != node.end() && prune_it->is_array()) {
            base /= std::max(1.0, static_cast<double>(prune_it->size()) * 3.0);
        }
        // Multi-traversal scales with start vertex count
        if (t == "multi_traversal") {
            auto sv_it = node.find("start_vertices");
            if (sv_it != node.end() && sv_it->is_array()) {
                base *= std::max(1.0, static_cast<double>(sv_it->size()));
            }
        }
        return base;
    }

    if (t == "filter_scan") {
        // Pre-filter reduces cardinality before the child traversal
        auto child_it     = node.find("child");
        double child_card = child_it != node.end() ? estimateCardinalityImpl(*child_it) : 1.0;
        return child_card / 4.0; // heuristic: filter removes ~75 % of rows
    }

    return 1.0;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

GraphQueryRewriter::GraphQueryRewriter(const std::optional<RewriteConfig> &config)
    : config_(config.value_or(RewriteConfig())) {}

// ─────────────────────────────────────────────────────────────────────────────
// Helper: isEnabled
// ─────────────────────────────────────────────────────────────────────────────

bool GraphQueryRewriter::isEnabled(RewriteRule rule) const {
    // Empty set = all rules enabled
    if (config_.enabled_rules.empty()) {
        return true;
    }
    return config_.enabled_rules.count(rule) > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// rewrite
// ─────────────────────────────────────────────────────────────────────────────

GraphQueryRewriter::RewriteResult GraphQueryRewriter::rewrite(const nlohmann::json &plan) const {
    using Clock           = std::chrono::steady_clock;
    const auto t0         = Clock::now();
    const double limit_ms = config_.rewrite_time_limit_ms;

    nlohmann::json current = plan;
    GraphRewriteStats stats;

    auto elapsed_ms = [&]() -> double { return std::chrono::duration<double, std::milli>(Clock::now() - t0).count(); };

    auto record = [&](const std::string &name, size_t n) {
        if (n > 0) {
            ++stats.rules_applied;
            stats.applied_rule_names.push_back(name);
            stats.total_transformations += n;
        }
    };

    for (size_t iter = 0; iter < kMaxIterations; ++iter) {
        if (limit_ms > 0.0 && elapsed_ms() >= limit_ms) {
            break;
        }

        size_t iter_changes = 0;

        // ── PREDICATE_PUSHDOWN / PRUNE_EARLY ─────────────────────────────
        if (isEnabled(RewriteRule::PREDICATE_PUSHDOWN) || isEnabled(RewriteRule::PRUNE_EARLY)) {
            size_t n = applyPredicatePushdown(current);
            record("PredicatePushdown", n);
            iter_changes += n;
        }

        if (limit_ms > 0.0 && elapsed_ms() >= limit_ms) {
            break;
        }

        // ── COMMON_SUBEXPRESSION ──────────────────────────────────────────
        if (isEnabled(RewriteRule::COMMON_SUBEXPRESSION)) {
            size_t n = applyCommonSubexpressionElimination(current);
            record("CommonSubexpressionElimination", n);
            iter_changes += n;
        }

        if (limit_ms > 0.0 && elapsed_ms() >= limit_ms) {
            break;
        }

        // ── JOIN_REORDERING ───────────────────────────────────────────────
        if (isEnabled(RewriteRule::JOIN_REORDERING)) {
            size_t n = applyJoinReordering(current);
            record("JoinReordering", n);
            iter_changes += n;
        }

        if (limit_ms > 0.0 && elapsed_ms() >= limit_ms) {
            break;
        }

        // ── MATERIALIZED_VIEW ─────────────────────────────────────────────
        if (isEnabled(RewriteRule::MATERIALIZED_VIEW)) {
            size_t n = applyMaterializedView(current, config_.aggressive_optimization);
            record("MaterializedView", n);
            iter_changes += n;
        }

        if (limit_ms > 0.0 && elapsed_ms() >= limit_ms) {
            break;
        }

        // ── QUERY_DECOMPOSITION ───────────────────────────────────────────
        if (isEnabled(RewriteRule::QUERY_DECOMPOSITION)) {
            size_t n = applyQueryDecomposition(current);
            record("QueryDecomposition", n);
            iter_changes += n;
        }

        if (limit_ms > 0.0 && elapsed_ms() >= limit_ms) {
            break;
        }

        // ── Custom rules ──────────────────────────────────────────────────
        for (const auto &cr : custom_rules_) {
            size_t n = cr.fn(current);
            record(cr.name, n);
            iter_changes += n;
        }

        if (iter_changes == 0) {
            break; // fixed point reached
        }
    }

    return {std::move(current), std::move(stats)};
}

// ─────────────────────────────────────────────────────────────────────────────
// explainRewrites
// ─────────────────────────────────────────────────────────────────────────────

std::string GraphQueryRewriter::explainRewrites(const nlohmann::json &original, const nlohmann::json &rewritten) const {
    std::ostringstream oss;
    oss << "Graph Query Rewrite Summary\n";
    oss << "═══════════════════════════\n";

    if (original == rewritten) {
        oss << "No rewrites applied — plan is already optimal.\n";
        return oss.str();
    }

    // Count prune conditions added
    size_t orig_prune = 0, new_prune = 0;
    countNodes(original, [&]([[maybe_unused]] const nlohmann::json &n) {
        if (!n.is_object()) {
            return false;
        }
        auto it = n.find("prune_conditions");
        if (it != n.end() && it->is_array()) {
            orig_prune += it->size();
        }
        return false;
    });
    countNodes(rewritten, [&]([[maybe_unused]] const nlohmann::json &n) {
        if (!n.is_object()) {
            return false;
        }
        auto it = n.find("prune_conditions");
        if (it != n.end() && it->is_array()) {
            new_prune += it->size();
        }
        return false;
    });
    if (new_prune > orig_prune) {
        oss << "• PredicatePushdown / PruneEarly: added " << (new_prune - orig_prune) << " prune condition(s) to graph"
            << " traversal node(s) — branches will be cut early during"
            << " BFS/DFS to reduce explored nodes.\n";
    }

    // Count CSE refs added
    size_t ref_count = countNodes(rewritten, [](const nlohmann::json &n) { return hasType(n, "ref"); });
    if (ref_count > 0) {
        oss << "• CommonSubexpressionElimination: introduced " << ref_count
            << " cached-ref node(s) — duplicate traversals replaced with"
            << " references to a single computed result.\n";
    }

    // Check join reordering
    bool join_reordered = false;
    countNodes(rewritten, [&]([[maybe_unused]] const nlohmann::json &n) {
        if (!hasType(n, "traversal_join")) {
            return false;
        }
        if (!hasType(original, "traversal_join")) {
            return false;
        }
        auto lo = original.find("left"), ro = original.find("right");
        auto ln = n.find("left"), rn = n.find("right");
        if (lo == original.end() || ln == n.end()) {
            return false;
        }
        if (*lo != *ln) {
            join_reordered = true;
        }
        return false;
    });
    if (join_reordered) {
        oss << "• JoinReordering: swapped traversal_join operands — the more"
            << " selective (lower-cardinality) side is now executed first.\n";
    }

    // Count materialized view tags
    size_t mat_count = countNodes(rewritten, [](const nlohmann::json &n) {
        auto it = n.find("use_materialized_view");
        return it != n.end() && it->is_boolean() && it->get<bool>();
    });
    if (mat_count > 0) {
        oss << "• MaterializedView: tagged " << mat_count
            << " traversal node(s) for sub-graph materialisation — repeated"
            << " access will use precomputed results.\n";
    }

    // Count decomposed subqueries
    size_t sq_count = countNodes(rewritten, [](const nlohmann::json &n) { return hasType(n, "parallel_subqueries"); });
    if (sq_count > 0) {
        oss << "• QueryDecomposition: decomposed multi-start traversal into " << sq_count
            << " independent parallel sub-queries.\n";
    }

    oss << "\nOriginal plan nodes : " << countNodes(original, [](const nlohmann::json &n) { return n.is_object(); })
        << "\nRewritten plan nodes: " << countNodes(rewritten, [](const nlohmann::json &n) { return n.is_object(); })
        << "\n";

    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// estimateSpeedup
// ─────────────────────────────────────────────────────────────────────────────

double GraphQueryRewriter::estimateSpeedup(const nlohmann::json &original, const nlohmann::json &rewritten) const {
    if (original == rewritten) {
        return 1.0;
    }

    double speedup = 1.0;

    // Predicate pushdown / prune early: each prune condition reduces the
    // effective search space by ~30 % on average.
    size_t orig_prune = 0, new_prune = 0;
    countNodes(original, [&]([[maybe_unused]] const nlohmann::json &n) {
        if (!n.is_object()) {
            return false;
        }
        auto it = n.find("prune_conditions");
        if (it != n.end() && it->is_array()) {
            orig_prune += it->size();
        }
        return false;
    });
    countNodes(rewritten, [&]([[maybe_unused]] const nlohmann::json &n) {
        if (!n.is_object()) {
            return false;
        }
        auto it = n.find("prune_conditions");
        if (it != n.end() && it->is_array()) {
            new_prune += it->size();
        }
        return false;
    });
    if (new_prune > orig_prune) {
        speedup *= std::pow(1.3, static_cast<double>(new_prune - orig_prune));
    }

    // CSE: each eliminated duplicate traversal avoids one full traversal pass.
    size_t ref_count = countNodes(rewritten, [](const nlohmann::json &n) { return hasType(n, "ref"); });
    if (ref_count > 0) {
        speedup *= (1.0 + 0.5 * static_cast<double>(ref_count));
    }

    // Join reordering: cutting the larger side first can halve intermediate
    // result sizes.
    size_t join_nodes = countNodes(rewritten, [](const nlohmann::json &n) { return hasType(n, "traversal_join"); });
    if (join_nodes > 0) {
        speedup *= std::pow(1.2, static_cast<double>(join_nodes));
    }

    // Materialised views: each materialised traversal avoids re-computation.
    size_t mat_count = countNodes(rewritten, [](const nlohmann::json &n) {
        auto it = n.find("use_materialized_view");
        return it != n.end() && it->is_boolean() && it->get<bool>();
    });
    if (mat_count > 0) {
        speedup *= (1.0 + 0.4 * static_cast<double>(mat_count));
    }

    // Query decomposition: parallelism gives near-linear speedup capped at
    // practical limits (~4×).
    size_t sq_count = countNodes(rewritten, [](const nlohmann::json &n) { return hasType(n, "parallel_subqueries"); });
    if (sq_count > 0) {
        // Each decomposed traversal can run in parallel; cap at 4×.
        auto sv_it = rewritten.find("subqueries");
        size_t nq  = sv_it != rewritten.end() && sv_it->is_array() ? sv_it->size() : 2;
        speedup *= std::min(4.0, static_cast<double>(nq) * 0.8);
    }

    return speedup;
}

// ─────────────────────────────────────────────────────────────────────────────
// addCustomRule / clearCustomRules
// ─────────────────────────────────────────────────────────────────────────────

void GraphQueryRewriter::addCustomRule(std::string_view name, std::function<size_t(nlohmann::json &)> rule) {
    if (!rule) {
        throw std::invalid_argument("GraphQueryRewriter::addCustomRule: null rule");
    }
    custom_rules_.push_back({std::string(name), std::move(rule)});
}

void GraphQueryRewriter::clearCustomRules() {
    custom_rules_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// estimateCardinality (public static)
// ─────────────────────────────────────────────────────────────────────────────

double GraphQueryRewriter::estimateCardinality(const nlohmann::json &node) {
    return estimateCardinalityImpl(node);
}

// ─────────────────────────────────────────────────────────────────────────────
// Rule: PredicatePushdown
// ─────────────────────────────────────────────────────────────────────────────

size_t GraphQueryRewriter::applyPredicatePushdown(nlohmann::json &plan) {
    size_t changes = 0;

    // ── Pass 1: For graph_traversal nodes, promote vertex_filters to
    //            prune_conditions so they are applied during traversal.
    auto promoteFilters = [&]([[maybe_unused]] nlohmann::json &node) {
        if (!hasType(node, "graph_traversal")) {
            return;
        }

        auto vf_it = node.find("vertex_filters");
        if (vf_it == node.end() || !vf_it->is_array() || vf_it->empty()) {
            return;
        }

        // Ensure prune_conditions array exists
        if (!node.contains("prune_conditions") || !node["prune_conditions"].is_array()) {
            node["prune_conditions"] = nlohmann::json::array();
        }

        // Copy each filter into prune_conditions (keep vertex_filters intact
        // for post-traversal verification — prune is an optimistic early cut)
        for (const auto &f : *vf_it) {
            // Avoid duplicate prune conditions
            bool already_pruned = false;
            for (const auto &p : node["prune_conditions"]) {
                if (p == f) {
                    already_pruned = true;
                    break;
                }
            }
            if (!already_pruned) {
                node["prune_conditions"].push_back(f);
                ++changes;
            }
        }
    };

    transformNodes(plan, [](const nlohmann::json &n) { return n.is_object(); }, promoteFilters);

    // ── Pass 2: For filter_scan nodes whose filter can be applied at the
    //            traversal level, push the filter into the child traversal.
    auto pushFilterScan = [&]([[maybe_unused]] nlohmann::json &node) {
        if (!hasType(node, "filter_scan")) {
            return;
        }

        auto filter_it = node.find("filter");
        auto child_it  = node.find("child");
        if (filter_it == node.end() || child_it == node.end()) {
            return;
        }
        if (!child_it->is_object()) {
            return;
        }
        if (!hasType(*child_it, "graph_traversal")) {
            return;
        }

        // Ensure vertex_filters array in child
        if (!child_it->contains("vertex_filters") || !(*child_it)["vertex_filters"].is_array()) {
            (*child_it)["vertex_filters"] = nlohmann::json::array();
        }

        // Push filter into child's vertex_filters
        bool already_present = false;
        for (const auto &f : (*child_it)["vertex_filters"]) {
            if (f == *filter_it) {
                already_present = true;
                break;
            }
        }
        if (!already_present) {
            (*child_it)["vertex_filters"].push_back(*filter_it);
            // Mark as pushed so we don't duplicate on next pass
            node["filter_pushed"] = true;
            ++changes;
        }
    };

    transformNodes(plan, [](const nlohmann::json &n) { return n.is_object(); }, pushFilterScan);

    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// Rule: CommonSubexpressionElimination
// ─────────────────────────────────────────────────────────────────────────────

size_t GraphQueryRewriter::applyCommonSubexpressionElimination(nlohmann::json &plan) {
    // Collect all graph_traversal sub-expressions and their occurrence count.
    std::unordered_map<std::string, size_t> seen;

    std::function<void(const nlohmann::json &)> collect = [&]([[maybe_unused]] const nlohmann::json &n) {
        if (!n.is_object()) {
            return;
        }
        if (hasType(n, "graph_traversal")) {
            seen[n.dump()]++;
        }
        for (const auto &[k, v] : n.items()) {
            collect(v);
        }
        if (n.is_array()) {
            for (const auto &elem : n) {
                collect(elem);
            }
        }
    };
    collect(plan);

    // Keep only expressions that appear more than once.
    std::unordered_map<std::string, std::string> expr_to_alias;
    size_t alias_counter = 0;
    for (const auto &[key, cnt] : seen) {
        if (cnt > 1) {
            expr_to_alias[key] = "__gqr_cse_" + std::to_string(alias_counter++);
        }
    }

    if (expr_to_alias.empty()) {
        return 0;
    }

    // Replace second-and-later occurrences with ref nodes.
    std::unordered_map<std::string, size_t> occurrence;
    size_t changes = 0;

    std::function<void(nlohmann::json &)> replace = [&]([[maybe_unused]] nlohmann::json &n) {
        if (!n.is_object()) {
            return;
        }
        if (hasType(n, "graph_traversal")) {
            const std::string key = n.dump();
            auto alias_it         = expr_to_alias.find(key);
            if (alias_it != expr_to_alias.end()) {
                const size_t occ = occurrence[key]++;
                if (occ > 0) {
                    // Replace with a reference node
                    nlohmann::json ref;
                    ref["type"]  = "ref";
                    ref["alias"] = alias_it->second;
                    n            = std::move(ref);
                    ++changes;
                    return;
                }
                // First occurrence: tag with the alias for documentation
                n["cse_alias"] = alias_it->second;
            }
        }
        for (auto &[k, v] : n.items()) {
            replace(v);
        }
    };

    replace(plan);

    // Wrap the plan in a let node for each aliased expression so that the
    // runtime knows to compute and cache these sub-results.
    if (changes > 0) {
        for (const auto &[key, alias] : expr_to_alias) {
            nlohmann::json let_node;
            let_node["type"]  = "let";
            let_node["alias"] = alias;
            try {
                let_node["expr"] = nlohmann::json::parse(key);
            } catch (...) {
                // If the key cannot be re-parsed (shouldn't happen), skip.
                continue;
            }
            // Prepend let node as a wrapper around the plan
            nlohmann::json wrapper;
            wrapper["type"]     = "let_scope";
            wrapper["bindings"] = nlohmann::json::array();
            wrapper["bindings"].push_back(std::move(let_node));
            wrapper["body"] = std::move(plan);
            plan            = std::move(wrapper);
            break; // only wrap once; additional lets are in the bindings array
        }
        // If there were multiple CSEs, add remaining bindings
        auto bindings_it = plan.find("bindings");
        if (bindings_it != plan.end() && bindings_it->is_array()) {
            for (const auto &[key, alias] : expr_to_alias) {
                // Check if this alias already exists in bindings
                bool exists = false;
                for (const auto &b : *bindings_it) {
                    auto a = b.find("alias");
                    if (a != b.end() && a->get<std::string>() == alias) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    nlohmann::json let_node;
                    let_node["type"]  = "let";
                    let_node["alias"] = alias;
                    try {
                        let_node["expr"] = nlohmann::json::parse(key);
                    } catch (...) {
                        continue;
                    }
                    bindings_it->push_back(std::move(let_node));
                }
            }
        }
    }

    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// Rule: JoinReordering
// ─────────────────────────────────────────────────────────────────────────────

size_t GraphQueryRewriter::applyJoinReordering(nlohmann::json &plan) {
    size_t changes = 0;

    auto reorder = [&]([[maybe_unused]] nlohmann::json &node) {
        if (!hasType(node, "traversal_join")) {
            return;
        }

        auto left_it  = node.find("left");
        auto right_it = node.find("right");
        if (left_it == node.end() || right_it == node.end()) {
            return;
        }

        const double left_card  = estimateCardinalityImpl(*left_it);
        const double right_card = estimateCardinalityImpl(*right_it);

        // Place the more selective (lower cardinality) side on the left so it
        // is executed first and the result set passed into the right side is
        // as small as possible.
        if (right_card < left_card) {
            std::swap(*left_it, *right_it);
            ++changes;
        }
    };

    transformNodes(plan, [](const nlohmann::json &n) { return n.is_object(); }, reorder);
    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// Rule: MaterializedView
// ─────────────────────────────────────────────────────────────────────────────

size_t GraphQueryRewriter::applyMaterializedView(nlohmann::json &plan, bool aggressive) {
    // First pass: count occurrences of each graph_traversal fingerprint.
    std::unordered_map<std::string, size_t> occurrence;
    countNodes(plan, [&]([[maybe_unused]] const nlohmann::json &n) {
        if (hasType(n, "graph_traversal")) {
            // Build fingerprint from graph_id + direction + depth (ignore start vertex)
            nlohmann::json fp;
            auto gi = n.find("graph_id");
            auto di = n.find("direction");
            auto md = n.find("max_depth");
            auto vf = n.find("vertex_filters");
            if (gi != n.end()) {
                fp["graph_id"] = *gi;
            }
            if (di != n.end()) {
                fp["direction"] = *di;
            }
            if (md != n.end()) {
                fp["max_depth"] = *md;
            }
            if (vf != n.end()) {
                fp["vertex_filters"] = *vf;
            }
            occurrence[fp.dump()]++;
        }
        return false;
    });

    size_t changes = 0;

    auto tagForMaterialization = [&]([[maybe_unused]] nlohmann::json &node) {
        if (!hasType(node, "graph_traversal")) {
            return;
        }
        if (node.contains("use_materialized_view") && node["use_materialized_view"].is_boolean()
            && node["use_materialized_view"].get<bool>()) {
            return; // already tagged
        }

        nlohmann::json fp;
        auto gi = node.find("graph_id");
        auto di = node.find("direction");
        auto md = node.find("max_depth");
        auto vf = node.find("vertex_filters");
        if (gi != node.end()) {
            fp["graph_id"] = *gi;
        }
        if (di != node.end()) {
            fp["direction"] = *di;
        }
        if (md != node.end()) {
            fp["max_depth"] = *md;
        }
        if (vf != node.end()) {
            fp["vertex_filters"] = *vf;
        }
        const std::string key = fp.dump();

        bool should_materialize = false;
        if (occurrence[key] > 1) {
            // Multiple queries access the same subgraph pattern
            should_materialize = true;
        } else if (aggressive) {
            // In aggressive mode, tag any non-trivial traversal
            double depth = 1.0;
            if (md != node.end() && md->is_number()) {
                depth = md->get<double>();
            }
            should_materialize = (depth >= 2.0);
        }

        if (should_materialize) {
            node["use_materialized_view"] = true;
            // Generate a deterministic view name
            node["materialized_view_name"] = "mv_" + std::to_string(std::hash<std::string>{}(key) & 0xFFFFFFFFULL);
            ++changes;
        }
    };

    transformNodes(plan, [](const nlohmann::json &n) { return n.is_object(); }, tagForMaterialization);
    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// Rule: QueryDecomposition
// ─────────────────────────────────────────────────────────────────────────────

size_t GraphQueryRewriter::applyQueryDecomposition(nlohmann::json &plan) {
    size_t changes = 0;

    auto decompose = [&]([[maybe_unused]] nlohmann::json &node) {
        if (!hasType(node, "multi_traversal")) {
            return;
        }

        auto sv_it = node.find("start_vertices");
        if (sv_it == node.end() || !sv_it->is_array() || sv_it->size() < 2) {
            return;
        }
        // Already decomposed?
        if (hasType(node, "parallel_subqueries")) {
            return;
        }

        // Build one graph_traversal sub-query per start vertex.
        nlohmann::json subqueries = nlohmann::json::array();
        for (const auto &sv : *sv_it) {
            nlohmann::json sub;
            sub["type"] = "graph_traversal";
            // Copy shared fields from the multi_traversal node
            auto copy_field = [&]([[maybe_unused]] const std::string &key) {
                auto it = node.find(key);
                if (it != node.end()) {
                    sub[key] = *it;
                }
            };
            copy_field("graph_id");
            copy_field("direction");
            copy_field("min_depth");
            copy_field("max_depth");
            copy_field("vertex_filters");
            copy_field("prune_conditions");
            sub["start_vertex"] = sv;
            subqueries.push_back(std::move(sub));
        }

        // Replace node with a parallel_subqueries wrapper
        node["type"]       = "parallel_subqueries";
        node["parallel"]   = true;
        node["subqueries"] = std::move(subqueries);
        // Remove the multi_traversal-specific fields
        node.erase("start_vertices");
        ++changes;
    };

    transformNodes(plan, [](const nlohmann::json &n) { return n.is_object(); }, decompose);
    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// Static factory helpers
// ─────────────────────────────────────────────────────────────────────────────

nlohmann::json GraphQueryRewriter::makeTraversalPlan(std::string_view graph_id, std::string_view start_vertex,
                                                     std::string_view direction, int min_depth, int max_depth,
                                                     nlohmann::json vertex_filters) {
    nlohmann::json plan;
    plan["type"]             = "graph_traversal";
    plan["graph_id"]         = std::string(graph_id);
    plan["start_vertex"]     = std::string(start_vertex);
    plan["direction"]        = std::string(direction);
    plan["min_depth"]        = min_depth;
    plan["max_depth"]        = max_depth;
    plan["vertex_filters"]   = std::move(vertex_filters);
    plan["prune_conditions"] = nlohmann::json::array();
    return plan;
}

nlohmann::json GraphQueryRewriter::makeFilterScanPlan(nlohmann::json filter, nlohmann::json child) {
    nlohmann::json plan;
    plan["type"]   = "filter_scan";
    plan["filter"] = std::move(filter);
    plan["child"]  = std::move(child);
    return plan;
}

nlohmann::json GraphQueryRewriter::makeJoinPlan(nlohmann::json left, nlohmann::json right, std::string_view join_key) {
    nlohmann::json plan;
    plan["type"]     = "traversal_join";
    plan["left"]     = std::move(left);
    plan["right"]    = std::move(right);
    plan["join_key"] = std::string(join_key);
    return plan;
}

nlohmann::json GraphQueryRewriter::makeMultiTraversalPlan(std::string_view graph_id,
                                                          const std::vector<std::string> &start_vertices,
                                                          std::string_view direction, int max_depth,
                                                          nlohmann::json vertex_filters) {
    nlohmann::json plan;
    plan["type"]             = "multi_traversal";
    plan["graph_id"]         = std::string(graph_id);
    plan["start_vertices"]   = start_vertices;
    plan["direction"]        = std::string(direction);
    plan["min_depth"]        = 1;
    plan["max_depth"]        = max_depth;
    plan["vertex_filters"]   = std::move(vertex_filters);
    plan["prune_conditions"] = nlohmann::json::array();
    return plan;
}

} // namespace graph
} // namespace themis

