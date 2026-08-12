/**
 * @file policy_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
        /// Maximum number of policies the engine will hold.
        /// addPolicy() throws std::length_error when the limit is reached.
        /// 0 means unlimited (default).
        size_t max_policies = 0;
    };

    struct Policy {
        std::string id;
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

    /**
     * @brief Pluggable policy evaluator interface for external engines (e.g. OPA).
     *
     * Implement this interface and pass it to setOpaEvaluator() to route
     * authorization decisions through an external policy agent.
     * evaluate() returns std::nullopt when the external evaluator is
     * unavailable so PolicyEngine can fall back to native evaluation.
     */
    struct IPolicyEvaluator {
        virtual ~IPolicyEvaluator() = default;
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
        /// Incremented each time OPA is unavailable and native evaluation is used instead.
        std::atomic<uint64_t> opa_fallback_total{0};
    };

    PolicyEngine() = default;
    explicit PolicyEngine(const Config& config) : config_(config) {}

    // Load policies from JSON or YAML file (detected by extension)
    bool loadFromFile(const std::string& path, std::string* err = nullptr);
    // Save policies to JSON file
    bool saveToFile(const std::string& path, std::string* err = nullptr) const;

    /**
     * @brief Reload policies from the file last passed to loadFromFile().
     *
     * Checks whether the file's modification time has changed since the last
     * load.  If it has, the file is re-read and the in-memory policy set is
     * atomically replaced.  If the path is empty or the file has not changed
     * the method is a fast no-op.
     *
     * @param err  Optional: populated with a human-readable error string on
     *             failure (file read error, parse error, etc.).
     * @return true  if the policies were successfully reloaded (or were already
     *              up-to-date), false on error.
     */
    bool reloadIfChanged(std::string* err = nullptr);

    // Replace all policies
    void setPolicies(std::vector<Policy> policies);
    // Append single policy
    void addPolicy(const Policy& p);
    // Remove by id
    bool removePolicy(const std::string& id);
    // List
    std::vector<Policy> listPolicies() const;

    // Evaluate
    Decision authorize(const std::string& user_id,
                       const std::string& action,
                       const std::string& resource_path,
                       const std::optional<std::string>& client_ip  = std::nullopt,
                       const std::optional<std::string>& user_agent = std::nullopt) const;

    const Metrics& getMetrics() const { return metrics_; }

    /**
     * @brief Attach an AuditLogger that will receive POLICY_UPDATED events
     *        whenever policies are added, removed, or reloaded.
     *
     * Pass nullptr to detach.  The PolicyEngine does NOT take ownership; the
     * caller must ensure the logger outlives the engine.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Attach an external policy evaluator (e.g. OPA) for fine-grained ABAC.
     *
     * When set, authorize() calls evaluator->evaluate() first.  If the
     * evaluator returns std::nullopt (OPA unreachable / timeout), native
     * PolicyEngine evaluation is used as a fallback and
     * metrics_.opa_fallback_total is incremented.
     *
     * Pass nullptr to detach.  The PolicyEngine does NOT take ownership; the
     * caller must ensure the evaluator outlives the engine.
     */
    void setOpaEvaluator(IPolicyEvaluator* evaluator) { opa_evaluator_ = evaluator; }

    // JSON helpers
    static nlohmann::json toJson(const Policy& p);
    static std::optional<Policy> fromJson(const nlohmann::json& j);

private:
    bool matchSubject(const Policy& p, const std::string& user_id) const;
    bool matchAction(const Policy& p, const std::string& action) const;
    bool matchResource(const Policy& p, const std::string& resource_path) const;
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
