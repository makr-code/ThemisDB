/**
 * @file cross_border_transfer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief GDPR Chapter V transfer mechanism.
 *
 * Determines the legal basis on which personal data may be transferred
 * to a destination country that is not within the EEA.
 */
enum class TransferMechanism {
    ADEQUACY_DECISION,         ///< EU Commission adequacy decision (Art. 45)
    STANDARD_CONTRACTUAL_CLAUSES, ///< SCCs (Art. 46(2)(c/d))
    BINDING_CORPORATE_RULES,   ///< BCR (Art. 46(2)(b) / Art. 47)
    DEROGATION,                ///< Specific derogations (Art. 49)
    PROHIBITED,                ///< No valid transfer mechanism — deny transfer
};

/**
 * @brief Result of a cross-border transfer check.
 */
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

/**
 * @brief Evaluates GDPR Chapter V cross-border data transfer legality.
 *
 * Maintains a mapping of destination region codes to transfer mechanisms.
 * On each transfer check it resolves the mechanism, determines whether
 * the transfer is allowed, and returns the appropriate HTTP header value.
 *
 * The adequacy list (EU Commission-approved countries) is loaded from a
 * simple in-memory map that can be hot-reloaded via loadAdequacyList().
 *
 * Unknown destination regions default to PROHIBITED.
 *
 * Integration with PolicyEngine:
 *   After routing a request, if the request carries an
 *   X-Destination-Region header, callers should invoke checkTransfer() and
 *   deny the request if the result is not allowed.
 *
 * Thread safety: all public methods are thread-safe.
 */
class CrossBorderTransferPolicy {
public:
    CrossBorderTransferPolicy();
    ~CrossBorderTransferPolicy() = default;

    // Non-copyable
    CrossBorderTransferPolicy(const CrossBorderTransferPolicy&)            = delete;
    CrossBorderTransferPolicy& operator=(const CrossBorderTransferPolicy&) = delete;

    // ── Policy configuration ─────────────────────────────────────────────

    /**
     * @brief Replace the full adequacy / mechanism mapping.
     *
     * Keys are upper-case ISO 3166-1 alpha-2 country codes or arbitrary
     * region identifiers (e.g. "US", "CH", "IN", "CN").
     *
     * @param region_to_mechanism Map of region code → TransferMechanism.
     */
    void loadAdequacyList(
        const std::unordered_map<std::string, TransferMechanism>& region_to_mechanism);

    /**
     * @brief Add or update a single region entry.
     */
    void setRegionMechanism(const std::string& region, TransferMechanism mechanism);

    /**
     * @brief Return the mechanism currently registered for a region, or
     *        PROHIBITED if unknown.
     */
    TransferMechanism getMechanism(const std::string& region) const;

    /**
     * @brief Return a snapshot of all registered region → mechanism pairs.
     */
    std::unordered_map<std::string, TransferMechanism> getAdequacyList() const;

    // ── Transfer check ───────────────────────────────────────────────────

    /**
     * @brief Evaluate whether a data transfer to destination_region is allowed.
     *
     * Unknown regions → PROHIBITED.
     * PROHIBITED     → allowed=false.
     * All other mechanisms → allowed=true with mechanism code in the result.
     *
     * @param destination_region  Destination country/region code.
     * @param data_classification Optional classification label of the data
     *                            (reserved for future use in tiered policies).
     * @return TransferDecision with allow/deny + mechanism + header value.
     */
    TransferDecision checkTransfer(
        const std::string& destination_region,
        const std::optional<std::string>& data_classification = std::nullopt) const;

    // ── Helpers ──────────────────────────────────────────────────────────

    /// Convert a TransferMechanism enum to its X-Themis-Transfer-Mechanism
    /// header string (e.g. "ADEQUACY_DECISION").
    static std::string mechanismToHeaderValue(TransferMechanism m);

    /// Return a human-readable description of a transfer mechanism.
    static std::string mechanismDescription(TransferMechanism m);

    /// Default EU Commission adequacy list as of 2025.
    static std::unordered_map<std::string, TransferMechanism>
        defaultEuAdequacyList();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, TransferMechanism> region_map_;
};

} // namespace governance
} // namespace themis
