/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_format_registry.cpp                         ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-10                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     104                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "exporters/export_format_registry.h"
#include "exporters/jsonl_llm_exporter.h"
#include "exporters/parquet_exporter.h"
#include "exporters/arrow_ipc_exporter.h"
#include "exporters/huggingface_exporter.h"
#include "exporters/streaming_exporter.h"
#include "exporters/incremental_exporter.h"

#include <algorithm>
#include <stdexcept>

namespace themis::exporters {

ExportFormatRegistry& ExportFormatRegistry::instance() {
    static ExportFormatRegistry registry;
    return registry;
}

void ExportFormatRegistry::registerFormat(const std::string& format_key, Factory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    formats_[format_key] = std::move(factory);
}

std::unique_ptr<IExporter> ExportFormatRegistry::createExporter(
    const std::string& format_key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = formats_.find(format_key);
    if (it == formats_.end()) {
        throw std::invalid_argument("ExportFormatRegistry: unknown format '" + format_key + "'");
    }
    return it->second();
}

bool ExportFormatRegistry::hasFormat(const std::string& format_key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return formats_.count(format_key) > 0;
}

std::vector<std::string> ExportFormatRegistry::registeredFormats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> keys;
    keys.reserve(formats_.size());
    for (const auto& [k, _] : formats_) {
        keys.push_back(k);
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

void ExportFormatRegistry::registerBuiltins() {
    // JSONL / LLM fine-tuning
    registerFormat("jsonl",     []() -> std::unique_ptr<IExporter> { return std::make_unique<JSONLLLMExporter>(); });
    registerFormat("llm_jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JSONLLLMExporter>(); });

    // Parquet columnar
    registerFormat("parquet",   []() -> std::unique_ptr<IExporter> { return std::make_unique<ParquetExporter>(); });

    // Apache Arrow IPC
    registerFormat("arrow",        []() -> std::unique_ptr<IExporter> {
        ArrowIPCExportConfig cfg; cfg.format = ArrowIPCFormat::FILE; return std::make_unique<ArrowIPCExporter>(cfg);
    });
    registerFormat("arrow_stream", []() -> std::unique_ptr<IExporter> {
        ArrowIPCExportConfig cfg; cfg.format = ArrowIPCFormat::STREAM; return std::make_unique<ArrowIPCExporter>(cfg);
    });

    // Hugging Face Datasets
    registerFormat("huggingface", []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });
    registerFormat("hf_datasets", []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });

    // Streaming (memory-bounded, large collections)
    registerFormat("streaming",   []() -> std::unique_ptr<IExporter> { return std::make_unique<StreamingExporter>(); });

    // Incremental / delta export
    registerFormat("incremental", []() -> std::unique_ptr<IExporter> { return std::make_unique<IncrementalExporter>(); });
}

void ExportFormatRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    formats_.clear();
}

} // namespace themis::exporters
