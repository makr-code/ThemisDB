/**
 * @file parallel_scan.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>

namespace themis {

/**
 * @brief Configuration for the parallel full-table scan path.
 *
 * When a full-table scan encounters a collection with at least
 * @p parallel_threshold rows, the deserialization and predicate
 * evaluation work is split into fixed-size morsels and processed
 * concurrently via TBB task_group.  Smaller collections use the
 * existing sequential path to avoid TBB scheduling overhead.
 */
struct ParallelScanConfig {
    /// Minimum number of collected rows that trigger the parallel path.
    size_t parallel_threshold = 10'000;

    /// Number of rows in each parallel morsel (TBB task granularity).
    size_t morsel_size = 1'024;
};

} // namespace themis
