/**
 * @file cross_border_transfer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <optional>

namespace themis {
namespace governance {

// ============================================================================
// Types
// ============================================================================

enum class TransferMechanism {
    ADEQUACY_DECISION,         ///< EU Commission adequacy decision (Art. 45)
    STANDARD_CONTRACTUAL_CLAUSES, ///< SCCs (Art. 46(2)(c/d))
    BINDING_CORPORATE_RULES,   ///< BCR (Art. 46(2)(b) / Art. 47)
    DEROGATION,                ///< Specific derogations (Art. 49)
    PROHIBITED,                ///< No valid transfer mechanism — deny transfer
};

struct TransferDecision {
    bool allowed = false;
    TransferMechanism mechanism = TransferMechanism::PROHIBITED;
    std::string destination_region;   ///< As provided by caller
    std::string reason;               ///< Human-readable explanation
    std::string transfer_mechanism_header; ///< Value for X-Themis-Transfer-Mechanism
};

// ============================================================================
// CrossBorderTransferPolicy
// ============================================================================

class CrossBorderTransferPolicy {
public:
    CrossBorderTransferPolicy();
    ~CrossBorderTransferPolicy() = default;

    // Non-copyable
    CrossBorderTransferPolicy(const CrossBorderTransferPolicy&)            = delete;
    CrossBorderTransferPolicy& operator=(const CrossBorderTransferPolicy&) = delete;

    // ── Policy configuration ─────────────────────────────────────────────

    void loadAdequacyList(
        const std::unordered_map<std::string, TransferMechanism>& region_to_mechanism);

    /**
     * @brief Set Region Mechanism.
     * @param[in] region Input parameter.
     * @param[in] mechanism Input parameter.
     */
    void setRegionMechanism(const std::string& region, TransferMechanism mechanism);

    /**
     * @brief Get Mechanism.
     * @param[in] region Input parameter.
     * @return Return value.
     */
    TransferMechanism getMechanism(const std::string& region) const;

    std::unordered_map<std::string, TransferMechanism> getAdequacyList() const;

    // ── Transfer check ───────────────────────────────────────────────────

    TransferDecision checkTransfer(
        const std::string& destination_region,
        const std::optional<std::string>& data_classification = std::nullopt) const;


    /**
     * @brief Mechanism To Header Value.
     * @param[in] m Input parameter.
     * @return Return value.
     */
    static std::string mechanismToHeaderValue(TransferMechanism m);

    /**
     * @brief Mechanism Description.
     * @param[in] m Input parameter.
     * @return Return value.
     */
    static std::string mechanismDescription(TransferMechanism m);

    static std::unordered_map<std::string, TransferMechanism>
        defaultEuAdequacyList();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, TransferMechanism> region_map_;
};

} // namespace governance
} // namespace themis
