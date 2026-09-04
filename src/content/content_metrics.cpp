/**
 * @file content_metrics.cpp
 * @brief Metrics collection and aggregation for content processing performance tracking.
 * @version 0.0.47
 * @note Maturity: 🟡 BETA
 * @note Score: 82/100
 * @note Gap Summary: total=7; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=3, C=0, H=1, M=6, L=0
 * @note Status: Production Ready; Metrics collection and aggregation working; advanced dashboards deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/content_metrics.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace themis {
namespace content {

// ============================================================================
// Throughput Metrics
// ============================================================================

void ContentMetrics::recordIngestion(const std::string& mime_type, uint64_t size_bytes) {
    total_ingestions_++;
    total_bytes_processed_ += size_bytes;
    
    std::lock_guard<std::mutex> lock(mime_mutex_);
    mime_type_counts_[mime_type]++;
}

void ContentMetrics::recordValidation([[maybe_unused]] bool success) {
    total_validations_++;
    if (success) {
        successful_validations_++;
    } else {
        failed_validations_++;
    }
}

void ContentMetrics::recordProcessing(const std::string& /*mime_type*/, bool success) {
    total_processing_++;
    if (success) {
        successful_processing_++;
    } else {
        failed_processing_++;
    }
}

void ContentMetrics::recordExtraction([[maybe_unused]] bool success) {
    total_extractions_++;
    if (success) {
        successful_extractions_++;
    } else {
        failed_extractions_++;
    }
}

void ContentMetrics::recordChunking([[maybe_unused]] uint64_t chunk_count) {
    total_chunks_ += chunk_count;
}

void ContentMetrics::recordEmbedding([[maybe_unused]] uint64_t count) {
    total_embeddings_ += count;
}

void ContentMetrics::recordEmbeddingFailure() {
    embedding_failures_++;
}

void ContentMetrics::recordDedupCheck() {
    dedup_checks_++;
}

void ContentMetrics::recordDedupHit() {
    dedup_hits_++;
}

void ContentMetrics::recordPdfExtracted() {
    pdf_extracted_total_++;
}

void ContentMetrics::recordOfficeExtracted() {
    office_extracted_total_++;
}

void ContentMetrics::recordOcrExtracted() {
    ocr_extracted_total_++;
}

void ContentMetrics::recordExtractError() {
    extract_errors_total_++;
}

// ============================================================================
// Latency Metrics
// ============================================================================

void ContentMetrics::recordLatency(const std::string& operation, double latency_ms) {
    recordLatencyInternal(operation, latency_ms);
}

void ContentMetrics::recordLatencyInternal(const std::string& operation, double latency_ms) {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    auto& stats = latency_stats_[operation];
    
    stats.samples.push_back(latency_ms);
    stats.sum += latency_ms;
    stats.count++;
    
    if (stats.count == 1) {
        stats.min = latency_ms;
        stats.max = latency_ms;
    } else {
        stats.min = std::min(stats.min, latency_ms);
        stats.max = std::max(stats.max, latency_ms);
    }
}

std::map<std::string, double> ContentMetrics::getLatencyPercentiles(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(latency_mutex_);
    std::map<std::string, double> result;
    
    auto it = latency_stats_.find(operation);
    if (it == latency_stats_.end() || it->second.samples.empty()) {
        return result;
    }
    
    const auto& stats = it->second;
    auto sorted = stats.samples;
    std::sort(sorted.begin(), sorted.end());
    
    result["min"] = stats.min;
    result["max"] = stats.max;
    result["avg"] = stats.count > 0 ? stats.sum / stats.count : 0.0;
    result["p50"] = calculatePercentile(sorted, 0.50);
    result["p95"] = calculatePercentile(sorted, 0.95);
    result["p99"] = calculatePercentile(sorted, 0.99);
    result["count"] = static_cast<double>(stats.count);
    
    return result;
}

double ContentMetrics::calculatePercentile(const std::vector<double>& sorted_samples, double percentile) const {
    if (sorted_samples.empty()) {
      return 0.0;
    }
    
    double index = percentile * (static_cast<int>(sorted_samples.size()) - 1);
    size_t lower = static_cast<size_t>(std::floor(index));
    size_t upper = static_cast<size_t>(std::ceil(index));
    
    if (lower == upper) {
        return sorted_samples[lower];
    }
    
    double weight = index - lower;
    return sorted_samples[lower] * (1.0 - weight) + sorted_samples[upper] * weight;
}

// ============================================================================
// Error Metrics
// ============================================================================

void ContentMetrics::recordError([[maybe_unused]] int error_code) {
    total_errors_++;
    
    std::lock_guard<std::mutex> lock(error_mutex_);
    error_code_counts_[error_code]++;
}

void ContentMetrics::recordErrorCategory(const std::string& category) {
    std::lock_guard<std::mutex> lock(error_category_mutex_);
    error_category_counts_[category]++;
}

void ContentMetrics::recordTimeout(const std::string& operation) {
    total_timeouts_++;
    
    std::lock_guard<std::mutex> lock(timeout_mutex_);
    timeout_counts_[operation]++;
}

// ============================================================================
// Validation Metrics
// ============================================================================

void ContentMetrics::recordValidationViolation(const std::string& violation_type) {
    std::lock_guard<std::mutex> lock(violation_mutex_);
    violation_counts_[violation_type]++;
}

// ============================================================================
// Cache Metrics
// ============================================================================

void ContentMetrics::recordCacheHit() {
    cache_hits_++;
}

void ContentMetrics::recordCacheMiss() {
    cache_misses_++;
}

double ContentMetrics::getCacheHitRate() const {
    uint64_t hits = cache_hits_.load();
    uint64_t misses = cache_misses_.load();
    uint64_t total = hits + misses;
    
    if (total == 0) {
      return 0.0;
    }
    return (static_cast<double>(hits) / total) * 100.0;
}

// ============================================================================
// Format Distribution
// ============================================================================

uint64_t ContentMetrics::getCountByMimeType(const std::string& mime_type) const {
    std::lock_guard<std::mutex> lock(mime_mutex_);
    auto it = mime_type_counts_.find(mime_type);
    return it != mime_type_counts_.end() ? it->second : 0;
}

std::map<std::string, uint64_t> ContentMetrics::getMimeTypeCounts() const {
    std::lock_guard<std::mutex> lock(mime_mutex_);
    return mime_type_counts_;
}

// ============================================================================
// Aggregated Metrics
// ============================================================================

double ContentMetrics::getValidationSuccessRate() const {
    uint64_t total = total_validations_.load();
    if (total == 0) {
      return 100.0;
    }
    
    uint64_t successful = successful_validations_.load();
    return (static_cast<double>(successful) / total) * 100.0;
}

double ContentMetrics::getProcessingSuccessRate() const {
    uint64_t total = total_processing_.load();
    if (total == 0) {
      return 100.0;
    }
    
    uint64_t successful = successful_processing_.load();
    return (static_cast<double>(successful) / total) * 100.0;
}

// ============================================================================
// Export & Reset
// ============================================================================

json ContentMetrics::toJson() const {
    json j;
    
    // Throughput
    j["throughput"]["total_ingestions"] = total_ingestions_.load();
    j["throughput"]["total_bytes_processed"] = total_bytes_processed_.load();
    j["throughput"]["total_validations"] = total_validations_.load();
    j["throughput"]["successful_validations"] = successful_validations_.load();
    j["throughput"]["failed_validations"] = failed_validations_.load();
    j["throughput"]["total_processing"] = total_processing_.load();
    j["throughput"]["successful_processing"] = successful_processing_.load();
    j["throughput"]["failed_processing"] = failed_processing_.load();
    j["throughput"]["total_extractions"] = total_extractions_.load();
    j["throughput"]["successful_extractions"] = successful_extractions_.load();
    j["throughput"]["failed_extractions"] = failed_extractions_.load();
    j["throughput"]["total_chunks"] = total_chunks_.load();
    j["throughput"]["total_embeddings"] = total_embeddings_.load();
    j["throughput"]["embedding_failures"] = embedding_failures_.load();
    j["throughput"]["dedup_checks"] = dedup_checks_.load();
    j["throughput"]["dedup_hits"] = dedup_hits_.load();
    
    // Success rates
    j["rates"]["validation_success_rate"] = getValidationSuccessRate();
    j["rates"]["processing_success_rate"] = getProcessingSuccessRate();
    j["rates"]["cache_hit_rate"] = getCacheHitRate();
    
    // Errors
    j["errors"]["total_errors"] = total_errors_.load();
    j["errors"]["total_timeouts"] = total_timeouts_.load();
    j["errors"]["extract_errors_total"] = extract_errors_total_.load();

    // Format-specific extraction stats
    j["format_stats"]["pdf_extracted_total"] = pdf_extracted_total_.load();
    j["format_stats"]["office_extracted_total"] = office_extracted_total_.load();
    j["format_stats"]["ocr_extracted_total"] = ocr_extracted_total_.load();
    
    {
        std::lock_guard<std::mutex> lock(error_mutex_);
        j["errors"]["by_code"] = error_code_counts_;
    }
    
    {
        std::lock_guard<std::mutex> lock(error_category_mutex_);
        j["errors"]["by_category"] = error_category_counts_;
    }
    
    {
        std::lock_guard<std::mutex> lock(timeout_mutex_);
        j["errors"]["timeouts_by_operation"] = timeout_counts_;
    }
    
    // Cache
    j["cache"]["hits"] = cache_hits_.load();
    j["cache"]["misses"] = cache_misses_.load();
    
    // MIME types
    {
        std::lock_guard<std::mutex> lock(mime_mutex_);
        j["mime_types"] = mime_type_counts_;
    }
    
    // Violations
    {
        std::lock_guard<std::mutex> lock(violation_mutex_);
        j["validation_violations"] = violation_counts_;
    }
    
    // Latency
    std::lock_guard<std::mutex> lock(latency_mutex_);
    for (const auto& [operation, stats] : latency_stats_) {
        auto sorted = stats.samples;
        std::sort(sorted.begin(), sorted.end());
        
        j["latency"][operation]["min"] = stats.min;
        j["latency"][operation]["max"] = stats.max;
        j["latency"][operation]["avg"] = stats.count > 0 ? stats.sum / stats.count : 0.0;
        j["latency"][operation]["p50"] = calculatePercentile(sorted, 0.50);
        j["latency"][operation]["p95"] = calculatePercentile(sorted, 0.95);
        j["latency"][operation]["p99"] = calculatePercentile(sorted, 0.99);
        j["latency"][operation]["count"] = stats.count;
    }
    
    return j;
}

std::string ContentMetrics::toPrometheusFormat() const {
    std::ostringstream oss = {};
    
    // Throughput metrics
    oss << "# HELP content_ingestions_total Total number of content items ingested\n";
    oss << "# TYPE content_ingestions_total counter\n";
    oss << "content_ingestions_total " << total_ingestions_.load() << "\n\n";
    
    oss << "# HELP content_bytes_processed_total Total bytes of content processed\n";
    oss << "# TYPE content_bytes_processed_total counter\n";
    oss << "content_bytes_processed_total " << total_bytes_processed_.load() << "\n\n";
    
    oss << "# HELP content_validations_total Total content validation attempts\n";
    oss << "# TYPE content_validations_total counter\n";
    oss << "content_validations_total{status=\"success\"} " << successful_validations_.load() << "\n";
    oss << "content_validations_total{status=\"failure\"} " << failed_validations_.load() << "\n\n";
    
    oss << "# HELP content_processing_total Total content processing attempts\n";
    oss << "# TYPE content_processing_total counter\n";
    oss << "content_processing_total{status=\"success\"} " << successful_processing_.load() << "\n";
    oss << "content_processing_total{status=\"failure\"} " << failed_processing_.load() << "\n\n";
    
    oss << "# HELP content_chunks_total Total chunks created\n";
    oss << "# TYPE content_chunks_total counter\n";
    oss << "content_chunks_total " << total_chunks_.load() << "\n\n";
    
    oss << "# HELP content_embeddings_total Total embeddings generated\n";
    oss << "# TYPE content_embeddings_total counter\n";
    oss << "content_embeddings_total " << total_embeddings_.load() << "\n\n";
    
    // Error metrics
    oss << "# HELP content_errors_total Total content processing errors\n";
    oss << "# TYPE content_errors_total counter\n";
    oss << "content_errors_total " << total_errors_.load() << "\n\n";
    
    oss << "# HELP content_timeouts_total Total content processing timeouts\n";
    oss << "# TYPE content_timeouts_total counter\n";
    oss << "content_timeouts_total " << total_timeouts_.load() << "\n\n";

    // Format-specific counters
    oss << "# HELP content_pdf_extracted_total Total successfully extracted PDF documents\n";
    oss << "# TYPE content_pdf_extracted_total counter\n";
    oss << "content_pdf_extracted_total " << pdf_extracted_total_.load() << "\n\n";

    oss << "# HELP content_office_extracted_total Total successfully extracted Office documents\n";
    oss << "# TYPE content_office_extracted_total counter\n";
    oss << "content_office_extracted_total " << office_extracted_total_.load() << "\n\n";

    oss << "# HELP content_ocr_extracted_total Total successfully OCR-processed images\n";
    oss << "# TYPE content_ocr_extracted_total counter\n";
    oss << "content_ocr_extracted_total " << ocr_extracted_total_.load() << "\n\n";

    oss << "# HELP content_extract_errors_total Total PDF/document extraction errors\n";
    oss << "# TYPE content_extract_errors_total counter\n";
    oss << "content_extract_errors_total " << extract_errors_total_.load() << "\n\n";

    oss << "# HELP content_embedding_failures_total Total embedding generation failures (timeout or model error)\n";
    oss << "# TYPE content_embedding_failures_total counter\n";
    oss << "content_embedding_failures_total " << embedding_failures_.load() << "\n\n";

    oss << "# HELP content_dedup_checks_total Total perceptual deduplication checks performed\n";
    oss << "# TYPE content_dedup_checks_total counter\n";
    oss << "content_dedup_checks_total " << dedup_checks_.load() << "\n\n";

    oss << "# HELP content_dedup_hits_total Total near-duplicate items detected and rejected\n";
    oss << "# TYPE content_dedup_hits_total counter\n";
    oss << "content_dedup_hits_total " << dedup_hits_.load() << "\n\n";
    
    // Cache metrics
    oss << "# HELP content_cache_requests_total Total cache requests\n";
    oss << "# TYPE content_cache_requests_total counter\n";
    oss << "content_cache_requests_total{result=\"hit\"} " << cache_hits_.load() << "\n";
    oss << "content_cache_requests_total{result=\"miss\"} " << cache_misses_.load() << "\n\n";
    
    // Success rates (as gauges)
    oss << "# HELP content_validation_success_rate Validation success rate percentage\n";
    oss << "# TYPE content_validation_success_rate gauge\n";
    oss << "content_validation_success_rate " << std::fixed << std::setprecision(2) 
        << getValidationSuccessRate() << "\n\n";
    
    oss << "# HELP content_processing_success_rate Processing success rate percentage\n";
    oss << "# TYPE content_processing_success_rate gauge\n";
    oss << "content_processing_success_rate " << std::fixed << std::setprecision(2) 
        << getProcessingSuccessRate() << "\n\n";
    
    oss << "# HELP content_cache_hit_rate Cache hit rate percentage\n";
    oss << "# TYPE content_cache_hit_rate gauge\n";
    oss << "content_cache_hit_rate " << std::fixed << std::setprecision(2) 
        << getCacheHitRate() << "\n\n";
    
    // MIME type distribution
    {
        std::lock_guard<std::mutex> lock(mime_mutex_);
        if (!mime_type_counts_.empty()) {
            oss << "# HELP content_by_mime_type_total Content items by MIME type\n";
            oss << "# TYPE content_by_mime_type_total counter\n";
            for (const auto& [mime, count] : mime_type_counts_) {
                oss << "content_by_mime_type_total{mime_type=\"" << mime << "\"} " << count << "\n";
            }
            oss << "\n";
        }
    }
    
    // Error categories
    {
        std::lock_guard<std::mutex> lock(error_category_mutex_);
        if (!error_category_counts_.empty()) {
            oss << "# HELP content_errors_by_category_total Errors by category\n";
            oss << "# TYPE content_errors_by_category_total counter\n";
            for (const auto& [category, count] : error_category_counts_) {
                oss << "content_errors_by_category_total{category=\"" << category << "\"} " << count << "\n";
            }
            oss << "\n";
        }
    }
    
    // Latency percentiles
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        for (const auto& [operation, stats] : latency_stats_) {
            if (stats.samples.empty()) {
              continue;
            }
            
            auto sorted = stats.samples;
            std::sort(sorted.begin(), sorted.end());
            
            oss << "# HELP content_" << operation << "_latency_ms Content " << operation << " latency in milliseconds\n";
            oss << "# TYPE content_" << operation << "_latency_ms summary\n";
            oss << "content_" << operation << "_latency_ms{quantile=\"0.5\"} " << calculatePercentile(sorted, 0.50) << "\n";
            oss << "content_" << operation << "_latency_ms{quantile=\"0.95\"} " << calculatePercentile(sorted, 0.95) << "\n";
            oss << "content_" << operation << "_latency_ms{quantile=\"0.99\"} " << calculatePercentile(sorted, 0.99) << "\n";
            oss << "content_" << operation << "_latency_ms_sum " << stats.sum << "\n";
            oss << "content_" << operation << "_latency_ms_count " << stats.count << "\n\n";
        }
    }
    
    return oss.str();
}

void ContentMetrics::reset() {
    // Reset atomic counters
    total_ingestions_ = 0;
    total_bytes_processed_ = 0;
    total_validations_ = 0;
    successful_validations_ = 0;
    failed_validations_ = 0;
    total_processing_ = 0;
    successful_processing_ = 0;
    failed_processing_ = 0;
    total_extractions_ = 0;
    successful_extractions_ = 0;
    failed_extractions_ = 0;
    total_chunks_ = 0;
    total_embeddings_ = 0;
    total_errors_ = 0;
    total_timeouts_ = 0;
    pdf_extracted_total_ = 0;
    office_extracted_total_ = 0;
    ocr_extracted_total_ = 0;
    extract_errors_total_ = 0;
    embedding_failures_ = 0;
    dedup_checks_ = 0;
    dedup_hits_ = 0;
    cache_hits_ = 0;
    cache_misses_ = 0;
    
    // Reset maps
    {
        std::lock_guard<std::mutex> lock(mime_mutex_);
        mime_type_counts_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(error_mutex_);
        error_code_counts_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(error_category_mutex_);
        error_category_counts_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(violation_mutex_);
        violation_counts_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(timeout_mutex_);
        timeout_counts_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        latency_stats_.clear();
    }
}

} // namespace content
} // namespace themis
