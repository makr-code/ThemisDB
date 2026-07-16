/**
 * @file rag_context_budget_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>

namespace themis {
namespace prompt_engineering {

// ── Error type ───────────────────────────────────────────────────────────────

/**
 * @brief Thrown by `IRAGContextBudgetManager::allocate()` when the requested
 *        allocation would exceed the remaining budget.
 */
class BudgetExhaustedError : public std::runtime_error {
public:
    /**
     * @param requested  Token count that was requested.
     * @param remaining  Token count currently available.
     * @param total      Total budget configured for the manager.
     */
    BudgetExhaustedError(size_t requested, size_t remaining, size_t total)
        : std::runtime_error(
              "RAG context budget exhausted: requested " +
              std::to_string(requested) + " tokens but only " +
              std::to_string(remaining) + " / " +
              std::to_string(total) + " remain"),
          requested_(requested),
          remaining_(remaining),
          total_(total) {}

    size_t requested() const noexcept { return requested_; }
    size_t remaining() const noexcept { return remaining_; }
    size_t total()     const noexcept { return total_;     }

private:
    size_t requested_;
    size_t remaining_;
    size_t total_;
};

// ── Diagnostic snapshot ──────────────────────────────────────────────────────

/**
 * @brief Immutable point-in-time snapshot of the budget state.
 *
 * Returned by `IRAGContextBudgetManager::snapshot()` for observability.
 * All fields are consistent but may not reflect concurrent in-flight
 * allocations made after the snapshot was captured.
 */
struct BudgetSnapshot {
    size_t total_budget    = 0; ///< Total tokens configured for the manager.
    size_t allocated       = 0; ///< Tokens currently allocated across all handles.
    size_t remaining       = 0; ///< Tokens available for new allocations.
    size_t allocation_count = 0; ///< Total allocation calls since last reset().
};

// ── RAII budget handle ────────────────────────────────────────────────────────

/**
 * @brief RAII token-reservation handle returned by `allocate()`.
 *
 * The handle owns a reservation of `tokens()` tokens on the budget manager.
 * Tokens are automatically released when the handle is destroyed or when
 * `release()` is called explicitly.  Handles are move-only; copying is
 * intentionally disabled to prevent double-releases.
 */
class BudgetHandle {
public:
    /// Constructs an empty (no-op) handle.
    BudgetHandle() noexcept = default;

    /// Move construction transfers ownership; the source becomes a no-op handle.
    BudgetHandle(BudgetHandle&& other) noexcept
        : tokens_(other.tokens_), released_(other.released_),
          release_fn_(std::move(other.release_fn_)) {
        other.released_ = true;
    }

    /// Move assignment transfers ownership.
    BudgetHandle& operator=(BudgetHandle&& other) noexcept {
        if (this != &other) {
            release();  // release any current reservation
            tokens_     = other.tokens_;
            released_   = other.released_;
            release_fn_ = std::move(other.release_fn_);
            other.released_ = true;
        }
        return *this;
    }

    BudgetHandle(const BudgetHandle&)            = delete;
    BudgetHandle& operator=(const BudgetHandle&) = delete;

    /// Releases the reserved tokens back to the manager if not already released.
    ~BudgetHandle() noexcept { release(); }

    /**
     * @brief Explicitly release the reservation before the handle is destroyed.
     *
     * Idempotent — calling release() multiple times is safe.
     */
    void release() noexcept {
        if (!released_ && release_fn_) {
            released_ = true;
            release_fn_(tokens_);
        }
    }

    /// Returns the number of tokens held by this handle.
    size_t tokens() const noexcept { return released_ ? 0 : tokens_; }

    /// Returns true if the reservation has already been released.
    bool isReleased() const noexcept { return released_; }

private:
    friend class RagContextBudgetManager;

    explicit BudgetHandle(size_t tokens, std::function<void(size_t)> release_fn) noexcept
        : tokens_(tokens), released_(false), release_fn_(std::move(release_fn)) {}

    size_t                         tokens_     = 0;
    bool                           released_   = true;
    std::function<void(size_t)>    release_fn_;
};

// ── Abstract interface ────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for RAG-layer context-window budget management.
 *
 * Implementations track a fixed token budget and allow retrieval workers to
 * reserve slices of that budget via RAII `BudgetHandle`s.  The hard-limit
 * enforcement prevents context-stuffing: if a retrieval worker requests more
 * tokens than are available, `BudgetExhaustedError` is thrown.
 */
class IRAGContextBudgetManager {
public:
    virtual ~IRAGContextBudgetManager() = default;

    /**
     * @brief Reserve @p tokens from the budget.
     *
     * @param tokens  Number of tokens to reserve.
     * @return A `BudgetHandle` that holds the reservation until released.
     * @throws BudgetExhaustedError if `tokens > remaining()`.
     */
    [[nodiscard]] virtual BudgetHandle allocate(size_t tokens) = 0;

    /// Returns the number of tokens currently available for allocation.
    [[nodiscard]] virtual size_t remaining() const noexcept = 0;

    /// Returns the total budget configured at construction.
    [[nodiscard]] virtual size_t totalBudget() const noexcept = 0;

    /**
     * @brief Release all current allocations and reset counters.
     *
     * All outstanding `BudgetHandle`s become no-ops after reset().
     * Call only when the RAG pipeline is idle (e.g., between requests).
     */
    virtual void reset() noexcept = 0;

    /**
     * @brief Return a point-in-time diagnostic snapshot.
     *
     * Does not block allocation; the snapshot may be slightly stale in the
     * presence of concurrent allocations.
     */
    [[nodiscard]] virtual BudgetSnapshot snapshot() const noexcept = 0;
};

// ── Concrete implementation ───────────────────────────────────────────────────

/**
 * @brief Thread-safe concrete implementation of `IRAGContextBudgetManager`.
 *
 * Uses a `std::mutex` and two `std::atomic<size_t>` counters:
 *  - `allocated_`  — current sum of all unreleased token reservations.
 *  - `alloc_count_` — monotone counter of total `allocate()` calls (not
 *    decremented on release); used for diagnostics.
 *
 * Performance target: `allocate()` and `release()` hot paths ≤ 100 µs.
 */
class RagContextBudgetManager final : public IRAGContextBudgetManager {
public:
    /**
     * @param total_budget  Maximum tokens available to all concurrent callers.
     * @throws std::invalid_argument if total_budget == 0.
     */
    explicit RagContextBudgetManager(size_t total_budget);

    BudgetHandle allocate(size_t tokens) override;
    size_t remaining()   const noexcept override;
    size_t totalBudget() const noexcept override;
    void   reset()             noexcept override;
    BudgetSnapshot snapshot()  const noexcept override;

private:
    void doRelease(size_t tokens) noexcept;

    size_t                 total_budget_;
    std::atomic<size_t>    allocated_{0};
    std::atomic<size_t>    alloc_count_{0};
    mutable std::mutex     mutex_;
};

} // namespace prompt_engineering
} // namespace themis
