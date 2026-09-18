/**
 * @file content_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ContentMetrics {
public:
    ContentMetrics() = default;
    
    // ========================================================================
    // Throughput Metrics
    // ========================================================================
    
    /**
     * @brief Record Ingestion.
     * @param[in] mime_type Input parameter.
     * @param[in] size_bytes Input parameter.
     */
    void recordIngestion(const std::string& mime_type, uint64_t size_bytes);
    
    /**
     * @brief Record Validation.
     * @param[in] success Input parameter.
     */
    void recordValidation(bool success);
    
    /**
     * @brief Record Processing.
     * @param[in] mime_type Input parameter.
     * @param[in] success Input parameter.
     */
    void recordProcessing(const std::string& mime_type, bool success);
    
    /**
     * @brief Record Extraction.
     * @param[in] success Input parameter.
     */
    void recordExtraction(bool success);
    
    /**
     * @brief Record Chunking.
     * @param[in] chunk_count Input parameter.
     */
    void recordChunking(uint64_t chunk_count);
    
    /**
     * @brief Record Embedding.
     * @param[in] count Input parameter.
     */
    void recordEmbedding(uint64_t count);

    /**
     * @brief Record Embedding Failure.
     */
    void recordEmbeddingFailure();

    /**
     * @brief Record Dedup Check.
     */
    void recordDedupCheck();

    /**
     * @brief Record Dedup Hit.
     */
    void recordDedupHit();

    /**
     * @brief ======================================================================== Format-specific Metrics (content_pdf_extracted_total, etc.
     * @details ) ========================================================================
     */

    void recordPdfExtracted();

    /**
     * @brief Record Office Extracted.
     */
    void recordOfficeExtracted();

    /**
     * @brief Record Ocr Extracted.
     */
    void recordOcrExtracted();

    /**
     * @brief Record Extract Error.
     */
    void recordExtractError();
    
    // ========================================================================
    // Latency Metrics
    // ========================================================================
    
    /**
     * @brief Record Latency.
     * @param[in] operation Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordLatency(const std::string& operation, double latency_ms);
    
    std::map<std::string, double> getLatencyPercentiles(const std::string& operation) const;
    
    // ========================================================================
    // Error Metrics
    // ========================================================================
    
    /**
     * @brief Record Error.
     * @param[in] error_code Input parameter.
     */
    void recordError(int error_code);
    
    /**
     * @brief Record Error Category.
     * @param[in] category Input parameter.
     */
    void recordErrorCategory(const std::string& category);
    
    /**
     * @brief Record Timeout.
     * @param[in] operation Input parameter.
     */
    void recordTimeout(const std::string& operation);
    
    // ========================================================================
    // Validation Metrics
    // ========================================================================
    
    /**
     * @brief Record Validation Violation.
     * @param[in] violation_type Input parameter.
     */
    void recordValidationViolation(const std::string& violation_type);
    
    // ========================================================================
    // Cache Metrics
    // ========================================================================
    
    /**
     * @brief Record Cache Hit.
     */
    void recordCacheHit();
    
    /**
     * @brief Record Cache Miss.
     */
    void recordCacheMiss();
    
    /**
     * @brief Get Cache Hit Rate.
     * @return Return value.
     */
    double getCacheHitRate() const;
    
    // ========================================================================
    // Format Distribution
    // ========================================================================
    
    /**
     * @brief Get Count By Mime Type.
     * @param[in] mime_type Input parameter.
     * @return Return value.
     */
    uint64_t getCountByMimeType(const std::string& mime_type) const;
    
    std::map<std::string, uint64_t> getMimeTypeCounts() const;
    
    // ========================================================================
    // Aggregated Metrics
    // ========================================================================
    
    uint64_t getTotalIngestions() const { return total_ingestions_.load(); }
    
    uint64_t getTotalBytesProcessed() const { return total_bytes_processed_.load(); }
    
    uint64_t getTotalValidations() const { return total_validations_.load(); }
    
    uint64_t getSuccessfulValidations() const { return successful_validations_.load(); }
    
    uint64_t getFailedValidations() const { return failed_validations_.load(); }
    
    /**
     * @brief Get Validation Success Rate.
     * @return Return value.
     */
    double getValidationSuccessRate() const;
    
    uint64_t getTotalProcessing() const { return total_processing_.load(); }
    
    uint64_t getSuccessfulProcessing() const { return successful_processing_.load(); }
    
    uint64_t getFailedProcessing() const { return failed_processing_.load(); }
    
    /**
     * @brief Get Processing Success Rate.
     * @return Return value.
     */
    double getProcessingSuccessRate() const;
    
    uint64_t getTotalErrors() const { return total_errors_.load(); }
    
    uint64_t getTotalTimeouts() const { return total_timeouts_.load(); }

    uint64_t getPdfExtractedTotal() const { return pdf_extracted_total_.load(); }

    uint64_t getOfficeExtractedTotal() const { return office_extracted_total_.load(); }

    uint64_t getOcrExtractedTotal() const { return ocr_extracted_total_.load(); }

    uint64_t getExtractErrorsTotal() const { return extract_errors_total_.load(); }

    uint64_t getEmbeddingFailuresTotal() const { return embedding_failures_.load(); }

    uint64_t getDedupChecksTotal() const { return dedup_checks_.load(); }

    uint64_t getDedupHitsTotal() const { return dedup_hits_.load(); }
    
    // ========================================================================
    // Export & Reset
    // ========================================================================
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
    
    /**
     * @brief To Prometheus Format.
     * @return Return value.
     */
    std::string toPrometheusFormat() const;
    
    /**
     * @brief Reset the modification detection flag.
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
    /**
     * @brief Record Latency Internal.
     * @param[in] operation Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordLatencyInternal(const std::string& operation, double latency_ms);
    /**
     * @brief Calculate Percentile.
     * @param[in] sorted_samples Input parameter.
     * @param[in] percentile Input parameter.
     * @return Return value.
     */
    double calculatePercentile(const std::vector<double>& sorted_samples, double percentile) const;
};

} // namespace content
} // namespace themis
