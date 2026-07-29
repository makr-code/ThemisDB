// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_rpc_grpc_contract_hardening_focused.cpp
 * @brief Phase 4 focused contract-hardening tests for the rpc_grpc module.
 *
 * Test IDs: RPC-01 through RPC-08
 * No file I/O, no network, deterministic only.
 *
 * @see include/rpc_grpc/rpc_grpc_api_contract.h
 * @see src/rpc_grpc/ROADMAP.md — Phase 4 items
 */

#include "gtest/gtest.h"
#include "rpc_grpc/rpc_grpc_api_contract.h"

#include <cstdint>
#include <set>
#include <string>
#include <type_traits>
#include <utility>

namespace themis {
namespace rpc_grpc {
namespace test {

// Canonical PRNG seed (deterministic, release-pinned).
static constexpr uint32_t kSeed = 42;

// ============================================================================
// RPC-01 — Error code uniqueness
// ============================================================================

TEST(RpcGrpcContractHardening, RPC01_ErrorCodeUniqueness) {
    std::set<int32_t> seen;
    const int32_t codes[] = {
        static_cast<int32_t>(RpcGrpcError::kServerNotRunning),
        static_cast<int32_t>(RpcGrpcError::kServiceRegistration),
        static_cast<int32_t>(RpcGrpcError::kCredentialLoadFailed),
        static_cast<int32_t>(RpcGrpcError::kStreamAborted),
        static_cast<int32_t>(RpcGrpcError::kMethodNotFound),
        static_cast<int32_t>(RpcGrpcError::kTransportError),
        static_cast<int32_t>(RpcGrpcError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_TRUE(seen.insert(c).second) << "Duplicate error code: " << c;
    }
    EXPECT_EQ(seen.size(), 7u);
    (void)kSeed;
}

// ============================================================================
// RPC-02 — Error code range [8300, 8399]
// ============================================================================

TEST(RpcGrpcContractHardening, RPC02_ErrorCodeRange) {
    const int32_t codes[] = {
        static_cast<int32_t>(RpcGrpcError::kServerNotRunning),
        static_cast<int32_t>(RpcGrpcError::kServiceRegistration),
        static_cast<int32_t>(RpcGrpcError::kCredentialLoadFailed),
        static_cast<int32_t>(RpcGrpcError::kStreamAborted),
        static_cast<int32_t>(RpcGrpcError::kMethodNotFound),
        static_cast<int32_t>(RpcGrpcError::kTransportError),
        static_cast<int32_t>(RpcGrpcError::kInternalError),
    };
    for (auto c : codes) {
        EXPECT_GE(c, 8300) << "Code " << c << " below reserved base 8300";
        EXPECT_LE(c, 8399) << "Code " << c << " above reserved max 8399";
    }
}

// ============================================================================
// RPC-03 — Switch dispatch: all cases must be handled
// ============================================================================

TEST(RpcGrpcContractHardening, RPC03_SwitchDispatch) {
    auto describe = [](RpcGrpcError e) -> const char* {
        switch (e) {
            case RpcGrpcError::kSuccess:              return "success";
            case RpcGrpcError::kServerNotRunning:     return "server_not_running";
            case RpcGrpcError::kServiceRegistration:  return "service_registration";
            case RpcGrpcError::kCredentialLoadFailed: return "credential_load_failed";
            case RpcGrpcError::kStreamAborted:        return "stream_aborted";
            case RpcGrpcError::kMethodNotFound:       return "method_not_found";
            case RpcGrpcError::kTransportError:       return "transport_error";
            case RpcGrpcError::kInternalError:        return "internal_error";
        }
        return "unknown";
    };

    EXPECT_STREQ(describe(RpcGrpcError::kSuccess),              "success");
    EXPECT_STREQ(describe(RpcGrpcError::kServerNotRunning),     "server_not_running");
    EXPECT_STREQ(describe(RpcGrpcError::kCredentialLoadFailed), "credential_load_failed");
    EXPECT_STREQ(describe(RpcGrpcError::kInternalError),        "internal_error");
}

// ============================================================================
// RPC-04 — RpcServerState enum values are distinct
// ============================================================================

TEST(RpcGrpcContractHardening, RPC04_ServerStateDistinct) {
    std::set<int32_t> states = {
        static_cast<int32_t>(RpcServerState::Stopped),
        static_cast<int32_t>(RpcServerState::Starting),
        static_cast<int32_t>(RpcServerState::Active),
        static_cast<int32_t>(RpcServerState::Stopping),
    };
    EXPECT_EQ(states.size(), 4u);
}

// ============================================================================
// RPC-05 — RpcServiceDescriptor default values
// ============================================================================

TEST(RpcGrpcContractHardening, RPC05_ServiceDescriptorDefaults) {
    RpcServiceDescriptor desc;
    EXPECT_TRUE(desc.service_name.empty());
    EXPECT_TRUE(desc.proto_file.empty());
    EXPECT_TRUE(desc.require_auth);        // default must be secure (true)
    EXPECT_EQ(desc.max_concurrent_streams, 100u);
}

// ============================================================================
// RPC-06 — Copy semantics for RpcServiceDescriptor
// ============================================================================

TEST(RpcGrpcContractHardening, RPC06_ServiceDescriptorCopy) {
    RpcServiceDescriptor src;
    src.service_name           = "themis.QueryService";
    src.require_auth           = true;
    src.max_concurrent_streams = 50;

    RpcServiceDescriptor copy = src;
    EXPECT_EQ(copy.service_name,           src.service_name);
    EXPECT_EQ(copy.require_auth,           src.require_auth);
    EXPECT_EQ(copy.max_concurrent_streams, src.max_concurrent_streams);
}

// ============================================================================
// RPC-07 — Move semantics for RpcServiceDescriptor
// ============================================================================

TEST(RpcGrpcContractHardening, RPC07_ServiceDescriptorMove) {
    RpcServiceDescriptor src;
    src.service_name = "themis.MoveService";
    src.proto_file   = "/protos/move.proto";

    RpcServiceDescriptor moved = std::move(src);
    EXPECT_EQ(moved.service_name, "themis.MoveService");
    EXPECT_EQ(moved.proto_file,   "/protos/move.proto");
}

// ============================================================================
// RPC-08 — isRpcGrpcFailClosed predicate
// ============================================================================

TEST(RpcGrpcContractHardening, RPC08_FailClosedPredicate) {
    // Must be fail-closed.
    EXPECT_TRUE(isRpcGrpcFailClosed(RpcGrpcError::kServerNotRunning));
    EXPECT_TRUE(isRpcGrpcFailClosed(RpcGrpcError::kCredentialLoadFailed));
    EXPECT_TRUE(isRpcGrpcFailClosed(RpcGrpcError::kInternalError));

    // Must NOT be fail-closed.
    EXPECT_FALSE(isRpcGrpcFailClosed(RpcGrpcError::kSuccess));
    EXPECT_FALSE(isRpcGrpcFailClosed(RpcGrpcError::kServiceRegistration));
    EXPECT_FALSE(isRpcGrpcFailClosed(RpcGrpcError::kStreamAborted));
    EXPECT_FALSE(isRpcGrpcFailClosed(RpcGrpcError::kMethodNotFound));
    EXPECT_FALSE(isRpcGrpcFailClosed(RpcGrpcError::kTransportError));
}

} // namespace test
} // namespace rpc_grpc
} // namespace themis
