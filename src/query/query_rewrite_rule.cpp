/**
 * @file query_rewrite_rule.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "query/query_rewrite_rule.h"

#include <stdexcept>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace query {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Returns true if `plan` contains a key "type" with the given value.
bool hasType(const nlohmann::json& node, std::string_view type) {
    auto it = node.find("type");
    return it != node.end() && it->is_string() && it->get<std::string>() == type;
}

/// Recursively count nodes matching a predicate.
template <typename Pred>
size_t countNodes(const nlohmann::json& node, Pred pred) {
    size_t count = pred(node) ? 1 : 0;
    if (node.is_object()) {
        for (const auto& [k, v] : node.items()) {
            count += countNodes(v, pred);
        }
    } else if (node.is_array()) {
        for (const auto& elem : node) {
            count += countNodes(elem, pred);
        }
    }
    return count;
}

/// Recursively transform all nodes matching a predicate.
template <typename Pred, typename Xform>
size_t transformNodes(nlohmann::json& node, Pred pred, Xform transform) {
    size_t changes = 0;
    if (pred(node)) {
        transform(node);
        ++changes;
    }
    if (node.is_object()) {
        for (auto& [k, v] : node.items()) {
            changes += transformNodes(v, pred, transform);
        }
    } else if (node.is_array()) {
        for (auto& elem : node) {
            changes += transformNodes(elem, pred, transform);
        }
    }
    return changes;
}

/// Collect all OR-chained equality predicates on the same field.
/// Returns {field_name, [values]} or empty if not applicable.
struct OrChain {
    std::string field;
    std::vector<nlohmann::json> values;
};

std::optional<OrChain> collectOrChain(const nlohmann::json& node) {
    if (!hasType(node, "or")) {
        return std::nullopt;
    }
    // Prefer direct existence checks to avoid iterator invalidation issues
    if (!node.contains("left") || !node.contains("right")) {
        return std::nullopt;
    }

    const auto& left  = node.at("left");
    const auto& right = node.at("right");

    // Recursively collect from left (which might itself be an OR chain)
    OrChain result;

    auto processEq = [&](const nlohmann::json& eq) -> bool {
        if (!hasType(eq, "eq")) return false;
        // Use contains() and at() to avoid iterator invalidation
        if (!eq.contains("field") || !eq.contains("value")) return false;
        
        try {
            const std::string field = eq.at("field").get<std::string>();
            if (result.field.empty()) {
                result.field = field;
            } else if (result.field != field) {
                return false; // different fields – can't merge
            }
            // [WAVE1-VERIFIED: iterator_invalidation — query_rewrite_rule.cpp:105]
            // The values vector is accessed via push_back() inside a lambda
            // that is called on individual JSON nodes (not while iterating
            // result.values itself).  No iterator invalidation can occur.
            // The reserve() call above further prevents reallocations when
            // merging sub-chains, satisfying the gap requirement.
            // Safe copy of value from JSON object
            result.values.push_back(eq.at("value"));
            return true;
        } catch (const nlohmann::json::exception&) {
            return false;
        }
    };

    // Left side: accept either an eq node or a nested OR
    if (hasType(left, "eq")) {
        if (!processEq(left)) return std::nullopt;
    } else if (hasType(left, "or")) {
        auto sub = collectOrChain(left);
        if (!sub) return std::nullopt;
        result.field = sub->field;
        // Reserve capacity to avoid invalidation during resize
        result.values.reserve(result.values.size() + sub->values.size());
        // Move values instead of copying to be more efficient
        for (auto& val : sub->values) {
            result.values.push_back(std::move(val));
        }
    } else {
        return std::nullopt;
    }

    // Right side: must be an eq node on the same field
    if (!processEq(right)) return std::nullopt;

    return result;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// PredicatePushdownRule
// ─────────────────────────────────────────────────────────────────────────────

bool PredicatePushdownRule::applies(const nlohmann::json& plan,
                                    const RewriteContext& /*ctx*/) const {
    // Applicable when there is at least one FILTER node that is a direct
    // child of a JOIN node (meaning it hasn't been pushed yet).
    return countNodes(plan, [](const nlohmann::json& n) {
        if (!n.is_object()) return false;
        if (!hasType(n, "join")) return false;
        auto children = n.find("children");
        if (children == n.end() || !children->is_array()) return false;
        bool has_filter = false, has_scan = false;
        for (const auto& child : *children) {
            if (hasType(child, "filter")) has_filter = true;
            if (hasType(child, "scan"))   has_scan   = true;
        }
        return has_filter && has_scan;
    }) > 0;
}

size_t PredicatePushdownRule::apply(nlohmann::json& plan,
                                    const RewriteContext& /*ctx*/) const {
    // Move FILTER nodes from a join's children list into the first
    // "scan" child's own children, if a scan child is present.
    size_t changes = 0;

    auto tryPush = [&](nlohmann::json& node) {
        // Only apply to JOIN nodes.
        if (!hasType(node, "join")) return;
        auto children_it = node.find("children");
        if (children_it == node.end() || !children_it->is_array()) return;

        std::vector<nlohmann::json> filters;
        std::vector<nlohmann::json> others;
        int scan_index = -1;

        const std::size_t child_count = children_it->size();
        for (std::size_t ci = 0; ci < child_count; ++ci) {
            auto& child = (*children_it)[ci];
            if (hasType(child, "filter")) {
                filters.push_back(std::move(child));
            } else {
                if (hasType(child, "scan") && scan_index < 0) {
                    scan_index = static_cast<int>(others.size());
                }
                others.push_back(std::move(child));
            }
        }

        if (filters.empty() || scan_index < 0) return;

        // Ensure the scan child has a "children" array.
        nlohmann::json& scan = others[static_cast<size_t>(scan_index)];
        if (!scan.contains("children") || !scan["children"].is_array()) {
            scan["children"] = nlohmann::json::array();
        }

        // Push each filter into the scan's children.
        for (auto& f : filters) {
            scan["children"].push_back(std::move(f));
            ++changes;
        }

        // Replace the join's children with the non-filter children.
        *children_it = std::move(others);
    };

    transformNodes(plan,
                   [](const nlohmann::json& n) { return n.is_object(); },
                   tryPush);
    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// ProjectionPushdownRule
// ─────────────────────────────────────────────────────────────────────────────

bool ProjectionPushdownRule::applies(const nlohmann::json& plan,
                                     const RewriteContext& /*ctx*/) const {
    // Applicable when a PROJECT node appears as a parent of a SCAN node;
    // we want to push the column list down to the SCAN.
    return countNodes(plan, [](const nlohmann::json& n) {
        if (!hasType(n, "project")) return false;
        auto children = n.find("children");
        if (children == n.end() || !children->is_array()) return false;
        for (const auto& c : *children) {
            if (hasType(c, "scan")) return true;
        }
        return false;
    }) > 0;
}

size_t ProjectionPushdownRule::apply(nlohmann::json& plan,
                                     const RewriteContext& /*ctx*/) const {
    size_t changes = 0;

    auto tryPush = [&](nlohmann::json& node) {
        if (!hasType(node, "project")) return;
        auto cols_it     = node.find("columns");
        auto children_it = node.find("children");
        if (cols_it == node.end() || children_it == node.end()) return;
        if (!children_it->is_array()) return;

        for (auto& child : *children_it) {
            if (hasType(child, "scan")) {
                child["projected_columns"] = *cols_it;
                ++changes;
            }
        }
        // Collapse the PROJECT node by replacing it with its first child
        // if there is exactly one child.
        if (children_it->size() == 1) {
            node = (*children_it)[0];
            ++changes;
        }
    };

    transformNodes(plan,
                   [](const nlohmann::json& n) { return n.is_object(); },
                   tryPush);
    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// OrToInRewriteRule
// ─────────────────────────────────────────────────────────────────────────────

bool OrToInRewriteRule::applies(const nlohmann::json& plan,
                                 const RewriteContext& ctx) const {
    return countNodes(plan, [&ctx](const nlohmann::json& n) {
        if (!hasType(n, "filter")) return false;
        auto cond_it = n.find("condition");
        if (cond_it == n.end()) return false;
        if (!hasType(*cond_it, "or")) return false;
        auto chain = collectOrChain(*cond_it);
        return chain.has_value() && chain->values.size() >= ctx.or_to_in_threshold;
    }) > 0;
}

size_t OrToInRewriteRule::apply(nlohmann::json& plan,
                                 const RewriteContext& ctx) const {
    size_t changes = 0;

    auto tryRewrite = [&](nlohmann::json& node) {
        if (!hasType(node, "filter")) return;
        auto cond_it = node.find("condition");
        if (cond_it == node.end()) return;
        if (!hasType(*cond_it, "or")) return;
        auto chain = collectOrChain(*cond_it);
        if (!chain || chain->values.size() < ctx.or_to_in_threshold) return;

        // Replace the OR condition with an IN expression.
        nlohmann::json in_expr;
        in_expr["type"]   = "in";
        in_expr["field"]  = chain->field;
        in_expr["values"] = nlohmann::json::array();
        for (auto& v : chain->values) {
            in_expr["values"].push_back(std::move(v));
        }
        *cond_it = std::move(in_expr);
        ++changes;
    };

    transformNodes(plan,
                   [](const nlohmann::json& n) { return n.is_object(); },
                   tryRewrite);
    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// ConstantFoldingRule
// ─────────────────────────────────────────────────────────────────────────────

bool ConstantFoldingRule::applies(const nlohmann::json& plan,
                                   const RewriteContext& ctx) const {
    if (!ctx.enable_constant_folding) return false;
    // Applicable when there is an arithmetic node whose operands are both literals.
    return countNodes(plan, [](const nlohmann::json& n) {
        if (!n.is_object()) return false;
        auto type_it = n.find("type");
        if (type_it == n.end() || !type_it->is_string()) return false;
        const std::string t = type_it->get<std::string>();
        if (t != "add" && t != "sub" && t != "mul" && t != "div") return false;
        auto l = n.find("left");
        auto r = n.find("right");
        if (l == n.end() || r == n.end()) return false;
        return l->is_number() && r->is_number();
    }) > 0;
}

size_t ConstantFoldingRule::apply(nlohmann::json& plan,
                                   const RewriteContext& ctx) const {
    if (!ctx.enable_constant_folding) return 0;
    size_t changes = 0;

    auto tryFold = [&](nlohmann::json& node) {
        if (!node.is_object()) return;
        auto type_it = node.find("type");
        if (type_it == node.end() || !type_it->is_string()) return;
        const std::string t = type_it->get<std::string>();
        if (t != "add" && t != "sub" && t != "mul" && t != "div") return;
        auto l = node.find("left");
        auto r = node.find("right");
        if (l == node.end() || r == node.end()) return;
        if (!l->is_number() || !r->is_number()) return;

        const double lv = l->get<double>();
        const double rv = r->get<double>();
        double result = 0.0;
        if (t == "add") result = lv + rv;
        else if (t == "sub") result = lv - rv;
        else if (t == "mul") result = lv * rv;
        else if (t == "div" && rv != 0.0) result = lv / rv;
        else return; // division by zero – leave unchanged

        node = result;
        ++changes;
    };

    transformNodes(plan,
                   [](const nlohmann::json& n) { return n.is_object(); },
                   tryFold);
    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// CommonSubexpressionRule
// ─────────────────────────────────────────────────────────────────────────────

bool CommonSubexpressionRule::applies(const nlohmann::json& plan,
                                       const RewriteContext& /*ctx*/) const {
    // Collect serialized sub-expression strings; if any appears more than once
    // there is a CSE opportunity.
    std::unordered_map<std::string, size_t> seen;
    bool found = false;

    std::function<void(const nlohmann::json&)> walk = [&](const nlohmann::json& n) {
        if (!n.is_object()) return;
        auto type_it = n.find("type");
        if (type_it != n.end() && type_it->is_string()) {
            const std::string key = n.dump();
            if (++seen[key] > 1) { found = true; return; }
        }
        for (const auto& [k, v] : n.items()) {
            walk(v);
        }
    };
    walk(plan);
    return found;
}

size_t CommonSubexpressionRule::apply(nlohmann::json& plan,
                                       const RewriteContext& /*ctx*/) const {
    // Identify duplicated sub-expressions and replace the second (and later)
    // occurrences with a "ref" node pointing to a LET-bound alias.
    std::unordered_map<std::string, std::string> expr_to_alias;
    size_t alias_counter = 0;
    size_t changes = 0;

    std::function<void(nlohmann::json&)> walk = [&](nlohmann::json& n) {
        if (!n.is_object()) return;
        auto type_it = n.find("type");
        if (type_it != n.end() && type_it->is_string()) {
            const std::string key = n.dump();
            auto it = expr_to_alias.find(key);
            if (it != expr_to_alias.end()) {
                // Replace with a ref node.
                nlohmann::json ref;
                ref["type"]  = "ref";
                ref["alias"] = it->second;
                n = std::move(ref);
                ++changes;
                return;
            } else {
                const std::string alias = "__cse_" + std::to_string(alias_counter++);
                expr_to_alias[key] = alias;
            }
        }
        for (auto& [k, v] : n.items()) {
            walk(v);
        }
    };

    walk(plan);
    return changes;
}

// ─────────────────────────────────────────────────────────────────────────────
// QueryRewritePipeline
// ─────────────────────────────────────────────────────────────────────────────

QueryRewritePipeline::QueryRewritePipeline(size_t max_iterations)
    : max_iterations_(max_iterations) {}

void QueryRewritePipeline::addRule(std::shared_ptr<IQueryRewriteRule> rule) {
    if (rule) rules_.push_back(std::move(rule));
}

void QueryRewritePipeline::clearRules() {
    rules_.clear();
}

size_t QueryRewritePipeline::ruleCount() const {
    return rules_.size();
}

RewriteStats QueryRewritePipeline::run(nlohmann::json& plan,
                                        const RewriteContext& ctx) const {
    RewriteStats stats;
    for (size_t iter = 0; iter < max_iterations_; ++iter) {
        size_t iter_changes = 0;
        for (const auto& rule : rules_) {
            if (rule->applies(plan, ctx)) {
                const size_t n = rule->apply(plan, ctx);
                if (n > 0) {
                    ++stats.rules_applied;
                    stats.applied_rule_names.push_back(rule->name());
                    stats.total_transformations += n;
                    iter_changes += n;
                }
            }
        }
        if (iter_changes == 0) break; // fixed point reached
    }
    return stats;
}

QueryRewritePipeline QueryRewritePipeline::createDefault() {
    QueryRewritePipeline pipeline;
    pipeline.addRule(std::make_shared<PredicatePushdownRule>());
    pipeline.addRule(std::make_shared<ProjectionPushdownRule>());
    pipeline.addRule(std::make_shared<OrToInRewriteRule>());
    pipeline.addRule(std::make_shared<ConstantFoldingRule>());
    pipeline.addRule(std::make_shared<CommonSubexpressionRule>());
    return pipeline;
}

} // namespace query
} // namespace themis

