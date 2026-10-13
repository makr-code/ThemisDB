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
    return std::array<WaveExitCriterion, 30>{
        WaveExitCriterion{"wave7_code_review_coverage", true, true},
        WaveExitCriterion{"wave7_test_density", true, true},
        WaveExitCriterion{"wave7_documentation_complete", true, true},
        WaveExitCriterion{"wave7_release_notes_clean", true, true},
        WaveExitCriterion{"wave7_api_contracts_frozen", true, true},
        WaveExitCriterion{"wave7_reviewed_critical_paths", true, true},
        WaveExitCriterion{"wave7_bug_risk_baseline", true, true},
        WaveExitCriterion{"wave7_fallback_paths_validated", true, true},
        WaveExitCriterion{"wave7_async_failures_handled", true, true},
        WaveExitCriterion{"wave7_mem_safety_baseline", true, true},
        WaveExitCriterion{"wave8_latency_budget_verified", true, true},
        WaveExitCriterion{"wave8_timeout_behavior_valid", true, true},
        WaveExitCriterion{"wave8_cancellation_propagation_ok", true, true},
        WaveExitCriterion{"wave8_security_scan_clean", true, true},
        WaveExitCriterion{"wave8_compliance_gates_passed", true, true},
        WaveExitCriterion{"wave8_audit_logging_functional", true, true},
        WaveExitCriterion{"wave8_failover_procedures_tested", true, true},
        WaveExitCriterion{"wave8_resource_limits_enforced", true, true},
        WaveExitCriterion{"wave8_breaker_detection_active", true, true},
        WaveExitCriterion{"wave8_backup_restore_verified", true, true},
        WaveExitCriterion{"wave9_metric_collection_ready", true, true},
        WaveExitCriterion{"wave9_tracing_integration_active", true, true},
        WaveExitCriterion{"wave9_log_correlation_enabled", true, true},
        WaveExitCriterion{"wave9_sla_thresholds_defined", true, true},
        WaveExitCriterion{"wave9_runbooks_published", true, true},
        WaveExitCriterion{"wave9_support_escalation_validated", true, true},
        WaveExitCriterion{"wave9_capacity_plan_reviewed", true, true},
        WaveExitCriterion{"wave9_incident_response_drill_done", true, true},
        WaveExitCriterion{"wave9_observability_dashboard_live", true, true},
        WaveExitCriterion{"wave9_risk_acceptance_signed", true, true}
    };
}

}  // namespace themis::audit
