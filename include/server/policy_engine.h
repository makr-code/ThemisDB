/**
 * @file policy_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <optional>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declaration to avoid pulling in the full AuditLogger header
namespace utils { class AuditLogger; }

// Simple Ranger-like Policy Engine (MVP)
// - Subject: users or wildcard "*"
// - Actions: read, write, delete, query, admin, vector.search, vector.write
// - Resources: path patterns (e.g., "/entities/users:*", "/query", "/vector/*")
// - Conditions (optional): allowed_ip_prefixes (e.g., "10.0.", "192.168."),
//   UTC time-window, User-Agent substring allowlist
// - Optional OPA (Open Policy Agent) integration via IPolicyEvaluator/OpaAdapter
//
// Configuration formats:
// - Supports JSON and YAML files for loading policies. Saving currently writes JSON.

class PolicyEngine {
public:
    struct Config {
        size_t max_policies = 0;
    };

    struct Policy {
        std::string id = {};
        std::string name;
        std::unordered_set<std::string> subjects;   // user ids or "*"
        std::unordered_set<std::string> actions;    // e.g., "read", "write", "query", "admin", "vector.search"
        std::vector<std::string> resources;         // simple path-prefix matching (starts_with)
        bool effect_allow = true;                   // allow=true, deny=false
        // Optional conditions
        std::vector<std::string> allowed_ip_prefixes; // any match passes; empty -> ignore
        // ABAC: UTC hour-of-day window (0–23, inclusive on both ends; -1 = no restriction)
        int time_window_utc_hours_start = -1;
        int time_window_utc_hours_end   = -1;
        // ABAC: User-Agent substring allowlist (any match passes; empty = no restriction)
        std::vector<std::string> allowed_user_agent_patterns;
    };

    struct Decision {
        bool allowed = true;            // default allow when no policies configured
        std::string policy_id;          // matched policy id
        std::string reason;             // explanation
    };

    struct IPolicyEvaluator {
        /**
         * @brief IPolicy Evaluator.
         * @return Return value.
         */
        virtual ~IPolicyEvaluator() = default;
        /**
         * @brief Evaluate.
         * @param[in] user_id Identifier of the user.
         * @param[in] action Input parameter.
         * @param[in] resource_path Path to the resource.
         * @param[in] client_ip Input parameter.
         * @param[in] user_agent Input parameter.
         * @return Return value.
         */
        virtual std::optional<Decision> evaluate(
            const std::string& user_id,
            const std::string& action,
            const std::string& resource_path,
            const std::optional<std::string>& client_ip,
            const std::optional<std::string>& user_agent) const = 0;
    };

    struct Metrics {
        std::atomic<uint64_t> policy_allow_total{0};
        std::atomic<uint64_t> policy_deny_total{0};
        std::atomic<uint64_t> policy_eval_total{0};
        std::atomic<uint64_t> opa_fallback_total{0};
    };

    PolicyEngine() = default;
    explicit PolicyEngine(const Config& config) : config_(config) {}

    // Load policies from JSON or YAML file (detected by extension)
    bool loadFromFile(const std::string& path, std::string* err = nullptr);
    // Save policies to JSON file
    bool saveToFile(const std::string& path, std::string* err = nullptr) const;

    bool reloadIfChanged(std::string* err = nullptr);

    // Replace all policies
    /**
     * @brief Set Policies.
     * @param[in] policies Input parameter.
     */
    void setPolicies(std::vector<Policy> policies);
    // Append single policy
    /**
     * @brief Add Policy.
     * @param[in] p Input parameter.
     */
    void addPolicy(const Policy& p);
    // Remove by id
    /**
     * @brief Remove a retention policy by name.
     * @param[in] id Input parameter.
     * @return True when the policy existed and was removed.
     */
    bool removePolicy(const std::string& id);
    // List
    /**
     * @brief List Policies.
     * @return Return value.
     */
    std::vector<Policy> listPolicies() const;

    // Evaluate
    Decision authorize(const std::string& user_id,
                       const std::string& action,
                       const std::string& resource_path,
                       const std::optional<std::string>& client_ip  = std::nullopt,
                       const std::optional<std::string>& user_agent = std::nullopt) const;

    const Metrics& getMetrics() const { return metrics_; }

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Set Opa Evaluator.
     * @param[in,out] evaluator Input/output parameter.
     * @details Implements setOpaEvaluator without additional internal calls.
     */
    void setOpaEvaluator(IPolicyEvaluator* evaluator) { opa_evaluator_ = evaluator; }

    // JSON helpers
    /**
     * @brief To Json.
     * @param[in] p Input parameter.
     * @return Return value.
     */
    static nlohmann::json toJson(const Policy& p);
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static std::optional<Policy> fromJson(const nlohmann::json& j);

private:
    /**
     * @brief Match Subject.
     * @param[in] p Input parameter.
     * @param[in] user_id Identifier of the user.
     * @return True when the operation succeeds.
     */
    bool matchSubject(const Policy& p, const std::string& user_id) const;
    /**
     * @brief Match Action.
     * @param[in] p Input parameter.
     * @param[in] action Input parameter.
     * @return True when the operation succeeds.
     */
    bool matchAction(const Policy& p, const std::string& action) const;
    /**
     * @brief Match Resource.
     * @param[in] p Input parameter.
     * @param[in] resource_path Path to the resource.
     * @return True when the operation succeeds.
     */
    bool matchResource(const Policy& p, const std::string& resource_path) const;
    /**
     * @brief Match Conditions.
     * @param[in] p Input parameter.
     * @param[in] client_ip Input parameter.
     * @param[in] user_agent Input parameter.
     * @return True when the operation succeeds.
     */
    bool matchConditions(const Policy& p,
                         const std::optional<std::string>& client_ip,
                         const std::optional<std::string>& user_agent) const;

    mutable std::mutex mutex_;
    Config config_;
    std::vector<Policy> policies_;
    mutable Metrics metrics_;
    utils::AuditLogger* audit_logger_ = nullptr;  // optional; non-owning
    IPolicyEvaluator*   opa_evaluator_ = nullptr;  // optional OPA evaluator; non-owning

    // Hot-reload state
    std::string loaded_file_path_;
    std::chrono::system_clock::time_point last_loaded_mtime_;
};

} // namespace themis
