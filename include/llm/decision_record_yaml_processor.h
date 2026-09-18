/**
 * @file decision_record_yaml_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <map>
#include <optional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <filesystem>
#include <chrono>
#include <cstdint>

namespace themis {
namespace llm {

struct DecisionRecord {
    // ─── Identity ──────────────────────────────────────────────────────────────
    std::string record_id;

    std::string decision_type;

    // ─── Source ────────────────────────────────────────────────────────────────
    std::string component;

    std::optional<std::string> shard_id;

    // ─── Timing ────────────────────────────────────────────────────────────────
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};

    int64_t latency_ms{0};

    // ─── Outcome ───────────────────────────────────────────────────────────────
    std::string outcome;

    std::optional<float> confidence;

    // ─── LoRA-specific ─────────────────────────────────────────────────────────
    std::optional<int> lora_round;

    std::optional<float> epsilon_spent;

    std::optional<size_t> participants;

    std::optional<float> accuracy_delta;

    // ─── Free-form parameters ──────────────────────────────────────────────────
    std::map<std::string, std::string> parameters;

    // ─── Audit trail reference ─────────────────────────────────────────────────
    std::optional<std::string> audit_ref;
};

// ─────────────────────────────────────────────────────────────────────────────

class DecisionRecordYamlProcessor {
public:
    struct Config {
        std::filesystem::path log_dir{"logs/decisions"};

        size_t max_queue_depth{10'000};

        bool create_daily_subdirs{true};

        std::chrono::milliseconds flush_timeout_ms{10'000};
    };

    DecisionRecordYamlProcessor();

    /**
     * @brief Decision Record Yaml Processor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit DecisionRecordYamlProcessor(Config config);

    ~DecisionRecordYamlProcessor();

    // Non-copyable, non-movable (owns a thread).
    DecisionRecordYamlProcessor(const DecisionRecordYamlProcessor&) = delete;
    DecisionRecordYamlProcessor& operator=(const DecisionRecordYamlProcessor&) = delete;
    DecisionRecordYamlProcessor(DecisionRecordYamlProcessor&&) = delete;
    DecisionRecordYamlProcessor& operator=(DecisionRecordYamlProcessor&&) = delete;

    /**
     * @brief Submit.
     * @param[in] record Input parameter.
     * @return True when the operation succeeds.
     */
    bool submit(DecisionRecord record);

    [[nodiscard]] bool flush();

    // ─── Statistics ────────────────────────────────────────────────────────────
    struct Stats {
        size_t submitted{0};  ///< Total records handed to submit().
        size_t written{0};    ///< Records successfully written to disk.
        size_t dropped{0};    ///< Records dropped due to full queue.
        size_t errors{0};     ///< Records that failed to write (I/O error).
    };

    /**
     * @brief Get Stats.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    Stats getStats() const noexcept;

private:
    /**
     * @brief Processor Thread.
     */
    void processorThread();
    /**
     * @brief Write Record.
     * @param[in] record Input parameter.
     */
    void writeRecord(const DecisionRecord& record);
    /**
     * @brief Record Path.
     * @param[in] record Input parameter.
     * @return Return value.
     */
    std::filesystem::path recordPath(const DecisionRecord& record) const;
    /**
     * @brief Generate Id.
     * @return Return value.
     */
    std::string generateId() const;
    /**
     * @brief Format Timestamp.
     * @param[in] tp Input parameter.
     * @return Return value.
     */
    std::string formatTimestamp(std::chrono::system_clock::time_point tp) const;
    /**
     * @brief To Yaml.
     * @param[in] record Input parameter.
     * @return Return value.
     */
    std::string toYaml(const DecisionRecord& record) const;

    Config config_;

    std::queue<DecisionRecord> queue_;
    size_t in_flight_{0};
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    std::thread thread_;

    std::atomic<size_t> submitted_{0};
    std::atomic<size_t> written_{0};
    std::atomic<size_t> dropped_{0};
    std::atomic<size_t> errors_{0};
};

} // namespace llm
} // namespace themis

