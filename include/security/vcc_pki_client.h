/**
 * @file vcc_pki_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: MIT
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

#include <nlohmann/json.hpp>

namespace themis {

// Basic certificate container used by the PKI client.
struct X509Certificate {
	std::string id = {};
	std::string pem;
	std::string subject;
	std::string issuer;
	int64_t not_before_ms{0};
	int64_t not_after_ms{0};
	std::string key_usage;
	std::vector<std::string> san;

	/**
	 * @brief Is Valid.
	 * @return True on success.
	 */
	bool isValid() const;
	/**
	 * @brief Is Expired.
	 * @param[in] now_ms Input parameter.
	 * @return True on success.
	 */
	bool isExpired(int64_t now_ms) const;
	/**
	 * @brief To Json.
	 * @return Return value.
	 */
	nlohmann::json toJson() const;
	/**
	 * @brief From Json.
	 * @param[in] j Input parameter.
	 * @return Return value.
	 */
	static X509Certificate fromJson(const nlohmann::json& j);
};

// Single entry in a certificate revocation list.
struct CRLEntry {
	std::string serial_number;
	int64_t revocation_time_ms{0};
	std::string reason;

	/**
	 * @brief To Json.
	 * @return Return value.
	 */
	nlohmann::json toJson() const;
	/**
	 * @brief From Json.
	 * @param[in] j Input parameter.
	 * @return Return value.
	 */
	static CRLEntry fromJson(const nlohmann::json& j);
};

// Request payload used when asking the CA for a new certificate.
struct CertificateRequest {
	std::string common_name;
	std::string organization;
	std::vector<std::string> san;
	std::string key_usage;
	int validity_days{0};

	/**
	 * @brief To Json.
	 * @return Return value.
	 */
	nlohmann::json toJson() const;
};

// TLS configuration used by the PKI client.
struct TLSConfig {
	std::string ca_cert_path;
	std::string client_cert_path;
	std::string client_key_path;
	bool verify_server{true};
	bool use_mtls{false};
};

/** @brief Vccpki client component. */
class VCCPKIClient {
public:
	VCCPKIClient(const std::string& base_url, const TLSConfig& tls_config, int timeout_ms);
	~VCCPKIClient();

	VCCPKIClient(VCCPKIClient&&) noexcept;
	VCCPKIClient& operator=(VCCPKIClient&&) noexcept;

	/**
	 * @brief Http Get.
	 * @param[in] path Input parameter.
	 * @return Return value.
	 */
	std::string httpGet(const std::string& path);
	/**
	 * @brief Http Post.
	 * @param[in] path Input parameter.
	 * @param[in] body Input parameter.
	 * @return Return value.
	 */
	std::string httpPost(const std::string& path, const nlohmann::json& body);

	/**
	 * @brief Request Certificate.
	 * @param[in] request Input parameter.
	 * @return Return value.
	 */
	X509Certificate requestCertificate(const CertificateRequest& request);
	/**
	 * @brief Get Certificate.
	 * @param[in] cert_id Input parameter.
	 * @return Return value.
	 */
	X509Certificate getCertificate(const std::string& cert_id);
	/**
	 * @brief Get CRL.
	 * @return Return value.
	 */
	std::vector<CRLEntry> getCRL();
	/**
	 * @brief Is Revoked.
	 * @param[in] cert_id Input parameter.
	 * @param[in] crl Input parameter.
	 * @return True on success.
	 */
	bool isRevoked(const std::string& cert_id, const std::vector<CRLEntry>& crl) const;
	/**
	 * @brief Health Check.
	 * @return True on success.
	 */
	bool healthCheck();
	/**
	 * @brief Parse Certificate.
	 * @param[in] pem Input parameter.
	 * @return Return value.
	 */
	X509Certificate parseCertificate(const std::string& pem);
	/**
	 * @brief Validate Cert Chain.
	 * @param[in] cert Input parameter.
	 * @return True on success.
	 */
	bool validateCertChain(const X509Certificate& cert) const;

private:
	struct Impl;

	std::string base_url_;
	TLSConfig tls_config_;
	int timeout_ms_{0};
	std::unique_ptr<Impl> impl_;
};

} // namespace themis
