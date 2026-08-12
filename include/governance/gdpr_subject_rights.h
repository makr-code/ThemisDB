/**
 * @file gdpr_subject_rights.h
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

/**
 * @brief Regulation context for a data subject rights request.
 */
enum class Regulation {
    GDPR,  ///< EU General Data Protection Regulation
    CCPA,  ///< California Consumer Privacy Act / CPRA
};

/**
 * @brief Outcome of a single-store erasure operation.
 */
struct StoreErasureResult {
    std::string store_id;          ///< Identifier of the store (e.g. "vector_index")
    bool        success = false;   ///< Whether erasure succeeded
    std::string error_message;     ///< Non-empty on failure
    uint64_t    records_erased = 0;///< Number of records deleted / pseudonymised
};

/**
 * @brief Complete report returned by GdprSubjectRightsManager::requestErasure().
 */
struct ErasureReport {
    std::string subject_id;
    Regulation  regulation = Regulation::GDPR;
    std::string reason;
    std::string operator_id;
    std::chrono::system_clock::time_point timestamp;
    bool        fully_erased = false; ///< true when ALL stores reported success
    std::vector<StoreErasureResult> store_results;

    /// Returns a JSON-serialisable summary.
    std::unordered_map<std::string, std::string> toSummaryMap() const;
};

/**
 * @brief Portable data package returned by requestPortability().
 *
 * The payload is a JSON or CSV byte string; the TSA signature (if a signer
 * is configured) provides eIDAS-compliant legal evidence for GDPR Art. 20.
 */
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

/**
 * @brief Interface that each data store must implement to participate in
 *        cross-module GDPR erasure.
 *
 * Each participating module (index, graph, document, audit log) registers
 * an IGdprEraseTarget with GdprSubjectRightsManager.  The manager calls
 * eraseSubject() on every registered target during requestErasure().
 */
class IGdprEraseTarget {
public:
    virtual ~IGdprEraseTarget() = default;

    /**
     * @brief Return a stable identifier for this store (used in ErasureReport).
     */
    [[nodiscard]] virtual std::string storeId() const = 0;

    /**
     * @brief Delete or pseudonymise all records belonging to subject_id.
     *
     * Implementations must:
     *   - Audit-log entries: pseudonymise (replace PII with "[ERASED_<subject_id>]")
     *     rather than deleting, to preserve tamper-evidence.
     *   - All other records: delete.
     *
     * @param subject_id  The data subject to erase.
     * @param regulation  Regulatory context (GDPR or CCPA).
     * @return StoreErasureResult describing what was done.
     */
    [[nodiscard]] virtual StoreErasureResult eraseSubject(const std::string& subject_id,
                                             Regulation regulation) = 0;

    /**
     * @brief Export all records belonging to subject_id.
     *
     * @param subject_id Data subject identifier.
     * @param format     "json" or "csv".
     * @return Serialised export as a byte vector (empty on failure).
     */
    [[nodiscard]] virtual std::vector<uint8_t> exportSubjectData(
        const std::string& subject_id,
        const std::string& format) = 0;
};

// ============================================================================
// GdprSubjectRightsManager
// ============================================================================

/**
 * @brief Orchestrates GDPR Article 17 (right to erasure) and
 *        Article 20 (data portability) across all registered data stores.
 *
 * Usage:
 * @code
 * auto mgr = std::make_shared<GdprSubjectRightsManager>();
 * mgr->registerEraseTarget(std::make_shared<VectorIndexEraseAdapter>(...));
 * mgr->registerEraseTarget(std::make_shared<GraphStoreEraseAdapter>(...));
 *
 * auto report = mgr->requestErasure("user-1234", Regulation::GDPR, "user request");
 * if (!report.fully_erased) { // handle partial failure
 * }
 * @endcode
 *
 * Thread safety: registerEraseTarget() and requestErasure()/requestPortability()
 * are thread-safe.  Concurrent erasure requests for the same subject_id are
 * serialised per-subject.
 */
class GdprSubjectRightsManager {
public:
    /**
     * @brief TSA signing callback — receives raw export bytes, returns
     *        an RFC 3161 timestamp token (or empty vector on failure).
     */
    using TsaSigner = std::function<std::vector<uint8_t>(
        const std::vector<uint8_t>& data)>;

    explicit GdprSubjectRightsManager(TsaSigner tsa_signer = nullptr);
    ~GdprSubjectRightsManager() = default;

    // Non-copyable
    GdprSubjectRightsManager(const GdprSubjectRightsManager&)            = delete;
    GdprSubjectRightsManager& operator=(const GdprSubjectRightsManager&) = delete;

    // ── Target registration ──────────────────────────────────────────────

    /**
     * @brief Register a data store as an erasure/portability target.
     *
     * @param target Non-null shared_ptr to an IGdprEraseTarget.
     * @throws std::invalid_argument if target is null or its storeId() is empty.
     */
    void registerEraseTarget(std::shared_ptr<IGdprEraseTarget> target);

    /**
     * @brief Return the number of registered erase targets.
     */
    size_t targetCount() const;

    // ── Article 17: Right to Erasure ─────────────────────────────────────

    /**
     * @brief Execute a GDPR/CCPA erasure request across all registered stores.
     *
     * Calls eraseSubject() on every registered target.  On partial failure,
     * the report lists failed stores; no rollback is attempted (erasure is
     * forward-only by design).
     *
     * Concurrent calls for the same subject_id are serialised via a per-subject
     * mutex.
     *
     * @param subject_id  Data subject identifier.
     * @param regulation  GDPR or CCPA.
     * @param reason      Human-readable reason (e.g. "user_request", "legal_hold_lift").
     * @param operator_id Identity of the operator issuing the request.
     * @return ErasureReport with per-store outcomes.
     */
    ErasureReport requestErasure(const std::string& subject_id,
                                  Regulation regulation,
                                  const std::string& reason,
                                  const std::string& operator_id = "system");

    // ── Article 20: Data Portability ─────────────────────────────────────

    /**
     * @brief Export all data belonging to a subject across all registered stores.
     *
     * Collects exports from every registered target, concatenates them, then
     * optionally signs the bundle with the TSA signer (eIDAS evidence).
     *
     * @param subject_id Data subject identifier.
     * @param format     "json" (default) or "csv".
     * @return PortabilityPackage with payload and optional TSA signature.
     */
    PortabilityPackage requestPortability(const std::string& subject_id,
                                           const std::string& format = "json");

private:
    std::vector<std::shared_ptr<IGdprEraseTarget>> targets_;
    TsaSigner tsa_signer_;
    mutable std::mutex targets_mutex_;

    // Per-subject serialisation to prevent concurrent erasure of the same subject
    std::unordered_map<std::string, std::mutex> subject_mutexes_;
    std::mutex subject_map_mutex_;

    std::mutex& getSubjectMutex(const std::string& subject_id);
};

} // namespace governance
} // namespace themis
