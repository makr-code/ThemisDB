/**
 * @file streaming_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/streaming_exporter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

#include "exporters/aql_predicate_filter.h"
#include "exporters/export_encryption.h"
#include "exporters/exporter_errors.h"
#include "exporters/exporter_interface.h"
#include "exporters/stream_writer.h"
#include "utils/logger.h"

using json = nlohmann::json;

namespace themis::exporters {

// ─────────────────────────────────────────────────────────────────────────────
// VectorExportCursor
// ─────────────────────────────────────────────────────────────────────────────

VectorExportCursor::VectorExportCursor(const std::vector<BaseEntity> &entities, size_t page_size)
    : entities_(entities), page_size_(page_size > 0 ? page_size : 1) {}

bool VectorExportCursor::hasNext() const {
    return static_cast<bool>(offset_ < entities_.size());
}

std::vector<BaseEntity> VectorExportCursor::nextPage() {
    size_t end = std::min(offset_ + page_size_, entities_.size());
    std::vector<BaseEntity> page(entities_.begin() + offset_, entities_.begin() + end);
    offset_ = end;
    return page;
}

bool VectorExportCursor::seekTo(size_t offset) {
    if (offset > entities_.size()) {
        return false;
    }
    offset_ = offset;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// StreamingExporter
// ─────────────────────────────────────────────────────────────────────────────

StreamingExporter::StreamingExporter(const StreamingExportConfig &config)
    : config_(config), metrics_(std::make_shared<ExporterMetrics>()) {}

ExportStats StreamingExporter::exportEntities(const std::vector<BaseEntity> &entities, const ExportOptions &options) {
    // Policy check before any cursor or file is opened (EXP-001).
    enforceExportPolicy(options);
    VectorExportCursor cursor(entities, config_.page_size);
    return exportFromCursor(cursor, options);
}

ExportStats StreamingExporter::exportFromCursor(ExportCursor &cursor, const ExportOptions &options) {
    ExportStats stats;
    stats.metrics   = metrics_;
    auto start_time = std::chrono::steady_clock::now();

    // Resume from checkpoint if available
    size_t resume_offset = 0;
    if (!config_.checkpoint_path.empty()) {
        resume_offset = readCheckpoint(config_.checkpoint_path);
        if (resume_offset > 0) {
            if (cursor.seekTo(resume_offset)) {
                metrics_->recordCheckpoint();
                THEMIS_INFO("StreamingExporter: resuming from checkpoint offset {}", resume_offset);
            } else {
                THEMIS_WARN("StreamingExporter: seekTo({}) failed, starting from beginning", resume_offset);
                resume_offset = 0;
            }
        }
    }

    const size_t total_count = cursor.totalCount();

    // Configure StreamWriter
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

    try {
        StreamWriter writer(writer_config);

        // AQL predicate filter (compiled once, reused per entity)
        std::unique_ptr<AqlPredicateFilter> aql_filter = {};

        if (!options.filter_expression.empty()) {
            aql_filter = std::make_unique<AqlPredicateFilter>(options.filter_expression);
        }

        bool limit_reached = false;

        // Process pages until the cursor is exhausted or a size limit is hit
        while (!limit_reached && cursor.hasNext()) {
            // Honour file size limit
            if (writer.isLimitReached()) {
                THEMIS_WARN("StreamingExporter: size limit reached after {} entities", stats.exported_entities);
                break;
            }

            // Enforce max_buffer_bytes: flush when the in-flight buffer is full
            if (config_.max_buffer_bytes > 0 && stats.bytes_written >= config_.max_buffer_bytes) {
                writer.flush();
            }

            auto page = cursor.nextPage();

            for (const auto &entity : page) {
                stats.total_entities++;

                try {
                    // AQL predicate filter
                    if (aql_filter && !aql_filter->evaluate(entity)) {
                        continue;
                    }

                    std::string line = formatEntity(entity, options);
                    if (line.empty()) {
                        continue;
                    }

                    line += '\n';
                    writer.write(line);
                    stats.bytes_written += line.size();
                    stats.exported_entities++;

                    // Progress reporting with ETA
                    if (options.progress_callback && stats.exported_entities % options.progress_interval == 0) {
                        auto now       = std::chrono::steady_clock::now();
                        stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);

                        if (total_count > 0) {
                            stats.estimated_eta_seconds
                                = calculateETA(stats.exported_entities, total_count, start_time);
                        }

                        options.progress_callback(stats);
                    }

                } catch (const SizeLimitException &) {
                    throw; // propagate size-limit errors
                } catch (const ExporterException &e) {
                    stats.failed_entities++;
                    stats.errors.push_back("Entity " + entity.getPrimaryKey() + ": " + e.what());
                    metrics_->recordError("exporter_exception");

                    if (static_cast<int>(stats.errors.size()) >= options.max_errors) {
                        THEMIS_ERROR("StreamingExporter: max errors reached, stopping");
                        limit_reached = true;
                        break;
                    }
                    if (!options.continue_on_error) {
                        throw;
                    }
                } catch (const std::exception &e) {
                    stats.failed_entities++;
                    stats.errors.push_back("Entity " + entity.getPrimaryKey() + ": " + std::string(e.what()));
                    metrics_->recordError("std_exception");

                    if (static_cast<int>(stats.errors.size()) >= options.max_errors) {
                        THEMIS_ERROR("StreamingExporter: max errors reached, stopping");
                        limit_reached = true;
                        break;
                    }
                    if (!options.continue_on_error) {
                        throw;
                    }
                }
            }

            // Persist checkpoint after each completed page
            if (!config_.checkpoint_path.empty()) {
                writeCheckpoint(config_.checkpoint_path, cursor.currentOffset());
            }
        }

        writer.close();

        // Compression metrics
        if (options.compress) {
            metrics_->recordCompression(writer.getBytesWritten(), writer.getCompressedBytesWritten());
        }

        const bool has_v2_encryption
            = options.encryption_config && !options.encryption_config->empty();

        // Optional AES-256-GCM encryption of the output file.
        // When enabled, the plaintext export file is encrypted in-place:
        // the ciphertext overwrites the original file at output_path, and
        // the plaintext bytes are securely discarded from memory.
        if (!has_v2_encryption && options.encryption.enabled && !options.output_path.empty()) {
            const std::string tmp_path = options.output_path + ".enc_tmp";
            ExportEncryption encryptor(options.encryption);
            encryptor.encryptFile(options.output_path, tmp_path);
            // Atomic replace: rename temp -> output_path
            std::error_code ec = {};
            std::filesystem::rename(tmp_path, options.output_path, ec);
            if (ec) {
                throw ExportIOException("ExportEncryption: rename failed: " + ec.message(), options.output_path);
            }
            const auto encrypted_size = static_cast<size_t>(std::filesystem::file_size(options.output_path, ec));
            if (!ec) {
                metrics_->recordEncryption(stats.bytes_written, encrypted_size);
            }
            THEMIS_INFO("StreamingExporter: encrypted output file ({} plaintext bytes, "
                        "{} encrypted bytes, job_id={})",
                        stats.bytes_written, encrypted_size, options.encryption.job_id);
        }

        // P3: Encrypt output file if configured
        if (has_v2_encryption) {
            if (options.encryption.enabled) {
                THEMIS_WARN("StreamingExporter: both legacy encryption and encryption_config are set; "
                            "using encryption_config and ignoring legacy settings.");
            }
            const std::string enc_tmp = options.output_path + ".enc_tmp";
            try {
                ExportEncryptor encryptor(*options.encryption_config);
                const size_t enc_bytes = encryptor.encryptFile(options.output_path, enc_tmp);
                std::error_code rename_ec = {};
                std::filesystem::rename(enc_tmp, options.output_path, rename_ec);
                if (rename_ec) {
                    std::filesystem::remove(enc_tmp);
                    throw ExportIOException("Failed to rename encrypted file: " + rename_ec.message(), enc_tmp);
                }
                metrics_->recordEncryption(enc_bytes);
            } catch (const std::exception &) {
                std::error_code ec = {};
                std::filesystem::remove(enc_tmp, ec);
                throw;
            }
        }

    } catch (const ExportIOException &e) {
        stats.errors.push_back("[IO] " + std::string(e.what()) + " (file: " + e.getFilePath() + ")");
        metrics_->recordError("io_exception");
    }

    auto end_time  = std::chrono::steady_clock::now();
    stats.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Final ETA is zero (export complete)
    stats.estimated_eta_seconds = 0.0;

    metrics_->recordExport(stats.exported_entities, stats.bytes_written, stats.duration);

    THEMIS_INFO("StreamingExporter: exported {} entities ({} bytes) in {}ms", stats.exported_entities,
                stats.bytes_written, stats.duration.count());

    return stats;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string StreamingExporter::formatEntity(const BaseEntity &entity, const ExportOptions &options) {
    auto all_fields = entity.getAllFields();
    if (all_fields.empty()) {
        return "";
    }

    json j;
    j["_id"] = entity.getPrimaryKey();

    const bool use_include = !options.include_fields.empty();
    const bool use_exclude = !options.exclude_fields.empty();

    for (const auto &[key, val] : all_fields) {
        // Apply field selection
        if (use_include) {
            bool included = false;
            for (const auto &f : options.include_fields) {
                if (f == key) {
                    included = true;
                    break;
                }
            }
            if (!included) {
                continue;
            }
        }
        if (use_exclude) {
            bool excluded = false;
            for (const auto &f : options.exclude_fields) {
                if (f == key) {
                    excluded = true;
                    break;
                }
            }
            if (excluded) {
                continue;
            }
        }

        // Serialise the variant value to JSON
        std::visit(
            [&](const auto &v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    j[key] = nullptr;
                } else if constexpr (std::is_same_v<T, std::vector<float>>) {
                    j[key] = v;
                } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                    // Encode binary as zero-padded hex string
                    std::ostringstream hex = {};
                    hex << std::hex << std::setfill('0');
                    for (uint8_t b : v) {
                        hex << std::setw(2) << static_cast<int>(b);
                    }
                    j[key] = hex.str();
                } else {
                    j[key] = v;
                }
            },
            val);
    }

    return j.dump();
}

void StreamingExporter::writeCheckpoint(const std::string &path, size_t offset) {
    // Atomic write: write to a temp file then rename
    const std::string tmp_path = path + ".tmp";
    {
        std::ofstream tmp(tmp_path, std::ios::trunc);
        if (!tmp.is_open()) {
            THEMIS_WARN("StreamingExporter: could not write checkpoint to {}", path);
            return;
        }
        tmp << offset << '\n';
    }
    std::error_code ec = {};
    std::filesystem::rename(tmp_path, path, ec);
    if (ec) {
        THEMIS_WARN("StreamingExporter: checkpoint rename failed: {}", ec.message());
    }
}

size_t StreamingExporter::readCheckpoint(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        return 0;
    }
    size_t offset = 0;
    f >> offset;
    return f ? offset : 0;
}

double StreamingExporter::calculateETA(size_t processed, size_t total,
                                       std::chrono::steady_clock::time_point start_time) {
    if (processed == 0 || total == 0 || processed >= total) {
        return 0.0;
    }
    auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time).count();
    double rate  = static_cast<double>(processed) / elapsed; // entities/s
    return static_cast<double>(total - processed) / rate;
}

} // namespace themis::exporters
