/**
 * @file grpc_rpc_adapter.cpp
 * @brief gRPC transport adapters for distributed 2PC/3PC RPC bridges.
 *
 * ## Bridge architecture (W9-7..W9-9)
 *
 * Phase-1 PREPARE is proxied via `ThemisCoreService::BeginTransaction` with
 * `options["2pc_prepare"] = "1"` and isolation=SERIALIZABLE.  This is a
 * deliberate bridge choice: the existing proto does not have a dedicated
 * `Prepare` RPC.  Once the service schema is extended with a first-class
 * `PrepareTransaction` RPC, this adapter should be updated to call it.
 *
 * Phase-2 maps directly to `CommitTransaction` / `RollbackTransaction`.
 *
 * All gRPC code lives inside `#ifdef THEMIS_HAS_CORE_GRPC` blocks.  When the
 * flag is absent (e.g. unit-test / headless builds) both adapters return
 * fail-closed callables that log a warning and vote ABORT / throw, preventing
 * silent data loss.
 *
 * @version 0.0.1
 * @note Wave: Wave 9 Block 2 (W9-7..W9-9)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "transaction/grpc_rpc_adapter.h"
#include "utils/logger.h"

#include <chrono>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>

// ── gRPC-specific includes (only when the gRPC transport is compiled in) ──────
#ifdef THEMIS_HAS_CORE_GRPC
#include <grpcpp/grpcpp.h>
// Generated stub header (produced by protoc from proto/themis_core.proto)
#include "themis_core.grpc.pb.h"
#endif  // THEMIS_HAS_CORE_GRPC

namespace themis::transaction {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Milliseconds capped at the given deadline as a system_clock time_point.
inline std::chrono::system_clock::time_point makeDeadline(
    std::chrono::milliseconds ms) noexcept
{
    return std::chrono::system_clock::now() + ms;
}

/// Simple exponential-backoff delays for Phase-2 retries.
constexpr std::chrono::milliseconds kRetryDelays[3] = {
    std::chrono::milliseconds{100},
    std::chrono::milliseconds{200},
    std::chrono::milliseconds{400},
};

constexpr int kMaxRetries = 3;

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// W9-7 — GrpcRpcPhase1Adapter::make()
// ─────────────────────────────────────────────────────────────────────────────

DistributedTransactionManager::RpcPhase1Fn GrpcRpcPhase1Adapter::make(
    const std::map<std::string, std::string>& node_addresses,
    std::chrono::milliseconds                  timeout)
{
#ifdef THEMIS_HAS_CORE_GRPC
    // Capture by value so the returned callable is self-contained and safe to
    // call from any thread after the creator scope has exited.
    return [node_addresses, timeout](
               const std::string& node_id,
               const std::string& txn_id,
               const std::set<std::string>& /*affected_keys*/) -> bool
    {
        auto it = node_addresses.find(node_id);
        if (it == node_addresses.end()) {
            THEMIS_WARN("GrpcRpcPhase1Adapter: no address for node_id={} txn={} — voting ABORT",
                        node_id, txn_id);
            return false;
        }

        const std::string& endpoint = it->second;

        // Create an insecure channel.  Production deployments MUST pass mTLS
        // credentials here; replace `InsecureChannelCredentials()` with the
        // appropriate `SslCredentials` / `local_credentials` configuration.
        auto channel = grpc::CreateChannel(endpoint,
                                           grpc::InsecureChannelCredentials());
        auto stub    = themis::core::ThemisCoreService::NewStub(channel);

        grpc::ClientContext ctx;
        ctx.set_deadline(makeDeadline(timeout));

        // Phase-1 PREPARE proxied via BeginTransaction.
        // options["2pc_prepare"]="1" signals to the remote node that this
        // Begin is actually a Phase-1 PREPARE, not a regular transaction open.
        // This is the bridge architecture: a dedicated PrepareTransaction RPC
        // should replace this when the proto schema is extended.
        themis::core::BeginTransactionRequest req;
        req.set_isolation_level(
            themis::core::BeginTransactionRequest::SERIALIZABLE);
        req.set_timeout_ms(static_cast<int32_t>(timeout.count()));
        (*req.mutable_options())["2pc_prepare"]    = "1";
        (*req.mutable_options())["2pc_txn_id"]     = txn_id;
        (*req.mutable_options())["2pc_coordinator"] = "local";

        themis::core::BeginTransactionResponse resp;
        grpc::Status status = stub->BeginTransaction(&ctx, req, &resp);

        if (!status.ok()) {
            THEMIS_WARN("GrpcRpcPhase1Adapter: node={} txn={} gRPC error [{}/{}] — voting ABORT",
                        node_id, txn_id,
                        static_cast<int>(status.error_code()),
                        status.error_message());
            return false;
        }

        if (!resp.success()) {
            THEMIS_WARN("GrpcRpcPhase1Adapter: node={} txn={} remote BeginTransaction success=false "
                        "(error: {}) — voting ABORT",
                        node_id, txn_id,
                        resp.has_error() ? resp.error().message() : "(none)");
            return false;
        }

        THEMIS_INFO("GrpcRpcPhase1Adapter: node={} txn={} Phase-1 PREPARE → COMMIT vote "
                    "(remote_txn_id={})",
                    node_id, txn_id, resp.transaction_id());
        return true;
    };

#else  // THEMIS_HAS_CORE_GRPC not defined — fail-closed stub

    (void)node_addresses;
    (void)timeout;

    return [](const std::string& node_id,
              const std::string& txn_id,
              const std::set<std::string>&) -> bool
    {
        THEMIS_WARN("GrpcRpcPhase1Adapter: THEMIS_HAS_CORE_GRPC is not defined — "
                    "node={} txn={} voting ABORT (no gRPC transport available)",
                    node_id, txn_id);
        return false;
    };

#endif  // THEMIS_HAS_CORE_GRPC
}

// ─────────────────────────────────────────────────────────────────────────────
// W9-8 — GrpcRpcPhase2Adapter::make()
// ─────────────────────────────────────────────────────────────────────────────

DistributedTransactionManager::RpcPhase2Fn GrpcRpcPhase2Adapter::make(
    const std::map<std::string, std::string>& node_addresses,
    std::chrono::milliseconds                  timeout)
{
#ifdef THEMIS_HAS_CORE_GRPC

    return [node_addresses, timeout](
               const std::string& node_id,
               const std::string& txn_id,
               bool               do_commit)
    {
        auto it = node_addresses.find(node_id);
        if (it == node_addresses.end()) {
            THEMIS_WARN("GrpcRpcPhase2Adapter: no address for node_id={} txn={} {} — skipping",
                        node_id, txn_id, do_commit ? "COMMIT" : "ROLLBACK");
            throw std::runtime_error(
                "GrpcRpcPhase2Adapter: unknown node_id=" + node_id);
        }

        const std::string& endpoint = it->second;
        const char*        phase2_op = do_commit ? "COMMIT" : "ROLLBACK";

        for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
            // Fresh channel + stub per attempt — channels remain valid across
            // attempts but we re-acquire the deadline each time.
            auto channel = grpc::CreateChannel(endpoint,
                                               grpc::InsecureChannelCredentials());
            auto stub    = themis::core::ThemisCoreService::NewStub(channel);
            grpc::ClientContext ctx;
            ctx.set_deadline(makeDeadline(timeout));

            grpc::Status status;
            bool         remote_success = false;
            std::string  remote_error;

            if (do_commit) {
                themis::core::CommitTransactionRequest req;
                req.set_transaction_id(txn_id);
                themis::core::CommitTransactionResponse resp;
                status         = stub->CommitTransaction(&ctx, req, &resp);
                remote_success = status.ok() && resp.success();
                if (status.ok() && !resp.success() && resp.has_error()) {
                    remote_error = resp.error().message();
                }
            } else {
                themis::core::RollbackTransactionRequest req;
                req.set_transaction_id(txn_id);
                themis::core::RollbackTransactionResponse resp;
                status         = stub->RollbackTransaction(&ctx, req, &resp);
                remote_success = status.ok() && resp.success();
                if (status.ok() && !resp.success() && resp.has_error()) {
                    remote_error = resp.error().message();
                }
            }

            if (remote_success) {
                THEMIS_INFO("GrpcRpcPhase2Adapter: node={} txn={} {} confirmed "
                            "(attempt {}/{})",
                            node_id, txn_id, phase2_op,
                            attempt + 1, kMaxRetries);
                return;  // ── success path ──────────────────────────────────
            }

            const bool is_last = (attempt + 1 == kMaxRetries);
            if (!status.ok()) {
                if (is_last) {
                    THEMIS_ERROR("GrpcRpcPhase2Adapter: node={} txn={} {} gRPC error [{}/{}] "
                                 "on final attempt {}/{} — giving up",
                                 node_id, txn_id, phase2_op,
                                 static_cast<int>(status.error_code()),
                                 status.error_message(),
                                 attempt + 1, kMaxRetries);
                } else {
                    THEMIS_WARN("GrpcRpcPhase2Adapter: node={} txn={} {} gRPC error [{}/{}] "
                                "on attempt {}/{} — retrying in {}ms",
                                node_id, txn_id, phase2_op,
                                static_cast<int>(status.error_code()),
                                status.error_message(),
                                attempt + 1, kMaxRetries,
                                kRetryDelays[attempt].count());
                }
            } else {
                // status.ok() but remote_success == false
                if (is_last) {
                    THEMIS_ERROR("GrpcRpcPhase2Adapter: node={} txn={} {} remote failure "
                                 "(error: {}) on final attempt {}/{} — giving up",
                                 node_id, txn_id, phase2_op, remote_error,
                                 attempt + 1, kMaxRetries);
                } else {
                    THEMIS_WARN("GrpcRpcPhase2Adapter: node={} txn={} {} remote failure "
                                "(error: {}) on attempt {}/{} — retrying in {}ms",
                                node_id, txn_id, phase2_op, remote_error,
                                attempt + 1, kMaxRetries,
                                kRetryDelays[attempt].count());
                }
            }

            if (!is_last) {
                std::this_thread::sleep_for(kRetryDelays[attempt]);
            }
        }

        // All retries exhausted.
        throw std::runtime_error(
            "GrpcRpcPhase2Adapter: Phase-2 " + std::string(phase2_op) +
            " failed for node=" + node_id + " txn=" + txn_id +
            " after " + std::to_string(kMaxRetries) + " attempts");
    };

#else  // THEMIS_HAS_CORE_GRPC not defined — fail-closed stub

    (void)node_addresses;
    (void)timeout;

    return [](const std::string& node_id,
              const std::string& txn_id,
              bool               do_commit)
    {
        THEMIS_WARN("GrpcRpcPhase2Adapter: THEMIS_HAS_CORE_GRPC is not defined — "
                    "node={} txn={} {} cannot be delivered (no gRPC transport)",
                    node_id, txn_id, do_commit ? "COMMIT" : "ROLLBACK");
        throw std::runtime_error(
            "GrpcRpcPhase2Adapter: no gRPC transport (THEMIS_HAS_CORE_GRPC not defined) "
            "for node=" + node_id + " txn=" + txn_id);
    };

#endif  // THEMIS_HAS_CORE_GRPC
}

}  // namespace themis::transaction
