/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            project_metrics.cpp                                ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-27                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "projects/project_metrics.h"

#include <sstream>

namespace themis {
namespace projects {

std::string ProjectMetrics::getMetricsText() const {
    const uint64_t changes  = changes_total_.load(std::memory_order_relaxed);
    const uint64_t calls    = diff_calls_total_.load(std::memory_order_relaxed);
    const uint64_t duration = diff_duration_ms_total_.load(std::memory_order_relaxed);

    if (changes == 0 && calls == 0) {
        return "";
    }

    std::ostringstream out;

    out << "# HELP projects_changes_total Total collaboration change events recorded.\n";
    out << "# TYPE projects_changes_total counter\n";
    out << "projects_changes_total " << changes << "\n";

    out << "# HELP project_diff_calls_total Total ProjectDiff::diff() invocations.\n";
    out << "# TYPE project_diff_calls_total counter\n";
    out << "project_diff_calls_total " << calls << "\n";

    out << "# HELP project_diff_duration_ms_total Cumulative diff computation time in milliseconds.\n";
    out << "# TYPE project_diff_duration_ms_total counter\n";
    out << "project_diff_duration_ms_total " << duration << "\n";

    return out.str();
}

} // namespace projects
} // namespace themis
