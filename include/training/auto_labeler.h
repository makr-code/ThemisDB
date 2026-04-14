/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auto_labeler.h                                     ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:30:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     219                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ac63c2ec8d  2026-04-12  [WIP] Update developer documentation for module training ... ║
    • f499362b11  2026-03-11  feat(training): wire LegalAutoLabeler DB fetch to AQL que... ║
    • ce712594b0  2026-03-09  feat(training): Phase 3 enhancements - checkpoint manager... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace themis {

// Forward declare to avoid circular dependency
namespace analytics {
    struct LegalModality;
}

// Forward declare QueryEngine to keep the training header free of heavy dependencies
class QueryEngine;

namespace training {

/**
 * @brief Content modality of a training sample.
 *
 * Populated by the ModalityDetector in auto_labeler.cpp so that the training
 * pipeline can apply modality-specific confidence thresholds.
 */
enum class ContentModality {
    TEXT_CLAUSE,  ///< Plain-text legal clause (default modality)
    TABLE,        ///< Structured table (e.g., damages schedule)
    CITATION,     ///< Embedded statutory or case-law citation
    OCR_IMAGE,    ///< Scanned page processed via OCR
    UNKNOWN       ///< Modality could not be determined
};

/**
 * @brief Training sample with auto-generated labels
 */
struct TrainingSample {
    std::string input;           ///< Input text
    std::string output;          ///< Expected output/label
    std::string category;        ///< Category (e.g., "obligation", "permission")
    float confidence;            ///< Confidence score [0.0, 1.0]
    std::string source_id;       ///< Source document ID
    std::string metadata;        ///< Additional metadata (JSON)
    ContentModality modality = ContentModality::TEXT_CLAUSE; ///< Content modality
    
    TrainingSample() : confidence(0.0f) {}
};

/**
 * @brief Auto-labeling statistics
 */
struct LabelingStats {
    size_t documents_processed = 0;
    size_t samples_created = 0;
    size_t high_confidence_samples = 0;  ///< confidence >= 0.8
    size_t low_confidence_samples = 0;   ///< confidence < 0.5
    double elapsed_seconds = 0.0;
    
    LabelingStats() = default;
};

/**
 * @brief Labeling progress callback
 */
using LabelingCallback = std::function<void(size_t processed, 
                                            size_t total,
                                            const std::string& status)>;

/**
 * @brief Target domain for auto-labeling and sample extraction.
 *
 * Controls domain-specific keyword dictionaries and NLP heuristics used by
 * `LegalAutoLabeler` when the external NlpTextAnalyzer is unavailable or
 * returns no modalities.
 *
 * - LEGAL    : German legal text (modal verbs: muss/soll/kann/darf/…)
 * - MEDICAL  : Medical / clinical text (obligatory / recommended / optional care)
 * - FINANCIAL: Financial regulatory text (obligation / prohibition / disclosure)
 */
enum class DomainType {
    LEGAL,      ///< German legal / regulatory domain (default)
    MEDICAL,    ///< Medical / clinical / pharmaceutical domain
    FINANCIAL,  ///< Financial regulation / compliance domain
};

/**
 * @brief Configuration for auto-labeling
 */
struct AutoLabelConfig {
    std::string source_collection;          ///< Collection with raw documents
    std::string target_collection;          ///< Collection for training samples
    std::string language_code = "de";       ///< Language code (e.g., "de" for German)
    std::string modal_verbs_config;         ///< Path to modal verbs YAML config
    float min_confidence = 0.5f;            ///< Minimum confidence to include sample
    bool flag_low_confidence = true;        ///< Flag low-confidence for review
    size_t batch_size = 100;                ///< Documents per batch
    DomainType domain_type = DomainType::LEGAL; ///< Target domain for sample extraction

    AutoLabelConfig() = default;
};

/**
 * @brief Legal document auto-labeler
 * 
 * Automatically labels legal documents using the Legal Modality Analyzer from PR #1.
 * Creates training samples from documents by extracting legal modalities (modal verbs
 * like "muss", "soll", "kann") with deontic logic annotations.
 * 
 * Integration with PR #1:
 * - Uses NlpTextAnalyzer::extractLegalModalities() for modal verb detection
 * - Generates training samples with category, strength, and deontic logic
 * - Flags low-confidence samples for human review
 * 
 * Example usage:
 * @code
 * AutoLabelConfig config;
 * config.source_collection = "legal_documents";
 * config.target_collection = "legal_training_samples";
 * config.language_code = "de";
 * config.min_confidence = 0.5f;
 * 
 * LegalAutoLabeler labeler(config, db);
 * auto stats = labeler.labelAll();
 * std::cout << "Created " << stats.samples_created << " training samples\n";
 * @endcode
 */
class LegalAutoLabeler {
public:
    /**
     * @brief Construct auto-labeler without a database connection
     * @param config Labeling configuration
     * @param db_connection Database connection string (informational)
     * @param engine Optional AQL query engine; when non-null, labelAll() and
     *               labelQuery() fetch document IDs from the database via AQL.
     *               Pass nullptr (the default) to operate in test/offline mode,
     *               where no documents are fetched from the database.
     */
    explicit LegalAutoLabeler(const AutoLabelConfig& config,
                              const std::string& db_connection,
                              QueryEngine* engine = nullptr);
    
    ~LegalAutoLabeler();
    
    // Delete copy
    LegalAutoLabeler(const LegalAutoLabeler&) = delete;
    LegalAutoLabeler& operator=(const LegalAutoLabeler&) = delete;
    
    /**
     * @brief Label all documents in source collection
     * @param callback Optional progress callback
     * @return Labeling statistics
     */
    LabelingStats labelAll(LabelingCallback callback = nullptr);
    
    /**
     * @brief Label a specific document
     * @param document_id Document ID
     * @return Vector of generated training samples
     */
    std::vector<TrainingSample> labelDocument(const std::string& document_id);
    
    /**
     * @brief Label documents matching a query
     * @param aql_query AQL query to select documents
     * @param callback Optional progress callback
     * @return Labeling statistics
     */
    LabelingStats labelQuery(const std::string& aql_query,
                            LabelingCallback callback = nullptr);
    
    /**
     * @brief Get low-confidence samples for human review
     * @param min_confidence Minimum confidence threshold
     * @return Vector of low-confidence samples
     */
    std::vector<TrainingSample> getLowConfidenceSamples(float min_confidence = 0.5f);
    
    /**
     * @brief Update sample confidence after human review
     * @param sample_id Sample ID
     * @param new_confidence Updated confidence score
     * @param reviewed_by User who reviewed the sample
     */
    void updateSampleConfidence(const std::string& sample_id,
                               float new_confidence,
                               const std::string& reviewed_by);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
