#pragma once

#include <array>
#include <string_view>

namespace themis::audit {

struct WaveExitCriterion {
    std::string_view name;
    bool required{true};
    bool present{true};
};

inline constexpr std::array<WaveExitCriterion, 30> makeWaveExitCriteria() {
    return {{
        {"wave7_code_review_coverage", true, true},
        {"wave7_test_density", true, true},
        {"wave7_documentation_complete", true, true},
        {"wave7_release_notes_clean", true, true},
        {"wave7_api_contracts_frozen", true, true},
        {"wave7_reviewed_critical_paths", true, true},
        {"wave7_bug_risk_baseline", true, true},
        {"wave7_fallback_paths_validated", true, true},
        {"wave7_async_failures_handled", true, true},
        {"wave7_mem_safety_baseline", true, true},
        {"wave8_latency_budget_verified", true, true},
        {"wave8_timeout_behavior_valid", true, true},
        {"wave8_cancellation_propagation_ok", true, true},
        {"wave8_security_scan_clean", true, true},
        {"wave8_compliance_gates_passed", true, true},
        {"wave8_audit_logging_functional", true, true},
        {"wave8_failover_procedures_tested", true, true},
        {"wave8_resource_limits_enforced", true, true},
        {"wave8_breaker_detection_active", true, true},
        {"wave8_backup_restore_verified", true, true},
        {"wave9_metric_collection_ready", true, true},
        {"wave9_tracing_integration_active", true, true},
        {"wave9_log_correlation_enabled", true, true},
        {"wave9_sla_thresholds_defined", true, true},
        {"wave9_runbooks_published", true, true},
        {"wave9_support_escalation_validated", true, true},
        {"wave9_capacity_plan_reviewed", true, true},
        {"wave9_incident_response_drill_done", true, true},
        {"wave9_observability_dashboard_live", true, true},
        {"wave9_risk_acceptance_signed", true, true},
        {"wave9_release_signoff_complete", true, true}
    }};
}

}  // namespace themis::audit
