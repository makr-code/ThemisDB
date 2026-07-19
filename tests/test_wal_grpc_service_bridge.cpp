/*
 * ThemisDB | File: test_wal_grpc_service_bridge.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 91/100
 * Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_wal_grpc_service_bridge.cpp
 * @brief Unit tests for WalGrpcService service-fn bridge (STUB #49).
 *
 * Tests verify strict fail-closed behavior in non-proto builds: constructor
 * throws without a callback, accepts a non-null injected pointer, and throws
 * when callback execution fails or returns nullptr.
 *
 * Tests: WAL-GRPC-BRIDGE-01..03
 *
 */

#include <gtest/gtest.h>
#include "server/wal_grpc_service.h"
#include <cstdint>
#include <stdexcept>

using themis::server::WalGrpcService;

class WalGrpcServiceBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        WalGrpcService::setServiceFn({});   // always clean global state
    }

    static WalGrpcService makeService() {
        return WalGrpcService(nullptr);
    }
};

// ── WAL-GRPC-BRIDGE-01 ─────────────────────────────────────────────────────
// Without any injected ServiceFn construction must fail closed.
TEST_F(WalGrpcServiceBridgeTest, NoServiceFnThrows) {
    WalGrpcService::setServiceFn({});   // ensure clean state
    EXPECT_THROW((void)makeService(), std::runtime_error);
}

// ── WAL-GRPC-BRIDGE-02 ─────────────────────────────────────────────────────
// When a ServiceFn is registered the constructor picks up the returned pointer.
TEST_F(WalGrpcServiceBridgeTest, InjectedPointerIsReturned) {
    void* expected = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xBEEF));
    WalGrpcService::setServiceFn([expected]() { return expected; });

    auto svc = makeService();
    EXPECT_EQ(svc.service(), expected);
}

// ── WAL-GRPC-BRIDGE-03 ─────────────────────────────────────────────────────
// If the ServiceFn throws, construction must fail.
TEST_F(WalGrpcServiceBridgeTest, ServiceFnExceptionThrows) {
    WalGrpcService::setServiceFn([]() -> void* {
        throw std::runtime_error("simulated wal-grpc service-fn failure");
    });
    EXPECT_THROW((void)makeService(), std::runtime_error);
}

// If the ServiceFn returns nullptr, construction must fail.
TEST_F(WalGrpcServiceBridgeTest, NullServiceFnResultThrows) {
    WalGrpcService::setServiceFn([]() -> void* {
        return nullptr;
    });
    EXPECT_THROW((void)makeService(), std::runtime_error);
}
