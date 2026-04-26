/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            database_domain_auto_labeler.h                     ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-17                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file database_domain_auto_labeler.h
 * @brief Auto-labeler for database-domain training samples (IMPL-A1).
 *
 * Implements `DatabaseDomainAutoLabeler` which labels (query, plan, Δlatency)
 * triples with `DomainType::DATABASE_OPTIMIZER` and a sigmoid-based confidence
 * score derived from the measured query latency improvement.
 *
 * Three input sources are supported:
 *   - BaoOptimizer decision logs  (`labelFromBaoDecision`)
 *   - DBA feedback entries        (`labelFromDBAFeedback`)
 *   - Bulk log-file import        (`labelFromLogFile`)
 *
 * All public methods are const; the labeler is stateless apart from the
 * configurable `sensitivity_ms` parameter.
 *
 * ### Paper reference
 * THEMISDB_LORA_RESEARCH_PAPER.md §4 — Dataset Construction (Phase 1).
 *
 * ### Confidence formula
 * @code
 *   confidence = sigmoid(|delta_p99_ms| / sensitivity_ms)
 * @endcode
 * where `sigmoid(x) = 1 / (1 + exp(-x))`.
 * Special case: `computeConfidence(0.0)` returns exactly 0.5 (sigmoid(0)).
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "training/auto_labeler.h"   // DomainType, TrainingSample

namespace themis::training {

/**
 * @brief Feedback entry from a DBA or system operator.
 *
 * Mirrors the shape of entries produced by `FeedbackCollector` so that
 * `labelFromDBAFeedback()` can process them without depending on the full
 * feedback-collector header.
 */
struct FeedbackEntry {
    std::string query_text;     ///< Original SQL query
    std::string plan_json;      ///< EXPLAIN output (JSON)
    bool        is_positive;    ///< true = accepted recommendation
    double      delta_p99_ms;   ///< Latency delta observed (negative = faster)
    std::string source_id;      ///< Feedback origin identifier
};

/**
 * @brief A labeled training sample for the database-optimizer domain.
 */
struct LabeledDbSample {
    std::string query_text;     ///< SQL query text
    std::string plan_json;      ///< Query execution plan (JSON)
    DomainType  label;          ///< Domain label (always DATABASE_OPTIMIZER for this labeler)
    double      confidence;     ///< [0.0, 1.0]
    std::string source;         ///< "bao_log" | "dba_feedback" | "synthetic"
    double      delta_p99_ms;   ///< Raw latency delta that produced this sample
};

/**
 * @brief Auto-labeler for database-domain LoRA training samples.
 *
 * Stateless helper class (all methods are `const`).  Instantiation is
 * cheap; the same instance may be shared across threads.
 *
 * Example:
 * @code
 * DatabaseDomainAutoLabeler labeler;
 * auto sample = labeler.labelFromBaoDecision(query, plan_json, -15.0);
 * // sample.confidence ≈ 0.82, sample.label == DomainType::DATABASE_OPTIMIZER
 * @endcode
 */
class DatabaseDomainAutoLabeler {
public:
    /**
     * @brief Construct with optional sensitivity parameter.
     * @param sensitivity_ms  Latency delta (ms) that maps to sigmoid inflection
     *                        point (confidence = 0.73).  Default: 10.0 ms.
     */
    explicit DatabaseDomainAutoLabeler(double sensitivity_ms = 10.0);

    // ── Primary labeling APIs ─────────────────────────────────────────────────

    /**
     * @brief Create a labeled sample from a BaoOptimizer decision log entry.
     *
     * @param query        SQL query string.
     * @param bao_plan_json  JSON-serialized BaoOptimizer plan.
     * @param delta_p99_ms Latency improvement: negative = query got faster.
     * @return LabeledDbSample with `source = "bao_log"`.
     */
    LabeledDbSample labelFromBaoDecision(
        const std::string& query,
        const std::string& bao_plan_json,
        double             delta_p99_ms) const;

    /**
     * @brief Create a labeled sample from a DBA feedback entry.
     *
     * Negative (is_positive=false) feedback receives confidence ≥ 0.9,
     * reflecting high signal strength.
     *
     * @param entry  FeedbackEntry from the DBA or system operator.
     * @return LabeledDbSample with `source = "dba_feedback"`.
     */
    LabeledDbSample labelFromDBAFeedback(const FeedbackEntry& entry) const;

    /**
     * @brief Batch-label samples from a query log file.
     *
     * Each line in the file must be a JSON object with at least the fields
     * `"query"`, `"plan"`, and `"delta_p99_ms"`.  Lines that cannot be
     * parsed are silently skipped and counted in the returned stats.
     *
     * @param log_path       Path to the query log file.
     * @param max_samples    Maximum number of samples to return (0 = unlimited).
     * @param min_confidence Filter: samples below this threshold are discarded.
     * @return Vector of labeled samples (may be empty if file is missing / empty).
     */
    std::vector<LabeledDbSample> labelFromLogFile(
        const std::string& log_path,
        size_t             max_samples    = 50000,
        double             min_confidence = 0.0) const;

    // ── Confidence computation ────────────────────────────────────────────────

    /**
     * @brief Map a latency delta to a confidence score in [0.0, 1.0].
     *
     * confidence = sigmoid(|delta_p99_ms| / sensitivity_ms)
     *
     * @param delta_p99_ms  Measured latency improvement (sign is ignored).
     * @return Confidence in [0.5, 1.0).  Returns exactly 0.5 when delta == 0.
     */
    [[nodiscard]] double computeConfidence(double delta_p99_ms) const;

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] double sensitivityMs() const { return sensitivity_ms_; }

    // ── JSONL export ──────────────────────────────────────────────────────────

    /**
     * @brief Serialize a batch of labeled samples to a JSONL string.
     *
     * Each sample is emitted as one compact JSON object on its own line:
     * @code
     * {"query":"SELECT …","explain_plan":"…","latency_delta_ms":-42.5}
     * @endcode
     *
     * The output is suitable for appending to a `.jsonl` log file or piping
     * to a downstream ingestion tool.
     *
     * @param samples  Samples to serialize (may be empty).
     * @return JSONL string; empty string when @p samples is empty.
     */
    [[nodiscard]] static std::string exportToJsonl(
        const std::vector<LabeledDbSample>& samples);

private:
    double sensitivity_ms_;

    /// Internal: build a LabeledDbSample from raw components.
    LabeledDbSample buildSample(
        const std::string& query,
        const std::string& plan_json,
        double             delta_p99_ms,
        const std::string& source) const;
};

} // namespace themis::training
