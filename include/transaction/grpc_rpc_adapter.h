/**
 * @file grpc_rpc_adapter.h
 * @brief gRPC transport adapters for the distributed 2PC/3PC RPC bridges.
 *
 * Provides `GrpcRpcPhase1Adapter` and `GrpcRpcPhase2Adapter`, which produce
 * `DistributedTransactionManager::RpcPhase1Fn` / `RpcPhase2Fn` callables that
 * drive Phase-1 PREPARE and Phase-2 COMMIT/ABORT over a real gRPC channel.
 *
 * ## Architecture note — W9 Block 2 (W9-7..W9-9)
 *
 * The `DistributedTransactionManager` exposes two static injection points:
 *   - `setRpcPhase1Fn(fn)` — called per remote participant during Phase-1
 *   - `setRpcPhase2Fn(fn)` — called per remote participant during Phase-2
 *
 * These adapters fulfil those injection points with real gRPC calls instead of
 * the in-process simulation fallback.  All gRPC code is conditionally compiled
 * behind `THEMIS_HAS_CORE_GRPC` so the translation unit compiles cleanly in
 * test/headless builds that do not link gRPC.
 *
 * ## Proto service used
 *
 * `ThemisCoreService` (proto/themis_core.proto) is the only distributed service
 * currently proto-compiled in this repository.  Because no transaction-specific
 * `Prepare` RPC exists yet, Phase-1 PREPARE is proxied via `BeginTransaction`
 * (isolation=SERIALIZABLE, timeout=prepare_timeout_ms, options["2pc_prepare"]="1").
 * A `success=true` response → COMMIT vote; any failure, timeout, or
 * `success=false` → ABORT vote.  This is documented as a bridge architecture
 * that a dedicated `Prepare` RPC should replace when the service schema is
 * extended.
 *
 * Phase-2 maps directly to `CommitTransaction` / `RollbackTransaction`.
 *
 * @version 0.0.2
 * @note Maturity: 🟡 BETA — wired, tested in-process; pending real gRPC CI lane
 * @note Wave: Wave 9 Block 2 (W9-7..W9-9); mTLS added Wave 10 (W10-A)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "transaction/distributed_transaction_manager.h"

#include <chrono>
#include <map>
#include <optional>
#include <string>

namespace themis::transaction {

// ─────────────────────────────────────────────────────────────────────────────
// W10-A — mTLS credential configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Mutual-TLS credential bundle for gRPC channel creation.
 *
 * This path is fail-closed: the adapter will only create a secure gRPC channel
 * when all three PEM fields are present and valid. Missing material is rejected
 * unless `allow_insecure` is explicitly set for a local test-only override.
 *
 * @note For production deployments populate from files or a secret manager —
 *       never hard-code PEM material in source code.
 */
struct MtlsConfig {
    /// PEM-encoded root CA certificate used to verify the server's certificate.
    std::string ca_cert_pem;
    /// PEM-encoded client certificate presented to the server.
    std::string client_cert_pem;
    /// PEM-encoded client private key corresponding to @p client_cert_pem.
    std::string client_key_pem;
    /**
     * @brief Optional TLS server-name override.
     *
     * When non-empty this is set via `grpc::ChannelArguments::SetSslTargetNameOverride`.
     * Useful in test environments where the server certificate CN does not
     * match the dial address (e.g. `"localhost"` vs `"127.0.0.1"`).
     */
    std::string target_name_override;
    /**
     * @brief Allow the explicit development/test-only insecure fallback.
     *
     * This must remain false in production. The distributed transaction path
     * treats any other usage as a hard error to avoid silent trust degradation.
     */
    bool allow_insecure = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// W9-7 — Phase-1 PREPARE gRPC adapter
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Factory for a `RpcPhase1Fn` that sends Phase-1 PREPARE over gRPC.
 *
 * Usage:
 * @code
 * #ifdef THEMIS_HAS_CORE_GRPC
 *     DistributedTransactionManager::setRpcPhase1Fn(
 *         GrpcRpcPhase1Adapter::make(node_addresses, 500ms));
 * #endif
 * @endcode
 *
 * When `THEMIS_HAS_CORE_GRPC` is not defined, `make()` returns a callable that
 * always votes ABORT and logs a warning, so test/headless builds fail-closed.
 */
class GrpcRpcPhase1Adapter {
public:
    /**
     * @brief Create a `RpcPhase1Fn` callable.
     *
     * @param node_addresses  Map from node_id → "host:port".  Unknown node_ids
     *                        vote ABORT.
     * @param timeout         gRPC deadline applied to every PREPARE call.
     * @param mtls            Optional mTLS credential bundle.  When present and
     *                        all three PEM fields are non-empty, the channel is
     *                        created with `grpc::SslCredentials`.  Missing PEM
     *                        material is rejected unless `allow_insecure` is
     *                        explicitly set for a local test override.
     * @return                Callable compatible with
     *                        `DistributedTransactionManager::RpcPhase1Fn`.
     */
    [[nodiscard]] static DistributedTransactionManager::RpcPhase1Fn make(
        const std::map<std::string, std::string>& node_addresses,
        std::chrono::milliseconds                  timeout,
        std::optional<MtlsConfig>                  mtls = std::nullopt);
};

// ─────────────────────────────────────────────────────────────────────────────
// W9-8 — Phase-2 COMMIT/ABORT gRPC adapter
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Factory for a `RpcPhase2Fn` that sends Phase-2 COMMIT/ROLLBACK over
 *        gRPC, with exponential-backoff retry (3 attempts: 100 ms / 200 ms /
 *        400 ms).
 *
 * Usage:
 * @code
 * #ifdef THEMIS_HAS_CORE_GRPC
 *     DistributedTransactionManager::setRpcPhase2Fn(
 *         GrpcRpcPhase2Adapter::make(node_addresses, 2000ms));
 * #endif
 * @endcode
 *
 * The callable throws `std::runtime_error` after all retries are exhausted so
 * the coordinator's `runPhase2Unlocked()` can surface the partial-failure to
 * its WAL and caller.
 */
class GrpcRpcPhase2Adapter {
public:
    /**
     * @brief Create a `RpcPhase2Fn` callable.
     *
     * @param node_addresses  Map from node_id → "host:port".
     * @param timeout         Per-attempt gRPC deadline.
     * @param mtls            Optional mTLS credential bundle (see
     *                        `GrpcRpcPhase1Adapter::make` for semantics).
     * @return                Callable compatible with
     *                        `DistributedTransactionManager::RpcPhase2Fn`.
     */
    [[nodiscard]] static DistributedTransactionManager::RpcPhase2Fn make(
        const std::map<std::string, std::string>& node_addresses,
        std::chrono::milliseconds                  timeout,
        std::optional<MtlsConfig>                  mtls = std::nullopt);
};

}  // namespace themis::transaction
