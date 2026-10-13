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
    return {{
        {"community", true, true, true},
        {"community_public_api_boundary", true, true, true},
        {"community_license_compliance", true, true, true},
        {"community_flag_gate", true, true, true},
        {"minimal", true, true, true},
        {"minimal_core_api_set", true, true, true},
        {"minimal_dependency_budget", true, true, true},
        {"enterprise", true, true, true},
        {"enterprise_advanced_features", true, true, true},
        {"enterprise_audit_suite", true, true, true},
        {"enterprise_perf_tuning", true, true, true},
        {"military", true, true, true},
        {"military_security_policy", true, true, true},
        {"military_certification_readiness", true, true, true},
        {"military_hardening_flags", true, true, true}
    }};
}

}  // namespace themis::audit
