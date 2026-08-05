// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_metadata_security_rbac_diagnostics_focused.cpp
 * @brief Phase C2: RBAC diagnostics tests for IMetadataSecurityProvider.
 * @note Test IDs: MCH-SEC01..MCH-SEC04
 *
 *   MCH-SEC01  ADMIN grant implies READ_SCHEMA on the granted resource
 *   MCH-SEC02  revoke() removes a specific grant, leaving others intact
 *   MCH-SEC03  revokeAll() removes all grants for a principal
 *   MCH-SEC04  hasPermission() returns false after revoke (no exception thrown)
 *
 * Self-contained: header-only types, no RocksDB, no network I/O.
 * Canonical PRNG seed: kRbacSeed = 42.
 *
 * @see include/metadata/imetadata_security_provider.h
 * @see src/metadata/ROADMAP.md — Phase C diagnostics items
 */

#include <gtest/gtest.h>

#include "metadata/imetadata_security_provider.h"

#include <string>

using namespace themis::metadata;

namespace {

[[maybe_unused]] static constexpr uint64_t kRbacSeed = 42;

} // anonymous namespace

// ---------------------------------------------------------------------------
// MCH-SEC01: ADMIN grant implies READ_SCHEMA on the granted resource
// ---------------------------------------------------------------------------
TEST(MetadataSecurityRbacDiagnosticsTest, MCHSEC01_AdminGrantImpliesReadSchema) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("ops", MetadataOperation::ADMIN, "*");

    EXPECT_TRUE(sec.hasPermission("ops", MetadataOperation::READ_SCHEMA,      "*"));
    EXPECT_TRUE(sec.hasPermission("ops", MetadataOperation::WRITE_SCHEMA,     "*"));
    EXPECT_TRUE(sec.hasPermission("ops", MetadataOperation::READ_STATISTICS,  "*"));
    EXPECT_TRUE(sec.hasPermission("ops", MetadataOperation::WRITE_STATISTICS, "*"));
}

// ---------------------------------------------------------------------------
// MCH-SEC02: revoke() removes a specific grant, leaving others intact
// ---------------------------------------------------------------------------
TEST(MetadataSecurityRbacDiagnosticsTest, MCHSEC02_RevokeRemovesSpecificGrant) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("analyst", MetadataOperation::READ_SCHEMA,     "*");
    sec.grant("analyst", MetadataOperation::READ_STATISTICS, "*");

    EXPECT_TRUE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA,     "*"));
    EXPECT_TRUE(sec.hasPermission("analyst", MetadataOperation::READ_STATISTICS, "*"));

    sec.revoke("analyst", MetadataOperation::READ_SCHEMA, "*");

    EXPECT_FALSE(sec.hasPermission("analyst", MetadataOperation::READ_SCHEMA, "*"));
    EXPECT_TRUE(sec.hasPermission("analyst", MetadataOperation::READ_STATISTICS, "*"));
}

// ---------------------------------------------------------------------------
// MCH-SEC03: revokeAll() removes all grants for a principal
// ---------------------------------------------------------------------------
TEST(MetadataSecurityRbacDiagnosticsTest, MCHSEC03_RevokeAllClearsAllGrants) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("dba", MetadataOperation::READ_SCHEMA,  "*");
    sec.grant("dba", MetadataOperation::WRITE_SCHEMA, "*");
    sec.grant("dba", MetadataOperation::ADMIN,        "*");

    EXPECT_TRUE(sec.hasPermission("dba", MetadataOperation::READ_SCHEMA,  "*"));

    sec.revokeAll("dba");

    EXPECT_FALSE(sec.hasPermission("dba", MetadataOperation::READ_SCHEMA,  "*"));
    EXPECT_FALSE(sec.hasPermission("dba", MetadataOperation::WRITE_SCHEMA, "*"));
    EXPECT_FALSE(sec.hasPermission("dba", MetadataOperation::ADMIN,        "*"));
}

// ---------------------------------------------------------------------------
// MCH-SEC04: hasPermission() returns false after revoke (no exception thrown)
// ---------------------------------------------------------------------------
TEST(MetadataSecurityRbacDiagnosticsTest, MCHSEC04_HasPermissionFalseAfterRevoke) {
    InMemoryRbacMetadataSecurityProvider sec;
    sec.grant("reporter", MetadataOperation::READ_LINEAGE, "orders");

    EXPECT_TRUE(sec.hasPermission("reporter", MetadataOperation::READ_LINEAGE, "orders"));

    sec.revoke("reporter", MetadataOperation::READ_LINEAGE, "orders");

    // hasPermission() must not throw — returns false
    ASSERT_NO_THROW({
        const bool has = sec.hasPermission(
            "reporter", MetadataOperation::READ_LINEAGE, "orders");
        EXPECT_FALSE(has);
    });
}
