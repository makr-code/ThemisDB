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

/// @brief Transaction isolation levels per SQL standard.
enum class IsolationLevel : uint8_t {
    READ_UNCOMMITTED = 0,  ///< Lowest isolation; allows dirty reads
    READ_COMMITTED = 1,    ///< Default; prevents dirty reads
    REPEATABLE_READ = 2,   ///< Prevents non-repeatable reads
    SERIALIZABLE = 3       ///< Highest isolation; acts as if transactions ran serially
};

/// @brief Transaction state machine.
enum class TransactionState : uint8_t {
    STARTED = 0,     ///< Transaction created but not yet active
    ACTIVE = 1,      ///< Actively executing operations
    COMMITTED = 2,   ///< Successfully committed
    ABORTED = 3,     ///< Rolled back or aborted due to error
    FAILED = 4       ///< Failed during commit/rollback (unrecoverable state)
};

/// @brief Single operation for transaction rollback.
struct Operation {
    std::string op_type;
    std::string target;
    std::string data_snapshot;
    std::chrono::system_clock::time_point timestamp;
    bool is_reversible = true;
};

/**
 * @class TransactionContext
 * @brief Manages a single transaction's lifecycle and state.
 */
class TransactionContext {
public:
    explicit TransactionContext(
        const std::string& transaction_id,
        IsolationLevel isolation_level = IsolationLevel::READ_COMMITTED
    );

    ~TransactionContext() = default;

    std::string get_id() const noexcept;
    TransactionState get_state() const noexcept;
    IsolationLevel get_isolation_level() const noexcept;
    std::chrono::system_clock::time_point get_start_time() const noexcept;

    void mark_active() noexcept;
    void mark_committed() noexcept;
    void mark_aborted() noexcept;
    void mark_failed() noexcept;

    bool is_active() const noexcept;
    bool is_terminal() const noexcept;

    void record_operation(const Operation& op) noexcept;
    const std::vector<Operation>& get_operations() const noexcept;
    void clear_operations() noexcept;

    bool create_savepoint(const std::string& savepoint_name) noexcept;
    std::vector<std::string> get_savepoints() const noexcept;
    size_t get_savepoint_operation_count(const std::string& savepoint_name) const noexcept;

private:
    std::string transaction_id_;
    IsolationLevel isolation_level_;
    TransactionState state_ = TransactionState::STARTED;
    std::chrono::system_clock::time_point start_time_;
    std::vector<Operation> operations_;
    std::map<std::string, size_t> savepoints_;
};

/**
 * @class TransactionHandle
 * @brief RAII handle for managing transaction lifetime.
 */
class TransactionHandle {
public:
    explicit TransactionHandle(std::shared_ptr<TransactionContext> context) noexcept;

    TransactionContext* get() noexcept;
    const TransactionContext* get() const noexcept;
    TransactionContext& operator*() noexcept;
    const TransactionContext& operator*() const noexcept;
    TransactionContext* operator->() noexcept;
    const TransactionContext* operator->() const noexcept;
    explicit operator bool() const noexcept;

private:
    std::shared_ptr<TransactionContext> context_;
};

} // namespace chimera

#endif // CHIMERA_TRANSACTION_HPP
