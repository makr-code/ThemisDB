#include "sharding/pki_shard_certificate.h"
#include "utils/openssl_deleter.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>

// OpenSSL headers
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>

namespace themis::sharding {

namespace {
    // Helper: Read file contents
    std::optional<std::string> readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return std::nullopt;
        }
        
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    
    // Helper: Convert ASN1_TIME to ISO 8601 string
    std::string asn1TimeToString(const ASN1_TIME* time) {
        if (!time) return "";
        
        auto bio = themis::utils::BIOPtr(BIO_new(BIO_s_mem()));
        ASN1_TIME_print(bio.get(), time);
        
        char* data = nullptr;
        long len = BIO_get_mem_data(bio.get(), &data);
        std::string result(data, len);
        
        return result;
    }
    
    // Helper: Get extension value by NID
    [[maybe_unused]]
    std::optional<std::string> getExtensionValue(X509* cert, int nid) {
        int idx = X509_get_ext_by_NID(cert, nid, -1);
        if (idx < 0) {
            return std::nullopt;
        }
        
        X509_EXTENSION* ext = X509_get_ext(cert, idx);
        if (!ext) {
            return std::nullopt;
        }
        
        ASN1_OCTET_STRING* data = X509_EXTENSION_get_data(ext);
        if (!data) {
            return std::nullopt;
        }
        
        const unsigned char* p = data->data;
        long len = data->length;
        
        return std::string(reinterpret_cast<const char*>(p), len);
    }
}

std::optional<std::string> PKIShardCertificate::extractCustomOID(void* x509_cert_ptr, const std::string& oid_string) {
    X509* cert = static_cast<X509*>(x509_cert_ptr);
    if (!cert) {
        return std::nullopt;
    }
    
    // Convert OID string to ASN1_OBJECT
    auto oid = utils::ASN1ObjectPtr(OBJ_txt2obj(oid_string.c_str(), 0));
    if (!oid) {
        return std::nullopt;
    }
    
    // Find extension in certificate by OID
    int ext_count = X509_get_ext_count(cert);
    for (int i = 0; i < ext_count; i++) {
        X509_EXTENSION* ext = X509_get_ext(cert, i);
        if (!ext) continue;
        
        ASN1_OBJECT* ext_oid = X509_EXTENSION_get_object(ext);
        if (!ext_oid) continue;
        
        // Compare OIDs
        if (OBJ_cmp(ext_oid, oid.get()) == 0) {
            // Extract value from extension
            ASN1_OCTET_STRING* data = X509_EXTENSION_get_data(ext);
            if (!data) continue;
            
            // Parse ASN.1 data using OpenSSL's proper parsing
            const unsigned char* p = data->data;
            long xlen = data->length;
            
            // Try to parse as UTF8String using OpenSSL's d2i function
            ASN1_UTF8STRING* utf8str = nullptr;
            p = data->data;  // Reset pointer for d2i
            utf8str = d2i_ASN1_UTF8STRING(nullptr, &p, xlen);
            
            if (utf8str) {
                std::string result(
                    reinterpret_cast<const char*>(ASN1_STRING_get0_data(utf8str)),
                    ASN1_STRING_length(utf8str)
                );
                ASN1_UTF8STRING_free(utf8str);
                return result;
            }
            
            // Fallback: try as IA5String or PrintableString
            ASN1_STRING* str = nullptr;
            p = data->data;  // Reset pointer
            str = d2i_ASN1_PRINTABLESTRING(nullptr, &p, xlen);
            
            if (str) {
                std::string result(
                    reinterpret_cast<const char*>(ASN1_STRING_get0_data(str)),
                    ASN1_STRING_length(str)
                );
                ASN1_STRING_free(str);
                return result;
            }
            
            // Last resort: treat entire data as raw string
            return std::string(reinterpret_cast<const char*>(data->data), data->length);
        }
    }
    
    return std::nullopt;
}

bool ShardCertificateInfo::isValidNow() const {
    // For Phase 2, we'll implement a simple check
    // In production, this should parse not_before/not_after and compare with current time
    // For now, return true if both dates are set
    return !not_before.empty() && !not_after.empty();
}

std::optional<ShardCertificateInfo> PKIShardCertificate::parseCertificate(const std::string& cert_path) {
    auto pem_data = readFile(cert_path);
    if (!pem_data) {
        return std::nullopt;
    }
    
    return parseCertificatePEM(*pem_data);
}

std::optional<ShardCertificateInfo> PKIShardCertificate::parseCertificatePEM(const std::string& pem_data) {
    auto bio = utils::make_bio_mem_buf(pem_data.c_str(), static_cast<int>(pem_data.size()));
    if (!bio) {
        return std::nullopt;
    }
    
    auto cert = utils::read_x509_from_bio(bio.get());
    
    if (!cert) {
        return std::nullopt;
    }
    
    ShardCertificateInfo info;
    
    // Extract subject CN
    X509_NAME* subject = X509_get_subject_name(cert.get());
    if (subject) {
        char cn_buf[256] = {0};
        X509_NAME_get_text_by_NID(subject, NID_commonName, cn_buf, sizeof(cn_buf));
        info.subject_cn = cn_buf;
    }
    
    // Extract issuer CN
    X509_NAME* issuer = X509_get_issuer_name(cert.get());
    if (issuer) {
        char issuer_buf[256] = {0};
        X509_NAME_get_text_by_NID(issuer, NID_commonName, issuer_buf, sizeof(issuer_buf));
        info.issuer_cn = issuer_buf;
    }
    
    // Extract serial number
    ASN1_INTEGER* serial = X509_get_serialNumber(cert.get());
    if (serial) {
        auto bn = utils::BIGNUMPtr(ASN1_INTEGER_to_BN(serial, nullptr));
        if (bn) {
            char* hex = BN_bn2hex(bn.get());
            if (hex) {
                info.serial_number = hex;
                OPENSSL_free(hex);
            }
        }
    }
    
    // Extract validity dates
    const ASN1_TIME* not_before = X509_get0_notBefore(cert.get());
    const ASN1_TIME* not_after = X509_get0_notAfter(cert.get());
    info.not_before = asn1TimeToString(not_before);
    info.not_after = asn1TimeToString(not_after);
    
    // Parse Subject Alternative Names
    parseSAN(cert.get(), info);
    
    // Parse custom extensions (shard-specific)
    // Uses COMPATIBLE mode by default (try OID first, fallback to CN)
    parseCustomExtensions(cert.get(), info, ValidationMode::COMPATIBLE);
    
    return info;
}

bool PKIShardCertificate::verifyCertificate(const std::string& cert_path, const std::string& ca_cert_path) {
    // Read certificate
    auto cert_pem = readFile(cert_path);
    if (!cert_pem) {
        return false;
    }
    
    // Read CA certificate
    auto ca_pem = readFile(ca_cert_path);
    if (!ca_pem) {
        return false;
    }
    
    // Parse certificate
    auto cert_bio = utils::make_bio_mem_buf(cert_pem->c_str(), static_cast<int>(cert_pem->size()));
    auto cert = utils::read_x509_from_bio(cert_bio.get());
    
    if (!cert) {
        return false;
    }
    
    // Parse CA certificate
    auto ca_bio = utils::make_bio_mem_buf(ca_pem->c_str(), static_cast<int>(ca_pem->size()));
    auto ca_cert = utils::read_x509_from_bio(ca_bio.get());
    
    if (!ca_cert) {
        return false;
    }
    
    // Get CA public key
    auto ca_pubkey = utils::EVPKeyPtr(X509_get_pubkey(ca_cert.get()));
    if (!ca_pubkey) {
        return false;
    }
    
    // Verify certificate signature
    int result = X509_verify(cert.get(), ca_pubkey.get());
    
    return result == 1;
}

bool PKIShardCertificate::isRevoked(const std::string& serial_number, const std::string& crl_path) {
    // Read CRL file
    auto crl_pem = readFile(crl_path);
    if (!crl_pem) {
        return false; // If CRL doesn't exist, assume not revoked
    }
    
    auto bio = utils::make_bio_mem_buf(crl_pem->c_str(), static_cast<int>(crl_pem->size()));
    if (!bio) {
        return false;
    }
    
    auto crl = utils::read_x509_crl_from_bio(bio.get());
    
    if (!crl) {
        return false;
    }
    
    // Check if serial number is in CRL
    STACK_OF(X509_REVOKED)* revoked = X509_CRL_get_REVOKED(crl.get());
    if (!revoked) {
        return false;
    }
    
    bool found = false;
    for (int i = 0; i < sk_X509_REVOKED_num(revoked); ++i) {
        X509_REVOKED* r = sk_X509_REVOKED_value(revoked, i);
        const ASN1_INTEGER* r_serial = X509_REVOKED_get0_serialNumber(r);
        
        auto bn = utils::BIGNUMPtr(ASN1_INTEGER_to_BN(r_serial, nullptr));
        if (bn) {
            char* hex = BN_bn2hex(bn.get());
            if (hex) {
                if (serial_number == hex) {
                    found = true;
                }
                OPENSSL_free(hex);
            }
        }
        
        if (found) break;
    }
    
    return found;
}

std::optional<std::string> PKIShardCertificate::getShardId(const std::string& cert_path) {
    auto info = parseCertificate(cert_path);
    if (!info || info->shard_id.empty()) {
        return std::nullopt;
    }
    
    return info->shard_id;
}

bool PKIShardCertificate::validateShardCertificate(const ShardCertificateInfo& info) {
    // Check validity dates
    if (!info.isValidNow()) {
        return false;
    }
    
    // Check shard ID is present
    if (info.shard_id.empty()) {
        return false;
    }
    
    // Check at least one capability
    if (info.capabilities.empty()) {
        return false;
    }
    
    // Check token range is valid
    // Special case: start=0, end=0 indicates "full range" and is valid
    // Otherwise, require start < end
    if (info.token_range_start >= info.token_range_end && info.token_range_end != 0) {
        return false;
    }
    
    return true;
}

bool PKIShardCertificate::parseCustomExtensions(void* x509_cert_ptr, ShardCertificateInfo& info, ValidationMode mode) {
    X509* cert = static_cast<X509*>(x509_cert_ptr);
    if (!cert) {
        return false;
    }
    
    // Try to extract shard_id from custom OID first
    auto oid_shard_id = extractCustomOID(cert, OIDRegistry::SHARD_ID_OID);
    if (oid_shard_id) {
        info.shard_id = *oid_shard_id;
        info.shard_id_from_oid = true;
    }
    
    // Try to extract region from custom OID
    auto oid_region = extractCustomOID(cert, OIDRegistry::REGION_OID);
    if (oid_region) {
        info.region = *oid_region;
    }
    
    // Try to extract role from custom OID
    auto oid_role = extractCustomOID(cert, OIDRegistry::ROLE_OID);
    if (oid_role) {
        info.role = *oid_role;
    }
    
    // Fallback to CN extraction if OID not found and mode allows
    if (info.shard_id.empty() && mode != ValidationMode::STRICT) {
        if (info.subject_cn.find("shard-") == 0) {
            // Extract shard ID from CN (e.g., "shard-001.themis.local" -> "shard_001")
            size_t dot_pos = info.subject_cn.find('.');
            std::string shard_name = info.subject_cn.substr(0, dot_pos);
            // Replace dash with underscore
            for (char& c : shard_name) {
                if (c == '-') c = '_';
            }
            info.shard_id = shard_name;
            info.shard_id_from_oid = false;
        }
    }
    
    // Default capabilities if not provided
    if (info.capabilities.empty()) {
        info.capabilities = {"read", "write"};
    }
    
    // Default role if not set
    if (info.role.empty()) {
        info.role = "primary";
    }
    
    // Note: Token range is NOT set by default here.
    // If both start and end are 0, it indicates "full range" (special case).
    // The validation logic accepts this as valid.
    
    return true;
}

bool PKIShardCertificate::parseSAN(void* x509_cert_ptr, ShardCertificateInfo& info) {
    X509* cert = static_cast<X509*>(x509_cert_ptr);
    
    GENERAL_NAMES* san_names = static_cast<GENERAL_NAMES*>(
        X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr)
    );
    
    if (!san_names) {
        return false;
    }
    
    int num_sans = sk_GENERAL_NAME_num(san_names);
    for (int i = 0; i < num_sans; ++i) {
        GENERAL_NAME* gen_name = sk_GENERAL_NAME_value(san_names, i);
        
        if (gen_name->type == GEN_DNS) {
            ASN1_STRING* dns = gen_name->d.dNSName;
            std::string dns_str(reinterpret_cast<const char*>(ASN1_STRING_get0_data(dns)),
                              ASN1_STRING_length(dns));
            info.san_dns.push_back(dns_str);
        }
        else if (gen_name->type == GEN_IPADD) {
            ASN1_OCTET_STRING* ip = gen_name->d.iPAddress;
            // Convert IP address to string
            if (ip->length == 4) { // IPv4
                char ip_str[16];
                snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
                        ip->data[0], ip->data[1], ip->data[2], ip->data[3]);
                info.san_ip.push_back(ip_str);
            }
        }
        else if (gen_name->type == GEN_URI) {
            ASN1_STRING* uri = gen_name->d.uniformResourceIdentifier;
            std::string uri_str(reinterpret_cast<const char*>(ASN1_STRING_get0_data(uri)),
                              ASN1_STRING_length(uri));
            info.san_uri.push_back(uri_str);
        }
    }
    
    GENERAL_NAMES_free(san_names);
    return true;
}

bool PKIShardCertificate::validateWithMode(const ShardCertificateInfo& info, ValidationMode mode) {
    // Check validity dates
    if (!info.isValidNow()) {
        return false;
    }
    
    // Check shard ID is present
    if (info.shard_id.empty()) {
        return false;
    }
    
    // STRICT mode: Require OID-based extraction
    if (mode == ValidationMode::STRICT && !info.shard_id_from_oid) {
        return false;
    }
    
    // Check at least one capability
    if (info.capabilities.empty()) {
        return false;
    }
    
    // Check token range is valid
    // Special case: start=0, end=0 indicates "full range" and is valid
    // Otherwise, require start < end
    if (info.token_range_start >= info.token_range_end && info.token_range_end != 0) {
        return false;
    }
    
    return true;
}

std::optional<ShardIdentity> PKIShardCertificate::extractIdentity(const std::string& cert_path) {
    auto info = parseCertificate(cert_path);
    if (!info) {
        return std::nullopt;
    }
    
    ShardIdentity identity;
    identity.shard_id = info->shard_id;
    identity.region = info->region;
    identity.role = info->role;
    identity.from_oid = info->shard_id_from_oid;
    
    // Collect all SANs into a single vector
    identity.sans.insert(identity.sans.end(), info->san_dns.begin(), info->san_dns.end());
    identity.sans.insert(identity.sans.end(), info->san_ip.begin(), info->san_ip.end());
    identity.sans.insert(identity.sans.end(), info->san_uri.begin(), info->san_uri.end());
    
    return identity;
}

bool PKIShardCertificate::validateEKU(const std::string& cert_path) {
    auto pem_data = readFile(cert_path);
    if (!pem_data) {
        return false;
    }
    
    auto bio = utils::make_bio_mem_buf(pem_data->c_str(), static_cast<int>(pem_data->size()));
    if (!bio) {
        return false;
    }
    
    auto cert = utils::read_x509_from_bio(bio.get());
    if (!cert) {
        return false;
    }
    
    // Get Extended Key Usage extension
    EXTENDED_KEY_USAGE* eku = static_cast<EXTENDED_KEY_USAGE*>(
        X509_get_ext_d2i(cert.get(), NID_ext_key_usage, nullptr, nullptr)
    );
    
    if (!eku) {
        // No EKU extension, accept (compatibility mode)
        return true;
    }
    
    // Check if custom node authentication OID is present
    auto node_auth_oid = utils::ASN1ObjectPtr(OBJ_txt2obj(OIDRegistry::NODE_AUTH_EKU, 0));
    if (!node_auth_oid) {
        EXTENDED_KEY_USAGE_free(eku);
        return false;
    }
    
    bool found = false;
    int eku_count = sk_ASN1_OBJECT_num(eku);
    for (int i = 0; i < eku_count; i++) {
        ASN1_OBJECT* oid = sk_ASN1_OBJECT_value(eku, i);
        if (OBJ_cmp(oid, node_auth_oid.get()) == 0) {
            found = true;
            break;
        }
    }
    
    EXTENDED_KEY_USAGE_free(eku);
    return found;
}

} // namespace themis::sharding
