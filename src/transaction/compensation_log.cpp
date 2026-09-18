/**
 * @file compensation_log.cpp
 * @brief Implementation of CompensationLog for SAGA orchestration.
 */

#include "transaction/compensation_log.h"

namespace themis::transaction {

CompensationLog::CompensationLog(const std::string& saga_id)
    : saga_id_(saga_id) {
}

/**
 * @brief Record Compensation Attempt.
 * @param[in] step_name Name of the step.
 * @return Return value.
 * @details Calls: lock(), size(), std::chrono::system_clock::now(), push_back().
 */
uint32_t CompensationLog::recordCompensationAttempt(const std::string& step_name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto& entries = entries_[step_name];
    uint32_t sequence = static_cast<uint32_t>(entries.size()) + 1;

    CompensationLogEntry entry;
    entry.saga_id = saga_id_;
    entry.step_name = step_name;
    entry.sequence_number = sequence;
    entry.timestamp = std::chrono::system_clock::now();
    entry.succeeded = false;

    entries.push_back(entry);
    return sequence;
}

/**
 * @brief Record Compensation Success.
 * @param[in] step_name Name of the step.
 * @param[in] sequence_number Input parameter.
 * @details Calls: lock(), find(), end(), size().
 */
void CompensationLog::recordCompensationSuccess(
    const std::string& step_name,
    uint32_t sequence_number) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(step_name);
    if (it != entries_.end()  && static_cast<size_t>(sequence_number) <= static_cast<uint32_t>(it->second.size())) {
        it->second[static_cast<int>(sequence_number - 1)].succeeded = true;
    }
}

/**
 * @brief Record Compensation Failure.
 * @param[in] step_name Name of the step.
 * @param[in] sequence_number Input parameter.
 * @param[in] error_detail Input parameter.
 * @details Calls: lock(), find(), end(), size().
 */
void CompensationLog::recordCompensationFailure(
    const std::string& step_name,
    uint32_t sequence_number,
    const std::string& error_detail) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(step_name);
    if (it != entries_.end()  && static_cast<size_t>(sequence_number) <= static_cast<uint32_t>(it->second.size())) {
        it->second[static_cast<int>(sequence_number - 1)].succeeded = false;
        it->second[static_cast<int>(sequence_number - 1)].error_detail = error_detail;
    }
}

bool CompensationLog::hasSucceeded(const std::string& step_name) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(step_name);
    if (it == entries_.end()) {
      return false;
    }

    for (const auto& entry : it->second) {
        if (entry.succeeded) {
          return true;
        }
    }
    return false;
}

std::vector<CompensationLogEntry> CompensationLog::getEntries() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<CompensationLogEntry> result = {};

    for (const auto& [step_name, entries] : entries_) {
        for (const auto& entry : entries) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<CompensationLogEntry> CompensationLog::getEntriesForStep(
    const std::string& step_name) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(step_name);
    if (it == entries_.end()) return {};
    return it->second;
}

/**
 * @brief Clear.
 * @details Calls: lock().
 */
void CompensationLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

} // namespace themis::transaction
