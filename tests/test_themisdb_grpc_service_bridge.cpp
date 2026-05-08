/**
 * @file test_themisdb_grpc_service_bridge.cpp
 * @brief Unit tests for ThemisDBGrpcService service-fn bridge (STUB #59).
 *
 * Tests verify that the ServiceFn injection mechanism works correctly in
 * non-proto builds: null by default (fail-closed), picks up an injected
 * pointer, and silently clamps to nullptr on callback exceptions.
 *
 * Tests: API-GRPC-BRIDGE-01..03
 */

#include <gtest/gtest.h>
#include "api/themisdb_grpc_service.h"
#include <cstdint>
#include <stdexcept>

using themis::api::ThemisDBGrpcService;

// ── Helpers ────────────────────────────────────────────────────────────────

static ThemisDBGrpcService makeService() {
    // Both constructors delegate to buildImpl(); use the simplest form.
    return ThemisDBGrpcService(nullptr, nullptr);
}

// ── API-GRPC-BRIDGE-01 ─────────────────────────────────────────────────────
// Without any injected ServiceFn the service pointer must be null (fail-closed).
TEST(ThemisDBGrpcServiceBridgeTest, NoServiceFnReturnsNull) {
    ThemisDBGrpcService::setServiceFn({});   // ensure clean state
    auto svc = makeService();
    EXPECT_EQ(svc.service(), nullptr);
}

// ── API-GRPC-BRIDGE-02 ─────────────────────────────────────────────────────
// When a ServiceFn is registered the constructor picks up the returned pointer.
TEST(ThemisDBGrpcServiceBridgeTest, InjectedPointerIsReturned) {
    void* expected = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xDEAD));
    ThemisDBGrpcService::setServiceFn([expected]() { return expected; });

    auto svc = makeService();
    EXPECT_EQ(svc.service(), expected);

    ThemisDBGrpcService::setServiceFn({});   // cleanup
}

// ── API-GRPC-BRIDGE-03 ─────────────────────────────────────────────────────
// If the ServiceFn throws, the service pointer must remain null (fail-closed).
TEST(ThemisDBGrpcServiceBridgeTest, ServiceFnExceptionFailsClosed) {
    ThemisDBGrpcService::setServiceFn([]() -> void* {
        throw std::runtime_error("simulated service-fn failure");
    });

    EXPECT_NO_THROW({
        auto svc = makeService();
        EXPECT_EQ(svc.service(), nullptr);
    });

    ThemisDBGrpcService::setServiceFn({});   // cleanup
}
