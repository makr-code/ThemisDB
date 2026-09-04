/**
 * @file project_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    std::ostringstream out = {};

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
