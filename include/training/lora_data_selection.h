/**
 * @file lora_data_selection.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>

namespace themis {
namespace training {

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief Per-domain keyword list for BM25 domain relevance scoring.
 */
using DomainKeywords = std::map<std::string, std::vector<std::string>>;

/**
 * @brief Complete configuration for the automated LoRA data selection pipeline.
 *
 * Matches the YAML schema:
 * @code
 * lora_data_selection:
 *   min_length_tokens: 50
 *   max_length_tokens: 10000
 *   required_language: "de"
 *   max_toxicity_score: 0.3
 *   minhash_threshold: 0.95
 *   ...
 * @endcode
 */
struct LoRADataSelectionConfig {
    // ---- Stage 1: Quality filtering ----
    size_t min_length_tokens   = 50;          ///< Minimum token count per sample
    size_t max_length_tokens   = 10000;       ///< Maximum token count per sample
    std::string required_language = "de";     ///< ISO 639-1 language code
    double max_toxicity_score  = 0.3;         ///< Max allowed toxicity (0..1)
    bool   enable_pii_check    = true;        ///< Strip/reject PII samples

    // ---- Stage 2: Deduplication ----
    double minhash_threshold   = 0.95;        ///< Jaccard threshold for near-duplicate removal
    size_t minhash_num_perm    = 128;         ///< Number of MinHash permutations

    // ---- Stage 3: Embedding & clustering ----
    std::string embedding_model = "multilingual-e5-large";
    size_t clustering_k_ratio   = 50;         ///< k = target_samples / clustering_k_ratio

    // ---- Stage 4: Quality & difficulty scoring ----
    std::string perplexity_model    = "gpt2";
    double perplexity_weight        = 0.4;    ///< Weight in combined quality score
    double diversity_weight         = 0.3;    ///< Weight for token-type ratio (TTR)
    double domain_relevance_weight  = 0.3;    ///< Weight for BM25 domain relevance
    DomainKeywords domain_keywords;           ///< Domain → keyword list

    // ---- Stage 5: Curriculum stratified sampling ----
    double easy_ratio   = 0.1;    ///< Fraction of easy samples (low difficulty)
    double medium_ratio = 0.7;    ///< Fraction of medium samples
    double hard_ratio   = 0.2;    ///< Fraction of hard samples (high difficulty)
    size_t target_samples = 5000; ///< Total samples to select

    // ---- Audit ----
    bool        audit          = true;   ///< Record provenance audit entry
    /// Path to append JSON Lines audit records.  Empty = no file persistence.
    std::string audit_log_path = "logs/lora_data_selection_audit.jsonl";

    LoRADataSelectionConfig() = default;

    /**
     * @brief Load configuration from a YAML file.
     *
     * Reads the @p section block (default: `lora_data_selection`) from the
     * YAML file at @p path and fills a new config object.
     *
     * Uses a built-in line-by-line parser – no external yaml-cpp dependency.
     * Supports live-reload: call again at any time to obtain an updated config.
     *
     * @param path    Path to the YAML configuration file.
     * @param section Top-level YAML key containing the data-selection block.
     * @throws std::runtime_error if the file cannot be opened.
     */
    static LoRADataSelectionConfig loadFromYAML(
        const std::string& path,
        const std::string& section = "lora_data_selection");

    /**
     * @brief Parse configuration from an in-memory YAML string.
     *
     * Useful for unit testing or when the YAML content is already loaded.
     *
     * @param yaml_text YAML text containing the data-selection section.
     * @param section   Top-level key to read (default: `lora_data_selection`).
     */
    static LoRADataSelectionConfig fromYAMLString(
        const std::string& yaml_text,
        const std::string& section = "lora_data_selection");
};

// ============================================================================
// Data sample representation
// ============================================================================

/**
 * @brief Single training sample as used by the data selection pipeline.
 */
struct DataSample {
    std::string id;       ///< Unique document / sample identifier
    std::string text;     ///< Full text content (input + output concatenated or separate)
    std::string language; ///< Detected language (ISO 639-1)
    /// Optional domain tag (e.g. "legal", "medical", "tech").
    /// When set, BM25 domain relevance scoring uses only the keywords for
    /// this domain instead of aggregating across all domains.
    std::string domain;

    // Computed scores (filled during pipeline stages)
    double quality_score    = 0.0;  ///< Combined quality score [0..1]
    double difficulty_score = 0.0;  ///< Difficulty estimate [0..1]
    bool   is_duplicate     = false;

    DataSample() = default;
    DataSample(std::string id_, std::string text_)
        : id(std::move(id_)), text(std::move(text_)) {}
};

// ============================================================================
// Audit trail
// ============================================================================

/**
 * @brief Provenance record written when audit=true.
 *
 * Records which samples were selected, when, by which pipeline version,
 * and the key configuration parameters used.
 */
struct SelectionAuditEntry {
    std::string pipeline_version = "1.0";
    std::chrono::system_clock::time_point timestamp;
    std::string config_hash;          ///< SHA-256 / FNV hash of serialized config
    size_t input_sample_count  = 0;
    size_t output_sample_count = 0;
    size_t filtered_by_quality = 0;
    size_t filtered_by_dedup   = 0;
    size_t filtered_by_cluster = 0;
    std::vector<std::string> selected_ids; ///< IDs of selected samples
    /// Number of selected samples per declared domain.
    /// Empty when no samples carry a domain tag.
    std::map<std::string, size_t> domain_distribution;

    SelectionAuditEntry() : timestamp(std::chrono::system_clock::now()) {}

    /**
     * @brief Serialize to a single JSON Lines (JSONL) string.
     *
     * Produces one compact JSON object per call, suitable for appending to a
     * `.jsonl` file.  Uses a self-contained serializer – no external JSON
     * library dependency.
     */
    std::string toJSONL() const;
};

// ============================================================================
// Pipeline result
// ============================================================================

/**
 * @brief Result produced by DataSelectionPipeline::run().
 */
struct DataSelectionResult {
    bool success = false;
    std::vector<DataSample> selected_samples;
    SelectionAuditEntry     audit_entry;
    std::string             error_message;
    double elapsed_seconds  = 0.0;

    DataSelectionResult() = default;
};

// ============================================================================
// Progress callback
// ============================================================================

/**
 * @brief Called after each pipeline stage with progress information.
 * @param stage   Stage name (e.g. "quality_filter", "deduplication", …)
 * @param count   Number of samples passing this stage so far
 * @param message Human-readable status message
 */
using SelectionProgressCallback =
    std::function<void(const std::string& stage,
                       size_t count,
                       const std::string& message)>;

// ============================================================================
// Monitoring metrics snapshot
// (Defined here so DataSelectionPipeline::computeMetrics can reference it)
// ============================================================================

/**
 * @brief Runtime monitoring metrics used by the self-improvement module
 *        to decide whether adaptive thresholds should be adjusted.
 */
struct DataSelectionMetrics {
    double avg_quality_score     = 0.0;
    double avg_difficulty_score  = 0.0;
    /// Average type-token ratio (TTR) of the selected sample set.
    /// Range [0, 1]: higher values indicate greater vocabulary diversity.
    /// Computed by `DataSelectionPipeline::computeMetrics()` using
    /// normalised (lowercased, punctuation-stripped) tokens.
    double diversity_score       = 0.0;
    double filter_rejection_rate = 0.0;   ///< Fraction of candidates rejected by Stage 1
    double dedup_removal_rate    = 0.0;   ///< Fraction removed by Stage 2
    double training_accuracy     = 0.0;   ///< Last known training accuracy
    double inference_latency_ms  = 0.0;   ///< Last observed inference latency
    double duplicate_ratio       = 0.0;   ///< Fraction of near-duplicates detected

    DataSelectionMetrics() = default;
};

// ============================================================================
// Pipeline
// ============================================================================

/**
 * @brief Automated multi-stage data selection pipeline for LoRA training.
 *
 * Implements the five-stage selection process described in the feature issue:
 *
 *  Stage 1 – Quality Filtering:
 *      Token-length check, language detection, toxicity heuristic, PII check.
 *
 *  Stage 2 – Deduplication:
 *      MinHash / Jaccard near-duplicate removal with configurable threshold.
 *
 *  Stage 3 – Vector Clustering:
 *      Simulated k-means centroid selection for diversity-oriented sampling.
 *
 *  Stage 4 – Quality & Difficulty Scoring:
 *      Combined score from perplexity estimate, type-token ratio (TTR),
 *      and BM25 domain relevance.
 *
 *  Stage 5 – Curriculum Stratified Sampling:
 *      Partitions scored samples into easy / medium / hard buckets and
 *      samples according to configured ratios (default 10/70/20 %).
 *
 * Configuration is supplied via @ref LoRADataSelectionConfig which maps
 * directly to the YAML schema in LoRATrainerConfig.yaml.
 *
 * Usage:
 * @code
 * LoRADataSelectionConfig cfg;
 * cfg.min_length_tokens   = 50;
 * cfg.minhash_threshold   = 0.95;
 * cfg.target_samples      = 5000;
 * cfg.audit               = true;
 *
 * DataSelectionPipeline pipeline(cfg);
 * auto result = pipeline.run(raw_samples);
 * if (result.success) {
 *     // use result.selected_samples for training
 * }
 * @endcode
 */
class DataSelectionPipeline {
public:
    /**
     * @brief Construct pipeline with the given configuration.
     */
    explicit DataSelectionPipeline(const LoRADataSelectionConfig& config);
    ~DataSelectionPipeline();

    DataSelectionPipeline(const DataSelectionPipeline&) = delete;
    DataSelectionPipeline& operator=(const DataSelectionPipeline&) = delete;

    /**
     * @brief Execute all five pipeline stages on @p input_samples.
     * @param input_samples Raw samples loaded from the training collection.
     * @param callback      Optional per-stage progress callback.
     * @return Selection result including selected samples and audit entry.
     */
    DataSelectionResult run(
        const std::vector<DataSample>& input_samples,
        SelectionProgressCallback callback = nullptr);

    /**
     * @brief Run only Stage 1: quality filtering.
     *
     * Applies the shared prompt-safety policy before token/language/toxicity
     * checks. Samples matching blocked prompt-injection patterns are rejected
     * (fail-closed). Allowed samples continue with sanitized control-token
     * redaction applied to their text.
     * @return Samples that pass all quality filters.
     */
    std::vector<DataSample> filterByQuality(
        const std::vector<DataSample>& samples) const;

    /**
     * @brief Run only Stage 2: MinHash deduplication.
     * @return Samples with duplicates removed.
     */
    std::vector<DataSample> deduplicate(
        const std::vector<DataSample>& samples) const;

    /**
     * @brief Run only Stage 3: cluster-based diversity sampling.
     * @param samples   Samples to cluster.
     * @param k         Number of clusters (computed from config if 0).
     * @return Centroid-nearest samples covering diverse clusters.
     */
    std::vector<DataSample> clusterAndSample(
        const std::vector<DataSample>& samples,
        size_t k = 0) const;

    /**
     * @brief Run only Stage 4: quality/difficulty scoring.
     * Modifies quality_score and difficulty_score in-place.
     */
    void scoreQualityAndDifficulty(std::vector<DataSample>& samples) const;

    /**
     * @brief Run only Stage 5: curriculum stratified sampling.
     * @param scored_samples Samples with difficulty_score populated.
     * @param target         Total samples to return (0 = use config).
     * @return Stratified subset (easy + medium + hard).
     */
    std::vector<DataSample> stratifiedSample(
        const std::vector<DataSample>& scored_samples,
        size_t target = 0) const;

    /**
     * @brief Update the pipeline configuration (live reload support).
     */
    void setConfig(const LoRADataSelectionConfig& config);

    /**
     * @brief Get the current pipeline configuration.
     */
    const LoRADataSelectionConfig& getConfig() const;

    /**
     * @brief Derive a @ref DataSelectionMetrics snapshot from a completed
     *        pipeline result.
     *
     * Computes per-stage rejection rates and average quality/difficulty/
     * diversity scores from the returned samples.  Useful for feeding the
     * result directly into @ref SelfImprovementModule::applyAdaptiveRules()
     * and @ref SelfImprovementModule::needsRollback().
     *
     * @param result  The result returned by a previous `run()` call.
     * @return Populated metrics snapshot (all fields 0.0 for empty results).
     */
    static DataSelectionMetrics computeMetrics(const DataSelectionResult& result);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Self-improvement configuration
// ============================================================================

/**
 * @brief A single adaptive rule loaded from SelfImprovementModule.yaml.
 *
 * Each rule watches one monitoring @p metric, compares it against a
 * @p condition threshold, and applies an @p action (adjusting a config
 * field by @p delta) when the condition is met.
 */
struct AdaptiveRule {
    std::string metric;     ///< Monitored metric name (e.g. "avg_quality_score")
    std::string condition;  ///< Comparison operator + threshold (e.g. "< 0.60")
    std::string action;     ///< Field to adjust (e.g. "decrease_max_toxicity_score")
    double      delta = 0.0;///< Amount to add to the target field when triggered

    AdaptiveRule() = default;
};

/**
 * @brief Configuration for the adaptive self-improvement module.
 *
 * Controls automatic periodic re-selection and threshold adaptation.
 */
struct SelfImprovementConfig {
    bool   enabled                 = true;
    size_t period_seconds          = 86400;   ///< Re-selection period (default: 24 h)
    bool   threshold_auto_adjust   = true;    ///< Adapt thresholds from monitoring data
    double latency_target_ms       = 5000.0;  ///< Target max inference latency
    bool   accuracy_monitoring     = true;    ///< Track accuracy metrics

    // ---- Rollback thresholds ----
    double accuracy_rollback_threshold = 0.10; ///< Trigger rollback when accuracy drops > this
    double min_avg_quality_score       = 0.50; ///< Minimum acceptable avg quality score
    double max_error_rate              = 0.05; ///< Maximum acceptable error rate
    size_t cooldown_hours              = 6;    ///< Minimum hours between rollbacks

    // ---- Diversity monitoring ----
    bool   diversity_monitoring  = true;
    double min_diversity_score   = 0.30; ///< Minimum acceptable type-token ratio

    std::vector<AdaptiveRule> adaptive_rules; ///< Rules loaded from YAML

    SelfImprovementConfig() = default;

    /**
     * @brief Load configuration from a YAML file.
     *
     * Reads the `self_improvement:` section (or a custom @p section) from
     * the file at @p path.  Uses the same built-in line parser as
     * `LoRADataSelectionConfig::loadFromYAML()`.
     *
     * @throws std::runtime_error if the file cannot be opened.
     */
    static SelfImprovementConfig loadFromYAML(
        const std::string& path,
        const std::string& section = "self_improvement");

    /**
     * @brief Parse configuration from an in-memory YAML string.
     */
    static SelfImprovementConfig fromYAMLString(
        const std::string& yaml_text,
        const std::string& section = "self_improvement");
};

// ============================================================================
// SelfImprovementModule
// ============================================================================

/**
 * @brief Applies adaptive threshold adjustment rules to a
 *        @ref LoRADataSelectionConfig based on observed monitoring metrics.
 *
 * Intended usage (e.g., from a background thread or scheduler):
 * @code
 * SelfImprovementModule module(si_config);
 *
 * DataSelectionMetrics m;
 * m.avg_quality_score    = 0.55; // below 0.60 threshold
 * m.inference_latency_ms = 4200;
 *
 * LoRADataSelectionConfig updated = module.applyAdaptiveRules(current_cfg, m);
 * pipeline.setConfig(updated);   // live-reload
 * @endcode
 */
class SelfImprovementModule {
public:
    explicit SelfImprovementModule(const SelfImprovementConfig& config);
    ~SelfImprovementModule();

    SelfImprovementModule(const SelfImprovementModule&) = delete;
    SelfImprovementModule& operator=(const SelfImprovementModule&) = delete;

    /**
     * @brief Evaluate all adaptive rules against @p metrics.
     *
     * For each rule whose condition is satisfied, the corresponding field in
     * a copy of @p current_config is adjusted by the rule's delta.  The
     * modified copy is returned; the original is never mutated.
     *
     * @param current_config  Config snapshot to start from.
     * @param metrics         Current monitoring metrics.
     * @return Updated config (unchanged if no rules triggered, or if
     *         `threshold_auto_adjust` is false).
     */
    LoRADataSelectionConfig applyAdaptiveRules(
        const LoRADataSelectionConfig& current_config,
        const DataSelectionMetrics&    metrics) const;

    /**
     * @brief Return how many rules were triggered on the last call to
     *        `applyAdaptiveRules()`.
     */
    size_t lastTriggeredRuleCount() const;

    /**
     * @brief Decide whether the current metrics warrant a rollback.
     *
     * Returns true when any of the following holds:
     *  - `training_accuracy` has dropped by more than
     *    `accuracy_rollback_threshold` relative to a baseline of 1.0 – i.e.
     *    `(1.0 - metrics.training_accuracy) > accuracy_rollback_threshold`
     *  - `avg_quality_score < min_avg_quality_score`
     *  - `diversity_monitoring` is true and
     *    `diversity_score < min_diversity_score`
     *
     * Always returns false when `enabled` is false.
     *
     * @param metrics  Current monitoring snapshot.
     */
    bool needsRollback(const DataSelectionMetrics& metrics) const;

    /**
     * @brief Decide whether a new data selection run is due.
     *
     * Returns true when `enabled` is true and at least `period_seconds`
     * have elapsed since @p last_selection_time.
     *
     * @param last_selection_time  Time-point of the most recent pipeline run.
     */
    bool needsReselection(
        std::chrono::system_clock::time_point last_selection_time) const;

    /**
     * @brief Update the self-improvement configuration (live reload).
     */
    void setConfig(const SelfImprovementConfig& config);

    /**
     * @brief Get the current self-improvement configuration.
     */
    const SelfImprovementConfig& getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
