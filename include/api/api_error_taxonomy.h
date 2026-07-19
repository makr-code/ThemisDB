#pragma once

/**
 * @file api_error_taxonomy.h
 * @brief Unified error taxonomy across all ThemisDB API transport adapters.
 *
 * @details Maps transport-specific failure classes to canonical ThemisDB error
 * codes and HTTP status codes.  Provides a single authoritative source of truth
 * for error semantics shared by HTTP/REST, GraphQL, gRPC, and WebSocket layers.
 *
 * Design goals (Phase 3, Q4 2026):
 *  - Standardize fail-closed behavior for malformed payload and unsupported
 *    capability states.
 *  - Unify error taxonomy across transport adapters and middleware paths.
 *  - Keep error class → HTTP status → ThemisDB error code mapping deterministic
 *    and testable.
 *
 * ### Usage
 * ```cpp
 * auto fc = TransportContractValidator::validate(...);
 * if (fc != TransportFailureClass::None) {
 *     auto code = ApiErrorTaxonomy::toErrorCode(fc);
 *     auto status = ApiErrorTaxonomy::toHttpStatus(fc);
 *     auto msg    = ApiErrorTaxonomy::toMessage(fc, "my-adapter");
 *     return tl::unexpected(themis::Error(code, msg));
 * }
 * ```
 */

#include <string>
#include <string_view>
#include "api/api_transport_contracts.h"
#include "utils/error_registry.h"

namespace themis {
namespace api {

/**
 * @brief Stateless mapping utilities that translate `TransportFailureClass`
 *        values to canonical ThemisDB error codes, HTTP status codes, and
 *        human-readable error message prefixes.
 *
 * All functions are `static` and thread-safe by construction (no shared state).
 */
class ApiErrorTaxonomy {
public:
    // Not constructible — all members are static.
    ApiErrorTaxonomy() = delete;

    /**
     * @brief Map a transport failure class to a ThemisDB error code.
     *
     * Error code mapping:
     * | TransportFailureClass      | ErrorCode                    |
     * |----------------------------|------------------------------|
     * | None                       | (not an error)               |
     * | MalformedRequest           | ERR_API_INVALID_REQUEST      |
     * | PayloadTooLarge            | ERR_API_INVALID_REQUEST      |
     * | UnsupportedVersion         | ERR_API_INVALID_REQUEST      |
     * | ContentTypeMissing         | ERR_API_INVALID_REQUEST      |
     * | ContentTypeMismatch        | ERR_API_INVALID_REQUEST      |
     * | Unauthorized               | ERR_API_UNAUTHORIZED         |
     * | RateLimitExceeded          | ERR_API_RATE_LIMIT           |
     * | CapabilityUnavailable      | ERR_API_INVALID_REQUEST      |
     * | InternalError              | ERR_API_INTERNAL_ERROR       |
     *
     * @param fc  Transport failure class to map.
     * @return Corresponding `themis::errors::ErrorCode`.
     */
    [[nodiscard]] static themis::errors::ErrorCode toErrorCode(
        TransportFailureClass fc) noexcept {
        switch (fc) {
            case TransportFailureClass::None:
                return themis::errors::ErrorCode::ERR_API_INTERNAL_ERROR; // should not be called
            case TransportFailureClass::Unauthorized:
                return themis::errors::ErrorCode::ERR_API_UNAUTHORIZED;
            case TransportFailureClass::RateLimitExceeded:
                return themis::errors::ErrorCode::ERR_API_RATE_LIMIT;
            case TransportFailureClass::InternalError:
                return themis::errors::ErrorCode::ERR_API_INTERNAL_ERROR;
            case TransportFailureClass::MalformedRequest:
            case TransportFailureClass::PayloadTooLarge:
            case TransportFailureClass::UnsupportedVersion:
            case TransportFailureClass::ContentTypeMissing:
            case TransportFailureClass::ContentTypeMismatch:
            case TransportFailureClass::CapabilityUnavailable:
            default:
                return themis::errors::ErrorCode::ERR_API_INVALID_REQUEST;
        }
    }

    /**
     * @brief Map a transport failure class to an HTTP status code.
     *
     * Status code mapping:
     * | TransportFailureClass      | HTTP Status |
     * |----------------------------|-------------|
     * | None                       | 200         |
     * | MalformedRequest           | 400         |
     * | PayloadTooLarge            | 413         |
     * | UnsupportedVersion         | 400         |
     * | ContentTypeMissing         | 415         |
     * | ContentTypeMismatch        | 415         |
     * | Unauthorized               | 401         |
     * | RateLimitExceeded          | 429         |
     * | CapabilityUnavailable      | 501         |
     * | InternalError              | 500         |
     *
     * @param fc  Transport failure class to map.
     * @return HTTP status code integer.
     */
    [[nodiscard]] static constexpr int toHttpStatus(
        TransportFailureClass fc) noexcept {
        switch (fc) {
            case TransportFailureClass::None:
                return 200;
            case TransportFailureClass::MalformedRequest:
            case TransportFailureClass::UnsupportedVersion:
                return 400;
            case TransportFailureClass::Unauthorized:
                return 401;
            case TransportFailureClass::RateLimitExceeded:
                return 429;
            case TransportFailureClass::PayloadTooLarge:
                return 413;
            case TransportFailureClass::ContentTypeMissing:
            case TransportFailureClass::ContentTypeMismatch:
                return 415;
            case TransportFailureClass::CapabilityUnavailable:
                return 501;
            case TransportFailureClass::InternalError:
            default:
                return 500;
        }
    }

    /**
     * @brief Produce a structured error message prefix for the given failure class.
     *
     * The format is `ERR_<DOMAIN>_<CATEGORY>: <human-readable description>`.
     * Callers should append request-specific context (e.g., received payload
     * size, unsupported version string) after the returned prefix.
     *
     * @param fc            Transport failure class.
     * @param adapter_name  Adapter identifier used in the message (e.g., "http-rest").
     * @return Structured error message prefix string.
     */
    [[nodiscard]] static std::string toMessage(TransportFailureClass fc,
                                               std::string_view adapter_name) {
        std::string prefix(adapter_name);
        prefix += ": ";
        switch (fc) {
            case TransportFailureClass::None:
                return prefix + "no error";
            case TransportFailureClass::MalformedRequest:
                return prefix + "ERR_TRANSPORT_MALFORMED_REQUEST: request method or path is missing or too long (path limit: " + std::to_string(kMaxPathBytes) + " bytes)";
            case TransportFailureClass::PayloadTooLarge:
                return prefix + "ERR_TRANSPORT_PAYLOAD_TOO_LARGE: request body exceeds maximum allowed size";
            case TransportFailureClass::UnsupportedVersion:
                return prefix + "ERR_TRANSPORT_UNSUPPORTED_VERSION: X-API-Version header names an unsupported version; accepted: v1, v2";
            case TransportFailureClass::ContentTypeMissing:
                return prefix + "ERR_TRANSPORT_CONTENT_TYPE_MISSING: Content-Type header is required for POST/PUT/PATCH with a body";
            case TransportFailureClass::ContentTypeMismatch:
                return prefix + "ERR_TRANSPORT_CONTENT_TYPE_MISMATCH: Content-Type is not acceptable for this endpoint";
            case TransportFailureClass::Unauthorized:
                return prefix + "ERR_TRANSPORT_UNAUTHORIZED: request lacks valid authentication credentials";
            case TransportFailureClass::RateLimitExceeded:
                return prefix + "ERR_TRANSPORT_RATE_LIMIT_EXCEEDED: request rate exceeds configured limit for this key";
            case TransportFailureClass::CapabilityUnavailable:
                return prefix + "ERR_TRANSPORT_CAPABILITY_UNAVAILABLE: requested transport feature is not available in this deployment";
            case TransportFailureClass::InternalError:
            default:
                return prefix + "ERR_TRANSPORT_INTERNAL: unexpected internal transport error";
        }
    }

    /**
     * @brief Classify a `TransportFailureClass` as client-side or server-side.
     *
     * Client-side failures (4xx) are caused by invalid or unauthorized requests
     * and should not trigger server-side alerting.  Server-side failures (5xx)
     * indicate adapter or infrastructure problems.
     *
     * @param fc  Transport failure class.
     * @return `true` if the failure is client-side (HTTP 4xx); `false` for 5xx.
     */
    [[nodiscard]] static constexpr bool isClientError(
        TransportFailureClass fc) noexcept {
        const int status = toHttpStatus(fc);
        return status >= 400 && status < 500;
    }
};

} // namespace api
} // namespace themis
