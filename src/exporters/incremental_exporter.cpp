/**
 * @file incremental_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/incremental_exporter.h"
#include "exporters/aql_predicate_filter.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_interface.h"
#include "exporters/export_encryption.h"
#include "exporters/stream_writer.h"
#include "utils/logger.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

using json = nlohmann::json;

namespace themis::exporters {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

IncrementalExporter::IncrementalExporter(const IncrementalExportConfig& config)
    : config_(config), metrics_(std::make_shared<ExporterMetrics>()) {}

// ─────────────────────────────────────────────────────────────────────────────
// IExporter::exportEntities
// ─────────────────────────────────────────────────────────────────────────────

ExportStats IncrementalExporter::exportEntities(
    const std::vector<BaseEntity>& entities,
    const ExportOptions& options
) {
    // Policy check before any cursor or file is opened (EXP-001).
    enforceExportPolicy(options);

    ExportStats stats;
    stats.metrics = metrics_;
    const auto start_time = std::chrono::steady_clock::now();

    // --- Step 1: read the watermark ---
    const int64_t watermark = readWatermark();
    THEMIS_INFO("IncrementalExporter: starting delta export, watermark={}", watermark);

    // --- Step 2: configure output writer ---
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

    int64_t max_sequence = watermark;  // track highest sequence in this run
    bool limit_reached = false;
    bool full_scan_completed = true;

    try {
        StreamWriter writer(writer_config);
        std::unique_ptr<AqlPredicateFilter> aql_filter;
        if (!options.filter_expression.empty()) {
            try {
                aql_filter = std::make_unique<AqlPredicateFilter>(options.filter_expression);
            } catch (const AqlPredicateFilterException& e) {
                throw ExporterException(
                    errors::ErrorCode::ERR_EXPORT_CONFIG_INVALID,
                    std::string("IncrementalExporter: invalid filter_expression: ") + e.what(),
                    "filter_expression=" + options.filter_expression
                );
            }
        }

        for (const auto& entity : entities) {
            stats.total_entities++;

            // --- Step 3: extract sequence and filter ---
            const int64_t seq = extractSequence(entity);

            if (seq == std::numeric_limits<int64_t>::min()) {
                // Field absent or unparseable
                if (!config_.export_missing_sequence) {
                    stats.skipped_entities++;
                    metrics_->recordDeltaDocSkipped();
                    continue;
                }
                // fail-open: export entities without a sequence field
            } else if (seq <= watermark) {
                stats.skipped_entities++;
                metrics_->recordDeltaDocSkipped();
                continue;
            }

            if (aql_filter && !aql_filter->evaluate(entity)) {
                stats.skipped_entities++;
                metrics_->recordQualityFilterRejection("aql_predicate_filtered");
                continue;
            }

            // --- Step 4: write the entity ---
            try {
                if (writer.isLimitReached()) {
                    THEMIS_WARN("IncrementalExporter: size limit reached after {} entities",
                                stats.exported_entities);
                    limit_reached = true;
                    full_scan_completed = false;
                    break;
                }

                std::string line = formatEntity(entity, options);
                if (line.empty()) {
                    continue;
                }

                line += '\n';
                writer.write(line);
                stats.bytes_written += line.size();
                stats.exported_entities++;

                // Update the high-water sequence
                if (seq != std::numeric_limits<int64_t>::min() && seq > max_sequence) {
                    max_sequence = seq;
                }

                // Progress callback
                if (options.progress_callback &&
                    stats.exported_entities % options.progress_interval == 0) {

                    auto now = std::chrono::steady_clock::now();
                    stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - start_time);
                    options.progress_callback([[maybe_unused]] stats);
                }

            } catch (const SizeLimitException&) {
                limit_reached = true;
                full_scan_completed = false;
                break;
            } catch (const ExporterException& e) {
                stats.failed_entities++;
                stats.errors.push_back("Entity " + entity.getPrimaryKey() + ": " + e.what());
                metrics_->recordError("exporter_exception");
                if (stats.errors.size() >= options.max_errors) {
                    limit_reached = true;
                    full_scan_completed = false;
                    break;
                }
                if (!options.continue_on_error) {
                    throw;
                }
            } catch (const std::exception& e) {
                stats.failed_entities++;
                stats.errors.push_back("Entity " + entity.getPrimaryKey() + ": " +
                                       std::string(e.what()));
                metrics_->recordError("std_exception");
                if (stats.errors.size() >= options.max_errors) {
                    limit_reached = true;
                    full_scan_completed = false;
                    break;
                }
                if (!options.continue_on_error) {
                    throw;
                }
            }
        }

        // exported up to any limits; watermark still valid

        writer.close();

        // P3: Encrypt output file if configured
        if (options.encryption_config && !options.encryption_config->empty()) {
            const std::string enc_tmp = options.output_path + ".enc_tmp";
            try {
                ExportEncryptor encryptor(*options.encryption_config);
                const size_t enc_bytes =
                    encryptor.encryptFile(options.output_path, enc_tmp);
                std::error_code rename_ec;
                std::filesystem::rename(enc_tmp, options.output_path, rename_ec);
                if (rename_ec) {
                    std::filesystem::remove(enc_tmp);
                    throw ExportIOException(
                        "Failed to rename encrypted file: " + rename_ec.message(),
                        enc_tmp);
                }
                metrics_->recordEncryption(enc_bytes);
            } catch ([[maybe_unused]] const std::exception& e) {
                std::error_code ec;
                std::filesystem::remove(enc_tmp, ec);
                throw;
            }
        }

    } catch (const ExportIOException& e) {
        stats.errors.push_back("[IO] " + std::string(e.what()) +
                               " (file: " + e.getFilePath() + ")");
        metrics_->recordError("io_exception");
        // Do NOT update the watermark on IO failure; the caller can retry.
        auto end_time = std::chrono::steady_clock::now();
        stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        metrics_->recordExport(stats.exported_entities, stats.bytes_written, stats.duration);
        return stats;
    }

    // --- Step 5: atomically update the watermark ---
    if (!config_.watermark_path.empty() && max_sequence > watermark && full_scan_completed && !limit_reached) {
        // Build an ISO-8601 timestamp for the watermark file
        const auto now_tp = std::chrono::system_clock::now();
        const auto now_t  = std::chrono::system_clock::to_time_t(now_tp);
        std::tm tm_buf{};
#if defined(_WIN32)
        gmtime_s(&tm_buf, &now_t);
#else
        gmtime_r(&now_t, &tm_buf);
#endif
        std::ostringstream ts;
        ts << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");

        if (!writeWatermark(max_sequence, stats.exported_entities, ts.str())) {
            THEMIS_WARN("IncrementalExporter: failed to write watermark to {}",
                        config_.watermark_path);
        }
    } else if (!config_.watermark_path.empty() && max_sequence > watermark && !full_scan_completed) {
        THEMIS_WARN("IncrementalExporter: partial scan detected; watermark update skipped to preserve delta consistency");
    }

    const auto end_time = std::chrono::steady_clock::now();
    stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    stats.estimated_eta_seconds = 0.0;

    metrics_->recordExport(stats.exported_entities, stats.bytes_written, stats.duration);

    THEMIS_INFO("IncrementalExporter: exported {} entities, skipped {} (watermark {}→{}), "
                "{} bytes, {}ms",
                stats.exported_entities, stats.skipped_entities,
                watermark, max_sequence,
                stats.bytes_written, stats.duration.count());

    return stats;
}

// ─────────────────────────────────────────────────────────────────────────────
// Watermark I/O
// ─────────────────────────────────────────────────────────────────────────────

int64_t IncrementalExporter::readWatermark() const {
    if (config_.watermark_path.empty()) {
        return std::numeric_limits<int64_t>::min();  // full-export mode
    }

    std::ifstream f(config_.watermark_path);
    if (!f.is_open()) {
        return std::numeric_limits<int64_t>::min();  // no previous run
    }

    try {
        json j;
        f >> j;
        if (j.contains("last_sequence") && j["last_sequence"].is_number_integer()) {
            return j["last_sequence"].get<int64_t>();
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("IncrementalExporter: failed to parse watermark file {}: {}",
                    config_.watermark_path, e.what());
    }
    return std::numeric_limits<int64_t>::min();
}

bool IncrementalExporter::writeWatermark(int64_t sequence,
                                          size_t exported_count,
                                          const std::string& timestamp) const {
    const std::string tmp_path = config_.watermark_path + ".tmp";
    {
        std::ofstream tmp(tmp_path, std::ios::trunc);
        if (!tmp.is_open()) {
            return false;
        }
        json j;
        j["last_sequence"]    = sequence;
        j["last_export_time"] = timestamp;
        j["exported_count"]   = exported_count;
        tmp << j.dump(2) << '\n';
        if (!tmp.good()) {
            return false;
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmp_path, config_.watermark_path, ec);
    if (ec) {
        THEMIS_WARN("IncrementalExporter: watermark rename failed: {}", ec.message());
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

int64_t IncrementalExporter::extractSequence(const BaseEntity& entity) const {
    // Try integer first (most common for monotonic sequence counters)
    auto int_val = entity.getFieldAsInt(config_.sequence_field);
    if (int_val.has_value()) {
        return *int_val;
    }

    // Fall back to double (e.g., Unix timestamp stored as floating point)
    auto dbl_val = entity.getFieldAsDouble(config_.sequence_field);
    if (dbl_val.has_value()) {
        return static_cast<int64_t>(*dbl_val);
    }

    return std::numeric_limits<int64_t>::min();  // absent or unparseable
}

std::string IncrementalExporter::formatEntity(const BaseEntity& entity,
                                              const ExportOptions& options) {
    auto all_fields = entity.getAllFields();
    if (all_fields.empty()) {
        return "";
    }

    json j;
    j["_id"] = entity.getPrimaryKey();

    const bool use_include = !options.include_fields.empty();
    const bool use_exclude = !options.exclude_fields.empty();

    for (const auto& [key, val] : all_fields) {
        if (use_include) {
            bool included = false;
            for (const auto& f : options.include_fields) {
                if (f == key) { included = true; break; }
            }
            if (!included) { continue; }
        }
        if (use_exclude) {
            bool excluded = false;
            for (const auto& f : options.exclude_fields) {
                if (f == key) { excluded = true; break; }
            }
            if (excluded) { continue; }
        }

        std::visit([&]([[maybe_unused]] const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                j[key] = nullptr;
            } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                j[key] = v;
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                std::ostringstream hex;
                hex << std::hex << std::setfill('0');
                for (uint8_t b : v) {
                    hex << std::setw(2) << static_cast<int>(b);
                }
                j[key] = hex.str();
            } else {
                j[key] = v;
            }
        }, val);
    }

    return j.dump();
}

} // namespace themis::exporters
