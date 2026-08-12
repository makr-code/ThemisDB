/**
 * @file test_themis_core_grpc_service_bridge.cpp
 * @brief Unit tests for ThemisCoreServiceImpl service-instance bridge (STUB #58).
 *
 * Tests verify strict fail-closed behavior in non-proto builds: constructor
 * throws without a callback, accepts a non-null injected pointer, and throws
 * when callback execution fails or returns nullptr.
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
// Without any injected accessor construction must fail closed.
TEST(ThemisCoreServiceImplBridgeTest, NoAccessorThrows) {
    ThemisCoreServiceImpl::setServiceInstanceFn({});   // ensure clean state
    EXPECT_THROW((void)makeService(), std::runtime_error);
    ThemisCoreServiceImpl::setServiceInstanceFn({});   // cleanup
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
// If the accessor callback throws, construction must fail.
TEST(ThemisCoreServiceImplBridgeTest, AccessorExceptionThrows) {
    ThemisCoreServiceImpl::setServiceInstanceFn([]() -> void* {
        throw std::runtime_error("simulated accessor failure");
    });
    EXPECT_THROW((void)makeService(), std::runtime_error);
    ThemisCoreServiceImpl::setServiceInstanceFn({});   // cleanup
}

// If the accessor callback returns nullptr, construction must fail.
TEST(ThemisCoreServiceImplBridgeTest, NullAccessorResultThrows) {
    ThemisCoreServiceImpl::setServiceInstanceFn([]() -> void* {
        return nullptr;
    });
    EXPECT_THROW((void)makeService(), std::runtime_error);
    ThemisCoreServiceImpl::setServiceInstanceFn({});   // cleanup
}
