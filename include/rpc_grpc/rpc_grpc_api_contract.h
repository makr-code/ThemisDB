/**
 * @file rpc_grpc_api_contract.h
 * @brief Design contract and shared defaults for the rpc_grpc module.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Defines contract types and shared defaults for the rpc_grpc module. This is
 * not the IRPCServer interface implemented by src/rpc_grpc/grpc_plugin.cpp;
 * consult that runtime API and implementation for behavior not explicitly
 * wired to this header.
 *
 * ## §API Contracts
 *
 * Intended behavioral invariants:
 *   1. A gRPC server may not accept requests before StartServer() completes
 *      successfully; any call in kStopped state yields kServerNotRunning.
 *   2. The current runtime registers externally owned grpc::Service pointers
 *      before start; null, late, and duplicate-pointer registrations are
 *      rejected or ignored. No service-count limit is enforced.
 *   3. TLS reload validates new material before stopping the listener, then
 *      restarts it with the new files. Failure to start with new material
 *      triggers a restart attempt with the previous configuration; reload
 *      returns true only when the new listener starts. The operation has
 *      downtime, and rollback failure leaves the listener stopped.
 *   4. Stream aborts propagate kStreamAborted to all in-flight RPC handlers on
 *      that stream; handlers MUST NOT access stream state after receiving it.
 *   5. A service dispatcher should reject unknown methods with
 *      kMethodNotFound rather than crashing.
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
 * - TLS reload is a stop/start operation, not an atomic in-place credential
 *   swap. Callers must serialize it with other lifecycle operations.
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
#include <cstddef>
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

/// Default maximum receive message size in bytes (100 MiB).
inline constexpr std::size_t kDefaultMaxReceiveMessageBytes = 100u * 1024u * 1024u;

/// Default maximum send message size in bytes (100 MiB).
inline constexpr std::size_t kDefaultMaxSendMessageBytes = 100u * 1024u * 1024u;

/// Maximum service name length in bytes.
inline constexpr std::size_t kMaxServiceNameBytes = 256;

/// Maximum RPC method name length in bytes.
inline constexpr std::size_t kMaxMethodNameBytes = 512;

// ============================================================================
// § 4  Supporting struct — service registration descriptor
// ============================================================================

/**
 * @brief Reference metadata for a gRPC service registration.
 *
 * This descriptor is not consumed by the current GRPCServer::registerService
 * implementation; its fields do not enforce service or stream limits.
 */
struct RpcServiceDescriptor {
    std::string service_name;   ///< Fully-qualified gRPC service name.
    std::string proto_file;     ///< Path to .proto file (informational; may be empty).
    bool        require_auth{true}; ///< Whether the service requires authenticated callers.
    uint32_t    max_concurrent_streams{100}; ///< Intended metadata; not enforced by GRPCServer.
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
