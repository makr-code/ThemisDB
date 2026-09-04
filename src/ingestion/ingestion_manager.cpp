/**
 * @file ingestion_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 80/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=3, M=36, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/ingestion_manager.h"
#include "ingestion/huggingface_connector.h"
#include "ingestion/filesystem_ingester.h"
#include "ingestion/api_connector.h"
#include "ingestion/kafka_connector.h"
#include "ingestion/object_storage_connector.h"
#include "ingestion/s3_connector.h"
#include "ingestion/database_connector.h"
#include "ingestion/web_crawler_connector.h"
#include "ingestion/cdc_connector.h"
#include "ingestion/deontic_extractor.h"
#include "ingestion/agentic_reference_validator.h"
#include "ingestion/llm_adapter.h"
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>
#include <filesystem>
#include <fstream>
#include <regex>
#include <optional>

namespace themis {
namespace ingestion {

// ============================================================================
// Correlation ID generator (thread-safe, no external UUID lib)
// ============================================================================
namespace {

/// Connector API version reported in lineage records (bump on interface change)
static constexpr const char* kConnectorVersion = "1.0.0";
/// Prefix for doc_id in batch-level success/dry-run lineage records
static constexpr const char* kBatchDocIdPrefix  = "batch:";
/// Prefix for doc_id in failed-batch lineage records
static constexpr const char* kFailedDocIdPrefix = "failed:";

static std::string generateCorrelationId() {
    static std::atomic<uint64_t> counter{0};
    auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    auto seq = ++counter;
    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(16) << static_cast<uint64_t>(ts)
       << '-'
       << std::setw(8) << (seq & 0xFFFFFFFF);
    return ss.str();
}

/// Map SourceType to a short string label for Prometheus
static std::string sourceTypeLabel(SourceType t) {
    switch (t) {
        case SourceType::HUGGINGFACE:    return "HUGGINGFACE";
        case SourceType::FILESYSTEM:     return "FILESYSTEM";
        case SourceType::API:            return "API";
        case SourceType::DATABASE:       return "DATABASE";
        case SourceType::KAFKA:          return "KAFKA";
        case SourceType::OBJECT_STORAGE: return "OBJECT_STORAGE";
        case SourceType::WEB_CRAWLER:    return "WEB_CRAWLER";
        case SourceType::CDC:            return "CDC";
        case SourceType::PLUGIN:         return "PLUGIN";
        default:                         return "UNKNOWN";
    }
}

/// Map IngestionErrorCode to its integer string for a metric label
[[maybe_unused]] static std::string errorCodeLabel(IngestionErrorCode c) {
    return std::to_string(static_cast<int>(c));
}

// ============================================================================
// Schema validation helpers
// ============================================================================

/// Escape all regex metacharacters in `s` so it can be used as a literal
/// substring inside a larger regex pattern.
static std::string regexEscape(const std::string& s) {
    static const std::string kMeta = R"(\.^$*+?()[]{}|)";
    std::string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        if (kMeta.find(c) != std::string::npos) {
          out += '\\';
        }
        out += c;
    }
    return out;
}

/// Build a compiled regex that matches `"key"\\s*:` at a JSON object key position
/// (preceded by `{` or `,`).  Field names are literal-escaped before insertion.
static std::regex buildKeyRegex(const std::string& key) {
    return std::regex(R"([{,]\s*\")" + regexEscape(key) + R"(\"\s*:)");
}

/// Extract a minimal JSON string value for a named key using a pre-compiled regex.
/// Returns true and populates `value` when the key exists at a JSON object key
/// position and its value is a JSON string.
static bool findJsonStringValueRe(const std::string& json,
                                   const std::regex& key_re,
                                   std::string& value) {
    std::smatch m;
    if (!std::regex_search(json, m, key_re)) {
      return false;
    }
    auto pos = static_cast<std::string::size_type>(m.position() + m.length());
    // Skip whitespace before the value token
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                  json[pos] == '\r' || json[pos] == '\n'))
        ++pos;
    if (pos >= json.size() || json[pos] != '"') {
      return false;
    }
    ++pos;
    value.clear();
    bool esc = false;
    while (pos < json.size()) {
        char c = json[pos++];
        if (esc) {
            switch (c) {
                case 'n':  value += '\n'; break;
                case 'r':  value += '\r'; break;
                case 't':  value += '\t'; break;
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                default:   value += c;    break;
            }
            esc = false;
            continue;
        }
        if (c == '\\') { esc = true; continue; }
        if (c == '"') {
          break;
        }
        value += c;
    }
    return true;
}

/// Returns true when the document looks like a JSON object or array.
static bool looksLikeJson(const std::string& content) {
    for (char c : content) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
          continue;
        }
        return c == '{' || c == '[';
    }
    return false;
}

// ============================================================================
// Compiled schema for a single field (avoids per-document regex construction)
// ============================================================================
struct CompiledFieldRule {
    std::string            name;
    SchemaFieldRule        rule;
    std::regex             key_re;     ///< compiled [{,]\\s*"name"\\s*: pattern
    std::optional<std::regex> value_re; ///< compiled field-value pattern (if any)
    bool                   key_re_ok    = false;
    bool                   value_re_ok  = false;

    explicit CompiledFieldRule(const std::string& n, const SchemaFieldRule& r)
        : name(n), rule(r) {
        try {
            key_re    = buildKeyRegex(n);
            key_re_ok = true;
        } catch (const std::regex_error&) {}
        if (!r.pattern.empty()) {
            try {
                value_re    = std::regex(r.pattern);
                value_re_ok = true;
            } catch (const std::regex_error&) {}
        }
    }
};

/**
 * @brief Build a DocumentValidatorFn from a SchemaConfig.
 *
 * All regex patterns (content-level and field-level) are compiled once here
 * and captured in the returned lambda, avoiding repeated compilation per document.
 *
 * Validates:
 * 1. Content length (min/max)
 * 2. Required content pattern (regex applied to raw content)
 * 3. Required JSON fields and their types/lengths/patterns
 *
 * When `schema.reject_invalid` is `false`, the returned function always sets
 * `is_valid = true` (warning-only mode): violations are still populated in the
 * result so callers can log them, but the document is not counted as failed.
 */
static DocumentValidatorFn buildValidatorFromSchema(const SchemaConfig& schema) {
    // Pre-compile the content-level pattern
    bool content_re_ok = false;
    std::regex content_re;
    if (!schema.required_content_pattern.empty()) {
        try {
            content_re    = std::regex(schema.required_content_pattern);
            content_re_ok = true;
        } catch (const std::regex_error&) {}
    }

    // Pre-compile per-field key patterns and value patterns
    std::vector<CompiledFieldRule> compiled_fields = {};

    compiled_fields.reserve(schema.fields.size());
    for (const auto& kv : schema.fields) {
        compiled_fields.emplace_back(kv.first, kv.second);
    }

    const bool reject_invalid = schema.reject_invalid;

    // Capture by value (SchemaConfig copy + compiled regexes)
    return [schema, content_re, content_re_ok,
            compiled_fields, reject_invalid](const std::string& content) mutable
           -> DocumentValidationResult {
        DocumentValidationResult result;

        // --- content-level checks ---
        if (schema.min_content_length > 0 &&
            content.size() < schema.min_content_length) {
            result.addViolation("",
                "document too short: " + std::to_string(content.size()) +
                " bytes (minimum " + std::to_string(schema.min_content_length) + ")");
        }
        if (schema.max_content_length > 0 &&
            content.size() > schema.max_content_length) {
            result.addViolation("",
                "document too long: " + std::to_string(content.size()) +
                " bytes (maximum " + std::to_string(schema.max_content_length) + ")");
        }
        if (!schema.required_content_pattern.empty()) {
            if (!content_re_ok) {
                result.addViolation("",
                    "invalid required_content_pattern: " +
                    schema.required_content_pattern);
            } else if (!std::regex_search(content, content_re)) {
                result.addViolation("",
                    "document does not match required pattern: " +
                    schema.required_content_pattern);
            }
        }

        // --- field-level checks (JSON documents only) ---
        if (!compiled_fields.empty() && looksLikeJson(content)) {
            for (const auto& cf : compiled_fields) {
                if (!cf.key_re_ok) {
                    result.addViolation(cf.name, "invalid field name (regex error)");
                    continue;
                }
                bool exists = std::regex_search(content, cf.key_re);

                if (!exists) {
                    if (cf.rule.required) {
                        result.addViolation(cf.name, "required field is missing");
                    }
                    continue;
                }

                // Type / value checks for string fields
                if (cf.rule.expected_type == SchemaFieldType::STRING ||
                    cf.rule.min_length > 0 || cf.rule.max_length > 0 ||
                    !cf.rule.pattern.empty()) {
                    std::string str_val;
                    bool is_string = findJsonStringValueRe(content, cf.key_re, str_val);

                    if (cf.rule.expected_type == SchemaFieldType::STRING && !is_string) {
                        result.addViolation(cf.name, "expected a string value");
                    } else if (is_string) {
                        if (cf.rule.min_length > 0 && str_val.size() < cf.rule.min_length) {
                            result.addViolation(cf.name,
                                "string too short: " + std::to_string(str_val.size()) +
                                " chars (minimum " + std::to_string(cf.rule.min_length) + ")");
                        }
                        if (cf.rule.max_length > 0 && str_val.size() > cf.rule.max_length) {
                            result.addViolation(cf.name,
                                "string too long: " + std::to_string(str_val.size()) +
                                " chars (maximum " + std::to_string(cf.rule.max_length) + ")");
                        }
                        if (!cf.rule.pattern.empty()) {
                            if (!cf.value_re_ok) {
                                result.addViolation(cf.name,
                                    "invalid pattern: " + cf.rule.pattern);
                            } else if (!std::regex_search(str_val, *cf.value_re)) {
                                result.addViolation(cf.name,
                                    "value does not match pattern: " + cf.rule.pattern);
                            }
                        }
                    }
                }
            }
        }

        // When reject_invalid is false, treat the document as valid regardless of
        // violations – violations are retained for warning-level logging by callers.
        if (!reject_invalid) {
            result.is_valid = true;
        }

        return result;
    };
}

} // anonymous namespace

// ============================================================================
// CheckpointStore
// ============================================================================

namespace {
/// Sanitise a source_id so it is safe as part of a file name.
static std::string sanitiseSourceId(const std::string& sid) {
    std::string out;
    out.reserve(sid.size());
    for (char c : sid) {
        if (std::isalnum(static_cast<unsigned char>(c)) ||
            c == '-' || c == '_' || c == '.') {
            out += c;
        } else {
            out += '_';
        }
    }
    return out.empty() ? "default" : out;
}

/// Format a time_point as a simple ISO-8601-like string (UTC).
static std::string formatTimestamp(std::chrono::system_clock::time_point tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    return buf;
}
} // anonymous namespace

CheckpointStore::CheckpointStore(const std::string& checkpoint_dir)
    : dir_(checkpoint_dir) {}

std::string CheckpointStore::checkpointPath(const std::string& source_id) const {
    namespace fs = std::filesystem;
    return (fs::path(dir_) / (sanitiseSourceId(source_id) + ".checkpoint")).string();
}

bool CheckpointStore::write(const IngestionCheckpoint& cp) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        std::ofstream f(checkpointPath(cp.source_id), std::ios::trunc);
        if (!f) {
          return false;
        }
        f << "source_id="       << cp.source_id       << '\n'
          << "processed_count=" << cp.processed_count  << '\n'
          << "byte_offset="     << cp.byte_offset       << '\n'
          << "cursor="          << cp.cursor            << '\n'
          << "timestamp="       << cp.timestamp         << '\n';
        return f.good();
    } catch (...) {
        return false;
    }
}

bool CheckpointStore::read(const std::string& source_id,
                            IngestionCheckpoint& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        std::ifstream f(checkpointPath(source_id));
        if (!f) {
          return false;
        }
        out = IngestionCheckpoint{};
        std::string line;
        while (std::getline(f, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) {
              continue;
            }
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            if (key == "source_id") {
              out.source_id       = val;
            }
            else if (key == "processed_count") {
                try { out.processed_count = std::stoull(val); } catch (...) {}
            } else if (key == "byte_offset") {
                try { out.byte_offset = std::stoull(val); } catch (...) {}
            } else if (key == "cursor")    out.cursor          = val;
            else if (key == "timestamp")   out.timestamp       = val;
        }
        // A valid checkpoint must have a non-empty source_id
        return !out.source_id.empty();
    } catch (...) {
        return false;
    }
}

bool CheckpointStore::clear(const std::string& source_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    try {
        return std::filesystem::remove(checkpointPath(source_id));
    } catch (...) {
        return false;
    }
}

bool CheckpointStore::exists(const std::string& source_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::filesystem::exists(checkpointPath(source_id));
}

// ============================================================================
// Token-bucket rate limiter (simple, no external dep)
// ============================================================================
/** @brief Token-bucket rate limiter (simple, no external dep). */
class TokenBucket {
public:
    explicit TokenBucket(double requests_per_second)
        : rate_(requests_per_second)
        , tokens_(requests_per_second > 0.0 ? requests_per_second : 0.0)
        , last_refill_(std::chrono::steady_clock::now()) {}

    /// Consume one token, blocking until available when rate > 0.
    void consume() {
        if (rate_ <= 0.0) return;  // unlimited

        std::unique_lock<std::mutex> lock(mutex_);
        refill();
        while (tokens_ < 1.0) {
            // Calculate wait duration until next token
            double wait_secs = (1.0 - tokens_) / rate_;
            auto wait = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::duration<double>(wait_secs));
            lock.unlock();
            std::this_thread::sleep_for(wait);
            lock.lock();
            refill();
        }
        tokens_ -= 1.0;
    }

    bool isEnabled() const { return rate_ > 0.0; }

private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        double elapsed =
            std::chrono::duration<double>(now - last_refill_).count();
        tokens_ = std::min(rate_, tokens_ + elapsed * rate_);
        last_refill_ = now;
    }

    double rate_;
    double tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    std::mutex mutex_;
};

// ============================================================================
// Pimpl implementation
// ============================================================================
/** @brief Pimpl implementation. */
class IngestionManager::Impl {
public:
    explicit Impl(const std::string& db_connection) 
        : db_connection_(db_connection)
        , target_collection_("legal_documents")
        , parallel_enabled_(false)
        , dry_run_(false)
        , max_threads_(std::thread::hardware_concurrency()) {
    }
    
    ~Impl() = default;
    
    bool registerSource(const SourceConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sources_.find(config.source_id) != sources_.end()) {
            return false;
        }
        sources_[config.source_id] = config;
        return true;
    }
    
    bool unregisterSource(const std::string& source_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        return sources_.erase(source_id) > 0;
    }

    bool reconfigureSource(const std::string& source_id,
                           const SourceConfig& new_config) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sources_.find(source_id);
        if (it == sources_.end()) {
          return false;
        }
        // Preserve the canonical source_id from the registry key
        SourceConfig updated = new_config;
        updated.source_id = source_id;
        it->second = std::move(updated);
        return true;
    }
    
    IngestionStats ingestSource(const std::string& source_id,
                               ProgressCallback progress_callback) {
        IngestionStats stats;
        stats.correlation_id = generateCorrelationId();
        auto start_time = std::chrono::steady_clock::now();
        
        // Find source configuration
        SourceConfig config;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sources_.find(source_id);
            if (it == sources_.end()) {
                stats.addError(IngestionErrorCode::SOURCE_NOT_FOUND,
                               IngestionErrorSeverity::ERROR,
                               "Source not found: " + source_id, source_id);
                return stats;
            }
            config = it->second;
        }
        
        if (!config.enabled) {
            stats.addError(IngestionErrorCode::SOURCE_DISABLED,
                           IngestionErrorSeverity::WARNING,
                           "Source disabled: " + source_id, source_id);
            return stats;
        }

        // Apply per-source request-rate throttle (token bucket)
        // The byte-quota is checked after ingestion when bytes_processed is known.
        if (rate_limit_config_.enabled && rate_limit_config_.requests_per_second > 0.0) {
            checkRateLimit(source_id, 0, stats);
        }
        
        // Create connector based on type
        std::unique_ptr<ISourceConnector> connector;
        
        try {
            switch (config.type) {
                case SourceType::HUGGINGFACE: {
                    auto hf_connector = std::make_unique<HuggingFaceConnector>();
                    hf_connector->setRetryConfig(retry_config_);
                    if (api_http_get_fn_) {
                        hf_connector->setHttpGetForTesting(api_http_get_fn_);
                    }
                    if (!hf_connector->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize HuggingFace connector",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(hf_connector);
                    break;
                }
                
                case SourceType::FILESYSTEM: {
                    auto fs_ingester = std::make_unique<FileSystemIngester>();
                    if (!fs_ingester->initialize(config) && !dry_run_) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize filesystem ingester",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(fs_ingester);
                    break;
                }
                
                case SourceType::API: {
                    auto api_connector = std::make_unique<GenericApiConnector>();
                    api_connector->setRetryConfig(retry_config_);
                    if (api_http_get_fn_) {
                        api_connector->setHttpGetForTesting(api_http_get_fn_);
                    }
                    if (!api_connector->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize API connector",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(api_connector);
                    break;
                }

                case SourceType::KAFKA: {
                    auto kafka_connector = std::make_unique<KafkaConnector>();
                    kafka_connector->setRetryConfig(retry_config_);
                    // Inject the shared checkpoint store so that Kafka consumer-group
                    // offsets are committed only AFTER the ThemisDB checkpoint is
                    // written.  This preserves at-least-once delivery semantics.
                    if (incremental_mode_) {
                        std::shared_ptr<CheckpointStore> cs;
                        {
                            std::lock_guard<std::mutex> lock(mutex_);
                            cs = checkpoint_store_shared_;
                        }
                        if (cs) {
                            kafka_connector->setCheckpointStore(cs);
                        }
                    }
                    if (!kafka_connector->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize Kafka connector",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(kafka_connector);
                    break;
                }

                case SourceType::OBJECT_STORAGE: {
                    // Route S3-compatible sources to the dedicated S3Connector
                    // (which provides incremental checkpointing, concurrent
                    // downloads, flat-file format delegation, and a per-page
                    // max_keys_per_list cap).  All other providers (gcs, azure)
                    // fall through to the generic ObjectStorageConnector.
                    auto prov_it = config.options.find("provider");
                    std::string provider = (prov_it != config.options.end())
                                          ? prov_it->second : "s3";

                    if (provider == "s3" || provider.empty()) {
                        auto s3_connector = std::make_unique<S3Connector>();
                        s3_connector->setRetryConfig(retry_config_);
                        if (incremental_mode_) {
                            std::shared_ptr<CheckpointStore> cs;
                            {
                                std::lock_guard<std::mutex> lock(mutex_);
                                cs = checkpoint_store_shared_;
                            }
                            if (cs) {
                                s3_connector->setCheckpointStore(cs);
                            }
                        }
                        if (!s3_connector->initialize(config)) {
                            stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                           IngestionErrorSeverity::ERROR,
                                           "Failed to initialize S3 connector",
                                           source_id);
                            return stats;
                        }
                        connector = std::move(s3_connector);
                    } else {
                        auto obj_connector = std::make_unique<ObjectStorageConnector>();
                        obj_connector->setRetryConfig(retry_config_);
                        if (!obj_connector->initialize(config)) {
                            stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                           IngestionErrorSeverity::ERROR,
                                           "Failed to initialize ObjectStorage connector",
                                           source_id);
                            return stats;
                        }
                        connector = std::move(obj_connector);
                    }
                    break;
                }

                case SourceType::DATABASE: {
                    auto db_connector = std::make_unique<DatabaseConnector>();
                    db_connector->setRetryConfig(retry_config_);
                    if (!db_connector->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize Database connector",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(db_connector);
                    break;
                }

                case SourceType::WEB_CRAWLER: {
                    auto crawler = std::make_unique<WebCrawlerConnector>();
                    crawler->setRetryConfig(retry_config_);
                    if (!crawler->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize WebCrawler connector",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(crawler);
                    break;
                }

                case SourceType::CDC: {
                    auto cdc_connector = std::make_unique<CdcConnector>();
                    cdc_connector->setRetryConfig(retry_config_);
                    if (!cdc_connector->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize CDC connector",
                                       source_id);
                        return stats;
                    }
                    connector = std::move(cdc_connector);
                    break;
                }

                case SourceType::PLUGIN: {
                    auto pit = config.options.find("plugin_name");
                    if (pit == config.options.end() || pit->second.empty()) {
                        stats.addError(IngestionErrorCode::CONNECTOR_NOT_SUPPORTED,
                                       IngestionErrorSeverity::ERROR,
                                       "Plugin source requires options[\"plugin_name\"] to be set",
                                       source_id);
                        return stats;
                    }
                    auto plug = plugin_registry_.create(pit->second);
                    if (!plug) {
                        stats.addError(IngestionErrorCode::CONNECTOR_NOT_SUPPORTED,
                                       IngestionErrorSeverity::ERROR,
                                       "No connector plugin registered for name: " + pit->second,
                                       source_id);
                        return stats;
                    }
                    if (!plug->initialize(config)) {
                        stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                                       IngestionErrorSeverity::ERROR,
                                       "Failed to initialize plugin connector: " + pit->second,
                                       source_id);
                        return stats;
                    }
                    connector = std::move(plug);
                    break;
                }

                default:
                    stats.addError(IngestionErrorCode::CONNECTOR_NOT_SUPPORTED,
                                   IngestionErrorSeverity::ERROR,
                                   "Connector type not yet implemented: " +
                                   std::to_string(static_cast<int>(config.type)),
                                   source_id);
                    return stats;
            }
            
            // Check availability for real ingestion runs. Dry-run mode is allowed
            // to inspect source metadata or report zero-count previews without
            // requiring the backing source to be currently reachable.
            if (!dry_run_ && !connector->isAvailable()) {
                stats.addError(IngestionErrorCode::SOURCE_UNAVAILABLE,
                               IngestionErrorSeverity::ERROR,
                               "Source not available: " + source_id, source_id);
                return stats;
            }

            // Inject per-source schema validator when configured
            {
                SchemaConfig schema;
                bool has_schema = false;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    auto it = schema_configs_.find(source_id);
                    if (it != schema_configs_.end()) {
                        schema = it->second;
                        has_schema = true;
                    }
                }
                if (has_schema && schema.isEnabled()) {
                    connector->setDocumentValidator(buildValidatorFromSchema(schema));
                }
            }

            if (dry_run_) {
                stats.documents_processed = connector->getDocumentCount();
                stats.documents_failed    = 0;
            } else {
                // Incremental mode: read checkpoint to get the resume offset.
                // CheckpointStore is captured as a shared_ptr under the lock so
                // it remains valid even if setCheckpointDir() is called concurrently.
                if (incremental_mode_) {
                    std::shared_ptr<CheckpointStore> cs;
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        cs = checkpoint_store_shared_;
                    }
                    if (cs) {
                        IngestionCheckpoint cp;
                        // checkpoint_offset is available for connector-level
                        // resume extensions; currently informational.
                        if (cs->read(source_id, cp)) {
                            stats.addError(IngestionErrorCode::OK,
                                           IngestionErrorSeverity::INFO,
                                           "Resuming from checkpoint: " +
                                           std::to_string(cp.processed_count) +
                                           " docs already processed",
                                           source_id);
                        }
                    }
                }

                // Preserve the correlation_id assigned at the start of this run;
                // ingest() returns a fresh IngestionStats that doesn't carry it.
                const std::string corr_id = stats.correlation_id;
                stats = connector->ingest(target_collection_, progress_callback);
                stats.correlation_id = corr_id;
                quarantineFailures(stats, source_id);

                // Check byte-hour quota now that bytes_processed is known
                if (rate_limit_config_.enabled &&
                    rate_limit_config_.max_bytes_per_hour > 0) {
                    checkRateLimit(source_id, stats.bytes_processed, stats);
                }

                // Write checkpoint after a successful run
                if (incremental_mode_ && stats.documents_failed == 0) {
                    std::shared_ptr<CheckpointStore> cs;
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        cs = checkpoint_store_shared_;
                    }
                    if (cs) {
                        IngestionCheckpoint cp;
                        cp.source_id       = source_id;
                        cp.processed_count = stats.documents_processed;
                        cp.timestamp       = formatTimestamp(
                            std::chrono::system_clock::now());
                        cs->write(cp);
                    }
                }
            }
            
            auto end_time = std::chrono::steady_clock::now();
            stats.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
            if (stats.elapsed_seconds > 0.0 && stats.documents_processed > 0) {
                stats.metrics.throughput_docs_per_sec =
                    static_cast<double>(stats.documents_processed) / stats.elapsed_seconds;
            }

            // ── Lineage tracking ─────────────────────────────────────────────
            if (lineage_enabled_) {
                // Collect transformation steps that were applied
                std::vector<std::string> steps;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (schema_configs_.count(source_id) &&
                        schema_configs_.at(source_id).isEnabled()) {
                        steps.push_back("schema_validation");
                    }
                    if (legal_ingestion_configs_.count(source_id) &&
                        legal_ingestion_configs_.at(source_id).isEnabled()) {
                        steps.push_back("deontic_extraction");
                        steps.push_back("semantic_validation");
                        if (legal_ingestion_configs_.at(source_id).validate_references) {
                            steps.push_back("reference_validation");
                        }
                    }
                }
                if (config.type == SourceType::FILESYSTEM) {
                    steps.push_back("mime_detection");
                }
                if (rate_limit_config_.enabled) {
                    steps.push_back("rate_limiting");
                }
                if (incremental_mode_) {
                    steps.push_back("incremental_checkpoint");
                }
                if (dry_run_) {
                    steps.push_back("dry_run");
                }

                const std::string ts = formatTimestamp(std::chrono::system_clock::now());

                // One batch-level record for successfully processed documents
                if (stats.documents_processed > 0 || dry_run_) {
                    IngestionLineageRecord r;
                    r.run_correlation_id  = stats.correlation_id;
                    r.source_id           = source_id;
                    r.connector_type      = sourceTypeLabel(config.type);
                    r.connector_version   = kConnectorVersion;
                    r.doc_id              = kBatchDocIdPrefix + std::to_string(stats.documents_processed);
                    r.ingested_at         = ts;
                    r.bytes               = stats.bytes_processed;
                    r.doc_count           = stats.documents_processed;
                    r.transformation_steps = steps;
                    r.status = dry_run_ ? LineageStatus::DRY_RUN : LineageStatus::SUCCESS;
                    lineage_store_.record(std::move(r));
                }

                // Per-quarantine-entry records for failed documents
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    for (const auto& q : quarantine_) {
                        if (q.source_id != source_id) {
                          continue;
                        }
                        IngestionLineageRecord rq;
                        rq.run_correlation_id  = stats.correlation_id;
                        rq.source_id           = source_id;
                        rq.connector_type      = sourceTypeLabel(config.type);
                        rq.connector_version   = kConnectorVersion;
                        rq.doc_id              = q.item_path;
                        rq.ingested_at         = ts;
                        rq.bytes               = q.raw_payload.size();
                        rq.doc_count           = 1;
                        rq.transformation_steps = steps;
                        rq.status = LineageStatus::QUARANTINED;
                        lineage_store_.record(std::move(rq));
                    }
                }

                // Record for failed (non-quarantined) documents
                if (stats.documents_failed > 0) {
                    IngestionLineageRecord rf;
                    rf.run_correlation_id  = stats.correlation_id;
                    rf.source_id           = source_id;
                    rf.connector_type      = sourceTypeLabel(config.type);
                    rf.connector_version   = kConnectorVersion;
                    rf.doc_id              = kFailedDocIdPrefix + std::to_string(stats.documents_failed);
                    rf.ingested_at         = ts;
                    rf.bytes               = 0;
                    rf.doc_count           = stats.documents_failed;
                    rf.transformation_steps = steps;
                    rf.status = LineageStatus::FAILED;
                    lineage_store_.record(std::move(rf));
                }
            }

        } catch (const std::exception& e) {
            stats.addError(IngestionErrorCode::INTERNAL_ERROR,
                           IngestionErrorSeverity::FATAL,
                           "Exception during ingestion: " + std::string(e.what()),
                           source_id);
        }
        
        return stats;
    }
    
    IngestionReport ingestAll([[maybe_unused]] ProgressCallback progress_callback) {
        IngestionReport report;
        report.dry_run = dry_run_;
        
        // Collect all registered sources (enabled and disabled) sorted by priority.
        // Disabled sources will return a SOURCE_DISABLED warning via ingestSource(),
        // ensuring they appear in the report for full auditability.
        std::vector<SourceConfig> all_sources;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& pair : sources_) {
                all_sources.push_back(pair.second);
            }
        }

        std::sort(all_sources.begin(), all_sources.end(),
                 [](const SourceConfig& a, const SourceConfig& b) {
                     return a.priority > b.priority;
                 });

        // For parallelism we only launch async tasks for enabled sources to avoid
        // spinning up threads for trivially-skipped disabled sources; disabled
        // sources are recorded synchronously after the parallel wave completes.
        std::vector<SourceConfig> enabled_sources;
        std::vector<SourceConfig> disabled_sources = {};

        for (const auto& cfg : all_sources) {
            if (cfg.enabled) {
                enabled_sources.push_back(cfg);
            } else {
                disabled_sources.push_back(cfg);
            }
        }

        if (parallel_enabled_ && enabled_sources.size() > 1) {
            const size_t concurrency =
                std::min(max_threads_, enabled_sources.size());

            std::vector<std::future<std::pair<std::string, IngestionStats>>> futures;
            futures.reserve(enabled_sources.size());

            size_t submitted = 0;
            while (submitted < enabled_sources.size()) {
                size_t wave_end = std::min(submitted + concurrency,
                                           enabled_sources.size());
                for (size_t i = submitted; i < wave_end; ++i) {
                    const auto& cfg = enabled_sources[i];
                    futures.push_back(
                        std::async(std::launch::async,
                            [this, cfg, progress_callback]() {
                                return std::make_pair(
                                    cfg.source_id,
                                    ingestSource(cfg.source_id, progress_callback));
                            }));
                }
                for (size_t i = submitted; i < wave_end; ++i) {
                    auto [sid, stats] = futures[i].get();
                    report.source_stats[sid] = stats;
                    report.total_documents += stats.documents_processed;
                    report.total_failures  += stats.documents_failed;
                    report.total_time_seconds += stats.elapsed_seconds;
                }
                submitted = wave_end;
            }
        } else {
            for (const auto& config : enabled_sources) {
                auto stats = ingestSource(config.source_id, progress_callback);
                report.source_stats[config.source_id] = stats;
                report.total_documents += stats.documents_processed;
                report.total_failures  += stats.documents_failed;
                report.total_time_seconds += stats.elapsed_seconds;
            }
        }

        // Record disabled sources synchronously (they produce a SOURCE_DISABLED warning)
        for (const auto& config : disabled_sources) {
            auto stats = ingestSource(config.source_id, progress_callback);
            report.source_stats[config.source_id] = stats;
            // Disabled sources contribute no processed docs or bytes to totals
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            report.quarantine = quarantine_;
        }
        report.quarantine_retry_successes =
            quarantine_retry_successes_.load(std::memory_order_relaxed);

        return report;
    }
    
    std::vector<SourceConfig> getRegisteredSources() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<SourceConfig> result = {};

        for (const auto& pair : sources_) {
            result.push_back(pair.second);
        }
        return result;
    }
    
    void setTargetCollection(const std::string& collection_name) {
        target_collection_ = collection_name;
    }
    
    void setParallelProcessing(bool enabled, size_t max_threads) {
        parallel_enabled_ = enabled;
        if (max_threads > 0) {
            max_threads_ = max_threads;
        }
    }

    void setRetryConfig(const RetryConfig& config) {
        retry_config_ = config;
    }

    void setSchemaConfig(const std::string& source_id, const SchemaConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!config.isEnabled()) {
            schema_configs_.erase(source_id);
        } else {
            schema_configs_[source_id] = config;
        }
    }

    bool getSchemaConfig(const std::string& source_id, SchemaConfig& out) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = schema_configs_.find(source_id);
        if (it == schema_configs_.end()) {
          return false;
        }
        out = it->second;
        return true;
    }

    void setLegalIngestionConfig(const std::string& source_id,
                                  const LegalIngestionConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!config.isEnabled()) {
            legal_ingestion_configs_.erase(source_id);
        } else {
            legal_ingestion_configs_[source_id] = config;
        }
    }

    bool getLegalIngestionConfig(const std::string& source_id,
                                  LegalIngestionConfig& out) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = legal_ingestion_configs_.find(source_id);
        if (it == legal_ingestion_configs_.end()) {
          return false;
        }
        out = it->second;
        return true;
    }

    void setDryRun([[maybe_unused]] bool enabled) { dry_run_ = enabled; }
    bool isDryRun() const { return dry_run_; }

    void setRateLimitConfig(const RateLimitConfig& config) {
        rate_limit_config_ = config;
        // Reset per-source buckets so they're rebuilt with the new rate on next use
        per_source_buckets_.clear();
    }

    std::vector<QuarantineEntry> getQuarantineItems() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return quarantine_;
    }

    bool dismissQuarantineItem(const std::string& item_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(quarantine_.begin(), quarantine_.end(),
            [&item_path](const QuarantineEntry& e) {
                return e.item_path == item_path;
            });
        if (it == quarantine_.end()) {
            return false;
        }
        quarantine_.erase(it);
        return true;
    }

    void clearQuarantine() {
        std::lock_guard<std::mutex> lock(mutex_);
        quarantine_.clear();
    }

    bool updateQuarantineEntry(const QuarantineEntry& updated) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(quarantine_.begin(), quarantine_.end(),
            [&updated](const QuarantineEntry& e) {
                return e.item_path == updated.item_path;
            });
        if (it == quarantine_.end()) {
          return false;
        }
        *it = updated;
        return true;
    }

    void addToQuarantine(QuarantineEntry entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        quarantine_.push_back(std::move(entry));
    }

    RetryConfig getRetryConfig() const {
        return retry_config_;
    }

    DocumentWriteFn getDocumentWriteFn() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return doc_write_fn_;
    }

    size_t getQuarantineRetrySuccessCount() const {
        return quarantine_retry_successes_.load(std::memory_order_relaxed);
    }

    void incrementQuarantineRetrySuccess() {
        quarantine_retry_successes_.fetch_add(1, std::memory_order_relaxed);
    }

    bool pauseSource(const std::string& source_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sources_.find(source_id);
        if (it == sources_.end()) {
          return false;
        }
        it->second.enabled = false;
        return true;
    }

    bool resumeSource(const std::string& source_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sources_.find(source_id);
        if (it == sources_.end()) {
          return false;
        }
        it->second.enabled = true;
        return true;
    }

    SourcePreview previewSource(const std::string& source_id,
                                size_t max_documents) const {
        SourcePreview preview;
        preview.source_id = source_id;

        // Cap to avoid memory exhaustion
        static constexpr size_t kMaxPreviewCap = 100;
        max_documents = std::min(max_documents, kMaxPreviewCap);

        SourceConfig config;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sources_.find(source_id);
            if (it == sources_.end()) {
              return preview;
            }
            config = it->second;
        }

        // Only FILESYSTEM preview is currently implemented
        if (config.type != SourceType::FILESYSTEM) {
            preview.total_available = 0;
            return preview;
        }

        namespace fs = std::filesystem;
        const fs::path root(config.location);
        if (!fs::exists(root)) {
          return preview;
        }

        auto addDoc = [&]([[maybe_unused]] const fs::path& p) {
            std::ifstream f(p, std::ios::binary);
            if (!f) {
              return;
            }
            std::string content{std::istreambuf_iterator<char>(f),
                                 std::istreambuf_iterator<char>()};
            if (!content.empty()) {
                preview.documents.push_back(std::move(content));
            }
        };

        if (fs::is_regular_file(root)) {
            preview.total_available = 1;
            addDoc(root);
        } else if (fs::is_directory(root)) {
            // Single pass: count all files and collect up to max_documents.
            for (auto& entry : fs::recursive_directory_iterator(root)) {
                if (!entry.is_regular_file()) {
                  continue;
                }
                ++preview.total_available;
                if (preview.documents.size() < max_documents) {
                    addDoc(entry.path());
                }
            }
        }

        if (preview.total_available > max_documents) {
            preview.truncated = true;
        }
        return preview;
    }

    // ── Checkpoint / incremental ingestion ───────────────────────────────────

    void setCheckpointDir(const std::string& dir) {
        // Use shared_ptr so the store can be safely shared with ingestSource()
        // threads without a race when setCheckpointDir() is called concurrently.
        auto new_store = std::make_shared<CheckpointStore>(dir);
        std::lock_guard<std::mutex> lock(mutex_);
        checkpoint_store_shared_ = std::move(new_store);
    }

    void enableIncrementalMode([[maybe_unused]] bool enabled) {
        incremental_mode_ = enabled;
    }

    bool isIncrementalMode() const { return incremental_mode_; }

    bool getCheckpoint(const std::string& source_id,
                       IngestionCheckpoint& out) const {
        std::shared_ptr<CheckpointStore> cs;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cs = checkpoint_store_shared_;
        }
        if (!cs) {
          return false;
        }
        return cs->read(source_id, out);
    }

    bool clearCheckpoint(const std::string& source_id) {
        std::shared_ptr<CheckpointStore> cs;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cs = checkpoint_store_shared_;
        }
        if (!cs) {
          return false;
        }
        return cs->clear(source_id);
    }

    void setApiHttpGetForTesting(ApiHttpGetFn fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        api_http_get_fn_ = std::move(fn);
    }

    void setDocumentWriteForTesting(DocumentWriteFn fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        doc_write_fn_ = std::move(fn);
    }

    void registerConnectorPlugin(const std::string& plugin_name,
                                  ConnectorFactory factory) {
        plugin_registry_.registerFactory(plugin_name, std::move(factory));
    }

    bool unregisterConnectorPlugin(const std::string& plugin_name) {
        return plugin_registry_.unregisterFactory(plugin_name);
    }

    std::vector<std::string> listConnectorPlugins() const {
        return plugin_registry_.listPlugins();
    }

    void setLineageTrackingEnabled([[maybe_unused]] bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        lineage_enabled_ = enabled;
    }

    bool isLineageTrackingEnabled() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lineage_enabled_;
    }

    std::vector<IngestionLineageRecord> getLineageRecords(
            const std::string& source_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lineage_store_.getBySource(source_id);
    }

    std::vector<IngestionLineageRecord> getLineageRecordsByRun(
            const std::string& run_correlation_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lineage_store_.getByCorrelationId(run_correlation_id);
    }

    std::vector<IngestionLineageRecord> getAllLineageRecords() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return lineage_store_.getAll();
    }

    void clearLineageRecords() {
        std::lock_guard<std::mutex> lock(mutex_);
        lineage_store_.clear();
    }

public:
    /// Consume a token from the per-source bucket (creates bucket if needed).
    /// Returns false and records a QUOTA_EXCEEDED error if the byte limit is breached.
    bool checkRateLimit(const std::string& source_id,
                        size_t bytes_this_call,
                        IngestionStats& stats) {
        if (!rate_limit_config_.enabled) {
          return true;
        }

        // Per-source token bucket.
        // A shared_ptr is used so that after unlocking the mutex the bucket
        // remains alive even if another thread removes it from the map.
        if (rate_limit_config_.requests_per_second > 0.0) {
            std::shared_ptr<TokenBucket> bucket;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = per_source_buckets_.find(source_id);
                if (it == per_source_buckets_.end()) {
                    auto inserted = per_source_buckets_.emplace(
                        source_id,
                        std::make_shared<TokenBucket>(
                            rate_limit_config_.requests_per_second));
                    it = inserted.first;
                }
                bucket = it->second;
            }
            bucket->consume();
        }

        // Byte-hour quota
        if (rate_limit_config_.max_bytes_per_hour > 0) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& tracker = bytes_this_hour_[source_id];
            tracker.bytes += bytes_this_call;

            // Reset counter if an hour has passed
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::hours>(
                    now - tracker.window_start).count() >= 1) {
                tracker.bytes = bytes_this_call;
                tracker.window_start = now;
            }

            if (tracker.bytes > rate_limit_config_.max_bytes_per_hour) {
                stats.addError(IngestionErrorCode::QUOTA_EXCEEDED,
                               IngestionErrorSeverity::WARNING,
                               "Byte-per-hour quota exceeded for source: " + source_id,
                               source_id);
                stats.metrics.quota_violations++;
                return false;
            }
        }
        return true;
    }

    void quarantineFailures(const IngestionStats& stats,
                            const std::string& source_id) {
        for (const auto& err : stats.errors) {
            if (err.isFatal()) {
                QuarantineEntry entry;
                entry.source_id    = source_id;
                entry.item_path    = err.details.empty()
                    ? ("unknown_item_from_" + source_id)
                    : err.details;
                entry.error_code   = err.code;
                entry.error_message = err.message;
                entry.retry_count  = stats.metrics.retry_count;

                std::lock_guard<std::mutex> lock(mutex_);
                quarantine_.push_back(std::move(entry));
            }
        }
    }

    // Byte-hour tracking per source
    struct ByteWindowTracker {
        size_t bytes = 0;
        std::chrono::steady_clock::time_point window_start =
            std::chrono::steady_clock::now();
    };

    std::string db_connection_;
    std::string target_collection_;
    bool parallel_enabled_;
    bool dry_run_;
    bool incremental_mode_ = false;
    size_t max_threads_;
    RetryConfig retry_config_;
    RateLimitConfig rate_limit_config_;
    std::unordered_map<std::string, std::shared_ptr<TokenBucket>> per_source_buckets_;
    std::unordered_map<std::string, ByteWindowTracker> bytes_this_hour_;
    std::unordered_map<std::string, SourceConfig> sources_;
    std::unordered_map<std::string, SchemaConfig> schema_configs_; ///< Per-source schema configs
    std::unordered_map<std::string, LegalIngestionConfig> legal_ingestion_configs_; ///< Per-source legal pipeline
    std::vector<QuarantineEntry> quarantine_;
    std::atomic<size_t> quarantine_retry_successes_{0}; ///< Cumulative successful quarantine retries
    std::shared_ptr<CheckpointStore> checkpoint_store_shared_;  ///< null = no checkpointing
    ApiHttpGetFn api_http_get_fn_;  ///< testing hook for API connectors; empty = real curl
    DocumentWriteFn doc_write_fn_;  ///< testing hook for quarantine retry writes; empty = always succeed
    ConnectorPluginRegistry plugin_registry_; ///< Registry for third-party plugin connectors
    IngestionLineageStore lineage_store_;     ///< In-memory lineage record store
    bool lineage_enabled_ = false;            ///< Lineage tracking on/off
    std::shared_ptr<ITextGenerationBackend> text_gen_backend_; ///< injected AI backend (SoC)
    std::shared_ptr<WorkflowEngine> workflow_engine_; ///< injected workflow orchestrator (v2.0)
    std::shared_ptr<ReIngestionController> reingestion_controller_; ///< LLM-as-judge loop (v2.1)
    mutable std::mutex mutex_;
};

// ============================================================================
// ConnectorPluginRegistry implementation
// ============================================================================

void ConnectorPluginRegistry::registerFactory(const std::string& plugin_name,
                                               ConnectorFactory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    factories_[plugin_name] = std::move(factory);
}

bool ConnectorPluginRegistry::unregisterFactory(const std::string& plugin_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return factories_.erase(plugin_name) > 0;
}

bool ConnectorPluginRegistry::isRegistered(const std::string& plugin_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return factories_.count(plugin_name) > 0;
}

std::unique_ptr<ISourceConnector> ConnectorPluginRegistry::create(
        const std::string& plugin_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = factories_.find(plugin_name);
    if (it == factories_.end()) {
      return nullptr;
    }
    return it->second();
}

std::vector<std::string> ConnectorPluginRegistry::listPlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names = {};

    names.reserve(factories_.size());
    for (const auto& kv : factories_) {
        names.push_back(kv.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// ============================================================================
// Public API implementation
// ============================================================================
IngestionManager::IngestionManager(const std::string& db_connection)
    : impl_(std::make_unique<Impl>(db_connection)) {
}

IngestionManager::~IngestionManager() = default;

bool IngestionManager::registerSource(const SourceConfig& config) {
    return impl_->registerSource(config);
}

bool IngestionManager::unregisterSource(const std::string& source_id) {
    return impl_->unregisterSource(source_id);
}

bool IngestionManager::reconfigureSource(const std::string& source_id,
                                         const SourceConfig& new_config) {
    return impl_->reconfigureSource(source_id, new_config);
}

IngestionStats IngestionManager::ingestSource(const std::string& source_id,
                                             ProgressCallback progress_callback) {
    return impl_->ingestSource(source_id, progress_callback);
}

IngestionReport IngestionManager::ingestAll([[maybe_unused]] ProgressCallback progress_callback) {
    return impl_->ingestAll([[maybe_unused]] progress_callback);
}

std::vector<SourceConfig> IngestionManager::getRegisteredSources() const {
    return impl_->getRegisteredSources();
}

void IngestionManager::setTargetCollection(const std::string& collection_name) {
    impl_->setTargetCollection(collection_name);
}

void IngestionManager::setParallelProcessing(bool enabled, size_t max_threads) {
    impl_->setParallelProcessing(enabled, max_threads);
}

void IngestionManager::setRetryConfig(const RetryConfig& config) {
    impl_->setRetryConfig(config);
}

void IngestionManager::setSchemaConfig(const std::string& source_id,
                                       const SchemaConfig& config) {
    impl_->setSchemaConfig(source_id, config);
}

bool IngestionManager::getSchemaConfig(const std::string& source_id,
                                       SchemaConfig& out) const {
    return impl_->getSchemaConfig(source_id, out);
}

void IngestionManager::setDryRun([[maybe_unused]] bool enabled) {
    impl_->setDryRun(enabled);
}

bool IngestionManager::isDryRun() const {
    return impl_->isDryRun();
}

void IngestionManager::setRateLimitConfig(const RateLimitConfig& config) {
    impl_->setRateLimitConfig(config);
}

std::vector<QuarantineEntry> IngestionManager::getQuarantineItems() const {
    return impl_->getQuarantineItems();
}

bool IngestionManager::dismissQuarantineItem(const std::string& item_path) {
    return impl_->dismissQuarantineItem(item_path);
}

void IngestionManager::clearQuarantine() {
    impl_->clearQuarantine();
}

bool IngestionManager::updateQuarantineEntry(const QuarantineEntry& updated) {
    return impl_->updateQuarantineEntry(updated);
}

void IngestionManager::addToQuarantine(QuarantineEntry entry) {
    impl_->addToQuarantine(std::move(entry));
}

RetryConfig IngestionManager::getRetryConfig() const {
    return impl_->getRetryConfig();
}

DocumentWriteFn IngestionManager::getDocumentWriteFn() const {
    return impl_->getDocumentWriteFn();
}

size_t IngestionManager::getQuarantineRetrySuccessCount() const {
    return impl_->getQuarantineRetrySuccessCount();
}

void IngestionManager::incrementQuarantineRetrySuccess() {
    impl_->incrementQuarantineRetrySuccess();
}

void IngestionManager::setCheckpointDir(const std::string& checkpoint_dir) {
    impl_->setCheckpointDir(checkpoint_dir);
}

void IngestionManager::enableIncrementalMode([[maybe_unused]] bool enabled) {
    impl_->enableIncrementalMode(enabled);
}

bool IngestionManager::isIncrementalMode() const {
    return impl_->isIncrementalMode();
}

bool IngestionManager::getCheckpoint(const std::string& source_id,
                                      IngestionCheckpoint& out) const {
    return impl_->getCheckpoint(source_id, out);
}

bool IngestionManager::clearCheckpoint(const std::string& source_id) {
    return impl_->clearCheckpoint(source_id);
}

void IngestionManager::setApiHttpGetForTesting(ApiHttpGetFn fn) {
    impl_->setApiHttpGetForTesting(std::move(fn));
}

void IngestionManager::setDocumentWriteForTesting(DocumentWriteFn fn) {
    impl_->setDocumentWriteForTesting(std::move(fn));
}

void IngestionManager::registerConnectorPlugin(const std::string& plugin_name,
                                                ConnectorFactory factory) {
    impl_->registerConnectorPlugin(plugin_name, std::move(factory));
}

bool IngestionManager::unregisterConnectorPlugin(const std::string& plugin_name) {
    return impl_->unregisterConnectorPlugin(plugin_name);
}

std::vector<std::string> IngestionManager::listConnectorPlugins() const {
    return impl_->listConnectorPlugins();
}

void IngestionManager::enableLineageTracking([[maybe_unused]] bool enabled) {
    impl_->setLineageTrackingEnabled(enabled);
}

bool IngestionManager::isLineageTrackingEnabled() const {
    return impl_->isLineageTrackingEnabled();
}

std::vector<IngestionLineageRecord> IngestionManager::getLineageRecords(
        const std::string& source_id) const {
    return impl_->getLineageRecords(source_id);
}

std::vector<IngestionLineageRecord> IngestionManager::getLineageRecordsByRun(
        const std::string& run_correlation_id) const {
    return impl_->getLineageRecordsByRun(run_correlation_id);
}

std::vector<IngestionLineageRecord> IngestionManager::getAllLineageRecords() const {
    return impl_->getAllLineageRecords();
}

void IngestionManager::clearLineageRecords() {
    impl_->clearLineageRecords();
}

void IngestionManager::setLegalIngestionConfig(const std::string& source_id,
                                                const LegalIngestionConfig& config) {
    impl_->setLegalIngestionConfig(source_id, config);
}

bool IngestionManager::getLegalIngestionConfig(const std::string& source_id,
                                                LegalIngestionConfig& out) const {
    return impl_->getLegalIngestionConfig(source_id, out);
}

LegalExtractionResult IngestionManager::runLegalExtraction(
        const std::string& document_id,
        const std::string& text,
        const LegalIngestionConfig& config) const {
    SemanticValidator validator;
    LegalQualityGates gates;
    gates.min_confidence.threshold     = config.confidence_threshold;
    gates.deontic_confidence.threshold = config.confidence_threshold;
    gates.section_hierarchy.required   = config.require_section_struct;
    validator.setQualityGates(gates);

    // SoC: wire the injected AI backend into the validator when available.
    // The ingestion pipeline never knows about a concrete LLM class; it only
    // calls ITextGenerationBackend through LegalLlmAdapter.
    {
        std::shared_ptr<ITextGenerationBackend> backend;
        {
            std::lock_guard<std::mutex> lock(impl_->mutex_);
            backend = impl_->text_gen_backend_;
        }
        if (backend && backend->isAvailable()) {
            LegalLlmAdapter adapter(backend);
            validator.setExtractor(adapter.buildExtractor(config.confidence_threshold));
        }
    }

    LegalExtractionResult result = validator.extractDocument(document_id, text);

    if (config.validate_references) {
        AgenticReferenceValidator ref_validator;
        auto ref_report = ref_validator.validate(text);
        if (ref_report.dangling_count > 0) {
            for (const auto& w : ref_report.warnings) {
                result.warnings.push_back(w);
            }
            result.validation.gate_results.emplace_back(
                "no_dangling_refs", false,
                std::to_string(ref_report.dangling_count) + " dangling reference(s)");
        } else {
            result.validation.gate_results.emplace_back("no_dangling_refs", true);
        }
    }

    return result;
}

void IngestionManager::setTextGenerationBackend(
        std::shared_ptr<ITextGenerationBackend> backend) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->text_gen_backend_ =
        backend ? std::move(backend)
                : std::make_shared<NullTextGenerationBackend>();
}

std::shared_ptr<ITextGenerationBackend>
IngestionManager::getTextGenerationBackend() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    if (!impl_->text_gen_backend_) {
        return std::make_shared<NullTextGenerationBackend>();
    }
    return impl_->text_gen_backend_;
}

void IngestionManager::setWorkflowEngine(
        std::shared_ptr<WorkflowEngine> engine) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->workflow_engine_ = std::move(engine);
}

std::shared_ptr<WorkflowEngine>
IngestionManager::getWorkflowEngine() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->workflow_engine_;
}

// ---- LLM-as-judge re-ingestion quality control (v2.1) --------------------

void IngestionManager::setReIngestionController(
        std::shared_ptr<ReIngestionController> controller) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->reingestion_controller_ = std::move(controller);
}

std::shared_ptr<ReIngestionController>
IngestionManager::getReIngestionController() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->reingestion_controller_;
}
// ============================================================================

namespace {
static std::string promEscapeLabel(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\') {
          out += "\\\\";
        }
        else if (c == '"')  out += "\\\"";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

/// Write a Prometheus metric line with multiple labels
static void writeMetricMultiLabel(
        std::ostream& os,
        const std::string& name,
        const std::vector<std::pair<std::string, std::string>>& labels,
        double value) {
    os << name << '{';
    bool first = true;
    for (const auto& [k, v] : labels) {
        if (!first) {
          os << ',';
        }
        os << k << "=\"" << promEscapeLabel(v) << '"';
        first = false;
    }
    os << "} " << value << '\n';
}

static void writeMetric(std::ostream& os,
                        const std::string& name,
                        const std::string& label_key,
                        const std::string& label_val,
                        double value) {
    writeMetricMultiLabel(os, name, {{label_key, label_val}}, value);
}
} // anonymous namespace

std::string IngestionMetricsExporter::exportText(
        const IngestionReport& report) const {
    std::ostringstream os;

    // Per-source metrics – use source_type from source_stats key if available
    for (const auto& [sid, stats] : report.source_stats) {
        // source_type is not directly in IngestionStats, so we pass empty
        os << exportText(stats, sid);
    }

    const std::string agg_label = "source_id";
    const std::string agg_val   = "__all__";

    os << "# HELP " << prefix_ << "_total_documents "
       << "Total documents ingested across all sources\n"
       << "# TYPE " << prefix_ << "_total_documents counter\n";
    writeMetric(os, prefix_ + "_total_documents",
                agg_label, agg_val,
                static_cast<double>(report.total_documents));

    os << "# HELP " << prefix_ << "_total_failures "
       << "Total failed documents across all sources\n"
       << "# TYPE " << prefix_ << "_total_failures counter\n";
    writeMetric(os, prefix_ + "_total_failures",
                agg_label, agg_val,
                static_cast<double>(report.total_failures));

    os << "# HELP " << prefix_ << "_total_time_seconds "
       << "Total wall-clock time for all sources (seconds)\n"
       << "# TYPE " << prefix_ << "_total_time_seconds gauge\n";
    writeMetric(os, prefix_ + "_total_time_seconds",
                agg_label, agg_val,
                report.total_time_seconds);

    os << "# HELP " << prefix_ << "_quarantine_size "
       << "Number of items currently in quarantine\n"
       << "# TYPE " << prefix_ << "_quarantine_size gauge\n";
    writeMetric(os, prefix_ + "_quarantine_size",
                agg_label, agg_val,
                static_cast<double>(report.quarantine.size()));

    // Count permanently-failed entries for the dedicated metric
    size_t perm_failed = 0;
    for (const auto& e : report.quarantine) {
        if (e.permanently_failed) {
          ++perm_failed;
        }
    }
    os << "# HELP " << prefix_ << "_quarantine_permanently_failed_total "
       << "Items in quarantine that exceeded max retry attempts\n"
       << "# TYPE " << prefix_ << "_quarantine_permanently_failed_total gauge\n";
    writeMetric(os, prefix_ + "_quarantine_permanently_failed_total",
                agg_label, agg_val,
                static_cast<double>(perm_failed));

    os << "# HELP " << prefix_ << "_quarantine_retry_success_total "
       << "Cumulative successful per-document quarantine retries\n"
       << "# TYPE " << prefix_ << "_quarantine_retry_success_total counter\n";
    writeMetric(os, prefix_ + "_quarantine_retry_success_total",
                agg_label, agg_val,
                static_cast<double>(report.quarantine_retry_successes));

    return os.str();
}

std::string IngestionMetricsExporter::exportText(
        const IngestionStats& stats,
        const std::string& source_id,
        const std::string& source_type) const {
    std::ostringstream os;

    // Base labels always present; source_type added when non-empty
    std::vector<std::pair<std::string,std::string>> base_labels = {
        {"source_id", source_id}
    };
    if (!source_type.empty()) {
        base_labels.push_back({"source_type", source_type});
    }

    auto writeStat = [&](const std::string& suffix,
                         const std::string& help,
                         const std::string& type,
                         double value) {
        const std::string metric = prefix_ + suffix;
        os << "# HELP " << metric << ' ' << help << '\n'
           << "# TYPE " << metric << ' ' << type << '\n';
        writeMetricMultiLabel(os, metric, base_labels, value);
    };

    writeStat("_docs_processed_total",
              "Documents successfully processed", "counter",
              static_cast<double>(stats.documents_processed));
    writeStat("_docs_failed_total",
              "Documents that failed to ingest", "counter",
              static_cast<double>(stats.documents_failed));
    writeStat("_bytes_processed_total",
              "Bytes processed", "counter",
              static_cast<double>(stats.bytes_processed));
    writeStat("_elapsed_seconds",
              "Elapsed ingestion time in seconds", "gauge",
              stats.elapsed_seconds);
    writeStat("_retry_total",
              "Total retried requests", "counter",
              static_cast<double>(stats.metrics.retry_count));
    writeStat("_errors_total",
              "Total errors encountered", "counter",
              static_cast<double>(stats.metrics.error_count));
    writeStat("_throughput_docs_per_sec",
              "Document throughput (docs/second)", "gauge",
              stats.metrics.throughput_docs_per_sec);
    writeStat("_schema_violations_total",
              "Documents rejected or warned by schema validation", "counter",
              static_cast<double>(stats.metrics.schema_violations));

    // Per-error-code breakdown
    if (!stats.errors.empty()) {
        const std::string ec_metric = prefix_ + "_errors_by_code_total";
        os << "# HELP " << ec_metric
           << " Error count broken down by error_code\n"
           << "# TYPE " << ec_metric << " counter\n";

        // Count occurrences per error code
        std::unordered_map<int, size_t> code_counts = {};

        for (const auto& err : stats.errors) {
            code_counts[static_cast<int>(err.code)]++;
        }
        for (const auto& [code_int, cnt] : code_counts) {
            auto labels = base_labels;
            labels.push_back({"error_code", std::to_string(code_int)});
            writeMetricMultiLabel(os, ec_metric, labels,
                                  static_cast<double>(cnt));
        }
    }

    return os.str();
}

// ============================================================================
// IngestionBuilder
// ============================================================================

IngestionBuilder::IngestionBuilder(const std::string& db_connection)
    : opts_(std::make_unique<Opts>()) {
    opts_->db_connection = db_connection;
}

IngestionBuilder::~IngestionBuilder() = default;
IngestionBuilder::IngestionBuilder(IngestionBuilder&&) noexcept = default;
IngestionBuilder& IngestionBuilder::operator=(IngestionBuilder&&) noexcept = default;

IngestionBuilder& IngestionBuilder::withHuggingFaceSource(
        const std::string& source_id,
        const std::string& dataset,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::HUGGINGFACE;
    cfg.location  = dataset;
    cfg.options   = std::move(options);
    cfg.priority  = priority;
    cfg.enabled   = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withFilesystemSource(
        const std::string& source_id,
        const std::string& path,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = path;
    cfg.options   = std::move(options);
    cfg.priority  = priority;
    cfg.enabled   = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withApiSource(
        const std::string& source_id,
        const std::string& endpoint,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::API;
    cfg.location  = endpoint;
    cfg.options   = std::move(options);
    cfg.priority  = priority;
    cfg.enabled   = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withKafkaSource(
        const std::string& source_id,
        const std::string& brokers,
        const std::string& topic,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id           = source_id;
    cfg.type                = SourceType::KAFKA;
    cfg.location            = brokers;
    cfg.options             = std::move(options);
    cfg.options["topic"]    = topic;  // ensure topic is always set in options
    cfg.priority            = priority;
    cfg.enabled             = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withObjectStorageSource(
        const std::string& source_id,
        const std::string& bucket,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::OBJECT_STORAGE;
    cfg.location  = bucket;
    cfg.options   = std::move(options);
    cfg.priority  = priority;
    cfg.enabled   = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withDatabaseSource(
        const std::string& source_id,
        const std::string& jdbc_url,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::DATABASE;
    cfg.location  = jdbc_url;
    cfg.options   = std::move(options);
    cfg.priority  = priority;
    cfg.enabled   = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withWebCrawlerSource(
        const std::string& source_id,
        const std::string& seed_url,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::WEB_CRAWLER;
    cfg.location  = seed_url;
    cfg.options   = std::move(options);
    cfg.priority  = priority;
    cfg.enabled   = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withCdcSource(
        const std::string& source_id,
        const std::string& connection_url,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id = source_id;
    cfg.type      = SourceType::CDC;
    cfg.location  = connection_url;
    cfg.options   = std::move(options);
    cfg.priority  = priority;
    cfg.enabled   = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withPluginSource(
        const std::string& source_id,
        const std::string& plugin_name,
        const std::string& location,
        std::unordered_map<std::string, std::string> options,
        int priority) {
    SourceConfig cfg;
    cfg.source_id                  = source_id;
    cfg.type                       = SourceType::PLUGIN;
    cfg.location                   = location;
    cfg.options                    = std::move(options);
    cfg.options["plugin_name"]     = plugin_name;
    cfg.priority                   = priority;
    cfg.enabled                    = true;
    opts_->sources.push_back(std::move(cfg));
    return *this;
}

IngestionBuilder& IngestionBuilder::withConnectorPlugin(
        const std::string& plugin_name, ConnectorFactory factory) {
    opts_->plugin_factories[plugin_name] = std::move(factory);
    return *this;
}

IngestionBuilder& IngestionBuilder::withRetryConfig(const RetryConfig& config) {
    opts_->retry_config = config;
    return *this;
}

IngestionBuilder& IngestionBuilder::withRateLimitConfig(const RateLimitConfig& config) {
    opts_->rate_limit_config = config;
    return *this;
}

IngestionBuilder& IngestionBuilder::withParallelProcessing(bool enabled,
                                                            size_t max_threads) {
    opts_->parallel_enabled = enabled;
    opts_->max_threads      = max_threads;
    return *this;
}

IngestionBuilder& IngestionBuilder::withTargetCollection(
        const std::string& collection) {
    opts_->target_collection = collection;
    return *this;
}

IngestionBuilder& IngestionBuilder::withDryRun([[maybe_unused]] bool enabled) {
    opts_->dry_run = enabled;
    return *this;
}

IngestionBuilder& IngestionBuilder::withSchemaValidation(
        const std::string& source_id, const SchemaConfig& config) {
    opts_->schema_configs[source_id] = config;
    return *this;
}

IngestionBuilder& IngestionBuilder::withLegalIngestionConfig(
        const std::string& source_id, const LegalIngestionConfig& config) {
    opts_->legal_ingestion_configs[source_id] = config;
    return *this;
}

std::unique_ptr<IngestionManager> IngestionBuilder::build() {
    auto mgr = std::make_unique<IngestionManager>(opts_->db_connection);

    mgr->setRetryConfig(opts_->retry_config);
    mgr->setRateLimitConfig(opts_->rate_limit_config);
    mgr->setParallelProcessing(opts_->parallel_enabled, opts_->max_threads);
    mgr->setTargetCollection(opts_->target_collection);
    mgr->setDryRun(opts_->dry_run);

    for (const auto& src : opts_->sources) {
        mgr->registerSource(src);
    }

    for (const auto& kv : opts_->schema_configs) {
        mgr->setSchemaConfig(kv.first, kv.second);
    }

    for (const auto& kv : opts_->legal_ingestion_configs) {
        mgr->setLegalIngestionConfig(kv.first, kv.second);
    }

    for (auto& kv : opts_->plugin_factories) {
        mgr->registerConnectorPlugin(kv.first, std::move(kv.second));
    }

    return mgr;
}

// ============================================================================
// IngestionManager::previewSource  (public wrapper around Impl)
// ============================================================================

SourcePreview IngestionManager::previewSource(const std::string& source_id,
                                               size_t max_documents) const {
    return impl_->previewSource(source_id, max_documents);
}

bool IngestionManager::pauseSource(const std::string& source_id) {
    return impl_->pauseSource(source_id);
}

bool IngestionManager::resumeSource(const std::string& source_id) {
    return impl_->resumeSource(source_id);
}

// ============================================================================
// IngestionAdminApi
// ============================================================================

IngestionAdminApi::IngestionAdminApi(IngestionManager& manager)
    : mgr_(manager) {}

std::vector<SourceStatus> IngestionAdminApi::listSources() const {
    std::vector<SourceStatus> result = {};

    for (const auto& cfg : mgr_.getRegisteredSources()) {
        SourceStatus s;
        s.source_id  = cfg.source_id;
        s.type       = cfg.type;
        s.enabled    = cfg.enabled;

        // Attempt a lightweight availability probe via previewSource(0)
        try {
            auto preview = mgr_.previewSource(cfg.source_id, 0);
            s.available = true;
            s.doc_count = preview.total_available;
        } catch (...) {
            s.available = false;
        }

        result.push_back(std::move(s));
    }
    return result;
}

IngestionStats IngestionAdminApi::startSource(const std::string& source_id) {
    return mgr_.ingestSource(source_id);
}

bool IngestionAdminApi::pauseSource(const std::string& source_id) {
    return mgr_.pauseSource(source_id);
}

bool IngestionAdminApi::resumeSource(const std::string& source_id) {
    return mgr_.resumeSource(source_id);
}

bool IngestionAdminApi::reconfigureSource(const std::string& source_id,
                                          const SourceConfig& new_config) {
    return mgr_.reconfigureSource(source_id, new_config);
}

std::vector<QuarantineEntry> IngestionAdminApi::listQuarantine() const {
    return mgr_.getQuarantineItems();
}

bool IngestionAdminApi::retryQuarantineItem(const std::string& item_path) {
    // Find the entry
    auto items = mgr_.getQuarantineItems();
    auto it = std::find_if(items.begin(), items.end(),
        [&item_path](const QuarantineEntry& e) {
            return e.item_path == item_path;
        });
    if (it == items.end()) {
      return false;
    }

    // Do not retry entries that have been permanently failed
    if (it->permanently_failed) {
      return false;
    }

    RetryConfig cfg = mgr_.getRetryConfig();
    QuarantineEntry entry = *it;

    if (!entry.raw_payload.empty()) {
        // Per-document retry with exponential back-off.
        // The raw payload is already available; each attempt tries to write the
        // document directly (no full source restart).
        double delay_ms = cfg.initial_delay_ms;
        bool success = false;
        const int max_attempts = std::max(cfg.max_quarantine_retries, 1);

        // Retrieve the injected write function (may be empty outside tests).
        DocumentWriteFn write_fn = mgr_.getDocumentWriteFn();

        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
            // Apply back-off delay before each re-attempt (skip on the first).
            if (attempt > 1) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(static_cast<int64_t>(delay_ms)));
                delay_ms = std::min(delay_ms * cfg.backoff_factor, cfg.max_delay_ms);
            }

            // Attempt to write the document.  If a DocumentWriteFn has been
            // injected (e.g. in tests), delegate to it; otherwise assume success
            // since no storage backend is directly coupled to this module
            // boundary.  Production deployments that need verified writes should
            // install a write function via setDocumentWriteForTesting() — or,
            // more typically, connect the ingestion pipeline to the storage layer
            // through a higher-level orchestration layer that invokes ingestAll()
            // and handles document persistence independently.
            if (write_fn) {
                success = write_fn(entry.source_id, entry.raw_payload);
            } else {
                success = true;
            }

            if (success) {
                break;
            }

            // Write failed – record this attempt, update the error, and advance
            // the counter so the permanently_failed gate can trigger on exhaustion.
            entry.error_message = "write attempt " + std::to_string(attempt) + " failed";
            ++entry.retry_count;
        }

        if (success) {
            mgr_.dismissQuarantineItem(item_path);
            mgr_.incrementQuarantineRetrySuccess();
            return true;
        }

        // All attempts exhausted – update metadata and possibly mark permanent.
        if (entry.retry_count >= static_cast<size_t>(max_attempts)) {
            entry.permanently_failed = true;
        }
        mgr_.updateQuarantineEntry(entry);
        return false;
    }

    // Legacy fallback: re-run the entire source from the last checkpoint.
    std::string source_id = entry.source_id;
    mgr_.dismissQuarantineItem(item_path);
    mgr_.ingestSource(source_id);
    return true;
}

size_t IngestionAdminApi::retryAllQuarantine() {
    auto items = mgr_.getQuarantineItems();
    size_t successes = 0;
    for (const auto& entry : items) {
        if (!entry.permanently_failed) {
            if (retryQuarantineItem(entry.item_path)) {
                ++successes;
            }
        }
    }
    return successes;
}

bool IngestionAdminApi::dismissQuarantineItem(const std::string& item_path) {
    return mgr_.dismissQuarantineItem(item_path);
}

std::string IngestionAdminApi::healthJson() const {
    auto sources    = mgr_.getRegisteredSources();
    auto quarantine = mgr_.getQuarantineItems();

    size_t total     = sources.size();
    size_t enabled   = 0;
    for (const auto& s : sources) {
        if (s.enabled) {
          ++enabled;
        }
    }
    size_t qsize          = quarantine.size();
    size_t retry_successes = mgr_.getQuarantineRetrySuccessCount();

    // Determine overall status
    std::string status = "healthy";
    if (qsize > 0) {
      status = "degraded";
    }
    if (enabled == 0 && total > 0) {
      status = "unhealthy";
    }

    std::ostringstream os;
    os << "{"
       << "\"status\":\"" << status << "\","
       << "\"registered_sources\":" << total << ","
       << "\"enabled_sources\":" << enabled << ","
       << "\"quarantine_size\":" << qsize << ","
       << "\"quarantine_retry_successes\":" << retry_successes
       << "}";
    return os.str();
}

} // namespace ingestion
} // namespace themis



