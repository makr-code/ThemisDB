#pragma once

/**
 * @file api_transport_policy.h
 * @brief Shared API transport policy enforcement middleware.
 *
 * @details `TransportPolicyMiddleware` is an `IHttpHandler` decorator that
 * enforces the canonical API transport policy rules (Phase 2 + Phase 3,
 * Q4 2026) before dispatching to the wrapped application handler.
 *
 * Enforced rules (fail-closed on any violation):
 *  1. Method and path must be non-empty (`TransportFailureClass::MalformedRequest`).
 *  2. Request path must not exceed `kMaxPathBytes`.
 *  3. Request body must not exceed `kMaxPayloadBytes`.
 *  4. X-API-Version header, if present, must be in `kSupportedApiVersions`.
 *  5. POST / PUT / PATCH with a non-empty body must include a `Content-Type` header.
 *
 * All policy violations produce a structured error via `ApiErrorTaxonomy` so
 * that the error code, HTTP status, and message are consistent across all
 * transport adapters that use this middleware.
 *
 * ### Typical usage
 * ```cpp
 * // Default bounds:
 * auto policy = std::make_shared<TransportPolicyMiddleware>(inner_handler);
 *
 * // Optional: override bounds from the default config.
 * TransportPolicyConfig cfg;
 * cfg.max_payload_bytes = 1 * 1024 * 1024; // 1 MiB for this endpoint
 * auto custom_policy = std::make_shared<TransportPolicyMiddleware>(inner_handler, cfg);
 *
 * auto result = policy->handle(request);
 * ```
 *
 * ### Thread safety
 * `TransportPolicyMiddleware` is immutable after construction and safe to call
 * concurrently from any number of threads.
 */

#include <memory>
#include <cstddef>
#include "api/http_handler.h"
#include "api/api_transport_contracts.h"
#include "api/api_error_taxonomy.h"

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// TransportPolicyConfig — runtime-configurable bounds
// ---------------------------------------------------------------------------

struct TransportPolicyConfig {
    std::size_t max_payload_bytes = kMaxPayloadBytes;

    std::size_t max_path_bytes = kMaxPathBytes;

    bool enforce_content_type = true;

    bool enforce_api_version = true;

    [[nodiscard]] TransportPolicyConfig normalized() const noexcept {
        TransportPolicyConfig cfg = *this;
        if (cfg.max_payload_bytes > kMaxPayloadBytes) {
            cfg.max_payload_bytes = kMaxPayloadBytes;
        }
        if (cfg.max_path_bytes > kMaxPathBytes) {
            cfg.max_path_bytes = kMaxPathBytes;
        }
        return cfg;
    }
};

// ---------------------------------------------------------------------------
// TransportPolicyMiddleware — policy enforcement decorator
// ---------------------------------------------------------------------------

class TransportPolicyMiddleware final : public IHttpHandler,
                                        public ITransportContract {
public:
    /**
     * @brief Transport Policy Middleware.
     * @param[in] inner Input parameter.
     * @return Return value.
     */
    explicit TransportPolicyMiddleware(std::shared_ptr<IHttpHandler> inner);

    TransportPolicyMiddleware(std::shared_ptr<IHttpHandler> inner,
                              const TransportPolicyConfig& config);

    ~TransportPolicyMiddleware() override = default;

    // Non-copyable; movable.
    TransportPolicyMiddleware(const TransportPolicyMiddleware&) = delete;
    TransportPolicyMiddleware& operator=(const TransportPolicyMiddleware&) = delete;
    TransportPolicyMiddleware(TransportPolicyMiddleware&&) noexcept = default;
    TransportPolicyMiddleware& operator=(TransportPolicyMiddleware&&) noexcept = default;

    // -----------------------------------------------------------------------
    // IHttpHandler interface
    // -----------------------------------------------------------------------

    [[nodiscard]] themis::Result<HttpResponse> handle(
        const HttpRequest& request) override;

    [[nodiscard]] bool requiresAuthentication() const noexcept override;

    [[nodiscard]] std::string_view handlerName() const noexcept override;

    // -----------------------------------------------------------------------
    // ITransportContract interface
    // -----------------------------------------------------------------------

    [[nodiscard]] TransportCapability capabilities() const noexcept override;

    [[nodiscard]] std::string_view adapterName() const noexcept override;

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    [[nodiscard]] const TransportPolicyConfig& config() const noexcept {
        return config_;
    }

private:
    std::shared_ptr<IHttpHandler> inner_;
    TransportPolicyConfig         config_;

    [[nodiscard]] TransportFailureClass applyPolicy(
        const HttpRequest& request) const noexcept;
};

} // namespace api
} // namespace themis
