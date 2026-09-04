/**
 * @file eid_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Delegated Interface**: eID/eIDAS credential validation is delegated to external
 *       eIDAS service providers. This header defines the contract. Implementation delegated
 *       to national eID provider plugins (e.g., German BSI eID, Austrian ID Austria).
 */


/**
 * ThemisDB — German eID Authentication Integrator
 *
 * Provides integration with the German electronic identity card (eID)
 * authentication system defined by the Bundesdruckerei and regulated by
 * BSI Technical Guidelines TR-03110 (Advanced Security Mechanisms for
 * Machine Readable Travel Documents) and TR-03130 (eID-Server).
 *
 * The eID card (Personalausweis, Aufenthaltstitel, eID-Karte für EU-Bürger)
 * enables secure online authentication with verified identity attributes
 * (ICAO 9303 chip-based) via the Online-Ausweisfunktion (OAF).
 *
 * Authentication flow (BSI TR-03130, §5):
 *   1. Service Provider (SP) redirects user to eID-Server (BSI-certified).
 *   2. eID-Server initiates PAOS/SAML exchange with the AusweisApp2 client.
 *   3. AusweisApp2 accesses the chip via NFC or card reader (BSI TR-03127).
 *   4. PACE protocol establishes a secure channel; Chip Authentication and
 *      Terminal Authentication (EAC2) verify the terminal certificate.
 *   5. eID-Server returns a SAML assertion with verified attributes.
 *   6. SP validates the SAML assertion signature (eID-Server certificate).
 *
 * Implementations shipped in this header-only file:
 *   - EIDAttribute          — a verified identity attribute
 *   - EIDIdentity           — complete eID-verified identity record
 *   - EIDAuthConfig         — configuration for the eID integration
 *   - EIDAuthResult         — result of an eID authentication attempt
 *   - IEIDAuthenticator     — abstract authentication interface
 *   - InMemoryEIDAuthenticator — test / simulation implementation
 *
 * Standards references:
 *   - BSI TR-03110 v2.21 — Advanced Security Mechanisms for eMRTD
 *   - BSI TR-03127 v1.20 — Architecture eID-Karte
 *   - BSI TR-03130 v3.3  — eID-Server
 *   - ISO/IEC 7816-4     — Identification cards – ICC with contacts
 *   - ICAO Doc 9303       — Machine Readable Travel Documents
 *   - eIDAS Regulation (EU) No 910/2014 — Electronic identification
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace auth {

// ── EIDAttributeType ─────────────────────────────────────────────────────────

/**
 * @brief Identity attribute types readable from the eID chip (BSI TR-03127, §5).
 *
 * The relying party may request a subset of these attributes; the holder
 * must consent on the AusweisApp2 before they are disclosed.
 */
enum class EIDAttributeType {
    DOCUMENT_TYPE,          ///< "ID" for Personalausweis, "AR" for Aufenthaltstitel
    ISSUING_STATE,          ///< ISO 3166-1 alpha-3 country code (e.g. "DEU")
    DATE_OF_EXPIRY,         ///< YYYYMMDD expiry date of the document
    GIVEN_NAMES,            ///< Given name(s) as on the document
    FAMILY_NAMES,           ///< Family name(s) as on the document
    NOM_DE_PLUME,           ///< Artist / pseudonym name
    ACADEMIC_TITLE,         ///< Academic title (Dr., Prof., …)
    DATE_OF_BIRTH,          ///< YYYYMMDD date of birth
    PLACE_OF_BIRTH,         ///< Free-text birth place
    NATIONALITY,            ///< ISO 3166-1 alpha-3 nationality code
    BIRTH_NAME,             ///< Birth name if different from family name
    PLACE_OF_RESIDENCE,     ///< Structured address (XMELD format)
    MUNICIPALITY_ID,        ///< AGS municipality identifier (Amtlicher Gemeindeschlüssel)
    RESIDENCE_PERMIT_I,     ///< Aufenthaltstitel category I
    RESIDENCE_PERMIT_II,    ///< Aufenthaltstitel category II
    COMMUNITY_ID,           ///< eID community identifier (pseudonymous per SP)
    ADDRESS_VERIFICATION,   ///< Address verified flag (true/false)
    AGE_VERIFICATION,       ///< Age-threshold verification result (true/false)
    RESTRICTED_ID,          ///< Sector-specific pseudonymous identifier
};

// ── EIDAttribute ──────────────────────────────────────────────────────────────

/**
 * @brief A single verified identity attribute returned by the eID chip.
 */
struct EIDAttribute {
    EIDAttributeType type;      ///< Attribute type
    std::string value;          ///< Attribute value (string representation)
    bool verified{false};       ///< True if the value was verified by the eID-Server
};

// ── EIDAssuranceLevel ─────────────────────────────────────────────────────────

/**
 * @brief eIDAS Level of Assurance (LoA) of the authentication.
 *
 * Corresponds to eIDAS Regulation Article 8 and BSI TR-03107.
 */
enum class EIDAssuranceLevel {
    LOW,            ///< eIDAS LoA "low"
    SUBSTANTIAL,    ///< eIDAS LoA "substantial"
    HIGH,           ///< eIDAS LoA "high" — required for German Personalausweis
};

// ── EIDIdentity ───────────────────────────────────────────────────────────────

/**
 * @brief Complete eID-verified identity record.
 *
 * Returned to the relying party after a successful EAC2 / SAML flow.
 * Only attributes explicitly requested in EIDAuthConfig::requested_attributes
 * and consented to by the holder will be populated.
 */
struct EIDIdentity {
    std::string transaction_id;     ///< End-to-end correlation ID from eID-Server
    std::string eid_server_id;      ///< Identifier of the eID-Server instance
    EIDAssuranceLevel assurance{EIDAssuranceLevel::HIGH};

    std::vector<EIDAttribute> attributes; ///< Verified identity attributes

    std::chrono::system_clock::time_point authenticated_at;

    /**
     * @brief Find a specific attribute by type.
     * @return The attribute value, or std::nullopt if not present.
     */
    std::optional<std::string> getAttribute(EIDAttributeType type) const {
        for (const auto& a : attributes) {
            if (a.type == type) {
              return a.value;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Convenience: return the holder's full name (GIVEN_NAMES + FAMILY_NAMES).
     */
    std::string fullName() const {
        auto given  = getAttribute(EIDAttributeType::GIVEN_NAMES);
        auto family = getAttribute(EIDAttributeType::FAMILY_NAMES);
        if (!given && !family) {
          return "";
        }
        if (!given) {
          return *family;
        }
        if (!family) {
          return *given;
        }
        return *given + " " + *family;
    }
};

// ── EIDAuthConfig ─────────────────────────────────────────────────────────────

/**
 * @brief Configuration for the eID authentication integration.
 *
 * The relying party must hold a valid eID-Server certificate issued by the
 * Bundesdruckerei / DMPS (Document Management and Personalisation System).
 */
struct EIDAuthConfig {
    bool enabled{false};

    // eID-Server endpoints (BSI TR-03130, §4)
    std::string eid_server_url;         ///< PAOS endpoint, e.g. "https://eid.example.de/eIDServer/ServiceRequests"
    std::string sp_return_url;          ///< SP callback URL after authentication
    std::string terminal_certificate;   ///< PEM-encoded terminal certificate (AT cert)
    std::string terminal_key_path;      ///< Path to terminal private key (never logged)
    std::string eid_server_ca_cert;     ///< eID-Server CA certificate for TLS validation

    // Requested attributes (holder must consent for each)
    std::vector<EIDAttributeType> requested_attributes;

    // Age / community verification
    std::optional<int> age_verification_threshold; ///< Verify age ≥ N years (or nullopt)
    std::string community_id_sector;               ///< Sector for RESTRICTED_ID derivation

    // Session management
    std::chrono::seconds session_timeout{std::chrono::seconds(300)};

    // Allowed assurance levels
    EIDAssuranceLevel minimum_assurance{EIDAssuranceLevel::HIGH};
};

// ── EIDAuthError ──────────────────────────────────────────────────────────────

/**
 * @brief Structured error from an eID authentication attempt.
 */
enum class EIDAuthErrorCode {
    NONE,
    INVALID_CONFIGURATION,      ///< Missing or invalid EIDAuthConfig field
    EID_SERVER_UNREACHABLE,     ///< Network error reaching the eID-Server
    EID_SERVER_REJECTED,        ///< eID-Server returned an error response
    CHIP_ACCESS_FAILED,         ///< PACE or EAC2 protocol failure on the chip
    CERTIFICATE_EXPIRED,        ///< Terminal certificate has expired
    USER_CANCELLED,             ///< Holder cancelled authentication in AusweisApp2
    CONSENT_DENIED,             ///< Holder denied one or more requested attributes
    SAML_SIGNATURE_INVALID,     ///< SAML assertion signature could not be verified
    SESSION_TIMEOUT,            ///< Authentication session timed out
    INTERNAL_ERROR,             ///< Unexpected internal error
};

// ── EIDAuthResult ─────────────────────────────────────────────────────────────

/**
 * @brief Result of an eID authentication attempt.
 */
struct EIDAuthResult {
    class IdentityResult {
    public:
        IdentityResult() = default;
        IdentityResult(EIDIdentity id)
            : value_(std::move(id)) {}

        IdentityResult& operator=(EIDIdentity id) {
            value_ = std::move(id);
            return *this;
        }

        IdentityResult& operator=(std::optional<EIDIdentity> id) {
            value_ = std::move(id);
            return *this;
        }

        bool has_value() const { return value_.has_value(); }
        explicit operator bool() const { return value_.has_value(); }

        EIDIdentity& value() { return value_.value(); }
        const EIDIdentity& value() const { return value_.value(); }

        EIDIdentity* operator->() {
            return value_ ? &(*value_) : nullptr;
        }

        const EIDIdentity* operator->() const {
            return value_ ? &(*value_) : nullptr;
        }

        std::string fullName() const {
            return value_ ? value_->fullName() : std::string{};
        }

        std::optional<std::string> getAttribute(EIDAttributeType type) const {
            return value_ ? value_->getAttribute(type) : std::nullopt;
        }

    private:
        std::optional<EIDIdentity> value_;
    };

    bool success{false};
    EIDAuthErrorCode error_code{EIDAuthErrorCode::NONE};
    std::string error_message;
    IdentityResult identity; ///< Populated on success

    // Legacy optional-like helpers used in older tests.
    bool has_value() const {
        return success && identity.has_value();
    }

    const EIDIdentity* operator->() const {
        return identity.operator->();
    }

    EIDIdentity* operator->() {
        return identity.operator->();
    }

    static EIDAuthResult Success(EIDIdentity id) {
        EIDAuthResult r;
        r.success  = true;
        r.identity = std::move(id);
        return r;
    }

    static EIDAuthResult Failure(EIDAuthErrorCode code, std::string msg) {
        EIDAuthResult r;
        r.success       = false;
        r.error_code    = code;
        r.error_message = std::move(msg);
        return r;
    }
};

// Legacy compatibility types for older tests.
enum class EIDSessionStatus {
    PENDING,
    COMPLETED,
    FAILED
};

struct EIDAuthRequest {
    std::string transaction_id;
    std::string service_provider;
    std::vector<EIDAttributeType> requested_attributes;
    EIDAssuranceLevel minimum_assurance{EIDAssuranceLevel::HIGH};
};

struct EIDAuthSession {
    std::string session_id;
    std::string redirect_url;
    EIDSessionStatus status{EIDSessionStatus::PENDING};
};

// ── IEIDAuthenticator ─────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for eID-based authentication.
 *
 * Implementations MUST be thread-safe.
 */
class IEIDAuthenticator {
public:
    virtual ~IEIDAuthenticator() = default;

    /**
     * @brief Initialise the authenticator with the given configuration.
     *
     * Must be called before any authentication attempt.
     * @return true on success; false if the configuration is invalid.
     */
    [[nodiscard]] virtual bool initialize(const EIDAuthConfig& config) = 0;

    /**
     * @brief Return true if the authenticator has been successfully initialised.
     */
    [[nodiscard]] virtual bool isInitialized() const = 0;

    /**
     * @brief Begin an eID authentication session.
     *
     * Generates a SAML AuthnRequest / PAOS request and returns the redirect
     * URL to which the user should be sent.
     *
     * @param session_id  Caller-provided session identifier for correlation.
     * @return            The redirect URL (non-empty) or an empty string on error.
     */
    [[nodiscard]] virtual std::string beginAuthSession(std::string_view session_id) = 0;

    /**
     * @brief Complete an eID authentication session.
     *
     * Called by the SP callback handler with the SAML response received from
     * the eID-Server after the holder completes authentication on AusweisApp2.
     *
     * @param session_id     Session identifier returned by beginAuthSession().
     * @param saml_response  Base64-encoded SAML response from the eID-Server.
     * @return               Authentication result with the verified identity or
     *                       a structured error.
     */
    [[nodiscard]] virtual EIDAuthResult completeAuthSession(std::string_view session_id,
                                              std::string_view saml_response) = 0;

    /**
     * @brief Revoke / invalidate an active authentication session.
     */
    virtual void revokeSession(std::string_view session_id) = 0;

    /**
     * @brief Return all active session IDs.
     */
    [[nodiscard]] virtual std::vector<std::string> activeSessions() const = 0;

    /**
     * @brief Return the current configuration (copy).
     */
    [[nodiscard]] virtual EIDAuthConfig config() const = 0;
};

// ── InMemoryEIDAuthenticator ──────────────────────────────────────────────────

/**
 * @brief In-memory simulation of IEIDAuthenticator for unit-testing.
 *
 * Does NOT perform real EAC2 / SAML processing.  Instead, it allows the
 * test to pre-configure expected identities via registerTestIdentity() and
 * returns them deterministically when completeAuthSession() is called.
 *
 * In a production deployment, this class MUST be replaced by a real
 * eID-Server client (e.g. AusweisApp-SDK wrapper, or a BSI-certified
 * eID-Server middleware library).
 */
class InMemoryEIDAuthenticator : public IEIDAuthenticator {
public:
    // ── Test helper ───────────────────────────────────────────────────────────

    /**
     * @brief Pre-configure a verified identity for a given session.
     *
     * When completeAuthSession() is called with @p session_id, the authenticator
     * returns a successful result with this identity.
     */
    void registerTestIdentity(std::string_view session_id,
                               const EIDIdentity& identity) {
        std::unique_lock<std::mutex> lk(mutex_);
        test_identities_[std::string(session_id)] = identity;
    }

    // Legacy helper name kept for compatibility with older tests.
    void storeIdentity(const EIDIdentity& identity) {
        registerTestIdentity(identity.transaction_id, identity);
    }

    /**
     * @brief Pre-configure a failure result for a given session.
     */
    void registerTestFailure(std::string_view session_id,
                             EIDAuthErrorCode code,
                             std::string message) {
        std::unique_lock<std::mutex> lk(mutex_);
        test_failures_[std::string(session_id)] = {code, std::move(message)};
    }

    // ── IEIDAuthenticator ─────────────────────────────────────────────────────

    bool initialize(const EIDAuthConfig& config) override {
        if (!config.enabled) {
          return false;
        }
        if (config.eid_server_url.empty()) {
          return false;
        }
        if (config.terminal_certificate.empty()) {
          return false;
        }
        std::unique_lock<std::mutex> lk(mutex_);
        config_      = config;
        initialized_ = true;
        return true;
    }

    bool isInitialized() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return initialized_;
    }

    std::string beginAuthSession(std::string_view session_id) override {
        std::unique_lock<std::mutex> lk(mutex_);
        if (!initialized_) {
            return "";
        }
        const std::string sid(session_id);
        active_sessions_.insert(sid);
        return config_.eid_server_url + "?sessionId=" + sid;
    }

    // Legacy overload used by older tests.
    EIDAuthSession beginAuthSession(const EIDAuthRequest& request) {
        EIDAuthSession session;
        session.session_id = request.transaction_id;
        session.redirect_url = beginAuthSession(request.transaction_id);
        session.status = session.redirect_url.empty()
            ? EIDSessionStatus::FAILED
            : EIDSessionStatus::PENDING;
        return session;
    }

    EIDAuthResult completeAuthSession(std::string_view session_id,
                                      std::string_view saml_response) override {
        std::unique_lock<std::mutex> lk(mutex_);
        if (!initialized_) {
            return EIDAuthResult::Failure(EIDAuthErrorCode::INVALID_CONFIGURATION,
                                         "Authenticator not initialized");
        }
        const std::string sid(session_id);
        if (!active_sessions_.count(sid)) {
            return EIDAuthResult::Failure(EIDAuthErrorCode::SESSION_TIMEOUT,
                                         "Unknown or expired session: " + sid);
        }
        if (saml_response.empty()) {
            return EIDAuthResult::Failure(EIDAuthErrorCode::SAML_SIGNATURE_INVALID,
                                         "Empty SAML response");
        }
        // Check pre-configured failure.
        auto fail_it = test_failures_.find(sid);
        if (fail_it != test_failures_.end()) {
            active_sessions_.erase(sid);
            return EIDAuthResult::Failure(fail_it->second.first,
                                         fail_it->second.second);
        }
        // Return pre-configured identity.
        auto id_it = test_identities_.find(sid);
        if (id_it != test_identities_.end()) {
            active_sessions_.erase(sid);
            return EIDAuthResult::Success(id_it->second);
        }
        // Default: return a minimal verified identity.
        EIDIdentity id;
        id.transaction_id    = sid;
        id.eid_server_id     = "test-eid-server";
        id.assurance         = EIDAssuranceLevel::HIGH;
        id.authenticated_at  = std::chrono::system_clock::now();
        active_sessions_.erase(sid);
        return EIDAuthResult::Success(std::move(id));
    }

    void revokeSession(std::string_view session_id) override {
        std::unique_lock<std::mutex> lk(mutex_);
        active_sessions_.erase(std::string(session_id));
    }

    std::vector<std::string> activeSessions() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return {active_sessions_.begin(), active_sessions_.end()};
    }

    EIDAuthConfig config() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return config_;
    }

private:
    mutable std::mutex mutex_;
    bool initialized_{false};
    EIDAuthConfig config_;
    std::set<std::string> active_sessions_;
    std::map<std::string, EIDIdentity> test_identities_;
    std::map<std::string, std::pair<EIDAuthErrorCode, std::string>> test_failures_;
};

} // namespace auth
} // namespace themis

