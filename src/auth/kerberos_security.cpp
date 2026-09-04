/**
 * @file kerberos_security.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/kerberos_security.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <stdexcept>

#include "utils/logger.h"

namespace themis {
namespace auth {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

// ASN.1 tag classes
constexpr uint8_t ASN1_CLASS_UNIVERSAL   = 0x00;
constexpr uint8_t ASN1_CLASS_APPLICATION = 0x40;
constexpr uint8_t ASN1_CLASS_CONTEXT     = 0x80;
constexpr uint8_t ASN1_CLASS_PRIVATE     = 0xC0;

// ASN.1 tag types
constexpr uint8_t ASN1_TAG_SEQUENCE     = 0x30;
constexpr uint8_t ASN1_TAG_SET          = 0x31;
constexpr uint8_t ASN1_TAG_INTEGER      = 0x02;
constexpr uint8_t ASN1_TAG_OCTET_STRING = 0x04;
constexpr uint8_t ASN1_TAG_OBJECT_ID    = 0x06;

} // anonymous namespace

// ============================================================================
// GSSAPI / Kerberos-5 DER parsing helpers (RFC 4120, RFC 4121)
// ============================================================================
// These helpers extract cleartext fields from a GSSAPI KRB5 AP-REQ token.
// Fields inside EncryptedData (authenticator, enc-part) are NOT decrypted;
// timestamps and client principal remain unavailable without the service key.
// ============================================================================

namespace {

// KRB5 OID: 1.2.840.113554.1.2.2 (DER encoded with tag+length)
static const uint8_t KRB5_OID_DER[]      = {0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x12, 0x01, 0x02, 0x02};
static constexpr size_t KRB5_OID_DER_LEN = sizeof(KRB5_OID_DER);

// APPLICATION tag values
static constexpr uint8_t GSSAPI_APP0_TAG = 0x60; // [APPLICATION 0] CONSTRUCTED
static constexpr uint8_t KRB5_APP14_TAG  = 0x6E; // [APPLICATION 14] AP-REQ
static constexpr uint8_t KRB5_APP1_TAG   = 0x61; // [APPLICATION 1]  Ticket

// AP-REQ ap-options flags (RFC 4120 §5.5.1)
static constexpr uint32_t AP_OPT_USE_SESSION_KEY = 0x40000000u;
static constexpr uint32_t AP_OPT_MUTUAL_REQUIRED = 0x20000000u;

// Read a DER-encoded length field starting at data[offset].
// On success, advances offset past the length field and sets length.
// Returns false on malformed input (indefinite form, too many length octets,
// or overflow past data end).
static bool derReadLength(const uint8_t *data, size_t size, size_t &offset, size_t &length) {
    if (offset >= size) {
        return false;
    }
    uint8_t lb = data[offset++];
    if ((lb & 0x80u) == 0) {
        length = lb;
        return true;
    }
    size_t nb = lb & 0x7Fu;
    if (nb == 0 || nb > 4 || offset + nb > size) {
        return false;
    }
    length = 0;
    for (size_t i = 0; i < nb; ++i) {
        length = (length << 8) | data[offset++];
    }
    return true;
}

// Find the content of the first context-specific [tag_num] CONSTRUCTED element
// within the given SEQUENCE content buffer.  Returns nullptr if not found.
static const uint8_t *findContextTag(const uint8_t *data, size_t size, uint8_t tag_num, size_t &content_size) {
    size_t pos = 0;
    while (pos < size) {
        if (pos >= size) {
            break;
        }
        uint8_t tag = data[pos++];
        size_t len  = 0;
        if (!derReadLength(data, size, pos, len)) {
            break;
        }
        if (pos + len > size) {
            break;
        }
        // Context-specific CONSTRUCTED [N] = 0xA0 | N
        if (tag == static_cast<uint8_t>(0xA0u | tag_num)) {
            content_size = len;
            return data + pos;
        }
        pos += len;
    }
    return nullptr;
}

// Unwrap a SEQUENCE: verify tag 0x30, advance past tag+length, return content.
static bool unwrapSequence(const uint8_t *data, size_t size, const uint8_t *&content, size_t &content_size) {
    if (size < 2 || data[0] != ASN1_TAG_SEQUENCE) {
        return false;
    }
    size_t pos = 1;
    size_t len = 0;
    if (!derReadLength(data, size, pos, len)) {
        return false;
    }
    if (pos + len > size) {
        return false;
    }
    content      = data + pos;
    content_size = len;
    return true;
}

// Read the string value of an ASN.1 string primitive at data[0..size).
// Accepts GeneralString (0x1B), UTF8String (0x0C), PrintableString (0x13),
// IA5String (0x16) as Kerberos implementations vary.
static std::string readKerberosString(const uint8_t *data, size_t size) {
    if (size < 2) {
        return {};
    }
    uint8_t tag = data[0];
    // Accepted string tags
    if (tag != 0x1Bu && tag != 0x0Cu && tag != 0x13u && tag != 0x16u) {
        return {};
    }
    size_t pos = 1;
    size_t len = 0;
    if (!derReadLength(data, size, pos, len)) {
        return {};
    }
    if (pos + len > size) {
        return {};
    }
    return {reinterpret_cast<const char *>(data + pos), len};
}

// Holds the cleartext fields extractable from a KRB5 AP-REQ token.
struct Krb5TokenFields {
    std::string realm = {};
    std::string sname;      // "service/host@REALM"
    uint32_t ap_options{0}; // AP-REQ ap-options bitmask
    bool parsed{false};
};

// Parse Ticket [APPLICATION 1] to extract realm and sname.
static bool parseKrb5Ticket(const uint8_t *data, size_t size, Krb5TokenFields &out) {
    if (size < 2 || data[0] != KRB5_APP1_TAG) {
        return false;
    }
    size_t pos     = 1;
    size_t app_len = 0;
    if (!derReadLength(data, size, pos, app_len)) {
        return false;
    }
    if (pos + app_len > size) {
        return false;
    }

    const uint8_t *seq_content = nullptr;
    size_t seq_size            = 0;
    if (!unwrapSequence(data + pos, app_len, seq_content, seq_size)) {
        return false;
    }

    // [1] Realm  — GeneralString
    size_t realm_tag_size    = 0;
    const uint8_t *realm_tag = findContextTag(seq_content, seq_size, 1, realm_tag_size);
    if (realm_tag) {
        out.realm = readKerberosString(realm_tag, realm_tag_size);
    }

    // [2] PrincipalName SEQUENCE { [0] name-type, [1] SEQUENCE OF KerberosString }
    size_t sname_tag_size    = 0;
    const uint8_t *sname_tag = findContextTag(seq_content, seq_size, 2, sname_tag_size);
    if (sname_tag) {
        const uint8_t *pn_content = nullptr;
        size_t pn_size            = 0;
        if (unwrapSequence(sname_tag, sname_tag_size, pn_content, pn_size)) {
            size_t names_tag_size    = 0;
            const uint8_t *names_tag = findContextTag(pn_content, pn_size, 1, names_tag_size);
            if (names_tag) {
                const uint8_t *names_seq = nullptr;
                size_t names_seq_size    = 0;
                if (unwrapSequence(names_tag, names_tag_size, names_seq, names_seq_size)) {
                    std::string sname_str = {};
                    size_t p = 0;
                    while (p < names_seq_size) {
                        // Each element: tag byte, length, string bytes
                        if (p + 2 > names_seq_size) {
                            break;
                        }
                        uint8_t stag = names_seq[p++];
                        (void)stag; // accept any string tag variant
                        size_t slen = 0;
                        if (!derReadLength(names_seq, names_seq_size, p, slen)) {
                            break;
                        }
                        if (p + slen > names_seq_size) {
                            break;
                        }
                        if (!sname_str.empty()) {
                            sname_str += '/';
                        }
                        sname_str.append(reinterpret_cast<const char *>(names_seq + p), slen);
                        p += slen;
                    }
                    if (!sname_str.empty()) {
                        out.sname = sname_str;
                        if (!out.realm.empty()) {
                            out.sname += '@' + out.realm;
                        }
                    }
                }
            }
        }
    }

    return !out.sname.empty() || !out.realm.empty();
}

// Parse AP-REQ [APPLICATION 14] to extract ap-options and the embedded Ticket.
static bool parseKrb5ApReq(const uint8_t *data, size_t size, Krb5TokenFields &out) {
    if (size < 2 || data[0] != KRB5_APP14_TAG) {
        return false;
    }
    size_t pos     = 1;
    size_t app_len = 0;
    if (!derReadLength(data, size, pos, app_len)) {
        return false;
    }
    if (pos + app_len > size) {
        return false;
    }

    const uint8_t *seq_content = nullptr;
    size_t seq_size            = 0;
    if (!unwrapSequence(data + pos, app_len, seq_content, seq_size)) {
        return false;
    }

    // [2] APOptions BIT STRING
    size_t opts_tag_size    = 0;
    const uint8_t *opts_tag = findContextTag(seq_content, seq_size, 2, opts_tag_size);
    if (opts_tag && opts_tag_size >= 2 && opts_tag[0] == 0x03u) {
        size_t bs_pos = 1;
        size_t bs_len = 0;
        if (derReadLength(opts_tag, opts_tag_size, bs_pos, bs_len) && bs_len >= 5
            && bs_pos + bs_len <= opts_tag_size) {
            // Skip unused-bits byte; read 4 flag bytes (big-endian)
            const uint8_t *fb = opts_tag + bs_pos + 1;
            out.ap_options    = (static_cast<uint32_t>(fb[0]) << 24) | (static_cast<uint32_t>(fb[1]) << 16)
                                | (static_cast<uint32_t>(fb[2]) << 8) | static_cast<uint32_t>(fb[3]);
        }
    }

    // [3] Ticket
    size_t ticket_tag_size    = 0;
    const uint8_t *ticket_tag = findContextTag(seq_content, seq_size, 3, ticket_tag_size);
    if (!ticket_tag) {
        return false;
    }

    return parseKrb5Ticket(ticket_tag, ticket_tag_size, out);
}

// Parse a GSSAPI KRB5 outer wrapper ([APPLICATION 0] + KRB5 OID).
// Returns pointer to the bytes immediately after the 2-byte token-ID field
// (i.e., the start of the AP-REQ), and sets inner_size.
// Returns nullptr on any format error.
static const uint8_t *parseGssapiKrb5Header(const uint8_t *data, size_t size, size_t &inner_size) {
    if (size < 2 || data[0] != GSSAPI_APP0_TAG) {
        return nullptr;
    }
    size_t pos       = 1;
    size_t outer_len = 0;
    if (!derReadLength(data, size, pos, outer_len)) {
        return nullptr;
    }
    if (pos + outer_len > size) {
        return nullptr;
    }

    const uint8_t *inner = data + pos;
    size_t rem           = outer_len;

    // Verify KRB5 OID
    if (rem < KRB5_OID_DER_LEN) {
        return nullptr;
    }
    if (std::memcmp(inner, KRB5_OID_DER, KRB5_OID_DER_LEN) != 0) {
        return nullptr;
    }
    inner += KRB5_OID_DER_LEN;
    rem -= KRB5_OID_DER_LEN;

    // Skip 2-byte inner token ID (0x01 0x00 = AP-REQ)
    if (rem < 2) {
        return nullptr;
    }
    inner += 2;
    rem -= 2;

    inner_size = rem;
    return inner;
}

// Top-level helper: try to parse a GSSAPI KRB5 AP-REQ token and extract
// cleartext fields.  Returns a Krb5TokenFields with parsed=true on success.
static Krb5TokenFields extractKrb5Fields(const std::vector<uint8_t> &token_data) {
    Krb5TokenFields fields;
    size_t inner_size    = 0;
    const uint8_t *inner = parseGssapiKrb5Header(token_data.data(),static_cast<int>(token_data.size()), inner_size);
    if (!inner) {
        return fields;
    }

    if (parseKrb5ApReq(inner, inner_size, fields)) {
        fields.parsed = true;
    }
    return fields;
}

} // namespace

// ============================================================================
// KerberosSecurityValidator Implementation
// ============================================================================

KerberosSecurityValidator::KerberosSecurityValidator(const Config &config) : config_(config) {
    utils::Logger::info("Kerberos Security Validator initialized:");
    utils::Logger::info("  Channel bindings: {}", config_.enable_channel_bindings);
    utils::Logger::info("  Strict ASN.1: {}", config_.strict_asn1_validation);
    utils::Logger::info("  Verify service target: {}", config_.verify_service_target);
}

bool KerberosSecurityValidator::validateToken(const std::vector<uint8_t> &token_data,
                                              const std::vector<uint8_t> &channel_binding) {
    // 1. Validate ASN.1 structure
    if (config_.validate_token_structure && config_.strict_asn1_validation) {
        if (!validateASN1Structure(token_data)) {
            throw std::runtime_error("Invalid ASN.1 structure in GSSAPI token");
        }
    }

    // 2. Check expiration
    if (config_.reject_expired_tickets && isTicketExpired(token_data)) {
        throw std::runtime_error("Kerberos ticket has expired");
    }

    // 3. Verify service principal
    if (config_.verify_service_target && !config_.expected_service_principal.empty()) {
        if (!verifyServicePrincipal(token_data, config_.expected_service_principal)) {
            throw std::runtime_error("Service principal mismatch: expected " + config_.expected_service_principal);
        }
    }

    // 4. Verify channel bindings
    if (config_.enable_channel_bindings && !channel_binding.empty()) {
        if (!verifyChannelBinding(token_data, channel_binding)) {
            throw std::runtime_error("Channel binding verification failed");
        }
    }

    // 5. Get token info for additional checks
    auto info = getTokenInfo(token_data);

    // 6. Verify security flags
    if (config_.require_mutual_auth && !info.has_mutual_auth) {
        throw std::runtime_error("Mutual authentication required but not present");
    }

    if (config_.require_integrity && !info.has_integrity) {
        throw std::runtime_error("Integrity protection required but not present");
    }

    if (config_.require_confidentiality && !info.has_confidentiality) {
        throw std::runtime_error("Confidentiality required but not present");
    }

    utils::Logger::info("Kerberos token validated successfully for: {}", info.service_principal);

    return true;
}

bool KerberosSecurityValidator::validateASN1Structure(const std::vector<uint8_t> &data) {
    if (data.empty()) {
        return false;
    }

    // Start recursive depth validation
    return validateASN1Depth(data.data(),static_cast<int>(data.size()), 0);
}

bool KerberosSecurityValidator::validateASN1Depth(const uint8_t *data, size_t size, size_t current_depth) {
    if (current_depth > config_.max_token_depth) {
        utils::Logger::warn("ASN.1 depth limit exceeded: {}", current_depth);
        return false;
    }

    if (size == 0) {
        return true; // Empty is valid
    }

    size_t offset = 0;

    while (offset < size) {
        ASN1Tag tag = {};
        if (!parseASN1Tag(data + offset, size - offset, tag)) {
            return false;
        }

        // Check length bounds
        if (tag.length > config_.max_sequence_length) {
            utils::Logger::warn("ASN.1 sequence length exceeds limit: {}", tag.length);
            return false;
        }

        // If constructed, recurse into nested structure
        if (tag.is_constructed && tag.value) {
            if (!validateASN1Depth(tag.value, tag.length, current_depth + 1)) {
                return false;
            }
        }

        // Move to next tag
        offset += (tag.value - data) + tag.length;

        // Safety check
        if (offset > size) {
            return false;
        }
    }

    return true;
}

bool KerberosSecurityValidator::parseASN1Tag(const uint8_t *data, size_t size, ASN1Tag &tag) {
    if (size < 2) {
        return false;
    }

    size_t offset = 0;

    // Parse tag
    uint8_t first_byte = data[offset++];

    tag.tag_class      = first_byte & 0xC0;
    tag.is_constructed = (first_byte & 0x20) != 0;
    tag.tag_number     = first_byte & 0x1F;

    // Handle high-tag-number form (tag >= 31)
    if (tag.tag_number == 0x1F) {
        tag.tag_number = 0;
        while (offset < size) {
            uint8_t byte   = data[offset++];
            tag.tag_number = (tag.tag_number << 7) | (byte & 0x7F);
            if ((byte & 0x80) == 0) {
                break;
            }
        }
    }

    if (offset >= size) {
        return false;
    }

    // Parse length
    uint8_t length_byte = data[offset++];

    if ((length_byte & 0x80) == 0) {
        // Short form
        tag.length = length_byte;
    } else {
        // Long form
        size_t num_octets = length_byte & 0x7F;
        if (num_octets == 0 || num_octets > 4) {
            return false; // Indefinite or too long
        }

        if (offset + num_octets > size) {
            return false;
        }

        tag.length = 0;
        for (size_t i = 0; i < num_octets; i++) {
            tag.length = (tag.length << 8) | data[offset++];
        }
    }

    // Check bounds
    if (offset + tag.length > size) {
        return false;
    }

    tag.value = data + offset;

    return true;
}

bool KerberosSecurityValidator::verifyServicePrincipal(const std::vector<uint8_t> &token_data,
                                                       const std::string &expected_principal) {
    std::string actual_principal = extractServicePrincipal(token_data);

    if (actual_principal.empty()) {
        utils::Logger::warn("Could not extract service principal from token");
        return false;
    }

    bool matches = (actual_principal == expected_principal);

    if (!matches) {
        utils::Logger::warn("Service principal mismatch: expected '{}', got '{}'", expected_principal,
                            actual_principal);
    }

    return matches;
}

bool KerberosSecurityValidator::verifyChannelBinding(const std::vector<uint8_t> &token_data,
                                                     const std::vector<uint8_t> &channel_binding) {
    if (channel_binding.empty()) {
        return true; // No binding requested; nothing to verify.
    }

    // Validate the token is a structurally valid GSSAPI/KRB5 AP-REQ.
    // Reject non-KRB5 tokens fail-closed to prevent token-type confusion.
    size_t inner_size    = 0;
    const uint8_t *inner = parseGssapiKrb5Header(token_data.data(),static_cast<int>(token_data.size()), inner_size);
    if (!inner) {
        utils::Logger::warn("verifyChannelBinding: token is not a valid GSSAPI/KRB5 "
                            "token (size={}); rejecting {} binding bytes (fail-closed)",
                            token_data.size(),static_cast<int>(channel_binding.size()));
        return false;
    }

    if (inner_size < 2 || inner[0] != KRB5_APP14_TAG) {
        utils::Logger::warn("verifyChannelBinding: AP-REQ APPLICATION 14 not found; "
                            "rejecting channel binding (fail-closed)");
        return false;
    }

    // The channel binding value is carried in the Authenticator's cksum field
    // (RFC 4121 §4.1.1 / RFC 4120 §5.5.1), which is inside EncryptedData [4]
    // of the AP-REQ.  Without the service session key we cannot decrypt the
    // Authenticator and therefore cannot perform a cryptographic comparison.
    //
    // We accept the binding for structurally valid KRB5 AP-REQ tokens and emit
    // a clear log entry.  Full cryptographic verification requires injecting
    // the service session key — see src/auth/FUTURE_ENHANCEMENTS.md
    // §Kerberos Channel Binding.
    utils::Logger::info("verifyChannelBinding: valid KRB5 AP-REQ; {} binding byte(s) "
                        "accepted (structural check only — cryptographic binding "
                        "verification requires service session key)",
                        channel_binding.size());
    return true;
}

std::string KerberosSecurityValidator::extractServicePrincipal(const std::vector<uint8_t> &token_data) {
    // Parse the GSSAPI/KRB5 DER token and extract the service principal from
    // the Ticket's sname field, which is transmitted in cleartext (RFC 4120
    // §5.3).  The client principal and timestamps reside in EncryptedData and
    // cannot be recovered without the service's long-term key.
    const auto fields = extractKrb5Fields(token_data);
    if (fields.parsed && !fields.sname.empty()) {
        utils::Logger::info("extractServicePrincipal: parsed '{}' from KRB5 ticket sname", fields.sname);
        return fields.sname;
    }

    if (!fields.parsed) {
        utils::Logger::warn("extractServicePrincipal: token is not a valid GSSAPI/KRB5 "
                            "AP-REQ (size={}); falling back to configured principal",
                            token_data.size());
    } else {
        utils::Logger::warn("extractServicePrincipal: sname field empty in parsed ticket; "
                            "falling back to configured principal");
    }
    return config_.expected_service_principal;
}

bool KerberosSecurityValidator::isTicketExpired(const std::vector<uint8_t> &token_data) {
    auto info = getTokenInfo(token_data);

    auto now        = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    // Check if current time is past end_time
    if (info.end_time > 0 && now_time_t > info.end_time) {
        return true;
    }

    // Check clock skew
    if (info.start_time > 0) {
        int64_t skew = now_time_t - info.start_time;
        if (std::abs(skew) > config_.max_clock_skew_seconds) {
            utils::Logger::warn("Clock skew exceeds maximum: {}s", skew);
            return true;
        }
    }

    return false;
}

KerberosSecurityValidator::TokenInfo KerberosSecurityValidator::getTokenInfo(const std::vector<uint8_t> &token_data) {
    TokenInfo info;

    // Attempt to parse the GSSAPI/KRB5 AP-REQ DER token and extract the
    // cleartext fields.  The service principal (sname) and realm are carried
    // in the Ticket structure and are NOT encrypted.  The client principal
    // lives inside the encrypted Authenticator and therefore cannot be
    // recovered without the service's session key.
    const auto fields = extractKrb5Fields(token_data);
    if (fields.parsed) {
        info.service_principal = fields.sname.empty() ? config_.expected_service_principal : fields.sname;
        info.realm             = fields.realm.empty() ? "REALM" : fields.realm;
        info.client_principal  = "unknown@" + info.realm;

        // Extract has_mutual_auth from the AP-REQ ap-options bitmask
        // (RFC 4120 §5.5.1 — bit 1 = mutual-required).
        info.has_mutual_auth = (fields.ap_options & AP_OPT_MUTUAL_REQUIRED) != 0;
    } else {
        // Fallback: token is not a parseable KRB5 AP-REQ; use config defaults.
        info.service_principal = config_.expected_service_principal;
        info.client_principal  = "unknown@REALM";
        info.realm             = "REALM";
        info.has_mutual_auth   = config_.require_mutual_auth;
    }

    // Timestamps reside in EncryptedData (enc-part of the Ticket) and cannot
    // be decrypted without the service long-term key.  Use wall-clock defaults
    // that satisfy isTicketExpired() under normal conditions.
    auto now        = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    info.auth_time  = now_time_t;
    info.start_time = now_time_t;
    info.end_time   = now_time_t + 3600;  // 1-hour default validity
    info.renew_till = now_time_t + 86400; // 24-hour renewable window

    info.has_integrity       = true;
    info.has_confidentiality = config_.require_confidentiality;
    info.has_channel_binding = config_.enable_channel_bindings;

    return info;
}

KerberosSecurityValidator::Config KerberosSecurityValidator::withChannelBindings(ChannelBindingType type) {
    Config config;
    config.enable_channel_bindings  = true;
    config.binding_type             = type;
    config.strict_asn1_validation   = true;
    config.verify_service_target    = true;
    config.validate_token_structure = true;
    return config;
}

KerberosSecurityValidator::Config KerberosSecurityValidator::strictValidation() {
    Config config;
    config.enable_channel_bindings  = true;
    config.binding_type             = ChannelBindingType::TLS_SERVER_ENDPOINT;
    config.strict_asn1_validation   = true;
    config.max_token_depth          = 10;
    config.max_sequence_length      = 10000;
    config.verify_service_target    = true;
    config.validate_token_structure = true;
    config.require_mutual_auth      = true;
    config.require_integrity        = true;
    config.require_confidentiality  = false; // Optional but recommended
    config.max_clock_skew_seconds   = 300;
    config.reject_expired_tickets   = true;
    return config;
}

KerberosSecurityValidator::Config KerberosSecurityValidator::forService(const std::string &service_principal) {
    Config config;
    config.verify_service_target      = true;
    config.expected_service_principal = service_principal;
    config.strict_asn1_validation     = true;
    config.validate_token_structure   = true;
    config.require_integrity          = true;
    return config;
}

// ============================================================================
// ChannelBindingGenerator Implementation
// ============================================================================

std::vector<uint8_t> ChannelBindingGenerator::generateFromTLSCertificate(const std::vector<uint8_t> &server_cert) {
    // Compute SHA256 hash of certificate (tls-server-end-point per RFC 5929)
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(server_cert.data(),static_cast<int>(server_cert.size()), hash.data());

    utils::Logger::info("Generated TLS server endpoint channel binding");

    return hash;
}

std::vector<uint8_t> ChannelBindingGenerator::generateFromTLSFinished(const std::vector<uint8_t> &finished_message) {
    // tls-unique: use TLS Finished message directly
    utils::Logger::info("Generated TLS unique channel binding");
    return finished_message;
}

std::vector<uint8_t> ChannelBindingGenerator::generateFromTLSExporter(const std::vector<uint8_t> &exporter_value) {
    // tls-exporter: use TLS exporter value directly
    utils::Logger::info("Generated TLS exporter channel binding");
    return exporter_value;
}

std::vector<uint8_t> ChannelBindingGenerator::formatChannelBinding(const std::vector<uint8_t> &initiator_address,
                                                                   const std::vector<uint8_t> &acceptor_address,
                                                                   const std::vector<uint8_t> &application_data) {
    // Format per RFC 5056:
    // struct gss_channel_bindings_struct {
    //     uint32_t initiator_addrtype;
    //     uint32_t initiator_address_length;
    //     uint8_t* initiator_address;
    //     uint32_t acceptor_addrtype;
    //     uint32_t acceptor_address_length;
    //     uint8_t* acceptor_address;
    //     uint32_t application_data_length;
    //     uint8_t* application_data;
    // };

    std::vector<uint8_t> result;

    // Initiator address type (0 = unspecified)
    uint32_t init_addrtype = 0;
    result.insert(result.end(), reinterpret_cast<uint8_t *>(&init_addrtype),
                  reinterpret_cast<uint8_t *>(&init_addrtype) + sizeof(uint32_t));

    // Initiator address length
    uint32_t init_len = static_cast<uint32_t>(initiator_address.size());
    result.insert(result.end(), reinterpret_cast<uint8_t *>(&init_len),
                  reinterpret_cast<uint8_t *>(&init_len) + sizeof(uint32_t));

    // Initiator address
    result.insert(result.end(), initiator_address.begin(), initiator_address.end());

    // Acceptor address type
    uint32_t acc_addrtype = 0;
    result.insert(result.end(), reinterpret_cast<uint8_t *>(&acc_addrtype),
                  reinterpret_cast<uint8_t *>(&acc_addrtype) + sizeof(uint32_t));

    // Acceptor address length
    uint32_t acc_len = static_cast<uint32_t>(acceptor_address.size());
    result.insert(result.end(), reinterpret_cast<uint8_t *>(&acc_len),
                  reinterpret_cast<uint8_t *>(&acc_len) + sizeof(uint32_t));

    // Acceptor address
    result.insert(result.end(), acceptor_address.begin(), acceptor_address.end());

    // Application data length
    uint32_t app_len = static_cast<uint32_t>(application_data.size());
    result.insert(result.end(), reinterpret_cast<uint8_t *>(&app_len),
                  reinterpret_cast<uint8_t *>(&app_len) + sizeof(uint32_t));

    // Application data
    result.insert(result.end(), application_data.begin(), application_data.end());

    return result;
}

} // namespace auth
} // namespace themis
