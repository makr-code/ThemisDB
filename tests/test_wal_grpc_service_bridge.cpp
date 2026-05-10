/**
 * @file test_wal_grpc_service_bridge.cpp
 * @brief Unit tests for WalGrpcService service-fn bridge (STUB #49).
 *
 * Tests verify that the ServiceFn injection mechanism works correctly in
 * non-proto builds: null by default (fail-closed), picks up an injected
 * pointer, and silently clamps to nullptr on callback exceptions.
 *
 * Tests: WAL-GRPC-BRIDGE-01..03
 *
 * Note: WAL_GRPC_STUB must be allowed (THEMIS_ALLOW_WAL_GRPC_STUB=1) so that
 * the constructor does not throw when proto stubs are absent.
 */

#include <gtest/gtest.h>
#include "server/wal_grpc_service.h"
#include <cstdlib>
#include <cstdint>
#include <stdexcept>

using themis::server::WalGrpcService;

namespace {

void setEnvVar(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

void unsetEnvVar(const char* name) {
#ifdef _WIN32
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

} // namespace

// Allow the non-proto constructor to succeed in these tests.
class WalGrpcServiceBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure the production-mode guard does not trip in the test environment.
        setEnvVar("THEMIS_ALLOW_WAL_GRPC_STUB", "1");
    }
    void TearDown() override {
        WalGrpcService::setServiceFn({});   // always clean global state
        unsetEnvVar("THEMIS_ALLOW_WAL_GRPC_STUB");
    }

    static WalGrpcService makeService() {
        return WalGrpcService(nullptr);
    }
};

// ── WAL-GRPC-BRIDGE-01 ─────────────────────────────────────────────────────
// Without any injected ServiceFn the service pointer must be null (fail-closed).
TEST_F(WalGrpcServiceBridgeTest, NoServiceFnReturnsNull) {
    WalGrpcService::setServiceFn({});   // ensure clean state
    auto svc = makeService();
    EXPECT_EQ(svc.service(), nullptr);
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
// If the ServiceFn throws, the service pointer must remain null (fail-closed).
TEST_F(WalGrpcServiceBridgeTest, ServiceFnExceptionFailsClosed) {
    WalGrpcService::setServiceFn([]() -> void* {
        throw std::runtime_error("simulated wal-grpc service-fn failure");
    });

    EXPECT_NO_THROW({
        auto svc = makeService();
        EXPECT_EQ(svc.service(), nullptr);
    });
}
