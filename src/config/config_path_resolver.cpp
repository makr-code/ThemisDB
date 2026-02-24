/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_path_resolver.cpp                           ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     547                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b01c41c10  2026-02-22  fix(config): use thread-safe C++20 chrono date formatting... ║
    • 7f5ce7a1a  2026-02-22  feat(config): add DeprecationAggregator for legacy path u... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "config/config_path_resolver.h"
#include "config/config_errors.h"
#include "config/path_mapping_metadata.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <thread>
#include <unordered_map>

namespace themis {
namespace config {

// ═══════════════════════════════════════════════════════════
// DeprecationAggregator – tracks per-path legacy usage counts
// and emits periodic structured log reports.
// ═══════════════════════════════════════════════════════════

class ConfigPathResolver::DeprecationAggregator {
public:
    static constexpr int DEFAULT_INTERVAL_SECONDS = 300;

    ~DeprecationAggregator() {
        stop();
    }

    /**
     * Increment the usage counter for a legacy path.
     * Thread-safe; called from ConfigPathResolver::tryResolve().
     */
    void incrementUsage(const std::string& legacy_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        usage_counts_[legacy_path]++;
    }

    /**
     * Build and return a snapshot of the current deprecation report,
     * sorted by descending usage count.
     */
    std::vector<ConfigPathResolver::DeprecationEntry> getReport() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ConfigPathResolver::DeprecationEntry> report;
        report.reserve(usage_counts_.size());

        for (const auto& [path, count] : usage_counts_) {
            ConfigPathResolver::DeprecationEntry entry;
            entry.legacy_path = path;
            entry.usage_count = count;

            auto metadata = ConfigPathResolver::getMetadata(path);
            if (metadata) {
                entry.new_path           = metadata->new_path;
                entry.category           = metadata->category;
                entry.removal_date       = metadata->removal_date;
                entry.migration_guide_url = metadata->migration_guide_url;
            } else {
                entry.new_path = ConfigPathResolver::mapLegacyToNew(path);
                entry.category = ConfigPathResolver::inferCategory(entry.new_path);
            }
            report.push_back(std::move(entry));
        }

        std::sort(report.begin(), report.end(),
                  [](const DeprecationEntry& a, const DeprecationEntry& b) {
                      return a.usage_count > b.usage_count;
                  });
        return report;
    }

    /**
     * Reset all usage counters (called from ConfigPathResolver::resetMetrics()).
     */
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        usage_counts_.clear();
    }

    /**
     * Start the background reporter thread.
     *
     * @param interval_seconds How often to emit the aggregated report (default 300 s).
     */
    void start(int interval_seconds = DEFAULT_INTERVAL_SECONDS) {
        std::lock_guard<std::mutex> tlock(thread_mutex_);
        if (running_.load()) {
            return;  // Already running
        }
        running_ = true;
        interval_ = std::chrono::seconds(interval_seconds);
        reporter_thread_ = std::thread([this]() { reporterLoop(); });
    }

    /**
     * Stop the background reporter thread and join it.
     */
    void stop() {
        {
            std::lock_guard<std::mutex> lock(cv_mutex_);
            running_ = false;
        }
        cv_.notify_all();

        std::lock_guard<std::mutex> tlock(thread_mutex_);
        if (reporter_thread_.joinable()) {
            reporter_thread_.join();
        }
    }

    bool isRunning() const { return running_.load(); }

private:
    void reporterLoop() {
        while (running_.load()) {
            std::unique_lock<std::mutex> lock(cv_mutex_);
            cv_.wait_for(lock, interval_, [this]() { return !running_.load(); });
            if (running_.load()) {
                logReport();
            }
        }
    }

    void logReport() {
        auto report = getReport();
        if (report.empty()) {
            return;
        }

        spdlog::info("[CONFIG] Legacy path deprecation report ({} paths in use):",
                     report.size());
        for (const auto& entry : report) {
            std::string removal_str = "N/A";
            if (entry.removal_date.has_value()) {
                // Use chrono year_month_day for thread-safe, locale-independent formatting
                auto days = std::chrono::floor<std::chrono::days>(*entry.removal_date);
                std::chrono::year_month_day ymd{days};
                auto y = static_cast<int>(ymd.year());
                auto m = static_cast<unsigned>(ymd.month());
                auto d = static_cast<unsigned>(ymd.day());
                std::ostringstream oss;
                oss << y
                    << '-' << (m < 10 ? "0" : "") << m
                    << '-' << (d < 10 ? "0" : "") << d;
                removal_str = oss.str();
            }
            const std::string& guide_str =
                entry.migration_guide_url.value_or("N/A");

            spdlog::warn("[CONFIG] Legacy path report: "
                         "{{path: '{}', hits: {}, removal_date: '{}', guide: '{}'}}",
                         entry.legacy_path, entry.usage_count,
                         removal_str, guide_str);
        }
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, uint64_t> usage_counts_;

    std::mutex thread_mutex_;
    std::mutex cv_mutex_;
    std::condition_variable cv_;
    std::thread reporter_thread_;
    std::chrono::seconds interval_{DEFAULT_INTERVAL_SECONDS};
    std::atomic<bool> running_{false};
};

// ═══════════════════════════════════════════════════════════
// Cache Configuration: read env vars once at startup
// ═══════════════════════════════════════════════════════════

static ConfigPathResolver::CacheConfig readCacheEnvConfig() {
    size_t capacity = static_cast<size_t>(ConfigPathResolver::kCacheCapacity);
    int ttl_seconds = ConfigPathResolver::kCacheTtlSeconds;

    if (const char* env = std::getenv("THEMIS_CONFIG_CACHE_CAPACITY")) {
        try {
            int val = std::stoi(env);
            if (val >= 10 && val <= 100000) {
                capacity = static_cast<size_t>(val);
            }
            // Out-of-range values fall through to default silently at static-init time
        } catch (...) {
            // Invalid value: keep default
        }
    }

    if (const char* env = std::getenv("THEMIS_CONFIG_CACHE_TTL_SECONDS")) {
        try {
            int val = std::stoi(env);
            if (val >= 1 && val <= 86400) {
                ttl_seconds = val;
            }
        } catch (...) {
            // Invalid value: keep default
        }
    }

    return {capacity, ttl_seconds};
}

// ═══════════════════════════════════════════════════════════
// Static Members Initialization
// ═══════════════════════════════════════════════════════════

ConfigPathResolver::Metrics ConfigPathResolver::metrics_;
ConfigPathResolver::CacheConfig ConfigPathResolver::cache_config_ = readCacheEnvConfig();
LRUCacheWithTTL<std::string, std::string> ConfigPathResolver::cache_(
    ConfigPathResolver::cache_config_.capacity,
    ConfigPathResolver::cache_config_.ttl_seconds);
std::atomic<bool> ConfigPathResolver::caching_enabled_{true};
ConfigPathResolver::DeprecationAggregator ConfigPathResolver::aggregator_;
std::atomic<bool> ConfigPathResolver::aggregation_enabled_{false};

// ═══════════════════════════════════════════════════════════
// Path Mapping Table: Legacy → New
// ═══════════════════════════════════════════════════════════

const std::map<std::string, std::string> ConfigPathResolver::PATH_MAPPING = {
    // AI/ML Configurations
    {"config/lora_training_config.yaml", "config/ai_ml/lora_training_config.yaml"},
    {"config/vision_config.yaml", "config/ai_ml/vision/config.yaml"},
    {"config/llm_system_prompts.yaml", "config/ai_ml/llm/system_prompts.yaml"},
    {"config/rag_judge.yaml", "config/ai_ml/rag_judge.yaml"},
    {"config/voice_assistant.yaml", "config/ai_ml/voice_assistant.yaml"},
    
    // Security Configurations
    {"config/pii_patterns.yaml", "config/security/pii_patterns.yaml"},
    {"config/rbac_roles.json", "config/security/rbac_roles.json"},
    {"config/user_roles.json", "config/security/user_roles.json"},
    {"config/graph_protection.yaml", "config/security/graph_protection.yaml"},
    {"config/auth_kerberos.example.yaml", "config/security/auth_kerberos.example.yaml"},
    
    // Compliance & Ethics
    {"config/ethical_guidelines.yaml", "config/compliance/ethical_guidelines.yaml"},
    {"config/governance.yaml", "config/compliance/governance.yaml"},
    {"config/audit.yaml", "config/compliance/audit/audit.yaml"},
    {"config/ai_audit_config.yaml", "config/compliance/audit/ai_audit_config.yaml"},
    
    // Data Management
    {"config/mime_types.yaml", "config/data_management/mime_types.yaml"},
    {"config/storage_redundancy.yaml", "config/data_management/storage_redundancy.yaml"},
    {"config/retention_policies.yaml", "config/data_management/retention_policies.yaml"},
    {"config/cdc_retention.yaml", "config/data_management/cdc_retention.yaml"},
    
    // Performance Configurations
    {"config/scaling_optimizations.yaml", "config/performance/scaling_optimizations.yaml"},
    {"config/acceleration.yaml", "config/performance/acceleration.yaml"},
    {"config/config_2ssd_performance.yaml", "config/performance/config_2ssd_performance.yaml"},
    {"config/config_multi_ssd.yaml", "config/performance/config_multi_ssd.yaml"},
    {"config/query_cache_mixed.yaml", "config/performance/query_cache/mixed.yaml"},
    {"config/query_cache_olap.yaml", "config/performance/query_cache/olap.yaml"},
    {"config/query_cache_oltp.yaml", "config/performance/query_cache/oltp.yaml"},
    
    // Deprecated/Backup Files
    {"config/phase2_optimizations.json", "config/deprecated/phase2_optimizations.json"},
    {"config/phase3_optimizations.json", "config/deprecated/phase3_optimizations.json"},
    {"config/policies.json.backup", "config/deprecated/policies.json.backup"},
    
    // Core Configurations
    {"config/config.yaml", "config/core/config.yaml"},
    {"config/config-minimal.yaml", "config/core/config-minimal.yaml"},
    {"config/security.yaml", "config/core/security.yaml"},
    {"config/updates.yaml", "config/core/updates.yaml"},
    
    // Platform Configurations
    {"config/config.rpi3.json", "config/platform/rpi3.json"},
    {"config/config.rpi4.json", "config/platform/rpi4.json"},
    {"config/config.rpi5.json", "config/platform/rpi5.json"},
    {"config/config.qnap.json", "config/platform/qnap.json"},
    
    // Networking
    {"config/connection_pool_config.yaml", "config/networking/connection_pool_config.yaml"},
    
    // Content Processing
    {"config/content_processors.yaml", "config/content/processors.yaml"},
    {"config/fem_edge_type_defaults.yaml", "config/content/fem_edge_type_defaults.yaml"},
    
    // Monitoring
    {"config/prometheus_arm.yml", "config/monitoring/prometheus/arm.yml"},
    {"config/prometheus_ethics.yml", "config/monitoring/prometheus/ethics.yml"},
    
    // Features
    {"config/features.yaml.example", "config/features/features.example.yaml"},
    {"config/capability_auto_generation.yaml", "config/features/capability_auto_generation.yaml"},
    
    // Assistants
    {"config/docs_assistant.yaml", "config/assistants/docs_assistant.yaml"},
    {"config/feedback_config.yaml", "config/assistants/feedback_config.yaml"},
    
    // Processing
    {"config/cep_rules.yaml", "config/processing/cep_rules.yaml"},
    
    // Licensing
    {"config/community_license.default.json", "config/licensing/community/default.json"},
    {"config/community_license.example.json", "config/licensing/community/example.json"},
    {"config/enterprise_license.example.json", "config/licensing/enterprise/example.json"},
    {"config/enterprise_license.test.json", "config/licensing/enterprise/test.json"},
};

// ═══════════════════════════════════════════════════════════
// Metadata Table with Deprecation Information
// ═══════════════════════════════════════════════════════════

// Helper to create a date from ISO string (YYYY-MM-DD)
static std::chrono::system_clock::time_point parseDate(const std::string& iso_date) {
    // Simple parser for YYYY-MM-DD format
    // In production, use a proper date parsing library
    std::tm tm = {};
    std::istringstream ss(iso_date);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

const std::map<std::string, PathMappingMetadata> ConfigPathResolver::METADATA_TABLE = {
    // ── AI/ML Configurations ─────────────────────────────────────────────────
    {
        "config/lora_training_config.yaml",
        {
            "config/lora_training_config.yaml",
            "config/ai_ml/lora_training_config.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/vision_config.yaml",
        {
            "config/vision_config.yaml",
            "config/ai_ml/vision/config.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/llm_system_prompts.yaml",
        {
            "config/llm_system_prompts.yaml",
            "config/ai_ml/llm/system_prompts.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/rag_judge.yaml",
        {
            "config/rag_judge.yaml",
            "config/ai_ml/rag_judge.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/voice_assistant.yaml",
        {
            "config/voice_assistant.yaml",
            "config/ai_ml/voice_assistant.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Security Configurations ───────────────────────────────────────────────
    {
        "config/pii_patterns.yaml",
        {
            "config/pii_patterns.yaml",
            "config/security/pii_patterns.yaml",
            "security",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/rbac_roles.json",
        {
            "config/rbac_roles.json",
            "config/security/rbac_roles.json",
            "security",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/user_roles.json",
        {
            "config/user_roles.json",
            "config/security/user_roles.json",
            "security",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/graph_protection.yaml",
        {
            "config/graph_protection.yaml",
            "config/security/graph_protection.yaml",
            "security",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/auth_kerberos.example.yaml",
        {
            "config/auth_kerberos.example.yaml",
            "config/security/auth_kerberos.example.yaml",
            "security",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Compliance & Ethics ───────────────────────────────────────────────────
    {
        "config/ethical_guidelines.yaml",
        {
            "config/ethical_guidelines.yaml",
            "config/compliance/ethical_guidelines.yaml",
            "compliance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/governance.yaml",
        {
            "config/governance.yaml",
            "config/compliance/governance.yaml",
            "compliance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/audit.yaml",
        {
            "config/audit.yaml",
            "config/compliance/audit/audit.yaml",
            "compliance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/ai_audit_config.yaml",
        {
            "config/ai_audit_config.yaml",
            "config/compliance/audit/ai_audit_config.yaml",
            "compliance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Data Management ───────────────────────────────────────────────────────
    {
        "config/mime_types.yaml",
        {
            "config/mime_types.yaml",
            "config/data_management/mime_types.yaml",
            "data_management",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/storage_redundancy.yaml",
        {
            "config/storage_redundancy.yaml",
            "config/data_management/storage_redundancy.yaml",
            "data_management",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/retention_policies.yaml",
        {
            "config/retention_policies.yaml",
            "config/data_management/retention_policies.yaml",
            "data_management",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/cdc_retention.yaml",
        {
            "config/cdc_retention.yaml",
            "config/data_management/cdc_retention.yaml",
            "data_management",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Performance Configurations ────────────────────────────────────────────
    {
        "config/scaling_optimizations.yaml",
        {
            "config/scaling_optimizations.yaml",
            "config/performance/scaling_optimizations.yaml",
            "performance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/acceleration.yaml",
        {
            "config/acceleration.yaml",
            "config/performance/acceleration.yaml",
            "performance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/config_2ssd_performance.yaml",
        {
            "config/config_2ssd_performance.yaml",
            "config/performance/config_2ssd_performance.yaml",
            "performance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/config_multi_ssd.yaml",
        {
            "config/config_multi_ssd.yaml",
            "config/performance/config_multi_ssd.yaml",
            "performance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/query_cache_mixed.yaml",
        {
            "config/query_cache_mixed.yaml",
            "config/performance/query_cache/mixed.yaml",
            "performance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/query_cache_olap.yaml",
        {
            "config/query_cache_olap.yaml",
            "config/performance/query_cache/olap.yaml",
            "performance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/query_cache_oltp.yaml",
        {
            "config/query_cache_oltp.yaml",
            "config/performance/query_cache/oltp.yaml",
            "performance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Deprecated/Backup Files ───────────────────────────────────────────────
    {
        "config/phase2_optimizations.json",
        {
            "config/phase2_optimizations.json",
            "config/deprecated/phase2_optimizations.json",
            "deprecated",
            parseDate("2023-01-01"),
            parseDate("2025-12-31"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/phase3_optimizations.json",
        {
            "config/phase3_optimizations.json",
            "config/deprecated/phase3_optimizations.json",
            "deprecated",
            parseDate("2023-01-01"),
            parseDate("2025-12-31"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/policies.json.backup",
        {
            "config/policies.json.backup",
            "config/deprecated/policies.json.backup",
            "deprecated",
            parseDate("2023-01-01"),
            parseDate("2025-12-31"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Core Configurations ───────────────────────────────────────────────────
    {
        "config/config.yaml",
        {
            "config/config.yaml",
            "config/core/config.yaml",
            "core",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/config-minimal.yaml",
        {
            "config/config-minimal.yaml",
            "config/core/config-minimal.yaml",
            "core",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/security.yaml",
        {
            "config/security.yaml",
            "config/core/security.yaml",
            "core",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/updates.yaml",
        {
            "config/updates.yaml",
            "config/core/updates.yaml",
            "core",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Platform Configurations ───────────────────────────────────────────────
    {
        "config/config.rpi3.json",
        {
            "config/config.rpi3.json",
            "config/platform/rpi3.json",
            "platform",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/config.rpi4.json",
        {
            "config/config.rpi4.json",
            "config/platform/rpi4.json",
            "platform",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/config.rpi5.json",
        {
            "config/config.rpi5.json",
            "config/platform/rpi5.json",
            "platform",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/config.qnap.json",
        {
            "config/config.qnap.json",
            "config/platform/qnap.json",
            "platform",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Networking ────────────────────────────────────────────────────────────
    {
        "config/connection_pool_config.yaml",
        {
            "config/connection_pool_config.yaml",
            "config/networking/connection_pool_config.yaml",
            "networking",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Content Processing ────────────────────────────────────────────────────
    {
        "config/content_processors.yaml",
        {
            "config/content_processors.yaml",
            "config/content/processors.yaml",
            "content",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/fem_edge_type_defaults.yaml",
        {
            "config/fem_edge_type_defaults.yaml",
            "config/content/fem_edge_type_defaults.yaml",
            "content",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Monitoring ────────────────────────────────────────────────────────────
    {
        "config/prometheus_arm.yml",
        {
            "config/prometheus_arm.yml",
            "config/monitoring/prometheus/arm.yml",
            "monitoring",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/prometheus_ethics.yml",
        {
            "config/prometheus_ethics.yml",
            "config/monitoring/prometheus/ethics.yml",
            "monitoring",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Features ──────────────────────────────────────────────────────────────
    {
        "config/features.yaml.example",
        {
            "config/features.yaml.example",
            "config/features/features.example.yaml",
            "features",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/capability_auto_generation.yaml",
        {
            "config/capability_auto_generation.yaml",
            "config/features/capability_auto_generation.yaml",
            "features",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Assistants ────────────────────────────────────────────────────────────
    {
        "config/docs_assistant.yaml",
        {
            "config/docs_assistant.yaml",
            "config/assistants/docs_assistant.yaml",
            "assistants",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/feedback_config.yaml",
        {
            "config/feedback_config.yaml",
            "config/assistants/feedback_config.yaml",
            "assistants",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Processing ────────────────────────────────────────────────────────────
    {
        "config/cep_rules.yaml",
        {
            "config/cep_rules.yaml",
            "config/processing/cep_rules.yaml",
            "processing",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Licensing ─────────────────────────────────────────────────────────────
    {
        "config/community_license.default.json",
        {
            "config/community_license.default.json",
            "config/licensing/community/default.json",
            "licensing",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/community_license.example.json",
        {
            "config/community_license.example.json",
            "config/licensing/community/example.json",
            "licensing",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/enterprise_license.example.json",
        {
            "config/enterprise_license.example.json",
            "config/licensing/enterprise/example.json",
            "licensing",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/enterprise_license.test.json",
        {
            "config/enterprise_license.test.json",
            "config/licensing/enterprise/test.json",
            "licensing",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
};

// ═══════════════════════════════════════════════════════════
// Public API Implementation
// ═══════════════════════════════════════════════════════════

std::string ConfigPathResolver::resolve(const std::string& legacy_path) {
    auto result = tryResolve(legacy_path);
    if (result) {
        metrics_.resolution_hits++;
        return *result;
    }
    
    metrics_.resolution_misses++;
    
    // Build list of attempted paths for error message
    std::vector<std::string> attempted_paths;
    std::string normalized = normalizePath(legacy_path);
    std::string new_path = mapLegacyToNew(normalized);
    
    if (!new_path.empty() && new_path != normalized) {
        attempted_paths.push_back(new_path);
    }
    attempted_paths.push_back(normalized);
    
    throw ConfigNotFoundException(legacy_path, attempted_paths);
}

std::optional<std::string> ConfigPathResolver::tryResolve(const std::string& legacy_path) {
    std::string normalized = normalizePath(legacy_path);
    
    // Check cache first if enabled
    if (caching_enabled_.load()) {
        auto cached = cache_.get(normalized);
        if (cached) {
            metrics_.cache_hits++;
            return *cached;
        }
        metrics_.cache_misses++;
    }
    
    // Validate path to prevent security issues
    try {
        validatePath(normalized);
    } catch (const InvalidPathException&) {
        return std::nullopt;
    }
    
    // Try new path first
    std::string new_path = mapLegacyToNew(normalized);
    std::string resolved_path;
    
    if (!new_path.empty() && std::filesystem::exists(new_path)) {
        if (normalized != new_path) {
            spdlog::debug("ConfigPathResolver: Using new config path: {} -> {}", 
                         normalized, new_path);
            metrics_.new_path_hits++;
        }
        resolved_path = new_path;
    }
    // Fall back to legacy path with warning
    else if (std::filesystem::exists(normalized)) {
        if (!new_path.empty() && new_path != normalized) {
            // Track usage for aggregation report
            aggregator_.incrementUsage(normalized);

            if (!aggregation_enabled_.load()) {
                // Per-call warning (only when aggregation is disabled)
                auto metadata = getMetadata(normalized);
                if (metadata && metadata->isDeprecated()) {
                    if (metadata->isRemovalDue()) {
                        spdlog::error("ConfigPathResolver: {}", metadata->getDeprecationMessage());
                    } else {
                        spdlog::warn("ConfigPathResolver: {}", metadata->getDeprecationMessage());
                    }
                } else {
                    spdlog::warn("ConfigPathResolver: Using legacy config path: {}. "
                                "Please migrate to: {}", normalized, new_path);
                }
            }
            metrics_.legacy_fallbacks++;
        }
        resolved_path = normalized;
    }
    else {
        // Track unmapped requests
        if (new_path.empty() || new_path == normalized) {
            metrics_.unmapped_requests++;
        }
        // Neither path exists
        return std::nullopt;
    }
    
    // Cache the resolved path if caching is enabled
    if (caching_enabled_.load()) {
        cache_.put(normalized, resolved_path);
    }
    
    return resolved_path;
}

std::string ConfigPathResolver::mapLegacyToNew(const std::string& legacy_path) {
    std::string normalized = normalizePath(legacy_path);
    
    auto it = PATH_MAPPING.find(normalized);
    if (it != PATH_MAPPING.end()) {
        return it->second;
    }
    
    // No mapping found - return the path as-is
    return normalized;
}

bool ConfigPathResolver::isLegacyPath(const std::string& path) {
    std::string normalized = normalizePath(path);
    return PATH_MAPPING.find(normalized) != PATH_MAPPING.end();
}

const std::map<std::string, std::string>& ConfigPathResolver::legacyPathMappings() {
    return PATH_MAPPING;
}

std::optional<PathMappingMetadata> ConfigPathResolver::getMetadata(const std::string& legacy_path) {
    std::string normalized = normalizePath(legacy_path);
    auto it = METADATA_TABLE.find(normalized);
    if (it != METADATA_TABLE.end()) {
        return it->second;
    }
    
    // If not in metadata table, create basic metadata from mapping table
    auto mapping_it = PATH_MAPPING.find(normalized);
    if (mapping_it != PATH_MAPPING.end()) {
        return PathMappingMetadata{
            normalized,
            mapping_it->second,
            inferCategory(mapping_it->second),
            std::nullopt,  // No deprecation date
            std::nullopt,  // No removal date
            std::nullopt   // No migration guide
        };
    }
    
    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════
// Private Helper Methods
// ═══════════════════════════════════════════════════════════

std::string ConfigPathResolver::normalizePath(const std::string& path) {
    std::string normalized = path;
    
    // Replace backslashes with forward slashes
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    // Remove leading "./"
    if (normalized.starts_with("./")) {
        normalized = normalized.substr(2);
    }
    
    // Remove trailing slashes
    while (normalized.ends_with("/") && normalized.length() > 1) {
        normalized.pop_back();
    }
    
    return normalized;
}

void ConfigPathResolver::validatePath(const std::string& path) {
    // Check for path traversal attempts
    if (path.find("..") != std::string::npos) {
        throw InvalidPathException(path, "path traversal not allowed");
    }

    // Check for null bytes
    if (path.find('\0') != std::string::npos) {
        throw InvalidPathException(path, "null bytes not allowed in path");
    }

    // Reject absolute paths that are not rooted inside a config directory
    std::filesystem::path fs_path(path);
    if (fs_path.is_absolute()) {
        std::string str = path;
        std::replace(str.begin(), str.end(), '\\', '/');
        if (str.find("/config/") == std::string::npos &&
            str.find("/config") != str.size() - 7) {
            throw InvalidPathException(path, "absolute path outside config directory");
        }
    }

    // Reject symlinks that resolve outside the config root (prevents symlink escapes)
    std::error_code ec;
    if (std::filesystem::is_symlink(fs_path, ec) && !ec) {
        auto canonical = std::filesystem::canonical(fs_path, ec);
        if (!ec) {
            auto cwd = std::filesystem::current_path(ec);
            if (!ec) {
                // The symlink target must reside under cwd (our effective config root)
                auto cwd_str      = cwd.generic_string();
                auto canonical_str = canonical.generic_string();
                if (canonical_str.find(cwd_str) != 0) {
                    throw InvalidPathException(path, "symlink escapes config root");
                }
            }
        }
    }
}

void ConfigPathResolver::resetMetrics() {
    metrics_.resolution_hits = 0;
    metrics_.resolution_misses = 0;
    metrics_.legacy_fallbacks = 0;
    metrics_.new_path_hits = 0;
    metrics_.unmapped_requests = 0;
    metrics_.cache_hits = 0;
    metrics_.cache_misses = 0;
    aggregator_.reset();
}

void ConfigPathResolver::setCachingEnabled(bool enabled) {
    caching_enabled_.store(enabled);
    if (!enabled) {
        cache_.clear();
    }
}

ConfigPathResolver::CacheConfig ConfigPathResolver::currentCacheConfig() {
    return cache_config_;
}

void ConfigPathResolver::setAggregationEnabled(bool enabled, int interval_seconds) {
    aggregation_enabled_.store(enabled);
    if (enabled) {
        aggregator_.start(interval_seconds);
    } else {
        aggregator_.stop();
    }
}

std::vector<ConfigPathResolver::DeprecationEntry> ConfigPathResolver::deprecationReport() {
    return aggregator_.getReport();
}

std::string ConfigPathResolver::inferCategory(const std::string& new_path) {
    // Extract category from path (e.g., "config/ai_ml/..." -> "ai_ml")
    size_t first_slash = new_path.find('/');
    if (first_slash == std::string::npos) {
        return "unknown";
    }
    
    size_t second_slash = new_path.find('/', first_slash + 1);
    if (second_slash == std::string::npos) {
        return "unknown";
    }
    
    return new_path.substr(first_slash + 1, second_slash - first_slash - 1);
}

} // namespace config
} // namespace themis
