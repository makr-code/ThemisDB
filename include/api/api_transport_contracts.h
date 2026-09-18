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

/**
 * @brief Current major API version.
 *
 * Incremented on breaking transport-contract changes only.  Minor enhancements
 * within a major version must remain backward compatible.
 */
inline constexpr int kApiMajorVersion = 2;

/**
 * @brief List of supported API version strings accepted in X-API-Version.
 *
 * "v1" is the legacy version; it remains supported until the v1 sunset date.
 * "v2" is the current version.
 *
 * Validators must accept any value in this list and reject all others with a
 * well-formed ERR_API_INVALID_REQUEST response (fail-closed contract).
 */
inline constexpr std::string_view kSupportedApiVersions[] = {"v1", "v2"};

/**
 * @brief Maximum permitted request payload size in bytes (10 MiB).
 *
 * Enforced at the transport layer before any application-level processing.
 * Requests exceeding this limit are rejected with ERR_API_INVALID_REQUEST.
 */
inline constexpr std::size_t kMaxPayloadBytes = 10UL * 1024UL * 1024UL;

/**
 * @brief Maximum permitted request path length in bytes.
 *
 * Prevents resource exhaustion from pathological routing inputs.
 */
inline constexpr std::size_t kMaxPathBytes = 4096;

/**
 * @brief Maximum permitted request header value length in bytes.
 */
inline constexpr std::size_t kMaxHeaderValueBytes = 8192;

// ---------------------------------------------------------------------------
// TransportCapability — optional features that adapters may support
// ---------------------------------------------------------------------------

/**
 * @brief Capability flags that a transport adapter may advertise.
 *
 * Adapters report their capabilities via `ITransportContract::capabilities()`.
 * Callers must not assume any capability is present without checking.
 */
enum class TransportCapability : uint32_t {
    None                = 0x00,
    TlsSupport          = 0x01, ///< Adapter supports TLS/mTLS connections.
    CompressionSupport  = 0x02, ///< Adapter supports gzip/brotli response compression.
    StreamingSupport    = 0x04, ///< Adapter supports streaming responses (SSE / gRPC streams).
    ReflectionSupport   = 0x08, ///< Adapter exposes schema reflection (debug builds only).
    RateLimitSupport    = 0x10, ///< Adapter enforces per-key rate limiting.
    SubscriptionSupport = 0x20, ///< Adapter supports long-lived subscription connections.
};

/**
 * @brief Bitwise OR for TransportCapability flags.
 * @param a Left-hand capability set.
 * @param b Right-hand capability set.
 * @return Combined capability set.
 */
inline constexpr TransportCapability operator|(TransportCapability a,
                                               TransportCapability b) noexcept {
    return static_cast<TransportCapability>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * @brief Bitwise AND for TransportCapability flags.
 * @param a Left-hand capability set.
 * @param b Right-hand capability set.
 * @return Intersection of the two capability sets.
 */
inline constexpr TransportCapability operator&(TransportCapability a,
                                               TransportCapability b) noexcept {
    return static_cast<TransportCapability>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/**
 * @brief Test whether a capability is present in a combined set.
 * @param set   Combined capability flags.
 * @param flag  Single capability to test.
 * @return `true` if @p flag is set in @p set.
 */
inline constexpr bool hasCapability(TransportCapability set,
                                    TransportCapability flag) noexcept {
    return (set & flag) != TransportCapability::None;
}

// ---------------------------------------------------------------------------
// FailureContract — well-typed transport-level failure modes
// ---------------------------------------------------------------------------

/**
 * @brief Canonical failure classes at the transport layer.
 *
 * Every transport adapter must map its internal errors to one of these
 * failure classes so that observability and alerting systems can reason
 * uniformly across GraphQL, gRPC, and WebSocket surfaces.
 *
 * The failure class determines the HTTP status code and error code returned
 * to the client (see ApiErrorTaxonomy for the mapping).
 */
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

/**
 * @brief Interface that every API transport adapter (HTTP, GraphQL, gRPC,
 *        WebSocket) must implement.
 *
 * The contract formalizes three obligations:
 *
 * 1. **Capability advertisement** — adapters report which optional features
 *    are active so that generic policy middleware can make routing decisions
 *    without hard-coding adapter types.
 *
 * 2. **Version support declaration** — adapters declare the set of API
 *    version strings they accept.  The base implementation returns the
 *    global kSupportedApiVersions list.
 *
 * 3. **Failure classification** — adapters must provide `classifyFailure()`
 *    so that the policy middleware can emit uniform error codes and metrics
 *    regardless of which transport generated the failure.
 *
 * ### Thread safety
 * All methods must be safe to call concurrently from multiple threads.
 * Capability and version queries are typically const and stateless.
 */
class ITransportContract {
public:
    virtual ~ITransportContract() = default;

    /**
     * @brief Return the capability flags active for this adapter.
     *
     * Default implementation returns `TransportCapability::None`.
     * Override to advertise optional capabilities that are configured at
     * runtime (e.g., TLS, rate limiting).
     */
    [[nodiscard]] virtual TransportCapability capabilities() const noexcept {
        return TransportCapability::None;
    }

    /**
     * @brief Return the API version strings accepted by this adapter.
     *
     * Default implementation returns the global kSupportedApiVersions list.
     * Override only if the adapter restricts supported versions below the
     * global list (e.g., a v2-only gRPC endpoint).
     *
     * @return Vector of accepted version strings (e.g., {"v1", "v2"}).
     */
    [[nodiscard]] virtual std::vector<std::string> supportedVersions() const {
        std::vector<std::string> versions = {};

        for (const auto& v : kSupportedApiVersions) {
            versions.emplace_back(v);
        }
        return versions;
    }

    /**
     * @brief Classify an inbound request for pre-processing validation.
     *
     * Called by the policy middleware before the request is dispatched to the
     * application handler.  Returns `TransportFailureClass::None` if the request
     * passes all transport-layer checks.
     *
     * Default implementation returns `TransportFailureClass::None` (pass-through).
     * Subclasses performing transport-specific validation should override this.
     *
     * @param method         HTTP method or transport-specific operation name.
     * @param path           Request path or gRPC service/method name.
     * @param content_type   Value of the Content-Type header, or empty.
     * @param payload_bytes  Request body size in bytes.
     * @param api_version    Value of X-API-Version header, or empty.
     * @return Failure class; `TransportFailureClass::None` indicates a valid request.
     */
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

    /**
     * @brief Human-readable adapter name used in logs and metric labels.
     *
     * @return Stable identifier string, e.g. "http-rest", "graphql", "grpc", "websocket".
     */
    [[nodiscard]] virtual std::string_view adapterName() const noexcept = 0;
};

// ---------------------------------------------------------------------------
// TransportContractValidator — stateless helpers for contract checks
// ---------------------------------------------------------------------------

/**
 * @brief Stateless utility functions for enforcing transport contracts.
 *
 * All methods are `constexpr`-friendly or `static` and can be used in
 * middleware, tests, and validators without constructing an object.
 *
 * ### Failure contract guarantees
 * - `isSupportedVersion()` returns `false` for any version string not in
 *   kSupportedApiVersions; empty strings (absent header) are treated as valid
 *   (the header is optional and defaults to the latest supported version).
 * - `isPayloadWithinLimit()` uses the global kMaxPayloadBytes bound.
 * - `requiresContentType()` returns `true` for all mutating HTTP methods.
 */
class TransportContractValidator {
public:
    /**
     * @brief Check whether @p version is in the supported versions list.
     *
     * Empty @p version (header absent) is treated as valid because the header
     * is optional; routing and handlers default to the latest supported version.
     *
     * @param version  Value of the X-API-Version header, or empty.
     * @return `true` if the version is supported or absent.
     */
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

    /**
     * @brief Check whether @p payload_bytes is within the configured limit.
     *
     * @param payload_bytes  Request body size in bytes.
     * @return `true` if within the kMaxPayloadBytes limit.
     */
    [[nodiscard]] static constexpr bool isPayloadWithinLimit(
        std::size_t payload_bytes) noexcept {
        return payload_bytes <= kMaxPayloadBytes;
    }

    /**
     * @brief Check whether the given HTTP method requires a Content-Type header.
     *
     * Returns `true` for POST, PUT, and PATCH.  GET, DELETE, HEAD, and OPTIONS
     * do not require a Content-Type (they may carry a body, but it is unusual
     * and not enforced at the transport layer).
     *
     * @param method  HTTP method string (case-sensitive, should be upper-case).
     * @return `true` if Content-Type enforcement applies.
     */
    [[nodiscard]] static bool requiresContentType(std::string_view method) noexcept {
        return method == "POST" || method == "PUT" || method == "PATCH";
    }

    /**
     * @brief Check whether a path is within the configured length limit.
     *
     * @param path  Request path string.
     * @return `true` if the path length is within kMaxPathBytes.
     */
    [[nodiscard]] static constexpr bool isPathLengthValid(
        std::string_view path) noexcept {
        return path.size() <= kMaxPathBytes;
    }

    /**
     * @brief Validate a request against all baseline transport contract rules.
     *
     * Runs in order: method non-empty → path non-empty and length valid →
     * payload within limit → version supported → Content-Type present for
     * mutating methods (if body non-empty).
     *
     * @param method         HTTP method string.
     * @param path           Request path.
     * @param content_type   Content-Type header value, or empty.
     * @param payload_bytes  Request body size in bytes.
     * @param api_version    X-API-Version header value, or empty.
     * @return `TransportFailureClass::None` on success, or the first violated
     *         contract class.
     */
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
