/**
 * @file decision_record_yaml_processor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/decision_record_yaml_processor.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

namespace themis {
namespace llm {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

DecisionRecordYamlProcessor::DecisionRecordYamlProcessor()
    : DecisionRecordYamlProcessor(Config{}) {}

DecisionRecordYamlProcessor::DecisionRecordYamlProcessor(Config config)
    : config_(std::move(config))
{
    thread_ = std::thread(&DecisionRecordYamlProcessor::processorThread, this);
    THEMIS_INFO("DecisionRecordYamlProcessor started (log_dir='{}')",
                config_.log_dir.string());
}

DecisionRecordYamlProcessor::~DecisionRecordYamlProcessor() {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        stop_ = true;
    }
    cv_.notify_one();
    if (thread_.joinable()) {
        // thread_join_no_timeout (W4): bounded join via joinThreadWithin
        if (!themis::utils::joinThreadWithin(thread_)) {
            THEMIS_WARN("[DecisionRecordYamlProcessor] thread did not finish within shutdown deadline; detaching.");
        }
    }
    THEMIS_INFO("DecisionRecordYamlProcessor stopped "
                "(written={}, dropped={}, errors={})",
                written_.load(std::memory_order_acquire), dropped_.load(std::memory_order_acquire), errors_.load(std::memory_order_acquire));
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

bool DecisionRecordYamlProcessor::submit(DecisionRecord record) {
    if (record.record_id.empty()) {
        record.record_id = generateId();
    }

    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (static_cast<int>(queue_.size()) >= config_.max_queue_depth) {
            ++dropped_;
            return false;
        }
        ++submitted_;
        queue_.push(std::move(record));
    }
    cv_.notify_one();
    return true;
}

bool DecisionRecordYamlProcessor::flush() {
    auto pred = [this] {
        const size_t submitted = submitted_.load(std::memory_order_relaxed);
        const size_t finalized = written_.load(std::memory_order_relaxed)
                               + errors_.load(std::memory_order_relaxed)
                               + dropped_.load(std::memory_order_relaxed);
        return queue_.empty() && in_flight_ == 0 && finalized >= submitted;
    };

    // B2-blocking_no_timeout: cap the legacy no-timeout path to prevent indefinite deadlock.
    static constexpr std::chrono::seconds kMaxFlushTimeout{30};

    std::unique_lock<std::mutex> lk(mutex_);
    if (config_.flush_timeout_ms.count() == 0) {
        // Legacy: flush_timeout_ms == 0 previously waited indefinitely.  Cap to kMaxFlushTimeout
        // to prevent deadlock when a background thread stalls or is never started.
        if (!cv_.wait_for(lk, kMaxFlushTimeout, pred)) {
            spdlog::warn("DecisionRecordYamlProcessor::flush() timed out after {} s "
                         "(legacy no-timeout config). Some records may not be flushed.",
                         kMaxFlushTimeout.count());
            return false;
        }
        return true;
    }
    return cv_.wait_for(lk, config_.flush_timeout_ms, pred);
}

DecisionRecordYamlProcessor::Stats DecisionRecordYamlProcessor::getStats() const noexcept {
    Stats s;
    s.submitted = submitted_.load(std::memory_order_relaxed);
    s.written   = written_.load(std::memory_order_relaxed);
    s.dropped   = dropped_.load(std::memory_order_relaxed);
    s.errors    = errors_.load(std::memory_order_relaxed);
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// Background thread
// ─────────────────────────────────────────────────────────────────────────────

void DecisionRecordYamlProcessor::processorThread() {
    while (true) {
        DecisionRecord rec;

        {
            std::unique_lock<std::mutex> lk(mutex_);
            cv_.wait(lk, [this] { return !queue_.empty() || stop_.load(std::memory_order_acquire); });

            if (queue_.empty()) {
                // stop_ is true and queue is drained — exit.
                cv_.notify_all(); // wake any flush() waiters
                break;
            }

            rec = std::move(queue_.front());
            queue_.pop();
            ++in_flight_;
        }

        writeRecord(rec);

        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (in_flight_ > 0) {
                --in_flight_;
            }
            if (queue_.empty() && in_flight_ == 0) {
                cv_.notify_all();
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// YAML serialisation
// ─────────────────────────────────────────────────────────────────────────────

std::string DecisionRecordYamlProcessor::toYaml(const DecisionRecord& r) const {
    YAML::Emitter out;
    out << YAML::Comment("ThemisDB Decision Record — generated by DecisionRecordYamlProcessor");
    out << YAML::BeginDoc;
    out << YAML::BeginMap;

    // Identity
    out << YAML::Key << "record_id"     << YAML::Value << r.record_id;
    out << YAML::Key << "decision_type" << YAML::Value << r.decision_type;
    out << YAML::Key << "component"     << YAML::Value << r.component;
    if (r.shard_id) {
        out << YAML::Key << "shard_id" << YAML::Value << *r.shard_id;
    }

    // Timing
    out << YAML::Key << "timestamp"  << YAML::Value << formatTimestamp(r.timestamp);
    out << YAML::Key << "latency_ms" << YAML::Value << r.latency_ms;

    // Outcome
    out << YAML::Key << "outcome" << YAML::Value << r.outcome;
    if (r.confidence) {
        out << YAML::Key << "confidence" << YAML::Value << *r.confidence;
    }

    // LoRA-specific (emitted only when present)
    if (r.lora_round || r.epsilon_spent || r.participants || r.accuracy_delta) {
        out << YAML::Key << "lora" << YAML::Value << YAML::BeginMap;
        if (r.lora_round)     { out << YAML::Key << "round"          << YAML::Value << *r.lora_round; }
        if (r.epsilon_spent)  { out << YAML::Key << "epsilon_spent"  << YAML::Value << *r.epsilon_spent; }
        if (r.participants)   { out << YAML::Key << "participants"   << YAML::Value << static_cast<int>(*r.participants); }
        if (r.accuracy_delta) { out << YAML::Key << "accuracy_delta" << YAML::Value << *r.accuracy_delta; }
        out << YAML::EndMap;
    }

    // Free-form parameters
    if (!r.parameters.empty()) {
        out << YAML::Key << "parameters" << YAML::Value << YAML::BeginMap;
        for (const auto& [k, v] : r.parameters) {
            out << YAML::Key << k << YAML::Value << v;
        }
        out << YAML::EndMap;
    }

    // Audit reference
    if (r.audit_ref) {
        out << YAML::Key << "audit_ref" << YAML::Value << *r.audit_ref;
    }

    out << YAML::EndMap;
    out << YAML::EndDoc;

    return std::string(out.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// File I/O
// ─────────────────────────────────────────────────────────────────────────────

std::filesystem::path DecisionRecordYamlProcessor::recordPath(
    const DecisionRecord& r) const
{
    // Build directory: log_dir / YYYY-MM-DD  (or just log_dir)
    std::filesystem::path dir = config_.log_dir;
    if (config_.create_daily_subdirs) {
        // Extract date from timestamp
        auto tt = std::chrono::system_clock::to_time_t(r.timestamp);
        std::tm tm_utc{};
#if defined(_WIN32)
        gmtime_s(&tm_utc, &tt);
#else
        gmtime_r(&tt, &tm_utc);
#endif
        char date_buf[11]; // "YYYY-MM-DD\0"
        std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tm_utc);
        dir /= date_buf;
    }

    // Filename: <ISO-timestamp>_<decision_type>_<record_id>.yaml
    // Sanitise decision_type for use in a filename.
    std::string safe_type = r.decision_type;
    for (char& c : safe_type) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<'  || c == '>' || c == '|' || c == ' ') {
            c = '_';
        }
    }

    std::string fname = formatTimestamp(r.timestamp) + "_" + safe_type
                        + "_" + r.record_id + ".yaml";
    // Replace colons from ISO timestamp in filename (Windows compatibility)
    for (char& c : fname) {
        if (c == ':') {
          c = '-';
        }
    }

    return dir / fname;
}

void DecisionRecordYamlProcessor::writeRecord(const DecisionRecord& r) {
    try {
        auto path = recordPath(r);

        // Prevent accidental overwrite when multiple records resolve to the
        // same base filename (e.g. identical record_id/timestamp in tests).
        if (std::filesystem::exists(path)) {
            const auto parent = path.parent_path();
            const auto stem = path.stem().string();
            const auto ext = path.extension().string();

            for (size_t suffix = 1; suffix <= 10'000; ++suffix) {
                const auto candidate = parent / (stem + "_" + std::to_string(suffix) + ext);
                if (!std::filesystem::exists(candidate)) {
                    path = candidate;
                    break;
                }
            }
        }

        // Create parent directory on demand (including daily subdir)
        std::error_code ec = {};
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec) {
            THEMIS_WARN("DecisionRecordYamlProcessor: cannot create directory '{}': {}",
                        path.parent_path().string(), ec.message());
            ++errors_;
            return;
        }

        std::ofstream ofs(path, std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) {
            THEMIS_WARN("DecisionRecordYamlProcessor: cannot open '{}' for writing",
                        path.string());
            ++errors_;
            return;
        }

        ofs << toYaml(r) << '\n';
        ofs.close();

        if (ofs.fail()) {
            THEMIS_WARN("DecisionRecordYamlProcessor: write failed for '{}'",
                        path.string());
            ++errors_;
            return;
        }

        ++written_;

    } catch (const std::exception& e) {
        THEMIS_WARN("DecisionRecordYamlProcessor: unexpected exception: {}", e.what());
        ++errors_;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string DecisionRecordYamlProcessor::generateId() const {
    // "dr-<unix_ms>-<6-hex-random>"
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFF);

    std::ostringstream oss = {};
    oss << "dr-" << now_ms << "-"
        << std::hex << std::setw(6) << std::setfill('0') << dist(rng);
    return oss.str();
}

std::string DecisionRecordYamlProcessor::formatTimestamp(
    std::chrono::system_clock::time_point tp) const
{
    auto tt = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()).count() % 1000;

    std::tm tm_utc{};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &tt);
#else
    gmtime_r(&tt, &tm_utc);
#endif

    std::ostringstream oss = {};
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms << 'Z';
    return oss.str();
}

} // namespace llm
} // namespace themis

