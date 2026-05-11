/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            decision_record_yaml_processor.h                   ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-17                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     230                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include &lt;map&gt;
#include &lt;optional&gt;
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <filesystem>
#include <chrono>
#include <cstdint>

namespace themis {
namespace llm {

/**
 * @brief Lightweight decision record for LLM / LoRA runtime decisions.
 *
 * DecisionRecord captures the *what*, *why*, and *outcome* of every autonomous
 * decision made by the LLM/LoRA stack at runtime.  It is intentionally simpler
 * than the compliance-focused AIDecisionAudit: no RocksDB dependency, no PKI
 * signing — just a plain YAML file on disk, written by an independent async
 * thread.
 *
 * All fields that are optional are only emitted in the YAML output when set,
 * keeping the files concise for high-frequency decisions (e.g. FEDERATED_ROUND).
 */
struct DecisionRecord {
    // ─── Identity ──────────────────────────────────────────────────────────────
    /// Unique identifier for this record (set automatically if empty on submit).
    std::string record_id;

    /// Semantic decision type.  Well-known values:
    ///   "FEDERATED_ROUND" | "LORA_ADAPTER_SELECTION" | "THRESHOLD_UPDATE"
    ///   "LORA_RANK_ADJUSTMENT" | "LOOP_TRIGGER" | "LAYER_SELECTION"
    ///   "OR_ADAPTIVE_THRESHOLD_CHANGE" | "CIRCUIT_BREAKER_OPEN"
    ///   "CIRCUIT_BREAKER_CLOSED" | "BACKPRESSURE_DROP" | "GDPR_ERASE"
    std::string decision_type;

    // ─── Source ────────────────────────────────────────────────────────────────
    /// Component that produced the decision (class name, e.g. "LoRAFederationCoordinator").
    std::string component;

    /// Optional: shard that produced the decision.
    std::optional<std::string> shard_id;

    // ─── Timing ────────────────────────────────────────────────────────────────
    /// When the decision was made.  Set automatically to now() if default-constructed.
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};

    /// How long the decision took (ms).  0 if not measured.
    int64_t latency_ms{0};

    // ─── Outcome ───────────────────────────────────────────────────────────────
    /// Human-readable outcome.  Well-known values:
    ///   "SUCCESS" | "SKIPPED_BUDGET" | "TIMEOUT" | "ERROR" | "DROPPED"
    std::string outcome;

    /// Confidence score [0.0, 1.0] — present only when meaningful.
    std::optional<float> confidence;

    // ─── LoRA-specific ─────────────────────────────────────────────────────────
    /// Federation round number (Layer 11B).
    std::optional&lt;int&gt; lora_round;

    /// Differential privacy ε consumed in this round.
    std::optional<float> epsilon_spent;

    /// Number of shards that participated.
    std::optional<size_t> participants;

    /// Model accuracy delta after applying the global adapter delta.
    std::optional<float> accuracy_delta;

    // ─── Free-form parameters ──────────────────────────────────────────────────
    /// Key-value pairs capturing algorithm / config values used for this decision.
    /// Emitted as a YAML mapping block.
    std::map<std::string, std::string> parameters;

    // ─── Audit trail reference ─────────────────────────────────────────────────
    /// Optional: links back to a full AIDecisionAudit entry (decision_id).
    std::optional<std::string> audit_ref;
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Async YAML writer for DecisionRecord objects.
 *
 * DecisionRecordYamlProcessor runs as a **single background thread** completely
 * independent of the core LLM / LoRA execution path.  Callers submit records via
 * a lock-free non-blocking submit() call; the background thread drains the queue
 * and writes one YAML file per record.
 *
 * File layout:
 * @code
 *   logs/decisions/
 *   └── YYYY-MM-DD/
 *       ├── <timestamp>_<decision_type>_<record_id>.yaml
 *       └── ...
 * @endcode
 *
 * Backpressure: when the queue reaches max_queue_depth, submit() drops the
 * incoming record and increments the dropped counter (non-blocking, fire-and-
 * forget semantics).  The caller can inspect getStats() to detect sustained
 * overload.
 *
 * Thread safety: all public methods are thread-safe.
 *
 * Lifecycle:
 * - The background thread is started in the constructor.
 * - The destructor signals stop, then waits for the queue to drain before
 *   joining the thread (graceful shutdown).
 *
 * Usage:
 * @code
 *   DecisionRecordYamlProcessor::Config cfg;
 *   cfg.log_dir = "logs/decisions";
 *   DecisionRecordYamlProcessor proc(cfg);
 *
 *   DecisionRecord r;
 *   r.decision_type = "FEDERATED_ROUND";
 *   r.component     = "LoRAFederationCoordinator";
 *   r.outcome       = "SUCCESS";
 *   r.lora_round    = 42;
 *   proc.submit(r);   // non-blocking
 * @endcode
 */
class DecisionRecordYamlProcessor {
public:
    struct Config {
        /// Directory where YAML files are written.
        std::filesystem::path log_dir{"logs/decisions"};

        /// Maximum number of records that may sit in the queue.
        /// submit() drops new records silently when this limit is reached.
        size_t max_queue_depth{10'000};

        /// When true, one subdirectory per calendar day is created automatically.
        bool create_daily_subdirs{true};
    };

    /**
     * @brief Construct with default configuration.
     */
    DecisionRecordYamlProcessor();

    /**
     * @brief Construct the processor and start the background thread.
     * @param config  Configuration.  log_dir is created on first write.
     *
     * @note  The default-argument form `= {}` triggers GCC DR1607 when Config
     *        has default member initialisers of non-trivially-constructible
     *        types.  We therefore provide separate default + config overloads.
     */
    explicit DecisionRecordYamlProcessor(Config config);

    /**
     * @brief Destructor — drains the queue, then stops and joins the background thread.
     */
    ~DecisionRecordYamlProcessor();

    // Non-copyable, non-movable (owns a thread).
    DecisionRecordYamlProcessor(const DecisionRecordYamlProcessor&) = delete;
    DecisionRecordYamlProcessor& operator=(const DecisionRecordYamlProcessor&) = delete;
    DecisionRecordYamlProcessor(DecisionRecordYamlProcessor&&) = delete;
    DecisionRecordYamlProcessor& operator=(DecisionRecordYamlProcessor&&) = delete;

    /**
     * @brief Submit a decision record for async YAML serialisation.
     *
     * This method is **non-blocking** and **thread-safe**.  The record is enqueued
     * and written by the background thread.  If the queue is full the record is
     * silently dropped and false is returned.
     *
     * @param record  The record to submit.  record_id is generated if empty.
     * @return true if the record was enqueued; false if it was dropped.
     */
    bool submit(DecisionRecord record);

    /**
     * @brief Block until the queue is empty (for testing / graceful shutdown).
     */
    void flush();

    // ─── Statistics ────────────────────────────────────────────────────────────
    struct Stats {
        size_t submitted{0};  ///< Total records handed to submit().
        size_t written{0};    ///< Records successfully written to disk.
        size_t dropped{0};    ///< Records dropped due to full queue.
        size_t errors{0};     ///< Records that failed to write (I/O error).
    };

    /**
     * @brief Return a snapshot of processor statistics.  Thread-safe.
     */
    Stats getStats() const noexcept;

private:
    void processorThread();
    void writeRecord(const DecisionRecord& record);
    std::filesystem::path recordPath(const DecisionRecord& record) const;
    std::string generateId() const;
    std::string formatTimestamp(std::chrono::system_clock::time_point tp) const;
    std::string toYaml(const DecisionRecord& record) const;

    Config config_;

    std::queue<DecisionRecord> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    std::thread thread_;

    std::atomic<size_t> submitted_{0};
    std::atomic<size_t> written_{0};
    std::atomic<size_t> dropped_{0};
    std::atomic<size_t> errors_{0};
};

} // namespace llm
} // namespace themis
