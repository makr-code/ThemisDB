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
