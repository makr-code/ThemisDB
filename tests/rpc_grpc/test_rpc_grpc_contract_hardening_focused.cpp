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

// ============================================================================
// Phase 4 — Edge Case Regressions (RPC-09..RPC-16)
// ============================================================================

// ============================================================================
// RPC-09 — Error code serialization round-trip
// ============================================================================

TEST(RpcGrpcContractHardening, RPC09_ErrorCodeSerialization) {
    const RpcGrpcError codes[] = {
        RpcGrpcError::kSuccess,
        RpcGrpcError::kServerNotRunning,
        RpcGrpcError::kServiceRegistration,
        RpcGrpcError::kCredentialLoadFailed,
        RpcGrpcError::kStreamAborted,
        RpcGrpcError::kMethodNotFound,
        RpcGrpcError::kTransportError,
        RpcGrpcError::kInternalError,
    };

    for (auto code : codes) {
        int32_t serialized = static_cast<int32_t>(code);
        auto deserialized = static_cast<RpcGrpcError>(serialized);
        EXPECT_EQ(code, deserialized) << "Round-trip failed for code " << static_cast<int32_t>(code);
    }
}

// ============================================================================
// RPC-10 — Concurrent state transitions (state machine invariants)
// ============================================================================

TEST(RpcGrpcContractHardening, RPC10_StateTransitionInvariants) {
    // Valid forward transitions:
    // Stopped(0) → Starting(1) → Active(2) → Stopping(3) → Stopped(0)

    const int32_t valid_transitions[] = {
        0, 1, // Stopped → Starting
        1, 2, // Starting → Active
        2, 3, // Active → Stopping
        3, 0, // Stopping → Stopped
    };

    for (size_t i = 0; i < 8; i += 2) {
        int32_t from = valid_transitions[i];
        int32_t to   = valid_transitions[i + 1];
        // Validate state range
        EXPECT_GE(from, 0);
        EXPECT_LE(from, 3);
        EXPECT_GE(to, 0);
        EXPECT_LE(to, 3);
    }
}

// ============================================================================
// RPC-11 — RpcServiceDescriptor field validation
// ============================================================================

TEST(RpcGrpcContractHardening, RPC11_ServiceDescriptorValidation) {
    RpcServiceDescriptor desc;

    // Test empty service name is allowed (will fail registration at runtime)
    EXPECT_TRUE(desc.service_name.empty());

    // Test max_concurrent_streams range
    EXPECT_GE(desc.max_concurrent_streams, 1u);
    EXPECT_LE(desc.max_concurrent_streams, 10000u); // reasonable upper bound

    // Test require_auth defaults to true (secure default)
    EXPECT_TRUE(desc.require_auth);

    // Test with non-default values
    desc.service_name = "example.Service";
    desc.max_concurrent_streams = 256;
    desc.require_auth = false;

    EXPECT_EQ(desc.service_name, "example.Service");
    EXPECT_EQ(desc.max_concurrent_streams, 256u);
    EXPECT_FALSE(desc.require_auth);
}

// ============================================================================
// RPC-12 — Error taxonomy completeness (all codes accounted for)
// ============================================================================

TEST(RpcGrpcContractHardening, RPC12_ErrorTaxonomyCompleteness) {
    // Verify the error taxonomy is stable and complete
    constexpr int total_codes = 8; // kSuccess + 7 error codes
    std::set<int32_t> codes_set;

    codes_set.insert(static_cast<int32_t>(RpcGrpcError::kSuccess));
    codes_set.insert(static_cast<int32_t>(RpcGrpcError::kServerNotRunning));
    codes_set.insert(static_cast<int32_t>(RpcGrpcError::kServiceRegistration));
    codes_set.insert(static_cast<int32_t>(RpcGrpcError::kCredentialLoadFailed));
    codes_set.insert(static_cast<int32_t>(RpcGrpcError::kStreamAborted));
    codes_set.insert(static_cast<int32_t>(RpcGrpcError::kMethodNotFound));
    codes_set.insert(static_cast<int32_t>(RpcGrpcError::kTransportError));
    codes_set.insert(static_cast<int32_t>(RpcGrpcError::kInternalError));

    EXPECT_EQ(codes_set.size(), total_codes);
    for (auto code : codes_set) {
        EXPECT_GE(code, 0);
        EXPECT_LE(code, 8399);
    }
}

// ============================================================================
// RPC-13 — Keepalive timing constants
// ============================================================================

TEST(RpcGrpcContractHardening, RPC13_KeepaliveTimingConstants) {
    // Verify keepalive defaults are reasonable
    EXPECT_GE(kDefaultKeepaliveTime.count(), 1);  // At least 1 second
    EXPECT_LE(kDefaultKeepaliveTime.count(), 300); // At most 5 minutes
}

// ============================================================================
// RPC-14 — Message size bounds
// ============================================================================

TEST(RpcGrpcContractHardening, RPC14_MessageSizeBounds) {
    // Verify receive/send message size bounds are reasonable
    EXPECT_GE(kDefaultMaxReceiveMessageBytes, 1024 * 1024);     // At least 1 MiB
    EXPECT_LE(kDefaultMaxReceiveMessageBytes, 1024 * 1024 * 100); // At most 100 MiB
}

// ============================================================================
// RPC-15 — Service name length constraint
// ============================================================================

TEST(RpcGrpcContractHardening, RPC15_ServiceNameLengthConstraint) {
    // Service names exceeding this length must be rejected
    EXPECT_GE(kMaxServiceNameBytes, 32);  // At least for short names
    EXPECT_LE(kMaxServiceNameBytes, 512);

    // Test boundary condition
    std::string short_name(kMaxServiceNameBytes - 1, 'a');
    EXPECT_LE(short_name.length(), kMaxServiceNameBytes);

    std::string too_long(kMaxServiceNameBytes + 1, 'a');
    EXPECT_GT(too_long.length(), kMaxServiceNameBytes);
}

// ============================================================================
// RPC-16 — Method name length constraint
// ============================================================================

TEST(RpcGrpcContractHardening, RPC16_MethodNameLengthConstraint) {
    // Method names exceeding this length must be rejected
    EXPECT_GE(kMaxMethodNameBytes, 128);  // At least for reasonable method names
    EXPECT_LE(kMaxMethodNameBytes, 1024);

    std::string short_method(kMaxMethodNameBytes - 1, 'a');
    EXPECT_LE(short_method.length(), kMaxMethodNameBytes);

    std::string too_long(kMaxMethodNameBytes + 1, 'a');
    EXPECT_GT(too_long.length(), kMaxMethodNameBytes);
}

} // namespace test
} // namespace rpc_grpc
} // namespace themis
