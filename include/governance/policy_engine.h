/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_engine.h                                    ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:55:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     93                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <unordered_map>
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

    // Evaluate headers for a given route key (e.g., "/vector/search" or handler name)
    // If audit logger is set and mode is "enforce", logs the policy decision
    PolicyDecision evaluate(const std::unordered_map<std::string, std::string>& headers,
                            const std::string& route) const;

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

    static std::string normalize(const std::string& s);
};

} // namespace governance
} // namespace themis
