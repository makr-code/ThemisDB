/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            parallel_scan.h                                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:18:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     47                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 841df4fce9  2026-02-26  feat(query): parallel full-table scan for large collections ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
