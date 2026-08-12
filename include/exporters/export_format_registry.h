/**
 * @file export_format_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
/// | Format key        | Exporter class                                 |
/// |-------------------|------------------------------------------------|
/// | jsonl             | JSONLLLMExporter                               |
/// | llm_jsonl         | JSONLLLMExporter                               |
/// | parquet           | ParquetExporter                                |
/// | arrow             | ArrowIPCExporter (file)                        |
/// | arrow_stream      | ArrowIPCExporter (stream)                      |
/// | huggingface       | HuggingFaceExporter                            |
/// | hf_datasets       | HuggingFaceExporter                            |
/// | streaming         | StreamingExporter                              |
/// | incremental       | IncrementalExporter                            |
/// | jsonl_alpaca      | JSONLLLMExporter (FormatTemplateType::ALPACA)  |
/// | jsonl_sharegpt    | JSONLLLMExporter (FormatTemplateType::SHAREGPT)|
/// | jsonl_chatml      | JSONLLLMExporter (FormatTemplateType::CHATML)  |
/// | jsonl_openai_ft   | JSONLLLMExporter (FormatTemplateType::OPENAI_FINETUNING) |
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
    /// Registers plain formats and the four instruction-tuning template
    /// shortcuts: `jsonl_alpaca`, `jsonl_sharegpt`, `jsonl_chatml`,
    /// `jsonl_openai_ft`.
    void registerBuiltins();

    /// @brief Load user-defined template formats from a JSON config file.
    ///
    /// Reads the file at \p config_path, parses it as a JSON object, and
    /// registers each described template format via registerFormat().
    ///
    /// ### Expected JSON schema
    /// ```json
    /// {
    ///   "templates": [
    ///     {
    ///       "format_key":    "jsonl_my_template",
    ///       "template_type": "alpaca",
    ///       "field_mapping": {
    ///         "instruction_field":  "question",
    ///         "input_field":        "context",
    ///         "output_field":       "answer",
    ///         "system_field":       "system_prompt",
    ///         "user_field":         "user_message",
    ///         "assistant_field":    "assistant_response"
    ///       }
    ///     }
    ///   ]
    /// }
    /// ```
    /// Accepted `template_type` values: `"alpaca"`, `"sharegpt"`, `"chatml"`,
    /// `"openai_finetuning"`.  The `field_mapping` object is optional; absent
    /// fields use the `FormatTemplateFieldMapping` defaults.
    ///
    /// @throws std::runtime_error   when \p config_path cannot be opened.
    /// @throws std::invalid_argument when the JSON structure is invalid or a
    ///         required field (`format_key`, `template_type`) is missing.
    /// @throws nlohmann::json::parse_error when the file is not valid JSON.
    void loadTemplatesFromConfig(const std::string& config_path);

    /// @brief Load user-defined template formats from a JSON string.
    ///
    /// Equivalent to loadTemplatesFromConfig() but accepts the JSON content
    /// directly as a string — useful for testing and in-process configuration.
    ///
    /// @throws std::invalid_argument when the JSON structure is invalid or a
    ///         required field is missing.
    /// @throws nlohmann::json::parse_error when \p json_str is not valid JSON.
    void loadTemplatesFromJson(const std::string& json_str);

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
