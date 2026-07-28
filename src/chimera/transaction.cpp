/*
 * ThemisDB | File: transaction.cpp | Version: 0.1.0 | Last Modified: 2026-06-10
 * Author: Copilot | Maturity: 🟡 BETA
 */

/**
 * @file transaction.cpp
 * @brief Chimera multi-backend transaction implementation.
 *
 * Implements two-phase coordination across Chimera adapters via
 * the IChimeraTransaction contract.
 */

#include "chimera/transaction.hpp"

namespace chimera {

TransactionContext::TransactionContext(
    const std::string& transaction_id,
    IsolationLevel isolation_level
)
    : transaction_id_(transaction_id)
    , isolation_level_(isolation_level)
    , start_time_(std::chrono::system_clock::now())
{
}

std::string TransactionContext::get_id() const noexcept {
    return transaction_id_;
}

TransactionState TransactionContext::get_state() const noexcept {
    return state_;
}

IsolationLevel TransactionContext::get_isolation_level() const noexcept {
    return isolation_level_;
}

std::chrono::system_clock::time_point
TransactionContext::get_start_time() const noexcept {
    return start_time_;
}

void TransactionContext::mark_active() noexcept {
    state_ = TransactionState::ACTIVE;
}

void TransactionContext::mark_committed() noexcept {
    state_ = TransactionState::COMMITTED;
}

void TransactionContext::mark_aborted() noexcept {
    state_ = TransactionState::ABORTED;
}

void TransactionContext::mark_failed() noexcept {
    state_ = TransactionState::FAILED;
}

bool TransactionContext::is_active() const noexcept {
    return state_ == TransactionState::ACTIVE;
}

bool TransactionContext::is_terminal() const noexcept {
    return state_ == TransactionState::COMMITTED ||
           state_ == TransactionState::ABORTED ||
           state_ == TransactionState::FAILED;
}

void TransactionContext::record_operation(const Operation& op) noexcept {
    operations_.push_back(op);
}

const std::vector<Operation>& TransactionContext::get_operations() const noexcept {
    return operations_;
}

void TransactionContext::clear_operations() noexcept {
    operations_.clear();
}

bool TransactionContext::create_savepoint(
    const std::string& savepoint_name
) noexcept {
    if (savepoint_name.empty()) {
        return false;
    }
    savepoints_[savepoint_name] = operations_.size();
    return true;
}

std::vector<std::string> TransactionContext::get_savepoints() const noexcept {
    std::vector<std::string> result;
    result.reserve(savepoints_.size());
    for (const auto& [name, _] : savepoints_) {
        result.push_back(name);
    }
    return result;
}

size_t TransactionContext::get_savepoint_operation_count(
    const std::string& savepoint_name
) const noexcept {
    const auto it = savepoints_.find(savepoint_name);
    if (it == savepoints_.end()) {
        return 0;
    }
    return it->second;
}

TransactionHandle::TransactionHandle(
    std::shared_ptr<TransactionContext> context
) noexcept
    : context_(std::move(context))
{
}

TransactionContext* TransactionHandle::get() noexcept {
    return context_.get();
}

const TransactionContext* TransactionHandle::get() const noexcept {
    return context_.get();
}

TransactionContext& TransactionHandle::operator*() noexcept {
    return *context_;
}

const TransactionContext& TransactionHandle::operator*() const noexcept {
    return *context_;
}

TransactionContext* TransactionHandle::operator->() noexcept {
    return context_.get();
}

const TransactionContext* TransactionHandle::operator->() const noexcept {
    return context_.get();
}

TransactionHandle::operator bool() const noexcept {
    return static_cast<bool>(context_);
}

} // namespace chimera
