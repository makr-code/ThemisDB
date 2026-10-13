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
    return std::array<RoadmapAlignmentCheck, 12>{
        RoadmapAlignmentCheck{"roadmap_current_status_section", true, true, true},
        RoadmapAlignmentCheck{"roadmap_in_progress_section", true, true, true},
        RoadmapAlignmentCheck{"roadmap_planned_features_section", true, true, true},
        RoadmapAlignmentCheck{"roadmap_implementation_phases_section", true, true, true},
        RoadmapAlignmentCheck{"roadmap_production_readiness_section", true, true, true},
        RoadmapAlignmentCheck{"roadmap_known_issues_section", true, true, true},
        RoadmapAlignmentCheck{"roadmap_breaking_changes_section", true, true, true},
        RoadmapAlignmentCheck{"roadmap_wave7_gate_alignment", true, true, true},
        RoadmapAlignmentCheck{"roadmap_wave8_gate_alignment", true, true, true},
        RoadmapAlignmentCheck{"roadmap_wave9_gate_alignment", true, true, true},
        RoadmapAlignmentCheck{"documentation_governance_alignment", true, true, true},
        RoadmapAlignmentCheck{"api_contract_drift_detection", true, true, true}
    };
}

}  // namespace themis::audit
