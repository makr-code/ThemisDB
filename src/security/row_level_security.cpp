/**
 * @file row_level_security.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/row_level_security.h"
#include "security/access_control_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <stdexcept>

namespace themis {
namespace security {

// ============================================================================
// RLSPredicate helpers
// ============================================================================

namespace {

/// Retrieve the value to compare against from the security context.
/// Returns empty string when the key is not found.
std::string resolveUserAttr(const std::string& user_attr, const SecurityContext& ctx) {
    if (user_attr == "user_id") {
        return ctx.user_id;
    }
    auto it = ctx.attributes.find(user_attr);
    if (it != ctx.attributes.end()) {
        return it->second;
    }
    return {};
}

/// Compare two JSON scalars (string / number / bool) with a given operator.
/// Returns false if either operand cannot be compared.
bool compareScalar(const nlohmann::json& lhs, const std::string& op, const nlohmann::json& rhs) {
    if (op == "eq") {
      return lhs == rhs;
    }
    if (op == "ne") {
      return lhs != rhs;
    }

    // Numeric comparisons require both sides to be numbers.
    if (lhs.is_number() && rhs.is_number()) {
        double l = lhs.get<double>();
        double r = rhs.get<double>();
        if (op == "lt") {
          return l < r;
        }
        if (op == "le") {
          return l <= r;
        }
        if (op == "gt") {
          return l > r;
        }
        if (op == "ge") {
          return l >= r;
        }
    }

    // String comparisons.
    if (lhs.is_string() && rhs.is_string()) {
        const auto& l = lhs.get_ref<const std::string&>();
        const auto& r = rhs.get_ref<const std::string&>();
        if (op == "lt") {
          return l < r;
        }
        if (op == "le") {
          return l <= r;
        }
        if (op == "gt") {
          return l > r;
        }
        if (op == "ge") {
          return l >= r;
        }
    }

    return false;
}

} // anonymous namespace

// ============================================================================
// RLSPredicate::evaluate
// ============================================================================

bool RLSPredicate::evaluate(const nlohmann::json& row, const SecurityContext& ctx) const {
    if (field.empty() || op.empty()) {
        THEMIS_WARN("RLSPredicate: empty field or op – predicate trivially true");
        return true;
    }

    // Retrieve the row field value.
    if (!row.contains(field)) {
        // Field absent: the predicate fails (deny by default for absent fields).
        THEMIS_DEBUG("RLSPredicate: field '{}' not present in row – predicate false", field);
        return false;
    }
    const nlohmann::json& row_val = row.at(field);

    // Resolve the right-hand side.
    nlohmann::json rhs_json;
    if (!user_attr.empty()) {
        // Dynamic: derive from security context.
        std::string attr_val = resolveUserAttr(user_attr, ctx);
        rhs_json = attr_val;
    } else {
        // Static: parse the stored value string.
        try {
            rhs_json = nlohmann::json::parse(value);
        } catch (...) {
            // Treat value as plain string if not valid JSON.
            rhs_json = value;
        }
    }

    // "in" operator: check membership in an array.
    if (op == "in") {
        if (rhs_json.is_array()) {
            return std::any_of(
                rhs_json.begin(), rhs_json.end(),
                [&row_val](const nlohmann::json& elem) { return elem == row_val; }
            );
        }
        THEMIS_WARN("RLSPredicate: 'in' operator requires array rhs – predicate false");
        return false;
    }

    return compareScalar(row_val, op, rhs_json);
}

// ============================================================================
// RLSPredicate serialisation
// ============================================================================

nlohmann::json RLSPredicate::toJson() const {
    return {
        {"field",     field},
        {"op",        op},
        {"value",     value},
        {"user_attr", user_attr}
    };
}

RLSPredicate RLSPredicate::fromJson(const nlohmann::json& j) {
    RLSPredicate p;
    p.field     = j.value("field",     "");
    p.op        = j.value("op",        "eq");
    p.value     = j.value("value",     "");
    p.user_attr = j.value("user_attr", "");
    return p;
}

// ============================================================================
// RLSPolicy serialisation
// ============================================================================

nlohmann::json RLSPolicy::toJson() const {
    return {
        {"id",               id},
        {"collection",       collection},
        {"applicable_roles", applicable_roles},
        {"predicate",        predicate.toJson()},
        {"type",             (type == RLSPolicyType::PERMISSIVE) ? "permissive" : "restrictive"},
        {"enabled",          enabled}
    };
}

RLSPolicy RLSPolicy::fromJson(const nlohmann::json& j) {
    RLSPolicy p;
    p.id         = j.value("id",         "");
    p.collection = j.value("collection", "");
    p.enabled    = j.value("enabled",    true);

    if (j.contains("applicable_roles") && j["applicable_roles"].is_array()) {
        for (const auto& r : j["applicable_roles"]) {
            p.applicable_roles.push_back(r.get<std::string>());
        }
    }

    if (j.contains("predicate") && j["predicate"].is_object()) {
        p.predicate = RLSPredicate::fromJson(j["predicate"]);
    }

    std::string type_str = j.value("type", "permissive");
    p.type = (type_str == "restrictive") ? RLSPolicyType::RESTRICTIVE
                                         : RLSPolicyType::PERMISSIVE;
    return p;
}

// ============================================================================
// RLSManager – policy management
// ============================================================================

void RLSManager::addPolicy(const RLSPolicy& policy) {
    if (policy.id.empty()) {
        throw std::invalid_argument("RLSManager::addPolicy: policy id must not be empty");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    policies_[policy.id] = policy;
    THEMIS_INFO("RLSManager: registered policy '{}' for collection '{}'",
                policy.id, policy.collection.empty() ? "*" : policy.collection);
}

bool RLSManager::removePolicy(const std::string& policy_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = policies_.find(policy_id);
    if (it == policies_.end()) {
        return false;
    }
    policies_.erase(it);
    THEMIS_INFO("RLSManager: removed policy '{}'", policy_id);
    return true;
}

std::optional<RLSPolicy> RLSManager::getPolicy(const std::string& policy_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = policies_.find(policy_id);
    if (it != policies_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> RLSManager::listPolicies() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids = {};

    ids.reserve(policies_.size());
    for (const auto& [id, _] : policies_) {
        ids.push_back(id);
    }
    return ids;
}

void RLSManager::clearPoliciesForCollection(const std::string& collection) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = policies_.begin(); it != policies_.end();) {
        if (it->second.collection == collection) {
            it = policies_.erase(it);
        } else {
            ++it;
        }
    }
    THEMIS_INFO("RLSManager: cleared all policies for collection '{}'",
                collection.empty() ? "*" : collection);
}

void RLSManager::clearAllPolicies() {
    std::lock_guard<std::mutex> lock(mutex_);
    policies_.clear();
    THEMIS_INFO("RLSManager: cleared all policies");
}

size_t RLSManager::loadFromJson(const nlohmann::json& j) {
    if (!j.contains("policies") || !j["policies"].is_array()) {
        return 0;
    }
    size_t loaded = 0;
    for (const auto& pj : j["policies"]) {
        try {
            RLSPolicy p = RLSPolicy::fromJson(pj);
            if (!p.id.empty()) {
                addPolicy(p);
                ++loaded;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("RLSManager::loadFromJson: skipping malformed policy: {}", e.what());
        }
    }
    return loaded;
}

nlohmann::json RLSManager::toJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& [_, p] : policies_) {
        arr.push_back(p.toJson());
    }
    return {{"policies", arr}};
}

// ============================================================================
// RLSManager – internal helpers
// ============================================================================

std::vector<const RLSPolicy*> RLSManager::matchingPolicies(
    const std::string& collection,
    const SecurityContext& ctx
) const {
    // Called with mutex_ held.
    std::vector<const RLSPolicy*> matches = {};

    for (const auto& [_, policy] : policies_) {
        if (!policy.enabled) {
            continue;
        }
        // Collection filter: empty policy.collection means "all collections".
        if (!policy.collection.empty() && policy.collection != collection) {
            continue;
        }
        // Role filter: empty applicable_roles means "all users".
        if (!policy.applicable_roles.empty()) {
            bool role_match = false;
            for (const auto& required_role : policy.applicable_roles) {
                if (std::find(ctx.roles.begin(), ctx.roles.end(), required_role)
                    != ctx.roles.end()) {
                    role_match = true;
                    break;
                }
            }
            if (!role_match) {
                continue;
            }
        }
        matches.push_back(&policy);
    }
    return matches;
}

// ============================================================================
// RLSManager – row filtering
// ============================================================================

bool RLSManager::isActive(
    const std::string& collection,
    const SecurityContext& ctx
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !matchingPolicies(collection, ctx).empty();
}

nlohmann::json RLSManager::filterRows(
    const std::string& collection,
    const SecurityContext& ctx,
    const nlohmann::json& rows
) const {
    if (!rows.is_array()) {
        return rows;  // Not an array – pass through unchanged.
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto applicable = matchingPolicies(collection, ctx);

    if (applicable.empty()) {
        // No RLS for this collection/user – all rows pass.
        return rows;
    }

    // Partition into permissive and restrictive.
    std::vector<const RLSPolicy*> permissive_policies;
    std::vector<const RLSPolicy*> restrictive_policies = {};

    for (const auto* p : applicable) {
        if (p->type == RLSPolicyType::PERMISSIVE) {
            permissive_policies.push_back(p);
        } else {
            restrictive_policies.push_back(p);
        }
    }

    nlohmann::json result = nlohmann::json::array();
    size_t total = rows.size();
    size_t removed = 0;

    for (const auto& row : rows) {
        bool passes = true;

        // Permissive gate: row passes if ANY permissive policy allows it.
        if (!permissive_policies.empty()) {
            bool any_permissive_allows = false;
            for (const auto* p : permissive_policies) {
                if (p->predicate.evaluate(row, ctx)) {
                    any_permissive_allows = true;
                    break;
                }
            }
            if (!any_permissive_allows) {
                passes = false;
            }
        }

        // Restrictive gate: row passes only if ALL restrictive policies allow it.
        if (passes && !restrictive_policies.empty()) {
            for (const auto* p : restrictive_policies) {
                if (!p->predicate.evaluate(row, ctx)) {
                    passes = false;
                    break;
                }
            }
        }

        if (passes) {
            result.push_back(row);
        } else {
            ++removed;
        }
    }

    if (removed > 0) {
        THEMIS_DEBUG("RLSManager: filtered {} of {} rows for user '{}' on collection '{}'",
                     removed, total, ctx.user_id,
                     collection.empty() ? "<all>" : collection);
    }

    return result;
}

} // namespace security
} // namespace themis

