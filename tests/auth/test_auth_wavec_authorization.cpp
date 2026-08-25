/**
 * @file test_auth_wavec_authorization.cpp
 * @brief Wave C unit tests — AUTH-AuthZ-01 through AUTH-AuthZ-08
 *
 * Validates the ABAC policy contract types declared in
 * auth/authorization_policy.h.  These data structures form the input
 * contract for OPA/Ranger policy adapters; correctness here ensures
 * downstream evaluation is always given well-formed arguments.
 *
 * Test IDs: AUTH-AuthZ-01 … AUTH-AuthZ-08
 */

#include <gtest/gtest.h>

#include "auth/authorization_policy.h"

#include <string>
#include <vector>

namespace themis {
namespace auth {
namespace tests {

// ---------------------------------------------------------------------------
// AUTH-AuthZ-01: PolicyDecision default construction uses NOT_APPLICABLE
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-AuthZ-01 — A default-constructed PolicyEvaluationResult must
 *        carry PolicyDecision::NOT_APPLICABLE so that a policy with no
 *        opinion does not accidentally ALLOW or DENY.
 */
TEST(AuthorizationPolicyTest, AUTH_AuthZ_01_PolicyDecisionDefaultIsNotApplicable) {
    PolicyEvaluationResult result;

    EXPECT_EQ(result.decision, PolicyDecision::NOT_APPLICABLE)
        << "Default PolicyEvaluationResult.decision must be NOT_APPLICABLE";
}

// ---------------------------------------------------------------------------
// AUTH-AuthZ-02: PolicyDecision ALLOW carries a reason string
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-AuthZ-02 — A PolicyEvaluationResult can be constructed with
 *        ALLOW and an accompanying reason string; both must round-trip.
 */
TEST(AuthorizationPolicyTest, AUTH_AuthZ_02_PolicyDecisionAllowCarriesReason) {
    PolicyEvaluationResult result;
    result.decision  = PolicyDecision::ALLOW;
    result.reason    = "user holds required role";
    result.policy_id = "policy-001";

    EXPECT_EQ(result.decision,  PolicyDecision::ALLOW);
    EXPECT_EQ(result.reason,    "user holds required role");
    EXPECT_EQ(result.policy_id, "policy-001");
}

// ---------------------------------------------------------------------------
// AUTH-AuthZ-03: SubjectAttributes default role is empty string
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-AuthZ-03 — A default-constructed SubjectAttributes must have
 *        an empty role field.  Policy rules that depend on role membership
 *        must not accidentally match against an uninitialised value.
 */
TEST(AuthorizationPolicyTest, AUTH_AuthZ_03_SubjectAttributesDefaultRoleIsEmpty) {
    SubjectAttributes subject;

    EXPECT_TRUE(subject.role.empty())
        << "Default SubjectAttributes.role must be an empty string";
}

// ---------------------------------------------------------------------------
// AUTH-AuthZ-04: ResourceAttributes resource_type is settable
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-AuthZ-04 — resource_type in ResourceAttributes must accept
 *        arbitrary string values and preserve them without truncation.
 */
TEST(AuthorizationPolicyTest, AUTH_AuthZ_04_ResourceAttributesResourceTypeIsSettable) {
    ResourceAttributes resource;
    resource.resource_type = "document";

    EXPECT_EQ(resource.resource_type, "document");

    resource.resource_type = "collection";
    EXPECT_EQ(resource.resource_type, "collection");
}

// ---------------------------------------------------------------------------
// AUTH-AuthZ-05: EnvironmentAttributes is_mfa_verified defaults to false
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-AuthZ-05 — is_mfa_verified in EnvironmentAttributes must
 *        default to false so that policies requiring MFA deny by default
 *        when no environment context is supplied.
 */
TEST(AuthorizationPolicyTest, AUTH_AuthZ_05_EnvironmentAttributesMfaDefaultFalse) {
    EnvironmentAttributes env;

    EXPECT_FALSE(env.is_mfa_verified)
        << "Default EnvironmentAttributes.is_mfa_verified must be false";
}

// ---------------------------------------------------------------------------
// AUTH-AuthZ-06: SubjectAttributes groups vector starts empty
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-AuthZ-06 — A default-constructed SubjectAttributes must have
 *        an empty groups vector.  Group-membership policies must not fire
 *        against uninitialised data.
 */
TEST(AuthorizationPolicyTest, AUTH_AuthZ_06_SubjectAttributesGroupsStartEmpty) {
    SubjectAttributes subject;

    EXPECT_TRUE(subject.groups.empty())
        << "Default SubjectAttributes.groups must be an empty vector";
}

// ---------------------------------------------------------------------------
// AUTH-AuthZ-07: PolicyDecision DENY with reason set
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-AuthZ-07 — A PolicyEvaluationResult set to DENY must preserve
 *        both the decision and the associated reason string for audit logging.
 */
TEST(AuthorizationPolicyTest, AUTH_AuthZ_07_PolicyDecisionDenyHasReason) {
    PolicyEvaluationResult result;
    result.decision = PolicyDecision::DENY;
    result.reason   = "classification level exceeds clearance";

    EXPECT_EQ(result.decision, PolicyDecision::DENY);
    EXPECT_FALSE(result.reason.empty());
    EXPECT_EQ(result.reason, "classification level exceeds clearance");
}

// ---------------------------------------------------------------------------
// AUTH-AuthZ-08: ResourceAttributes classification is settable
// ---------------------------------------------------------------------------

/**
 * @brief AUTH-AuthZ-08 — The classification field in ResourceAttributes must
 *        accept standard data sensitivity labels and preserve them correctly.
 */
TEST(AuthorizationPolicyTest, AUTH_AuthZ_08_ResourceAttributesClassificationIsSettable) {
    ResourceAttributes resource;

    const std::vector<std::string> labels = {
        "public", "internal", "confidential", "secret", "top_secret"
    };

    for (const auto& label : labels) {
        resource.classification = label;
        EXPECT_EQ(resource.classification, label)
            << "classification label mismatch for: " << label;
    }
}

// ---------------------------------------------------------------------------
// Bonus structural checks (not individually numbered but support AUTH-AuthZ tests)
// ---------------------------------------------------------------------------

/**
 * @brief Verifies SubjectAttributes tenant_id and clearance_level are settable.
 */
TEST(AuthorizationPolicyTest, SubjectAttributesTenantAndClearanceAreSettable) {
    SubjectAttributes subject;
    subject.tenant_id      = "tenant-42";
    subject.clearance_level = "secret";

    EXPECT_EQ(subject.tenant_id,       "tenant-42");
    EXPECT_EQ(subject.clearance_level, "secret");
}

/**
 * @brief Verifies PolicyEvaluationResult applicable_policies list is settable.
 */
TEST(AuthorizationPolicyTest, PolicyEvaluationResultApplicablePoliciesAreSettable) {
    PolicyEvaluationResult result;
    result.applicable_policies = {"policy-a", "policy-b"};

    ASSERT_EQ(result.applicable_policies.size(), 2u);
    EXPECT_EQ(result.applicable_policies[0], "policy-a");
    EXPECT_EQ(result.applicable_policies[1], "policy-b");
}

} // namespace tests
} // namespace auth
} // namespace themis
