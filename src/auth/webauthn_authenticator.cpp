/**
 * @file webauthn_authenticator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/webauthn_authenticator.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <nlohmann/json.hpp>
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

#include "utils/audit_logger.h"

namespace themis {
namespace auth {

// ============================================================================
// Anonymous namespace: minimal CBOR decoder + helpers
// ============================================================================

namespace {

// ---------------------------------------------------------------------------
// Minimal CBOR decoder
//
// Covers the subset used in WebAuthn:
//   unsigned integers, negative integers, byte strings, text strings, maps,
//   and tag items (tag is consumed and the wrapped item is decoded).
// ---------------------------------------------------------------------------

/// Read CBOR argument value (length or integer payload) and advance pos.
static size_t cborReadArg(const std::vector<uint8_t> &d, size_t pos, uint64_t &out) {
    if (pos >= d.size()) {
        throw std::runtime_error("CBOR: truncated data");
    }
    const uint8_t info = d[pos] & 0x1F;
    ++pos;
    if (info <= 23) {
        out = info;
        return pos;
    }
    if (info == 24) {
        if (pos >= d.size()) {
            throw std::runtime_error("CBOR: truncated 1-byte arg");
        }
        out = d[pos++];
        return pos;
    }
    if (info == 25) {
        if (pos + 1 >= d.size()) {
            throw std::runtime_error("CBOR: truncated 2-byte arg");
        }
        out = (static_cast<uint64_t>(d[pos]) << 8) | d[pos + 1];
        pos += 2;
        return pos;
    }
    if (info == 26) {
        if (pos + 3 >= d.size()) {
            throw std::runtime_error("CBOR: truncated 4-byte arg");
        }
        out = (static_cast<uint64_t>(d[pos]) << 24) | (static_cast<uint64_t>(d[pos + 1]) << 16)
              | (static_cast<uint64_t>(d[pos + 2]) << 8) | static_cast<uint64_t>(d[pos + 3]);
        pos += 4;
        return pos;
    }
    if (info == 27) {
        if (pos + 7 >= d.size()) {
            throw std::runtime_error("CBOR: truncated 8-byte arg");
        }
        out = 0;
        for (int i = 0; i < 8; ++i) {
            out = (out << 8) | d[pos + i];
        }
        pos += 8;
        return pos;
    }
    throw std::runtime_error("CBOR: unsupported additional info " + std::to_string(static_cast<int>(info)));
}

/// Skip one CBOR item, returning the new position.
static size_t cborSkip(const std::vector<uint8_t> &d, size_t pos) {
    if (pos >= d.size()) {
        throw std::runtime_error("CBOR: truncated (skip)");
    }
    const uint8_t initial = d[pos];
    const uint8_t major   = initial >> 5;
    uint64_t arg          = 0;

    switch (major) {
        case 0:
        [[fallthrough]];\n        case 1: // unsigned / negative integer
            return cborReadArg(d, pos, arg);

        case 2:
        [[fallthrough]];\n        case 3: // byte / text string
            pos = cborReadArg(d, pos, arg);
            if (pos + arg > d.size()) {
                throw std::runtime_error("CBOR: string out of bounds (skip)");
            }
            return pos + static_cast<size_t>(arg);

        case 4: // array
            pos = cborReadArg(d, pos, arg);
            for (uint64_t i = 0; i < arg; ++i) {
                pos = cborSkip(d, pos);
            }
            return pos;

        case 5: // map
            pos = cborReadArg(d, pos, arg);
            for (uint64_t i = 0; i < arg; ++i) {
                pos = cborSkip(d, pos);
                pos = cborSkip(d, pos);
            }
            return pos;

        case 6: // semantic tag: skip the tag number, then skip the tagged item
            pos = cborReadArg(d, pos, arg);
            return cborSkip(d, pos);

        default:
        [[fallthrough]];\n        case 7: { // float / simple
            const uint8_t info = initial & 0x1F;
            ++pos;
            if (info == 24) {
                return pos + 1;
            }
            if (info == 25) {
                return pos + 2;
            }
            if (info == 26) {
                return pos + 4;
            }
            if (info == 27) {
                return pos + 8;
            }
            return pos;
        }
    }
}

// ---------------------------------------------------------------------------
// Parse attestation object CBOR map (string keys: "fmt", "attStmt", "authData")
// ---------------------------------------------------------------------------

static void cborParseAttestationObject(const std::vector<uint8_t> &d, std::string &fmt,
                                       std::vector<uint8_t> &auth_data) {
    size_t pos = 0;
    if (pos >= d.size() || (d[pos] >> 5) != 5) {
        throw std::runtime_error("CBOR: expected map for attestation object");
    }

    uint64_t count;
    pos = cborReadArg(d, pos, count);

    for (uint64_t i = 0; i < count; ++i) {
        // Key must be a text string
        if (pos >= d.size() || (d[pos] >> 5) != 3) {
            pos = cborSkip(d, pos);
            pos = cborSkip(d, pos);
            continue;
        }
        uint64_t klen;
        pos = cborReadArg(d, pos, klen);
        if (pos + klen > d.size()) {
            throw std::runtime_error("CBOR: key text out of bounds");
        }
        const std::string key(d.begin() + pos, d.begin() + pos + klen);
        pos += klen;

        if (key == "fmt") {
            if (pos >= d.size() || (d[pos] >> 5) != 3) {
                throw std::runtime_error("CBOR: fmt value must be text");
            }
            uint64_t vlen;
            pos = cborReadArg(d, pos, vlen);
            if (pos + vlen > d.size()) {
                throw std::runtime_error("CBOR: fmt text out of bounds");
            }
            fmt.assign(d.begin() + pos, d.begin() + pos + vlen);
            pos += vlen;

        } else if (key == "authData") {
            if (pos >= d.size() || (d[pos] >> 5) != 2) {
                throw std::runtime_error("CBOR: authData must be a byte string");
            }
            uint64_t vlen;
            pos = cborReadArg(d, pos, vlen);
            if (pos + vlen > d.size()) {
                throw std::runtime_error("CBOR: authData out of bounds");
            }
            auth_data.assign(d.begin() + pos, d.begin() + pos + vlen);
            pos += vlen;

        } else {
            pos = cborSkip(d, pos);
        }
    }

    if (auth_data.empty()) {
        throw std::runtime_error("CBOR: attestation object missing authData");
    }
}

// ---------------------------------------------------------------------------
// COSE key fields parsed from a CBOR integer-keyed map
// ---------------------------------------------------------------------------

struct CoseKeyFields {
    int64_t kty{0}; ///< key 1  (2=EC2, 3=RSA)
    int64_t alg{0}; ///< key 3  (-7=ES256, -257=RS256)
    int64_t crv{0}; ///< key -1 (integer, EC2: 1=P-256); 0 if not present as integer

    // Byte-string values
    std::vector<uint8_t> neg1_bytes; ///< key -1 bytes (RSA modulus n)
    std::vector<uint8_t> neg2_bytes; ///< key -2 bytes (EC x coord or RSA exponent e)
    std::vector<uint8_t> neg3_bytes; ///< key -3 bytes (EC y coord)
};

static void cborParseCoseKey(const std::vector<uint8_t> &d, size_t pos, CoseKeyFields &out) {
    if (pos >= d.size() || (d[pos] >> 5) != 5) {
        throw std::runtime_error("CBOR: expected map for COSE key");
    }

    uint64_t count;
    pos = cborReadArg(d, pos, count);

    for (uint64_t i = 0; i < count; ++i) {
        if (pos >= d.size()) {
            throw std::runtime_error("CBOR: truncated COSE key map");
        }

        // --- read integer key ---
        const uint8_t k_initial = d[pos];
        const uint8_t k_major   = k_initial >> 5;
        int64_t k{0};
        if (k_major == 0) { // positive integer key
            uint64_t v;
            pos = cborReadArg(d, pos, v);
            k   = static_cast<int64_t>(v);
        } else if (k_major == 1) { // negative integer key
            uint64_t v;
            pos = cborReadArg(d, pos, v);
            k   = -1 - static_cast<int64_t>(v);
        } else {
            pos = cborSkip(d, pos);
            pos = cborSkip(d, pos);
            continue;
        }

        // --- read value ---
        if (pos >= d.size()) {
            throw std::runtime_error("CBOR: truncated COSE key value");
        }
        const uint8_t v_major = d[pos] >> 5;

        auto readInt = [&]() -> int64_t {
            uint64_t v;
            pos = cborReadArg(d, pos, v);
            return static_cast<int64_t>(v);
        };
        auto readNegInt = [&]() -> int64_t {
            uint64_t v;
            pos = cborReadArg(d, pos, v);
            return -1 - static_cast<int64_t>(v);
        };
        auto readBytes = [&]() -> std::vector<uint8_t> {
            uint64_t vlen;
            pos = cborReadArg(d, pos, vlen);
            if (pos + vlen > d.size()) {
                throw std::runtime_error("CBOR: byte value out of bounds");
            }
            std::vector<uint8_t> b(d.begin() + pos, d.begin() + pos + vlen);
            pos += vlen;
            return b;
        };

        if (k == 1) { // kty
            if (v_major == 0) {
                out.kty = readInt();
            } else if (v_major == 1) {
                out.kty = readNegInt();
            } else {
                pos = cborSkip(d, pos);
            }

        } else if (k == 3) { // alg
            if (v_major == 0) {
                out.alg = readInt();
            } else if (v_major == 1) {
                out.alg = readNegInt();
            } else {
                pos = cborSkip(d, pos);
            }

        } else if (k == -1) { // crv (EC integer) or n (RSA bytes)
            if (v_major == 2) {
                out.neg1_bytes = readBytes();
            } else if (v_major == 0) {
                out.crv = readInt();
            } else if (v_major == 1) {
                out.crv = readNegInt();
            } else {
                pos = cborSkip(d, pos);
            }

        } else if (k == -2) { // x (EC) or e (RSA)
            if (v_major == 2) {
                out.neg2_bytes = readBytes();
            } else {
                pos = cborSkip(d, pos);
            }

        } else if (k == -3) { // y (EC)
            if (v_major == 2) {
                out.neg3_bytes = readBytes();
            } else {
                pos = cborSkip(d, pos);
            }

        } else {
            pos = cborSkip(d, pos);
        }
    }
}

// ---------------------------------------------------------------------------
// Base64URL helpers (RFC 4648 §5, no padding)
// ---------------------------------------------------------------------------

static const char kB64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64UrlEncodeImpl(const uint8_t *data, std::size_t len) {
    std::string out;
    out.reserve(((len + 2) / 3) * 4);

    for (std::size_t i = 0; i < len; i += 3) {
        const uint32_t b0 = data[i];
        const uint32_t b1 = (i + 1 < len) ? data[i + 1] : 0u;
        const uint32_t b2 = (i + 2 < len) ? data[i + 2] : 0u;
        const uint32_t t  = (b0 << 16) | (b1 << 8) | b2;
        out += kB64Table[(t >> 18) & 0x3F];
        out += kB64Table[(t >> 12) & 0x3F];
        out += (i + 1 < len) ? kB64Table[(t >> 6) & 0x3F] : '=';
        out += (i + 2 < len) ? kB64Table[(t) & 0x3F] : '=';
    }
    // Standard → URL-safe: + → -, / → _; strip padding
    for (char &c : out) {
        if (c == '+') {
            c = '-';
        } else if (c == '/') {
            c = '_';
        }
    }
    while (!out.empty() && out.back() == '=') {
        out.pop_back();
    }
    return out;
}

static std::vector<uint8_t> base64UrlDecodeImpl(const std::string &input) {
    std::string padded = input;
    // URL-safe → standard
    for (char &c : padded) {
        if (c == '-') {
            c = '+';
        } else if (c == '_') {
            c = '/';
        }
    }
    // Restore padding
    while (padded.size() % 4 != 0) {
        padded += '=';
    }

    // Build decode table
    static const int8_t kDec[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54, 55,
        56, 57, 58, 59, 60, 61, -1, -1, -1, 0,  -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
        13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32,
        33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    };

    std::vector<uint8_t> out;
    out.reserve(padded.size() / 4 * 3);

    for (std::size_t i = 0; i < padded.size(); i += 4) {
        const int8_t a = kDec[static_cast<uint8_t>(padded[i])];
        const int8_t b = kDec[static_cast<uint8_t>(padded[i + 1])];
        const int8_t c = kDec[static_cast<uint8_t>(padded[i + 2])];
        const int8_t d = kDec[static_cast<uint8_t>(padded[i + 3])];
        if (a < 0 || b < 0) {
            throw std::runtime_error("Base64URL: invalid character");
        }
        out.push_back(static_cast<uint8_t>((a << 2) | (b >> 4)));
        if (padded[i + 2] != '=') {
            out.push_back(static_cast<uint8_t>((b << 4) | (c >> 2)));
        }
        if (padded[i + 3] != '=') {
            out.push_back(static_cast<uint8_t>((c << 6) | d));
        }
    }
    return out;
}

} // anonymous namespace

// ============================================================================
// CredentialCreationOptions::to_json
// ============================================================================

nlohmann::json WebAuthnAuthenticator::CredentialCreationOptions::to_json() const {
    nlohmann::json j;
    j["challenge"] = challenge;
    j["rp"]        = {{"id", rp.id}, {"name", rp.name}};
    j["user"]      = {{"id", user.id}, {"name", user.name}, {"displayName", user.display_name}};

    nlohmann::json params = nlohmann::json::array();
    for (const auto &alg : pub_key_cred_params) {
        int alg_id = (alg == "ES256") ? -7 : (alg == "RS256") ? -257 : 0;
        params.push_back({{"type", "public-key"}, {"alg", alg_id}});
    }
    j["pubKeyCredParams"] = params;

    if (timeout_ms) {
        j["timeout"] = *timeout_ms;
    }
    j["attestation"] = attestation;

    nlohmann::json sel;
    if (authenticator_selection.authenticator_attachment) {
        sel["authenticatorAttachment"] = *authenticator_selection.authenticator_attachment;
    }
    sel["requireResidentKey"]   = authenticator_selection.require_resident_key;
    sel["userVerification"]     = authenticator_selection.user_verification;
    j["authenticatorSelection"] = sel;

    if (!exclude_credentials.empty()) {
        nlohmann::json excl = nlohmann::json::array();
        for (const auto &cid : exclude_credentials) {
            excl.push_back({{"type", "public-key"}, {"id", cid}});
        }
        j["excludeCredentials"] = excl;
    }
    return j;
}

// ============================================================================
// CredentialRequestOptions::to_json
// ============================================================================

nlohmann::json WebAuthnAuthenticator::CredentialRequestOptions::to_json() const {
    nlohmann::json j;
    j["challenge"]        = challenge;
    j["rpId"]             = rp_id;
    j["userVerification"] = user_verification;
    if (timeout_ms) {
        j["timeout"] = *timeout_ms;
    }

    if (!allow_credentials.empty()) {
        nlohmann::json allow = nlohmann::json::array();
        for (const auto &cid : allow_credentials) {
            allow.push_back({{"type", "public-key"}, {"id", cid}});
        }
        j["allowCredentials"] = allow;
    }
    return j;
}

// ============================================================================
// Constructor
// ============================================================================

WebAuthnAuthenticator::WebAuthnAuthenticator(const RelyingParty &rp) : rp_(rp), expected_origin_("https://" + rp.id) {
    if (rp.id.empty()) {
        throw AuthException(AuthError(AuthErrorCode::AUTH_CONFIG_INVALID, "WebAuthn configuration error",
                                      "RelyingParty.id must not be empty"));
    }
    spdlog::info("WebAuthnAuthenticator: initialized for RP '{}' (origin: {})", rp_.id, expected_origin_);
}

// ============================================================================
// Testing helpers
// ============================================================================

void WebAuthnAuthenticator::setRandBytesForTesting(std::function<void(unsigned char *, std::size_t)> fn) {
    rand_bytes_fn_ = std::move(fn);
}

void WebAuthnAuthenticator::setExpectedOrigin(const std::string &origin) {
    expected_origin_ = origin;
}

// ============================================================================
// Registration ceremony
// ============================================================================

WebAuthnAuthenticator::CredentialCreationOptions WebAuthnAuthenticator::startRegistration(const User &user,
                                                                                          bool resident_key) {
    const std::string challenge = generateChallenge();

    CredentialCreationOptions opts;
    opts.challenge                                    = challenge;
    opts.rp                                           = rp_;
    opts.user                                         = user;
    opts.pub_key_cred_params                          = {"ES256", "RS256"};
    opts.timeout_ms                                   = 60000; // 60 s
    opts.attestation                                  = "none";
    opts.authenticator_selection.require_resident_key = resident_key;
    opts.authenticator_selection.user_verification    = "preferred";
    if (resident_key) {
        opts.authenticator_selection.authenticator_attachment = "platform";
        opts.authenticator_selection.user_verification        = "required";
    }

    spdlog::info("WebAuthn: startRegistration for user '{}'", user.name);
    return opts;
}

WebAuthnAuthenticator::AttestationResult WebAuthnAuthenticator::completeRegistration(const nlohmann::json &cred) {
    // --- 1. Extract top-level fields ---
    const std::string type = cred.value("type", "");
    if (type != "public-key") {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn registration failed",
                         "credential type must be 'public-key', got '" + type + "'");
    }

    const nlohmann::json &response    = cred.at("response");
    const std::string client_data_b64 = response.at("clientDataJSON").get<std::string>();
    const std::string attest_obj_b64  = response.at("attestationObject").get<std::string>();

    // --- 2. Decode and verify clientDataJSON ---
    const std::vector<uint8_t> client_data_bytes = base64UrlDecode(client_data_b64);
    const ClientData cd                          = parseClientDataJSON(client_data_bytes);

    if (cd.type != "webauthn.create") {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn registration failed",
                         "clientData.type must be 'webauthn.create', got '" + cd.type + "'");
    }
    verifyAndConsumeChallenge(cd.challenge);

    if (cd.origin != expected_origin_) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn registration failed",
                         "origin mismatch: expected '" + expected_origin_ + "', got '" + cd.origin + "'");
    }

    // --- 3. Decode and parse attestation object ---
    std::string fmt;
    std::vector<uint8_t> auth_data_bytes;
    try {
        const std::vector<uint8_t> attest_obj_bytes = base64UrlDecode(attest_obj_b64);
        cborParseAttestationObject(attest_obj_bytes, fmt, auth_data_bytes);
    } catch (const std::exception &ex) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn registration failed",
                         std::string("attestation object parse error: ") + ex.what());
    }

    // --- 4. Parse authenticator data ---
    AuthData ad;
    try {
        ad = parseAuthData(auth_data_bytes);
    } catch (const std::exception &ex) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn registration failed",
                         std::string("authData parse error: ") + ex.what());
    }

    // --- 5. Verify RP ID hash ---
    const std::vector<uint8_t> expected_hash = sha256(rp_.id);
    if (!std::equal(ad.rp_id_hash.begin(), ad.rp_id_hash.end(), expected_hash.begin())) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn registration failed", "rpIdHash mismatch");
    }

    // --- 6. Verify User Presence flag (bit 0) ---
    constexpr uint8_t kFlagUP = 0x01;
    if (!(ad.flags & kFlagUP)) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INVALID_CREDENTIALS, "WebAuthn registration failed",
                         "User Presence (UP) flag is not set");
    }

    // --- 7. Attested credential data must be present ---
    if (!ad.has_attested_credential) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn registration failed",
                         "AT flag not set in authData; no attested credential");
    }

    // --- 8. Parse COSE key → DER SPKI ---
    std::vector<uint8_t> spki;
    std::string algorithm;
    try {
        auto [s, a] = coseKeyToSpki(ad.cose_key_bytes);
        spki        = std::move(s);
        algorithm   = std::move(a);
    } catch (const AuthException &) {
        throw;
    } catch (const std::exception &ex) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn registration failed",
                         std::string("COSE key parse error: ") + ex.what());
    }

    spdlog::info("WebAuthn: registration complete (alg={}, fmt={})", algorithm, fmt);
    if (audit_logger_) {
        audit_logger_->logSecurityEvent(utils::SecurityEventType::MFA_ENROLLED, "", "webauthn/register", {});
    }

    AttestationResult result;
    result.credential_id = ad.credential_id;
    result.public_key    = std::move(spki);
    result.algorithm     = algorithm;
    result.sign_count    = ad.sign_count;
    result.aaguid        = std::move(ad.aaguid);
    return result;
}

// ============================================================================
// Authentication ceremony
// ============================================================================

WebAuthnAuthenticator::CredentialRequestOptions
WebAuthnAuthenticator::startAuthentication(const std::optional<std::string> & /*user_id*/) {
    const std::string challenge = generateChallenge();

    CredentialRequestOptions opts;
    opts.challenge         = challenge;
    opts.rp_id             = rp_.id;
    opts.timeout_ms        = 60000;
    opts.user_verification = "preferred";

    spdlog::info("WebAuthn: startAuthentication (rpId='{}')", rp_.id);
    return opts;
}

WebAuthnAuthenticator::AssertionResult
WebAuthnAuthenticator::completeAuthentication(const nlohmann::json &cred, const std::vector<uint8_t> &stored_public_key,
                                              uint32_t stored_sign_count) {
    // --- 1. Extract fields ---
    const std::string type = cred.value("type", "");
    if (type != "public-key") {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn authentication failed",
                         "credential type must be 'public-key'");
    }

    const std::string credential_id_b64 = cred.value("id", "");

    const nlohmann::json &response    = cred.at("response");
    const std::string client_data_b64 = response.at("clientDataJSON").get<std::string>();
    const std::string auth_data_b64   = response.at("authenticatorData").get<std::string>();
    const std::string signature_b64   = response.at("signature").get<std::string>();
    const std::string user_handle_b64 = response.value("userHandle", "");

    // --- 2. Decode and verify clientDataJSON ---
    const std::vector<uint8_t> client_data_bytes = base64UrlDecode(client_data_b64);
    const ClientData cd                          = parseClientDataJSON(client_data_bytes);

    if (cd.type != "webauthn.get") {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn authentication failed",
                         "clientData.type must be 'webauthn.get', got '" + cd.type + "'");
    }
    verifyAndConsumeChallenge(cd.challenge);

    if (cd.origin != expected_origin_) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn authentication failed",
                         "origin mismatch: expected '" + expected_origin_ + "', got '" + cd.origin + "'");
    }

    // --- 3. Parse authenticator data ---
    const std::vector<uint8_t> auth_data_bytes = base64UrlDecode(auth_data_b64);
    const std::vector<uint8_t> signature_bytes = base64UrlDecode(signature_b64);

    AuthData ad;
    try {
        ad = parseAuthData(auth_data_bytes);
    } catch (const std::exception &ex) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn authentication failed",
                         std::string("authData parse error: ") + ex.what());
    }

    // --- 4. Verify RP ID hash ---
    const std::vector<uint8_t> expected_hash = sha256(rp_.id);
    if (!std::equal(ad.rp_id_hash.begin(), ad.rp_id_hash.end(), expected_hash.begin())) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn authentication failed", "rpIdHash mismatch");
    }

    // --- 5. Verify User Presence flag ---
    constexpr uint8_t kFlagUP = 0x01;
    if (!(ad.flags & kFlagUP)) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INVALID_CREDENTIALS, "WebAuthn authentication failed",
                         "User Presence (UP) flag is not set");
    }

    // --- 6. Verify signature counter (detect cloned authenticators) ---
    // Per spec §7.2 step 17: if stored_sign_count > 0 or new counter > 0,
    // the new counter MUST be greater than the stored counter.
    if (stored_sign_count > 0 || ad.sign_count > 0) {
        if (ad.sign_count <= stored_sign_count) {
            THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn authentication failed",
                             "signature counter rollback detected (possible cloned authenticator)");
        }
    }

    // --- 7. Verify signature ---
    const std::vector<uint8_t> client_data_hash = sha256(client_data_bytes);
    try {
        verifySignature(auth_data_bytes, client_data_hash, signature_bytes, stored_public_key);
    } catch (const AuthException &) {
        if (audit_logger_) {
            audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_FAILED, "", "webauthn/auth", {});
        }
        throw;
    }

    spdlog::info("WebAuthn: authentication succeeded (credId={})", credential_id_b64);
    if (audit_logger_) {
        audit_logger_->logSecurityEvent(utils::SecurityEventType::LOGIN_SUCCESS, "", "webauthn/auth", {});
    }

    AssertionResult result;
    result.credential_id = credential_id_b64;
    result.sign_count    = ad.sign_count;
    if (!user_handle_b64.empty()) {
        const auto uh      = base64UrlDecode(user_handle_b64);
        result.user_handle = std::string(uh.begin(), uh.end());
    }
    return result;
}

// ============================================================================
// Private helpers – challenge management
// ============================================================================

std::string WebAuthnAuthenticator::generateChallenge() {
    std::array<unsigned char, 32> raw{};
    fillRandomBytes(raw.data(), raw.size());

    const std::string b64 = base64UrlEncode(std::vector<uint8_t>(raw.begin(), raw.end()));

    {
        std::lock_guard<std::mutex> lock(challenges_mutex_);
        purgeExpiredChallenges();
        pending_challenges_[b64].expires_at = std::chrono::system_clock::now() + kChallengeTTL;
    }
    return b64;
}

void WebAuthnAuthenticator::verifyAndConsumeChallenge(const std::string &challenge_b64) {
    std::lock_guard<std::mutex> lock(challenges_mutex_);
    purgeExpiredChallenges();

    auto it = pending_challenges_.find(challenge_b64);
    if (it == pending_challenges_.end()) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_TOKEN_INVALID, "WebAuthn challenge invalid",
                         "challenge not found or already consumed");
    }
    pending_challenges_.erase(it);
}

void WebAuthnAuthenticator::purgeExpiredChallenges() {
    // Caller must hold challenges_mutex_
    const auto now = std::chrono::system_clock::now();
    for (auto it = pending_challenges_.begin(); it != pending_challenges_.end();) {
        if (it->second.expires_at <= now) {
            it = pending_challenges_.erase(it);
        } else {
            ++it;
        }
    }
}

void WebAuthnAuthenticator::fillRandomBytes(unsigned char *buf, std::size_t len) {
    if (rand_bytes_fn_) {
        rand_bytes_fn_(buf, len);
        return;
    }
    if (RAND_bytes(buf, static_cast<int>(len)) != 1) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn internal error", "RAND_bytes failed");
    }
}

// ============================================================================
// Private helpers – cryptographic primitives
// ============================================================================

std::vector<uint8_t> WebAuthnAuthenticator::sha256(const std::vector<uint8_t> &data) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    SHA256(data.data(), data.size(), digest.data());
    return std::vector<uint8_t>(digest.begin(), digest.end());
}

std::vector<uint8_t> WebAuthnAuthenticator::sha256(const std::string &data) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    SHA256(reinterpret_cast<const unsigned char *>(data.data()), data.size(), digest.data());
    return std::vector<uint8_t>(digest.begin(), digest.end());
}

// ============================================================================
// Private helpers – base64url codec
// ============================================================================

std::string WebAuthnAuthenticator::base64UrlEncode(const std::vector<uint8_t> &data) {
    return base64UrlEncodeImpl(data.data(), data.size());
}

std::vector<uint8_t> WebAuthnAuthenticator::base64UrlDecode(const std::string &input) {
    try {
        return base64UrlDecodeImpl(input);
    } catch (const std::exception &ex) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn base64url decode error", ex.what());
    }
}

// ============================================================================
// Private helpers – clientDataJSON
// ============================================================================

WebAuthnAuthenticator::ClientData
WebAuthnAuthenticator::parseClientDataJSON(const std::vector<uint8_t> &client_data_json) {
    const std::string json_str(client_data_json.begin(), client_data_json.end());
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json_str);
    } catch (const nlohmann::json::exception &ex) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn clientDataJSON parse error", ex.what());
    }

    ClientData cd;
    cd.type      = j.value("type", "");
    cd.challenge = j.value("challenge", "");
    cd.origin    = j.value("origin", "");
    return cd;
}

// ============================================================================
// Private helpers – authenticatorData
// ============================================================================

WebAuthnAuthenticator::AuthData WebAuthnAuthenticator::parseAuthData(const std::vector<uint8_t> &auth_data_bytes) {
    // Minimum: 37 bytes (rpIdHash[32] + flags[1] + signCount[4])
    if (auth_data_bytes.size() < 37) {
        throw std::runtime_error("authData too short (" + std::to_string(auth_data_bytes.size()) + " bytes)");
    }

    AuthData ad;
    std::copy(auth_data_bytes.begin(), auth_data_bytes.begin() + 32, ad.rp_id_hash.begin());
    ad.flags      = auth_data_bytes[32];
    ad.sign_count = (static_cast<uint32_t>(auth_data_bytes[33]) << 24)
                    | (static_cast<uint32_t>(auth_data_bytes[34]) << 16)
                    | (static_cast<uint32_t>(auth_data_bytes[35]) << 8) | static_cast<uint32_t>(auth_data_bytes[36]);

    constexpr uint8_t kFlagAT = 0x40;
    if (!(ad.flags & kFlagAT)) {
        return ad; // no attested credential data
    }

    // Attested credential data starts at byte 37
    // Layout: AAGUID[16] + credentialIdLength[2] + credentialId[N] + credentialPublicKey[CBOR]
    if (auth_data_bytes.size() < 37 + 16 + 2) {
        throw std::runtime_error("authData too short for attested credential");
    }

    size_t off = 37;
    ad.aaguid.assign(auth_data_bytes.begin() + off, auth_data_bytes.begin() + off + 16);
    off += 16;

    const uint16_t cred_id_len
        = (static_cast<uint16_t>(auth_data_bytes[off]) << 8) | static_cast<uint16_t>(auth_data_bytes[off + 1]);
    off += 2;

    if (auth_data_bytes.size() < off + cred_id_len) {
        throw std::runtime_error("authData too short for credentialId");
    }

    const std::vector<uint8_t> cred_id_bytes(auth_data_bytes.begin() + off,
                                             auth_data_bytes.begin() + off + cred_id_len);
    ad.credential_id = base64UrlEncodeImpl(cred_id_bytes.data(), cred_id_bytes.size());
    off += cred_id_len;

    // Remainder is the CBOR-encoded COSE public key
    ad.cose_key_bytes.assign(auth_data_bytes.begin() + off, auth_data_bytes.end());
    ad.has_attested_credential = true;
    return ad;
}

// ============================================================================
// Private helpers – parseAttestationObject
// ============================================================================

void WebAuthnAuthenticator::parseAttestationObject(const std::vector<uint8_t> &cbor_bytes, std::string &fmt,
                                                   std::vector<uint8_t> &auth_data) {
    cborParseAttestationObject(cbor_bytes, fmt, auth_data);
}

// ============================================================================
// Private helpers – COSE key → DER SPKI
// ============================================================================

std::pair<std::vector<uint8_t>, std::string>
WebAuthnAuthenticator::coseKeyToSpki(const std::vector<uint8_t> &cose_key_bytes) {
    CoseKeyFields fields;
    try {
        cborParseCoseKey(cose_key_bytes, 0, fields);
    } catch (const std::exception &ex) {
        throw std::runtime_error(std::string("COSE key CBOR error: ") + ex.what());
    }

    // kty == 2 → EC2 (ECDSA); kty == 3 → RSA
    if (fields.kty == 2) {
        // ES256: ECDSA-P256-SHA256
        // crv must be 1 (P-256)
        if (fields.crv != 1) {
            throw AuthException(
                AuthError(AuthErrorCode::AUTH_NOT_IMPLEMENTED, "Unsupported WebAuthn key",
                          "Only EC curve P-256 (crv=1) is supported; got crv=" + std::to_string(fields.crv)));
        }

        if (fields.neg2_bytes.size() != 32 || fields.neg3_bytes.size() != 32) {
            throw std::runtime_error("EC P-256 key: x or y coordinate is not 32 bytes");
        }

        // Build uncompressed EC point: 0x04 || x || y
        std::vector<uint8_t> point;
        point.reserve(65);
        point.push_back(0x04);
        point.insert(point.end(), fields.neg2_bytes.begin(), fields.neg2_bytes.end());
        point.insert(point.end(), fields.neg3_bytes.begin(), fields.neg3_bytes.end());

        // Build EVP_PKEY using modern OpenSSL 3.x fromdata API
        OSSL_PARAM_BLD *bld = OSSL_PARAM_BLD_new();
        if (!bld) {
            throw std::runtime_error("OSSL_PARAM_BLD_new failed");
        }
        OSSL_PARAM_BLD_push_utf8_string(bld, OSSL_PKEY_PARAM_GROUP_NAME, "P-256", 0);
        OSSL_PARAM_BLD_push_octet_string(bld, OSSL_PKEY_PARAM_PUB_KEY, point.data(), point.size());
        OSSL_PARAM *params = OSSL_PARAM_BLD_to_param(bld);
        OSSL_PARAM_BLD_free(bld);
        if (!params) {
            throw std::runtime_error("OSSL_PARAM_BLD_to_param failed");
        }

        EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr);
        EVP_PKEY *pkey     = nullptr;
        const bool ok      = pctx && EVP_PKEY_fromdata_init(pctx) > 0
                             && EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params) > 0;
        OSSL_PARAM_free(params);
        EVP_PKEY_CTX_free(pctx);
        if (!ok || !pkey) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("EVP_PKEY_fromdata failed for EC P-256 key");
        }

        unsigned char *der = nullptr;
        const int der_len  = i2d_PUBKEY(pkey, &der);
        EVP_PKEY_free(pkey);

        if (der_len <= 0 || !der) {
            throw std::runtime_error("i2d_PUBKEY failed for EC key");
        }

        std::vector<uint8_t> spki(der, der + der_len);
        OPENSSL_free(der);
        return {std::move(spki), "ES256"};

    } else if (fields.kty == 3) {
        // RS256: RSA-PKCS1v1.5-SHA256
        if (fields.neg1_bytes.empty() || fields.neg2_bytes.empty())
            throw std::runtime_error("RSA COSE key missing modulus or exponent");

        BIGNUM *n = BN_bin2bn(fields.neg1_bytes.data(), static_cast<int>(fields.neg1_bytes.size()), nullptr);
        BIGNUM *e = BN_bin2bn(fields.neg2_bytes.data(), static_cast<int>(fields.neg2_bytes.size()), nullptr);
        if (!n || !e) {
            BN_free(n);
            BN_free(e);
            throw std::runtime_error("BN_bin2bn failed");
        }

        OSSL_PARAM_BLD *bld = OSSL_PARAM_BLD_new();
        if (!bld) {
            BN_free(n);
            BN_free(e);
            throw std::runtime_error("OSSL_PARAM_BLD_new failed");
        }
        OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_N, n);
        OSSL_PARAM_BLD_push_BN(bld, OSSL_PKEY_PARAM_RSA_E, e);
        OSSL_PARAM *params = OSSL_PARAM_BLD_to_param(bld);
        OSSL_PARAM_BLD_free(bld);
        BN_free(n);
        BN_free(e);
        if (!params)
            throw std::runtime_error("OSSL_PARAM_BLD_to_param failed");

        EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr);
        EVP_PKEY *pkey     = nullptr;
        const bool ok      = pctx && EVP_PKEY_fromdata_init(pctx) > 0
                             && EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params) > 0;
        OSSL_PARAM_free(params);
        EVP_PKEY_CTX_free(pctx);
        if (!ok || !pkey) {
            EVP_PKEY_free(pkey);
            throw std::runtime_error("EVP_PKEY_fromdata failed for RSA key");
        }

        unsigned char *der = nullptr;
        const int der_len  = i2d_PUBKEY(pkey, &der);
        EVP_PKEY_free(pkey);

        if (der_len <= 0 || !der)
            throw std::runtime_error("i2d_PUBKEY failed for RSA key");

        std::vector<uint8_t> spki(der, der + der_len);
        OPENSSL_free(der);
        return {std::move(spki), "RS256"};

    } else {
        throw AuthException(
            AuthError(AuthErrorCode::AUTH_NOT_IMPLEMENTED, "Unsupported WebAuthn key type",
                      "kty=" + std::to_string(fields.kty) + " is not supported; use kty=2 (EC) or kty=3 (RSA)"));
    }
}

// ============================================================================
// Private helpers – signature verification
// ============================================================================

void WebAuthnAuthenticator::verifySignature(const std::vector<uint8_t> &auth_data_bytes,
                                            const std::vector<uint8_t> &client_data_hash,
                                            const std::vector<uint8_t> &signature_bytes,
                                            const std::vector<uint8_t> &spki_bytes) {
    // Load public key from DER SPKI
    const unsigned char *p = spki_bytes.data();
    EVP_PKEY *pkey         = d2i_PUBKEY(nullptr, &p, static_cast<long>(spki_bytes.size()));
    if (!pkey) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn signature verification failed",
                         "d2i_PUBKEY failed: cannot load stored public key");
    }

    // Signed data = authData || SHA256(clientDataJSON)
    std::vector<uint8_t> msg;
    msg.reserve(auth_data_bytes.size() + client_data_hash.size());
    msg.insert(msg.end(), auth_data_bytes.begin(), auth_data_bytes.end());
    msg.insert(msg.end(), client_data_hash.begin(), client_data_hash.end());

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        EVP_PKEY_free(pkey);
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INTERNAL_ERROR, "WebAuthn signature verification failed",
                         "EVP_MD_CTX_new failed");
    }

    const bool ok = EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, pkey) == 1
                    && EVP_DigestVerifyUpdate(ctx, msg.data(), msg.size()) == 1
                    && EVP_DigestVerifyFinal(ctx, signature_bytes.data(), signature_bytes.size()) == 1;

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    if (!ok) {
        THROW_AUTH_ERROR(AuthErrorCode::AUTH_INVALID_CREDENTIALS, "WebAuthn authentication failed",
                         "signature verification failed");
    }
}

} // namespace auth
} // namespace themis
