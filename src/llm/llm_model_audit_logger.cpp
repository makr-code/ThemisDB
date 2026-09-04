/**
 * @file llm_model_audit_logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=9, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/llm_model_audit_logger.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <ctime>
#include <utility>

namespace themis {
namespace llm {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/// Convert a LLMModelAuditEventType to a compact string label.
static std::string eventTypeName(LLMModelAuditEventType t) {
    switch (t) {
        case LLMModelAuditEventType::INFERENCE_STARTED:    return "INFERENCE_STARTED";
        case LLMModelAuditEventType::INFERENCE_COMPLETED:  return "INFERENCE_COMPLETED";
        case LLMModelAuditEventType::INFERENCE_FAILED:     return "INFERENCE_FAILED";
        case LLMModelAuditEventType::MODEL_LOADED:         return "MODEL_LOADED";
        case LLMModelAuditEventType::MODEL_UNLOADED:       return "MODEL_UNLOADED";
        case LLMModelAuditEventType::MODEL_SWITCHED:       return "MODEL_SWITCHED";
        case LLMModelAuditEventType::MODEL_REGISTERED:     return "MODEL_REGISTERED";
        case LLMModelAuditEventType::MODEL_UPDATED:        return "MODEL_UPDATED";
        case LLMModelAuditEventType::MODEL_DELETED:        return "MODEL_DELETED";
        case LLMModelAuditEventType::MODEL_IMPORTED:       return "MODEL_IMPORTED";
        case LLMModelAuditEventType::MODEL_EXPORTED:       return "MODEL_EXPORTED";
        case LLMModelAuditEventType::MODEL_QUANTIZED:      return "MODEL_QUANTIZED";
        case LLMModelAuditEventType::QUANTIZATION_FAILED:  return "QUANTIZATION_FAILED";
        case LLMModelAuditEventType::FINETUNING_STARTED:   return "FINETUNING_STARTED";
        case LLMModelAuditEventType::FINETUNING_COMPLETED: return "FINETUNING_COMPLETED";
        case LLMModelAuditEventType::FINETUNING_FAILED:    return "FINETUNING_FAILED";
        case LLMModelAuditEventType::MODEL_ENCRYPTED:      return "MODEL_ENCRYPTED";
        case LLMModelAuditEventType::MODEL_SIGNED:         return "MODEL_SIGNED";
        case LLMModelAuditEventType::SIGNATURE_VERIFIED:   return "SIGNATURE_VERIFIED";
        case LLMModelAuditEventType::SIGNATURE_FAILED:     return "SIGNATURE_FAILED";
        case LLMModelAuditEventType::CHECKSUM_MISMATCH:    return "CHECKSUM_MISMATCH";
        case LLMModelAuditEventType::MODEL_DEPLOYED:       return "MODEL_DEPLOYED";
        case LLMModelAuditEventType::MODEL_UNDEPLOYED:     return "MODEL_UNDEPLOYED";
        case LLMModelAuditEventType::DEPLOYMENT_FAILED:    return "DEPLOYMENT_FAILED";
        case LLMModelAuditEventType::PERFORMANCE_DEGRADED: return "PERFORMANCE_DEGRADED";
        case LLMModelAuditEventType::OOM_ERROR:            return "OOM_ERROR";
        case LLMModelAuditEventType::CACHE_HIT:            return "CACHE_HIT";
        case LLMModelAuditEventType::CACHE_MISS:           return "CACHE_MISS";
        case LLMModelAuditEventType::PROMPT_BLOCKED:       return "PROMPT_BLOCKED";
        case LLMModelAuditEventType::PROMPT_REDACTED:      return "PROMPT_REDACTED";
        default:                                           return "UNKNOWN";
    }
}

/// ISO-8601 UTC timestamp string for the current moment.
static std::string nowISO8601() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};
#ifdef _WIN32
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// ---------------------------------------------------------------------------
// Pimpl — production implementation
// ---------------------------------------------------------------------------

/** @brief Pimpl — production implementation. */
class LLMModelAuditLogger::Impl {
public:
    explicit Impl(utils::AuditLoggerConfig cfg) : config(std::move(cfg)) {
        if (!config.log_path.empty()) {
            // Ensure parent directory exists
            std::filesystem::path p(config.log_path);
            if (p.has_parent_path()) {
                std::error_code ec;
                std::filesystem::create_directories(p.parent_path(), ec);
                if (ec) {
                    spdlog::warn("LLMModelAuditLogger: could not create log directory '{}': {}",
                                 p.parent_path().string(), ec.message());
                }
            }
            ofs.open(config.log_path, std::ios::app);
            if (!ofs.is_open()) {
                spdlog::warn("LLMModelAuditLogger: could not open log file '{}' — "
                             "audit events will only be emitted via spdlog",
                             config.log_path);
            }
        }
    }

    /// Append a single JSON-lines record.  Thread-safe.
    ///
    /// Performance note: std::ofstream buffers writes internally so each
    /// call does NOT necessarily cause a syscall.  For very high event rates
    /// a background flush thread can be added later; the current approach
    /// keeps the implementation simple while being adequate for audit workloads.
    void writeLine(LLMModelAuditEventType event_type,
                   const std::string& model_id,
                   const json& details) {
        if (!enabled) {
          return;
        }

        const std::string ts = nowISO8601();
        json record = {
            {"timestamp_iso8601", ts},
            {"event_type",        eventTypeName(event_type)},
            {"model_id",          model_id},
            {"details",           details}
        };

        const std::string line = record.dump() + "\n";

        std::lock_guard<std::mutex> lk(mu);
        if (ofs.is_open()) {
            ofs << line;
        }
    }

    void flushFile() {
        std::lock_guard<std::mutex> lk(mu);
        if (ofs.is_open()) {
          ofs.flush();
        }
    }

    utils::AuditLoggerConfig config;
    bool enabled = true;

    // Internal in-memory event log (kept alongside file output for
    // queryLogs() and exportAnalytics() without re-parsing the file).
    struct Record {
        std::chrono::system_clock::time_point ts;
        LLMModelAuditEventType                event_type;
        std::string                           model_id;
        json                                  details;
    };
    std::vector<Record> records;

    std::mutex   mu;
    std::ofstream ofs;
};

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

LLMModelAuditLogger::LLMModelAuditLogger(const utils::AuditLoggerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {
    spdlog::debug("LLMModelAuditLogger initialized: log_path='{}'",
                  impl_->config.log_path);
}

LLMModelAuditLogger::~LLMModelAuditLogger() = default;

// ---------------------------------------------------------------------------
// Core write helper (shared by logInference / logEvent)
// ---------------------------------------------------------------------------

static void appendRecord(LLMModelAuditLogger::Impl* impl,
                         LLMModelAuditEventType event_type,
                         const std::string& model_id,
                         const json& details) {
    if (!impl->enabled) {
      return;
    }

    // 1. Persist to JSONL file
    impl->writeLine(event_type, model_id, details);

    // 2. Keep in-memory for queryLogs / exportAnalytics
    {
        std::lock_guard<std::mutex> lk(impl->mu);
        impl->records.push_back({
            std::chrono::system_clock::now(),
            event_type,
            model_id,
            details
        });
    }
}

// ---------------------------------------------------------------------------
// Public logging methods
// ---------------------------------------------------------------------------

void LLMModelAuditLogger::logInference(const LLMModelInferenceAudit& audit) {
    spdlog::debug("LLM inference audit model={} request={}",
                  audit.model_id, audit.request_id);

    appendRecord(impl_.get(),
                 audit.success
                     ? LLMModelAuditEventType::INFERENCE_COMPLETED
                     : LLMModelAuditEventType::INFERENCE_FAILED,
                 audit.model_id,
                 audit.toJSON());
}

void LLMModelAuditLogger::logEvent(
    LLMModelAuditEventType event_type,
    const std::string& model_id,
    const json& details) {
    spdlog::debug("LLM model event model={} event={}",
                  model_id, eventTypeName(event_type));

    appendRecord(impl_.get(), event_type, model_id, details);
}

void LLMModelAuditLogger::logModelLifecycle(
    LLMModelAuditEventType event_type,
    const std::string& model_id,
    const std::string& version,
    const json& metadata) {
    spdlog::debug("LLM model lifecycle model={} version={}", model_id, version);

    json details = metadata;
    details["version"] = version;
    appendRecord(impl_.get(), event_type, model_id, details);
}

void LLMModelAuditLogger::logFineTuning(
    LLMModelAuditEventType event_type,
    const std::string& model_id,
    const std::string& base_model_id,
    int num_samples,
    float final_loss,
    const json& hyperparameters) {
    spdlog::debug("LLM fine-tuning model={} base={} samples={} loss={}",
                  model_id, base_model_id, num_samples, final_loss);

    json details = hyperparameters;
    details["base_model_id"] = base_model_id;
    details["num_samples"]   = num_samples;
    details["final_loss"]    = final_loss;
    appendRecord(impl_.get(), event_type, model_id, details);
}

void LLMModelAuditLogger::logDeployment(
    LLMModelAuditEventType event_type,
    const std::string& model_id,
    const std::string& deployment_target,
    const json& config) {
    spdlog::debug("LLM deployment model={} target={}", model_id, deployment_target);

    json details = config;
    details["deployment_target"] = deployment_target;
    appendRecord(impl_.get(), event_type, model_id, details);
}

void LLMModelAuditLogger::logPolicyViolation(
    const std::string& model_id,
    const std::string& request_id,
    const std::string& rule_name,
    const std::string& reason,
    bool was_blocked) {
    const auto event_type = was_blocked
        ? LLMModelAuditEventType::PROMPT_BLOCKED
        : LLMModelAuditEventType::PROMPT_REDACTED;

    json details = {
        {"request_id",  request_id},
        {"rule_name",   rule_name},
        {"reason",      reason},
        {"was_blocked", was_blocked}
    };

    appendRecord(impl_.get(), event_type, model_id, details);

    spdlog::info("PromptPolicy audit: model={} request={} rule='{}' blocked={}",
                 model_id, request_id, rule_name, was_blocked);
}

// ---------------------------------------------------------------------------
// Query / Export
// ---------------------------------------------------------------------------

std::vector<json> LLMModelAuditLogger::queryLogs(
    const std::string& model_id,
    std::optional<std::chrono::system_clock::time_point> start_time,
    std::optional<std::chrono::system_clock::time_point> end_time) {

    std::lock_guard<std::mutex> lk(impl_->mu);
    std::vector<json> result;
    for (const auto& r : impl_->records) {
        if (!model_id.empty() && r.model_id != model_id) {
          continue;
        }
        if (start_time && r.ts < *start_time) {
          continue;
        }
        if (end_time   && r.ts >= *end_time) {
          continue;
        }

        json entry = r.details;
        entry["event_type"]       = eventTypeName(r.event_type);
        entry["model_id"]         = r.model_id;
        entry["timestamp_unix"]   = std::chrono::system_clock::to_time_t(r.ts);
        result.push_back(std::move(entry));
    }
    return result;
}

std::vector<LLMModelInferenceAudit> LLMModelAuditLogger::getInferenceHistory(
    const std::string& model_id,
    int limit) {
    spdlog::debug("LLM getInferenceHistory model={} limit={}", model_id, limit);
    // NOTE: This method intentionally returns an empty vector.
    // The in-memory record store holds events as generic JSON (via the Impl::Record
    // struct) to avoid duplicating typed struct state.  For inference history
    // queries use queryLogs() which returns the full JSON payload per event.
    // A future improvement is to deserialise Record::details back into
    // LLMModelInferenceAudit when event_type == INFERENCE_COMPLETED/FAILED.
    return {};
}

size_t LLMModelAuditLogger::exportAnalytics(
    std::ostream& out_stream,
    const std::string& model_id,
    std::optional<std::chrono::system_clock::time_point> start_time,
    std::optional<std::chrono::system_clock::time_point> end_time) {

    std::lock_guard<std::mutex> lk(impl_->mu);
    size_t count = 0;

    for (const auto& r : impl_->records) {
        if (!model_id.empty() && r.model_id != model_id) {
          continue;
        }
        if (start_time && r.ts < *start_time) {
          continue;
        }
        if (end_time   && r.ts >= *end_time) {
          continue;
        }

        // Build a canonical JSONL record
        auto t = std::chrono::system_clock::to_time_t(r.ts);
        std::tm tm_utc{};
#ifdef _WIN32
        gmtime_s(&tm_utc, &t);
#else
        gmtime_r(&t, &tm_utc);
#endif
        std::ostringstream ts_ss;
        ts_ss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");

        json line_obj = {
            {"timestamp_iso8601", ts_ss.str()},
            {"event_type",        eventTypeName(r.event_type)},
            {"model_id",          r.model_id},
            {"details",           r.details}
        };

        out_stream << line_obj.dump() << "\n";
        ++count;
    }

    return count;
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

void LLMModelAuditLogger::setEnabled([[maybe_unused]] bool enabled) {
    std::lock_guard<std::mutex> lk(impl_->mu);
    impl_->enabled = enabled;
}

void LLMModelAuditLogger::flush() {
    impl_->flushFile();
}

json LLMModelAuditLogger::getModelStats(const std::string& model_id) {
    std::lock_guard<std::mutex> lk(impl_->mu);
    size_t total = 0, inferences = 0, failures = 0, policy_blocks = 0;
    for (const auto& r : impl_->records) {
        if (!model_id.empty() && r.model_id != model_id) {
          continue;
        }
        ++total;
        if (r.event_type == LLMModelAuditEventType::INFERENCE_COMPLETED) {
          ++inferences;
        }
        if (r.event_type == LLMModelAuditEventType::INFERENCE_FAILED) {
          ++failures;
        }
        if (r.event_type == LLMModelAuditEventType::PROMPT_BLOCKED) {
          ++policy_blocks;
        }
    }
    return json{
        {"model_id",      model_id},
        {"total_events",  total},
        {"inferences",    inferences},
        {"failures",      failures},
        {"policy_blocks", policy_blocks}
    };
}

} // namespace llm
} // namespace themis
