/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            exporter_metrics.h                                 ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     171                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    
    // Helper to update latency histogram
    void updateLatencyHistogram(std::chrono::milliseconds duration);
    
    // Helper to calculate percentile from histogram
    double calculatePercentile(double percentile) const;
};

} // namespace themis::exporters
