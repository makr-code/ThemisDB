/**
 * @file compression_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace themis {
namespace utils {

/**
 * @brief Compression performance metrics tracker
 * 
 * Tracks compression ratios, throughput, and performance across different
 * compression methods and data types.
 */
class CompressionMetrics {
public:
    struct MethodStats {
        uint64_t bytes_in{0};
        uint64_t bytes_out{0};
        uint64_t compression_time_us{0};
        uint64_t decompression_time_us{0};
        uint64_t compression_count{0};
        uint64_t decompression_count{0};
        
        double compression_ratio() const {
            return bytes_out > 0 ? static_cast<double>(bytes_in) / bytes_out : 0.0;
        }
        
        double avg_compression_time_ms() const {
            return compression_count > 0 ? compression_time_us / 1000.0 / compression_count : 0.0;
        }
        
        double avg_decompression_time_ms() const {
            return decompression_count > 0 ? decompression_time_us / 1000.0 / decompression_count : 0.0;
        }
        
        double compression_throughput_mbps() const {
            return compression_time_us > 0 ? (bytes_in / 1024.0 / 1024.0) / (compression_time_us / 1000000.0) : 0.0;
        }
    };
    
    /**
     * @brief TBD: Describe instance.
     * @return Return value.
     * @details Implements instance without additional internal calls.
     */
    static CompressionMetrics& instance() {
        static CompressionMetrics metrics;
        return metrics;
    }
    
    /**
     * @brief Record compression operation
     * @param[in] method Input parameter.
     * @param[in] bytes_in Input parameter.
     * @param[in] bytes_out Input parameter.
     * @param[in] duration Input parameter.
     */
    void record_compression(
        const std::string& method,
        size_t bytes_in,
        size_t bytes_out,
        std::chrono::microseconds duration
    );
    
    /**
     * @brief Record decompression operation
     * @param[in] method Input parameter.
     * @param[in] bytes_in Input parameter.
     * @param[in] bytes_out Input parameter.
     * @param[in] duration Input parameter.
     */
    void record_decompression(
        const std::string& method,
        size_t bytes_in,
        size_t bytes_out,
        std::chrono::microseconds duration
    );
    
    /**
     * @brief Get statistics for a specific method
     * @param[in] method Input parameter.
     * @return Return value.
     */
    MethodStats get_method_stats(const std::string& method) const;
    
    /**
     * @brief Get all method names
     * @return Return value.
     */
    std::vector<std::string> get_methods() const;
    
    /**
     * @brief Reset all statistics
     */
    void reset();
    
    /**
     * @brief Get human-readable summary
     * @return Return value.
     */
    std::string get_summary() const;
    
private:
    CompressionMetrics() = default;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, MethodStats> stats_;
};

/**
 * @brief RAII helper for measuring compression operations
 */
class CompressionTimer {
public:
    CompressionTimer(const std::string& method, size_t input_size, bool is_compression = true)
        : method_(method)
        , input_size_(input_size)
        , is_compression_(is_compression)
        , start_(std::chrono::steady_clock::now())
    {}
    
    ~CompressionTimer() {
        // Intentionally empty - call finish() explicitly
    }
    
    /**
     * @brief TBD: Describe finish.
     * @param[in] output_size Input parameter.
     * @details Calls: std::chrono::steady_clock::now(), CompressionMetrics::instance(), record_compression(), record_decompression().
     */
    void finish(size_t output_size) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        
        if (is_compression_) {
            CompressionMetrics::instance().record_compression(
                method_, input_size_, output_size, duration
            );
        } else {
            CompressionMetrics::instance().record_decompression(
                method_, input_size_, output_size, duration
            );
        }
    }
    
private:
    std::string method_;
    size_t input_size_;
    bool is_compression_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace utils
} // namespace themis
