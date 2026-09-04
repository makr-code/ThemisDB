/**
 * @file config_path_resolver.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=11, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "config/config_path_resolver.h"
#include "config/config_audit_log.h"
#include "config/config_errors.h"
#include "config/config_file_watcher.h"
#include "config/path_mapping_metadata.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <cstdio>
#include <iomanip>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace themis {
namespace config {

// ═══════════════════════════════════════════════════════════
// DeprecationAggregator – tracks per-path legacy usage counts
// and emits periodic structured log reports.
// ═══════════════════════════════════════════════════════════

/** @brief and emits periodic structured log reports. */
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
        std::vector<ConfigPathResolver::DeprecationEntry> report = {};

        report.reserve(usage_counts_.size());

        for (const auto& [path, count] : usage_counts_) {
            ConfigPathResolver::DeprecationEntry entry;
            entry.legacy_path = path;
            entry.usage_count = count;

            auto metadata = ConfigPathResolver::getMetadata(path);
            if (metadata) {
                entry.new_path = metadata->new_path;
                entry.category = metadata->category;
                entry.removal_date = metadata->removal_date;
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
    void start([[maybe_unused]] int interval_seconds = DEFAULT_INTERVAL_SECONDS) {
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
                std::ostringstream oss = {};
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
// Env-var helpers for cache configuration
// ═══════════════════════════════════════════════════════════

namespace {

/// Read THEMIS_CONFIG_CACHE_SIZE from the environment.
/// Valid range: [10, 100000]. Falls back to kDefaultCacheSize and prints a
/// warning to stderr when the variable is absent, unparseable, or out of
/// range.  Uses fprintf(stderr) rather than spdlog because this function is
/// called during static initialization before spdlog may be configured.
size_t readCacheSizeFromEnv() noexcept {
    const char* env = std::getenv("THEMIS_CONFIG_CACHE_SIZE");
    if (env && *env != '\0') {
        char* end = nullptr;
        const long val = std::strtol(env, &end, 10);
        if (end != env && *end == '\0') {
            if (val >= 10 && val <= 100000) {
                return static_cast<size_t>(val);
            }
            // Valid integer but out of range – warn and fall through to default.
            fprintf(stderr,
                    "[ThemisDB][config] THEMIS_CONFIG_CACHE_SIZE=%ld is out of range "
                    "[10, 100000]; using default %zu.\n",
                    val, ConfigPathResolver::kDefaultCacheSize);
        } else {
            fprintf(stderr,
                    "[ThemisDB][config] THEMIS_CONFIG_CACHE_SIZE=%s is not a valid "
                    "integer; using default %zu.\n",
                    env, ConfigPathResolver::kDefaultCacheSize);
        }
    }
    return ConfigPathResolver::kDefaultCacheSize;
}

/// Read THEMIS_CONFIG_CACHE_TTL from the environment.
/// Valid range: [1, 86400] seconds. Falls back to kDefaultCacheTtlSeconds and
/// prints a warning to stderr when the variable is absent, unparseable, or out
/// of range.
int readCacheTtlFromEnv() noexcept {
    const char* env = std::getenv("THEMIS_CONFIG_CACHE_TTL");
    if (env && *env != '\0') {
        char* end = nullptr;
        const long val = std::strtol(env, &end, 10);
        if (end != env && *end == '\0') {
            if (val >= 1 && val <= 86400) {
                return static_cast<int>(val);
            }
            // Valid integer but out of range – warn and fall through to default.
            fprintf(stderr,
                    "[ThemisDB][config] THEMIS_CONFIG_CACHE_TTL=%ld is out of range "
                    "[1, 86400]; using default %d.\n",
                    val, ConfigPathResolver::kDefaultCacheTtlSeconds);
        } else {
            fprintf(stderr,
                    "[ThemisDB][config] THEMIS_CONFIG_CACHE_TTL=%s is not a valid "
                    "integer; using default %d.\n",
                    env, ConfigPathResolver::kDefaultCacheTtlSeconds);
        }
    }
    return ConfigPathResolver::kDefaultCacheTtlSeconds;
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════
// Static Members Initialization
// ═══════════════════════════════════════════════════════════

const size_t ConfigPathResolver::kCacheSize        = readCacheSizeFromEnv();
const int    ConfigPathResolver::kCacheTtlSeconds  = readCacheTtlFromEnv();
ConfigPathResolver::Metrics ConfigPathResolver::metrics_;
ConfigPathResolver::CacheConfig ConfigPathResolver::cache_config_{
    ConfigPathResolver::kCacheSize,
    ConfigPathResolver::kCacheTtlSeconds
};
LRUCacheWithTTL<std::string, std::string> ConfigPathResolver::cache_(
    ConfigPathResolver::cache_config_.capacity,
    ConfigPathResolver::cache_config_.ttl_seconds);
std::atomic<bool> ConfigPathResolver::caching_enabled_{true};
ConfigPathResolver::DeprecationAggregator ConfigPathResolver::aggregator_;
std::atomic<bool> ConfigPathResolver::aggregation_enabled_{false};
std::map<std::string, std::atomic<uint64_t>> ConfigPathResolver::legacy_fallbacks_by_category_;
std::once_flag ConfigPathResolver::category_init_flag_;
std::atomic<ConfigEnvironment> ConfigPathResolver::current_env_{
    ConfigPathResolver::envFromEnvironmentVariable()};
volatile sig_atomic_t ConfigPathResolver::sighup_pending_ = 0;
ConfigAuditLog ConfigPathResolver::audit_log_;
std::atomic<double> ConfigPathResolver::legacy_fallback_threshold_{0.0};
std::atomic<uint64_t> ConfigPathResolver::last_threshold_warn_count_{0};
std::unique_ptr<ConfigFileWatcher> ConfigPathResolver::file_watcher_;

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

    // AI/ML – Additional LLM Configurations
    {"config/llm_config.example.yaml", "config/ai_ml/llm/config.example.yaml"},
    {"config/llm_config.production.yaml", "config/ai_ml/llm/config.production.yaml"},
    {"config/llm_extended_context.yaml", "config/ai_ml/llm/extended_context.yaml"},
    {"config/llm_remote_models.yaml", "config/ai_ml/llm/remote_models.yaml"},
    {"config/llm-models.yaml", "config/ai_ml/llm/models.yaml"},
    {"config/config_with_llm.yaml", "config/ai_ml/llm/config_with_llm.yaml"},
    {"config/few_shot_examples.yaml", "config/ai_ml/llm/few_shot_examples.yaml"},
    {"config/prompt_optimizer_config.yaml", "config/ai_ml/llm/prompt_optimizer_config.yaml"},

    // AI/ML – Vision and Model Configurations
    {"config/vision_licenses.yaml", "config/ai_ml/vision/licenses.yaml"},
    {"config/image_analysis.yaml", "config/ai_ml/vision/image_analysis.yaml"},
    {"config/phi3_lora_training.yaml", "config/ai_ml/phi3_lora_training.yaml"},
    {"config/default_model_config.yaml", "config/ai_ml/default_model_config.yaml"},
    {"config/flash_attention_config.yaml", "config/ai_ml/flash_attention_config.yaml"},
    {"config/self_awareness.yaml", "config/ai_ml/self_awareness.yaml"},

    // Distributed – Replication and Sharding
    {"config/replication.example.yaml", "config/distributed/replication/basic.example.yaml"},
    {"config/replication-ha.example.yaml", "config/distributed/replication/ha.example.yaml"},
    {"config/sharding-with-metrics.yaml", "config/distributed/sharding/with-metrics.yaml"},

    // Networking – Additional
    {"config/adaptive_routing.example.json", "config/networking/adaptive_routing.example.json"},

    // Performance – Additional
    {"config/ingestion-optimized.yaml", "config/performance/ingestion-optimized.yaml"},
    {"config/scaling_optimizations_quickstart.yaml", "config/performance/scaling_optimizations_quickstart.yaml"},

    // Compliance – Additional
    {"config/policies.yaml", "config/compliance/policies.yaml"},

    // Security – Additional
    {"config/timestamp_authority.yaml", "config/security/timestamp_authority.yaml"},

    // AI/ML – OCR Tesseract Language Packs
    {"config/tesseract_lang", "config/ai_ml/tesseract_lang"},
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
    // ── AI/ML – Additional LLM Configurations ────────────────────────────────
    {
        "config/llm_config.example.yaml",
        {
            "config/llm_config.example.yaml",
            "config/ai_ml/llm/config.example.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/llm_config.production.yaml",
        {
            "config/llm_config.production.yaml",
            "config/ai_ml/llm/config.production.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/llm_extended_context.yaml",
        {
            "config/llm_extended_context.yaml",
            "config/ai_ml/llm/extended_context.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/llm_remote_models.yaml",
        {
            "config/llm_remote_models.yaml",
            "config/ai_ml/llm/remote_models.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/llm-models.yaml",
        {
            "config/llm-models.yaml",
            "config/ai_ml/llm/models.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/config_with_llm.yaml",
        {
            "config/config_with_llm.yaml",
            "config/ai_ml/llm/config_with_llm.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/few_shot_examples.yaml",
        {
            "config/few_shot_examples.yaml",
            "config/ai_ml/llm/few_shot_examples.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/prompt_optimizer_config.yaml",
        {
            "config/prompt_optimizer_config.yaml",
            "config/ai_ml/llm/prompt_optimizer_config.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── AI/ML – Vision and Model Configurations ───────────────────────────────
    {
        "config/vision_licenses.yaml",
        {
            "config/vision_licenses.yaml",
            "config/ai_ml/vision/licenses.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/image_analysis.yaml",
        {
            "config/image_analysis.yaml",
            "config/ai_ml/vision/image_analysis.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/phi3_lora_training.yaml",
        {
            "config/phi3_lora_training.yaml",
            "config/ai_ml/phi3_lora_training.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/default_model_config.yaml",
        {
            "config/default_model_config.yaml",
            "config/ai_ml/default_model_config.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/flash_attention_config.yaml",
        {
            "config/flash_attention_config.yaml",
            "config/ai_ml/flash_attention_config.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/self_awareness.yaml",
        {
            "config/self_awareness.yaml",
            "config/ai_ml/self_awareness.yaml",
            "ai_ml",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Distributed – Replication and Sharding ────────────────────────────────
    {
        "config/replication.example.yaml",
        {
            "config/replication.example.yaml",
            "config/distributed/replication/basic.example.yaml",
            "distributed",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/replication-ha.example.yaml",
        {
            "config/replication-ha.example.yaml",
            "config/distributed/replication/ha.example.yaml",
            "distributed",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/sharding-with-metrics.yaml",
        {
            "config/sharding-with-metrics.yaml",
            "config/distributed/sharding/with-metrics.yaml",
            "distributed",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Networking – Additional ───────────────────────────────────────────────
    {
        "config/adaptive_routing.example.json",
        {
            "config/adaptive_routing.example.json",
            "config/networking/adaptive_routing.example.json",
            "networking",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Performance – Additional ──────────────────────────────────────────────
    {
        "config/ingestion-optimized.yaml",
        {
            "config/ingestion-optimized.yaml",
            "config/performance/ingestion-optimized.yaml",
            "performance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    {
        "config/scaling_optimizations_quickstart.yaml",
        {
            "config/scaling_optimizations_quickstart.yaml",
            "config/performance/scaling_optimizations_quickstart.yaml",
            "performance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Compliance – Additional ───────────────────────────────────────────────
    {
        "config/policies.yaml",
        {
            "config/policies.yaml",
            "config/compliance/policies.yaml",
            "compliance",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── Security – Additional ─────────────────────────────────────────────────
    {
        "config/timestamp_authority.yaml",
        {
            "config/timestamp_authority.yaml",
            "config/security/timestamp_authority.yaml",
            "security",
            parseDate("2024-01-01"),
            parseDate("2026-06-30"),
            "docs/config_migration_guide.md"
        }
    },
    // ── AI/ML – OCR Tesseract Language Packs ─────────────────────────────────
    {
        "config/tesseract_lang",
        {
            "config/tesseract_lang",
            "config/ai_ml/tesseract_lang",
            "ai_ml",
            parseDate("2026-01-01"),
            parseDate("2027-06-30"),
            "config/ai_ml/tesseract_lang/README.md"
        }
    },
};

static const bool kLegacyCategoryCountersBootstrapped = []() {
    (void)ConfigPathResolver::legacyFallbackCategories();
    return true;
}();

// ═══════════════════════════════════════════════════════════
// Public API Implementation
// ═══════════════════════════════════════════════════════════

std::string ConfigPathResolver::resolve(const std::string& legacy_path) {
    auto result = tryResolve(legacy_path);
    if (result) {
        return *result;
    }
    
    // Build list of attempted paths for error message
    std::vector<std::string> attempted_paths;
    std::string normalized = normalizePath(legacy_path);
    std::string new_path = mapLegacyToNew(normalized);
    ConfigEnvironment env = current_env_.load();

    // Include the overlay path in the error message when a non-prod environment is active
    if (env != ConfigEnvironment::PROD && !new_path.empty() && new_path != normalized) {
        std::string relative_part = new_path;
        const std::string config_prefix = "config/";
        if (relative_part.starts_with(config_prefix)) {
            relative_part = relative_part.substr(config_prefix.size());
        }
        attempted_paths.push_back("config/" + envToString(env) + "/" + relative_part);
    }

    if (!new_path.empty() && new_path != normalized) {
        attempted_paths.push_back(new_path);
    }
    attempted_paths.push_back(normalized);
    
    throw ConfigNotFoundException(legacy_path, attempted_paths);
}

std::optional<std::string> ConfigPathResolver::tryResolve(const std::string& legacy_path) {
    std::string normalized = normalizePath(legacy_path);
    try {
        validatePath(normalized);
    } catch (const InvalidPathException&) {
        metrics_.resolution_misses++;
        return std::nullopt;
    }

    ConfigEnvironment env = current_env_.load();

    // Build env-prefixed cache key to prevent cross-environment cache poisoning
    std::string cache_key = envToString(env) + ":" + normalized;
    std::string resolved_path = {};
    bool was_legacy_fallback = false;
    bool from_cache = false;

    if (sighup_pending_) {
        sighup_pending_ = 0;
        cache_.clear();
        spdlog::info("ConfigPathResolver: SIGHUP received - resolved path cache cleared");
    }

    if (caching_enabled_.load()) {
        auto cached = cache_.get(cache_key);
        if (cached) {
            metrics_.cache_hits++;
            resolved_path = *cached;
            was_legacy_fallback = isLegacyPath(normalized) && (*cached == normalized);
            from_cache = true;
        } else {
            metrics_.cache_misses++;
        }
    }

    std::string new_path = mapLegacyToNew(normalized);

    if (resolved_path.empty()) {
        if (env != ConfigEnvironment::PROD && !new_path.empty() && new_path != normalized) {
            std::string relative_part = new_path;
            const std::string config_prefix = "config/";
            if (relative_part.starts_with(config_prefix)) {
                relative_part = relative_part.substr(config_prefix.size());
            }
            std::string overlay_path = "config/" + envToString(env) + "/" + relative_part;
            if (std::filesystem::exists(overlay_path)) {
                spdlog::debug("ConfigPathResolver: Using env overlay path [{}]: {} -> {}",
                              envToString(env), normalized, overlay_path);
                resolved_path = overlay_path;
                metrics_.new_path_hits++;
            }
        }

        if (resolved_path.empty()) {
            if (!new_path.empty() && std::filesystem::exists(new_path)) {
                if (normalized != new_path) {
                    spdlog::debug("ConfigPathResolver: Using new config path: {} -> {}",
                                  normalized, new_path);
                    metrics_.new_path_hits++;
                }
                resolved_path = new_path;
            } else if (std::filesystem::exists(normalized)) {
                if (!new_path.empty() && new_path != normalized) {
                    aggregator_.incrementUsage(normalized);
                    was_legacy_fallback = true;

                    if (!aggregation_enabled_.load()) {
                        auto metadata = getMetadata(normalized);
                        if (metadata && metadata->isDeprecated()) {
                            if (metadata->isRemovalDue()) {
                                spdlog::error("ConfigPathResolver: {}", metadata->getDeprecationMessage());
                            } else {
                                spdlog::warn("ConfigPathResolver: {}", metadata->getDeprecationMessage());
                            }
                        } else {
                            spdlog::warn("ConfigPathResolver: Using legacy config path: {}. Please migrate to: {}",
                                         normalized, new_path);
                        }
                    }

                    metrics_.legacy_fallbacks++;
                    const std::string category_path = new_path.empty() ? normalized : new_path;
                    const std::string category = inferCategory(category_path);
                    auto it = legacy_fallbacks_by_category_.find(category);
                    if (it != legacy_fallbacks_by_category_.end()) {
                        it->second.fetch_add(1, std::memory_order_relaxed);
                    }
                    checkFallbackRateThreshold();
                } else {
                    metrics_.unmapped_requests++;
                }
                resolved_path = normalized;
            } else {
                metrics_.unmapped_requests++;
                metrics_.resolution_misses++;
                return std::nullopt;
            }
        }
    }

    if (caching_enabled_.load() && !from_cache) {
        cache_.put(cache_key, resolved_path);
    }

    metrics_.resolution_hits++;

    if (audit_log_.isEnabled()) {
        audit_log_.record({legacy_path, resolved_path,
            std::chrono::system_clock::now(), was_legacy_fallback, from_cache});
        spdlog::trace("[CONFIG AUDIT] path='{}' resolved='{}' legacy={} cache_hit={}",
                      legacy_path, resolved_path, was_legacy_fallback, from_cache);
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

std::vector<ConfigPathResolver::DeprecationEntry> ConfigPathResolver::deprecationReport() {
    return aggregator_.getReport();
}

std::string ConfigPathResolver::inferCategory(const std::string& new_path) {
    std::string normalized = normalizePath(new_path);

    const std::string prefix = "config/";
    if (normalized.starts_with(prefix)) {
        normalized = normalized.substr(prefix.size());
    }

    std::size_t first_slash = normalized.find('/');
    if (first_slash == std::string::npos) {
        return "root";
    }

    std::string category = normalized.substr(0, first_slash);
    if (category.empty()) {
        return "root";
    }
    return category;
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
        // Accept paths that contain "/config/" as a component, or end with "/config"
        constexpr std::string_view kConfigSuffix = "/config";
        const bool ends_with_config =
            static_cast<int>(str.size()) >= kConfigSuffix.size() &&
            str.compare(static_cast<int>(str.size()) - static_cast<int>(kConfigSuffix.size()) ,
                        kConfigSuffix.size(), kConfigSuffix) == 0;
        if (str.find("/config/") == std::string::npos && !ends_with_config) {
            throw InvalidPathException(path, "absolute path outside config directory");
        }
    }

    // Reject symlinks that resolve outside the config root (prevents symlink escapes)
    std::error_code ec = {};
    if (std::filesystem::is_symlink(fs_path, ec) && !ec) {
        auto canonical = std::filesystem::canonical(fs_path, ec);
        if (!ec) {
            std::filesystem::path config_root;
            if (fs_path.is_absolute()) {
                // For absolute paths, derive the config root by walking path components
                // up to and including the first "config" directory component.
                // The absolute-path check above guarantees the path already contains
                // "/config/" or ends with "/config", so this loop always finds a match.
                // Path traversal ("..") is already rejected before this point, so no
                // component can escape the derived config root.
                std::filesystem::path acc;
                for (const auto& component : fs_path) {
                    acc /= component;
                    if (component.string() == "config") {
                        config_root = acc;
                        break;
                    }
                }
                if (config_root.empty()) {
                    // Dead-code defensive guard: the absolute-path check above should
                    // have already thrown InvalidPathException for any path that lacks a
                    // "config" component, so this branch is never reached in practice.
                    config_root = fs_path.parent_path();
                }
            } else {
                config_root = std::filesystem::current_path(ec);
                if (ec) {
                    // Cannot determine config root; skip symlink check.
                    return;
                }
            }
            auto config_root_str = config_root.generic_string();
            auto canonical_str   = canonical.generic_string();
            // The canonical target must reside inside the config root tree.
            // We accept an exact match (symlink to the config root dir itself)
            // and any proper subdirectory (canonical starts with root + "/").
            // Appending "/" prevents false passes from directory-name prefixes
            // (e.g. "/opt/config_other" must not match root "/opt/config").
            if (canonical_str != config_root_str &&
                canonical_str.find(config_root_str + "/") != 0) {
                throw InvalidPathException(path, "symlink escapes config root");
            }
        }
    }
}

void ConfigPathResolver::initLegacyFallbackCategoryCounters() {
    std::call_once(category_init_flag_, []() {
        legacy_fallbacks_by_category_.clear();
        legacy_fallbacks_by_category_.emplace("unknown", 0);

        // Pre-populate all known categories so the label cardinality is fixed
        // up front. This allows lock-free increments during fallbacks and
        // keeps scrape-time iteration deterministic without touching PATH_MAPPING.
        for (const auto& entry : PATH_MAPPING) {
            const std::string category = inferCategory(entry.second);
            legacy_fallbacks_by_category_.try_emplace(category, 0);
        }
    });
}

void ConfigPathResolver::resetMetrics() {
    metrics_.resolution_hits = 0;
    metrics_.resolution_misses = 0;
    metrics_.legacy_fallbacks = 0;
    metrics_.new_path_hits = 0;
    metrics_.unmapped_requests = 0;
    metrics_.cache_hits = 0;
    metrics_.cache_misses = 0;
    last_threshold_warn_count_ = 0;
    aggregator_.reset();
    for (auto& entry : legacy_fallbacks_by_category_) {
        entry.second.store(0, std::memory_order_relaxed);
    }
}

std::vector<std::pair<std::string, uint64_t>> ConfigPathResolver::legacyFallbacksByCategory() {
    initLegacyFallbackCategoryCounters();
    std::vector<std::pair<std::string, uint64_t>> snapshot;
    snapshot.reserve(legacy_fallbacks_by_category_.size());
    for (const auto& entry : legacy_fallbacks_by_category_) {
        snapshot.emplace_back(entry.first, entry.second.load(std::memory_order_relaxed));
    }
    return snapshot;
}

std::vector<std::string> ConfigPathResolver::legacyFallbackCategories() {
    initLegacyFallbackCategoryCounters();
    std::vector<std::string> categories = {};

    categories.reserve(legacy_fallbacks_by_category_.size());
    for (const auto& entry : legacy_fallbacks_by_category_) {
        categories.push_back(entry.first);
    }
    return categories;
}

void ConfigPathResolver::setCachingEnabled([[maybe_unused]] bool enabled) {
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

void ConfigPathResolver::setLegacyFallbackRateThreshold([[maybe_unused]] double threshold) {
    // Clamp to [0.0, 1.0]
    if (threshold < 0.0) {
      threshold = 0.0;
    }
    if (threshold > 1.0) {
      threshold = 1.0;
    }
    legacy_fallback_threshold_.store(threshold, std::memory_order_relaxed);
}

double ConfigPathResolver::getLegacyFallbackRateThreshold() {
    return legacy_fallback_threshold_.load(std::memory_order_relaxed);
}

void ConfigPathResolver::checkFallbackRateThreshold() {
    const double threshold = legacy_fallback_threshold_.load(std::memory_order_relaxed);
    if (threshold <= 0.0) {
        return;
    }

    const uint64_t fallbacks = metrics_.legacy_fallbacks.load(std::memory_order_relaxed);
    const uint64_t new_hits = metrics_.new_path_hits.load(std::memory_order_relaxed);
    const uint64_t total = fallbacks + new_hits;
    if (total == 0) {
        return;
    }

    const double rate = static_cast<double>(fallbacks) / static_cast<double>(total);
    if (rate < threshold) {
        return;
    }

    uint64_t last_warn = last_threshold_warn_count_.load(std::memory_order_relaxed);
    const bool should_warn = (last_warn == 0) || (fallbacks >= last_warn * 2);
    if (should_warn &&
        last_threshold_warn_count_.compare_exchange_strong(last_warn, fallbacks, std::memory_order_relaxed)) {
        spdlog::warn(
            "[CONFIG] Legacy fallback rate {:.1f}% meets or exceeds threshold {:.1f}% "
            "(fallbacks: {}, total resolutions: {}). Migrate config paths to avoid this warning.",
            rate * 100.0, threshold * 100.0, fallbacks, total);
    }
}

std::string ConfigPathResolver::envToString(ConfigEnvironment env) {
    switch (env) {
        case ConfigEnvironment::DEV:
            return "dev";
        case ConfigEnvironment::STAGING:
            return "staging";
        case ConfigEnvironment::PROD:
        [[fallthrough]];\n        default:
            return "prod";
    }
}

ConfigEnvironment ConfigPathResolver::envFromEnvironmentVariable() {
    const char* raw = std::getenv("THEMIS_CONFIG_ENV");
    if (!raw) {
        return ConfigEnvironment::PROD;
    }

    std::string val(raw);
    for (char c : val) {
        if (c == '/' || c == '\\' || c == '$' || c == '`' || c == ';' ||
            c == '&' || c == '|' || c == '>' || c == '<' || c == '\0') {
            spdlog::warn("ConfigPathResolver: THEMIS_CONFIG_ENV contains invalid characters; defaulting to 'prod'");
            return ConfigEnvironment::PROD;
        }
    }

    std::transform(val.begin(), val.end(), val.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (val == "dev") {
        return ConfigEnvironment::DEV;
    }
    if (val == "staging") {
        return ConfigEnvironment::STAGING;
    }
    if (val == "prod") {
        return ConfigEnvironment::PROD;
    }

    spdlog::warn("ConfigPathResolver: Unknown THEMIS_CONFIG_ENV value '{}'; defaulting to 'prod'", val);
    return ConfigEnvironment::PROD;
}

void ConfigPathResolver::setEnvironment(ConfigEnvironment env) {
    current_env_.store(env);
    cache_.clear();
    spdlog::info("ConfigPathResolver: Active environment set to '{}'", envToString(env));
}

ConfigEnvironment ConfigPathResolver::getEnvironment() {
    return current_env_.load();
}

void ConfigPathResolver::setAuditLogEnabled([[maybe_unused]] bool enabled) {
    if (enabled) {
        audit_log_.enable();
    } else {
        audit_log_.disable();
    }
}

std::vector<AuditEntry> ConfigPathResolver::auditLog() {
    return audit_log_.getEntries();
}

void ConfigPathResolver::clearAuditLog() {
    audit_log_.clear();
}

void ConfigPathResolver::setAuditLogMaxEntries(std::size_t max) {
    audit_log_.setMaxEntries(max);
}
// ═══════════════════════════════════════════════════════════
// SIGHUP Hot-Reload
// ═══════════════════════════════════════════════════════════

// Static signal handler – must be async-signal-safe; only sets a flag.
void ConfigPathResolver::handleSighup([[maybe_unused]] int /*sig*/) {
    sighup_pending_ = 1;
}

void ConfigPathResolver::registerSighupHandler() {
#if defined(_WIN32)
    // SIGHUP is not defined on Windows; no-op.
    spdlog::debug("ConfigPathResolver: SIGHUP hot-reload not supported on Windows");
#else
    struct sigaction sa{};
    sa.sa_handler = ConfigPathResolver::handleSighup;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGHUP, &sa, nullptr) != 0) {
        spdlog::warn([[maybe_unused]] "ConfigPathResolver: Failed to register SIGHUP handler");
    } else {
        spdlog::info("ConfigPathResolver: SIGHUP hot-reload registered – "
                     "send SIGHUP to flush the resolved path cache at runtime");
    }
#endif
}

// ── inotify/kqueue/ReadDirectoryChangesW hot-reload ─────────────────────────

bool ConfigPathResolver::startHotReload(const std::string& watch_dir,
                                        std::chrono::milliseconds debounce) {
    if (file_watcher_ && file_watcher_->isRunning()) {
        spdlog::debug("ConfigPathResolver: file watcher already running on '{}'",
                      file_watcher_->watchPath());
        return true;
    }

    file_watcher_ = std::make_unique<ConfigFileWatcher>(
        watch_dir,
        []() {
            cache_.clear();
            spdlog::info("ConfigPathResolver: config file changed on disk – "
                         "resolved path cache cleared");
        },
        debounce);

    bool ok = file_watcher_->start();
    if (!ok) {
        file_watcher_.reset();
        spdlog::warn("ConfigPathResolver: file-based hot-reload could not be started "
                     "for '{}'; fall back to SIGHUP-only hot-reload", watch_dir);
    }
    return ok;
}

void ConfigPathResolver::stopHotReload() {
    if (file_watcher_) {
        file_watcher_->stop();
        file_watcher_.reset();
    }
}

} // namespace config
} // namespace themis

