/**
 * @file vcc_pki_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

	bool isValid() const;
	bool isExpired(int64_t now_ms) const;
	nlohmann::json toJson() const;
	static X509Certificate fromJson(const nlohmann::json& j);
};

// Single entry in a certificate revocation list.
struct CRLEntry {
	std::string serial_number;
	int64_t revocation_time_ms{0};
	std::string reason;

	nlohmann::json toJson() const;
	static CRLEntry fromJson(const nlohmann::json& j);
};

// Request payload used when asking the CA for a new certificate.
struct CertificateRequest {
	std::string common_name;
	std::string organization;
	std::vector<std::string> san;
	std::string key_usage;
	int validity_days{0};

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

	std::string httpGet(const std::string& path);
	std::string httpPost(const std::string& path, const nlohmann::json& body);

	X509Certificate requestCertificate(const CertificateRequest& request);
	X509Certificate getCertificate(const std::string& cert_id);
	std::vector<CRLEntry> getCRL();
	bool isRevoked(const std::string& cert_id, const std::vector<CRLEntry>& crl) const;
	bool healthCheck();
	X509Certificate parseCertificate(const std::string& pem);
	bool validateCertChain(const X509Certificate& cert) const;

private:
	struct Impl;

	std::string base_url_;
	TLSConfig tls_config_;
	int timeout_ms_{0};
	std::unique_ptr<Impl> impl_;
};

} // namespace themis
