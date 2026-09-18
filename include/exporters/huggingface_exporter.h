/**
 * @file huggingface_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "exporter_interface.h"
#include "exporter_metrics.h"
#include "jsonl_llm_exporter.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace themis {
namespace exporters {

struct HuggingFaceFeature {
    std::string name;
    std::string dtype = "string";  // "string", "int32", "int64", "float32", "float64", "bool"
    std::string hf_type = "Value"; // Hugging Face feature type: "Value", "Sequence", "ClassLabel"
};

struct HuggingFaceExporterConfig {
    // Dataset metadata (used in dataset_info.json and dataset card)
    std::string dataset_name;
    std::string description;
    std::string license = "other";
    std::string homepage;
    std::string citation;

    // Split configuration (default: single "train" split)
    std::string split_name = "train";

    // Feature/schema definition
    // If empty and infer_features is true, features will be inferred from the first batch
    std::vector<HuggingFaceFeature> features;
    bool infer_features = true;  // Infer missing feature dtypes from exported entities

    // Dataset card (README.md) generation
    bool generate_dataset_card = true;
    std::string dataset_card_template;  // Optional custom template; default is auto-generated

    // Tags for the dataset card YAML frontmatter
    std::vector<std::string> tags;
    std::string language = "en";
    std::string task_category;  // e.g., "text-generation", "question-answering"

    // Underlying JSONL configuration (field mapping, quality filters, PII, etc.)
    JSONLLLMConfig jsonl_config;
};

class HuggingFaceExporter : public IExporter {
public:
    explicit HuggingFaceExporter(const HuggingFaceExporterConfig& config = {});

    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    std::vector<std::string> getSupportedFormats() const override {
        return {"huggingface", "hf_datasets", "hf_jsonl"};
    }

    std::string getName() const override { return "huggingface_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Calls: clear().
     */
    void setConfig(const HuggingFaceExporterConfig& config) {
        config_ = config;
        inferred_features_.clear();
    }

    const HuggingFaceExporterConfig& getConfig() const { return config_; }

    std::string generateDatasetInfoJson(
        const ExportStats& stats,
        const std::string& dataset_name = {},
        size_t data_file_bytes = 0
    ) const;

    /**
     * @brief Generate Dataset Card.
     * @return Return value.
     */
    std::string generateDatasetCard() const;

    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

private:
    HuggingFaceExporterConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    /**
     * @brief Infer Dtype.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    static std::string inferDtype(const Value& value);

    /**
     * @brief Infer Features.
     * @param[in] entities Input parameter.
     */
    void inferFeatures(const std::vector<BaseEntity>& entities);

    /**
     * @brief Resolved Features.
     * @return Return value.
     */
    const std::vector<HuggingFaceFeature>& resolvedFeatures() const;

    mutable std::vector<HuggingFaceFeature> inferred_features_;
};

} // namespace exporters
} // namespace themis
