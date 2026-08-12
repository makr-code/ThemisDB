/**
 * @file huggingface_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// Represents a single feature (column) in a Hugging Face dataset schema
struct HuggingFaceFeature {
    std::string name;
    std::string dtype = "string";  // "string", "int32", "int64", "float32", "float64", "bool"
    std::string hf_type = "Value"; // Hugging Face feature type: "Value", "Sequence", "ClassLabel"
};

/// Configuration for Hugging Face Datasets-compatible export
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

/// Exporter producing Hugging Face Datasets-compatible output.
///
/// Output layout (output_path is treated as the dataset root directory):
///   <output_path>/
///     dataset_info.json       – HF dataset metadata / features schema
///     README.md               – dataset card (optional)
///     data/
///       <split>-00000-of-00001.jsonl  – JSONL records
///
/// The resulting directory can be loaded directly with:
///   datasets.load_dataset("json", data_dir="<output_path>")
/// or, after pushing to HF Hub, with:
///   datasets.load_dataset("<username>/<dataset_name>")
class HuggingFaceExporter : public IExporter {
public:
    explicit HuggingFaceExporter(const HuggingFaceExporterConfig& config = {});

    /// Export entities to a HuggingFace-compatible dataset directory.
    /// @param entities  Entities to export.
    /// @param options   options.output_path is used as the dataset root directory.
    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;

    std::vector<std::string> getSupportedFormats() const override {
        return {"huggingface", "hf_datasets", "hf_jsonl"};
    }

    std::string getName() const override { return "huggingface_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }

    /// Set custom configuration (clears any previously inferred features)
    void setConfig(const HuggingFaceExporterConfig& config) {
        config_ = config;
        inferred_features_.clear();
    }

    /// Get current configuration
    const HuggingFaceExporterConfig& getConfig() const { return config_; }

    /// Generate dataset_info.json content from the current configuration and export stats.
    /// @param stats            Stats from the completed JSONL export.
    /// @param dataset_name     Dataset name (falls back to config_.dataset_name).
    /// @param data_file_bytes  Byte size of the written data file (0 = use stats.bytes_written).
    std::string generateDatasetInfoJson(
        const ExportStats& stats,
        const std::string& dataset_name = {},
        size_t data_file_bytes = 0
    ) const;

    /// Generate README.md dataset card content.
    std::string generateDatasetCard() const;

    /// Get exporter metrics
    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }

private:
    HuggingFaceExporterConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;

    /// Infer HuggingFace feature dtype from a Value variant.
    static std::string inferDtype(const Value& value);

    /// Infer feature list from a set of entities (uses first entity with each field).
    void inferFeatures(const std::vector<BaseEntity>& entities);

    /// Return the resolved feature list (inferred or configured).
    const std::vector<HuggingFaceFeature>& resolvedFeatures() const;

    mutable std::vector<HuggingFaceFeature> inferred_features_;
};

} // namespace exporters
} // namespace themis
