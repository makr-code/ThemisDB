/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_engine.h                                    ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-25 08:31:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     150                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • e58f4c7a6  2026-02-22  feat(governance): add hot-reload for policy YAML with Pol... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <mutex>
#include <filesystem>

namespace themis {
namespace utils {
    class AuditLogger;
}

namespace governance {

struct ClassificationProfile {
    std::string level;  // offen, vs-nfd, geheim, streng-geheim
    bool encryption_required = false;
    bool ann_allowed = true;
    bool export_allowed = true;
    bool cache_allowed = true;
    std::string redaction_level = "standard";
    int retention_days = 365;
    bool log_encryption = false;
};

struct PolicyDecision {
    // Normalized classification: "offen", "vs-nfd", "geheim", "streng-geheim"
    std::string classification;
    // Mode: "enforce" | "observe"
    std::string mode;
    // Whether logs must be encrypted (Encrypt-then-Sign handled by logger)
    bool encrypt_logs = true;
    // Redaction profile: "none" | "standard" | "strict"
    std::string redaction = "standard";

    // Derived, route-relevant decisions
    bool ann_allowed = true;                 // Approximate NN allowed
    bool require_content_encryption = false; // Content blobs must be encrypted
    bool export_allowed = true;
    bool cache_allowed = true;
    int retention_days = 365;

    // CCPA/CPRA: set to true when the data subject has opted out of data sale.
    // When true, callers must not share or export this subject's data to third
    // parties.  PolicyEngine::evaluate() sets this flag automatically when a
    // subject ID is present in the headers and the subject is registered in the
    // opt-out registry via setCcpaOptOutSubjects().
    bool ccpa_opted_out = false;
};

/// Request passed to simulateDecision() for dry-run policy preview.
struct SimulationRequest {
    std::unordered_map<std::string, std::string> headers;
    std::string route;
};

/// Result returned by simulateDecision().
/// Contains the computed PolicyDecision plus dry-run metadata.
/// No audit entry is written when this result is produced.
struct SimulationResult {
    PolicyDecision decision;         // The computed access decision
    std::string matched_profile;     // Classification profile used ("" = heuristic fallback)
    std::string matched_resource;    // Resource-mapping key that resolved the classification
    bool dry_run = true;             // Always true; confirms no audit entry was written
};

class PolicyEngine {
public:
    PolicyEngine() = default;

    // Load policies from YAML file (returns false on error)
    bool loadFromYAML(const std::string& yaml_path);

    /**
     * @brief Reload policies if the source YAML file has changed on disk.
     *
     * Checks the modification time of the file last passed to loadFromYAML().
     * If the file has been modified, the new policy set is loaded atomically.
     * If the path is empty or the file has not changed, this is a fast no-op.
     *
     * @param err  Optional: populated with a human-readable error message on
     *             failure (file read error, parse error, etc.).
     * @return true  if policies are up-to-date (either reloaded or unchanged),
     *         false on error.
     */
    bool reloadIfChanged(std::string* err = nullptr);

    /// @return The file path last passed to loadFromYAML(), or empty string if
    ///         no file has been loaded yet.
    std::string getLoadedFilePath() const;

    // Set audit logger for automatic logging of policy evaluations
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    // ---- CCPA/CPRA opt-out registry ----------------------------------------

    /// Register a set of data subject IDs that have opted out of data sale.
    /// PolicyEngine::evaluate() will set PolicyDecision::ccpa_opted_out=true
    /// and PolicyDecision::export_allowed=false for any request whose
    /// "X-User-Id" header matches a subject in this registry.
    /// Thread-safe; atomically replaces the previous registry.
    void setCcpaOptOutSubjects(std::shared_ptr<std::unordered_set<std::string>> opt_out_registry);

    /// Return true if the given subject ID is registered as opted-out.
    bool isCcpaOptedOut(const std::string& subject_id) const;

    // Evaluate headers for a given route key (e.g., "/vector/search" or handler name)
    // If audit logger is set and mode is "enforce", logs the policy decision
    PolicyDecision evaluate(const std::unordered_map<std::string, std::string>& headers,
                            const std::string& route) const;

    /// Evaluate policies in dry-run (simulation) mode without writing an audit entry.
    ///
    /// Performs the same classification lookup, profile resolution, and header-override
    /// steps as evaluate(), but intentionally suppresses audit logging so that the
    /// caller can preview the access decision without any side effects on the audit
    /// trail.  This satisfies the "deterministic and side-effect-free" requirement for
    /// policy_validator.cpp dry-run usage described in FUTURE_ENHANCEMENTS.md.
    ///
    /// @param request  The simulation request (headers + route).
    /// @return SimulationResult containing the decision and which rule/profile was matched.
    SimulationResult simulateDecision(const SimulationRequest& request) const;

    // Get classification profile by name
    std::optional<ClassificationProfile> getClassificationProfile(const std::string& level) const;

    static bool isStrictClass(const std::string& cls);

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ClassificationProfile> classification_profiles_;
    std::unordered_map<std::string, std::string> resource_mapping_;
    std::string default_mode_ = "enforce";
    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;

    // Hot-reload state
    std::string loaded_yaml_path_;
    std::filesystem::file_time_type last_loaded_mtime_{};

    // CCPA/CPRA opt-out registry (may be null – treated as empty)
    std::shared_ptr<std::unordered_set<std::string>> ccpa_opt_out_subjects_;

    static std::string normalize(const std::string& s);
};

} // namespace governance
} // namespace themis
