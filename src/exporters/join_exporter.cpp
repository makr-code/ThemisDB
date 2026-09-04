/**
 * @file join_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/join_exporter.h"
#include "exporters/aql_predicate_filter.h"
#include "exporters/exporter_errors.h"
#include "exporters/stream_writer.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <sstream>

using json = nlohmann::json;

namespace themis::exporters {

// ── Construction ──────────────────────────────────────────────────────────────

JoinExporter::JoinExporter(const JoinExportConfig& config)
    : config_(config),
      metrics_(std::make_shared<ExporterMetrics>()) {}

// ── setRightCollection ────────────────────────────────────────────────────────

void JoinExporter::setRightCollection(const std::vector<BaseEntity>& right_entities) {
    if (config_.right_collection.empty()) {
        throw ExporterException(
            errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID,
            "JoinExporter: right_collection name must not be empty",
            "config_key=right_collection"
        );
    }

    right_table_.clear();
    right_table_bytes_ = 0;

    for (const auto& entity : right_entities) {
        const std::string key = entity.getFieldString(config_.right_key_field);
        if (key.empty()) {
            // Skip entities whose join-key field is absent/empty.
            continue;
        }

        const size_t entity_bytes = estimateEntityBytes(entity);

        if (config_.right_side_memory_limit_bytes > 0 &&
            right_table_bytes_ + entity_bytes > config_.right_side_memory_limit_bytes) {
            throw ExporterException(
                errors::ErrorCode::ERR_EXPORT_JOIN_MEMORY_LIMIT,
                "JoinExporter: right-side hash table exceeded memory budget of " +
                    std::to_string(config_.right_side_memory_limit_bytes) + " bytes",
                "right_collection=" + config_.right_collection
            );
        }

        right_table_.emplace(key, entity);
        right_table_bytes_ += entity_bytes;
    }

    THEMIS_INFO("JoinExporter: loaded {} right-side rows ({} bytes) from '{}'",
                right_table_.size(), right_table_bytes_, config_.right_collection);
    right_collection_loaded_ = true;
}

// ── exportEntities ────────────────────────────────────────────────────────────

ExportStats JoinExporter::exportEntities(
    const std::vector<BaseEntity>& entities,
    const ExportOptions& options
) {
    // Policy check before any cursor or file is opened.
    enforceExportPolicy(options);

    // Validate required config.
    if (config_.left_collection.empty()) {
        throw ExporterException(
            errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID,
            "JoinExporter: left_collection name must not be empty",
            "config_key=left_collection"
        );
    }
    if (config_.right_collection.empty()) {
        throw ExporterException(
            errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID,
            "JoinExporter: right_collection name must not be empty",
            "config_key=right_collection"
        );
    }
    if (!right_collection_loaded_) {
        throw ExporterException(
            errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID,
            "JoinExporter: setRightCollection() must be called before exportEntities()",
            "config_key=right_collection_loaded"
        );
    }

    ExportStats stats;
    stats.metrics = metrics_;
    auto start_time = std::chrono::steady_clock::now();

    // Build the optional AQL predicate filter (compiled once, reused per row).
    std::unique_ptr<AqlPredicateFilter> aql_filter;
    std::unique_ptr<AqlPredicateFilter> options_filter = {};

    if (!config_.join_predicate.empty()) {
        try {
            aql_filter = std::make_unique<AqlPredicateFilter>(config_.join_predicate);
        } catch (const AqlPredicateFilterException& e) {
            throw ExporterException(
                errors::ErrorCode::ERR_EXPORT_JOIN_PREDICATE_INVALID,
                std::string("JoinExporter: join_predicate parse failed: ") + e.what(),
                "join_predicate=" + config_.join_predicate
            );
        }
    }

    if (!options.filter_expression.empty()) {
        try {
            options_filter = std::make_unique<AqlPredicateFilter>(options.filter_expression);
        } catch (const AqlPredicateFilterException& e) {
            throw ExporterException(
                errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID,
                std::string("JoinExporter: filter_expression parse failed: ") + e.what(),
                "filter_expression=" + options.filter_expression
            );
        }
    }

    // Build optional PII detector.
    auto pii_detector = buildPIIDetector();

    // Open StreamWriter for output.
    StreamWriter::Config writer_config;
    writer_config.output_path   = options.output_path;
    writer_config.buffer_size   = options.buffer_size_bytes;
    writer_config.max_file_size = options.max_file_size_bytes;

    if (options.compress) {
        if (options.compression_type == "gzip" || options.compression_type == "zstd") {
            writer_config.compression = CompressionType::ZSTD;
        }
        writer_config.compression_level = options.compression_level;
    }

    StreamWriter writer(writer_config);

    const size_t total_count = entities.size();

    for (const auto& left_entity : entities) {
        stats.total_entities++;

        // Check output size limit.
        if (options.max_file_size_bytes > 0 &&
            stats.bytes_written >= options.max_file_size_bytes) {
            THEMIS_WARN("JoinExporter: output size limit reached at {} entities",
                        stats.total_entities);
            break;
        }

        try {
            // Probe hash table (inner join: skip unmatched left rows).
            const std::string left_key = left_entity.getFieldString(config_.left_key_field);
            auto it = right_table_.find(left_key);
            if (it == right_table_.end()) {
                stats.skipped_entities++;
                continue;
            }

            // Merge left and right into a single entity.
            BaseEntity merged = mergeEntities(left_entity, it->second);

            // Apply AQL join_predicate on the merged record.
            if (aql_filter && !aql_filter->evaluate(merged)) {
                stats.skipped_entities++;
                continue;
            }
            if (options_filter && !options_filter->evaluate(merged)) {
                stats.skipped_entities++;
                continue;
            }

            // Serialize merged entity to JSON.
            std::string line = merged.toJson();

            // PII detection and redaction on the serialized JSON string.
            if (pii_detector) {
                if (pii_detector->containsPII(line)) {
                    metrics_->recordPIIDetection();

                    if (config_.pii_config.fail_on_pii && !config_.pii_config.enable_redaction) {
                        throw ExporterException(
                            errors::ErrorCode::ERR_EXPORT_PII_VIOLATION,
                            "PII detected in join export data without redaction",
                            "entity_id=" + merged.getPrimaryKey()
                        );
                    }

                    if (config_.pii_config.enable_redaction) {
                        line = pii_detector->redactPII(line);
                        metrics_->recordPIIRedaction();
                    }
                }
            }

            // Write JSONL line.
            line += "\n";
            writer.write(line);
            stats.bytes_written    += line.size();
            stats.exported_entities++;

            // Progress callbacks.
            if (options.progress_callback &&
                stats.exported_entities % options.progress_interval == 0) {
                auto now = std::chrono::steady_clock::now();
                stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - start_time);
                if (total_count > 0 && stats.exported_entities < total_count) {
                    double elapsed = std::chrono::duration<double>(now - start_time).count();
                    double rate    = static_cast<double>(stats.exported_entities) / elapsed;
                    stats.estimated_eta_seconds =
                        static_cast<double>(total_count - stats.exported_entities) / rate;
                }
                options.progress_callback([[maybe_unused]] stats);
            }

        } catch (const ExporterException& e) {
            stats.failed_entities++;
            stats.errors.push_back(
                "Entity " + left_entity.getPrimaryKey() + ": [" +
                std::to_string(static_cast<int>(e.getErrorCode())) + "] " + e.what()
            );
            metrics_->recordError("exporter_exception");

            if (stats.errors.size() >= options.max_errors) {
                THEMIS_ERROR("JoinExporter: max errors reached, stopping export");
                break;
            }
            if (!options.continue_on_error) {
                throw;
            }
        } catch (const std::exception& e) {
            stats.failed_entities++;
            stats.errors.push_back(
                "Entity " + left_entity.getPrimaryKey() + ": " + e.what()
            );
            metrics_->recordError("generic_exception");

            if (stats.errors.size() >= options.max_errors) {
                THEMIS_ERROR("JoinExporter: max errors reached, stopping export");
                break;
            }
            if (!options.continue_on_error) {
                throw;
            }
        }
    }

    writer.flush();

    auto end_time  = std::chrono::steady_clock::now();
    stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    metrics_->recordExport(stats.exported_entities, stats.bytes_written, stats.duration);

    THEMIS_INFO("JoinExporter: exported {} / {} rows ({} bytes, {} ms)",
                stats.exported_entities, stats.total_entities,
                stats.bytes_written, stats.duration.count());

    return stats;
}

// ── mergeEntities ─────────────────────────────────────────────────────────────

BaseEntity JoinExporter::mergeEntities(
    const BaseEntity& left,
    const BaseEntity& right
) const {
    BaseEntity::FieldMap merged_fields;

    // Compute prefix-qualified field maps for both sides.
    const auto left_fields  = left.getAllFields();
    const auto right_fields = right.getAllFields();

    // Build a set of field names present in both (for ambiguity detection).
    std::set<std::string> common_names = {};

    for (const auto& [name, _] : left_fields) {
        if (right_fields.count(name)) {
            common_names.insert(name);
        }
    }

    if (config_.output_fields.empty()) {
        // No field selection: include all fields from both sides.
        // Prefix ambiguous fields with the collection name to avoid collisions.
        for (const auto& [name, val] : left_fields) {
            if (common_names.count(name)) {
                throw ExporterException(
                    errors::ErrorCode::ERR_EXPORT_JOIN_AMBIGUOUS_FIELD,
                    "JoinExporter: field '" + name +
                        "' exists in both collections — use output_fields aliases to disambiguate",
                    "field=" + name
                );
            }
            merged_fields[name] = val;
        }
        for (const auto& [name, val] : right_fields) {
            merged_fields[name] = val;
        }
    } else {
        // Honour explicit output_fields list (with optional "src_name:alias" syntax).
        // Qualified source names "left.<field>" / "right.<field>" are used to resolve
        // ambiguous names that exist in both collections.
        for (const auto& spec : config_.output_fields) {
            // Split "original:alias"
            const auto colon_pos = spec.find(':');
            const std::string src_name  = (colon_pos != std::string::npos)
                                              ? spec.substr(0, colon_pos)
                                              : spec;
            const std::string out_name  = (colon_pos != std::string::npos)
                                              ? spec.substr(colon_pos + 1)
                                              : spec;

            // Resolve "left.<field>" / "right.<field>" qualifiers.
            if (src_name.rfind("left.", 0) == 0) {
                const std::string field = src_name.substr(5);
                auto it = left_fields.find(field);
                if (it != left_fields.end()) {
                    merged_fields[out_name] = it->second;
                }
            } else if (src_name.rfind("right.", 0) == 0) {
                const std::string field = src_name.substr(6);
                auto it = right_fields.find(field);
                if (it != right_fields.end()) {
                    merged_fields[out_name] = it->second;
                }
            } else {
                // Unqualified name: check for ambiguity.
                const bool in_left  = (left_fields.count(src_name) > 0);
                const bool in_right = (right_fields.count(src_name) > 0);

                if (in_left && in_right) {
                    throw ExporterException(
                        errors::ErrorCode::ERR_EXPORT_JOIN_AMBIGUOUS_FIELD,
                        "JoinExporter: field '" + src_name +
                            "' exists in both collections — use 'left." + src_name +
                            "' or 'right." + src_name + "' to disambiguate",
                        "field=" + src_name
                    );
                }

                if (in_left) {
                    merged_fields[out_name] = left_fields.at(src_name);
                } else if (in_right) {
                    merged_fields[out_name] = right_fields.at(src_name);
                }
                // If the field is absent from both sides, silently skip it.
            }
        }
    }

    // Primary key of the merged entity is taken from the left side.
    return BaseEntity::fromFields(left.getPrimaryKey(), merged_fields);
}

// ── buildPIIDetector ──────────────────────────────────────────────────────────

std::unique_ptr<PIIDetector> JoinExporter::buildPIIDetector() const {
    if (!config_.pii_config.enable_detection) {
        return nullptr;
    }

    PIIDetector::Config pc;
    pc.detect_email       = config_.pii_config.detect_email;
    pc.detect_phone       = config_.pii_config.detect_phone;
    pc.detect_ssn         = config_.pii_config.detect_ssn;
    pc.detect_credit_card = config_.pii_config.detect_credit_card;

    if (config_.pii_config.redaction_strategy == "hash") {
        pc.default_strategy = PIIDetector::RedactionStrategy::HASH;
    } else if (config_.pii_config.redaction_strategy == "remove") {
        pc.default_strategy = PIIDetector::RedactionStrategy::REMOVE;
    } else if (config_.pii_config.redaction_strategy == "partial") {
        pc.default_strategy = PIIDetector::RedactionStrategy::PARTIAL;
    } else {
        pc.default_strategy = PIIDetector::RedactionStrategy::MASK;
    }

    return std::make_unique<PIIDetector>(pc);
}

// ── estimateEntityBytes ───────────────────────────────────────────────────────

size_t JoinExporter::estimateEntityBytes(const BaseEntity& entity) {
    // Use serialised JSON length as a conservative proxy for heap usage.
    const std::string serialised = entity.toJson();
    // Add primary key + per-entry overhead.
    return static_cast<int>(serialised.size()) + entity.getPrimaryKey().size() + 64;
}

} // namespace themis::exporters
