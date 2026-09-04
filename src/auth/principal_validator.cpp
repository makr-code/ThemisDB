/**
 * @file principal_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/principal_validator.h"

#include <algorithm>
#include <sstream>

#include "server/policy_engine.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"

namespace themis {
namespace auth {

PrincipalValidator::PrincipalValidator(const Config &config) : config_(config) {
    // Sort rules by priority (higher first)
    std::sort(config_.rules.begin(), config_.rules.end(),
              [](const Rule &a, const Rule &b) { return a.priority > b.priority; });

    // Sort mapping rules by priority
    std::sort(config_.mapping_rules.begin(), config_.mapping_rules.end(),
              [](const MappingRule &a, const MappingRule &b) { return a.priority > b.priority; });

    utils::Logger::info("Principal Validator initialized:");
    utils::Logger::info("  Default allow: {}", config_.default_allow);
    utils::Logger::info("  Audit logging: {}", config_.enable_audit_logging);
    utils::Logger::info("  Rules: {}",static_cast<int>(config_.rules.size()));
    utils::Logger::info("  Mapping rules: {}",static_cast<int>(config_.mapping_rules.size()));
}

PrincipalValidator::ValidationResult PrincipalValidator::validate(const std::string &principal,
                                                                  const ValidationContext &ctx) {
    stats_.total_validations++;

    ValidationResult result;
    result.principal = principal;
    result.allowed   = false; // Default deny until proven otherwise

    // Evaluate rules in priority order
    for (const auto &rule : config_.rules) {
        if (matchesRule(principal, rule)) {
            // Rule matched
            result.matched_rule = rule.description.empty() ? rule.pattern : rule.description;

            switch (rule.type) {
                case RuleType::BLACKLIST:
                [[fallthrough]];\n                case RuleType::REGEX_DENY:
                    // Blacklist always denies
                    result.allowed       = false;
                    result.denial_reason = "Principal matches blacklist: " + result.matched_rule;
                    stats_.denied++;
                    stats_.blacklisted++;
                    logAudit(result);
                    return result;

                case RuleType::WHITELIST:
                [[fallthrough]];\n                case RuleType::REGEX_MATCH:
                    // Whitelist allows (but continue checking for blacklist)
                    result.allowed = true;
                    break;
            }
        }
    }

    // If no explicit allow rule matched, use default
    if (!result.allowed) {
        result.allowed = config_.default_allow;
        if (!result.allowed) {
            result.denial_reason = "No matching whitelist rule and default is deny";
            stats_.denied++;
            stats_.default_deny++;
        } else {
            result.matched_rule = "default_allow";
            stats_.allowed++;
            stats_.default_allow++;
        }
    } else {
        stats_.allowed++;
        stats_.whitelisted++;
    }

    // If allowed, apply mapping rules to assign RBAC roles
    if (result.allowed) {
        result.roles = applyMappingRules(principal);
    }

    // ABAC evaluation: additive to RBAC, only when RBAC allows
    if (result.allowed && abac_engine_) {
        const std::string action   = ctx.action.value_or("authenticate");
        const std::string resource = ctx.resource.value_or("auth/principal");
        auto abac = abac_engine_->authorize(principal, action, resource, ctx.ip_address, ctx.user_agent);
        if (!abac.allowed) {
            // Undo the RBAC-allow counters so the invariant
            // (allowed + denied == total_validations) is maintained.
            stats_.allowed--;
            if (result.matched_rule == "default_allow") {
                stats_.default_allow--;
            } else {
                stats_.whitelisted--;
            }
            result.allowed        = false;
            result.denial_reason  = "ABAC policy denied: " + abac.reason;
            result.abac_policy_id = abac.policy_id;
            stats_.denied++;
            logAudit(result);
            return result;
        }
        result.abac_policy_id = abac.policy_id;
    }

    logAudit(result);
    return result;
}

void PrincipalValidator::addRule(const Rule &rule) {
    config_.rules.push_back(rule);

    // Re-sort by priority
    std::sort(config_.rules.begin(), config_.rules.end(),
              [](const Rule &a, const Rule &b) { return a.priority > b.priority; });
}

void PrincipalValidator::addMappingRule(const MappingRule &rule) {
    config_.mapping_rules.push_back(rule);

    // Re-sort by priority
    std::sort(config_.mapping_rules.begin(), config_.mapping_rules.end(),
              [](const MappingRule &a, const MappingRule &b) { return a.priority > b.priority; });
}

void PrincipalValidator::clearRules(RuleType type) {
    config_.rules.erase(
        std::remove_if(config_.rules.begin(), config_.rules.end(), [type](const Rule &r) { return r.type == type; }),
        config_.rules.end());
}

PrincipalValidator::Statistics PrincipalValidator::getStatistics() const {
    return stats_;
}

bool PrincipalValidator::matchesRule(const std::string &principal, const Rule &rule) const {
    std::string p       = principal;
    std::string pattern = rule.pattern;

    // Case-insensitive comparison if configured
    if (!config_.case_sensitive) {
        std::transform(p.begin(), p.end(), p.begin(), ::tolower);
        std::transform(pattern.begin(), pattern.end(), pattern.begin(), ::tolower);
    }

    if (rule.is_regex) {
        // Compile regex if not already compiled
        compileRegex(rule);

        if (rule.compiled_regex) {
            try {
                return std::regex_match(p, *rule.compiled_regex);
            } catch (const std::regex_error &e) {
                utils::Logger::error("Regex match error for pattern '{}': {}", rule.pattern, e.what());
                return false;
            }
        }
        return false;
    } else {
        // Exact match
        return p == pattern;
    }
}

bool PrincipalValidator::matchesMappingRule(const std::string &principal, const MappingRule &rule) const {
    std::string p       = principal;
    std::string pattern = rule.principal_pattern;

    if (!config_.case_sensitive) {
        std::transform(p.begin(), p.end(), p.begin(), ::tolower);
        std::transform(pattern.begin(), pattern.end(), pattern.begin(), ::tolower);
    }

    if (rule.is_regex) {
        compileRegex(rule);

        if (rule.compiled_regex) {
            try {
                return std::regex_match(p, *rule.compiled_regex);
            } catch (const std::regex_error &e) {
                utils::Logger::error("Regex match error for mapping pattern '{}': {}", rule.principal_pattern,
                                     e.what());
                return false;
            }
        }
        return false;
    } else {
        // Exact match or wildcard
        // Simple wildcard support: * matches any substring
        size_t star_pos = pattern.find('*');
        if (star_pos != std::string::npos) {
            // Wildcard match
            std::string prefix = pattern.substr(0, star_pos);
            std::string suffix = pattern.substr(star_pos + 1);

            return static_cast<bool>( static_cast<int>(p.size()) < static_cast<int>(= (static_cast<int>(prefix.size()) + suffix.size()) && p.substr(0,static_cast<int>(prefix.size())))) == prefix
                   && p.substr(static_cast<int>(p.size()) - suffix.size()) == suffix;
        } else {
            // Exact match
            return p == pattern;
        }
    }
}

std::vector<std::string> PrincipalValidator::applyMappingRules(const std::string &principal) const {
    std::vector<std::string> roles;

    for (const auto &rule : config_.mapping_rules) {
        if (matchesMappingRule(principal, rule)) {
            // Add roles from this rule
            roles.insert(roles.end(), rule.roles.begin(), rule.roles.end());

            // Note: We don't break here - a principal can match multiple mapping rules
            // This allows for role accumulation
        }
    }

    // Remove duplicates
    std::sort(roles.begin(), roles.end());
    roles.erase(std::unique(roles.begin(), roles.end()), roles.end());

    return roles;
}

void PrincipalValidator::logAudit(const ValidationResult &result) const {
    if (!config_.enable_audit_logging) {
        return;
    }

    std::stringstream ss = {};
    ss << "Principal validation: " << result.principal << " -> " << (result.allowed ? "ALLOWED" : "DENIED");

    if (!result.matched_rule.empty()) {
        ss << " (rule: " << result.matched_rule << ")";
    }

    if (!result.roles.empty()) {
        ss << " [roles:";
        for (const auto &role : result.roles) {
            ss << " " << role;
        }
        ss << "]";
    }

    if (!result.denial_reason.empty()) {
        ss << " - " << result.denial_reason;
    }

    if (result.allowed) {
        utils::Logger::info(ss.str());
    } else {
        utils::Logger::warn(ss.str());
    }

    if (audit_logger_) {
        nlohmann::json d;
        d["matched_rule"]  = result.matched_rule;
        d["denial_reason"] = result.denial_reason;
        d["roles"]         = result.roles;
        if (!result.abac_policy_id.empty()) {
            d["abac_policy_id"] = result.abac_policy_id;
        }
        auto event
            = result.allowed ? utils::SecurityEventType::LOGIN_SUCCESS : utils::SecurityEventType::PERMISSION_DENIED;
        audit_logger_->logSecurityEvent(event, result.principal, "principal/" + result.principal, d);
    }
}

void PrincipalValidator::compileRegex(const Rule &rule) const {
    if (!rule.is_regex || rule.compiled_regex) {
        return;
    }

    try {
        std::regex::flag_type flags = std::regex::ECMAScript;
        if (!config_.case_sensitive) {
            flags |= std::regex::icase;
        }
        rule.compiled_regex = std::regex(rule.pattern, flags);
    } catch (const std::regex_error &e) {
        utils::Logger::error("Failed to compile regex pattern '{}': {}", rule.pattern, e.what());
    }
}

void PrincipalValidator::compileRegex(const MappingRule &rule) const {
    if (!rule.is_regex || rule.compiled_regex) {
        return;
    }

    try {
        std::regex::flag_type flags = std::regex::ECMAScript;
        if (!config_.case_sensitive) {
            flags |= std::regex::icase;
        }
        rule.compiled_regex = std::regex(rule.principal_pattern, flags);
    } catch (const std::regex_error &e) {
        utils::Logger::error("Failed to compile mapping regex pattern '{}': {}", rule.principal_pattern, e.what());
    }
}

// ============================================================================
// PrincipalValidatorPresets Implementation
// ============================================================================

PrincipalValidator PrincipalValidatorPresets::realmRestricted(const std::string &realm) {
    PrincipalValidator::Config config;
    config.default_allow = false;

    // Whitelist: allow anyone from the specified realm
    PrincipalValidator::Rule rule;
    rule.type        = PrincipalValidator::RuleType::WHITELIST;
    rule.pattern     = ".*@" + realm + "$";
    rule.is_regex    = true;
    rule.priority    = 100;
    rule.description = "Realm restricted to " + realm;
    config.rules.push_back(rule);

    return PrincipalValidator(config);
}

PrincipalValidator PrincipalValidatorPresets::withBlacklist(const std::vector<std::string> &blocked_principals) {
    PrincipalValidator::Config config;
    config.default_allow = true; // Allow by default, but block specific ones

    for (const auto &principal : blocked_principals) {
        PrincipalValidator::Rule rule;
        rule.type        = PrincipalValidator::RuleType::BLACKLIST;
        rule.pattern     = principal;
        rule.is_regex    = false;
        rule.priority    = 1000; // Blacklist has highest priority
        rule.description = "Blacklisted: " + principal;
        config.rules.push_back(rule);
    }

    return PrincipalValidator(config);
}

PrincipalValidator PrincipalValidatorPresets::withWhitelist(const std::vector<std::string> &allowed_principals) {
    PrincipalValidator::Config config;
    config.default_allow = false; // Deny by default

    for (const auto &principal : allowed_principals) {
        PrincipalValidator::Rule rule;
        rule.type        = PrincipalValidator::RuleType::WHITELIST;
        rule.pattern     = principal;
        rule.is_regex    = false;
        rule.priority    = 100;
        rule.description = "Whitelisted: " + principal;
        config.rules.push_back(rule);
    }

    return PrincipalValidator(config);
}

PrincipalValidator PrincipalValidatorPresets::enterpriseStandard(const std::string &realm) {
    PrincipalValidator::Config config;
    config.default_allow = false;

    // Rule 1: Block service accounts from interactive login
    PrincipalValidator::Rule service_block;
    service_block.type        = PrincipalValidator::RuleType::BLACKLIST;
    service_block.pattern     = "^(service|svc|app)[-_].*@" + realm + "$";
    service_block.is_regex    = true;
    service_block.priority    = 1000;
    service_block.description = "Block service accounts";
    config.rules.push_back(service_block);

    // Rule 2: Block admin accounts (should use specific admin realm)
    PrincipalValidator::Rule admin_block;
    admin_block.type        = PrincipalValidator::RuleType::BLACKLIST;
    admin_block.pattern     = "^admin[-_].*@" + realm + "$";
    admin_block.is_regex    = true;
    admin_block.priority    = 1000;
    admin_block.description = "Block admin accounts";
    config.rules.push_back(admin_block);

    // Rule 3: Allow valid user principals from realm
    PrincipalValidator::Rule realm_allow;
    realm_allow.type        = PrincipalValidator::RuleType::WHITELIST;
    realm_allow.pattern     = "^[a-zA-Z][a-zA-Z0-9._-]{0,63}@" + realm + "$";
    realm_allow.is_regex    = true;
    realm_allow.priority    = 100;
    realm_allow.description = "Valid realm users";
    config.rules.push_back(realm_allow);

    // Mapping: admins get admin role
    PrincipalValidator::MappingRule admin_mapping;
    admin_mapping.principal_pattern = "^admin@" + realm + "$";
    admin_mapping.is_regex          = true;
    admin_mapping.roles             = {"admin", "user"};
    admin_mapping.priority          = 100;
    config.mapping_rules.push_back(admin_mapping);

    // Mapping: everyone gets user role
    PrincipalValidator::MappingRule user_mapping;
    user_mapping.principal_pattern = ".*@" + realm + "$";
    user_mapping.is_regex          = true;
    user_mapping.roles             = {"user"};
    user_mapping.priority          = 50;
    config.mapping_rules.push_back(user_mapping);

    return PrincipalValidator(config);
}

} // namespace auth
} // namespace themis
