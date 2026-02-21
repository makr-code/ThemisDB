/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            jsonl_llm_exporter.cpp                             ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     881                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "exporters/jsonl_llm_exporter.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_metrics.h"
#include "exporters/pii_detector.h"
#include "exporters/stream_writer.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <set>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace themis::exporters {

std::string ExportStats::toJson() const {
    json j;
    j["total_entities"] = total_entities;
    j["exported_entities"] = exported_entities;
    j["failed_entities"] = failed_entities;
    j["bytes_written"] = bytes_written;
    j["duration_ms"] = duration.count();
    j["errors"] = errors;
    
    // Include metrics if available
    if (metrics) {
        j["metrics"] = metrics->toJson();
    }
    
    return j.dump(2);
}

JSONLLLMExporter::JSONLLLMExporter(const JSONLLLMConfig& config)
    : config_(config), metrics_(std::make_shared<ExporterMetrics>()) {}

ExportStats JSONLLLMExporter::exportEntities(
    const std::vector<BaseEntity>& entities,
    const ExportOptions& options
) {
    ExportStats stats;
    stats.metrics = metrics_;  // Attach metrics to stats
    auto start_time = std::chrono::steady_clock::now();
    
    // P1: Tenant isolation check
    if (options.tenant_context && options.tenant_context->enforce_isolation) {
        // Check required scopes
        if (!options.tenant_context->hasScope("export:read") && 
            !options.tenant_context->hasScope("export:write")) {
            throw ExporterException(
                themis::errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,
                "Insufficient permissions for export operation",
                "tenant_id=" + options.tenant_context->tenant_id
            );
        }
        
        THEMIS_INFO("Export for tenant: {}, user: {}", 
                   options.tenant_context->tenant_id,
                   options.tenant_context->user_id);
    }
    
    // P1: Initialize PII detector if enabled
    std::unique_ptr<PIIDetector> pii_detector;
    if (config_.pii_config.enable_detection) {
        PIIDetector::Config pii_config;
        pii_config.detect_email = config_.pii_config.detect_email;
        pii_config.detect_phone = config_.pii_config.detect_phone;
        pii_config.detect_ssn = config_.pii_config.detect_ssn;
        pii_config.detect_credit_card = config_.pii_config.detect_credit_card;
        
        // Map redaction strategy string to enum
        if (config_.pii_config.redaction_strategy == "hash") {
            pii_config.default_strategy = PIIDetector::RedactionStrategy::HASH;
        } else if (config_.pii_config.redaction_strategy == "remove") {
            pii_config.default_strategy = PIIDetector::RedactionStrategy::REMOVE;
        } else if (config_.pii_config.redaction_strategy == "partial") {
            pii_config.default_strategy = PIIDetector::RedactionStrategy::PARTIAL;
        } else {
            pii_config.default_strategy = PIIDetector::RedactionStrategy::MASK;
        }
        
        pii_detector = std::make_unique<PIIDetector>(pii_config);
    }
    
    try {
        // P2: Use StreamWriter for compression and streaming
        StreamWriter::Config writer_config;
        writer_config.output_path = options.output_path;
        writer_config.buffer_size = options.buffer_size_bytes;
        writer_config.max_file_size = options.max_file_size_bytes;
        
        if (options.compress) {
            if (options.compression_type == "gzip") {
                writer_config.compression = CompressionType::GZIP;
            } else if (options.compression_type == "zstd") {
                // ZSTD not yet implemented
                throw ConfigException(
                    "ZSTD compression not yet supported",
                    "compression_type"
                );
            }
            writer_config.compression_level = options.compression_level;
        }
        
        StreamWriter writer(writer_config);
        
        std::set<std::string> seen_hashes;  // For duplicate detection
        
        for (const auto& entity : entities) {
            stats.total_entities++;
            
            // P2: Check size limit
            if (writer.isLimitReached()) {
                THEMIS_WARN("Export size limit reached, stopping at {} entities", stats.total_entities);
                break;
            }
            
            try {
                // P1: Tenant isolation - check entity belongs to tenant
                if (options.tenant_context && options.tenant_context->enforce_isolation) {
                    auto tenant_field = entity.getFieldAsString("tenant_id");
                    if (tenant_field && *tenant_field != options.tenant_context->tenant_id) {
                        metrics_->recordError("tenant_isolation_violation");
                        continue;  // Skip entity from different tenant
                    }
                }
                
                // Quality filtering
                if (!passesQualityFilter(entity)) {
                    metrics_->recordQualityFilterRejection("quality_filter_failed");
                    continue;
                }
                
                // Calculate weight
                double weight = calculateWeight(entity);
                
                // Format based on style
                std::string line;
                switch (config_.style) {
                    case JSONLFormat::Style::INSTRUCTION_TUNING:
                        line = formatInstructionTuning(entity, weight);
                        break;
                    case JSONLFormat::Style::CHAT_COMPLETION:
                        line = formatChatCompletion(entity, weight);
                        break;
                    case JSONLFormat::Style::TEXT_COMPLETION:
                        line = formatTextCompletion(entity, weight);
                        break;
                    default:
                        line = formatInstructionTuning(entity, weight);
                }
                
                if (line.empty()) {
                    metrics_->recordQualityFilterRejection("empty_formatted_line");
                    continue;
                }
                
                // P1: PII detection and redaction
                if (pii_detector) {
                    if (pii_detector->containsPII(line)) {
                        metrics_->recordPIIDetection();
                        
                        if (config_.pii_config.fail_on_pii && !config_.pii_config.enable_redaction) {
                            throw ExporterException(
                                themis::errors::ErrorCode::ERR_EXPORT_PII_VIOLATION,
                                "PII detected in export data without redaction",
                                "entity_id=" + entity.getPrimaryKey()
                            );
                        }
                        
                        if (config_.pii_config.enable_redaction) {
                            line = pii_detector->redactPII(line);
                            metrics_->recordPIIRedaction();
                        }
                    }
                }
                
                // Schema validation (Outlines open-source integration)
                if (config_.structured_gen.enable_schema_validation) {
                    std::string validation_error;
                    bool validation_passed = validateAgainstSchema(line, &validation_error);
                    metrics_->recordSchemaValidation(validation_passed);
                    
                    if (!validation_passed) {
                        if (config_.structured_gen.reject_invalid_samples) {
                            stats.failed_entities++;
                            stats.errors.push_back(
                                "Schema validation failed for " + entity.getPrimaryKey() + 
                                ": " + validation_error
                            );
                            metrics_->recordError("schema_validation_failed");
                            continue;  // Skip this sample
                        }
                        // Otherwise, log but continue
                        THEMIS_WARN("Schema validation warning for {}: {}", 
                                   entity.getPrimaryKey(), validation_error);
                    }
                    
                    // Add schema to output if requested (for Outlines)
                    if (config_.structured_gen.include_schema_in_output) {
                        try {
                            auto j = json::parse(line);
                            j["__schema__"] = json::parse(config_.structured_gen.json_schema);
                            line = j.dump();
                        } catch (const std::exception& e) {
                            THEMIS_WARN("Failed to add schema to output: {}", e.what());
                        }
                    }
                }
                
                // Track quality metrics
                if (config_.quality_metrics.enable_metrics) {
                    // Track length distribution
                    if (config_.quality_metrics.track_length_distribution) {
                        constexpr size_t BUCKET_SIZE = 100;  // 100-char buckets
                        size_t bucket = (line.size() / BUCKET_SIZE) * BUCKET_SIZE;
                        runtime_metrics_.length_distribution[bucket]++;
                    }
                }
                
                // Duplicate detection
                if (config_.quality.skip_duplicates) {
                    std::hash<std::string> hasher;
                    auto hash = std::to_string(hasher(line));
                    if (seen_hashes.count(hash)) {
                        metrics_->recordDuplicate();
                        continue;
                    }
                    seen_hashes.insert(hash);
                }
                
                // P2: Write using StreamWriter (handles compression)
                line += "\n";
                writer.write(line);
                stats.bytes_written += line.size();
                stats.exported_entities++;
                
                // Progress reporting
                if (options.progress_callback && 
                    stats.exported_entities % options.progress_interval == 0) {
                    options.progress_callback(stats);
                }
                
            } catch (const ExporterException& e) {
                stats.failed_entities++;
                std::string error_msg = "Entity " + entity.getPrimaryKey() + 
                                      ": [" + std::to_string(static_cast<int>(e.getErrorCode())) + 
                                      "] " + e.what();
                stats.errors.push_back(error_msg);
                metrics_->recordError("exporter_exception");
                
                if (stats.errors.size() >= options.max_errors) {
                    THEMIS_ERROR("Max errors reached, stopping export");
                    break;
                }
                
                if (!options.continue_on_error) {
                    throw;
                }
            } catch (const std::exception& e) {
                stats.failed_entities++;
                stats.errors.push_back(
                    "Entity " + entity.getPrimaryKey() + ": " + e.what()
                );
                metrics_->recordError("generic_exception");
                
                if (stats.errors.size() >= options.max_errors) {
                    THEMIS_ERROR("Max errors reached, stopping export");
                    break;
                }
                
                if (!options.continue_on_error) {
                    throw;
                }
            }
        }
        
        // Flush and close writer
        writer.close();
        
        auto end_time = std::chrono::steady_clock::now();
        stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );
        
        // P2: Record compression metrics
        if (options.compress) {
            metrics_->recordCompression(writer.getBytesWritten(), 
                                       writer.getCompressedBytesWritten());
        }
        
        // Record export metrics
        metrics_->recordExport(stats.exported_entities, stats.bytes_written, stats.duration);
        
        THEMIS_INFO("JSONL export completed: {} entities in {}ms{}",
                    stats.exported_entities, stats.duration.count(),
                    options.compress ? " (compressed)" : "");
        
        return stats;
        
    } catch (const ExportIOException& e) {
        stats.errors.push_back(
            "[" + std::to_string(static_cast<int>(e.getErrorCode())) + "] " + 
            e.what() + " (file: " + e.getFilePath() + ")"
        );
        metrics_->recordError("io_exception");
        
        auto end_time = std::chrono::steady_clock::now();
        stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );
        
        return stats;
    }
}

std::string JSONLLLMExporter::formatInstructionTuning(
    const BaseEntity& entity,
    double& weight
) {
    json j;
    
    auto& mapping = config_.field_mapping;
    
    // Required fields
    auto instruction = entity.getFieldAsString(mapping.instruction_field);
    auto output = entity.getFieldAsString(mapping.output_field);
    
    if (!instruction || !output) {
        return "";  // Skip if missing required fields
    }
    
    j["instruction"] = *instruction;
    j["output"] = *output;
    
    // Optional input field
    auto input = entity.getFieldAsString(mapping.input_field);
    if (input && !input->empty()) {
        j["input"] = *input;
    }
    
    // Add weight
    if (config_.weighting.enable_weights) {
        j["weight"] = weight;
    }
    
    // Add metadata
    if (config_.include_metadata) {
        auto metadata_str = extractMetadata(entity);
        if (!metadata_str.empty()) {
            j["metadata"] = json::parse(metadata_str);
        }
    }
    
    return j.dump();
}

std::string JSONLLLMExporter::formatChatCompletion(
    const BaseEntity& entity,
    double& weight
) {
    json j;
    auto& mapping = config_.field_mapping;
    
    json messages = json::array();
    
    // System message (optional)
    auto system = entity.getFieldAsString(mapping.system_field);
    if (system && !system->empty()) {
        messages.push_back({
            {"role", "system"},
            {"content", *system}
        });
    }
    
    // User message (required)
    auto user = entity.getFieldAsString(mapping.user_field);
    if (!user) {
        return "";
    }
    
    messages.push_back({
        {"role", "user"},
        {"content", *user}
    });
    
    // Assistant response (required)
    auto assistant = entity.getFieldAsString(mapping.assistant_field);
    if (!assistant) {
        return "";
    }
    
    messages.push_back({
        {"role", "assistant"},
        {"content", *assistant}
    });
    
    j["messages"] = messages;
    
    // Add weight
    if (config_.weighting.enable_weights) {
        j["weight"] = weight;
    }
    
    // Add metadata
    if (config_.include_metadata) {
        auto metadata_str = extractMetadata(entity);
        if (!metadata_str.empty()) {
            j["metadata"] = json::parse(metadata_str);
        }
    }
    
    return j.dump();
}

std::string JSONLLLMExporter::formatTextCompletion(
    const BaseEntity& entity,
    double& weight
) {
    json j;
    auto& mapping = config_.field_mapping;
    
    auto text = entity.getFieldAsString(mapping.text_field);
    if (!text) {
        return "";
    }
    
    j["text"] = *text;
    
    // Add weight
    if (config_.weighting.enable_weights) {
        j["weight"] = weight;
    }
    
    // Add metadata
    if (config_.include_metadata) {
        auto metadata_str = extractMetadata(entity);
        if (!metadata_str.empty()) {
            j["metadata"] = json::parse(metadata_str);
        }
    }
    
    return j.dump();
}

double JSONLLLMExporter::calculateWeight(const BaseEntity& entity) {
    auto& weight_cfg = config_.weighting;
    
    // Check for explicit weight field
    if (entity.hasField(weight_cfg.weight_field)) {
        auto weight = entity.getFieldAsDouble(weight_cfg.weight_field);
        if (weight) {
            return std::clamp(*weight, 0.0, 1.0);
        }
    }
    
    double calculated_weight = weight_cfg.default_weight;
    
    // Auto-weight by length
    if (weight_cfg.auto_weight_by_length) {
        auto output = entity.getFieldAsString(config_.field_mapping.output_field);
        if (output) {
            size_t length = output->size();
            // Longer responses get slightly higher weight (up to 1.5x)
            calculated_weight *= (1.0 + std::min(0.5, length / 2000.0));
        }
    }
    
    // Auto-weight by freshness
    if (weight_cfg.auto_weight_by_freshness) {
        auto timestamp_str = entity.getFieldAsString(weight_cfg.timestamp_field);
        if (timestamp_str) {
            try {
                // Parse timestamp (assuming Unix timestamp in milliseconds or seconds)
                int64_t timestamp_value = std::stoll(*timestamp_str);
                
                // Determine if timestamp is in seconds or milliseconds
                // Timestamps > 10^10 are likely in milliseconds
                auto timestamp_ms = (timestamp_value > 10000000000LL) ? 
                    timestamp_value : timestamp_value * 1000;
                
                auto timestamp_tp = std::chrono::system_clock::time_point(
                    std::chrono::milliseconds(timestamp_ms)
                );
                
                auto now = std::chrono::system_clock::now();
                auto age = std::chrono::duration_cast<std::chrono::hours>(
                    now - timestamp_tp
                ).count();
                
                // Validate timestamp is not in the future
                if (age < 0) {
                    THEMIS_WARN("Timestamp '{}' is in the future, skipping freshness calculation", 
                               *timestamp_str);
                    // Skip freshness calculation for future timestamps
                } else {
                    // Calculate freshness factor using exponential decay
                    // Age in days: divide hours by 24
                    double age_days = age / 24.0;
                    
                    // Decay factor: newer data gets weight closer to 1.0
                    // Data older than 365 days gets significantly reduced weight
                    // Using formula: freshness = exp(-age_days / decay_constant)
                    // where decay_constant = 180 (half-life of ~6 months)
                    double freshness_factor = std::exp(-age_days / 180.0);
                    
                    // Apply freshness factor (multiply by 0.5 to 1.0 range)
                    // Very fresh data (< 1 week): ~1.0x
                    // 6 month old data: ~0.5x
                    // 1+ year old data: ~0.25x
                    calculated_weight *= (0.5 + 0.5 * freshness_factor);
                }
                
            } catch (const std::exception& e) {
                // If timestamp parsing fails, log warning and use default weight
                THEMIS_WARN("Failed to parse timestamp '{}' for freshness calculation: {}", 
                           *timestamp_str, e.what());
            }
        }
    }
    
    return std::clamp(calculated_weight, 0.0, 2.0);
}

bool JSONLLLMExporter::passesQualityFilter(const BaseEntity& entity) {
    auto& quality = config_.quality;
    
    // Get output field based on style
    std::string output_field;
    switch (config_.style) {
        case JSONLFormat::Style::INSTRUCTION_TUNING:
            output_field = config_.field_mapping.output_field;
            break;
        case JSONLFormat::Style::CHAT_COMPLETION:
            output_field = config_.field_mapping.assistant_field;
            break;
        case JSONLFormat::Style::TEXT_COMPLETION:
            output_field = config_.field_mapping.text_field;
            break;
        default:
            output_field = config_.field_mapping.output_field;
    }
    
    auto output = entity.getFieldAsString(output_field);
    
    // Skip empty outputs
    if (quality.skip_empty_outputs && (!output || output->empty())) {
        return false;
    }
    
    // Check length constraints
    if (output) {
        size_t length = output->size();
        if (length < quality.min_text_length || length > quality.max_text_length) {
            return false;
        }
    }
    
    return true;
}

std::string JSONLLLMExporter::extractMetadata(const BaseEntity& entity) {
    json metadata;
    
    for (const auto& field_name : config_.metadata_fields) {
        if (entity.hasField(field_name)) {
            auto value = entity.getFieldAsString(field_name);
            if (value) {
                metadata[field_name] = *value;
            }
        }
    }
    
    if (metadata.empty()) {
        return "";
    }
    
    return metadata.dump();
}

// ============================================================================
// Outlines (Open-Source) Integration - Schema Validation
// ============================================================================

bool JSONLLLMExporter::validateAgainstSchema(const std::string& json_str, std::string* error) const {
    if (!config_.structured_gen.enable_schema_validation) {
        return true;  // Validation disabled
    }
    
    if (config_.structured_gen.json_schema.empty()) {
        if (error) *error = "Schema validation enabled but no schema provided";
        return false;
    }
    
    return validateJsonSchema(json_str, config_.structured_gen.json_schema, error);
}

bool JSONLLLMExporter::validateJsonSchema(
    const std::string& json_str,
    const std::string& schema,
    std::string* error
) const {
    try {
        auto data = json::parse(json_str);
        auto schema_json = json::parse(schema);
        
        // Basic JSON schema validation
        // For production, integrate nlohmann/json-schema-validator or similar
        // This is a simplified version for demonstration
        
        runtime_metrics_.total_validated++;
        
        if (schema_json.contains("required")) {
            for (const auto& required_field : schema_json["required"]) {
                std::string field = required_field.get<std::string>();
                if (!data.contains(field)) {
                    runtime_metrics_.schema_violations++;
                    std::string err_msg = "Missing required field: " + field;
                    if (error) {
                        *error = err_msg;
                    }
                    if (config_.structured_gen.log_validation_errors) {
                        runtime_metrics_.validation_errors.push_back(err_msg);
                    }
                    return false;
                }
            }
        }
        
        runtime_metrics_.schema_compliant++;
        return true;
        
    } catch (const std::exception& e) {
        runtime_metrics_.schema_violations++;
        if (error) {
            *error = std::string("Schema validation error: ") + e.what();
        }
        if (config_.structured_gen.log_validation_errors) {
            runtime_metrics_.validation_errors.push_back(*error);
        }
        return false;
    }
}

// ============================================================================
// LoRA Adapter Metadata (LoRAExchange.ai compatibility)
// ============================================================================

std::string JSONLLLMExporter::getAdapterMetadataJson() const {
    if (!config_.adapter_metadata.enable_tracking) {
        return "{}";
    }
    
    auto& meta = config_.adapter_metadata;
    json j;
    
    // Core metadata
    j["adapter_id"] = meta.adapter_id;
    j["adapter_version"] = meta.adapter_version;
    j["base_model"] = {
        {"name", meta.base_model_name},
        {"version", meta.base_model_version}
    };
    
    // Task specification
    j["task"] = {
        {"type", meta.task_type},
        {"domain", meta.domain},
        {"language", meta.language}
    };
    
    // Training configuration
    auto& train = meta.training_config;
    j["training"] = {
        {"dataset_name", train.dataset_name},
        {"num_samples", train.num_samples},
        {"epochs", train.epochs},
        {"learning_rate", train.learning_rate},
        {"lora_rank", train.lora_rank},
        {"lora_alpha", train.lora_alpha},
        {"lora_dropout", train.lora_dropout},
        {"target_modules", train.target_modules}
    };
    
    // Provenance
    j["provenance"] = {
        {"created_by", meta.created_by},
        {"data_source_uri", meta.data_source_uri},
        {"parent_adapter_id", meta.parent_adapter_id}
    };
    
    // Custom metadata
    if (!meta.custom_metadata.empty()) {
        j["custom"] = meta.custom_metadata;
    }
    
    // vLLM-specific configuration
    if (meta.vllm_config.enabled) {
        j["vllm"] = {
            {"enabled", true},
            {"adapter_path", meta.vllm_config.adapter_path},
            {"vllm_version", meta.vllm_config.vllm_version},
            {"max_lora_rank", meta.vllm_config.max_lora_rank},
            {"enable_multi_lora", meta.vllm_config.enable_multi_lora}
        };
        
        if (!meta.vllm_config.serving_config.empty()) {
            j["vllm"]["serving_config"] = meta.vllm_config.serving_config;
        }
    }
    
    // Add export timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time_t));
    j["exported_at"] = buf;
    
    return j.dump(2);
}

bool JSONLLLMExporter::setAdapterMetadataFromJson(const std::string& json_str, std::string* error) {
    try {
        auto j = json::parse(json_str);
        auto& meta = config_.adapter_metadata;
        
        meta.enable_tracking = true;
        
        if (j.contains("adapter_id")) meta.adapter_id = j["adapter_id"];
        if (j.contains("adapter_version")) meta.adapter_version = j["adapter_version"];
        
        if (j.contains("base_model")) {
            if (j["base_model"].contains("name")) 
                meta.base_model_name = j["base_model"]["name"];
            if (j["base_model"].contains("version")) 
                meta.base_model_version = j["base_model"]["version"];
        }
        
        if (j.contains("task")) {
            if (j["task"].contains("type")) meta.task_type = j["task"]["type"];
            if (j["task"].contains("domain")) meta.domain = j["task"]["domain"];
            if (j["task"].contains("language")) meta.language = j["task"]["language"];
        }
        
        if (j.contains("training")) {
            auto& train_json = j["training"];
            auto& train = meta.training_config;
            
            if (train_json.contains("dataset_name")) 
                train.dataset_name = train_json["dataset_name"];
            if (train_json.contains("num_samples")) 
                train.num_samples = train_json["num_samples"];
            if (train_json.contains("epochs")) 
                train.epochs = train_json["epochs"];
            if (train_json.contains("learning_rate")) 
                train.learning_rate = train_json["learning_rate"];
            if (train_json.contains("lora_rank")) 
                train.lora_rank = train_json["lora_rank"];
            if (train_json.contains("lora_alpha")) 
                train.lora_alpha = train_json["lora_alpha"];
            if (train_json.contains("lora_dropout")) 
                train.lora_dropout = train_json["lora_dropout"];
            if (train_json.contains("target_modules")) 
                train.target_modules = train_json["target_modules"].get<std::vector<std::string>>();
        }
        
        if (j.contains("provenance")) {
            auto& prov = j["provenance"];
            if (prov.contains("created_by")) meta.created_by = prov["created_by"];
            if (prov.contains("data_source_uri")) meta.data_source_uri = prov["data_source_uri"];
            if (prov.contains("parent_adapter_id")) meta.parent_adapter_id = prov["parent_adapter_id"];
        }
        
        if (j.contains("custom")) {
            meta.custom_metadata = j["custom"].get<std::map<std::string, std::string>>();
        }
        
        // vLLM configuration
        if (j.contains("vllm")) {
            auto& vllm_json = j["vllm"];
            auto& vllm = meta.vllm_config;
            
            if (vllm_json.contains("enabled")) vllm.enabled = vllm_json["enabled"];
            if (vllm_json.contains("adapter_path")) vllm.adapter_path = vllm_json["adapter_path"];
            if (vllm_json.contains("vllm_version")) vllm.vllm_version = vllm_json["vllm_version"];
            if (vllm_json.contains("max_lora_rank")) vllm.max_lora_rank = vllm_json["max_lora_rank"];
            if (vllm_json.contains("enable_multi_lora")) vllm.enable_multi_lora = vllm_json["enable_multi_lora"];
            if (vllm_json.contains("serving_config")) {
                vllm.serving_config = vllm_json["serving_config"].get<std::map<std::string, std::string>>();
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (error) {
            *error = std::string("Failed to parse adapter metadata: ") + e.what();
        }
        return false;
    }
}

// ============================================================================
// Quality Metrics Reporting
// ============================================================================

std::string JSONLLLMExporter::getQualityMetricsReport() const {
    if (!config_.quality_metrics.enable_metrics) {
        return "{}";
    }
    
    json j;
    
    // Schema compliance metrics
    if (config_.quality_metrics.track_schema_compliance && 
        config_.structured_gen.enable_schema_validation) {
        j["schema_validation"] = {
            {"total_validated", runtime_metrics_.total_validated},
            {"compliant", runtime_metrics_.schema_compliant},
            {"violations", runtime_metrics_.schema_violations},
            {"compliance_rate", runtime_metrics_.total_validated > 0 
                ? (double)runtime_metrics_.schema_compliant / runtime_metrics_.total_validated 
                : 0.0}
        };
        
        if (!runtime_metrics_.validation_errors.empty() && 
            config_.structured_gen.log_validation_errors) {
            // Show only last 10 errors to avoid huge reports
            size_t start = runtime_metrics_.validation_errors.size() > 10 
                ? runtime_metrics_.validation_errors.size() - 10 
                : 0;
            j["schema_validation"]["recent_errors"] = json::array();
            for (size_t i = start; i < runtime_metrics_.validation_errors.size(); ++i) {
                j["schema_validation"]["recent_errors"].push_back(
                    runtime_metrics_.validation_errors[i]
                );
            }
        }
    }
    
    // Length distribution
    if (config_.quality_metrics.track_length_distribution) {
        j["length_distribution"] = runtime_metrics_.length_distribution;
    }
    
    // Diversity score
    if (config_.quality_metrics.track_diversity_score) {
        j["diversity_score"] = runtime_metrics_.diversity_score;
    }
    
    return j.dump(2);
}

} // namespace themis::exporters
