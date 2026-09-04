/**
 * @file rag_context_budget_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/rag_context_budget_manager.h"

#include <stdexcept>

namespace themis {
namespace prompt_engineering {

RagContextBudgetManager::RagContextBudgetManager([[maybe_unused]] size_t total_budget)
    : total_budget_(total_budget) {
    if (total_budget == 0) {
        throw std::invalid_argument(
            "RagContextBudgetManager: total_budget must be > 0");
    }
}

BudgetHandle RagContextBudgetManager::allocate([[maybe_unused]] size_t tokens) {
    std::lock_guard<std::mutex> lock(mutex_);

    const size_t current_allocated = allocated_.load(std::memory_order_relaxed);
    const size_t avail = (current_allocated < total_budget_)
                             ? (total_budget_ - current_allocated)
                             : 0;
    if (tokens > avail) {
        throw BudgetExhaustedError(tokens, avail, total_budget_);
    }

    allocated_.fetch_add(tokens, std::memory_order_relaxed);
    alloc_count_.fetch_add(1, std::memory_order_relaxed);

    return BudgetHandle(tokens,
                        [this]([[maybe_unused]] size_t t) noexcept { doRelease(t); });
}

void RagContextBudgetManager::doRelease([[maybe_unused]] size_t tokens) noexcept {
    if (tokens == 0) return;
    // Clamp to prevent underflow if release is called after reset().
    size_t prev = allocated_.load(std::memory_order_relaxed);
    size_t desired;
    do {
        desired = (prev >= tokens) ? (prev - tokens) : 0;
    } while (!allocated_.compare_exchange_weak(prev, desired,
                                               std::memory_order_relaxed,
                                               std::memory_order_relaxed));
}

size_t RagContextBudgetManager::remaining() const noexcept {
    const size_t alloc = allocated_.load(std::memory_order_relaxed);
    return (alloc < total_budget_) ? (total_budget_ - alloc) : 0;
}

size_t RagContextBudgetManager::totalBudget() const noexcept {
    return total_budget_;
}

void RagContextBudgetManager::reset() noexcept {
    // Resetting invalidates all outstanding handles — their release() calls
    // will safely clamp at zero via doRelease().
    allocated_.store(0, std::memory_order_relaxed);
    alloc_count_.store(0, std::memory_order_relaxed);
}

BudgetSnapshot RagContextBudgetManager::snapshot() const noexcept {
    const size_t alloc = allocated_.load(std::memory_order_relaxed);
    const size_t rem   = (alloc < total_budget_) ? (total_budget_ - alloc) : 0;
    return BudgetSnapshot{
        total_budget_,
        alloc,
        rem,
        alloc_count_.load(std::memory_order_relaxed)
    };
}

} // namespace prompt_engineering
} // namespace themis
