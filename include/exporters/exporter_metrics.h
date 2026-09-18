/**
 * @file exporter_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis::exporters {

class ExporterMetrics {
public:
    ExporterMetrics() = default;
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
    /**
     * @brief Record Export.
     * @param[in] entity_count Input parameter.
     * @param[in] bytes_written Input parameter.
     * @param[in] duration Input parameter.
     */
    void recordExport(size_t entity_count, size_t bytes_written, 
                     std::chrono::milliseconds duration);
    
    /**
     * @brief Record Error.
     * @param[in] error_type Input parameter.
     */
    void recordError(const std::string& error_type);
    
    /**
     * @brief Record Duplicate.
     */
    void recordDuplicate();
    
    /**
     * @brief Record Quality Filter Rejection.
     * @param[in] reason Input parameter.
     */
    void recordQualityFilterRejection(const std::string& reason);
    
    /**
     * @brief Record Schema Validation.
     * @param[in] passed Input parameter.
     */
    void recordSchemaValidation(bool passed);
    
    /**
     * @brief Get Export Rate.
     * @return Return value.
     */
    double getExportRate() const;
    
    /**
     * @brief Get Throughput.
     * @return Return value.
     */
    double getThroughput() const;
    
    /**
     * @brief Get Average Latency.
     * @return Return value.
     */
    double getAverageLatency() const;
    
    /**
     * @brief Get P50 Latency.
     * @return Return value.
     */
    double getP50Latency() const;
    
    /**
     * @brief Get P95 Latency.
     * @return Return value.
     */
    double getP95Latency() const;
    
    /**
     * @brief Get P99 Latency.
     * @return Return value.
     */
    double getP99Latency() const;
    
    /**
     * @brief Get Total Errors.
     * @return Return value.
     */
    size_t getTotalErrors() const;
    
    std::map<std::string, size_t> getErrorsByType() const;
    
    /**
     * @brief Get Total Duplicates.
     * @return Return value.
     */
    size_t getTotalDuplicates() const;
    
    std::map<std::string, size_t> getQualityFilterRejections() const;
    
    struct SchemaValidationStats {
        size_t total_validated = 0;
        size_t passed = 0;
        size_t failed = 0;
        double pass_rate = 0.0;
    };
    /**
     * @brief Get Schema Validation Stats.
     * @return Return value.
     */
    SchemaValidationStats getSchemaValidationStats() const;
    
    void recordPIIDetection(size_t count = 1);
    
    void recordPIIRedaction(size_t count = 1);
    
    /**
     * @brief Get PIIDetections.
     * @return Return value.
     */
    size_t getPIIDetections() const;
    /**
     * @brief Get PIIRedactions.
     * @return Return value.
     */
    size_t getPIIRedactions() const;
    
    /**
     * @brief Record Compression.
     * @param[in] uncompressed_bytes Input parameter.
     * @param[in] compressed_bytes Input parameter.
     */
    void recordCompression(size_t uncompressed_bytes, size_t compressed_bytes);
    
    /**
     * @brief Get Compression Ratio.
     * @return Return value.
     */
    double getCompressionRatio() const;

    /**
     * @brief Record Parquet Bytes Written.
     * @param[in] bytes Input parameter.
     */
    void recordParquetBytesWritten(size_t bytes);

    /**
     * @brief Get Parquet Bytes Written.
     * @return Return value.
     */
    size_t getParquetBytesWritten() const;

    /**
     * @brief Record Checkpoint.
     */
    void recordCheckpoint();

    /**
     * @brief Get Checkpoint Count.
     * @return Return value.
     */
    size_t getCheckpointCount() const;

    void recordDeltaDocSkipped(size_t count = 1);

    /**
     * @brief Get Delta Docs Skipped.
     * @return Return value.
     */
    size_t getDeltaDocsSkipped() const;

    /**
     * @brief Record Encryption.
     * @param[in] plaintext_bytes Input parameter.
     * @param[in] encrypted_bytes Input parameter.
     */
    void recordEncryption(size_t plaintext_bytes, size_t encrypted_bytes);

    /**
     * @brief Get Encrypted Plaintext Bytes.
     * @return Return value.
     */
    size_t getEncryptedPlaintextBytes() const;

    /**
     * @brief Get Encrypted Output Bytes.
     * @return Return value.
     */
    size_t getEncryptedOutputBytes() const;
    /**
     * @brief Record Encryption.
     * @param[in] encrypted_bytes Input parameter.
     */
    void recordEncryption(size_t encrypted_bytes);

    /**
     * @brief Get Encrypted Bytes Written.
     * @return Return value.
     */
    size_t getEncryptedBytesWritten() const;

    /**
     * @brief Record Rate Limit Hit.
     */
    void recordRateLimitHit();

    /**
     * @brief Get Rate Limit Hits.
     * @return Return value.
     */
    size_t getRateLimitHits() const;

    /**
     * @brief Record Policy Denial.
     * @param[in] collection Input parameter.
     * @param[in] user Input parameter.
     */
    void recordPolicyDenial(const std::string& collection, const std::string& user);

    /**
     * @brief Get Policy Denials.
     * @return Return value.
     */
    size_t getPolicyDenials() const;

    /**
     * @brief Record Hub Upload Failure.
     * @param[in] reason Input parameter.
     */
    void recordHubUploadFailure(const std::string& reason);

    /**
     * @brief Get Hub Upload Failures.
     * @return Return value.
     */
    size_t getHubUploadFailures() const;

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief To String.
     * @return Return value.
     */
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
    
    /**
     * @brief Update Latency Histogram.
     * @param[in] duration Input parameter.
     */
    void updateLatencyHistogram(std::chrono::milliseconds duration);
    
    /**
     * @brief Calculate Percentile.
     * @param[in] percentile Input parameter.
     * @return Return value.
     */
    double calculatePercentile(double percentile) const;
};

} // namespace themis::exporters
