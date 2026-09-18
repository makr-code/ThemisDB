/**
 * @file pki_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "security/signing.h"
#include "security/hsm_provider.h"
#include "security/timestamp_authority.h"

namespace themis { namespace server {

/**
 * PKI API Handler - REST Endpoints for PKI Operations
 * 
 * Provides HTTP API for:
 * - Digital signatures (RSA/ECDSA)
 * - Signature verification
 * - Timestamp tokens (RFC 3161)
 * - Certificate operations
 * - HSM-backed signing
 * - eIDAS qualified signatures
 * 
 * Endpoints:
 * - POST /api/pki/sign - Sign data
 * - POST /api/pki/verify - Verify signature
 * - POST /api/pki/timestamp - Get timestamp token
 * - POST /api/pki/sign-with-timestamp - Sign + timestamp (eIDAS)
 * - GET  /api/pki/certificates - List certificates
 * - GET  /api/pki/certificates/{id} - Get certificate
 * - POST /api/pki/hsm/sign - HSM-backed signing
 */
class PkiApiHandler {
public:
    /**
     * @brief TBD: Describe PkiApiHandler.
     * @param[in] signing_service Input parameter.
     * @return Return value.
     */
    explicit PkiApiHandler(std::shared_ptr<SigningService> signing_service);
    
    // Constructor with HSM and TSA support
    PkiApiHandler(std::shared_ptr<SigningService> signing_service,
                  std::shared_ptr<security::HSMProvider> hsm_provider,
                  std::shared_ptr<security::TimestampAuthority> tsa);

    // === Digital Signatures ===
    
    /**
     * Sign data
     * POST /api/pki/sign
     * Body: {"data_b64": "base64-data", "algorithm": "RSA-SHA256", "key_id": "optional"}
     * Response: {"success": true, "signature_b64": "...", "algorithm": "...", "timestamp": 123456}
     * @brief TBD: Describe sign.
     * @param[in] key_id Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json sign(const std::string& key_id, const nlohmann::json& body);

    /**
     * Verify signature
     * POST /api/pki/verify
     * Body: {"data_b64": "...", "signature_b64": "...", "key_id": "optional"}
     * Response: {"success": true, "valid": true}
     * @brief TBD: Describe verify.
     * @param[in] key_id Input parameter.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json verify(const std::string& key_id, const nlohmann::json& body);

    // === HSM Operations ===
    
    /**
     * Sign with HSM
     * POST /api/pki/hsm/sign
     * Body: {"data_b64": "...", "key_label": "...", "algorithm": "RSA-SHA256"}
     * Response: {"success": true, "signature_b64": "...", "key_id": "...", "hsm_serial": "..."}
     * @brief TBD: Describe hsmSign.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json hsmSign(const nlohmann::json& body);
    
    /**
     * List HSM keys
     * GET /api/pki/hsm/keys
     * Response: {"success": true, "keys": [{label, id, algorithm, can_sign}, ...]}
     * @brief TBD: Describe hsmListKeys.
     * @return Return value.
     */
    nlohmann::json hsmListKeys();
    
    // === Timestamp Operations ===
    
    /**
     * Get timestamp for data
     * POST /api/pki/timestamp
     * Body: {"data_b64": "...", "hash_algorithm": "SHA256"}
     * Response: {"success": true, "timestamp_utc": "...", "token_b64": "...", "serial": "..."}
     * @brief TBD: Describe getTimestamp.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json getTimestamp(const nlohmann::json& body);
    
    /**
     * Verify timestamp
     * POST /api/pki/timestamp/verify
     * Body: {"data_b64": "...", "token_b64": "..."}
     * Response: {"success": true, "valid": true, "timestamp_utc": "..."}
     * @brief TBD: Describe verifyTimestamp.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json verifyTimestamp(const nlohmann::json& body);
    
    // === eIDAS Qualified Signatures ===
    
    /**
     * Create eIDAS qualified signature (sign + timestamp)
     * POST /api/pki/eidas/sign
     * Body: {"data_b64": "...", "key_id": "...", "use_hsm": true, "policy_oid": "..."}
     * Response: {
     *   "success": true,
     *   "signature": {"signature_b64": "...", "algorithm": "..."},
     *   "timestamp": {"timestamp_utc": "...", "token_b64": "..."},
     *   "eidas_compliant": true
     * }
     * @brief TBD: Describe eidasSign.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json eidasSign(const nlohmann::json& body);
    
    /**
     * Verify eIDAS signature
     * POST /api/pki/eidas/verify
     * Body: {"data_b64": "...", "signature_b64": "...", "timestamp_token_b64": "..."}
     * Response: {"success": true, "valid": true, "signature_valid": true, "timestamp_valid": true}
     * @brief TBD: Describe eidasVerify.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json eidasVerify(const nlohmann::json& body);
    
    // === Certificate Operations ===
    
    /**
     * List certificates
     * GET /api/pki/certificates
     * Response: {"success": true, "certificates": [{serial, subject, issuer, valid_from, valid_to}, ...]}
     * @brief TBD: Describe listCertificates.
     * @return Return value.
     */
    nlohmann::json listCertificates();
    
    /**
     * Get certificate by ID
     * GET /api/pki/certificates/{id}
     * Response: {"success": true, "certificate_pem": "...", "subject": "...", "issuer": "..."}
     * @brief TBD: Describe getCertificate.
     * @param[in] cert_id Input parameter.
     * @return Return value.
     */
    nlohmann::json getCertificate(const std::string& cert_id);
    
    // === Health & Status ===
    
    /**
     * Check PKI system status
     * GET /api/pki/status
     * Response: {
     *   "success": true,
     *   "hsm_available": true,
     *   "tsa_available": true,
     *   "signing_service_ready": true
     * }
     * @brief TBD: Describe getStatus.
     * @return Return value.
     */
    nlohmann::json getStatus();

private:
    std::shared_ptr<SigningService> signing_service_;
    std::shared_ptr<security::HSMProvider> hsm_provider_;
    std::shared_ptr<security::TimestampAuthority> tsa_;
    
    /**
     * @brief Helper: Decode base64
     * @param[in] b64 Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> decodeBase64(const std::string& b64);
    
    /**
     * @brief Helper: Encode base64
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::string encodeBase64(const std::vector<uint8_t>& data);
};

}} // namespace themis::server
