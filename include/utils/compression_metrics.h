#pragma once

#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <unordered_map>

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
        std::atomic<uint64_t> bytes_in{0};
        std::atomic<uint64_t> bytes_out{0};
        std::atomic<uint64_t> compression_time_us{0};
        std::atomic<uint64_t> decompression_time_us{0};
        std::atomic<uint64_t> compression_count{0};
        std::atomic<uint64_t> decompression_count{0};
        
        double compression_ratio() const {
            uint64_t in = bytes_in.load();
            uint64_t out = bytes_out.load();
            return out > 0 ? static_cast<double>(in) / out : 0.0;
        }
        
        double avg_compression_time_ms() const {
            uint64_t count = compression_count.load();
            return count > 0 ? compression_time_us.load() / 1000.0 / count : 0.0;
        }
        
        double avg_decompression_time_ms() const {
            uint64_t count = decompression_count.load();
            return count > 0 ? decompression_time_us.load() / 1000.0 / count : 0.0;
        }
        
        double compression_throughput_mbps() const {
            uint64_t time_us = compression_time_us.load();
            uint64_t bytes = bytes_in.load();
            return time_us > 0 ? (bytes / 1024.0 / 1024.0) / (time_us / 1000000.0) : 0.0;
        }
    };
    
    static CompressionMetrics& instance() {
        static CompressionMetrics metrics;
        return metrics;
    }
    
    // Record compression operation
    void record_compression(
        const std::string& method,
        size_t bytes_in,
        size_t bytes_out,
        std::chrono::microseconds duration
    ) {
        auto& stats = get_stats(method);
        stats.bytes_in.fetch_add(bytes_in);
        stats.bytes_out.fetch_add(bytes_out);
        stats.compression_time_us.fetch_add(duration.count());
        stats.compression_count.fetch_add(1);
    }
    
    // Record decompression operation
    void record_decompression(
        const std::string& method,
        size_t bytes_in,
        size_t bytes_out,
        std::chrono::microseconds duration
    ) {
        auto& stats = get_stats(method);
        stats.bytes_in.fetch_add(bytes_out); // Original size
        stats.bytes_out.fetch_add(bytes_in); // Compressed size
        stats.decompression_time_us.fetch_add(duration.count());
        stats.decompression_count.fetch_add(1);
    }
    
    // Get statistics for a specific method
    MethodStats get_method_stats(const std::string& method) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = stats_.find(method);
        return it != stats_.end() ? it->second : MethodStats{};
    }
    
    // Get all method names
    std::vector<std::string> get_methods() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> methods;
        methods.reserve(stats_.size());
        for (const auto& pair : stats_) {
            methods.push_back(pair.first);
        }
        return methods;
    }
    
    // Reset all statistics
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.clear();
    }
    
    // Get human-readable summary
    std::string get_summary() const;
    
private:
    CompressionMetrics() = default;
    
    MethodStats& get_stats(const std::string& method) {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_[method];
    }
    
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
