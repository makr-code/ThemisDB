/**
 * @file cross_border_transfer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/cross_border_transfer.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>

namespace themis {
namespace governance {

// ============================================================================
// Static helpers
// ============================================================================

std::string CrossBorderTransferPolicy::mechanismToHeaderValue(TransferMechanism m) {
    switch (m) {
        case TransferMechanism::ADEQUACY_DECISION:           return "ADEQUACY_DECISION";
        case TransferMechanism::STANDARD_CONTRACTUAL_CLAUSES: return "SCC";
        case TransferMechanism::BINDING_CORPORATE_RULES:    return "BCR";
        case TransferMechanism::DEROGATION:                  return "DEROGATION";
        case TransferMechanism::PROHIBITED:                  return "PROHIBITED";
    }
    return "PROHIBITED";
}

std::string CrossBorderTransferPolicy::mechanismDescription(TransferMechanism m) {
    switch (m) {
        case TransferMechanism::ADEQUACY_DECISION:
            return "Transfer permitted under EU Commission adequacy decision (GDPR Art. 45)";
        case TransferMechanism::STANDARD_CONTRACTUAL_CLAUSES:
            return "Transfer permitted under Standard Contractual Clauses (GDPR Art. 46(2)(c/d))";
        case TransferMechanism::BINDING_CORPORATE_RULES:
            return "Transfer permitted under Binding Corporate Rules (GDPR Art. 47)";
        case TransferMechanism::DEROGATION:
            return "Transfer permitted under specific derogation (GDPR Art. 49)";
        case TransferMechanism::PROHIBITED:
            return "Transfer prohibited: no valid transfer mechanism available";
    }
    return "Transfer prohibited";
}

// EU Commission adequacy decisions as of 2025 (GDPR Art. 45)
std::unordered_map<std::string, TransferMechanism>
CrossBorderTransferPolicy::defaultEuAdequacyList() {
    return {
        // Adequacy decisions
        {"AD", TransferMechanism::ADEQUACY_DECISION},  // Andorra
        {"AR", TransferMechanism::ADEQUACY_DECISION},  // Argentina
        {"CA", TransferMechanism::ADEQUACY_DECISION},  // Canada (commercial)
        {"CH", TransferMechanism::ADEQUACY_DECISION},  // Switzerland
        {"FO", TransferMechanism::ADEQUACY_DECISION},  // Faroe Islands
        {"GG", TransferMechanism::ADEQUACY_DECISION},  // Guernsey
        {"IL", TransferMechanism::ADEQUACY_DECISION},  // Israel
        {"IM", TransferMechanism::ADEQUACY_DECISION},  // Isle of Man
        {"JP", TransferMechanism::ADEQUACY_DECISION},  // Japan
        {"JE", TransferMechanism::ADEQUACY_DECISION},  // Jersey
        {"NZ", TransferMechanism::ADEQUACY_DECISION},  // New Zealand
        {"KR", TransferMechanism::ADEQUACY_DECISION},  // South Korea
        {"UK", TransferMechanism::ADEQUACY_DECISION},  // United Kingdom
        {"GB", TransferMechanism::ADEQUACY_DECISION},  // United Kingdom (ISO 3166)
        {"UY", TransferMechanism::ADEQUACY_DECISION},  // Uruguay
        // EEA member states — intra-EEA transfers are unrestricted (not GDPR Chapter V)
        // but listed as ADEQUACY_DECISION for convenience
        {"AT", TransferMechanism::ADEQUACY_DECISION}, {"BE", TransferMechanism::ADEQUACY_DECISION},
        {"BG", TransferMechanism::ADEQUACY_DECISION}, {"CY", TransferMechanism::ADEQUACY_DECISION},
        {"CZ", TransferMechanism::ADEQUACY_DECISION}, {"DE", TransferMechanism::ADEQUACY_DECISION},
        {"DK", TransferMechanism::ADEQUACY_DECISION}, {"EE", TransferMechanism::ADEQUACY_DECISION},
        {"ES", TransferMechanism::ADEQUACY_DECISION}, {"FI", TransferMechanism::ADEQUACY_DECISION},
        {"FR", TransferMechanism::ADEQUACY_DECISION}, {"GR", TransferMechanism::ADEQUACY_DECISION},
        {"HR", TransferMechanism::ADEQUACY_DECISION}, {"HU", TransferMechanism::ADEQUACY_DECISION},
        {"IE", TransferMechanism::ADEQUACY_DECISION}, {"IS", TransferMechanism::ADEQUACY_DECISION},
        {"IT", TransferMechanism::ADEQUACY_DECISION}, {"LI", TransferMechanism::ADEQUACY_DECISION},
        {"LT", TransferMechanism::ADEQUACY_DECISION}, {"LU", TransferMechanism::ADEQUACY_DECISION},
        {"LV", TransferMechanism::ADEQUACY_DECISION}, {"MT", TransferMechanism::ADEQUACY_DECISION},
        {"NL", TransferMechanism::ADEQUACY_DECISION}, {"NO", TransferMechanism::ADEQUACY_DECISION},
        {"PL", TransferMechanism::ADEQUACY_DECISION}, {"PT", TransferMechanism::ADEQUACY_DECISION},
        {"RO", TransferMechanism::ADEQUACY_DECISION}, {"SE", TransferMechanism::ADEQUACY_DECISION},
        {"SI", TransferMechanism::ADEQUACY_DECISION}, {"SK", TransferMechanism::ADEQUACY_DECISION},
        // SCCs commonly used
        {"US", TransferMechanism::STANDARD_CONTRACTUAL_CLAUSES},
        {"AU", TransferMechanism::STANDARD_CONTRACTUAL_CLAUSES},
        {"IN", TransferMechanism::STANDARD_CONTRACTUAL_CLAUSES},
        {"SG", TransferMechanism::STANDARD_CONTRACTUAL_CLAUSES},
        {"BR", TransferMechanism::STANDARD_CONTRACTUAL_CLAUSES},
        // Countries typically prohibited without specific mechanism
        {"CN", TransferMechanism::PROHIBITED},
        {"RU", TransferMechanism::PROHIBITED},
        {"BY", TransferMechanism::PROHIBITED},
        {"KP", TransferMechanism::PROHIBITED},
        {"IR", TransferMechanism::PROHIBITED},
    };
}

// ============================================================================
// CrossBorderTransferPolicy implementation
// ============================================================================

CrossBorderTransferPolicy::CrossBorderTransferPolicy() {
    region_map_ = defaultEuAdequacyList();
}

void CrossBorderTransferPolicy::loadAdequacyList(
    const std::unordered_map<std::string, TransferMechanism>& region_to_mechanism) {
    std::lock_guard<std::mutex> lock(mutex_);
    region_map_ = region_to_mechanism;
    THEMIS_INFO("CrossBorderTransferPolicy: loaded {} region entries",
                region_map_.size());
}

void CrossBorderTransferPolicy::setRegionMechanism(
    const std::string& region, TransferMechanism mechanism) {
    std::lock_guard<std::mutex> lock(mutex_);
    region_map_[region] = mechanism;
}

TransferMechanism CrossBorderTransferPolicy::getMechanism(
    const std::string& region) const {
    // Normalise to upper case for lookup
    std::string upper = region;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = region_map_.find(upper);
    if (it == region_map_.end()) {
        return TransferMechanism::PROHIBITED;
    }
    return it->second;
}

std::unordered_map<std::string, TransferMechanism>
CrossBorderTransferPolicy::getAdequacyList() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return region_map_;
}

TransferDecision CrossBorderTransferPolicy::checkTransfer(
    const std::string& destination_region,
    const std::optional<std::string>& /*data_classification*/) const {

    if (destination_region.empty()) {
        return {false, TransferMechanism::PROHIBITED, destination_region,
                "Destination region not specified; defaulting to PROHIBITED", "PROHIBITED"};
    }

    TransferMechanism mech = getMechanism(destination_region);

    TransferDecision decision;
    decision.destination_region        = destination_region;
    decision.mechanism                 = mech;
    decision.transfer_mechanism_header = mechanismToHeaderValue(mech);
    decision.reason                    = mechanismDescription(mech);
    decision.allowed                   = (mech != TransferMechanism::PROHIBITED);

    if (!decision.allowed) {
        THEMIS_WARN("CrossBorderTransfer: transfer to '{}' DENIED — {}",
                    destination_region, decision.reason);
    } else {
        THEMIS_DEBUG("CrossBorderTransfer: transfer to '{}' ALLOWED via {}",
                     destination_region, decision.transfer_mechanism_header);
    }

    return decision;
}

} // namespace governance
} // namespace themis
