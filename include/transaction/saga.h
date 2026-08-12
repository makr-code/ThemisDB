/**
 * @file saga.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <algorithm>
#include "storage/rocksdb_wrapper.h"

namespace themis {

// Forward declarations
class BaseEntity;
class SecondaryIndexManager;
class GraphIndexManager;
class VectorIndexManager;

/// SAGA Pattern: Distributed Transaction with Compensating Actions
/// 
/// Each operation in a transaction records a compensating action that can undo it.
/// On rollback, compensating actions are executed in reverse order.
/// Guarantees eventual consistency even if individual operations fail.

class Saga {
public:
    using CompensatingAction = std::function<void()>;
    
    struct Step {
        std::string operation_name;
        CompensatingAction compensate;
        std::chrono::system_clock::time_point executed_at;
        bool compensated = false;
        
        Step(std::string name, CompensatingAction action)
            : operation_name(std::move(name))
            , compensate(std::move(action))
            , executed_at(std::chrono::system_clock::now()) {}
    };
    
    Saga() = default;
    ~Saga();
    
    // Disable copy, enable move
    Saga(const Saga&) = delete;
    Saga& operator=(const Saga&) = delete;
    Saga(Saga&&) noexcept = default;
    Saga& operator=(Saga&&) noexcept = default;
    
    /// Add a step with its compensating action
    void addStep(std::string operation_name, CompensatingAction compensate);
    
    /// Execute all compensating actions in reverse order
    void compensate();

    /// Execute all compensating actions with retry.
    /// @param max_retries  Per-step retry attempts on exception (0 = no retry).
    /// @param backoff_ms   Initial backoff in milliseconds; doubled on each retry.
    void compensateWithRetry(int max_retries = 3,
                             std::chrono::milliseconds backoff_ms = std::chrono::milliseconds(50));

    /// Clear all steps (called after successful commit)
    void clear();
    
    /**
     * @brief Discard all steps with index >= @p n, without compensating them.
     *
     * Used by the named-savepoint layer to remove SAGA entries that correspond
     * to writes already undone by a RocksDB savepoint rollback.  Must only be
     * called while the transaction is still active (i.e. before compensate()).
     *
     * @param n  Target size; if >= stepCount() this is a no-op.
     */
    void trimToSize(size_t n);
    
    /// Get number of recorded steps
    size_t stepCount() const { return steps_.size(); }
    
    /// Get number of compensated steps
    size_t compensatedCount() const;
    
    /// Check if all steps have been compensated
    bool isFullyCompensated() const;
    
    /// Get step history for debugging
    std::vector<std::string> getStepHistory() const;
    
    /// Get duration since first step
    int64_t getDurationMs() const;

    /// SAGA execution metrics.
    struct Metrics {
        uint64_t total_steps{0};
        uint64_t compensated_steps{0};
        uint64_t failed_compensations{0};  ///< Steps that threw during compensation
        uint64_t retried_compensations{0}; ///< Steps that succeeded only after retry
        int64_t  duration_ms{0};
    };

    /// Return accumulated execution metrics.
    Metrics getMetrics() const;

private:
    std::vector<Step> steps_;
    bool compensated_ = false;

    // Cumulative metrics
    uint64_t metrics_failed_{0};
    uint64_t metrics_retried_{0};
};

/// SAGA-aware Transaction Operations
/// These track compensating actions for each operation

struct SagaOperation {
    /// Put entity with compensating delete
    static void putEntityWithCompensation(
        RocksDBWrapper& db,
        const std::string& key,
        const std::vector<uint8_t>& value,
        Saga& saga
    );
    
    /// Delete entity with compensating restore
    static void deleteEntityWithCompensation(
        RocksDBWrapper& db,
        const std::string& key,
        Saga& saga
    );
    
    /// Secondary index put with compensating delete
    static void indexPutWithCompensation(
        SecondaryIndexManager& idx,
        const std::string& table,
        const BaseEntity& entity,
        RocksDBWrapper::WriteBatchWrapper& batch,
        Saga& saga
    );
    
    /// Graph edge add with compensating delete
    static void graphAddWithCompensation(
        GraphIndexManager& graph,
        const BaseEntity& edge,
        RocksDBWrapper::WriteBatchWrapper& batch,
        Saga& saga
    );
    
    /// Vector add with compensating cache cleanup
    static void vectorAddWithCompensation(
        VectorIndexManager& vec,
        const BaseEntity& entity,
        RocksDBWrapper::WriteBatchWrapper& batch,
        const std::string& vectorField,
        Saga& saga
    );
};

} // namespace themis
