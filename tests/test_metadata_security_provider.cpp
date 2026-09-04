/**
 * Test: Metadata Security Provider
 *
 * Tests for IMetadataSecurityProvider / NoOpMetadataSecurityProvider /
 * InMemoryRbacMetadataSecurityProvider:
 *
 * Acceptance criteria:
 *   AC-SEC-1  NoOpMetadataSecurityProvider::hasPermission always returns true
 *   AC-SEC-2  NoOpMetadataSecurityProvider::assertPermission never throws
 *   AC-SEC-3  RBAC: unknown principal is denied for every operation
 *   AC-SEC-4  RBAC: grant() permits the exact (principal, op, resource) triple
 *   AC-SEC-5  RBAC: wildcard resource "*" permits op on any resource
 *   AC-SEC-6  RBAC: ADMIN operation implies all other ops on all resources
 *   AC-SEC-7  RBAC: revoke() removes a previously granted permission
 *   AC-SEC-8  RBAC: revokeAll() removes all permissions for a principal
 *   AC-SEC-9  RBAC: assertPermission() throws MetadataAccessDeniedException
 *   AC-SEC-10 RBAC: grant/hasPermission are thread-safe under concurrent access
 *   AC-SEC-11 Polymorphic usage via IMetadataSecurityProvider*
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "metadata/imetadata_security_provider.h"

#include <atomic>
#include <thread>
#include <vector>

using namespace themis::metadata;

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-1/2 — NoOpMetadataSecurityProvider
// ─────────────────────────────────────────────────────────────────────────────

TEST(NoOpMetadataSecurityProviderTest, HasPermissionAlwaysTrue) {
    NoOpMetadataSecurityProvider sec;
    EXPECT_TRUE(sec.hasPermission("alice", MetadataOperation::READ_SCHEMA, "orders"));
    EXPECT_TRUE(sec.hasPermission("bob",   MetadataOperation::WRITE_SCHEMA, "*"));
    EXPECT_TRUE(sec.hasPermission("",      MetadataOperation::ADMIN,        ""));
}

TEST(NoOpMetadataSecurityProviderTest, AssertPermissionNeverThrows) {
    NoOpMetadataSecurityProvider sec;
    EXPECT_NO_THROW(sec.assertPermission("alice", MetadataOperation::READ_SCHEMA, "orders"));
    EXPECT_NO_THROW(sec.assertPermission("",      MetadataOperation::ADMIN,       "*"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-3 — Unknown principal is denied
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryRbacMetadataSecurityProviderTest, UnknownPrincipalDenied) {
    InMemoryRbacMetadataSecurityProvider sec;
    EXPECT_FALSE(sec.hasPermission("stranger", MetadataOperation::READ_SCHEMA, "orders"));
}

TEST(InMemoryRbacMetadataSecurityProviderTest, UnknownPrincipalAllOperationsDenied) {
    InMemoryRbacMetadataSecurityProvider sec;
    // No grants at all
    EXPECT_FALSE(sec.hasPermission("x", MetadataOperation::READ_SCHEMA,    "t"));
    EXPECT_FALSE(sec.hasPermission("x", MetadataOperation::WRITE_SCHEMA,   "t"));
    EXPECT_FALSE(sec.hasPermission("x", MetadataOperation::READ_STATISTICS,"t"));
    EXPECT_FALSE(sec.hasPermission("x", MetadataOperation::WRITE_LINEAGE,  "t"));
    EXPECT_FALSE(sec.hasPermission("x", MetadataOperation::READ_AUDIT_LOG, "t"));
    EXPECT_FALSE(sec.hasPermission("x", MetadataOperation::ADMIN,          "t"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-4 — Exact (principal, op, resource) grant
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryRbacMetadataSecurityProviderTest, GrantExactTriplePermitted) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("analyst", MetadataOperation::READ_SCHEMA, "orders");

    EXPECT_TRUE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA, "orders"));
    // Other op on same resource — denied
    EXPECT_FALSE(sec.hasPermission("analyst", MetadataOperation::WRITE_SCHEMA, "orders"));
    // Same op on different resource — denied
    EXPECT_FALSE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA, "users"));
    // Different principal — denied
    EXPECT_FALSE(sec.hasPermission("bob",     MetadataOperation::READ_SCHEMA, "orders"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-5 — Wildcard resource "*"
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryRbacMetadataSecurityProviderTest, WildcardResourceGrantsAllResources) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("analyst", MetadataOperation::READ_SCHEMA, "*");

    EXPECT_TRUE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA, "orders"));
    EXPECT_TRUE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA, "users"));
    EXPECT_TRUE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA, "any_table"));
    // Different op — still denied
    EXPECT_FALSE(sec.hasPermission("analyst", MetadataOperation::WRITE_SCHEMA, "orders"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-6 — ADMIN implies all operations
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryRbacMetadataSecurityProviderTest, AdminImpliesAllOperations) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("superuser", MetadataOperation::ADMIN, "*");

    EXPECT_TRUE(sec.hasPermission("superuser", MetadataOperation::READ_SCHEMA,    "t"));
    EXPECT_TRUE(sec.hasPermission("superuser", MetadataOperation::WRITE_SCHEMA,   "t"));
    EXPECT_TRUE(sec.hasPermission("superuser", MetadataOperation::READ_STATISTICS,"t"));
    EXPECT_TRUE(sec.hasPermission("superuser", MetadataOperation::WRITE_STATISTICS,"t"));
    EXPECT_TRUE(sec.hasPermission("superuser", MetadataOperation::READ_LINEAGE,   "t"));
    EXPECT_TRUE(sec.hasPermission("superuser", MetadataOperation::WRITE_LINEAGE,  "t"));
    EXPECT_TRUE(sec.hasPermission("superuser", MetadataOperation::READ_AUDIT_LOG, "t"));
    EXPECT_TRUE(sec.hasPermission("superuser", MetadataOperation::ADMIN,          "t"));
}

TEST(InMemoryRbacMetadataSecurityProviderTest, AdminOnSpecificResourceImpliesAllOpsOnThatResource) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("restricted_admin", MetadataOperation::ADMIN, "orders");

    EXPECT_TRUE(sec.hasPermission("restricted_admin", MetadataOperation::READ_SCHEMA,  "orders"));
    EXPECT_TRUE(sec.hasPermission("restricted_admin", MetadataOperation::WRITE_SCHEMA, "orders"));
    // Different resource — not covered by ADMIN on "orders"
    EXPECT_FALSE(sec.hasPermission("restricted_admin", MetadataOperation::READ_SCHEMA, "users"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-7 — revoke()
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryRbacMetadataSecurityProviderTest, RevokeRemovesPermission) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("analyst", MetadataOperation::READ_SCHEMA, "orders");
    EXPECT_TRUE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA, "orders"));

    sec.revoke("analyst", MetadataOperation::READ_SCHEMA, "orders");
    EXPECT_FALSE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA, "orders"));
}

TEST(InMemoryRbacMetadataSecurityProviderTest, RevokeUnknownIsNoOp) {
    InMemoryRbacMetadataSecurityProvider sec;
    // Revoking a permission that was never granted must not crash
    EXPECT_NO_THROW(sec.revoke("nobody", MetadataOperation::READ_SCHEMA, "t"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-8 — revokeAll()
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryRbacMetadataSecurityProviderTest, RevokeAllRemovesAllPermissions) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("analyst", MetadataOperation::READ_SCHEMA,    "*");
    sec.grant("analyst", MetadataOperation::READ_STATISTICS,"*");

    sec.revokeAll("analyst");

    EXPECT_FALSE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA,    "t"));
    EXPECT_FALSE(sec.hasPermission("analyst", MetadataOperation::READ_STATISTICS,"t"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-9 — assertPermission() throws on denial
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryRbacMetadataSecurityProviderTest, AssertPermissionThrowsWhenDenied) {
    InMemoryRbacMetadataSecurityProvider sec;

    EXPECT_THROW(
        sec.assertPermission("stranger", MetadataOperation::READ_SCHEMA, "orders"),
        MetadataAccessDeniedException);
}

TEST(InMemoryRbacMetadataSecurityProviderTest, AssertPermissionDoesNotThrowWhenGranted) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("analyst", MetadataOperation::READ_SCHEMA, "orders");

    EXPECT_NO_THROW(
        sec.assertPermission("analyst", MetadataOperation::READ_SCHEMA, "orders"));
}

TEST(InMemoryRbacMetadataSecurityProviderTest, AccessDeniedExceptionCarriesPrincipalAndResource) {
    InMemoryRbacMetadataSecurityProvider sec;
    try {
        sec.assertPermission("alice", MetadataOperation::WRITE_SCHEMA, "orders");
        FAIL() << "Expected MetadataAccessDeniedException";
    } catch (const MetadataAccessDeniedException& ex) {
        EXPECT_EQ(ex.principal(), "alice");
        EXPECT_EQ(ex.operation(), MetadataOperation::WRITE_SCHEMA);
        EXPECT_EQ(ex.resource(),  "orders");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-10 — Thread-safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryRbacMetadataSecurityProviderTest, ConcurrentGrantAndCheck) {
    InMemoryRbacMetadataSecurityProvider sec;
    constexpr int kIter    = 200;
    constexpr int kThreads = 4;

    std::atomic<int> allowed{0};
    std::atomic<int> denied{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads + 1);

    // Writer thread: continuously grants and revokes
    threads.emplace_back([&] {
        for (int i = 0; i < kIter; ++i) {
            sec.grant("user", MetadataOperation::READ_SCHEMA, "*");
            sec.revoke("user", MetadataOperation::READ_SCHEMA, "*");
        }
    });

    // Reader threads: check permission concurrently
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kIter; ++i) {
                if (sec.hasPermission("user", MetadataOperation::READ_SCHEMA, "t")) {
                    ++allowed;
                } else {
                    ++denied;
                }
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    // No crash; allowed + denied == kThreads * kIter
    EXPECT_EQ(allowed.load() + denied.load(), kThreads * kIter);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SEC-11 — Polymorphic usage
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSecurityProviderPolymorphismTest, NoOpViaInterface) {
    std::unique_ptr<IMetadataSecurityProvider> sec =
        std::make_unique<NoOpMetadataSecurityProvider>();
    EXPECT_TRUE(sec->hasPermission("alice", MetadataOperation::WRITE_SCHEMA, "t"));
    EXPECT_NO_THROW(sec->assertPermission("alice", MetadataOperation::ADMIN, "t"));
}

TEST(MetadataSecurityProviderPolymorphismTest, RbacViaInterface) {
    std::unique_ptr<IMetadataSecurityProvider> sec =
        std::make_unique<InMemoryRbacMetadataSecurityProvider>();

    auto* rbac = dynamic_cast<InMemoryRbacMetadataSecurityProvider*>(sec.get());
    ASSERT_NE(rbac, nullptr);
    rbac->grant("alice", MetadataOperation::READ_SCHEMA, "*");

    EXPECT_TRUE(sec->hasPermission("alice", MetadataOperation::READ_SCHEMA, "orders"));
    EXPECT_THROW(sec->assertPermission("alice", MetadataOperation::WRITE_SCHEMA, "orders"),
                 MetadataAccessDeniedException);
}
