/**
 * @file content_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

/**
 * @brief Comprehensive metrics for content operations
 * 
 * Tracks performance, throughput, errors, and resource utilization
 * for the content ingestion and processing pipeline.
 * 
 * Thread-safe: All counters are atomic.
 */
class ContentMetrics {
public:
    ContentMetrics() = default;
    
    // ========================================================================
    // Throughput Metrics
    // ========================================================================
    
    /**
     * @brief Record content ingestion
     * @param mime_type MIME type of content
     * @param size_bytes Size in bytes
     */
    void recordIngestion(const std::string& mime_type, uint64_t size_bytes);
    
    /**
     * @brief Record content validation
     * @param success Whether validation passed
     */
    void recordValidation(bool success);
    
    /**
     * @brief Record content processing
     * @param mime_type MIME type
     * @param success Whether processing succeeded
     */
    void recordProcessing(const std::string& mime_type, bool success);
    
    /**
     * @brief Record content extraction
     * @param success Whether extraction succeeded
     */
    void recordExtraction(bool success);
    
    /**
     * @brief Record content chunking
     * @param chunk_count Number of chunks created
     */
    void recordChunking(uint64_t chunk_count);
    
    /**
     * @brief Record embedding generation
     * @param count Number of embeddings generated
     */
    void recordEmbedding(uint64_t count);

    /**
     * @brief Record a failed embedding call (content_embedding_failures_total)
     *
     * Incremented when the embedding pipeline times out or the model returns
     * an error.  Content is stored without an embedding in this case.
     */
    void recordEmbeddingFailure();

    /**
     * @brief Record a perceptual deduplication check (content_dedup_checks_total).
     *
     * Called once per `ingestRawBlob()` invocation on IMAGE or TEXT content
     * when a `DeduplicationChecker` is attached.
     */
    void recordDedupCheck();

    /**
     * @brief Record a near-duplicate detection hit (content_dedup_hits_total).
     *
     * Called when `DeduplicationChecker` identifies the ingested item as a
     * near-duplicate of an already-stored item.
     */
    void recordDedupHit();

    // ========================================================================
    // Format-specific Metrics (content_pdf_extracted_total, etc.)
    // ========================================================================

    /**
     * @brief Record a successful PDF extraction (content_pdf_extracted_total)
     */
    void recordPdfExtracted();

    /**
     * @brief Record a successful Office document extraction (content_office_extracted_total)
     */
    void recordOfficeExtracted();

    /**
     * @brief Record a successful OCR extraction (content_ocr_extracted_total)
     */
    void recordOcrExtracted();

    /**
     * @brief Record a PDF or generic extract-stage error (content_extract_errors_total)
     */
    void recordExtractError();
    
    // ========================================================================
    // Latency Metrics
    // ========================================================================
    
    /**
     * @brief Record operation latency
     * @param operation Operation name (validation, extraction, chunking, embedding)
     * @param latency_ms Latency in milliseconds
     */
    void recordLatency(const std::string& operation, double latency_ms);
    
    /**
     * @brief Get latency percentiles for an operation
     * @param operation Operation name
     * @return Map with p50, p95, p99 percentiles
     */
    std::map<std::string, double> getLatencyPercentiles(const std::string& operation) const;
    
    // ========================================================================
    // Error Metrics
    // ========================================================================
    
    /**
     * @brief Record error by code
     * @param error_code Error code integer
     */
    void recordError(int error_code);
    
    /**
     * @brief Record error by category
     * @param category Error category (validation, processing, security, etc.)
     */
    void recordErrorCategory(const std::string& category);
    
    /**
     * @brief Record timeout
     * @param operation Operation that timed out
     */
    void recordTimeout(const std::string& operation);
    
    // ========================================================================
    // Validation Metrics
    // ========================================================================
    
    /**
     * @brief Record validation violation
     * @param violation_type Type of violation (size, format, policy, mime)
     */
    void recordValidationViolation(const std::string& violation_type);
    
    // ========================================================================
    // Cache Metrics
    // ========================================================================
    
    /**
     * @brief Record cache hit
     */
    void recordCacheHit();
    
    /**
     * @brief Record cache miss
     */
    void recordCacheMiss();
    
    /**
     * @brief Get cache hit rate
     * @return Hit rate as percentage (0-100)
     */
    double getCacheHitRate() const;
    
    // ========================================================================
    // Format Distribution
    // ========================================================================
    
    /**
     * @brief Get count by MIME type
     * @param mime_type MIME type
     * @return Count of items processed
     */
    uint64_t getCountByMimeType(const std::string& mime_type) const;
    
    /**
     * @brief Get all MIME type counts
     * @return Map of MIME type to count
     */
    std::map<std::string, uint64_t> getMimeTypeCounts() const;
    
    // ========================================================================
    // Aggregated Metrics
    // ========================================================================
    
    /**
     * @brief Get total ingestion count
     */
    uint64_t getTotalIngestions() const { return total_ingestions_.load(); }
    
    /**
     * @brief Get total bytes processed
     */
    uint64_t getTotalBytesProcessed() const { return total_bytes_processed_.load(); }
    
    /**
     * @brief Get total validations
     */
    uint64_t getTotalValidations() const { return total_validations_.load(); }
    
    /**
     * @brief Get successful validations
     */
    uint64_t getSuccessfulValidations() const { return successful_validations_.load(); }
    
    /**
     * @brief Get failed validations
     */
    uint64_t getFailedValidations() const { return failed_validations_.load(); }
    
    /**
     * @brief Get validation success rate
     * @return Success rate as percentage (0-100)
     */
    double getValidationSuccessRate() const;
    
    /**
     * @brief Get total processing attempts
     */
    uint64_t getTotalProcessing() const { return total_processing_.load(); }
    
    /**
     * @brief Get successful processing count
     */
    uint64_t getSuccessfulProcessing() const { return successful_processing_.load(); }
    
    /**
     * @brief Get failed processing count
     */
    uint64_t getFailedProcessing() const { return failed_processing_.load(); }
    
    /**
     * @brief Get processing success rate
     * @return Success rate as percentage (0-100)
     */
    double getProcessingSuccessRate() const;
    
    /**
     * @brief Get total errors
     */
    uint64_t getTotalErrors() const { return total_errors_.load(); }
    
    /**
     * @brief Get total timeouts
     */
    uint64_t getTotalTimeouts() const { return total_timeouts_.load(); }

    /**
     * @brief Get total successfully extracted PDF documents
     */
    uint64_t getPdfExtractedTotal() const { return pdf_extracted_total_.load(); }

    /**
     * @brief Get total successfully extracted Office documents
     */
    uint64_t getOfficeExtractedTotal() const { return office_extracted_total_.load(); }

    /**
     * @brief Get total successfully extracted OCR images (content_ocr_extracted_total)
     */
    uint64_t getOcrExtractedTotal() const { return ocr_extracted_total_.load(); }

    /**
     * @brief Get total PDF/document extraction errors
     */
    uint64_t getExtractErrorsTotal() const { return extract_errors_total_.load(); }

    /**
     * @brief Get total embedding failures (content_embedding_failures_total)
     */
    uint64_t getEmbeddingFailuresTotal() const { return embedding_failures_.load(); }

    /**
     * @brief Get total deduplication checks (content_dedup_checks_total)
     */
    uint64_t getDedupChecksTotal() const { return dedup_checks_.load(); }

    /**
     * @brief Get total deduplication hits (content_dedup_hits_total)
     */
    uint64_t getDedupHitsTotal() const { return dedup_hits_.load(); }
    
    // ========================================================================
    // Export & Reset
    // ========================================================================
    
    /**
     * @brief Export metrics as JSON
     */
    json toJson() const;
    
    /**
     * @brief Export metrics in Prometheus text format
     */
    std::string toPrometheusFormat() const;
    
    /**
     * @brief Reset all metrics
     */
    void reset();
    
private:
    // Throughput counters
    std::atomic<uint64_t> total_ingestions_{0};
    std::atomic<uint64_t> total_bytes_processed_{0};
    std::atomic<uint64_t> total_validations_{0};
    std::atomic<uint64_t> successful_validations_{0};
    std::atomic<uint64_t> failed_validations_{0};
    std::atomic<uint64_t> total_processing_{0};
    std::atomic<uint64_t> successful_processing_{0};
    std::atomic<uint64_t> failed_processing_{0};
    std::atomic<uint64_t> total_extractions_{0};
    std::atomic<uint64_t> successful_extractions_{0};
    std::atomic<uint64_t> failed_extractions_{0};
    std::atomic<uint64_t> total_chunks_{0};
    std::atomic<uint64_t> total_embeddings_{0};
    std::atomic<uint64_t> embedding_failures_{0};
    std::atomic<uint64_t> dedup_checks_{0};
    std::atomic<uint64_t> dedup_hits_{0};
    
    // Error counters
    std::atomic<uint64_t> total_errors_{0};
    std::atomic<uint64_t> total_timeouts_{0};

    // Format-specific counters (content_pdf_extracted_total, content_office_extracted_total, content_ocr_extracted_total, content_extract_errors_total)
    std::atomic<uint64_t> pdf_extracted_total_{0};
    std::atomic<uint64_t> office_extracted_total_{0};
    std::atomic<uint64_t> ocr_extracted_total_{0};
    std::atomic<uint64_t> extract_errors_total_{0};
    
    // Cache counters
    std::atomic<uint64_t> cache_hits_{0};
    std::atomic<uint64_t> cache_misses_{0};
    
    // MIME type distribution (protected by mutex)
    mutable std::mutex mime_mutex_;
    std::map<std::string, uint64_t> mime_type_counts_;
    
    // Error code distribution (protected by mutex)
    mutable std::mutex error_mutex_;
    std::map<int, uint64_t> error_code_counts_;
    
    // Error category distribution (protected by mutex)
    mutable std::mutex error_category_mutex_;
    std::map<std::string, uint64_t> error_category_counts_;
    
    // Validation violation counts (protected by mutex)
    mutable std::mutex violation_mutex_;
    std::map<std::string, uint64_t> violation_counts_;
    
    // Timeout counts by operation (protected by mutex)
    mutable std::mutex timeout_mutex_;
    std::map<std::string, uint64_t> timeout_counts_;
    
    // Latency tracking (protected by mutex)
    mutable std::mutex latency_mutex_;
    struct LatencyStats {
        std::vector<double> samples;  // For percentile calculation
        double sum = 0.0;
        uint64_t count = 0;
        double min = 0.0;
        double max = 0.0;
    };
    std::map<std::string, LatencyStats> latency_stats_;
    
    // Helper methods
    void recordLatencyInternal(const std::string& operation, double latency_ms);
    double calculatePercentile(const std::vector<double>& sorted_samples, double percentile) const;
};

} // namespace content
} // namespace themis
