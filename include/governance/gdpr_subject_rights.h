/**
 * @file gdpr_subject_rights.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace themis {
namespace governance {

// ============================================================================
// Supporting types
// ============================================================================

enum class Regulation {
    GDPR,  ///< EU General Data Protection Regulation
    CCPA,  ///< California Consumer Privacy Act / CPRA
};

struct StoreErasureResult {
    std::string store_id;          ///< Identifier of the store (e.g. "vector_index")
    bool        success = false;   ///< Whether erasure succeeded
    std::string error_message;     ///< Non-empty on failure
    uint64_t    records_erased = 0;///< Number of records deleted / pseudonymised
};

struct ErasureReport {
    std::string subject_id;
    Regulation  regulation = Regulation::GDPR;
    std::string reason;
    std::string operator_id;
    std::chrono::system_clock::time_point timestamp;
    bool        fully_erased = false; ///< true when ALL stores reported success
    std::vector<StoreErasureResult> store_results;

    std::unordered_map<std::string, std::string> toSummaryMap() const;
};

struct PortabilityPackage {
    std::string subject_id;
    std::string format;              ///< "json" or "csv"
    std::vector<uint8_t> payload;    ///< Serialised data
    std::vector<uint8_t> tsa_signature; ///< RFC 3161 timestamp token (may be empty)
    std::chrono::system_clock::time_point issued_at;
};

// ============================================================================
// IGdprEraseTarget interface
// ============================================================================

class IGdprEraseTarget {
public:
    /**
     * @brief IGdpr Erase Target.
     * @return Return value.
     */
    virtual ~IGdprEraseTarget() = default;

    [[nodiscard]] virtual std::string storeId() const = 0;

    [[nodiscard]] virtual StoreErasureResult eraseSubject(const std::string& subject_id,
                                             Regulation regulation) = 0;

    [[nodiscard]] virtual std::vector<uint8_t> exportSubjectData(
        const std::string& subject_id,
        const std::string& format) = 0;
};

// ============================================================================
// GdprSubjectRightsManager
// ============================================================================

class GdprSubjectRightsManager {
public:
    using TsaSigner = std::function<std::vector<uint8_t>(
        const std::vector<uint8_t>& data)>;

    explicit GdprSubjectRightsManager(TsaSigner tsa_signer = nullptr);
    ~GdprSubjectRightsManager() = default;

    // Non-copyable
    GdprSubjectRightsManager(const GdprSubjectRightsManager&)            = delete;
    GdprSubjectRightsManager& operator=(const GdprSubjectRightsManager&) = delete;

    /**
     * @brief ── Target registration ──────────────────────────────────────────────
     * @param[in] target Input parameter.
     */

    void registerEraseTarget(std::shared_ptr<IGdprEraseTarget> target);

    /**
     * @brief Target Count.
     * @return Return value.
     */
    size_t targetCount() const;

    // ── Article 17: Right to Erasure ─────────────────────────────────────

    ErasureReport requestErasure(const std::string& subject_id,
                                  Regulation regulation,
                                  const std::string& reason,
                                  const std::string& operator_id = "system");

    // ── Article 20: Data Portability ─────────────────────────────────────

    PortabilityPackage requestPortability(const std::string& subject_id,
                                           const std::string& format = "json");

private:
    std::vector<std::shared_ptr<IGdprEraseTarget>> targets_;
    TsaSigner tsa_signer_;
    mutable std::mutex targets_mutex_;

    // Per-subject serialisation to prevent concurrent erasure of the same subject
    std::unordered_map<std::string, std::mutex> subject_mutexes_;
    std::mutex subject_map_mutex_;

    /**
     * @brief Get Subject Mutex.
     * @param[in] subject_id Identifier of the subject.
     * @return Return value.
     */
    std::mutex& getSubjectMutex(const std::string& subject_id);
};

} // namespace governance
} // namespace themis
