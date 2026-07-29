/**
 * @file exporter_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: exporter_metrics.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 408
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * PR History (last 5): #3215 [exporters] Implement incre... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "exporters/exporter_metrics.h"
#include <sstream>
#include <iomanip>

namespace themis::exporters {

using json = nlohmann::json;

// Latency bucket midpoints for percentile estimation (in milliseconds)
// These are the midpoints of each histogram bucket range:
// - 0-10ms range: midpoint = 5.0
// - 10-50ms range: midpoint = 30.0
// - 50-100ms range: midpoint = 75.0
// - 100-500ms range: midpoint = 300.0
// - 500ms+ range: represented as 500.0
constexpr double LATENCY_0_10MS_MIDPOINT = 5.0;
constexpr double LATENCY_10_50MS_MIDPOINT = 30.0;
constexpr double LATENCY_50_100MS_MIDPOINT = 75.0;
constexpr double LATENCY_100_500MS_MIDPOINT = 300.0;
constexpr double LATENCY_500PLUS_MIDPOINT = 500.0;

void ExporterMetrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    total_exports_ = 0;
    total_entities_ = 0;
    total_bytes_ = 0;
    total_duration_ms_ = 0;
    
    latency_histogram_.count_0_10ms = 0;
    latency_histogram_.count_10_50ms = 0;
    latency_histogram_.count_50_100ms = 0;
    latency_histogram_.count_100_500ms = 0;
    latency_histogram_.count_500plus = 0;
    
    total_errors_ = 0;
    errors_by_type_.clear();
    
    total_duplicates_ = 0;
    quality_filter_rejections_.clear();
    
    schema_validations_total_ = 0;
    schema_validations_passed_ = 0;
    schema_validations_failed_ = 0;

    pii_detections_ = 0;
    pii_redactions_ = 0;

    compression_uncompressed_bytes_ = 0;
    compression_compressed_bytes_ = 0;

    parquet_bytes_written_ = 0;

    checkpoint_count_ = 0;

    delta_docs_skipped_ = 0;

    encryption_plaintext_bytes_ = 0;
    encryption_output_bytes_    = 0;
    encrypted_bytes_written_ = 0;
    rate_limit_hits_ = 0;
    policy_denials_ = 0;
    hub_upload_failures_ = 0;
}

void ExporterMetrics::recordExport(size_t entity_count, size_t bytes_written,
                                   std::chrono::milliseconds duration) {
    // Lock to ensure consistent state across multiple atomics
    std::lock_guard<std::mutex> lock(mutex_);
    
    total_exports_++;
    total_entities_ += entity_count;
    total_bytes_ += bytes_written;
    total_duration_ms_ += duration.count();
    
    updateLatencyHistogram(duration);
}

void ExporterMetrics::recordError(const std::string& error_type) {
    total_errors_++;
    
    std::lock_guard<std::mutex> lock(mutex_);
    errors_by_type_[error_type]++;
}

void ExporterMetrics::recordDuplicate() {
    total_duplicates_++;
}

void ExporterMetrics::recordQualityFilterRejection(const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    quality_filter_rejections_[reason]++;
}

void ExporterMetrics::recordSchemaValidation(bool passed) {
    schema_validations_total_++;
    if (passed) {
        schema_validations_passed_++;
    } else {
        schema_validations_failed_++;
    }
}

double ExporterMetrics::getExportRate() const {
    size_t duration_ms = total_duration_ms_.load();
    if (duration_ms == 0) return 0.0;
    
    size_t entities = total_entities_.load();
    return (entities * 1000.0) / duration_ms;  // entities per second
}

double ExporterMetrics::getThroughput() const {
    size_t duration_ms = total_duration_ms_.load();
    if (duration_ms == 0) return 0.0;
    
    size_t bytes = total_bytes_.load();
    return (bytes * 1000.0) / duration_ms;  // bytes per second
}

double ExporterMetrics::getAverageLatency() const {
    size_t exports = total_exports_.load();
    if (exports == 0) return 0.0;
    
    size_t duration_ms = total_duration_ms_.load();
    return static_cast<double>(duration_ms) / exports;
}

double ExporterMetrics::getP50Latency() const {
    return calculatePercentile(0.50);
}

double ExporterMetrics::getP95Latency() const {
    return calculatePercentile(0.95);
}

double ExporterMetrics::getP99Latency() const {
    return calculatePercentile(0.99);
}

size_t ExporterMetrics::getTotalErrors() const {
    return total_errors_.load();
}

std::map<std::string, size_t> ExporterMetrics::getErrorsByType() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return errors_by_type_;
}

size_t ExporterMetrics::getTotalDuplicates() const {
    return total_duplicates_.load();
}

std::map<std::string, size_t> ExporterMetrics::getQualityFilterRejections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return quality_filter_rejections_;
}

ExporterMetrics::SchemaValidationStats ExporterMetrics::getSchemaValidationStats() const {
    SchemaValidationStats stats;
    stats.total_validated = schema_validations_total_.load();
    stats.passed = schema_validations_passed_.load();
    stats.failed = schema_validations_failed_.load();
    
    if (stats.total_validated > 0) {
        stats.pass_rate = (static_cast<double>(stats.passed) / stats.total_validated) * 100.0;
    }
    
    return stats;
}

void ExporterMetrics::recordPIIDetection(size_t count) {
    pii_detections_ += count;
}

void ExporterMetrics::recordPIIRedaction(size_t count) {
    pii_redactions_ += count;
}

size_t ExporterMetrics::getPIIDetections() const {
    return pii_detections_.load();
}

size_t ExporterMetrics::getPIIRedactions() const {
    return pii_redactions_.load();
}

void ExporterMetrics::recordCompression(size_t uncompressed_bytes, size_t compressed_bytes) {
    compression_uncompressed_bytes_ += uncompressed_bytes;
    compression_compressed_bytes_ += compressed_bytes;
}

double ExporterMetrics::getCompressionRatio() const {
    size_t uncompressed = compression_uncompressed_bytes_.load();
    size_t compressed = compression_compressed_bytes_.load();
    
    if (uncompressed == 0) return 0.0;
    return static_cast<double>(compressed) / uncompressed;
}

void ExporterMetrics::recordParquetBytesWritten(size_t bytes) {
    parquet_bytes_written_ += bytes;
}

size_t ExporterMetrics::getParquetBytesWritten() const {
    return parquet_bytes_written_.load();
}

void ExporterMetrics::recordCheckpoint() {
    checkpoint_count_++;
}

size_t ExporterMetrics::getCheckpointCount() const {
    return checkpoint_count_.load();
}

void ExporterMetrics::recordDeltaDocSkipped(size_t count) {
    delta_docs_skipped_ += count;
}

size_t ExporterMetrics::getDeltaDocsSkipped() const {
    return delta_docs_skipped_.load();
}

void ExporterMetrics::recordEncryption(size_t plaintext_bytes,
                                        size_t encrypted_bytes) {
    encryption_plaintext_bytes_ += plaintext_bytes;
    encryption_output_bytes_    += encrypted_bytes;
}

size_t ExporterMetrics::getEncryptedPlaintextBytes() const {
    return encryption_plaintext_bytes_.load();
}

size_t ExporterMetrics::getEncryptedOutputBytes() const {
    return encryption_output_bytes_.load();
}

void ExporterMetrics::recordEncryption(size_t encrypted_bytes) {
    encrypted_bytes_written_ += encrypted_bytes;
}

size_t ExporterMetrics::getEncryptedBytesWritten() const {
    return encrypted_bytes_written_.load();
}

void ExporterMetrics::recordRateLimitHit() {
    rate_limit_hits_++;
}

size_t ExporterMetrics::getRateLimitHits() const {
    return rate_limit_hits_.load();
}

void ExporterMetrics::recordPolicyDenial(const std::string& collection,
                                          const std::string& user) {
    policy_denials_++;
    // Also register under the unified error taxonomy so getErrorsByType()
    // reflects it alongside other error categories.
    recordError("policy_denied");
    (void)collection;
    (void)user;
}

size_t ExporterMetrics::getPolicyDenials() const {
    return policy_denials_.load();
}

void ExporterMetrics::recordHubUploadFailure(const std::string& reason) {
    hub_upload_failures_++;
    recordError("hub_upload_failure");
    (void)reason;
}

size_t ExporterMetrics::getHubUploadFailures() const {
    return hub_upload_failures_.load();
}

json ExporterMetrics::toJson() const {
    json j;
    
    // Export stats
    j["total_exports"] = total_exports_.load();
    j["total_entities"] = total_entities_.load();
    j["total_bytes"] = total_bytes_.load();
    j["total_duration_ms"] = total_duration_ms_.load();
    
    // Rates
    j["export_rate_per_sec"] = getExportRate();
    j["throughput_bytes_per_sec"] = getThroughput();
    
    // Latency
    j["average_latency_ms"] = getAverageLatency();
    j["p50_latency_ms"] = getP50Latency();
    j["p95_latency_ms"] = getP95Latency();
    j["p99_latency_ms"] = getP99Latency();
    
    // Errors
    j["total_errors"] = total_errors_.load();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        j["errors_by_type"] = errors_by_type_;
    }
    
    // Duplicates
    j["total_duplicates"] = total_duplicates_.load();
    
    // Quality filtering
    {
        std::lock_guard<std::mutex> lock(mutex_);
        j["quality_filter_rejections"] = quality_filter_rejections_;
    }
    
    // Schema validation
    auto schema_stats = getSchemaValidationStats();
    j["schema_validation"] = {
        {"total", schema_stats.total_validated},
        {"passed", schema_stats.passed},
        {"failed", schema_stats.failed},
        {"pass_rate", schema_stats.pass_rate}
    };
    
    // P1: PII detection
    j["pii_detections"] = pii_detections_.load();
    j["pii_redactions"] = pii_redactions_.load();
    
    // P2: Compression
    j["compression"] = {
        {"uncompressed_bytes", compression_uncompressed_bytes_.load()},
        {"compressed_bytes", compression_compressed_bytes_.load()},
        {"ratio", getCompressionRatio()}
    };

    // P3: Parquet export (exporter_parquet_bytes_written_total)
    j["exporter_parquet_bytes_written_total"] = parquet_bytes_written_.load();

    // Streaming: checkpoint events
    j["checkpoint_count"] = checkpoint_count_.load();

    // Delta: incremental export skipped docs (exporter_delta_docs_skipped_total)
    j["exporter_delta_docs_skipped_total"] = delta_docs_skipped_.load();

    // P3/Security: encryption metrics (exporter_encrypted_bytes_total)
    j["encryption"] = {
        {"plaintext_bytes", encryption_plaintext_bytes_.load()},
        {"output_bytes",    encryption_output_bytes_.load()}
    };
    // Encryption: bytes written to encrypted export files
    // (exporter_encrypted_bytes_written_total)
    j["exporter_encrypted_bytes_written_total"] = encrypted_bytes_written_.load();

    // HuggingFace rate-limit hits (exporters.huggingface.rate_limit_hit)
    j["exporters.huggingface.rate_limit_hit"] = rate_limit_hits_.load();

    // Policy denials (exporter_policy_denials_total)
    j["exporter_policy_denials_total"] = policy_denials_.load();

    // HuggingFace Hub upload failures (exporter_hub_upload_failures_total)
    j["exporter_hub_upload_failures_total"] = hub_upload_failures_.load();
    
    return j;
}

std::string ExporterMetrics::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    oss << "Export Metrics:\n";
    oss << "  Total Exports: " << total_exports_.load() << "\n";
    oss << "  Total Entities: " << total_entities_.load() << "\n";
    oss << "  Total Bytes: " << total_bytes_.load() << "\n";
    oss << "  Export Rate: " << getExportRate() << " entities/sec\n";
    oss << "  Throughput: " << getThroughput() << " bytes/sec\n";
    oss << "  Average Latency: " << getAverageLatency() << " ms\n";
    oss << "  P50 Latency: " << getP50Latency() << " ms\n";
    oss << "  P95 Latency: " << getP95Latency() << " ms\n";
    oss << "  P99 Latency: " << getP99Latency() << " ms\n";
    oss << "  Total Errors: " << total_errors_.load() << "\n";
    oss << "  Total Duplicates: " << total_duplicates_.load() << "\n";
    oss << "  Policy Denials: " << policy_denials_.load() << "\n";
    oss << "  Hub Upload Failures: " << hub_upload_failures_.load() << "\n";
    
    auto schema_stats = getSchemaValidationStats();
    oss << "  Schema Validation: " << schema_stats.passed << "/" 
        << schema_stats.total_validated << " passed (" 
        << schema_stats.pass_rate << "%)\n";
    
    return oss.str();
}

void ExporterMetrics::updateLatencyHistogram(std::chrono::milliseconds duration) {
    auto ms = duration.count();
    
    if (ms < 10) {
        latency_histogram_.count_0_10ms++;
    } else if (ms < 50) {
        latency_histogram_.count_10_50ms++;
    } else if (ms < 100) {
        latency_histogram_.count_50_100ms++;
    } else if (ms < 500) {
        latency_histogram_.count_100_500ms++;
    } else {
        latency_histogram_.count_500plus++;
    }
}

double ExporterMetrics::calculatePercentile(double percentile) const {
    size_t total = latency_histogram_.count_0_10ms.load() +
                   latency_histogram_.count_10_50ms.load() +
                   latency_histogram_.count_50_100ms.load() +
                   latency_histogram_.count_100_500ms.load() +
                   latency_histogram_.count_500plus.load();
    
    if (total == 0) return 0.0;
    
    size_t target = static_cast<size_t>(total * percentile);
    size_t cumulative = 0;
    
    cumulative += latency_histogram_.count_0_10ms.load();
    if (cumulative >= target) return LATENCY_0_10MS_MIDPOINT;
    
    cumulative += latency_histogram_.count_10_50ms.load();
    if (cumulative >= target) return LATENCY_10_50MS_MIDPOINT;
    
    cumulative += latency_histogram_.count_50_100ms.load();
    if (cumulative >= target) return LATENCY_50_100MS_MIDPOINT;
    
    cumulative += latency_histogram_.count_100_500ms.load();
    if (cumulative >= target) return LATENCY_100_500MS_MIDPOINT;
    
    return LATENCY_500PLUS_MIDPOINT;
}

} // namespace themis::exporters
