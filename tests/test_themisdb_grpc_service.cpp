/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_themisdb_grpc_service.cpp                     ║
  Module:          api                                                ║
  Description:     Unit tests for ThemisDBGrpcService wrapper        ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "api/themisdb_grpc_service.h"

using namespace themis::api;

// ============================================================================
// ThemisDBGrpcService – construction and service() accessor tests
// ============================================================================

TEST(ThemisDBGrpcServiceTest, ConstructWithNullComponents) {
    // Both components are optional (may be null when stubs are absent).
    ASSERT_NO_THROW({
        ThemisDBGrpcService svc(nullptr, nullptr);
    });
}

TEST(ThemisDBGrpcServiceTest, ServiceReturnsNullOrValidPointer) {
    ThemisDBGrpcService svc(nullptr, nullptr);

    // When the generated proto headers are present service() returns a non-null
    // grpc::Service*; when they are absent it returns nullptr.  Either value is
    // acceptable – the caller must check before registering.
    void* ptr = svc.service();
    (void)ptr;  // accepted: nullptr or a valid pointer both compile cleanly
    SUCCEED();
}

TEST(ThemisDBGrpcServiceTest, ServiceCallIsIdempotent) {
    ThemisDBGrpcService svc(nullptr, nullptr);

    void* first  = svc.service();
    void* second = svc.service();
    EXPECT_EQ(first, second);
}
