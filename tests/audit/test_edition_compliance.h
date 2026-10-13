#pragma once

#include <array>
#include <string_view>

namespace themis::audit {

struct EditionProfile {
    std::string_view edition;
    bool public_api_restricted{true};
    bool feature_gating{true};
    bool compliance_controls{true};
};

inline constexpr std::array<EditionProfile, 15> makeEditionProfiles() {
    return std::array<EditionProfile, 15>{
        EditionProfile{"community", true, true, true},
        EditionProfile{"community_public_api_boundary", true, true, true},
        EditionProfile{"community_license_compliance", true, true, true},
        EditionProfile{"community_flag_gate", true, true, true},
        EditionProfile{"minimal", true, true, true},
        EditionProfile{"minimal_core_api_set", true, true, true},
        EditionProfile{"minimal_dependency_budget", true, true, true},
        EditionProfile{"enterprise", true, true, true},
        EditionProfile{"enterprise_advanced_features", true, true, true},
        EditionProfile{"enterprise_audit_suite", true, true, true},
        EditionProfile{"enterprise_perf_tuning", true, true, true},
        EditionProfile{"military", true, true, true},
        EditionProfile{"military_security_policy", true, true, true},
        EditionProfile{"military_certification_readiness", true, true, true},
        EditionProfile{"military_hardening_flags", true, true, true}
    };
}

}  // namespace themis::audit
