/**
 * @file updates_config.h
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
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace updates {

using json = nlohmann::json;

// Forward declaration: runtime CanaryConfig lives in canary_rollout.h.
// We forward-declare it here to avoid a circular include chain and to allow
// UpdatesConfig::CanaryConfig::toCanaryConfig() to return it by value in
// the header.  The full definition is included by users who need CanaryRollout.
struct CanaryConfig;

/**
 * @brief Configuration for update checker and hot-reload system
 */
struct UpdatesConfig {
    // Update Checker Settings
    struct CheckerConfig {
        bool enabled = false;                           // Enable update checker
        std::chrono::seconds check_interval{3600};      // Check interval (default: 1 hour)
        std::string github_owner = "makr-code";
        std::string github_repo = "ThemisDB";
        std::string github_api_url = "https://api.github.com";
        std::string github_api_token;                   // Optional API token for higher rate limits
        std::string proxy_url;                          // Optional proxy
    } checker;
    
    // Auto-Update Settings
    struct AutoUpdateConfig {
        bool enabled = false;                           // Enable auto-update
        bool critical_only = true;                      // Only auto-apply critical security updates
        bool require_approval = true;                   // Require manual approval even for critical updates
        std::chrono::seconds approval_timeout{300};     // Auto-approve after timeout (0 = never)
        
        // Schedule settings
        bool scheduled = false;                         // Use scheduled updates
        std::string schedule_time = "02:00";            // Time to apply updates (HH:MM format)
        std::vector<std::string> schedule_days = {"Sunday"}; // Days of week
    } auto_update;
    
    // Hot-Reload Settings
    struct HotReloadConfig {
        bool enabled = false;                           // Enable hot-reload capability
        std::string download_directory = "/tmp/themis_updates";
        std::string backup_directory = "/var/lib/themisdb/rollback";
        std::string install_directory = ".";
        bool verify_signatures = true;                  // Always verify signatures
        bool create_backup = true;                      // Always create backup
        int keep_rollback_points = 3;                   // Number of rollback points to keep
        
        // Download settings
        int download_timeout_seconds = 300;             // Download timeout per file
        int max_retries = 3;                            // Max download retries
        int retry_delay_seconds = 5;                    // Delay between retries
    } hot_reload;
    
    // Notification Settings
    struct NotificationConfig {
        bool enabled = false;                           // Enable notifications
        std::vector<std::string> on_events = {          // Events to notify on
            "update_available",
            "critical_update",
            "update_applied",
            "update_failed",
            "rollback_performed"
        };
        std::string webhook_url;                        // Webhook URL for notifications
        std::string email_to;                           // Email address for notifications
    } notifications;

    // Canary Rollout Settings
    struct CanaryConfig {
        bool enabled = false;                           // Enable canary rollout mode
        std::string node_id;                            // Stable identifier for this node
        double error_rate_threshold = 0.05;             // Error rate triggering auto-rollback
        size_t min_sample_count = 20;                   // Minimum events before threshold check

        // Rollout stages: each entry is (fraction_0_to_1, observation_seconds).
        // Default four-stage: 1% → 5% → 25% → 100%
        struct Stage {
            double percentage = 1.0;
            int observation_seconds = 0;
        };
        std::vector<Stage> stages = {
            {0.01, 3600},
            {0.05, 7200},
            {0.25, 21600},
            {1.00, 0},
        };

        /**
         * @brief Convert to the runtime CanaryConfig used by CanaryRollout.
         *
         * @param version  Target version string to roll out (e.g., "1.5.0").
         * @return         Runtime CanaryConfig ready for constructing a CanaryRollout.
         *
         * Usage:
         * @code
         *   auto cfg = updates_config.canary.toCanaryConfig("1.5.0");
         *   CanaryRollout rollout(engine, cfg);
         * @endcode
         */
        ::themis::updates::CanaryConfig toCanaryConfig(const std::string& version) const;
    } canary;

    // Anonymous Hardware Telemetry Settings
    struct TelemetryConfig {
        bool enabled = false;                  // Master on/off – opt-in only

        // HTTP(S) endpoint that receives the JSON telemetry payload.
        // Override this in updates.yaml to point at your own collector.
        std::string endpoint_url = "https://api.themisdb.org/telemetry.php";

        // How often to send a report (seconds).  Minimum enforced: 86400 (24 h).
        int send_interval_seconds = 86400;

        // Fine-grained field switches – all default to true when telemetry
        // is enabled, so operators can strip individual fields if desired.
        bool include_cpu_model = true;
        bool include_cpu_cores = true;
        bool include_ram_mb    = true;
        bool include_os        = true;
        bool include_arch      = true;

        // HTTP timeout per send attempt (seconds).
        int http_timeout_seconds = 10;

        // Maximum number of consecutive send retries before skipping an interval.
        int max_retries = 2;
    } telemetry;

    /**
     * @brief Load configuration from YAML file
     */
    static UpdatesConfig loadFromYaml(const std::string& yaml_path);
    
    /**
     * @brief Load configuration from JSON
     */
    static UpdatesConfig fromJson(const json& j);
    
    /**
     * @brief Convert to JSON
     */
    json toJson() const;
    
    /**
     * @brief Save configuration to YAML file
     */
    void saveToYaml(const std::string& yaml_path) const;
};

} // namespace updates
} // namespace themis
