/**
 * @file huggingface_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/huggingface_exporter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>

#include "exporters/exporter_errors.h"
#include "exporters/exporter_interface.h"
#include "utils/logger.h"

using json   = nlohmann::json;
namespace fs = std::filesystem;

namespace themis::exporters {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

HuggingFaceExporter::HuggingFaceExporter(const HuggingFaceExporterConfig &config)
    : config_(config), metrics_(std::make_shared<ExporterMetrics>()) {}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------

ExportStats HuggingFaceExporter::exportEntities(const std::vector<BaseEntity> &entities, const ExportOptions &options) {
    // Policy check before any cursor or file is opened (EXP-001).
    enforceExportPolicy(options);

    ExportStats stats;
    stats.metrics   = metrics_;
    auto start_time = std::chrono::steady_clock::now();

    // Resolve dataset root directory from options.output_path
    const std::string dataset_root = options.output_path;
    if (dataset_root.empty()) {
        stats.errors.push_back("output_path must be set to the dataset root directory");
        stats.duration
            = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
        return stats;
    }

    try {
        // Create directory layout: <root>/data/
        const fs::path root_dir(dataset_root);
        const fs::path data_dir = root_dir / "data";
        fs::create_directories(data_dir);

        // Build data file name: <split>-00000-of-00001.jsonl
        const std::string split         = config_.split_name.empty() ? "train" : config_.split_name;
        const std::string data_filename = split + "-00000-of-00001.jsonl";
        const fs::path data_file        = data_dir / data_filename;

        // Optionally infer features from the entity set
        if (config_.infer_features && config_.features.empty()) {
            inferFeatures(entities);
        }

        // Delegate JSONL writing to JSONLLLMExporter
        JSONLLLMExporter jsonl_exporter(config_.jsonl_config);
        ExportOptions jsonl_options = options;
        jsonl_options.output_path   = data_file.string();

        ExportStats jsonl_stats = jsonl_exporter.exportEntities(entities, jsonl_options);

        // Propagate statistics
        stats.total_entities    = jsonl_stats.total_entities;
        stats.exported_entities = jsonl_stats.exported_entities;
        stats.failed_entities   = jsonl_stats.failed_entities;
        stats.bytes_written     = jsonl_stats.bytes_written;
        stats.errors            = jsonl_stats.errors;

        // Write dataset_info.json
        const std::string resolved_name
            = config_.dataset_name.empty() ? root_dir.filename().string() : config_.dataset_name;
        const std::string info_json = generateDatasetInfoJson(jsonl_stats, resolved_name, jsonl_stats.bytes_written);
        {
            const fs::path info_path = root_dir / "dataset_info.json";
            std::ofstream info_file(info_path);
            if (!info_file) {
                throw ExportIOException("Failed to open dataset_info.json for writing", info_path.string());
            }
            info_file << info_json;
        }
        stats.bytes_written += info_json.size();

        // Write README.md (dataset card) if requested
        if (config_.generate_dataset_card) {
            const std::string card   = generateDatasetCard();
            const fs::path card_path = root_dir / "README.md";
            std::ofstream card_file(card_path);
            if (!card_file) {
                THEMIS_WARN("Failed to write dataset card to {}", card_path.string());
            } else {
                card_file << card;
                stats.bytes_written += card.size();
            }
        }

        auto end_time  = std::chrono::steady_clock::now();
        stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        metrics_->recordExport(stats.exported_entities, stats.bytes_written, stats.duration);

        THEMIS_INFO("HuggingFace export completed: {} entities -> {} ({}ms)", stats.exported_entities, dataset_root,
                    stats.duration.count());

    } catch (const ExportIOException &e) {
        stats.errors.push_back("[" + std::to_string(static_cast<int>(e.getErrorCode())) + "] " + e.what()
                               + " (file: " + e.getFilePath() + ")");
        metrics_->recordError("io_exception");

        auto end_time  = std::chrono::steady_clock::now();
        stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    } catch (const std::filesystem::filesystem_error &e) {
        stats.errors.push_back(std::string("Filesystem error: ") + e.what() + " (path: " + e.path1().string() + ")");
        metrics_->recordError("filesystem_error");

        auto end_time  = std::chrono::steady_clock::now();
        stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    } catch (const std::exception &e) {
        stats.errors.push_back(std::string("Unexpected error: ") + e.what());
        metrics_->recordError("unexpected_error");

        auto end_time  = std::chrono::steady_clock::now();
        stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    }

    return stats;
}

// ---------------------------------------------------------------------------
// dataset_info.json generation
// ---------------------------------------------------------------------------

std::string HuggingFaceExporter::generateDatasetInfoJson(const ExportStats &stats, const std::string &dataset_name,
                                                         size_t data_file_bytes) const {
    const std::string resolved_name
        = dataset_name.empty() ? (config_.dataset_name.empty() ? "dataset" : config_.dataset_name) : dataset_name;

    const size_t byte_count = data_file_bytes > 0 ? data_file_bytes : stats.bytes_written;
    const std::string split = config_.split_name.empty() ? "train" : config_.split_name;

    json info;
    info["description"] = config_.description;
    info["citation"]    = config_.citation;
    info["homepage"]    = config_.homepage;
    info["license"]     = config_.license;

    // features
    json features_obj = json::object();
    for (const auto &feat : resolvedFeatures()) {
        features_obj[feat.name] = {{"dtype", feat.dtype}, {"_type", feat.hf_type}};
    }
    info["features"] = features_obj;

    // splits
    json split_obj;
    split_obj["name"]         = split;
    split_obj["num_bytes"]    = byte_count;
    split_obj["num_examples"] = stats.exported_entities;
    split_obj["dataset_name"] = resolved_name;
    info["splits"][split]     = split_obj;

    info["download_size"] = byte_count;
    info["dataset_size"]  = byte_count;
    info["builder_name"]  = "json";
    info["config_name"]   = "default";
    info["version"]       = {{"version_str", "0.0.0"}, {"major", 0}, {"minor", 0}, {"patch", 0}};

    return info.dump(2);
}

// ---------------------------------------------------------------------------
// README.md (dataset card) generation
// ---------------------------------------------------------------------------

/// Escape a string for safe embedding as a YAML double-quoted scalar.
/// Wraps the value in double quotes and escapes backslashes, double-quotes,
/// and control characters so that the resulting YAML front matter is always
/// syntactically valid regardless of the input.
static std::string yamlQuote(const std::string &s) {
    std::string out = {};
    out.reserve(static_cast<int>(s.size()) + 2);
    out += '"';
    for (unsigned char c : s) {
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else {
            out += static_cast<char>(c);
        }
    }
    out += '"';
    return out;
}

std::string HuggingFaceExporter::generateDatasetCard() const {
    // Use custom template if provided
    if (!config_.dataset_card_template.empty()) {
        return config_.dataset_card_template;
    }

    const std::string split = config_.split_name.empty() ? "train" : config_.split_name;
    const std::string name  = config_.dataset_name.empty() ? "dataset" : config_.dataset_name;

    std::ostringstream card = {};

    // YAML front matter
    card << "---\n";
    if (!config_.license.empty()) {
        card << "license: " << yamlQuote(config_.license) << "\n";
    }
    if (!config_.language.empty()) {
        card << "language:\n- " << yamlQuote(config_.language) << "\n";
    }
    if (!config_.task_category.empty()) {
        card << "task_categories:\n- " << yamlQuote(config_.task_category) << "\n";
    }
    if (!config_.tags.empty()) {
        card << "tags:\n";
        for (const auto &tag : config_.tags) {
            card << "- " << yamlQuote(tag) << "\n";
        }
    }

    // dataset_info block in YAML front matter
    card << "dataset_info:\n";
    card << "  features:\n";
    for (const auto &feat : resolvedFeatures()) {
        card << "  - name: " << yamlQuote(feat.name) << "\n";
        card << "    dtype: " << yamlQuote(feat.dtype) << "\n";
    }
    card << "  splits:\n";
    card << "  - name: " << yamlQuote(split) << "\n";
    card << "---\n\n";

    // Markdown body
    card << "# " << name << "\n\n";
    if (!config_.description.empty()) {
        card << config_.description << "\n\n";
    }

    card << "## Dataset Structure\n\n";
    card << "### Data Fields\n\n";
    for (const auto &feat : resolvedFeatures()) {
        card << "- **" << feat.name << "** (`" << feat.dtype << "`)\n";
    }
    card << "\n";

    card << "### Data Splits\n\n";
    card << "| Split | Examples |\n";
    card << "|-------|----------|\n";
    card << "| " << split << " | - |\n\n";

    if (!config_.citation.empty()) {
        card << "## Citation\n\n";
        card << "```bibtex\n" << config_.citation << "\n```\n";
    }

    return card.str();
}

// ---------------------------------------------------------------------------
// Feature inference helpers
// ---------------------------------------------------------------------------

std::string HuggingFaceExporter::inferDtype(const Value &value) {
    return std::visit(
        [](const auto &v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return "string";
            } else if constexpr (std::is_same_v<T, bool>) {
                return "bool";
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return "int64";
            } else if constexpr (std::is_same_v<T, double>) {
                return "float64";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "string";
            } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                return "sequence<float32>";
            } else {
                return "string";
            }
        },
        value);
}

void HuggingFaceExporter::inferFeatures(const std::vector<BaseEntity> &entities) {
    inferred_features_.clear();
    if (entities.empty()) {
        return;
    }

    // Collect all field names across all entities
    std::map<std::string, std::string> field_dtypes = {};

    for (const auto &entity : entities) {
        const auto fields = entity.getAllFields();
        for (const auto &[field_name, value] : fields) {
            if (field_dtypes.find(field_name) == field_dtypes.end()) {
                field_dtypes[field_name] = inferDtype(value);
            }
        }
    }

    for (const auto &[name, dtype] : field_dtypes) {
        HuggingFaceFeature feat;
        feat.name  = name;
        feat.dtype = dtype;
        inferred_features_.push_back(std::move(feat));
    }
}

const std::vector<HuggingFaceFeature> &HuggingFaceExporter::resolvedFeatures() const {
    if (!config_.features.empty()) {
        return config_.features;
    }
    return inferred_features_;
}

} // namespace themis::exporters
