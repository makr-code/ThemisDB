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
    const HttpRequest& req) const noexcept {

    // NOTE (no_retry_logic scanner suppression — Wave B/C, 8 findings):
    // Each rule below is a deterministic, synchronous policy check with no
    // external I/O.  Failures here are permanent for the *current request*
    // (malformed path, oversized body, missing header, etc.); retry semantics
    // are meaningless because a repeated identical request would fail in
    // exactly the same way.  Retry is the responsibility of the caller, not
    // of this validation layer.  The scanner heuristic that maps early-return
    // failure paths to missing retry logic does not apply here.

    // Rule 1: method and path must be non-empty.
    if (req.method.empty() || req.path.empty()) {
        return TransportFailureClass::MalformedRequest; // non-retryable: structural defect
    }

    // Rule 2: path length check.
    if (config_.max_path_bytes >= 0 && req.path.size() > static_cast<std::size_t>(config_.max_path_bytes)) {
        return TransportFailureClass::MalformedRequest; // non-retryable: request property
    }

    // Rule 3: payload size check.
    if (config_.max_payload_bytes >= 0 && req.body.size() > static_cast<std::size_t>(config_.max_payload_bytes)) {
        return TransportFailureClass::PayloadTooLarge; // non-retryable: request property
    }

    // Rule 4: API version check.
    if (config_.enforce_api_version) {
        auto it = req.headers.find("X-API-Version");
        if (it != req.headers.end()) {
            if (!TransportContractValidator::isSupportedVersion(it->second)) {
                return TransportFailureClass::UnsupportedVersion; // non-retryable: caller version
            }
        }
    }

    // Rule 5: Content-Type enforcement for mutating methods with a body.
    if (config_.enforce_content_type
        && TransportContractValidator::requiresContentType(req.method)
        && !req.body.empty()) {
        auto it_ct  = req.headers.find("Content-Type");
        auto it_ctL = req.headers.find("content-type");
        const bool has_ct = (it_ct  != req.headers.end() && !it_ct->second.empty())
                         || (it_ctL != req.headers.end() && !it_ctL->second.empty());
        if (!has_ct) {
            return TransportFailureClass::ContentTypeMissing; // non-retryable: request property
        }
    }

    return TransportFailureClass::None;
}

} // namespace api
} // namespace themis
