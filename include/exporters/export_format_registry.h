/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            export_format_registry.h                           ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-10                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "exporters/exporter_interface.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::exporters {

/// @brief Thread-safe singleton registry for pluggable export format writers.
///
/// New formats are registered with registerFormat() once (typically at
/// start-up or during plugin initialization).  Callers obtain an
/// `IExporter` instance by calling createExporter() with a format key
/// such as `"jsonl"`, `"parquet"`, `"arrow"`, or `"huggingface"`.
///
/// The registry is intentionally additive: registering a new format key
/// never changes the behaviour of existing keys, preserving the
/// "additive, non-breaking" constraint from the ROADMAP Breaking Changes
/// section.
///
/// ### Built-in formats (registered by registerBuiltins())
/// | Format key      | Exporter class             |
/// |-----------------|----------------------------|
/// | jsonl           | JSONLLLMExporter           |
/// | llm_jsonl       | JSONLLLMExporter           |
/// | parquet         | ParquetExporter            |
/// | arrow           | ArrowIPCExporter           |
/// | arrow_stream    | ArrowIPCExporter (stream)  |
/// | huggingface     | HuggingFaceExporter        |
/// | hf_datasets     | HuggingFaceExporter        |
/// | streaming       | StreamingExporter          |
/// | incremental     | IncrementalExporter        |
///
/// ### Example
/// ```cpp
/// auto& reg = ExportFormatRegistry::instance();
/// reg.registerBuiltins();                        // once at start-up
/// auto exp = reg.createExporter("parquet");
/// exp->exportEntities(entities, opts);
/// ```
class ExportFormatRegistry {
public:
    /// Factory function type — returns an owning IExporter pointer.
    using Factory = std::function<std::unique_ptr<IExporter>()>;

    /// Returns the process-global singleton.
    static ExportFormatRegistry& instance();

    /// @brief Register a factory for a format key.
    ///
    /// If `format_key` is already registered the old factory is replaced
    /// (allowing plug-ins to override built-in formats).
    /// Thread-safe.
    void registerFormat(const std::string& format_key, Factory factory);

    /// @brief Create an exporter for the given format key.
    ///
    /// @throws std::invalid_argument when `format_key` is unknown.
    /// Thread-safe.
    std::unique_ptr<IExporter> createExporter(const std::string& format_key) const;

    /// @returns true when at least one factory is registered for `format_key`.
    /// Thread-safe.
    bool hasFormat(const std::string& format_key) const;

    /// @returns sorted list of all registered format keys.
    /// Thread-safe.
    std::vector<std::string> registeredFormats() const;

    /// @brief Register all built-in format writers.
    ///
    /// Idempotent — safe to call multiple times.  Each call re-registers
    /// the built-in factories (useful for testing to reset to defaults).
    void registerBuiltins();

    /// @brief Remove all registered formats (useful for unit tests).
    void clear();

private:
    ExportFormatRegistry() = default;
    ~ExportFormatRegistry() = default;
    ExportFormatRegistry(const ExportFormatRegistry&) = delete;
    ExportFormatRegistry& operator=(const ExportFormatRegistry&) = delete;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Factory> formats_;
};

} // namespace themis::exporters
