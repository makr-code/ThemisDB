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

struct CompensationLogEntry {
    std::string saga_id;

    std::string step_name;

    uint32_t sequence_number = 0;

    std::chrono::system_clock::time_point timestamp;

    bool succeeded = false;

    std::string error_detail;
};

class CompensationLog {
public:
    /**
     * @brief Compensation Log.
     * @param[in] saga_id Identifier of the saga.
     * @return Return value.
     */
    explicit CompensationLog(const std::string& saga_id);

    ~CompensationLog() = default;

    // Non-copyable
    CompensationLog(const CompensationLog&) = delete;
    CompensationLog& operator=(const CompensationLog&) = delete;

    /**
     * @brief Record Compensation Attempt.
     * @param[in] step_name Name of the step.
     * @return Return value.
     */
    uint32_t recordCompensationAttempt(const std::string& step_name);

    /**
     * @brief Record Compensation Success.
     * @param[in] step_name Name of the step.
     * @param[in] sequence_number Input parameter.
     */
    void recordCompensationSuccess(
        const std::string& step_name,
        uint32_t sequence_number);

    void recordCompensationFailure(
        const std::string& step_name,
        uint32_t sequence_number,
        const std::string& error_detail = {});

    /**
     * @brief Has Succeeded.
     * @param[in] step_name Name of the step.
     * @return True when the operation succeeds.
     */
    bool hasSucceeded(const std::string& step_name) const;

    /**
     * @brief Get Entries.
     * @return Return value.
     */
    std::vector<CompensationLogEntry> getEntries() const;

    /**
     * @brief Get Entries For Step.
     * @param[in] step_name Name of the step.
     * @return Return value.
     */
    std::vector<CompensationLogEntry> getEntriesForStep(
        const std::string& step_name) const;

    /**
     * @brief Clear.
     */
    void clear();

private:
    std::string saga_id_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<CompensationLogEntry>> entries_;
};

} // namespace themis::transaction
