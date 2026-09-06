/**
 * @file saml_auth_provider.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/saml_auth_provider.h"
#include "auth/auth_error.h"
#include "utils/logger.h"

#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

/*static*/ nlohmann::json SamlAuthProvider::makeError(int status_code,
                                                       const std::string& message)
{
    return {{"status_code", status_code}, {"error", message}};
}

/*static*/ std::string SamlAuthProvider::defaultTokenFactory(
    const auth::SAMLClaims& claims)
{
    THEMIS_TRACE("SamlAuthProvider::defaultTokenFactory invoked (claims_ptr={})",
                 static_cast<const void*>(&claims));
    // Generate a 16-byte random token and hex-encode it, prefix with "saml_"
    unsigned char buf[16]{};
    RAND_bytes(buf, static_cast<int>(sizeof(buf)));
    std::ostringstream oss = {};
    oss << "saml_";
    for (unsigned char b : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned>(b);
    }
    // claims available for enrichment by custom factories
    return oss.str();
}

/*static*/ std::string SamlAuthProvider::urlEncode(const std::string& input)
{
    std::ostringstream oss = {};
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else {
            oss << '%' << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << static_cast<unsigned>(c);
        }
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SamlAuthProvider::SamlAuthProvider(const Config& config)
    : config_(config)
{
    if (!config_.token_factory) {
        config_.token_factory = &SamlAuthProvider::defaultTokenFactory;
    }
    authenticator_ = std::make_unique<auth::SAMLAuthenticator>(config_.saml);
    THEMIS_INFO("SamlAuthProvider initialized: sp_entity_id={}, idp_entity_id={}",
                config_.saml.sp_entity_id, config_.saml.idp_entity_id);
}

// ---------------------------------------------------------------------------
// Login – SP-initiated SSO redirect
// ---------------------------------------------------------------------------

nlohmann::json SamlAuthProvider::handleLogin(const std::string& relay_state)
{
    try {
        auto params = authenticator_->buildAuthnRequest(relay_state);

        // Store request_id with a 5-minute TTL for InResponseTo validation.
        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            evictExpiredPendingRequests();
            pending_requests_[params.request_id] =
                std::chrono::system_clock::now() + std::chrono::minutes(5);
        }

        THEMIS_INFO("SamlAuthProvider::handleLogin – request_id={}", params.request_id);
        return {
            {"redirect_url", params.url},
            {"request_id",   params.request_id}
        };
    } catch (const std::exception& e) {
        THEMIS_ERROR("SamlAuthProvider::handleLogin exception: {}", e.what());
        return makeError(500, std::string("Failed to build AuthnRequest: ") + e.what());
    }
}

// ---------------------------------------------------------------------------
// ACS – Assertion Consumer Service
// ---------------------------------------------------------------------------

nlohmann::json SamlAuthProvider::handleAcs(
    const std::string& saml_response_b64,
    const std::string& relay_state,
    const std::string& in_response_to)
{
    if (saml_response_b64.empty()) {
        return makeError(400, "Missing SAMLResponse");
    }

    // If in_response_to was provided, validate it is still pending.
    if (!in_response_to.empty()) {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        evictExpiredPendingRequests();
        auto it = pending_requests_.find(in_response_to);
        if (it == pending_requests_.end()) {
            THEMIS_WARN("SamlAuthProvider::handleAcs – unknown or expired request_id={}",
                        in_response_to);
            return makeError(400, "Unknown or expired SAMLRequest InResponseTo ID");
        }
        pending_requests_.erase(it); // consume: one-time use
    }

    try {
        auto claims = authenticator_->processResponse(saml_response_b64, in_response_to);

        // Issue internal token via configured factory.
        std::string token = config_.token_factory(claims);

        THEMIS_INFO("SamlAuthProvider::handleAcs – login success: user={}, email={}",
                    claims.subject_name_id, claims.email);

        nlohmann::json attrs = nlohmann::json::object();
        for (const auto& [name, value] : claims.raw_attributes) {
            if (attrs.contains(name)) {
                if (!attrs[name].is_array()) {
                    attrs[name] = nlohmann::json::array({attrs[name]});
                }
                attrs[name].push_back(value);
            } else {
                attrs[name] = value;
            }
        }

        return {
            {"token",         token},
            {"user_id",       claims.subject_name_id},
            {"email",         claims.email},
            {"issuer",        claims.issuer},
            {"session_index", claims.session_index},
            {"relay_state",   relay_state},
            {"attributes",    attrs}
        };
    } catch (const auth::AuthException& ex) {
        using EC = auth::AuthErrorCode;
        int http_status = 401;
        const auto& auth_error = ex.error();
        switch (auth_error.code()) {
            case EC::AUTH_INSUFFICIENT_PERMISSIONS:
                http_status = 403;
                break;
            case EC::SAML_CONDITIONS_FAILED:
                // Expired / not-yet-valid assertion
                http_status = 401;
                break;
            case EC::SAML_REPLAY_DETECTED:
            [[fallthrough]];
            case EC::SAML_INVALID_SIGNATURE:
            [[fallthrough]];
            case EC::SAML_INVALID_RESPONSE:
            [[fallthrough]];
            case EC::SAML_MISSING_ASSERTION:
            [[fallthrough]];
            case EC::SAML_DESTINATION_MISMATCH:
            [[fallthrough]];
            case EC::SAML_STATUS_FAILURE:
            [[fallthrough]];
            case EC::SAML_ISSUER_MISMATCH:
                http_status = 401;
                break;
            default:
                http_status = 401;
                break;
        }
        THEMIS_WARN("SamlAuthProvider::handleAcs – auth failure: {}", ex.what());
            return makeError(http_status, auth_error.publicMessage());
    } catch (const std::exception& e) {
        THEMIS_ERROR("SamlAuthProvider::handleAcs exception: {}", e.what());
        return makeError(500, "Internal SAML processing error");
    }
}

// ---------------------------------------------------------------------------
// SLO – Single Logout
// ---------------------------------------------------------------------------

nlohmann::json SamlAuthProvider::handleSlo(const std::string& session_index)
{
    if (config_.idp_slo_url.empty()) {
        THEMIS_INFO("SamlAuthProvider::handleSlo – no IdP SLO URL configured, acknowledging locally");
        return {{"success", true}};
    }

    // Build a simplified LogoutRequest redirect URL.
    // NOTE: A standards-compliant SLO (SAML 2.0 Bindings §3.4) requires a
    // DEFLATE-compressed, Base64-encoded, and optionally signed SAMLLogoutRequest
    // element.  This implementation omits the LogoutRequest body and is suitable
    // for IdPs that accept a basic redirect carrying the issuer parameter.
    // Full LogoutRequest generation with SP private-key signing can be added once
    // SP private-key configuration is supported.
    std::string slo_url = config_.idp_slo_url;
    slo_url += (slo_url.find('?') == std::string::npos ? "?" : "&");
    slo_url += "issuer=" + urlEncode(config_.saml.sp_entity_id);
    if (!session_index.empty()) {
        slo_url += "&SessionIndex=" + urlEncode(session_index);
    }

    THEMIS_INFO("SamlAuthProvider::handleSlo – redirecting to IdP SLO: {}", config_.idp_slo_url);
    return {
        {"success",      true},
        {"redirect_url", slo_url}
    };
}

// ---------------------------------------------------------------------------
// SP Metadata
// ---------------------------------------------------------------------------

std::string SamlAuthProvider::buildMetadataXml() const
{
    const auto& saml = config_.saml;
    std::ostringstream xml = {};
    xml << R"(<?xml version="1.0" encoding="UTF-8"?>)"  "\n"
        << R"(<md:EntityDescriptor)"
        << R"( xmlns:md="urn:oasis:names:tc:SAML:2.0:metadata")"
        << R"( xmlns:ds="http://www.w3.org/2000/09/xmldsig#")"
        << " entityID=\"" << saml.sp_entity_id << "\">\n"
        << "  <md:SPSSODescriptor"
        << " AuthnRequestsSigned=\"false\""
        << " WantAssertionsSigned=\"" << (saml.require_signed_assertion ? "true" : "false") << "\""
        << " protocolSupportEnumeration=\"urn:oasis:names:tc:SAML:2.0:protocol\">\n"
        << "    <md:AssertionConsumerService"
        << " Binding=\"urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST\""
        << " Location=\"" << saml.sp_acs_url << "\""
        << " index=\"1\"/>\n";

    if (!config_.sp_slo_url.empty()) {
        xml << "    <md:SingleLogoutService"
            << " Binding=\"urn:oasis:names:tc:SAML:2.0:bindings:HTTP-Redirect\""
            << " Location=\"" << config_.sp_slo_url << "\"/>\n";
    }

    xml << "  </md:SPSSODescriptor>\n";

    if (!config_.org_name.empty()) {
        xml << "  <md:Organization>\n"
            << "    <md:OrganizationName xml:lang=\"en\">" << config_.org_name << "</md:OrganizationName>\n"
            << "    <md:OrganizationDisplayName xml:lang=\"en\">"
            << config_.org_display_name << "</md:OrganizationDisplayName>\n"
            << "    <md:OrganizationURL xml:lang=\"en\">" << config_.org_url << "</md:OrganizationURL>\n"
            << "  </md:Organization>\n";
    }

    if (!config_.contact_email.empty()) {
        xml << "  <md:ContactPerson contactType=\"technical\">\n"
            << "    <md:EmailAddress>" << config_.contact_email << "</md:EmailAddress>\n"
            << "  </md:ContactPerson>\n";
    }

    xml << "</md:EntityDescriptor>\n";
    return xml.str();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void SamlAuthProvider::setClockForTesting(
    std::function<std::chrono::system_clock::time_point()> clock)
{
    authenticator_->setClockForTesting(std::move(clock));
}

void SamlAuthProvider::evictExpiredPendingRequests()
{
    // Caller must hold pending_mutex_
    const auto now = std::chrono::system_clock::now();
    for (auto it = pending_requests_.begin(); it != pending_requests_.end(); ) {
        if (it->second <= now) {
            it = pending_requests_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace server
} // namespace themis

