/**
 * @file async_metrics_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/cycle_metrics.h"
#include "performance/cycle_metrics_config.h"
#include "performance/lockfree_metrics_buffer.h"
#include "performance/runtime_config.h"
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <map>
#include <set>

namespace themis {
namespace performance {

/**
 * @brief Exporter entry points implemented in prometheus_exporter.
 * @param[in] metrics_list Input parameter.
 * @return Return value.
 * @details cpp / chimera_exporter.cpp.
 */
std::string exportPrometheusMetrics(const std::vector<MetricsEntry>& metrics_list);
/**
 * @brief Export Chimera Metrics.
 * @param[in] metrics_list Input parameter.
 * @return Return value.
 */
std::string exportChimeraMetrics(const std::vector<MetricsEntry>& metrics_list);

class CycleMetricsCollector {
public:
    /**
     * @brief Instance.
     * @return Return value.
     * @details Implements instance without additional internal calls.
     */
    static CycleMetricsCollector& instance() {
        static CycleMetricsCollector instance;
        return instance;
    }

    /**
     * @brief Record Operation.
     * @param[in] operation_name Name of the operation.
     * @param[in] metrics Input parameter.
     * @details Calls: RuntimeConfig::instance(), isOperationEnabled(), shouldMeasure(), BufferWrapper(), collector(), invalidate(), deregisterThreadBuffer(), wrapper().
     */
    void recordOperation(const std::string& operation_name, const OperationCycleMetrics& metrics) {
        // Check if this operation is enabled
        if (!RuntimeConfig::instance().isOperationEnabled(operation_name)) {
            return;
        }
        
        // Check sampling
        if (!RuntimeConfig::instance().shouldMeasure()) {
            return;
        }
        
        // Get thread-local buffer with RAII deregistration wrapper
        struct BufferWrapper {
            ThreadLocalMetricsBuffer buffer;
            CycleMetricsCollector* collector;
            bool registered = false;
            
            BufferWrapper(CycleMetricsCollector* c) : collector(c) {}
            
            ~BufferWrapper() {
                if (registered) {
                    // Mark buffer as invalid and deregister on thread exit
                    buffer.invalidate();
                    collector->deregisterThreadBuffer(&buffer);
                }
            }
        };
        
        thread_local BufferWrapper wrapper(this);
        
        // Register buffer on first use
        if (!wrapper.registered) {
            registerThreadBuffer(&wrapper.buffer);
            wrapper.registered = true;
        }
        
        // Record to buffer
        if (!wrapper.buffer.recordOperation(operation_name, metrics)) {
            // Buffer full - metric dropped
            dropped_metrics_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    /**
     * @brief Record Function Cycles.
     * @param[in] function_name Name of the function.
     * @param[in] cycles Input parameter.
     * @details Calls: lock(), push_back().
     */
    void recordFunctionCycles(const std::string& function_name, uint64_t cycles) {
        std::lock_guard<std::mutex> lock(function_metrics_mutex_);
        function_metrics_[function_name].push_back(cycles);
    }

    void start(int export_interval_seconds = 1) {
        if (running_.load(std::memory_order_acquire)) {
            return;
        }
        
        running_.store(true, std::memory_order_release);
        export_interval_ = export_interval_seconds;
        
        export_thread_ = std::thread([this]() {
            exportLoop();
        });
    }

    /**
     * @brief Stop.
     * @details Calls: load(), store(), notify_all(), joinable(), join().
     */
    void stop() {
        if (!running_.load(std::memory_order_acquire)) {
            return;
        }
        
        running_.store(false, std::memory_order_release);
        cv_.notify_all();
        
        if (export_thread_.joinable()) {
            export_thread_.join();
        }
    }

    /**
     * @brief Get Prometheus Metrics.
     * @return Return value.
     * @details Calls: lock(), exportPrometheusMetrics().
     */
    std::string getPrometheusMetrics() {
        std::lock_guard<std::mutex> lock(aggregated_metrics_mutex_);
        return exportPrometheusMetrics(aggregated_metrics_);
    }

    /**
     * @brief Get CHIMERAMetrics.
     * @return Return value.
     * @details Calls: lock(), exportChimeraMetrics().
     */
    std::string getCHIMERAMetrics() {
        std::lock_guard<std::mutex> lock(aggregated_metrics_mutex_);
        return exportChimeraMetrics(aggregated_metrics_);
    }

    uint64_t getDroppedMetrics() const {
        return dropped_metrics_.load(std::memory_order_relaxed);
    }

    ~CycleMetricsCollector() {
        stop();
    }

private:
    CycleMetricsCollector() 
        : running_(false), export_interval_(1), dropped_metrics_(0) {}

    /**
     * @brief Register Thread Buffer.
     * @param[in,out] buffer Input/output parameter.
     * @details Calls: lock(), insert().
     */
    void registerThreadBuffer(ThreadLocalMetricsBuffer* buffer) {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        thread_buffers_.insert(buffer);
    }
    
    /**
     * @brief Deregister Thread Buffer.
     * @param[in,out] buffer Input/output parameter.
     * @details Calls: lock(), erase().
     */
    void deregisterThreadBuffer(ThreadLocalMetricsBuffer* buffer) {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        thread_buffers_.erase(buffer);
    }

    /**
     * @brief Export Loop.
     * @details Calls: load(), lock(), wait_for(), std::chrono::seconds(), drainAllBuffers().
     */
    void exportLoop() {
        while (running_.load(std::memory_order_acquire)) {
            // Wait for interval or stop signal
            std::unique_lock<std::mutex> lock(cv_mutex_);
            cv_.wait_for(lock, std::chrono::seconds(export_interval_), [this]() {
                return !running_.load(std::memory_order_acquire);
            });
            
            if (!running_.load(std::memory_order_acquire)) {
                break;
            }
            
            // Drain all thread buffers
            drainAllBuffers();
        }
    }

    /**
     * @brief Drain All Buffers.
     * @details Calls: lock(), reserve(), size(), assign(), begin(), end(), is_valid(), drain().
     */
    void drainAllBuffers() {
        std::vector<MetricsEntry> new_metrics;
        
        // Take a snapshot of thread buffers to minimize lock hold time
        std::vector<ThreadLocalMetricsBuffer*> buffers_snapshot;
        {
            std::lock_guard<std::mutex> lock(buffers_mutex_);
            buffers_snapshot.reserve(thread_buffers_.size());
            buffers_snapshot.assign(thread_buffers_.begin(), thread_buffers_.end());
        }
        // Lock released - drain buffers without holding the lock
        
        // Drain all buffers lock-free (each buffer is lock-free SPSC)
        // Buffer validity is checked in drain() to prevent use-after-free
        for (auto* buffer : buffers_snapshot) {
            if (buffer && buffer->is_valid()) {
                buffer->drain(new_metrics);
            }
        }
        
        // Aggregate new metrics using atomic swap pattern
        if (!new_metrics.empty()) {
            std::lock_guard<std::mutex> lock(aggregated_metrics_mutex_);
            
            // Keep last N entries (e.g., 10000)
            constexpr size_t MAX_ENTRIES = 10000;
            if (aggregated_metrics_.size() + new_metrics.size() > MAX_ENTRIES) {
                size_t to_remove = aggregated_metrics_.size() + new_metrics.size() - MAX_ENTRIES;
                aggregated_metrics_.erase(
                    aggregated_metrics_.begin(),
                    aggregated_metrics_.begin() + to_remove
                );
            }
            
            aggregated_metrics_.insert(
                aggregated_metrics_.end(),
                new_metrics.begin(),
                new_metrics.end()
            );
        }
    }

    std::atomic<bool> running_;
    int export_interval_;
    std::thread export_thread_;
    
    std::mutex cv_mutex_;
    std::condition_variable cv_;
    
    std::mutex buffers_mutex_;
    std::set<ThreadLocalMetricsBuffer*> thread_buffers_;
    
    std::mutex aggregated_metrics_mutex_;
    std::vector<MetricsEntry> aggregated_metrics_;
    
    std::mutex function_metrics_mutex_;
    std::map<std::string, std::vector<uint64_t>> function_metrics_;
    
    std::atomic<uint64_t> dropped_metrics_;
};

class AsyncMetricsExporter {
public:
    /**
     * @brief Instance.
     * @return Return value.
     * @details Implements instance without additional internal calls.
     */
    static AsyncMetricsExporter& instance() {
        static AsyncMetricsExporter instance;
        return instance;
    }

    void start(int export_interval_seconds = 1) {
        CycleMetricsCollector::instance().start(export_interval_seconds);
    }

    /**
     * @brief Stop.
     * @details Calls: CycleMetricsCollector::instance().
     */
    void stop() {
        CycleMetricsCollector::instance().stop();
    }

    /**
     * @brief Get Prometheus Metrics.
     * @return Return value.
     * @details Calls: CycleMetricsCollector::instance().
     */
    std::string getPrometheusMetrics() {
        return CycleMetricsCollector::instance().getPrometheusMetrics();
    }

    /**
     * @brief Get CHIMERAMetrics.
     * @return Return value.
     * @details Calls: CycleMetricsCollector::instance().
     */
    std::string getCHIMERAMetrics() {
        return CycleMetricsCollector::instance().getCHIMERAMetrics();
    }

    uint64_t getDroppedMetrics() const {
        return CycleMetricsCollector::instance().getDroppedMetrics();
    }

private:
    AsyncMetricsExporter() = default;
    ~AsyncMetricsExporter() = default;
};

} // namespace performance
} // namespace themis
