/**
 * @file exporter_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: exporter_metrics.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 216
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #3215 [exporters] Implement incre... (2026-03-12) | #2593 feat(exporters): Parquet ex... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis::exporters {

/// Metrics for export operations
class ExporterMetrics {
public:
    ExporterMetrics() = default;
    
    /// Reset all metrics
    void reset();
    
    /// Record an export operation
    void recordExport(size_t entity_count, size_t bytes_written, 
                     std::chrono::milliseconds duration);
    
    /// Record an error
    void recordError(const std::string& error_type);
    
    /// Record a duplicate detection
    void recordDuplicate();
    
    /// Record a quality filter rejection
    void recordQualityFilterRejection(const std::string& reason);
    
    /// Record schema validation
    void recordSchemaValidation(bool passed);
    
    /// Get export rate (entities per second)
    double getExportRate() const;
    
    /// Get throughput (bytes per second)
    double getThroughput() const;
    
    /// Get average latency per entity (milliseconds)
    double getAverageLatency() const;
    
    /// Get P50 latency estimate (milliseconds)
    double getP50Latency() const;
    
    /// Get P95 latency estimate (milliseconds)
    double getP95Latency() const;
    
    /// Get P99 latency estimate (milliseconds)
    double getP99Latency() const;
    
    /// Get total errors
    size_t getTotalErrors() const;
    
    /// Get errors by type
    std::map<std::string, size_t> getErrorsByType() const;
    
    /// Get total duplicates detected
    size_t getTotalDuplicates() const;
    
    /// Get quality filter rejections
    std::map<std::string, size_t> getQualityFilterRejections() const;
    
    /// Get schema validation stats
    struct SchemaValidationStats {
        size_t total_validated = 0;
        size_t passed = 0;
        size_t failed = 0;
        double pass_rate = 0.0;
    };
    SchemaValidationStats getSchemaValidationStats() const;
    
    /// P1: Record PII detection
    void recordPIIDetection(size_t count = 1);
    
    /// P1: Record PII redaction
    void recordPIIRedaction(size_t count = 1);
    
    /// P1: Get PII detection stats
    size_t getPIIDetections() const;
    size_t getPIIRedactions() const;
    
    /// P2: Record compression stats
    void recordCompression(size_t uncompressed_bytes, size_t compressed_bytes);
    
    /// P2: Get compression ratio
    double getCompressionRatio() const;

    /// P3: Record Parquet bytes written (exporter_parquet_bytes_written_total)
    void recordParquetBytesWritten(size_t bytes);

    /// P3: Get total Parquet bytes written
    size_t getParquetBytesWritten() const;

    /// Streaming: Record a checkpoint event (resume events for Prometheus/Grafana)
    void recordCheckpoint();

    /// Streaming: Get total checkpoint events
    size_t getCheckpointCount() const;

    /// Delta: Record documents skipped because they are at or below the watermark
    /// (exporter_delta_docs_skipped_total)
    void recordDeltaDocSkipped(size_t count = 1);

    /// Delta: Get total documents skipped by incremental export
    size_t getDeltaDocsSkipped() const;

    /// P3/Security: Record an export encryption event (exporter_encrypted_bytes_total)
    /// @param plaintext_bytes  Number of plaintext bytes submitted for encryption.
    /// @param encrypted_bytes  Number of bytes written to the encrypted container.
    void recordEncryption(size_t plaintext_bytes, size_t encrypted_bytes);

    /// P3/Security: Get total plaintext bytes that were encrypted
    size_t getEncryptedPlaintextBytes() const;

    /// P3/Security: Get total bytes written to encrypted containers
    size_t getEncryptedOutputBytes() const;
    /// Encryption: Record bytes written to an encrypted export file
    /// (exporter_encrypted_bytes_written_total)
    void recordEncryption(size_t encrypted_bytes);

    /// Encryption: Get total bytes written to encrypted export files
    size_t getEncryptedBytesWritten() const;

    /// HuggingFace: Record an HTTP 429 rate-limit hit
    /// (exporters.huggingface.rate_limit_hit)
    void recordRateLimitHit();

    /// HuggingFace: Get total rate-limit hits recorded
    size_t getRateLimitHits() const;

    /// @brief Record an export denied by PolicyEngine.
    ///
    /// Increments the policy-denial counter and registers the event under the
    /// unified error type key "policy_denied" in @c getErrorsByType().
    ///
    /// @param collection   Name of the collection that was denied.
    /// @param user         Identity of the requesting user/service.
    void recordPolicyDenial(const std::string& collection, const std::string& user);

    /// @return Total number of policy-denial events recorded.
    size_t getPolicyDenials() const;

    /// @brief Record a HuggingFace Hub upload failure.
    ///
    /// Increments the hub-upload-failure counter and registers the event under
    /// the unified error type key "hub_upload_failure" in @c getErrorsByType().
    ///
    /// @param reason   Short reason string (e.g. HTTP status code or error class).
    void recordHubUploadFailure(const std::string& reason);

    /// @return Total number of hub-upload-failure events recorded.
    size_t getHubUploadFailures() const;

    /// Export metrics as JSON
    nlohmann::json toJson() const;
    
    /// Export metrics summary as string
    std::string toString() const;
    
private:
    mutable std::mutex mutex_;
    
    // Export statistics
    std::atomic<size_t> total_exports_{0};
    std::atomic<size_t> total_entities_{0};
    std::atomic<size_t> total_bytes_{0};
    std::atomic<size_t> total_duration_ms_{0};
    
    // Latency tracking (simple histogram)
    struct LatencyHistogram {
        std::atomic<size_t> count_0_10ms{0};    // 0-10ms
        std::atomic<size_t> count_10_50ms{0};   // 10-50ms
        std::atomic<size_t> count_50_100ms{0};  // 50-100ms
        std::atomic<size_t> count_100_500ms{0}; // 100-500ms
        std::atomic<size_t> count_500plus{0};   // 500ms+
    } latency_histogram_;
    
    // Error tracking
    std::atomic<size_t> total_errors_{0};
    std::map<std::string, size_t> errors_by_type_;
    
    // Duplicate detection
    std::atomic<size_t> total_duplicates_{0};
    
    // Quality filtering
    std::map<std::string, size_t> quality_filter_rejections_;
    
    // Schema validation
    std::atomic<size_t> schema_validations_total_{0};
    std::atomic<size_t> schema_validations_passed_{0};
    std::atomic<size_t> schema_validations_failed_{0};
    
    // P1: PII detection
    std::atomic<size_t> pii_detections_{0};
    std::atomic<size_t> pii_redactions_{0};
    
    // P2: Compression
    std::atomic<size_t> compression_uncompressed_bytes_{0};
    std::atomic<size_t> compression_compressed_bytes_{0};

    // P3: Parquet export bytes (exporter_parquet_bytes_written_total)
    std::atomic<size_t> parquet_bytes_written_{0};

    // Streaming: checkpoint events
    std::atomic<size_t> checkpoint_count_{0};

    // Delta: documents skipped by incremental filter (exporter_delta_docs_skipped_total)
    std::atomic<size_t> delta_docs_skipped_{0};

    // P3/Security: encryption metrics (exporter_encrypted_bytes_total)
    std::atomic<size_t> encryption_plaintext_bytes_{0};
    std::atomic<size_t> encryption_output_bytes_{0};
    // Encryption: bytes written to encrypted export files
    // (exporter_encrypted_bytes_written_total)
    std::atomic<size_t> encrypted_bytes_written_{0};

    // HuggingFace: HTTP 429 rate-limit hits (exporters.huggingface.rate_limit_hit)
    std::atomic<size_t> rate_limit_hits_{0};

    // Policy denials (exporter_policy_denials_total)
    std::atomic<size_t> policy_denials_{0};

    // HuggingFace Hub upload failures (exporter_hub_upload_failures_total)
    std::atomic<size_t> hub_upload_failures_{0};
    
    // Helper to update latency histogram
    void updateLatencyHistogram(std::chrono::milliseconds duration);
    
    // Helper to calculate percentile from histogram
    double calculatePercentile(double percentile) const;
};

} // namespace themis::exporters
