/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auto_labeler.h                                     ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     172                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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

namespace training {

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
     * @brief Construct auto-labeler
     * @param config Labeling configuration
     * @param db_connection Database connection string
     */
    explicit LegalAutoLabeler(const AutoLabelConfig& config,
                              const std::string& db_connection);
    
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
