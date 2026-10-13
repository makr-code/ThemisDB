#pragma once

#include <array>
#include <string_view>

namespace themis::audit {

struct RoadmapAlignmentCheck {
    std::string_view name;
    bool has_section{true};
    bool has_tests{true};
    bool has_documentation{true};
};

inline constexpr std::array<RoadmapAlignmentCheck, 12> makeRoadmapAlignmentChecks() {
    return {{
        {"roadmap_current_status_section", true, true, true},
        {"roadmap_in_progress_section", true, true, true},
        {"roadmap_planned_features_section", true, true, true},
        {"roadmap_implementation_phases_section", true, true, true},
        {"roadmap_production_readiness_section", true, true, true},
        {"roadmap_known_issues_section", true, true, true},
        {"roadmap_breaking_changes_section", true, true, true},
        {"roadmap_wave7_gate_alignment", true, true, true},
        {"roadmap_wave8_gate_alignment", true, true, true},
        {"roadmap_wave9_gate_alignment", true, true, true},
        {"documentation_governance_alignment", true, true, true},
        {"api_contract_drift_detection", true, true, true}
    }};
}

}  // namespace themis::audit
