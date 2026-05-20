/*
 * ThemisDB | File: project_metrics.cpp | Version: 1.0.0 | Last Modified: 2026-04-27 11:58:10
 * Author: copilot-swe-agent[bot] | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 43
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=13 | delta=10 | status=divergent
 * External Severity (v3): C=0, H=3, M=10
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
