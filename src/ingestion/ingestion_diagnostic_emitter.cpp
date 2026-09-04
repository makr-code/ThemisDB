/**
 * @file ingestion_diagnostic_emitter.cpp
 * @brief Implementation of diagnostic emitter for operator-visible incidents.
 *
 * Phase 3 (Error Handling & Edge Cases) — Operator Diagnostics
 */

#include "ingestion/ingestion_diagnostic_emitter.h"

#include <iomanip>
#include <sstream>

namespace themis {
namespace ingestion {

// ============================================================================
// DiagnosticIncident JSON serialization
// ============================================================================

std::string DiagnosticIncident::toJson() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);

    oss << "{"
        << "\"incident_id\":\"" << incident_id << "\","
        << "\"category\":" << static_cast<int>(category) << ","
        << "\"severity\":" << static_cast<int>(severity) << ","
        << "\"title\":\"" << title << "\","
        << "\"description\":\"" << description << "\","
        << "\"detected_at\":\"" << detected_at.time_since_epoch().count() << "\","
        << "\"occurrence_count\":" << occurrence_count << ","
        << "\"remediation_hint\":\"" << remediation_hint << "\","
        << "\"runbook_link\":\"" << runbook_link << "\","
        << "\"error_context\":" << error_context.toJson() << ","
        << "\"metrics\":{";

    bool first = true;
    for (const auto& kv : metrics) {
        if (!first) {
          oss << ",";
        }
        oss << "\"" << kv.first << "\":" << kv.second;
        first = false;
    }

    oss << "}}";
    return oss.str();
}

}  // namespace ingestion
}  // namespace themis
