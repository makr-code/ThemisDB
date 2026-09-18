/**
 * @file saga.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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


class Saga {
public:
    using CompensatingAction = std::function<void()>;
    
    struct Step {
        std::string operation_name = {};
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
    
    /**
     * @brief Add Step.
     * @param[in] operation_name Name of the operation.
     * @param[in] compensate Input parameter.
     */
    void addStep(std::string operation_name, CompensatingAction compensate);
    
    /**
     * @brief Compensate.
     */
    void compensate();

    void compensateWithRetry(int max_retries = 3,
                             std::chrono::milliseconds backoff_ms = std::chrono::milliseconds(50));

    /**
     * @brief Clear.
     */
    void clear();
    
    /**
     * @brief Trim To Size.
     * @param[in] n Input parameter.
     */
    void trimToSize(size_t n);
    
    size_t stepCount() const { return steps_.size(); }
    
    /**
     * @brief Compensated Count.
     * @return Return value.
     */
    size_t compensatedCount() const;
    
    /**
     * @brief Is Fully Compensated.
     * @return True when the operation succeeds.
     */
    bool isFullyCompensated() const;
    
    /**
     * @brief Get Step History.
     * @return Return value.
     */
    std::vector<std::string> getStepHistory() const;
    
    /**
     * @brief Get Duration Ms.
     * @return Return value.
     */
    int64_t getDurationMs() const;

    struct Metrics {
        uint64_t total_steps{0};
        uint64_t compensated_steps{0};
        uint64_t failed_compensations{0};  ///< Steps that threw during compensation
        uint64_t retried_compensations{0}; ///< Steps that succeeded only after retry
        int64_t  duration_ms{0};
    };

    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    Metrics getMetrics() const;

private:
    std::vector<Step> steps_;
    bool compensated_ = false;

    // Cumulative metrics
    uint64_t metrics_failed_{0};
    uint64_t metrics_retried_{0};
};


struct SagaOperation {
    /**
     * @brief Put Entity With Compensation.
     * @param[in,out] db Input/output parameter.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @param[in,out] saga Input/output parameter.
     */
    static void putEntityWithCompensation(
        RocksDBWrapper& db,
        const std::string& key,
        const std::vector<uint8_t>& value,
        Saga& saga
    );
    
    /**
     * @brief Delete Entity With Compensation.
     * @param[in,out] db Input/output parameter.
     * @param[in] key Input parameter.
     * @param[in,out] saga Input/output parameter.
     */
    static void deleteEntityWithCompensation(
        RocksDBWrapper& db,
        const std::string& key,
        Saga& saga
    );
    
    /**
     * @brief Index Put With Compensation.
     * @param[in,out] idx Input/output parameter.
     * @param[in] table Input parameter.
     * @param[in] entity Input parameter.
     * @param[in,out] batch Input/output parameter.
     * @param[in,out] saga Input/output parameter.
     */
    static void indexPutWithCompensation(
        SecondaryIndexManager& idx,
        const std::string& table,
        const BaseEntity& entity,
        RocksDBWrapper::WriteBatchWrapper& batch,
        Saga& saga
    );
    
    /**
     * @brief Graph Add With Compensation.
     * @param[in,out] graph Input/output parameter.
     * @param[in] edge Input parameter.
     * @param[in,out] batch Input/output parameter.
     * @param[in,out] saga Input/output parameter.
     */
    static void graphAddWithCompensation(
        GraphIndexManager& graph,
        const BaseEntity& edge,
        RocksDBWrapper::WriteBatchWrapper& batch,
        Saga& saga
    );
    
    /**
     * @brief Vector Add With Compensation.
     * @param[in,out] vec Input/output parameter.
     * @param[in] entity Input parameter.
     * @param[in,out] batch Input/output parameter.
     * @param[in] vectorField Input parameter.
     * @param[in,out] saga Input/output parameter.
     */
    static void vectorAddWithCompensation(
        VectorIndexManager& vec,
        const BaseEntity& entity,
        RocksDBWrapper::WriteBatchWrapper& batch,
        const std::string& vectorField,
        Saga& saga
    );
};

} // namespace themis
