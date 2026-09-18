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

class PkiApiHandler {
public:
    /**
     * @brief Pki Api Handler.
     * @param[in] signing_service Input parameter.
     * @return Return value.
     */
    explicit PkiApiHandler(std::shared_ptr<SigningService> signing_service);
    
    // Constructor with HSM and TSA support
    PkiApiHandler(std::shared_ptr<SigningService> signing_service,
                  std::shared_ptr<security::HSMProvider> hsm_provider,
                  std::shared_ptr<security::TimestampAuthority> tsa);

    
    /**
     * @brief Sign.
     * @param[in] key_id Identifier of the key.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json sign(const std::string& key_id, const nlohmann::json& body);

    /**
     * @brief Verify identity and enforce network policies for a request.
     * @param[in] key_id Identifier of the key.
     * @param[in] body Input parameter.
     * @return Verification result.
     */
    nlohmann::json verify(const std::string& key_id, const nlohmann::json& body);

    
    /**
     * @brief Hsm Sign.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json hsmSign(const nlohmann::json& body);
    
    /**
     * @brief Hsm List Keys.
     * @return Return value.
     */
    nlohmann::json hsmListKeys();
    
    
    /**
     * @brief Get Timestamp.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json getTimestamp(const nlohmann::json& body);
    
    /**
     * @brief Verify Timestamp.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json verifyTimestamp(const nlohmann::json& body);
    
    
    /**
     * @brief Eidas Sign.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json eidasSign(const nlohmann::json& body);
    
    /**
     * @brief Eidas Verify.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json eidasVerify(const nlohmann::json& body);
    
    
    /**
     * @brief List Certificates.
     * @return Return value.
     */
    nlohmann::json listCertificates();
    
    /**
     * @brief Get Certificate.
     * @param[in] cert_id Identifier of the cert.
     * @return Return value.
     */
    nlohmann::json getCertificate(const std::string& cert_id);
    
    
    /**
     * @brief Get Status.
     * @return Return value.
     */
    nlohmann::json getStatus();

private:
    std::shared_ptr<SigningService> signing_service_;
    std::shared_ptr<security::HSMProvider> hsm_provider_;
    std::shared_ptr<security::TimestampAuthority> tsa_;
    
    /**
     * @brief Decode Base64.
     * @param[in] b64 Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> decodeBase64(const std::string& b64);
    
    /**
     * @brief Encode Base64.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::string encodeBase64(const std::vector<uint8_t>& data);
};

}} // namespace themis::server
