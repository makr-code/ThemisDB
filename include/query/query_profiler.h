/**
 * @file query_profiler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdint>

namespace themis {
namespace query {

struct OperatorProfile {
    std::string operator_name;

    int64_t duration_ns = 0;

    size_t rows_in = 0;

    size_t rows_out = 0;

    size_t memory_bytes = 0;

    size_t io_reads = 0;
};

struct QueryProfile {
    std::string query_text;

    int64_t total_duration_ns = 0;

    size_t peak_memory_bytes = 0;

    std::vector<OperatorProfile> operators;

    size_t result_rows = 0;

    bool cache_hit = false;

    /**
     * @brief Slowest Operator.
     * @return Pointer to the result.
     */
    const OperatorProfile* slowestOperator() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Interface
// ─────────────────────────────────────────────────────────────────────────────

class IQueryProfiler {
public:
    /**
     * @brief IQuery Profiler.
     * @return Return value.
     */
    virtual ~IQueryProfiler() = default;

    /**
     * @brief Begin Query.
     * @param[in] query_text Input parameter.
     */
    virtual void beginQuery(const std::string& query_text) = 0;

    virtual void endQuery(size_t result_rows, bool cache_hit = false) = 0;

    /**
     * @brief Begin Operator.
     * @param[in] operator_name Name of the operator.
     */
    virtual void beginOperator(const std::string& operator_name) = 0;

    virtual void endOperator(size_t rows_in, size_t rows_out,
                             size_t memory_bytes = 0,
                             size_t io_reads = 0) = 0;

    [[nodiscard]] virtual QueryProfile getProfile() const = 0;

    /**
     * @brief Reset the modification detection flag.
     */
    virtual void reset() = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Concrete implementations
// ─────────────────────────────────────────────────────────────────────────────

class QueryProfiler : public IQueryProfiler {
public:
    ~QueryProfiler() override = default;
    QueryProfiler() = default;

    void beginQuery(const std::string& query_text) override;
    void endQuery(size_t result_rows, bool cache_hit = false) override;
    void beginOperator(const std::string& operator_name) override;
    void endOperator(size_t rows_in, size_t rows_out,
                     size_t memory_bytes = 0,
                     size_t io_reads = 0) override;
    QueryProfile getProfile() const override;
    void reset() override;

private:
    QueryProfile profile_;
    std::chrono::steady_clock::time_point query_start_;
    std::chrono::steady_clock::time_point op_start_;
    std::string current_operator_;
};

class NullQueryProfiler : public IQueryProfiler {
public:
    ~NullQueryProfiler() override = default;
    void beginQuery(const std::string&) override {}
    void endQuery(size_t, bool) override {}
    void beginOperator(const std::string&) override {}
    void endOperator(size_t, size_t, size_t, size_t) override {}
    QueryProfile getProfile() const override { return {}; }
    void reset() override {}
};

} // namespace query
} // namespace themis
