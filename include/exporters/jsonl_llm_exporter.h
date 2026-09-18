/**
 * @file jsonl_llm_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "exporter_interface.h"
#include "exporter_metrics.h"
#include "format_template.h"
#include "plugins/plugin_interface.h"
#include <map>
#include <memory>

namespace themis {
namespace exporters {

struct JSONLFormat {
    enum class Style {
        INSTRUCTION_TUNING,  // {"instruction": ..., "input": ..., "output": ...}
        CHAT_COMPLETION,     // {"messages": [{"role": ..., "content": ...}]}
        TEXT_COMPLETION,     // {"text": ...}
        CUSTOM              // User-defined mapping
    };
};

struct JSONLLLMConfig {
    JSONLFormat::Style style = JSONLFormat::Style::INSTRUCTION_TUNING;
    
    // Field mappings for different styles
    struct FieldMapping {
        std::string instruction_field = "question";
        std::string input_field = "context";
        std::string output_field = "answer";
        std::string text_field = "content";
        std::string system_field = "system_prompt";
        std::string user_field = "user_message";
        std::string assistant_field = "assistant_response";
    } field_mapping;
    
    // Weighting strategy for training importance
    struct WeightConfig {
        bool enable_weights = true;
        std::string weight_field = "importance";  // Field containing weight (0.0-1.0)
        double default_weight = 1.0;
        
        // Auto-weighting strategies
        bool auto_weight_by_length = false;       // Longer responses = higher weight
        bool auto_weight_by_freshness = false;    // Newer data = higher weight
        std::string timestamp_field = "created_at";
    } weighting;
    
    // Quality filtering
    struct QualityFilter {
        size_t min_text_length = 10;
        size_t max_text_length = 8192;
        bool skip_empty_outputs = true;
        bool skip_duplicates = true;

        // Toxicity filtering: skip samples whose toxicity score exceeds this
        // threshold. Score is in [0.0, 1.0]; 1.0 disables filtering (default).
        bool enable_toxicity_filter = false;
        double max_toxicity_score = 0.8;
    } quality;
    
    // Metadata enrichment
    bool include_metadata = true;
    std::vector<std::string> metadata_fields = {"source", "category", "tags"};
    
    // Structured generation support (Outlines open-source integration)
    struct StructuredGeneration {
        bool enable_schema_validation = false;
        std::string json_schema;  // JSON Schema for output validation
        bool include_schema_in_output = false;  // Add schema field to JSONL
        bool reject_invalid_samples = true;  // Skip samples that don't match schema
        bool log_validation_errors = true;
    } structured_gen;
    
    // LoRA adapter metadata tracking
    struct AdapterMetadata {
        bool enable_tracking = false;
        std::string adapter_id;  // Unique identifier for this LoRA adapter
        std::string adapter_version = "1.0.0";
        std::string base_model_name;  // e.g., "llama-2-7b", "mistralai/Mistral-7B-v0.1"
        std::string base_model_version;
        std::string task_type;  // e.g., "question-answering", "summarization"
        std::string domain;  // e.g., "legal", "medical", "general"
        std::string language = "en";
        
        // vLLM-specific configuration
        struct VLLMConfig {
            bool enabled = false;
            std::string adapter_path;  // Path where adapter will be deployed for vLLM
            std::string vllm_version = ">=0.4.0";  // Minimum vLLM version required
            int max_lora_rank = 16;  // Maximum LoRA rank supported
            bool enable_multi_lora = true;  // Support multi-LoRA batching
            std::map<std::string, std::string> serving_config;  // vLLM serving parameters
        } vllm_config;
        
        // Training metadata (to be filled during training)
        struct TrainingConfig {
            std::string dataset_name;
            size_t num_samples = 0;
            int epochs = 0;
            double learning_rate = 0.0;
            int lora_rank = 8;
            double lora_alpha = 16.0;
            double lora_dropout = 0.1;
            std::vector<std::string> target_modules;  // e.g., ["q_proj", "v_proj"]
        } training_config;
        
        // Provenance
        std::string created_by;
        std::string data_source_uri;  // ThemisDB connection string or query
        std::string parent_adapter_id;  // For incremental training
        std::map<std::string, std::string> custom_metadata;
    } adapter_metadata;
    
    // Quality metrics tracking
    struct QualityMetrics {
        bool enable_metrics = false;
        bool track_per_sample = false;
        bool aggregate_stats = true;
        
        // Metrics to track
        bool track_schema_compliance = true;
        bool track_length_distribution = true;
        bool track_diversity_score = true;  // Unique n-grams ratio
    } quality_metrics;
    
    // P1: PII Detection & Redaction
    struct PIIConfig {
        bool enable_detection = false;
        bool enable_redaction = false;
        bool detect_email = true;
        bool detect_phone = true;
        bool detect_ssn = true;
        bool detect_credit_card = true;
        
        // Redaction strategy: mask, hash, remove, partial
        std::string redaction_strategy = "mask";
        
        // Fields to check for PII
        std::vector<std::string> check_fields;  // Empty = check all text fields
        
        // Fail export on PII detection (without redaction)
        bool fail_on_pii = false;
    } pii_config;

    // Instruction-tuning format template.
    // When set to anything other than NONE this overrides the `style` field
    // and the entity is rendered through the selected named template.
    FormatTemplateType format_template_type = FormatTemplateType::NONE;

    // Field-name overrides for format templates.
    // Mirrors FieldMapping but is forwarded to IFormatTemplate::render().
    FormatTemplateFieldMapping template_field_mapping;
};

class JSONLLLMExporter : public IExporter {
public:
    explicit JSONLLLMExporter(const JSONLLLMConfig& config = {});
    
    ExportStats exportEntities(
        const std::vector<BaseEntity>& entities,
        const ExportOptions& options
    ) override;
    
    std::vector<std::string> getSupportedFormats() const override {
        return {"jsonl", "llm_jsonl", "lora_jsonl", "qlora_jsonl"};
    }
    
    std::string getName() const override { return "jsonl_llm_exporter"; }
    std::string getVersion() const override { return "1.0.0"; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Calls: makeFormatTemplate().
     */
    void setConfig(const JSONLLLMConfig& config) {
        config_ = config;
        format_template_ = makeFormatTemplate(config.format_template_type);
    }
    
    /**
     * @brief Validate Template.
     * @param[in] sample Input parameter.
     * @return Return value.
     */
    TemplateValidationResult validateTemplate(
        const std::vector<BaseEntity>& sample
    ) const;

    const JSONLLLMConfig& getConfig() const { return config_; }
    
    bool validateAgainstSchema(const std::string& json_str, std::string* error = nullptr) const;
    
    /**
     * @brief Get Adapter Metadata Json.
     * @return Return value.
     */
    std::string getAdapterMetadataJson() const;
    
    bool setAdapterMetadataFromJson(const std::string& json_str, std::string* error = nullptr);
    
    /**
     * @brief Get Quality Metrics Report.
     * @return Return value.
     */
    std::string getQualityMetricsReport() const;
    
    std::shared_ptr<ExporterMetrics> getMetrics() const { return metrics_; }
    
    /**
     * @brief Reset Metrics.
     * @details Calls: reset().
     */
    void resetMetrics() { if (metrics_) metrics_->reset(); }
    
private:
    JSONLLLMConfig config_;
    std::shared_ptr<ExporterMetrics> metrics_;
    std::unique_ptr<IFormatTemplate> format_template_;  // non-null when format_template_type != NONE

    // Export helpers
    /**
     * @brief Format Instruction Tuning.
     * @param[in] entity Input parameter.
     * @param[in,out] weight Input/output parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::string formatInstructionTuning(const BaseEntity& entity, double& weight,
                                        const ExportOptions& options);
    /**
     * @brief Format Chat Completion.
     * @param[in] entity Input parameter.
     * @param[in,out] weight Input/output parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::string formatChatCompletion(const BaseEntity& entity, double& weight,
                                     const ExportOptions& options);
    /**
     * @brief Format Text Completion.
     * @param[in] entity Input parameter.
     * @param[in,out] weight Input/output parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::string formatTextCompletion(const BaseEntity& entity, double& weight,
                                     const ExportOptions& options);
    /**
     * @brief Format With Template.
     * @param[in] entity Input parameter.
     * @param[in,out] weight Input/output parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::string formatWithTemplate(const BaseEntity& entity, double& weight,
                                   const ExportOptions& options);

    /**
     * @brief Calculate Weight.
     * @param[in] entity Input parameter.
     * @return Return value.
     */
    double calculateWeight(const BaseEntity& entity);
    /**
     * @brief Passes Quality Filter.
     * @param[in] entity Input parameter.
     * @return True when the operation succeeds.
     */
    bool passesQualityFilter(const BaseEntity& entity);
    /**
     * @brief Extract Metadata.
     * @param[in] entity Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    std::string extractMetadata(const BaseEntity& entity, const ExportOptions& options);

    /**
     * @brief Is Field Allowed.
     * @param[in] field_name Name of the field.
     * @param[in] include_fields Input parameter.
     * @param[in] exclude_fields Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isFieldAllowed(const std::string& field_name,
                                const std::vector<std::string>& include_fields,
                                const std::vector<std::string>& exclude_fields);
    
    // Schema validation helpers
    /**
     * @brief Validate Json Schema.
     * @param[in] json_str Input parameter.
     * @param[in] schema Input parameter.
     * @param[in,out] error Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateJsonSchema(const std::string& json_str, const std::string& schema, std::string* error) const;
    
    // Quality metrics tracking
    mutable struct RuntimeMetrics {
        size_t total_validated = 0;
        size_t schema_compliant = 0;
        size_t schema_violations = 0;
        std::map<size_t, size_t> length_distribution;  // bucket -> count
        double diversity_score = 0.0;
        std::vector<std::string> validation_errors;
    } runtime_metrics_;
};

#ifdef THEMIS_ENABLE_JSONL_PLUGIN
using ::themis::plugins::IThemisPlugin;
using ::themis::plugins::PluginCapabilities;
using ::themis::plugins::PluginType;

class JSONLLLMExporterPlugin : public ::themis::plugins::IThemisPlugin {
public:
    JSONLLLMExporterPlugin() = default;

    const char* getName() const override { return "jsonl_llm_exporter"; }
    const char* getVersion() const override { return "1.0.0"; }
    ::themis::plugins::PluginType getType() const override { return ::themis::plugins::PluginType::EXPORTER; }
    ::themis::plugins::PluginCapabilities getCapabilities() const override { return {}; }

    bool initialize([[maybe_unused]] const char* config_json) override {
        exporter_ = std::make_unique<JSONLLLMExporter>();
        return true;
    }

    void shutdown() override { exporter_.reset(); }

    void* getInstance() override { return static_cast<IExporter*>(exporter_.get()); }

private:
    std::unique_ptr<JSONLLLMExporter> exporter_;
};
#endif // THEMIS_ENABLE_JSONL_PLUGIN


} // namespace exporters
} // namespace themis
