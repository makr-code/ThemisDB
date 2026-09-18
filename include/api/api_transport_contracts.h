#pragma once

/**
 * @file api_transport_contracts.h
 * @brief Formal transport-surface contracts for all ThemisDB API adapters.
 *
 * @details Locks the transport-level interface contracts for the active major
 * release line (v1 / v2).  All transport adapters (HTTP, GraphQL, gRPC,
 * WebSocket) must conform to the invariants defined here.
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md):
 *  - Transport-facing contracts remain backward compatible within the major
 *    release line.
 *  - Adapter behavior is fail-closed on invalid or unsupported protocol input.
 *  - High-concurrency paths are bounded by explicit runtime controls.
 *  - Observability integration must not compromise request-path correctness.
 *
 * Phase 1 deliverable (Q3 2026): locks transport-surface contracts and defines
 * explicit failure contracts across GraphQL / gRPC / WebSocket adaptation paths.
 */

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdint>
#include "utils/expected.h"

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// API version constants
// ---------------------------------------------------------------------------

inline constexpr int kApiMajorVersion = 2;

inline constexpr std::string_view kSupportedApiVersions[] = {"v1", "v2"};

inline constexpr std::size_t kMaxPayloadBytes = 10UL * 1024UL * 1024UL;

inline constexpr std::size_t kMaxPathBytes = 4096;

inline constexpr std::size_t kMaxHeaderValueBytes = 8192;

// ---------------------------------------------------------------------------
// TransportCapability — optional features that adapters may support
// ---------------------------------------------------------------------------

enum class TransportCapability : uint32_t {
    None                = 0x00,
    TlsSupport          = 0x01, ///< Adapter supports TLS/mTLS connections.
    CompressionSupport  = 0x02, ///< Adapter supports gzip/brotli response compression.
    StreamingSupport    = 0x04, ///< Adapter supports streaming responses (SSE / gRPC streams).
    ReflectionSupport   = 0x08, ///< Adapter exposes schema reflection (debug builds only).
    RateLimitSupport    = 0x10, ///< Adapter enforces per-key rate limiting.
    SubscriptionSupport = 0x20, ///< Adapter supports long-lived subscription connections.
};

inline constexpr TransportCapability operator|(TransportCapability a,
                                               TransportCapability b) noexcept {
    return static_cast<TransportCapability>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline constexpr TransportCapability operator&(TransportCapability a,
                                               TransportCapability b) noexcept {
    return static_cast<TransportCapability>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline constexpr bool hasCapability(TransportCapability set,
                                    TransportCapability flag) noexcept {
    return (set & flag) != TransportCapability::None;
}

// ---------------------------------------------------------------------------
// FailureContract — well-typed transport-level failure modes
// ---------------------------------------------------------------------------

enum class TransportFailureClass : uint8_t {
    None = 0,            ///< No failure; request is valid.
    MalformedRequest,    ///< Request is syntactically invalid (bad method, empty path, etc.).
    PayloadTooLarge,     ///< Request body exceeds kMaxPayloadBytes.
    UnsupportedVersion,  ///< X-API-Version header names an unknown version.
    ContentTypeMissing,  ///< POST/PUT/PATCH body is present but Content-Type is absent.
    ContentTypeMismatch, ///< Content-Type header value is not acceptable for the endpoint.
    Unauthorized,        ///< Request lacks required authentication credentials.
    RateLimitExceeded,   ///< Request rate from this key/IP exceeds configured limits.
    CapabilityUnavailable,///< Requested transport feature is not available in this deployment.
    InternalError,       ///< Unexpected adapter-internal error (maps to 500).
};

// ---------------------------------------------------------------------------
// ITransportContract — interface every transport adapter must implement
// ---------------------------------------------------------------------------

class ITransportContract {
public:
    /**
     * @brief ITransport Contract.
     * @return Return value.
     */
    virtual ~ITransportContract() = default;

    [[nodiscard]] virtual TransportCapability capabilities() const noexcept {
        return TransportCapability::None;
    }

    [[nodiscard]] virtual std::vector<std::string> supportedVersions() const {
        std::vector<std::string> versions = {};

        for (const auto& v : kSupportedApiVersions) {
            versions.emplace_back(v);
        }
        return versions;
    }

    [[nodiscard]] virtual TransportFailureClass classifyFailure(
        std::string_view method,
        std::string_view path,
        std::string_view content_type,
        std::size_t      payload_bytes,
        std::string_view api_version) const noexcept {
        (void)method;
        (void)path;
        (void)content_type;
        (void)payload_bytes;
        (void)api_version;
        return TransportFailureClass::None;
    }

    [[nodiscard]] virtual std::string_view adapterName() const noexcept = 0;
};

// ---------------------------------------------------------------------------
// TransportContractValidator — stateless helpers for contract checks
// ---------------------------------------------------------------------------

class TransportContractValidator {
public:
    [[nodiscard]] static bool isSupportedVersion(std::string_view version) noexcept {
        if (version.empty()) {
            return true; // absent header → use default version
        }
        for (const auto& v : kSupportedApiVersions) {
            if (v == version) {
              return true;
            }
        }
        return false;
    }

    [[nodiscard]] static constexpr bool isPayloadWithinLimit(
        std::size_t payload_bytes) noexcept {
        return payload_bytes <= kMaxPayloadBytes;
    }

    [[nodiscard]] static bool requiresContentType(std::string_view method) noexcept {
        return method == "POST" || method == "PUT" || method == "PATCH";
    }

    [[nodiscard]] static constexpr bool isPathLengthValid(
        std::string_view path) noexcept {
        return path.size() <= kMaxPathBytes;
    }

    [[nodiscard]] static TransportFailureClass validate(
        std::string_view method,
        std::string_view path,
        std::string_view content_type,
        std::size_t      payload_bytes,
        std::string_view api_version) noexcept {

        if (method.empty() || path.empty()) {
            return TransportFailureClass::MalformedRequest;
        }
        if (!isPathLengthValid(path)) {
            return TransportFailureClass::MalformedRequest;
        }
        if (!isPayloadWithinLimit(payload_bytes)) {
            return TransportFailureClass::PayloadTooLarge;
        }
        if (!isSupportedVersion(api_version)) {
            return TransportFailureClass::UnsupportedVersion;
        }
        if (requiresContentType(method) && payload_bytes > 0 && content_type.empty()) {
            return TransportFailureClass::ContentTypeMissing;
        }
        return TransportFailureClass::None;
    }
};

} // namespace api
} // namespace themis
