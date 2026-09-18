/**
 * @file transaction.hpp
 * @brief Multi-backend transaction abstraction for the Chimera layer.
 *
 * Provides IChimeraTransaction, a unified handle for distributed
 * transactions that span heterogeneous backend adapters.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace chimera {

enum class IsolationLevel : uint8_t {
    READ_UNCOMMITTED = 0,  ///< Lowest isolation; allows dirty reads
    READ_COMMITTED = 1,    ///< Default; prevents dirty reads
    REPEATABLE_READ = 2,   ///< Prevents non-repeatable reads
    SERIALIZABLE = 3       ///< Highest isolation; acts as if transactions ran serially
};

enum class TransactionState : uint8_t {
    STARTED = 0,     ///< Transaction created but not yet active
    ACTIVE = 1,      ///< Actively executing operations
    COMMITTED = 2,   ///< Successfully committed
    ABORTED = 3,     ///< Rolled back or aborted due to error
    FAILED = 4       ///< Failed during commit/rollback (unrecoverable state)
};

struct Operation {
    std::string op_type;
    std::string target;
    std::string data_snapshot;
    std::chrono::system_clock::time_point timestamp;
    bool is_reversible = true;
};

class TransactionContext {
public:
    explicit TransactionContext(
        const std::string& transaction_id,
        IsolationLevel isolation_level = IsolationLevel::READ_COMMITTED
    );

    ~TransactionContext() = default;

    /**
     * @brief Get id.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::string get_id() const noexcept;
    /**
     * @brief Get state.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    TransactionState get_state() const noexcept;
    /**
     * @brief Get isolation level.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    IsolationLevel get_isolation_level() const noexcept;
    /**
     * @brief Get start time.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::chrono::system_clock::time_point get_start_time() const noexcept;

    /**
     * @brief Mark active.
     * @note Exception safety: noexcept.
     */
    void mark_active() noexcept;
    /**
     * @brief Mark committed.
     * @note Exception safety: noexcept.
     */
    void mark_committed() noexcept;
    /**
     * @brief Mark aborted.
     * @note Exception safety: noexcept.
     */
    void mark_aborted() noexcept;
    /**
     * @brief Mark failed.
     * @note Exception safety: noexcept.
     */
    void mark_failed() noexcept;

    /**
     * @brief Is active.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_active() const noexcept;
    /**
     * @brief Is terminal.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_terminal() const noexcept;

    /**
     * @brief Record operation.
     * @param[in] op Input parameter.
     * @note Exception safety: noexcept.
     */
    void record_operation(const Operation& op) noexcept;
    /**
     * @brief Get operations.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const std::vector<Operation>& get_operations() const noexcept;
    /**
     * @brief Clear operations.
     * @note Exception safety: noexcept.
     */
    void clear_operations() noexcept;

    /**
     * @brief Create savepoint.
     * @param[in] savepoint_name Name of the savepoint.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool create_savepoint(const std::string& savepoint_name) noexcept;
    /**
     * @brief Get savepoints.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::vector<std::string> get_savepoints() const noexcept;
    /**
     * @brief Get savepoint operation count.
     * @param[in] savepoint_name Name of the savepoint.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t get_savepoint_operation_count(const std::string& savepoint_name) const noexcept;

private:
    std::string transaction_id_;
    IsolationLevel isolation_level_;
    TransactionState state_ = TransactionState::STARTED;
    std::chrono::system_clock::time_point start_time_;
    std::vector<Operation> operations_;
    std::map<std::string, size_t> savepoints_;
};

class TransactionHandle {
public:
    /**
     * @brief Transaction Handle.
     * @param[in] context Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    explicit TransactionHandle(std::shared_ptr<TransactionContext> context) noexcept;

    /**
     * @brief Get.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    TransactionContext* get() noexcept;
    /**
     * @brief Get.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    const TransactionContext* get() const noexcept;
    TransactionContext& operator*() noexcept;
    const TransactionContext& operator*() const noexcept;
    TransactionContext* operator->() noexcept;
    const TransactionContext* operator->() const noexcept;
    /**
     * @brief Bool.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    explicit operator bool() const noexcept;

private:
    std::shared_ptr<TransactionContext> context_;
};

} // namespace chimera
