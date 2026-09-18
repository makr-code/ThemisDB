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

class ApiErrorTaxonomy {
public:
    // Not constructible — all members are static.
    ApiErrorTaxonomy() = delete;

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

    [[nodiscard]] static std::string toMessage(TransportFailureClass fc,
                                               std::string_view adapter_name) {
        /**
         * @brief Prefix.
         * @param[in] adapter_name Name of the adapter.
         * @return Return value.
         */
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

    [[nodiscard]] static constexpr bool isClientError(
        TransportFailureClass fc) noexcept {
        const int status = toHttpStatus(fc);
        return status >= 400 && status < 500;
    }
};

} // namespace api
} // namespace themis
