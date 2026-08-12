/**
 * @file compression_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/compression_metrics.h"
#include <sstream>
#include <iomanip>

namespace themis {
namespace utils {

void CompressionMetrics::record_compression(
    const std::string& method,
    size_t bytes_in,
    size_t bytes_out,
    std::chrono::microseconds duration
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& stats = stats_[method];
    stats.bytes_in += bytes_in;
    stats.bytes_out += bytes_out;
    stats.compression_time_us += duration.count();
    stats.compression_count += 1;
}

void CompressionMetrics::record_decompression(
    const std::string& method,
    size_t bytes_in,
    size_t bytes_out,
    std::chrono::microseconds duration
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& stats = stats_[method];
    stats.bytes_in += bytes_out; // Original size
    stats.bytes_out += bytes_in; // Compressed size
    stats.decompression_time_us += duration.count();
    stats.decompression_count += 1;
}

CompressionMetrics::MethodStats CompressionMetrics::get_method_stats(const std::string& method) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = stats_.find(method);
    return it != stats_.end() ? it->second : MethodStats{};
}

std::vector<std::string> CompressionMetrics::get_methods() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> methods;
    methods.reserve(stats_.size());
    for (const auto& pair : stats_) {
        methods.push_back(pair.first);
    }
    return methods;
}

void CompressionMetrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.clear();
}

std::string CompressionMetrics::get_summary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    
    oss << "=== Compression Metrics Summary ===\n";
    
    if (stats_.empty()) {
        oss << "No compression operations recorded.\n";
        return oss.str();
    }
    
    for (const auto& pair : stats_) {
        const auto& method = pair.first;
        const auto& stats = pair.second;
        
        oss << "\nMethod: " << method << "\n";
        oss << "  Compression Operations: " << stats.compression_count << "\n";
        oss << "  Decompression Operations: " << stats.decompression_count << "\n";
        oss << "  Total Bytes In: " << stats.bytes_in << " (" 
            << std::fixed << std::setprecision(2) 
            << (stats.bytes_in / 1024.0 / 1024.0) << " MB)\n";
        oss << "  Total Bytes Out: " << stats.bytes_out << " (" 
            << std::fixed << std::setprecision(2)
            << (stats.bytes_out / 1024.0 / 1024.0) << " MB)\n";
        oss << "  Compression Ratio: " << std::fixed << std::setprecision(2) 
            << stats.compression_ratio() << "x\n";
        oss << "  Avg Compression Time: " << std::fixed << std::setprecision(3) 
            << stats.avg_compression_time_ms() << " ms\n";
        oss << "  Avg Decompression Time: " << std::fixed << std::setprecision(3) 
            << stats.avg_decompression_time_ms() << " ms\n";
        oss << "  Compression Throughput: " << std::fixed << std::setprecision(2) 
            << stats.compression_throughput_mbps() << " MB/s\n";
    }
    
    return oss.str();
}

} // namespace utils
} // namespace themis
