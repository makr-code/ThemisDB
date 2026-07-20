/**
 * @file api_transport_policy.cpp
 * @brief Implementation of the shared API transport policy enforcement middleware.
 *
 * @details Implements `TransportPolicyMiddleware` which wraps any `IHttpHandler`
 * and enforces the canonical transport-level policy rules (Phase 2 + Phase 3,
 * Q4 2026) before delegating to the application handler.
 *
 * All policy violations are fail-closed: the request is rejected immediately
 * with a structured error produced by `ApiErrorTaxonomy`.  The wrapped handler
 * is not called on a violation.
 */

#include "api/api_transport_policy.h"

#include <stdexcept>

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TransportPolicyMiddleware::TransportPolicyMiddleware(
    std::shared_ptr<IHttpHandler> inner)
    : TransportPolicyMiddleware(std::move(inner), TransportPolicyConfig{}) {
}

TransportPolicyMiddleware::TransportPolicyMiddleware(
    std::shared_ptr<IHttpHandler> inner,
    const TransportPolicyConfig&  config)
    : inner_(std::move(inner))
    , config_(config.normalized()) {
    if (!inner_) {
        throw std::invalid_argument(
            "TransportPolicyMiddleware: wrapped handler must not be null");
    }
}

// ---------------------------------------------------------------------------
// IHttpHandler interface
// ---------------------------------------------------------------------------

themis::Result<HttpResponse> TransportPolicyMiddleware::handle(
    const HttpRequest& request) {

    const TransportFailureClass fc = applyPolicy(request);
    if (fc != TransportFailureClass::None) {
        const auto code = ApiErrorTaxonomy::toErrorCode(fc);
        const auto msg  = ApiErrorTaxonomy::toMessage(fc, adapterName());
        return tl::unexpected(themis::Error(code, msg));
    }

    return inner_->handle(request);
}

bool TransportPolicyMiddleware::requiresAuthentication() const noexcept {
    return inner_->requiresAuthentication();
}

std::string_view TransportPolicyMiddleware::handlerName() const noexcept {
    return "transport-policy";
}

// ---------------------------------------------------------------------------
// ITransportContract interface
// ---------------------------------------------------------------------------

TransportCapability TransportPolicyMiddleware::capabilities() const noexcept {
    if (const auto* contract =
            dynamic_cast<const ITransportContract*>(inner_.get())) {
        return contract->capabilities();
    }
    return TransportCapability::None;
}

std::string_view TransportPolicyMiddleware::adapterName() const noexcept {
    return "transport-policy-middleware";
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

TransportFailureClass TransportPolicyMiddleware::applyPolicy(
    const HttpRequest& request) const noexcept {

    // Rule 1: method and path must be non-empty.
    if (request.method.empty() || request.path.empty()) {
        return TransportFailureClass::MalformedRequest;
    }

    // Rule 2: path length check.
    if (request.path.size() > config_.max_path_bytes) {
        return TransportFailureClass::MalformedRequest;
    }

    // Rule 3: payload size check.
    if (request.body.size() > config_.max_payload_bytes) {
        return TransportFailureClass::PayloadTooLarge;
    }

    // Rule 4: API version check.
    if (config_.enforce_api_version) {
        auto it = request.headers.find("X-API-Version");
        if (it != request.headers.end()) {
            if (!TransportContractValidator::isSupportedVersion(it->second)) {
                return TransportFailureClass::UnsupportedVersion;
            }
        }
    }

    // Rule 5: Content-Type enforcement for mutating methods with a body.
    if (config_.enforce_content_type
        && TransportContractValidator::requiresContentType(request.method)
        && !request.body.empty()) {
        auto it_ct  = request.headers.find("Content-Type");
        auto it_ctL = request.headers.find("content-type");
        const bool has_ct = (it_ct  != request.headers.end() && !it_ct->second.empty())
                         || (it_ctL != request.headers.end() && !it_ctL->second.empty());
        if (!has_ct) {
            return TransportFailureClass::ContentTypeMissing;
        }
    }

    return TransportFailureClass::None;
}

} // namespace api
} // namespace themis
