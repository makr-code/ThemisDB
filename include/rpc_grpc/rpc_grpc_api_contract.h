/**
 * @file rpc_grpc_api_contract.h
 * @brief Frozen gRPC plugin and server lifecycle API contract for v1.x.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Defines the normative contract for the rpc_grpc module covering gRPC server
 * lifecycle operations, TLS/mTLS credential management, service registration,
 * stream adapter behaviour, and method-level observability.
 *
 * ## §API Contracts
 *
 * Key behavioural invariants:
 *   1. A gRPC server may not accept requests before StartServer() completes
 *      successfully; any call in kStopped state yields kServerNotRunning.
 *   2. Service registration is idempotent if the name and handler match;
 *      conflicting registrations (same name, different handler) produce
 *      kServiceRegistration.
 *   3. TLS credential reload is atomic: the old credentials remain active
 *      until the new certificate chain is fully validated.  Failed reload
 *      leaves credentials unchanged and returns kCredentialLoadFailed.
 *   4. Stream aborts propagate kStreamAborted to all in-flight RPC handlers on
 *      that stream; handlers MUST NOT access stream state after receiving it.
 *   5. All method dispatch operations are O(1) hash-map lookups; unknown
 *      methods produce kMethodNotFound, never a crash.
 *
 * ## §Error Taxonomy
 *
 * | Code  | Constant               | Meaning                                           |
 * |-------|------------------------|---------------------------------------------------|
 * | 0     | kSuccess               | Operation completed without error                 |
 * | 8300  | kServerNotRunning      | RPC server is not in Active state                 |
 * | 8301  | kServiceRegistration   | Service registration conflict or failure          |
 * | 8302  | kCredentialLoadFailed  | TLS/mTLS certificate or key load failed           |
 * | 8303  | kStreamAborted         | gRPC stream was aborted by transport or peer      |
 * | 8304  | kMethodNotFound        | Requested RPC method is not registered            |
 * | 8305  | kTransportError        | Underlying network or transport error             |
 * | 8306  | kInternalError         | Unclassified internal error; always deny          |
 *
 * ## §Threading Guarantees
 *
 * - Server lifecycle methods (Start/Stop) are not thread-safe; callers must
 *   serialise them externally.
 * - Service dispatch is thread-safe and lock-free after registration is
 *   complete.
 * - Credential reload acquires a short read-write lock; concurrent RPC calls
 *   proceed on the old credentials until the lock is released.
 *
 * ## §Contract Freeze
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/rpc_grpc/ROADMAP.md — Phase 1 item
 */

#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace themis {
namespace rpc_grpc {

// ============================================================================
// § 1  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the rpc_grpc module.
 *
 * All gRPC server/plugin operations return or throw with one of these codes.
 * Values are in the reserved range [8300, 8399].
 */
enum class RpcGrpcError : int32_t {
    kSuccess              = 0,
    kServerNotRunning     = 8300, ///< Server is not in Active state.
    kServiceRegistration  = 8301, ///< Service registration conflict or failure.
    kCredentialLoadFailed = 8302, ///< TLS/mTLS certificate or key load failed.
    kStreamAborted        = 8303, ///< gRPC stream aborted by transport or peer.
    kMethodNotFound       = 8304, ///< Requested RPC method is not registered.
    kTransportError       = 8305, ///< Underlying network or transport error.
    kInternalError        = 8306, ///< Unclassified internal error.
};

// ============================================================================
// § 2  Server lifecycle states
// ============================================================================

/**
 * @brief gRPC server lifecycle states.
 *
 * Valid forward transitions: Stopped → Starting → Active → Stopping → Stopped.
 */
enum class RpcServerState : int32_t {
    Stopped  = 0, ///< Server is not running and not accepting connections.
    Starting = 1, ///< Server start in progress (transient).
    Active   = 2, ///< Server running and accepting requests.
    Stopping = 3, ///< Server shutdown in progress (transient).
};

// ============================================================================
// § 3  Sizing and timing constraints
// ============================================================================

/// Default gRPC keepalive time (ping interval when idle).
inline constexpr std::chrono::seconds kDefaultKeepaliveTime{30};

/// Default maximum receive message size in bytes (4 MiB).
inline constexpr std::size_t kDefaultMaxReceiveMessageBytes = 4u * 1024u * 1024u;

/// Maximum service name length in bytes.
inline constexpr std::size_t kMaxServiceNameBytes = 256;

/// Maximum RPC method name length in bytes.
inline constexpr std::size_t kMaxMethodNameBytes = 512;

// ============================================================================
// § 4  Supporting struct — service registration descriptor
// ============================================================================

/**
 * @brief Descriptor for a gRPC service being registered on a running server.
 */
struct RpcServiceDescriptor {
    std::string service_name;   ///< Fully-qualified gRPC service name.
    std::string proto_file;     ///< Path to .proto file (informational; may be empty).
    bool        require_auth{true}; ///< Whether the service requires authenticated callers.
    uint32_t    max_concurrent_streams{100}; ///< Per-service concurrent stream cap.
};

// ============================================================================
// § 5  Fail-closed contract
// ============================================================================

/**
 * @brief Returns true when the given error mandates fail-closed denial.
 */
[[nodiscard]] inline constexpr bool isRpcGrpcFailClosed(RpcGrpcError e) noexcept {
    return e == RpcGrpcError::kServerNotRunning
        || e == RpcGrpcError::kCredentialLoadFailed
        || e == RpcGrpcError::kInternalError;
}

} // namespace rpc_grpc
} // namespace themis
