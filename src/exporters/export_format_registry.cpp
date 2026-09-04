/**
 * @file export_format_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/export_format_registry.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_map>

#include "exporters/arrow_ipc_exporter.h"
#include "exporters/format_template.h"
#include "exporters/huggingface_exporter.h"
#include "exporters/incremental_exporter.h"
#include "exporters/join_exporter.h"
#include "exporters/jsonl_llm_exporter.h"
#include "exporters/parquet_exporter.h"
#include "exporters/streaming_exporter.h"

namespace themis::exporters {

ExportFormatRegistry &ExportFormatRegistry::instance() {
    static ExportFormatRegistry registry;
    return registry;
}

void ExportFormatRegistry::registerFormat(const std::string &format_key, Factory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    formats_[format_key] = std::move(factory);
}

std::unique_ptr<IExporter> ExportFormatRegistry::createExporter(const std::string &format_key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = formats_.find(format_key);
    if (it == formats_.end()) {
        throw std::invalid_argument("ExportFormatRegistry: unknown format '" + format_key + "'");
    }
    return it->second();
}

bool ExportFormatRegistry::hasFormat(const std::string &format_key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return formats_.count(format_key) > 0;
}

std::vector<std::string> ExportFormatRegistry::registeredFormats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> keys = {};

    keys.reserve(formats_.size());
    for (const auto &[k, _] : formats_) {
        keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

void ExportFormatRegistry::registerBuiltins() {
    // JSONL / LLM fine-tuning
    registerFormat("jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JSONLLLMExporter>(); });
    registerFormat("llm_jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JSONLLLMExporter>(); });

    // Parquet columnar
    registerFormat("parquet", []() -> std::unique_ptr<IExporter> { return std::make_unique<ParquetExporter>(); });

    // Apache Arrow IPC
    registerFormat("arrow", []() -> std::unique_ptr<IExporter> {
        ArrowIPCExportConfig cfg;
        cfg.format = ArrowIPCFormat::FILE;
        return std::make_unique<ArrowIPCExporter>(cfg);
    });
    registerFormat("arrow_stream", []() -> std::unique_ptr<IExporter> {
        ArrowIPCExportConfig cfg;
        cfg.format = ArrowIPCFormat::STREAM;
        return std::make_unique<ArrowIPCExporter>(cfg);
    });

    // Hugging Face Datasets
    registerFormat("huggingface",
                   []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });
    registerFormat("hf_datasets",
                   []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });

    // Streaming (memory-bounded, large collections)
    registerFormat("streaming", []() -> std::unique_ptr<IExporter> { return std::make_unique<StreamingExporter>(); });

    // Incremental / delta export
    registerFormat("incremental",
                   []() -> std::unique_ptr<IExporter> { return std::make_unique<IncrementalExporter>(); });

    // Cross-collection join export (Issue #1722)
    registerFormat("join", []() -> std::unique_ptr<IExporter> { return std::make_unique<JoinExporter>(); });
    registerFormat("join_jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JoinExporter>(); });

    // Instruction-tuning template shortcuts
    registerFormat("jsonl_alpaca", []() -> std::unique_ptr<IExporter> {
        JSONLLLMConfig cfg;
        cfg.format_template_type = FormatTemplateType::ALPACA;
        return std::make_unique<JSONLLLMExporter>(cfg);
    });
    registerFormat("jsonl_sharegpt", []() -> std::unique_ptr<IExporter> {
        JSONLLLMConfig cfg;
        cfg.format_template_type = FormatTemplateType::SHAREGPT;
        return std::make_unique<JSONLLLMExporter>(cfg);
    });
    registerFormat("jsonl_chatml", []() -> std::unique_ptr<IExporter> {
        JSONLLLMConfig cfg;
        cfg.format_template_type = FormatTemplateType::CHATML;
        return std::make_unique<JSONLLLMExporter>(cfg);
    });
    registerFormat("jsonl_openai_ft", []() -> std::unique_ptr<IExporter> {
        JSONLLLMConfig cfg;
        cfg.format_template_type = FormatTemplateType::OPENAI_FINETUNING;
        return std::make_unique<JSONLLLMExporter>(cfg);
    });
}

void ExportFormatRegistry::loadTemplatesFromJson(const std::string &json_str) {
    static const std::unordered_map<std::string, FormatTemplateType> kTypeMap = {
        {"alpaca", FormatTemplateType::ALPACA},
        {"sharegpt", FormatTemplateType::SHAREGPT},
        {"chatml", FormatTemplateType::CHATML},
        {"openai_finetuning", FormatTemplateType::OPENAI_FINETUNING},
    };

    const auto j = nlohmann::json::parse(json_str); // throws nlohmann::json::parse_error on bad input

    if (!j.contains("templates") || !j["templates"].is_array()) {
        throw std::invalid_argument("ExportFormatRegistry: JSON config must contain a 'templates' array");
    }

    // --- Pass 1: validate all entries before touching the registry ---
    struct ValidatedEntry {
        std::string format_key;
        FormatTemplateType ttype;
        FormatTemplateFieldMapping mapping;
    };
    std::vector<ValidatedEntry> validated = {};

    validated.reserve(j["templates"].size());

    for (const auto &entry : j["templates"]) {
        if (!entry.contains("format_key") || !entry["format_key"].is_string()) {
            throw std::invalid_argument("ExportFormatRegistry: each template entry must have a 'format_key' string");
        }
        if (!entry.contains("template_type") || !entry["template_type"].is_string()) {
            throw std::invalid_argument("ExportFormatRegistry: each template entry must have a 'template_type' string");
        }

        const std::string format_key = entry["format_key"].get<std::string>();
        const std::string type_str   = entry["template_type"].get<std::string>();

        auto it = kTypeMap.find(type_str);
        if (it == kTypeMap.end()) {
            throw std::invalid_argument("ExportFormatRegistry: unknown template_type '" + type_str
                                        + "'; expected one of: alpaca, sharegpt, chatml, openai_finetuning");
        }

        FormatTemplateFieldMapping mapping;

        if (entry.contains("field_mapping") && entry["field_mapping"].is_object()) {
            const auto &fm = entry["field_mapping"];
            if (fm.contains("instruction_field") && fm["instruction_field"].is_string()) {
                mapping.instruction_field = fm["instruction_field"].get<std::string>();
            }
            if (fm.contains("input_field") && fm["input_field"].is_string()) {
                mapping.input_field = fm["input_field"].get<std::string>();
            }
            if (fm.contains("output_field") && fm["output_field"].is_string()) {
                mapping.output_field = fm["output_field"].get<std::string>();
            }
            if (fm.contains("system_field") && fm["system_field"].is_string()) {
                mapping.system_field = fm["system_field"].get<std::string>();
            }
            if (fm.contains("user_field") && fm["user_field"].is_string()) {
                mapping.user_field = fm["user_field"].get<std::string>();
            }
            if (fm.contains("assistant_field") && fm["assistant_field"].is_string()) {
                mapping.assistant_field = fm["assistant_field"].get<std::string>();
            }
        }

        validated.push_back({format_key, it->second, mapping});
    }

    // --- Pass 2: register — only reached when all entries are valid ---
    for (auto &ve : validated) {
        registerFormat(ve.format_key, [ttype = ve.ttype, mapping = ve.mapping]() -> std::unique_ptr<IExporter> {
            JSONLLLMConfig cfg;
            cfg.format_template_type   = ttype;
            cfg.template_field_mapping = mapping;
            return std::make_unique<JSONLLLMExporter>(cfg);
        });
    }
}

void ExportFormatRegistry::loadTemplatesFromConfig(const std::string &config_path) {
    std::ifstream f(config_path);
    if (!f.is_open()) {
        throw std::runtime_error("ExportFormatRegistry: cannot open template config file '" + config_path + "'");
    }
    const std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    loadTemplatesFromJson(content);
}

void ExportFormatRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    formats_.clear();
}

} // namespace themis::exporters
