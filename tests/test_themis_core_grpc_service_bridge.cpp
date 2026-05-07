/**
 * @file test_themis_core_grpc_service_bridge.cpp
 * @brief Unit tests for ThemisCoreServiceImpl service-instance bridge (STUB #58).
 *
 * Tests verify that the ServiceInstanceFn injection mechanism works correctly
 * in non-proto builds: null by default (fail-closed), picks up an injected
 * pointer, and silently clamps to nullptr on callback exceptions.
 *
 * Tests: CORE-GRPC-BRIDGE-01..03
 */

#include <gtest/gtest.h>
#include "server/themis_core_grpc_service.h"
#include <cstdint>
#include <stdexcept>

using themis::core::ThemisCoreServiceImpl;

// ── Helpers ────────────────────────────────────────────────────────────────

static ThemisCoreServiceImpl makeService() {
    return ThemisCoreServiceImpl(nullptr, nullptr, nullptr);
}

// ── CORE-GRPC-BRIDGE-01 ────────────────────────────────────────────────────
// Without any injected accessor the service instance must be null (fail-closed).
TEST(ThemisCoreServiceImplBridgeTest, NoAccessorReturnsNull) {
    ThemisCoreServiceImpl::setServiceInstanceFn({});   // ensure clean state
    auto svc = makeService();
    EXPECT_EQ(svc.getServiceInstance(), nullptr);
}

// ── CORE-GRPC-BRIDGE-02 ────────────────────────────────────────────────────
// When an accessor is registered the constructor picks up the returned pointer.
TEST(ThemisCoreServiceImplBridgeTest, InjectedPointerIsReturned) {
    void* expected = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xABCD));
    ThemisCoreServiceImpl::setServiceInstanceFn([expected]() { return expected; });

    auto svc = makeService();
    EXPECT_EQ(svc.getServiceInstance(), expected);

    ThemisCoreServiceImpl::setServiceInstanceFn({});   // cleanup
}

// ── CORE-GRPC-BRIDGE-03 ────────────────────────────────────────────────────
// If the accessor callback throws, the service pointer must remain null
// (fail-closed, no uncaught exception propagating to the caller).
TEST(ThemisCoreServiceImplBridgeTest, AccessorExceptionFailsClosed) {
    ThemisCoreServiceImpl::setServiceInstanceFn([]() -> void* {
        throw std::runtime_error("simulated accessor failure");
    });

    EXPECT_NO_THROW({
        auto svc = makeService();
        EXPECT_EQ(svc.getServiceInstance(), nullptr);
    });

    ThemisCoreServiceImpl::setServiceInstanceFn({});   // cleanup
}
