/**
 * @file compensation_log.h
 * @brief Compensation log for SAGA orchestration idempotency and recovery.
 * @version 1.0.0
 * @date 2026-08-17
 *
 * Provides durability and idempotency guarantees for SAGA compensation steps.
 * Records each compensation attempt to detect and handle duplicate retries.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>

namespace themis::transaction {

/**
 * @brief Compensation log entry tracking a single compensation attempt.
 */
struct CompensationLogEntry {
    /// Unique SAGA instance identifier.
    std::string saga_id;

    /// Name of the step being compensated.
    std::string step_name;

    /// Monotonic sequence number for this compensation (1st, 2nd, 3rd attempt, etc.).
    uint32_t sequence_number = 0;

    /// Timestamp when compensation was initiated.
    std::chrono::system_clock::time_point timestamp;

    /// Status: true = succeeded, false = failed/pending.
    bool succeeded = false;

    /// Optional error detail if compensation failed.
    std::string error_detail;
};

/**
 * @brief CompensationLog maintains durability and idempotency for SAGA compensation steps.
 *
 * For each SAGA compensation step that is attempted, a log entry is written
 * before execution. On retry, the same step name with an incremented sequence
 * number is logged. This allows operators to:
 * 1. Track which compensation steps have been attempted
 * 2. Detect and skip duplicate compensation (idempotency)
 * 3. Recover from coordinator crashes mid-compensation
 *
 * Thread-safety: All public methods are thread-safe.
 */
class CompensationLog {
public:
    /**
     * @brief Construct a compensation log.
     *
     * @param saga_id Unique identifier for the SAGA instance.
     */
    explicit CompensationLog(const std::string& saga_id);

    ~CompensationLog() = default;

    // Non-copyable
    CompensationLog(const CompensationLog&) = delete;
    CompensationLog& operator=(const CompensationLog&) = delete;

    /**
     * @brief Record a compensation step attempt.
     *
     * @param step_name Name of the step being compensated.
     * @return Sequence number for this attempt (1, 2, 3, ...).
     */
    uint32_t recordCompensationAttempt(const std::string& step_name);

    /**
     * @brief Mark a compensation step as succeeded.
     *
     * @param step_name Name of the step.
     * @param sequence_number Sequence number from recordCompensationAttempt().
     */
    void recordCompensationSuccess(
        const std::string& step_name,
        uint32_t sequence_number);

    /**
     * @brief Mark a compensation step as failed.
     *
     * @param step_name Name of the step.
     * @param sequence_number Sequence number from recordCompensationAttempt().
     * @param error_detail Optional error description.
     */
    void recordCompensationFailure(
        const std::string& step_name,
        uint32_t sequence_number,
        const std::string& error_detail = {});

    /**
     * @brief Check if a compensation step has already succeeded (for idempotency).
     *
     * @param step_name Name of the step.
     * @return true if at least one sequence attempt succeeded.
     */
    bool hasSucceeded(const std::string& step_name) const;

    /**
     * @brief Retrieve all log entries.
     *
     * @return Vector of all compensation log entries in order.
     */
    std::vector<CompensationLogEntry> getEntries() const;

    /**
     * @brief Retrieve entries for a specific step.
     *
     * @param step_name Name of the step.
     * @return Vector of log entries for that step.
     */
    std::vector<CompensationLogEntry> getEntriesForStep(
        const std::string& step_name) const;

    /**
     * @brief Clear all log entries.
     */
    void clear();

private:
    std::string saga_id_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<CompensationLogEntry>> entries_;
};

} // namespace themis::transaction
