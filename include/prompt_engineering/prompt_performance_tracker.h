/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_performance_tracker.h                       ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:54:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     197                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_performance_tracker.h
 * @brief Performance tracking for prompt templates
 * 
 * Tracks execution metrics for prompts including:
 * - Success rates
 * - Latency statistics
 * - User feedback scores
 * - Execution history
 * 
 * This is the data foundation for autonomous prompt optimization.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <mutex>
#include <optional>
#include <nlohmann/json.hpp>

namespace rocksdb { class ColumnFamilyHandle; }

namespace themis {
class RocksDBWrapper;

namespace prompt_engineering {

/**
 * @brief Metrics for a single prompt template
 */
struct PromptMetrics {
    std::string prompt_id;                          ///< Prompt template ID
    double success_rate = 0.0;                      ///< Success rate (0.0-1.0)
    double avg_latency_ms = 0.0;                    ///< Average latency in milliseconds
    double user_satisfaction = 0.0;                 ///< User feedback score (0.0-1.0)
    size_t total_executions = 0;                    ///< Total number of executions
    size_t failed_executions = 0;                   ///< Number of failed executions
    size_t feedback_count = 0;                      ///< Number of feedback entries received
    std::chrono::system_clock::time_point last_updated;  ///< Last update timestamp
    std::chrono::system_clock::time_point created_at;    ///< Creation timestamp
    
    /**
     * @brief Convert metrics to JSON
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Parse metrics from JSON
     */
    static PromptMetrics fromJson(const nlohmann::json& j);
};

/**
 * @brief Performance tracker for prompt templates
 * 
 * Tracks and aggregates performance metrics for prompts:
 * - Records execution results (success/failure, latency)
 * - Collects user feedback
 * - Identifies low-performing prompts
 * - Provides data for optimization decisions
 * 
 * Thread-safe for concurrent metric recording.
 */
class PromptPerformanceTracker {
public:
    /**
     * @brief Constructor for in-memory tracking
     */
    PromptPerformanceTracker();
    
    /**
     * @brief Constructor with RocksDB persistence
     * @param db RocksDB wrapper (not owned)
     * @param cf Column family handle (optional, uses default if null)
     */
    PromptPerformanceTracker(RocksDBWrapper* db, rocksdb::ColumnFamilyHandle* cf = nullptr);
    
    /**
     * @brief Record a prompt execution result
     * @param prompt_id Prompt template ID
     * @param success Whether execution succeeded
     * @param latency_ms Execution latency in milliseconds
     * @param user_feedback Optional user feedback score (0.0-1.0)
     */
    void recordExecution(
        const std::string& prompt_id,
        bool success,
        double latency_ms,
        double user_feedback = 0.0
    );
    
    /**
     * @brief Get metrics for a specific prompt
     * @param prompt_id Prompt template ID
     * @return Metrics if found, nullopt otherwise
     */
    std::optional<PromptMetrics> getMetrics(const std::string& prompt_id) const;
    
    /**
     * @brief Get all tracked prompt metrics
     * @return Vector of all prompt metrics
     */
    std::vector<PromptMetrics> getAllMetrics() const;
    
    /**
     * @brief Identify low-performing prompts
     * @param threshold Minimum acceptable success rate (default 0.7)
     * @param min_executions Minimum executions required for consideration (default 10)
     * @return Vector of prompt IDs with success rate below threshold
     */
    std::vector<std::string> getLowPerformingPrompts(
        double threshold = 0.7,
        size_t min_executions = 10
    ) const;
    
    /**
     * @brief Get top performing prompts
     * @param count Number of top prompts to return
     * @param min_executions Minimum executions required for consideration (default 10)
     * @return Vector of top performing prompt IDs with their success rates
     */
    std::vector<std::pair<std::string, double>> getTopPerformingPrompts(
        size_t count = 10,
        size_t min_executions = 10
    ) const;
    
    /**
     * @brief Reset metrics for a specific prompt
     * @param prompt_id Prompt template ID
     * @return true if metrics were reset, false if prompt not found
     */
    bool resetMetrics(const std::string& prompt_id);
    
    /**
     * @brief Clear all metrics
     */
    void clearAllMetrics();
    
    /**
     * @brief Get summary statistics across all prompts
     * @return JSON object with aggregate statistics
     */
    nlohmann::json getSummaryStatistics() const;

private:
    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, PromptMetrics> metrics_;
    
    // Optional persistence
    RocksDBWrapper* db_ = nullptr;
    rocksdb::ColumnFamilyHandle* cf_ = nullptr;
    
    static constexpr const char* KEY_PREFIX = "prompt_metrics:";
    
    /**
     * @brief Persist metrics to RocksDB
     */
    void persist(const std::string& prompt_id, const PromptMetrics& metrics);
    
    /**
     * @brief Load metrics from RocksDB
     */
    void loadFromDB();
    
    /**
     * @brief Update running averages incrementally
     */
    void updateAverages(PromptMetrics& metrics, bool success, double latency_ms, double user_feedback);
};

} // namespace prompt_engineering
} // namespace themis
