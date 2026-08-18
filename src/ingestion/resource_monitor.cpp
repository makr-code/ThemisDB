/**
 * @file resource_monitor.cpp
 * @brief Implementation of resource monitoring for ingestion pipelines.
 *
 * Phase 2.9 (Bounded Resources + Stress Tests) — ING-IMPL-002, ING-IMPL-004
 */

#include "ingestion/resource_monitor.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace themis {
namespace ingestion {

// ResourceMonitor is header-only except for monitoring thread implementation.
// The core functionality is already implemented in the header.

}  // namespace ingestion
}  // namespace themis
