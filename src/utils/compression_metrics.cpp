#include "utils/compression_metrics.h"
#include <sstream>
#include <iomanip>

namespace themis {
namespace utils {

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
        oss << "  Compression Operations: " << stats.compression_count.load() << "\n";
        oss << "  Decompression Operations: " << stats.decompression_count.load() << "\n";
        oss << "  Total Bytes In: " << stats.bytes_in.load() << " (" 
            << std::fixed << std::setprecision(2) 
            << (stats.bytes_in.load() / 1024.0 / 1024.0) << " MB)\n";
        oss << "  Total Bytes Out: " << stats.bytes_out.load() << " (" 
            << std::fixed << std::setprecision(2)
            << (stats.bytes_out.load() / 1024.0 / 1024.0) << " MB)\n";
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
