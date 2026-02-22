#include "auth/saml_authenticator.h"
#include "auth/auth_error.h"
#include "utils/logger.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <openssl/sha.h>
#include <openssl/err.h>

#include <pugixml.hpp>

#include <zlib.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace auth {

// ============================================================================
// Construction / destruction
// ============================================================================

SAMLAuthenticator::SAMLAuthenticator(const SAMLConfig& config)
    : config_(config)
    , clock_([]() { return std::chrono::system_clock::now(); })
{
    if (config_.sp_entity_id.empty())
        throw std::invalid_argument("SAMLConfig: sp_entity_id must not be empty");
    if (config_.sp_acs_url.empty())
        throw std::invalid_argument("SAMLConfig: sp_acs_url must not be empty");
    if (config_.idp_sso_url.empty())
        throw std::invalid_argument("SAMLConfig: idp_sso_url must not be empty");
    if (config_.idp_entity_id.empty())
        throw std::invalid_argument("SAMLConfig: idp_entity_id must not be empty");
    if (config_.idp_certificate_pem.empty())
        throw std::invalid_argument("SAMLConfig: idp_certificate_pem must not be empty");

    loadIdPCertificate();

    THEMIS_INFO("SAMLAuthenticator initialized: sp_entity_id={}, idp_entity_id={}",
                config_.sp_entity_id, config_.idp_entity_id);
}

SAMLAuthenticator::~SAMLAuthenticator() {
    if (idp_public_key_) {
        EVP_PKEY_free(static_cast<EVP_PKEY*>(idp_public_key_));
        idp_public_key_ = nullptr;
    }
}

void SAMLAuthenticator::loadIdPCertificate() {
    BIO* bio = BIO_new_mem_buf(config_.idp_certificate_pem.data(),
                               static_cast<int>(config_.idp_certificate_pem.size()));
    if (!bio) {
        throw std::runtime_error("SAML: Failed to create BIO for IdP certificate");
    }

    X509* cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!cert) {
        throw std::runtime_error("SAML: Failed to parse IdP X.509 certificate (PEM)");
    }

    EVP_PKEY* pkey = X509_get_pubkey(cert);
    X509_free(cert);

    if (!pkey) {
        throw std::runtime_error("SAML: Failed to extract public key from IdP certificate");
    }

    idp_public_key_ = static_cast<void*>(pkey);
}

// ============================================================================
// Clock override (testing)
// ============================================================================

void SAMLAuthenticator::setClockForTesting(
        std::function<std::chrono::system_clock::time_point()> clock) {
    clock_ = std::move(clock);
}

// ============================================================================
// AuthnRequest (SP-initiated flow)
// ============================================================================

std::string SAMLAuthenticator::generateRequestId() {
    // SAML IDs must be NCName-safe: start with '_' + 32 hex chars
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    std::ostringstream oss;
    oss << '_';
    oss << std::hex << std::setfill('0')
        << std::setw(16) << dist(gen)
        << std::setw(16) << dist(gen);
    return oss.str();
}

std::string SAMLAuthenticator::buildAuthnRequestXml(
        const std::string& request_id,
        const std::string& issue_instant) const
{
    std::ostringstream xml;
    xml << R"(<?xml version="1.0" encoding="UTF-8"?>)"
        << R"(<samlp:AuthnRequest)"
        << R"( xmlns:samlp="urn:oasis:names:tc:SAML:2.0:protocol")"
        << R"( xmlns:saml="urn:oasis:names:tc:SAML:2.0:assertion")"
        << " ID=\"" << request_id << "\""
        << " Version=\"2.0\""
        << " IssueInstant=\"" << issue_instant << "\""
        << " AssertionConsumerServiceURL=\"" << config_.sp_acs_url << "\""
        << " ProtocolBinding=\"urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST\""
        << " Destination=\"" << config_.idp_sso_url << "\">"
        << "<saml:Issuer>" << config_.sp_entity_id << "</saml:Issuer>"
        << "<samlp:NameIDPolicy"
        << " Format=\"" << config_.attr_name_id_format << "\""
        << " AllowCreate=\"true\"/>"
        << "<samlp:RequestedAuthnContext Comparison=\"exact\">"
        << "<saml:AuthnContextClassRef>"
        << config_.requested_authn_context
        << "</saml:AuthnContextClassRef>"
        << "</samlp:RequestedAuthnContext>"
        << "</samlp:AuthnRequest>";
    return xml.str();
}

// Build ISO 8601 UTC timestamp from a time_point
static std::string formatDateTime(std::chrono::system_clock::time_point tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_val{};
#ifdef _WIN32
    gmtime_s(&tm_val, &t);
#else
    gmtime_r(&t, &tm_val);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_val);
    return std::string(buf);
}

std::string SAMLAuthenticator::deflateAndBase64Encode(const std::string& input) {
    // DEFLATE without zlib wrapper (RFC 1951)
    uLongf bound = compressBound(static_cast<uLong>(input.size()));
    std::vector<uint8_t> compressed(bound);

    z_stream zs{};
    // Use deflateInit2 with -MAX_WBITS (raw deflate, no zlib header)
    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED,
                     -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("SAML: deflateInit2 failed");
    }

    zs.avail_in = static_cast<uInt>(input.size());
    zs.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    zs.avail_out = static_cast<uInt>(compressed.size());
    zs.next_out  = compressed.data();

    int rc = ::deflate(&zs, Z_FINISH);
    deflateEnd(&zs);

    if (rc != Z_STREAM_END) {
        throw std::runtime_error("SAML: deflate failed");
    }

    compressed.resize(zs.total_out);

    // Standard Base64 encode via OpenSSL BIO
    BIO* b64_bio  = BIO_new(BIO_f_base64());
    BIO* mem_bio  = BIO_new(BIO_s_mem());
    if (!b64_bio || !mem_bio) {
        BIO_free(b64_bio);
        BIO_free(mem_bio);
        throw std::runtime_error("SAML: BIO allocation failed for base64 encode");
    }
    BIO_push(b64_bio, mem_bio);
    BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64_bio, compressed.data(), static_cast<int>(compressed.size()));
    BIO_flush(b64_bio);

    BUF_MEM* buf_ptr{};
    BIO_get_mem_ptr(mem_bio, &buf_ptr);
    std::string encoded(buf_ptr->data, buf_ptr->length);
    BIO_free_all(b64_bio);
    return encoded;
}

std::string SAMLAuthenticator::urlEncode(const std::string& input) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else {
            oss << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        }
    }
    return oss.str();
}

std::string SAMLAuthenticator::buildAuthnRequestUrl(const std::string& relay_state) const {
    const std::string request_id   = generateRequestId();
    const std::string issue_instant = formatDateTime(clock_());
    const std::string xml           = buildAuthnRequestXml(request_id, issue_instant);
    const std::string deflated      = deflateAndBase64Encode(xml);
    const std::string encoded       = urlEncode(deflated);

    std::string url = config_.idp_sso_url + "?SAMLRequest=" + encoded;
    if (!relay_state.empty()) {
        url += "&RelayState=" + urlEncode(relay_state);
    }
    return url;
}

// ============================================================================
// Base64 decode (for SAMLResponse POST body)
// ============================================================================

std::vector<uint8_t> SAMLAuthenticator::base64Decode(const std::string& input) {
    BIO* b64_bio = BIO_new(BIO_f_base64());
    BIO* mem_bio = BIO_new_mem_buf(input.data(), static_cast<int>(input.size()));
    if (!b64_bio || !mem_bio) {
        BIO_free(b64_bio);
        BIO_free(mem_bio);
        return {};
    }
    BIO_push(b64_bio, mem_bio);
    BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);

    std::vector<uint8_t> decoded(input.size());
    int len = BIO_read(b64_bio, decoded.data(), static_cast<int>(decoded.size()));
    BIO_free_all(b64_bio);

    if (len < 0) return {};
    decoded.resize(static_cast<size_t>(len));
    return decoded;
}

// ============================================================================
// DateTime parsing
// ============================================================================

std::chrono::system_clock::time_point SAMLAuthenticator::parseDateTime(const std::string& s) {
    // Accepts: "2026-02-22T06:13:57Z" or "2026-02-22T06:13:57.000Z"
    if (s.size() < 20) {
        throw std::runtime_error("SAML: Invalid datetime format: " + s);
    }
    std::tm tm_val{};
    int year, mon, day, hour, min, sec;
    if (std::sscanf(s.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d",
                    &year, &mon, &day, &hour, &min, &sec) != 6) {
        throw std::runtime_error("SAML: Failed to parse datetime: " + s);
    }
    tm_val.tm_year = year - 1900;
    tm_val.tm_mon  = mon - 1;
    tm_val.tm_mday = day;
    tm_val.tm_hour = hour;
    tm_val.tm_min  = min;
    tm_val.tm_sec  = sec;
    tm_val.tm_isdst = 0;

#ifdef _WIN32
    std::time_t t = _mkgmtime(&tm_val);
#else
    std::time_t t = timegm(&tm_val);
#endif
    return std::chrono::system_clock::from_time_t(t);
}

// ============================================================================
// XML Signature verification
// ============================================================================

bool SAMLAuthenticator::verifyXmlSignature(
        const std::string& reference_xml,
        const std::string& signature_value_b64,
        const std::string& signed_info_c14n,
        const std::string& digest_value_b64,
        const std::string& digest_algorithm_uri,
        const std::string& sig_algorithm_uri) const
{
    // 1. Decode the claimed digest value from the Reference element
    auto claimed_digest = base64Decode(digest_value_b64);
    if (claimed_digest.empty()) {
        THEMIS_WARN("SAML: Empty digest value");
        return false;
    }

    // 2. Decode the signature bytes
    auto sig_bytes = base64Decode(signature_value_b64);
    if (sig_bytes.empty()) {
        THEMIS_WARN("SAML: Empty signature value");
        return false;
    }

    // 3. Select digest algorithm
    const EVP_MD* digest_md = nullptr;
    if (digest_algorithm_uri.find("sha256") != std::string::npos ||
        digest_algorithm_uri.find("SHA256") != std::string::npos) {
        digest_md = EVP_sha256();
    } else if (digest_algorithm_uri.find("sha1") != std::string::npos ||
               digest_algorithm_uri.find("SHA1") != std::string::npos) {
        digest_md = EVP_sha1();
    } else {
        THEMIS_WARN("SAML: Unsupported digest algorithm: {}", digest_algorithm_uri);
        return false;
    }

    // 4. Select signature algorithm
    const EVP_MD* sig_md = nullptr;
    if (sig_algorithm_uri.find("rsa-sha256") != std::string::npos ||
        sig_algorithm_uri.find("RSA-SHA256") != std::string::npos) {
        sig_md = EVP_sha256();
    } else if (sig_algorithm_uri.find("rsa-sha1") != std::string::npos ||
               sig_algorithm_uri.find("RSA-SHA1") != std::string::npos) {
        sig_md = EVP_sha1();
    } else {
        THEMIS_WARN("SAML: Unsupported signature algorithm: {}", sig_algorithm_uri);
        return false;
    }

    // 5. Compute the digest of the referenced element (reference_xml) and compare
    //    against the DigestValue in the Reference element.
    //    Note: proper SAML XML signature verification requires exclusive C14N
    //    (https://www.w3.org/TR/xml-exc-c14n/) for the referenced element. This
    //    implementation uses raw XML serialization as an approximation. For
    //    production deployments integrating with strict IdPs, integrate an XML C14N
    //    library (e.g. libxml2 with c14n support).
    {
        std::vector<uint8_t> computed_digest(EVP_MAX_MD_SIZE);
        unsigned int computed_len = 0;
        EVP_MD_CTX* mctx_ref = EVP_MD_CTX_new();
        if (!mctx_ref) return false;
        EVP_DigestInit_ex(mctx_ref, digest_md, nullptr);
        EVP_DigestUpdate(mctx_ref, reference_xml.data(), reference_xml.size());
        EVP_DigestFinal_ex(mctx_ref, computed_digest.data(), &computed_len);
        EVP_MD_CTX_free(mctx_ref);
        computed_digest.resize(computed_len);

        if (computed_digest.size() != claimed_digest.size() ||
            CRYPTO_memcmp(computed_digest.data(), claimed_digest.data(),
                          computed_digest.size()) != 0) {
            THEMIS_WARN("SAML: Reference DigestValue mismatch");
            return false;
        }
    }

    // 6. Verify RSA signature over the SignedInfo canonicalization
    EVP_PKEY* pkey = static_cast<EVP_PKEY*>(idp_public_key_);
    EVP_MD_CTX* mctx_sig = EVP_MD_CTX_new();
    if (!mctx_sig) return false;

    int verify_result = 0;
    if (EVP_DigestVerifyInit(mctx_sig, nullptr, sig_md, nullptr, pkey) == 1) {
        if (EVP_DigestVerifyUpdate(mctx_sig,
                                   signed_info_c14n.data(),
                                   signed_info_c14n.size()) == 1) {
            verify_result = EVP_DigestVerifyFinal(mctx_sig,
                                                  sig_bytes.data(),
                                                  sig_bytes.size());
        }
    }
    EVP_MD_CTX_free(mctx_sig);

    if (verify_result != 1) {
        THEMIS_WARN("SAML: XML signature verification failed");
        return false;
    }
    return true;
}

// ============================================================================
// Helper: extract text from an XML node selected by xpath-like path
// ============================================================================

namespace {

/// Find the first child element with the given local name (ignoring namespace prefix)
pugi::xml_node findChildByLocalName(const pugi::xml_node& parent, const char* local_name) {
    for (auto child : parent.children()) {
        std::string child_name = child.name();
        auto colon = child_name.rfind(':');
        std::string lname = (colon != std::string::npos)
                            ? child_name.substr(colon + 1)
                            : child_name;
        if (lname == local_name) return child;
    }
    return {};
}

/// Recursively find first element with given local name in the subtree
pugi::xml_node findDescendantByLocalName(const pugi::xml_node& root, const char* local_name) {
    for (auto child : root.children()) {
        std::string child_name = child.name();
        auto colon = child_name.rfind(':');
        std::string lname = (colon != std::string::npos)
                            ? child_name.substr(colon + 1)
                            : child_name;
        if (lname == local_name) return child;
        auto found = findDescendantByLocalName(child, local_name);
        if (found) return found;
    }
    return {};
}

/// Extract all text content from a node (including child nodes)
std::string nodeText(const pugi::xml_node& node) {
    std::string result;
    for (auto child : node.children()) {
        if (child.type() == pugi::node_pcdata || child.type() == pugi::node_cdata) {
            result += child.value();
        } else {
            result += nodeText(child);
        }
    }
    return result;
}

/// Serialize a node back to XML string
std::string nodeToString(const pugi::xml_node& node) {
    std::ostringstream oss;
    node.print(oss, "", pugi::format_raw);
    return oss.str();
}

/// Find the Signature element inside a node
pugi::xml_node findSignature(const pugi::xml_node& node) {
    return findDescendantByLocalName(node, "Signature");
}

} // anonymous namespace

// ============================================================================
// processResponse – main entry point
// ============================================================================

SAMLClaims SAMLAuthenticator::processResponse(
        const std::string& saml_response_b64,
        const std::string& in_response_to) const
{
    // ----------------------------------------------------------------
    // Step 1: Base64-decode the response
    // ----------------------------------------------------------------
    auto raw_bytes = base64Decode(saml_response_b64);
    if (raw_bytes.empty()) {
        THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_RESPONSE,
                         "Invalid SAML response",
                         "Base64 decode of SAMLResponse failed or produced empty output");
    }
    const std::string xml_str(reinterpret_cast<const char*>(raw_bytes.data()),
                               raw_bytes.size());

    // ----------------------------------------------------------------
    // Step 2: Parse XML
    // ----------------------------------------------------------------
    pugi::xml_document doc;
    auto parse_result = doc.load_string(xml_str.c_str());
    if (!parse_result) {
        THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_RESPONSE,
                         "Invalid SAML response",
                         std::string("XML parse error: ") + parse_result.description());
    }

    pugi::xml_node response_node = doc.first_child();
    if (!response_node) {
        THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_RESPONSE,
                         "Invalid SAML response",
                         "XML document is empty");
    }

    // ----------------------------------------------------------------
    // Step 3: Check Status
    // ----------------------------------------------------------------
    {
        auto status_node = findDescendantByLocalName(response_node, "Status");
        if (!status_node) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_RESPONSE,
                             "Invalid SAML response",
                             "Missing Status element");
        }
        auto status_code_node = findChildByLocalName(status_node, "StatusCode");
        if (!status_code_node) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_RESPONSE,
                             "Invalid SAML response",
                             "Missing StatusCode element");
        }
        std::string status_value = status_code_node.attribute("Value").as_string("");
        const std::string SUCCESS_STATUS = "urn:oasis:names:tc:SAML:2.0:status:Success";
        if (status_value != SUCCESS_STATUS) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_STATUS_FAILURE,
                             "Authentication failed",
                             "SAML Status is not Success: " + status_value);
        }
    }

    // ----------------------------------------------------------------
    // Step 4 & 5: Verify XML signatures
    // ----------------------------------------------------------------
    // Find Response-level signature
    auto response_sig = findSignature(response_node);

    // Find Assertion element (may be inside EncryptedAssertion in future)
    auto assertion_node = findDescendantByLocalName(response_node, "Assertion");
    if (!assertion_node) {
        THROW_AUTH_ERROR(AuthErrorCode::SAML_MISSING_ASSERTION,
                         "Invalid SAML response",
                         "No Assertion element found in SAMLResponse");
    }

    auto assertion_sig = findSignature(assertion_node);

    auto validateSignature = [&](const pugi::xml_node& element,
                                  const pugi::xml_node& sig_node,
                                  const std::string& element_name) {
        auto signed_info_node = findChildByLocalName(sig_node, "SignedInfo");
        if (!signed_info_node) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_SIGNATURE,
                             "Invalid SAML signature",
                             element_name + " Signature missing SignedInfo");
        }

        auto sig_value_node = findChildByLocalName(sig_node, "SignatureValue");
        if (!sig_value_node) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_SIGNATURE,
                             "Invalid SAML signature",
                             element_name + " Signature missing SignatureValue");
        }

        auto sig_method_node = findChildByLocalName(signed_info_node, "SignatureMethod");
        auto reference_node  = findChildByLocalName(signed_info_node, "Reference");
        if (!sig_method_node || !reference_node) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_SIGNATURE,
                             "Invalid SAML signature",
                             element_name + " SignedInfo missing SignatureMethod or Reference");
        }

        auto digest_method_node = findChildByLocalName(reference_node, "DigestMethod");
        auto digest_value_node  = findChildByLocalName(reference_node, "DigestValue");
        if (!digest_method_node || !digest_value_node) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_SIGNATURE,
                             "Invalid SAML signature",
                             element_name + " Reference missing DigestMethod or DigestValue");
        }

        std::string sig_alg     = sig_method_node.attribute("Algorithm").as_string("");
        std::string digest_alg  = digest_method_node.attribute("Algorithm").as_string("");
        std::string sig_value   = nodeText(sig_value_node);
        std::string digest_val  = nodeText(digest_value_node);

        // C14N of SignedInfo (use raw serialization as approximation; real
        // implementations use exclusive C14N but for structural correctness
        // the serialized form is used here—full C14N requires an XML C14N library)
        std::string signed_info_c14n = nodeToString(signed_info_node);
        std::string element_xml      = nodeToString(element);

        // Strip whitespace from base64 values
        sig_value.erase(std::remove_if(sig_value.begin(), sig_value.end(),
                                        [](char c){ return std::isspace((unsigned char)c); }),
                        sig_value.end());
        digest_val.erase(std::remove_if(digest_val.begin(), digest_val.end(),
                                         [](char c){ return std::isspace((unsigned char)c); }),
                         digest_val.end());

        if (!verifyXmlSignature(element_xml, sig_value, signed_info_c14n,
                                digest_val, digest_alg, sig_alg)) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_SIGNATURE,
                             "Invalid SAML signature",
                             element_name + " XML signature verification failed");
        }
    };

    if (config_.require_signed_response) {
        if (!response_sig) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_SIGNATURE,
                             "Invalid SAML response",
                             "SAMLResponse is not signed (require_signed_response=true)");
        }
        validateSignature(response_node, response_sig, "SAMLResponse");
    }

    if (config_.require_signed_assertion) {
        if (!assertion_sig) {
            // If the Response was signed and contains the Assertion, that is
            // sometimes acceptable; however our default requires an Assertion sig.
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_SIGNATURE,
                             "Invalid SAML response",
                             "Assertion is not signed (require_signed_assertion=true)");
        }
        validateSignature(assertion_node, assertion_sig, "Assertion");
    }

    // ----------------------------------------------------------------
    // Step 6: Validate Issuer
    // ----------------------------------------------------------------
    {
        auto issuer_node = findChildByLocalName(assertion_node, "Issuer");
        std::string issuer = issuer_node ? nodeText(issuer_node) : "";
        if (issuer != config_.idp_entity_id) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_ISSUER_MISMATCH,
                             "Invalid SAML response",
                             "Issuer mismatch: expected '" + config_.idp_entity_id +
                             "', got '" + issuer + "'");
        }
    }

    // ----------------------------------------------------------------
    // Step 7: Validate Conditions
    // ----------------------------------------------------------------
    const auto now = clock_();
    const auto skew = config_.clock_skew;

    {
        auto conditions_node = findDescendantByLocalName(assertion_node, "Conditions");
        if (conditions_node) {
            auto nb_attr = conditions_node.attribute("NotBefore");
            auto noa_attr = conditions_node.attribute("NotOnOrAfter");

            if (nb_attr) {
                auto not_before = parseDateTime(nb_attr.as_string(""));
                if (now + skew < not_before) {
                    THROW_AUTH_ERROR(AuthErrorCode::SAML_CONDITIONS_FAILED,
                                     "Invalid SAML response",
                                     "Assertion not yet valid (NotBefore)");
                }
            }

            if (noa_attr) {
                auto not_on_or_after = parseDateTime(noa_attr.as_string(""));
                if (now - skew >= not_on_or_after) {
                    THROW_AUTH_ERROR(AuthErrorCode::SAML_CONDITIONS_FAILED,
                                     "Invalid SAML response",
                                     "Assertion has expired (NotOnOrAfter)");
                }
            }

            // AudienceRestriction
            auto audience_restriction = findDescendantByLocalName(conditions_node, "AudienceRestriction");
            if (audience_restriction) {
                bool audience_ok = false;
                for (auto aud : audience_restriction.children()) {
                    std::string aud_name = aud.name();
                    auto colon = aud_name.rfind(':');
                    if ((colon != std::string::npos ? aud_name.substr(colon + 1) : aud_name) == "Audience") {
                        if (nodeText(aud) == config_.sp_entity_id) {
                            audience_ok = true;
                            break;
                        }
                    }
                }
                if (!audience_ok) {
                    THROW_AUTH_ERROR(AuthErrorCode::SAML_CONDITIONS_FAILED,
                                     "Invalid SAML response",
                                     "SP entity ID not in AudienceRestriction");
                }
            }
        }

        // SubjectConfirmation NotOnOrAfter
        auto subject_node = findDescendantByLocalName(assertion_node, "Subject");
        if (subject_node) {
            auto sc_node = findDescendantByLocalName(subject_node, "SubjectConfirmation");
            if (sc_node) {
                auto sc_data = findChildByLocalName(sc_node, "SubjectConfirmationData");
                if (sc_data) {
                    auto noa_attr2 = sc_data.attribute("NotOnOrAfter");
                    if (noa_attr2) {
                        auto not_on_or_after2 = parseDateTime(noa_attr2.as_string(""));
                        if (now - skew >= not_on_or_after2) {
                            THROW_AUTH_ERROR(AuthErrorCode::SAML_CONDITIONS_FAILED,
                                             "Invalid SAML response",
                                             "SubjectConfirmation has expired");
                        }
                    }

                    // Recipient check (ACS URL)
                    auto recipient_attr = sc_data.attribute("Recipient");
                    if (recipient_attr) {
                        std::string recipient = recipient_attr.as_string("");
                        if (!recipient.empty() && recipient != config_.sp_acs_url) {
                            THROW_AUTH_ERROR(AuthErrorCode::SAML_DESTINATION_MISMATCH,
                                             "Invalid SAML response",
                                             "SubjectConfirmationData Recipient mismatch");
                        }
                    }

                    // InResponseTo
                    if (!in_response_to.empty()) {
                        auto irt_attr = sc_data.attribute("InResponseTo");
                        if (irt_attr) {
                            std::string irt = irt_attr.as_string("");
                            if (!irt.empty() && irt != in_response_to) {
                                THROW_AUTH_ERROR(AuthErrorCode::SAML_CONDITIONS_FAILED,
                                                 "Invalid SAML response",
                                                 "InResponseTo mismatch");
                            }
                        }
                    }
                }
            }
        }
    }

    // ----------------------------------------------------------------
    // Step 9: Replay detection (TTL-based eviction using NotOnOrAfter + clock_skew)
    // ----------------------------------------------------------------
    std::string assertion_id = assertion_node.attribute("ID").as_string("");
    if (assertion_id.empty()) {
        THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_RESPONSE,
                         "Invalid SAML response",
                         "Assertion is missing ID attribute");
    }

    // Compute expiry for this assertion: use SubjectConfirmationData NotOnOrAfter
    // (already validated above) + clock_skew as the cache TTL.
    auto assertion_expiry = now + config_.clock_skew;
    {
        auto subject_n = findDescendantByLocalName(assertion_node, "Subject");
        if (subject_n) {
            auto sc_n = findDescendantByLocalName(subject_n, "SubjectConfirmation");
            if (sc_n) {
                auto scd_n = findChildByLocalName(sc_n, "SubjectConfirmationData");
                if (scd_n) {
                    auto noa = scd_n.attribute("NotOnOrAfter");
                    if (noa) {
                        try {
                            assertion_expiry = parseDateTime(noa.as_string("")) + config_.clock_skew;
                        } catch (...) {}
                    }
                }
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(replay_cache_mutex_);

        // Evict expired entries before checking/inserting (lazy TTL eviction).
        for (auto it = seen_assertion_ids_.begin(); it != seen_assertion_ids_.end(); ) {
            if (now >= it->second) {
                it = seen_assertion_ids_.erase(it);
            } else {
                ++it;
            }
        }

        // Check for replay
        if (seen_assertion_ids_.count(assertion_id)) {
            THROW_AUTH_ERROR(AuthErrorCode::SAML_REPLAY_DETECTED,
                             "Authentication replay detected",
                             "Assertion ID '" + assertion_id + "' has already been used");
        }

        // Enforce maximum cache size (after eviction). If still full, fail closed.
        if (seen_assertion_ids_.size() >= config_.max_replay_cache_size) {
            THEMIS_WARN("SAML: Replay cache is full ({} entries). "
                        "Rejecting assertion to prevent cache bypass. "
                        "Consider using a distributed TTL cache for high-volume deployments.",
                        config_.max_replay_cache_size);
            THROW_AUTH_ERROR(AuthErrorCode::SAML_INVALID_RESPONSE,
                             "Authentication temporarily unavailable",
                             "SAML replay cache is full; assertion rejected");
        }

        seen_assertion_ids_.emplace(assertion_id, assertion_expiry);
    }

    // ----------------------------------------------------------------
    // Step 10: Extract claims
    // ----------------------------------------------------------------
    SAMLClaims claims;
    claims.assertion_id = assertion_id;

    // Issuer
    {
        auto issuer_node = findChildByLocalName(assertion_node, "Issuer");
        claims.issuer = issuer_node ? nodeText(issuer_node) : "";
    }

    // IssueInstant
    {
        auto ii_attr = assertion_node.attribute("IssueInstant");
        if (ii_attr) {
            try { claims.issued_at = parseDateTime(ii_attr.as_string("")); }
            catch (const std::exception& e) {
                THEMIS_WARN("SAML: Failed to parse IssueInstant '{}': {}",
                            ii_attr.as_string(""), e.what());
            }
        }
    }

    // Subject / NameID
    {
        auto subject_node = findDescendantByLocalName(assertion_node, "Subject");
        if (subject_node) {
            auto name_id_node = findChildByLocalName(subject_node, "NameID");
            if (name_id_node) {
                claims.subject_name_id = nodeText(name_id_node);
                claims.name_id_format  = name_id_node.attribute("Format").as_string("");
            }
        }
    }

    // Conditions (for claims output)
    {
        auto cond_node = findDescendantByLocalName(assertion_node, "Conditions");
        if (cond_node) {
            auto nb_attr = cond_node.attribute("NotBefore");
            auto noa_attr = cond_node.attribute("NotOnOrAfter");
            try {
                if (nb_attr)  claims.not_before       = parseDateTime(nb_attr.as_string(""));
                if (noa_attr) claims.not_on_or_after   = parseDateTime(noa_attr.as_string(""));
            } catch (const std::exception& e) {
                THEMIS_WARN("SAML: Failed to parse Conditions datetime: {}", e.what());
            }

            auto ar_node = findDescendantByLocalName(cond_node, "AudienceRestriction");
            if (ar_node) {
                for (auto child : ar_node.children()) {
                    std::string cn = child.name();
                    auto c = cn.rfind(':');
                    if ((c != std::string::npos ? cn.substr(c + 1) : cn) == "Audience") {
                        claims.audience.push_back(nodeText(child));
                    }
                }
            }
        }
    }

    // SubjectConfirmationData NotBefore/NotOnOrAfter
    {
        auto sub_node = findDescendantByLocalName(assertion_node, "Subject");
        if (sub_node) {
            auto sc_node = findDescendantByLocalName(sub_node, "SubjectConfirmation");
            if (sc_node) {
                auto scd_node = findChildByLocalName(sc_node, "SubjectConfirmationData");
                if (scd_node) {
                    try {
                        auto nb  = scd_node.attribute("NotBefore");
                        auto noa = scd_node.attribute("NotOnOrAfter");
                        if (nb)  claims.not_before       = parseDateTime(nb.as_string(""));
                        if (noa) claims.not_on_or_after   = parseDateTime(noa.as_string(""));
                    } catch (const std::exception& e) {
                        THEMIS_WARN("SAML: Failed to parse SubjectConfirmationData datetime: {}",
                                    e.what());
                    }
                }
            }
        }
    }

    // AuthnStatement SessionIndex
    {
        auto authn_stmt = findDescendantByLocalName(assertion_node, "AuthnStatement");
        if (authn_stmt) {
            auto si_attr = authn_stmt.attribute("SessionIndex");
            if (si_attr) claims.session_index = si_attr.as_string("");
        }
    }

    // AttributeStatement
    {
        auto attr_stmt = findDescendantByLocalName(assertion_node, "AttributeStatement");
        if (attr_stmt) {
            for (auto attr_node : attr_stmt.children()) {
                // Attribute or saml:Attribute
                std::string attr_tag = attr_node.name();
                auto colon = attr_tag.rfind(':');
                std::string lname = (colon != std::string::npos)
                                    ? attr_tag.substr(colon + 1)
                                    : attr_tag;
                if (lname != "Attribute") continue;

                std::string attr_name = attr_node.attribute("Name").as_string("");
                for (auto val_node : attr_node.children()) {
                    std::string vn = val_node.name();
                    auto cv = vn.rfind(':');
                    if ((cv != std::string::npos ? vn.substr(cv + 1) : vn) == "AttributeValue") {
                        std::string val = nodeText(val_node);
                        claims.raw_attributes.emplace_back(attr_name, val);

                        // Map to known claims
                        if (attr_name == config_.attr_email ||
                            attr_name == "http://schemas.xmlsoap.org/ws/2005/05/identity/claims/emailaddress") {
                            if (claims.email.empty()) claims.email = val;
                        }
                        if (attr_name == "groups" || attr_name == "memberOf" ||
                            attr_name == "http://schemas.microsoft.com/ws/2008/06/identity/claims/groups") {
                            claims.attributes_groups.push_back(val);
                        }
                        if (attr_name == "roles" || attr_name == "Role" ||
                            attr_name == "http://schemas.microsoft.com/ws/2008/06/identity/claims/role") {
                            claims.attributes_roles.push_back(val);
                        }
                    }
                }
            }
        }
    }

    // Fallback: if email not found in attributes, use NameID if it looks like an email
    if (claims.email.empty() &&
        claims.name_id_format.find("emailAddress") != std::string::npos) {
        claims.email = claims.subject_name_id;
    }

    THEMIS_INFO("SAML: Authentication successful: subject={}, email={}, issuer={}",
                AuthError::maskSensitiveData(claims.subject_name_id),
                AuthError::maskSensitiveData(claims.email),
                claims.issuer);

    return claims;
}

} // namespace auth
} // namespace themis
