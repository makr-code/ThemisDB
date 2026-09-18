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

/**
 * @brief Configuration for `TransportPolicyMiddleware`.
 *
 * Allows individual deployment profiles to tighten (but not relax) the
 * default kMaxPayloadBytes / kMaxPathBytes limits.  Values larger than the
 * global limits are silently clamped to maintain the baseline contract.
 */
struct TransportPolicyConfig {
    /**
     * @brief Maximum request body size enforced by this policy instance.
     *
     * Must be ≤ `kMaxPayloadBytes`.  Values above the global limit are clamped.
     * Default: `kMaxPayloadBytes` (10 MiB).
     */
    std::size_t max_payload_bytes = kMaxPayloadBytes;

    /**
     * @brief Maximum request path length enforced by this policy instance.
     *
     * Must be ≤ `kMaxPathBytes`.  Values above the global limit are clamped.
     * Default: `kMaxPathBytes` (4096 bytes).
     */
    std::size_t max_path_bytes = kMaxPathBytes;

    /**
     * @brief If `true`, enforce the Content-Type requirement for
     *        POST / PUT / PATCH requests that carry a non-empty body.
     *
     * Default: `true` (enforcement enabled).
     */
    bool enforce_content_type = true;

    /**
     * @brief If `true`, reject requests carrying an X-API-Version header that
     *        names an unsupported version.
     *
     * Default: `true` (enforcement enabled).
     */
    bool enforce_api_version = true;

    /**
     * @brief Return a config clamped to the global transport limits.
     *
     * Callers that build a `TransportPolicyConfig` from untrusted sources
     * (e.g., operator configuration files) should call `normalized()` before
     * passing the config to the middleware constructor.
     */
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

/**
 * @brief `IHttpHandler` decorator that enforces the shared API transport policy.
 *
 * Wraps any `IHttpHandler` implementation and validates each inbound request
 * against the transport contract rules before delegation.  If any rule is
 * violated the middleware returns a structured error immediately, without
 * calling the wrapped handler (fail-closed behavior).
 *
 * @note Implements `ITransportContract` in addition to `IHttpHandler` so that
 * the middleware can participate in capability queries from outer layers.
 *
 * ### Error path guarantees
 * - Every policy violation produces an `themis::Error` with a code from
 *   `ApiErrorTaxonomy::toErrorCode()` and a message from
 *   `ApiErrorTaxonomy::toMessage()`.
 * - The failure class name is embedded in the error message for traceability.
 *
 * ### Thread safety
 * Immutable after construction; safe for concurrent calls from any thread.
 */
class TransportPolicyMiddleware final : public IHttpHandler,
                                        public ITransportContract {
public:
    /**
     * @brief Construct with a wrapped handler and default policy config.
     *
     * @param inner  Handler to delegate valid requests to.  Must not be null.
     *               Ownership is shared; the middleware holds a `shared_ptr`.
     * @throws std::invalid_argument if @p inner is null.
     */
    explicit TransportPolicyMiddleware(std::shared_ptr<IHttpHandler> inner);

    /**
     * @brief Construct with an explicit policy configuration.
     *
     * The config is normalized via `TransportPolicyConfig::normalized()` so
     * values above the global limits are silently clamped.
     *
     * @param inner   Handler to delegate valid requests to.  Must not be null.
     * @param config  Policy bounds to enforce.
     * @throws std::invalid_argument if @p inner is null.
     */
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

    /**
     * @brief Validate the request against transport policy rules, then delegate.
     *
     * If any policy rule is violated the method returns a structured error
     * without calling the wrapped handler.  On success, the response from the
     * inner handler is returned unchanged.
     *
     * Error codes are determined by `ApiErrorTaxonomy::toErrorCode()`.
     *
     * @param request  Inbound HTTP request.
     * @return `Result<HttpResponse>` from the inner handler, or a transport
     *         policy error if validation fails.
     */
    [[nodiscard]] themis::Result<HttpResponse> handle(
        const HttpRequest& request) override;

    /**
     * @brief Delegates to the wrapped handler's `requiresAuthentication()`.
     */
    [[nodiscard]] bool requiresAuthentication() const noexcept override;

    /**
     * @brief Returns `"transport-policy"`.
     */
    [[nodiscard]] std::string_view handlerName() const noexcept override;

    // -----------------------------------------------------------------------
    // ITransportContract interface
    // -----------------------------------------------------------------------

    /**
     * @brief Delegates capability reporting to the wrapped handler if it also
     *        implements `ITransportContract`; otherwise returns `None`.
     */
    [[nodiscard]] TransportCapability capabilities() const noexcept override;

    /**
     * @brief Returns `"transport-policy-middleware"`.
     */
    [[nodiscard]] std::string_view adapterName() const noexcept override;

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Return the active policy configuration.
     */
    [[nodiscard]] const TransportPolicyConfig& config() const noexcept {
        return config_;
    }

private:
    std::shared_ptr<IHttpHandler> inner_;
    TransportPolicyConfig         config_;

    /**
     * @brief Apply all policy checks and return the failure class.
     *
     * @param request  Request to validate.
     * @return `TransportFailureClass::None` on success; the first violated
     *         rule's failure class otherwise.
     */
    [[nodiscard]] TransportFailureClass applyPolicy(
        const HttpRequest& request) const noexcept;
};

} // namespace api
} // namespace themis
