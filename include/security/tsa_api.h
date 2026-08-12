/**
 * @file tsa_api.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

/**
 * RFC 3161 Timestamp Authority — API Interface & Wrapper
 *
 * This header defines the high-level, RFC 3161 compliant API surface for
 * Timestamp Authority (TSA) operations in ThemisDB.
 *
 * Design goals:
 *  1. Clean request/response value types (TSARequest, TSAResponse,
 *     TSAVerifyRequest) that decouple callers from the low-level
 *     TimestampToken/TSAConfig internals.
 *  2. Abstract interface (ITSAClient) for dependency injection and
 *     testability — production code and tests work with the interface,
 *     not a concrete class.
 *  3. Integration hooks (TSAEventHooks) that let the platform fire
 *     callbacks on timestamp issuance, verification, and errors without
 *     tight coupling to audit-log or monitoring modules.
 *  4. Factory function (createTSAClient) that wires the concrete
 *     TSAClientWrapper around the existing TimestampAuthority backend.
 *
 * RFC 3161 references:
 *  - Section 2   : Time-Stamp Protocol overview
 *  - Section 2.4 : Time-Stamp Request
 *  - Section 2.4.2: Time-Stamp Response / PKIStatusInfo
 *  - Section 3   : Timestamp token (TSTInfo)
 *
 * Usage:
 * ```cpp
 * TSAConfig cfg;
 * cfg.url = "https://freetsa.org/tsr";
 *
 * TSAEventHooks hooks;
 * hooks.on_timestamp_issued = [](const TSARequest& req,
 *                                const TSAResponse& resp) {
 *     if (resp.success) {
 *         auditLog("timestamp_issued", resp.serial_number);
 *     }
 * };
 *
 * auto client = createTSAClient(cfg, hooks);
 *
 * TSARequest req;
 * req.data = {0x01, 0x02, 0x03};
 * TSAResponse resp = client->requestTimestamp(req);
 * if (resp.success) {
 *     // store resp.token_b64 for long-term validation (eIDAS Art. 32)
 * }
 *
 * TSAVerifyRequest vreq;
 * vreq.data  = req.data;
 * vreq.token = resp.token_der;
 * bool valid = client->verifyToken(vreq);
 * ```
 */

#include "security/timestamp_authority.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace security {

// ============================================================================
// Request / Response value objects
// ============================================================================

/**
 * TSARequest — RFC 3161 Time-Stamp Request parameters.
 *
 * The caller provides either raw data (data_is_hash = false) or a
 * pre-computed digest (data_is_hash = true).  All other fields are optional
 * and default to the values in the TSAConfig passed to createTSAClient().
 */
struct TSARequest {
    /// Data to be timestamped, or a pre-computed message imprint.
    std::vector<uint8_t> data;

    /// When true, `data` is already a hash digest (SHA-256/384/512).
    /// When false (default), the client hashes `data` internally.
    bool data_is_hash = false;

    /// Override the hash algorithm for this request only.
    /// Leave empty to use the algorithm from TSAConfig.
    /// Supported: "SHA256" (default), "SHA384", "SHA512".
    std::string hash_algorithm;

    /// Request the TSA to include its signing certificate in the response.
    /// Defaults to true (recommended for long-term validation per eIDAS Art. 32).
    bool request_cert = true;

    /// Optional RFC 3161 §2.4 policy OID.
    /// Leave empty to use the server's default policy.
    std::string policy_oid;
};

/**
 * TSAResponse — RFC 3161 Time-Stamp Response.
 *
 * Wraps TimestampToken with additional request-level metadata (latency,
 * raw PKI status string) suitable for structured logging and monitoring.
 */
struct TSAResponse {
    /// True when the TSA returned a valid timestamp token (PKIStatus 0 or 1).
    bool success = false;

    // ------------------------------------------------------------------
    // Timestamp identification (from TSTInfo, RFC 3161 §2.4.2)
    // ------------------------------------------------------------------

    /// ISO 8601 UTC timestamp string, e.g. "20260301T193000Z".
    std::string timestamp_utc;

    /// Unix timestamp in milliseconds (derived from TSTInfo genTime).
    uint64_t timestamp_unix_ms = 0;

    /// TSA-assigned serial number (hex string).
    std::string serial_number;

    /// Policy OID applied by the TSA.
    std::string policy_oid;

    /// Hash algorithm used for the message imprint (e.g. "SHA256").
    std::string hash_algorithm;

    // ------------------------------------------------------------------
    // Token storage (for archival / re-verification)
    // ------------------------------------------------------------------

    /// DER-encoded PKCS#7 timestamp token.  Store for long-term validation.
    std::vector<uint8_t> token_der;

    /// Base64-encoded token.  Convenient for JSON/REST transport.
    std::string token_b64;

    // ------------------------------------------------------------------
    // TSA identity (populated when request_cert = true)
    // ------------------------------------------------------------------

    /// RFC 2253 subject name of the TSA signing certificate.
    std::string tsa_name;

    /// Serial number of the TSA signing certificate (hex).
    std::string tsa_serial;

    /// DER-encoded TSA signing certificate.
    std::vector<uint8_t> tsa_cert;

    // ------------------------------------------------------------------
    // RFC 3161 optional fields
    // ------------------------------------------------------------------

    /// True if the TSA included accuracy information in the token.
    bool has_accuracy = false;

    /// Accuracy: whole seconds component (RFC 3161 §2.4.2).
    uint32_t accuracy_seconds = 0;

    /// Accuracy: sub-second milliseconds component (0–999).
    uint32_t accuracy_millis = 0;

    /// Accuracy: sub-millisecond microseconds component (0–999).
    uint32_t accuracy_micros = 0;

    /// True if the TSA guarantees chronological ordering of tokens.
    bool ordering = false;

    // ------------------------------------------------------------------
    // Error / diagnostics
    // ------------------------------------------------------------------

    /// Human-readable error description (non-empty when success == false).
    std::string error_message;

    /// HTTP status code returned by the TSA server.
    int http_status = 0;

    /// PKIStatus value from the TSP response (0 = granted, 1 = grantedWithMods).
    int pki_status = 0;

    /// Round-trip latency for the TSP HTTP request.
    std::chrono::milliseconds request_latency{0};

    // ------------------------------------------------------------------
    // Convenience helpers
    // ------------------------------------------------------------------

    /// Build a TSAResponse from the low-level TimestampToken.
    static TSAResponse fromToken(const TimestampToken& tok,
                                 std::chrono::milliseconds latency = {});
};

/**
 * TSAVerifyRequest — parameters for verifying a stored timestamp token.
 */
struct TSAVerifyRequest {
    /// Original data (or pre-computed digest when data_is_hash = true).
    std::vector<uint8_t> data;

    /// When true, `data` is already a hash digest.
    bool data_is_hash = false;

    /// DER-encoded timestamp token to verify.
    /// Provide either token_der OR token_b64 (token_der takes precedence).
    std::vector<uint8_t> token_der;

    /// Base64-encoded timestamp token (alternative to token_der).
    std::string token_b64;
};

// ============================================================================
// Integration hooks
// ============================================================================

/**
 * TSAEventHooks — optional callbacks fired by TSAClientWrapper.
 *
 * All callbacks are invoked synchronously on the calling thread.
 * Leave any field as nullptr to suppress that callback.
 *
 * Typical use: wire these to the audit-log subsystem or Prometheus counters
 * without creating a compile-time dependency between the TSA module and
 * those subsystems.
 */
struct TSAEventHooks {
    /**
     * Fired after a successful or failed timestamp request.
     * @param req   The original request.
     * @param resp  The response (check resp.success for outcome).
     */
    std::function<void(const TSARequest&, const TSAResponse&)> on_timestamp_issued;

    /**
     * Fired after a token verification attempt.
     * @param req    The original verify request.
     * @param valid  True when the token matches the data.
     */
    std::function<void(const TSAVerifyRequest&, bool valid)> on_token_verified;

    /**
     * Fired whenever an internal error occurs (network failure, parse error, …).
     * @param error  Human-readable error description.
     */
    std::function<void(const std::string& error)> on_error;
};

// ============================================================================
// Abstract interface
// ============================================================================

/**
 * ITSAClient — abstract RFC 3161 Timestamp Authority client.
 *
 * All production code and tests should depend on this interface, not on
 * the concrete TSAClientWrapper or TimestampAuthority classes.
 *
 * Thread safety: implementations are not required to be thread-safe.
 * Callers must ensure serialised access or use one instance per thread.
 */
class ITSAClient {
public:
    virtual ~ITSAClient() = default;

    /**
     * Request a timestamp token from the TSA.
     *
     * @param req  Timestamp request (data or pre-computed hash).
     * @return     TSAResponse; check response.success before using the token.
     */
    [[nodiscard]] virtual TSAResponse requestTimestamp(const TSARequest& req) = 0;

    /**
     * Verify a previously obtained timestamp token against the original data.
     *
     * @param req  Verification request containing the data and token.
     * @return     true when the token is a valid imprint of req.data.
     */
    [[nodiscard]] virtual bool verifyToken(const TSAVerifyRequest& req) = 0;

    /**
     * Check whether the configured TSA endpoint is reachable.
     *
     * @return true when the TSA responds to a lightweight HEAD request.
     */
    [[nodiscard]] virtual bool isAvailable() = 0;

    /**
     * Return the most recent error message, or an empty string.
     */
    [[nodiscard]] virtual std::string getLastError() const = 0;
};

// ============================================================================
// Concrete wrapper
// ============================================================================

/**
 * TSAClientWrapper — production implementation of ITSAClient.
 *
 * Wraps the existing TimestampAuthority backend, translates between the
 * high-level TSARequest/TSAResponse types and the low-level TimestampToken,
 * and fires the registered TSAEventHooks callbacks.
 */
class TSAClientWrapper : public ITSAClient {
public:
    /**
     * Construct with an existing TimestampAuthority and optional hooks.
     *
     * @param authority  Backend TSA client (ownership transferred).
     * @param hooks      Optional integration callbacks.
     */
    explicit TSAClientWrapper(std::unique_ptr<TimestampAuthority> authority,
                              TSAEventHooks hooks = {});

    ~TSAClientWrapper() override = default;

    // Non-copyable (TimestampAuthority is non-copyable).
    TSAClientWrapper(const TSAClientWrapper&) = delete;
    TSAClientWrapper& operator=(const TSAClientWrapper&) = delete;

    // Movable.
    TSAClientWrapper(TSAClientWrapper&&) noexcept;
    TSAClientWrapper& operator=(TSAClientWrapper&&) noexcept;

    TSAResponse requestTimestamp(const TSARequest& req) override;
    bool verifyToken(const TSAVerifyRequest& req) override;
    bool isAvailable() override;
    std::string getLastError() const override;

private:
    std::unique_ptr<TimestampAuthority> authority_;
    TSAEventHooks hooks_;
    std::string last_error_;

    void fireError(const std::string& msg);
};

// ============================================================================
// Factory
// ============================================================================

/**
 * createTSAClient — convenience factory.
 *
 * Creates a TSAClientWrapper backed by a TimestampAuthority configured with
 * the provided TSAConfig.  The optional hooks are wired into the wrapper.
 *
 * @param config  TSA endpoint and algorithm configuration.
 * @param hooks   Optional integration callbacks (default: no-op).
 * @return        Heap-allocated ITSAClient (never null).
 */
std::unique_ptr<ITSAClient> createTSAClient(TSAConfig config,
                                            TSAEventHooks hooks = {});

} // namespace security
} // namespace themis
