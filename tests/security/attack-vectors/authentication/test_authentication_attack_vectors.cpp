/**
 * @file test_authentication_attack_vectors.cpp
 * @brief Authentication and authorization attack vector tests.
 *
 * Systematically validates that the RBAC, zero-trust, and access-control
 * subsystems correctly reject unauthorized access attempts.
 *
 * Attack categories covered:
 *   - Privilege escalation (unprivileged user claiming admin roles)
 *   - Permission boundary violations (role with limited scope exceeds it)
 *   - Deny-by-default verification (no roles → all resources denied)
 *   - Wildcard role abuse (role with "*:read" must not allow "*:write")
 *   - Role confusion: deleted role must not grant access
 *   - Lateral movement: valid role on wrong resource must deny
 *   - Empty / null identity must be denied
 *   - Role inheritance abuse: inherited role must not exceed parent permissions
 *
 * CWE mapping:
 *   CWE-269 – Improper Privilege Management
 *   CWE-284 – Improper Access Control
 *   CWE-285 – Improper Authorization
 *   CWE-286 – Incorrect User Management
 *
 * OWASP ASVS:
 *   V4.1  – General Access Control Design
 *   V4.2  – Operation Level Access Control
 *   V4.3  – Other Access Control Considerations
 *
 * Compliance:
 *   NIST SP 800-53 AC-3 – Access Enforcement
 *   NIST SP 800-53 AC-6 – Least Privilege
 */

#include <gtest/gtest.h>
#include "security/rbac.h"
#include "themis/runtime_license_gate.h"

#include <string>
#include <vector>
#include <optional>

using namespace themis::security;

// ─── Helpers ──────────────────────────────────────────────────────────────

/**
 * @brief Build a minimal RBAC instance with a "readonly" role and an
 *        "admin" role for use in multiple tests.
 */
static std::unique_ptr<RBAC> build_test_rbac() {
    RBACConfig cfg;
    cfg.config_path       = "";   // no file; use programmatic roles
    cfg.use_builtin_roles = false;

    auto rbac = std::make_unique<RBAC>(cfg);

    // readonly: can only read "data"
    Role readonly_role;
    readonly_role.name        = "readonly";
    readonly_role.description = "Read-only access to data collection";
    readonly_role.permissions = {{"data", "read"}};
    rbac->addRole(readonly_role);

    // operator: can read+write "data"
    Role operator_role;
    operator_role.name        = "operator";
    operator_role.description = "Operator — read/write data";
    operator_role.permissions = {{"data", "read"}, {"data", "write"}};
    rbac->addRole(operator_role);

    // admin: all permissions via wildcard
    Role admin_role;
    admin_role.name        = "admin";
    admin_role.description = "Administrator";
    admin_role.permissions = {{"*", "*"}};
    rbac->addRole(admin_role);

    return rbac;
}

// ─── Test Fixture ─────────────────────────────────────────────────────────

class AuthAttackVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::string license_error;
        rbac_feature_available_ =
            themis::license::RuntimeLicenseGate::instance().isFeatureAllowed("rbac", license_error);
        rbac_ = build_test_rbac();
    }

    std::unique_ptr<RBAC> rbac_;
    bool rbac_feature_available_{false};
};

// ============================================================================
// Positive tests: legitimate access must be granted
// ============================================================================

TEST_F(AuthAttackVectorTest, Positive_AdminCanDoEverything) {
    if (rbac_feature_available_) {
        EXPECT_TRUE(rbac_->checkPermission({"admin"}, "data",   "read"));
        EXPECT_TRUE(rbac_->checkPermission({"admin"}, "data",   "write"));
        EXPECT_TRUE(rbac_->checkPermission({"admin"}, "keys",   "rotate"));
        EXPECT_TRUE(rbac_->checkPermission({"admin"}, "config", "write"));
        EXPECT_TRUE(rbac_->checkPermission({"admin"}, "audit",  "read"));
    } else {
        EXPECT_FALSE(rbac_->checkPermission({"admin"}, "data",   "read"));
        EXPECT_FALSE(rbac_->checkPermission({"admin"}, "data",   "write"));
        EXPECT_FALSE(rbac_->checkPermission({"admin"}, "keys",   "rotate"));
        EXPECT_FALSE(rbac_->checkPermission({"admin"}, "config", "write"));
        EXPECT_FALSE(rbac_->checkPermission({"admin"}, "audit",  "read"));
    }
}

TEST_F(AuthAttackVectorTest, Positive_ReadonlyCanReadData) {
    if (rbac_feature_available_) {
        EXPECT_TRUE(rbac_->checkPermission({"readonly"}, "data", "read"));
    } else {
        EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "data", "read"));
    }
}

TEST_F(AuthAttackVectorTest, Positive_OperatorCanReadAndWriteData) {
    if (rbac_feature_available_) {
        EXPECT_TRUE(rbac_->checkPermission({"operator"}, "data", "read"));
        EXPECT_TRUE(rbac_->checkPermission({"operator"}, "data", "write"));
    } else {
        EXPECT_FALSE(rbac_->checkPermission({"operator"}, "data", "read"));
        EXPECT_FALSE(rbac_->checkPermission({"operator"}, "data", "write"));
    }
}

// ============================================================================
// Attack Vector: Privilege Escalation
// A user claims roles they do not actually hold.
// ============================================================================

/**
 * @brief The readonly role must not grant write access to data — even when
 *        an attacker claims to have admin privileges in their request.
 *        The RBAC check uses the authoritative role list, not a user-supplied one.
 */
TEST_F(AuthAttackVectorTest, Attack_PrivilegeEscalation_ReadonlyClaimsAdmin) {
    // Simulates an attacker submitting a token with role=["admin"] while
    // the authoritative store only grants them "readonly".
    // The test verifies that "readonly" alone must not pass admin-level checks.
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "keys",   "rotate"))
        << "readonly role must not rotate keys (admin-only operation)";
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "config", "write"))
        << "readonly role must not write config";
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "audit",  "delete"))
        << "readonly role must not delete audit logs";
}

/**
 * @brief A user with no roles must be denied access to all resources
 *        (deny-by-default, zero-trust principle).
 */
TEST_F(AuthAttackVectorTest, Attack_DenyByDefault_EmptyRoleList) {
    EXPECT_FALSE(rbac_->checkPermission({}, "data",   "read"))
        << "User with no roles must be denied read on data";
    EXPECT_FALSE(rbac_->checkPermission({}, "data",   "write"))
        << "User with no roles must be denied write on data";
    EXPECT_FALSE(rbac_->checkPermission({}, "config", "read"))
        << "User with no roles must be denied read on config";
}

// ============================================================================
// Attack Vector: Permission Boundary Violation
// A role with limited scope attempts to exceed it.
// ============================================================================

/**
 * @brief readonly can read "data" but must NOT be able to write it or
 *        access any other resource type.
 */
TEST_F(AuthAttackVectorTest, Attack_PermissionBoundary_ReadonlyCannotWrite) {
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "data", "write"))
        << "readonly role must not be able to write to data";
}

TEST_F(AuthAttackVectorTest, Attack_PermissionBoundary_ReadonlyCannotDeleteData) {
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "data", "delete"))
        << "readonly role must not be able to delete data";
}

TEST_F(AuthAttackVectorTest, Attack_PermissionBoundary_OperatorCannotRotateKeys) {
    EXPECT_FALSE(rbac_->checkPermission({"operator"}, "keys", "rotate"))
        << "operator role must not rotate encryption keys (requires admin)";
}

TEST_F(AuthAttackVectorTest, Attack_PermissionBoundary_OperatorCannotAccessAudit) {
    EXPECT_FALSE(rbac_->checkPermission({"operator"}, "audit", "delete"))
        << "operator role must not delete audit logs";
}

// ============================================================================
// Attack Vector: Lateral Movement
// A valid role for resource A is used to access resource B.
// ============================================================================

/**
 * @brief The "readonly" role grants access to "data" but must NOT grant
 *        access to "keys", "config", or "audit" resources even for "read".
 */
TEST_F(AuthAttackVectorTest, Attack_LateralMovement_ReadonlyCannotReadKeys) {
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "keys", "read"))
        << "readonly role must not read encryption keys (lateral movement to key store)";
}

TEST_F(AuthAttackVectorTest, Attack_LateralMovement_ReadonlyCannotReadConfig) {
    EXPECT_FALSE(rbac_->checkPermission({"readonly"}, "config", "read"))
        << "readonly role must not read system config (lateral movement)";
}

// ============================================================================
// Attack Vector: Deleted Role Must Not Grant Access
// Removing a role from the registry must immediately revoke its permissions.
// ============================================================================

TEST_F(AuthAttackVectorTest, Attack_DeletedRole_AccessRevoked) {
    // First confirm operator has access.
    if (rbac_feature_available_) {
        ASSERT_TRUE(rbac_->checkPermission({"operator"}, "data", "write"));
    } else {
        ASSERT_FALSE(rbac_->checkPermission({"operator"}, "data", "write"));
    }

    // Remove the "operator" role.
    rbac_->removeRole("operator");

    // After removal, checkPermission with a removed role name must fail.
    EXPECT_FALSE(rbac_->checkPermission({"operator"}, "data", "write"))
        << "Deleted role must no longer grant permissions";
}

// ============================================================================
// Attack Vector: Non-Existent Role
// A role name that was never registered must deny all access.
// ============================================================================

TEST_F(AuthAttackVectorTest, Attack_NonExistentRole_DeniedAll) {
    EXPECT_FALSE(rbac_->checkPermission({"phantom_role"}, "data", "read"))
        << "Unknown role must be denied all access (deny by default)";
    EXPECT_FALSE(rbac_->checkPermission({"phantom_role"}, "data", "write"))
        << "Unknown role must be denied write access";
    EXPECT_FALSE(rbac_->checkPermission({"phantom_role"}, "*",    "*"))
        << "Unknown role must not grant wildcard access";
}

// ============================================================================
// Attack Vector: Role Injection via Special Characters
// Attackers embed special characters in role names to confuse matching.
// ============================================================================

TEST_F(AuthAttackVectorTest, Attack_RoleInjection_SemicolonInRoleName) {
    EXPECT_FALSE(rbac_->checkPermission({"admin;readonly"}, "data", "read"))
        << "Semicolon-separated role string must not be treated as two roles";
}

TEST_F(AuthAttackVectorTest, Attack_RoleInjection_NullByteInRoleName) {
    std::string null_role = std::string("admin\0readonly", 14);
    EXPECT_FALSE(rbac_->checkPermission({null_role}, "keys", "rotate"))
        << "Null-byte injection in role name must not grant admin access";
}

TEST_F(AuthAttackVectorTest, Attack_RoleInjection_WildcardRoleName) {
    EXPECT_FALSE(rbac_->checkPermission({"*"}, "data", "read"))
        << "Wildcard '*' as a role name must not match all registered roles";
}

// ============================================================================
// Attack Vector: Multiple-Role Combinations
// ============================================================================

/**
 * @brief Two non-admin roles combined must not exceed the union of their
 *        individual permissions.  Neither "readonly" nor "operator" has
 *        key-rotation rights; having both must not grant it.
 */
TEST_F(AuthAttackVectorTest, Attack_MultiRole_CombinedRolesNoEscalation) {
    EXPECT_FALSE(rbac_->checkPermission({"readonly", "operator"}, "keys", "rotate"))
        << "readonly + operator combined must not escalate to key-rotation privilege";
}

/**
 * @brief Combining a valid role with a non-existent role must not elevate
 *        permissions beyond the valid role.
 */
TEST_F(AuthAttackVectorTest, Attack_MultiRole_NonExistentRoleNoEscalation) {
    // "readonly" can read data; adding "ghost_role" must not add write rights.
    if (rbac_feature_available_) {
        EXPECT_TRUE(rbac_->checkPermission({"readonly", "ghost_role"}, "data", "read"))
            << "Valid role right must still be granted when combined with unknown role";
    } else {
        EXPECT_FALSE(rbac_->checkPermission({"readonly", "ghost_role"}, "data", "read"));
    }
    EXPECT_FALSE(rbac_->checkPermission({"readonly", "ghost_role"}, "data", "write"))
        << "Unknown role must not escalate permissions beyond the valid role";
}

// ============================================================================
// Attack Vector: Role Lookup Integrity
// ============================================================================

TEST_F(AuthAttackVectorTest, Integrity_RoleExistsAfterAdd) {
    Role custom;
    custom.name        = "auditor";
    custom.description = "Audit log reader";
    custom.permissions = {{"audit", "read"}};
    rbac_->addRole(custom);

    auto found = rbac_->getRole("auditor");
    ASSERT_TRUE(found.has_value()) << "Newly added role must be retrievable";
    EXPECT_EQ(found->name, "auditor");
    EXPECT_EQ(found->permissions.size(), 1u);
    EXPECT_EQ(found->permissions[0].resource, "audit");
    EXPECT_EQ(found->permissions[0].action,   "read");
}

TEST_F(AuthAttackVectorTest, Integrity_RoleAbsentAfterRemove) {
    rbac_->removeRole("readonly");
    EXPECT_FALSE(rbac_->getRole("readonly").has_value())
        << "Removed role must not be found in the role registry";
}
