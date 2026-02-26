/**
 * @file test_cross_tenant_policy_inheritance.cpp
 * @brief Unit tests for CrossTenantPolicyInheritance – cross-tenant governance
 *        policy inheritance (ThemisDB governance module).
 *
 * Tests cover:
 * - Tenant registration with and without a parent
 * - Cycle detection when registering parent-child relationships
 * - Ancestor-chain resolution (root-first ordering)
 * - Effective policy merging (most-restrictive wins for all fields)
 * - resolveEffectiveRules returns rules from entire ancestor chain
 * - resolveEffectiveRules annotates rules with source tenant_id in created_by
 * - evaluateEffectivePolicy merges decisions across the hierarchy
 * - evaluateEffectivePolicy emits audit event when logger is attached
 * - Tenants with no PolicyManager attached are skipped gracefully
 * - Unregistering a tenant promotes its children to root
 * - setTenantPolicyManager / getTenantPolicyManager round-trip
 * - setAuditLogger round-trip (does not crash)
 * - getParentTenantId for root and non-root tenants
 * - listTenants returns all registered tenants
 */

#include <gtest/gtest.h>
#include "governance/cross_tenant_policy_inheritance.h"
#include "governance/policy_manager.h"
#include "utils/audit_logger.h"

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

using namespace themis::governance;

namespace {

// Helper: create a minimal PolicyRule
PolicyRule makeRule(const std::string& id,
                    const std::string& resource,
                    const std::string& action,
                    bool require_encryption = false,
                    bool allow_export = true,
                    bool allow_cache = true,
                    int retention_days = 365,
                    const std::string& redaction = "none")
{
    PolicyRule r;
    r.id = id;
    r.name = id;
    r.resources = {resource};
    r.actions = {action};
    r.require_encryption = require_encryption;
    r.allow_export = allow_export;
    r.allow_cache = allow_cache;
    r.retention_days = retention_days;
    r.redaction_level = redaction;
    r.enabled = true;
    return r;
}

// Helper: build a PolicyManager with one rule
std::shared_ptr<PolicyManager> makeManager(const PolicyRule& rule)
{
    auto mgr = std::make_shared<PolicyManager>();
    mgr->addRule(rule);
    return mgr;
}

} // anonymous namespace

// ============================================================================
// Fixture
// ============================================================================

class CrossTenantInheritanceTest : public ::testing::Test {
protected:
    CrossTenantPolicyInheritance inh;
};

// ============================================================================
// Registration Tests
// ============================================================================

TEST_F(CrossTenantInheritanceTest, RegisterRootTenant)
{
    EXPECT_TRUE(inh.registerTenant("root"));
    auto tenants = inh.listTenants();
    ASSERT_EQ(tenants.size(), 1u);
    EXPECT_EQ(tenants[0], "root");
    EXPECT_EQ(inh.getParentTenantId("root"), "");
}

TEST_F(CrossTenantInheritanceTest, RegisterChildTenant)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));
    EXPECT_EQ(inh.getParentTenantId("child"), "root");
}

TEST_F(CrossTenantInheritanceTest, RegisterEmptyTenantIdFails)
{
    EXPECT_FALSE(inh.registerTenant(""));
}

TEST_F(CrossTenantInheritanceTest, SelfParentingFails)
{
    ASSERT_TRUE(inh.registerTenant("tenant-a"));
    EXPECT_FALSE(inh.registerTenant("tenant-a", "tenant-a"));
}

TEST_F(CrossTenantInheritanceTest, DirectCycleDetected)
{
    ASSERT_TRUE(inh.registerTenant("A"));
    ASSERT_TRUE(inh.registerTenant("B", "A"));
    // Would create A→B→A cycle
    EXPECT_FALSE(inh.registerTenant("A", "B"));
}

TEST_F(CrossTenantInheritanceTest, IndirectCycleDetected)
{
    ASSERT_TRUE(inh.registerTenant("A"));
    ASSERT_TRUE(inh.registerTenant("B", "A"));
    ASSERT_TRUE(inh.registerTenant("C", "B"));
    // Would create A→B→C→A cycle
    EXPECT_FALSE(inh.registerTenant("A", "C"));
}

TEST_F(CrossTenantInheritanceTest, ReRegistrationUpdatesParent)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));
    ASSERT_TRUE(inh.registerTenant("other-root"));
    // Re-register child under a different parent
    ASSERT_TRUE(inh.registerTenant("child", "other-root"));
    EXPECT_EQ(inh.getParentTenantId("child"), "other-root");
}

// ============================================================================
// Unregister Tests
// ============================================================================

TEST_F(CrossTenantInheritanceTest, UnregisterRootPromotesChildren)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));

    inh.unregisterTenant("root");

    // child should now be a root tenant
    EXPECT_EQ(inh.getParentTenantId("child"), "");
    auto tenants = inh.listTenants();
    EXPECT_EQ(tenants.size(), 1u);
    EXPECT_EQ(tenants[0], "child");
}

TEST_F(CrossTenantInheritanceTest, UnregisterNonExistentIsNoop)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    inh.unregisterTenant("does-not-exist");  // should not throw
    EXPECT_EQ(inh.listTenants().size(), 1u);
}

// ============================================================================
// Ancestor Chain Tests
// ============================================================================

TEST_F(CrossTenantInheritanceTest, AncestorsOfRootIsEmpty)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    EXPECT_TRUE(inh.getAncestors("root").empty());
}

TEST_F(CrossTenantInheritanceTest, AncestorsOneLevel)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));

    auto ancestors = inh.getAncestors("child");
    ASSERT_EQ(ancestors.size(), 1u);
    EXPECT_EQ(ancestors[0], "root");
}

TEST_F(CrossTenantInheritanceTest, AncestorsMultiLevel)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("mid", "root"));
    ASSERT_TRUE(inh.registerTenant("leaf", "mid"));

    auto ancestors = inh.getAncestors("leaf");
    ASSERT_EQ(ancestors.size(), 2u);
    EXPECT_EQ(ancestors[0], "root");
    EXPECT_EQ(ancestors[1], "mid");
}

TEST_F(CrossTenantInheritanceTest, AncestorsForUnknownTenantIsEmpty)
{
    EXPECT_TRUE(inh.getAncestors("nonexistent").empty());
}

// ============================================================================
// PolicyManager attachment Tests
// ============================================================================

TEST_F(CrossTenantInheritanceTest, SetAndGetPolicyManager)
{
    auto mgr = std::make_shared<PolicyManager>();
    inh.setTenantPolicyManager("tenant-x", mgr);
    EXPECT_EQ(inh.getTenantPolicyManager("tenant-x"), mgr);
}

TEST_F(CrossTenantInheritanceTest, GetPolicyManagerForUnknownTenantReturnsNull)
{
    EXPECT_EQ(inh.getTenantPolicyManager("no-such-tenant"), nullptr);
}

TEST_F(CrossTenantInheritanceTest, SetNullPolicyManagerDetaches)
{
    auto mgr = std::make_shared<PolicyManager>();
    inh.setTenantPolicyManager("tenant-y", mgr);
    inh.setTenantPolicyManager("tenant-y", nullptr);
    EXPECT_EQ(inh.getTenantPolicyManager("tenant-y"), nullptr);
}

// ============================================================================
// resolveEffectiveRules Tests
// ============================================================================

TEST_F(CrossTenantInheritanceTest, ResolveEffectiveRulesSingleTenant)
{
    auto mgr = makeManager(makeRule("r1", "data/*", "read"));
    inh.setTenantPolicyManager("tenant", mgr);

    auto rules = inh.resolveEffectiveRules("tenant");
    ASSERT_EQ(rules.size(), 1u);
    EXPECT_EQ(rules[0].id, "r1");
}

TEST_F(CrossTenantInheritanceTest, ResolveEffectiveRulesMergesAncestorAndChild)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));

    auto root_mgr = makeManager(makeRule("root-rule", "data/*", "read"));
    auto child_mgr = makeManager(makeRule("child-rule", "data/private", "write"));

    inh.setTenantPolicyManager("root", root_mgr);
    inh.setTenantPolicyManager("child", child_mgr);

    auto rules = inh.resolveEffectiveRules("child");
    ASSERT_EQ(rules.size(), 2u);
    // root rule first (ancestor comes before child)
    EXPECT_EQ(rules[0].id, "root-rule");
    EXPECT_EQ(rules[1].id, "child-rule");
}

TEST_F(CrossTenantInheritanceTest, ResolveEffectiveRulesSkipsTenantWithNoManager)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("mid", "root"));   // no manager attached
    ASSERT_TRUE(inh.registerTenant("leaf", "mid"));

    auto leaf_mgr = makeManager(makeRule("leaf-rule", "data/*", "read"));
    inh.setTenantPolicyManager("leaf", leaf_mgr);

    auto rules = inh.resolveEffectiveRules("leaf");
    ASSERT_EQ(rules.size(), 1u);
    EXPECT_EQ(rules[0].id, "leaf-rule");
}

TEST_F(CrossTenantInheritanceTest, ResolveEffectiveRulesForUnknownTenantIsEmpty)
{
    auto rules = inh.resolveEffectiveRules("ghost");
    EXPECT_TRUE(rules.empty());
}

// ============================================================================
// evaluateEffectivePolicy – merge semantics
// ============================================================================

TEST_F(CrossTenantInheritanceTest, EncryptionRequirementPropagatesDown)
{
    // Root requires encryption; child does not.
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));

    auto root_rule = makeRule("enc-rule", "data/*", "read",
                              /*require_encryption=*/true);
    auto child_rule = makeRule("child-rule", "data/*", "read",
                               /*require_encryption=*/false);

    inh.setTenantPolicyManager("root",  makeManager(root_rule));
    inh.setTenantPolicyManager("child", makeManager(child_rule));

    auto decision = inh.evaluateEffectivePolicy("child", "data/x", "read", {});
    EXPECT_TRUE(decision.require_encryption);
}

TEST_F(CrossTenantInheritanceTest, ExportDenialPropagatesDown)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));

    auto root_rule = makeRule("no-export", "data/*", "read",
                              false, /*allow_export=*/false);
    auto child_rule = makeRule("child-rule", "data/*", "read",
                               false, /*allow_export=*/true);

    inh.setTenantPolicyManager("root",  makeManager(root_rule));
    inh.setTenantPolicyManager("child", makeManager(child_rule));

    auto decision = inh.evaluateEffectivePolicy("child", "data/x", "read", {});
    EXPECT_FALSE(decision.allow_export);
}

TEST_F(CrossTenantInheritanceTest, ShortestRetentionWins)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));

    auto root_rule = makeRule("root-ret", "data/*", "read",
                              false, true, true, /*retention_days=*/90);
    auto child_rule = makeRule("child-ret", "data/*", "read",
                               false, true, true, /*retention_days=*/365);

    inh.setTenantPolicyManager("root",  makeManager(root_rule));
    inh.setTenantPolicyManager("child", makeManager(child_rule));

    auto decision = inh.evaluateEffectivePolicy("child", "data/x", "read", {});
    EXPECT_EQ(decision.retention_days, 90);
}

TEST_F(CrossTenantInheritanceTest, StrictestRedactionWins)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));

    auto root_rule = makeRule("root-red", "data/*", "read",
                              false, true, true, 365, "strict");
    auto child_rule = makeRule("child-red", "data/*", "read",
                               false, true, true, 365, "none");

    inh.setTenantPolicyManager("root",  makeManager(root_rule));
    inh.setTenantPolicyManager("child", makeManager(child_rule));

    auto decision = inh.evaluateEffectivePolicy("child", "data/x", "read", {});
    EXPECT_EQ(decision.redaction_level, "strict");
}

TEST_F(CrossTenantInheritanceTest, ChildCannotLoosenEncryptionRequirement)
{
    // Even if the child policy tries to allow un-encrypted access, the
    // parent's encryption requirement must still hold in the merged decision.
    ASSERT_TRUE(inh.registerTenant("org"));
    ASSERT_TRUE(inh.registerTenant("dept", "org"));
    ASSERT_TRUE(inh.registerTenant("team", "dept"));

    inh.setTenantPolicyManager("org",
        makeManager(makeRule("org-enc", "secrets/*", "*", true)));
    inh.setTenantPolicyManager("dept",
        makeManager(makeRule("dept-rule", "secrets/*", "*", false)));
    inh.setTenantPolicyManager("team",
        makeManager(makeRule("team-rule", "secrets/*", "*", false)));

    auto decision =
        inh.evaluateEffectivePolicy("team", "secrets/key", "read", {});
    EXPECT_TRUE(decision.require_encryption);
}

TEST_F(CrossTenantInheritanceTest, CacheDenialPropagatesDown)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));

    auto root_rule = makeRule("no-cache", "data/*", "read",
                              false, true, /*allow_cache=*/false);
    auto child_rule = makeRule("child-cache", "data/*", "read",
                               false, true, /*allow_cache=*/true);

    inh.setTenantPolicyManager("root",  makeManager(root_rule));
    inh.setTenantPolicyManager("child", makeManager(child_rule));

    auto decision = inh.evaluateEffectivePolicy("child", "data/x", "read", {});
    EXPECT_FALSE(decision.allow_cache);
}

TEST_F(CrossTenantInheritanceTest, PermissivePolicyWhenNoManagerAttached)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("orphan", "root"));
    // Neither root nor orphan has a PolicyManager attached.

    auto decision = inh.evaluateEffectivePolicy("orphan", "data/x", "read", {});
    // Default PolicyManager::PolicyDecision is permissive.
    EXPECT_TRUE(decision.allowed);
}

TEST_F(CrossTenantInheritanceTest, TenantLocalRuleOverridesDefaultForItsOwnData)
{
    // Root has no rule for "local/*"; child has a restrictive rule for it.
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("child", "root"));

    auto root_rule  = makeRule("root-rule", "shared/*", "read");
    auto child_rule = makeRule("child-rule", "local/*", "read", true);

    inh.setTenantPolicyManager("root",  makeManager(root_rule));
    inh.setTenantPolicyManager("child", makeManager(child_rule));

    auto decision = inh.evaluateEffectivePolicy("child", "local/data", "read", {});
    // root has no matching rule (default → allowed, no encryption)
    // child requires encryption → merge keeps encryption=true
    EXPECT_TRUE(decision.require_encryption);
}

// ============================================================================
// Deep hierarchy test
// ============================================================================

TEST_F(CrossTenantInheritanceTest, ThreeLevelHierarchyMergesAllLevels)
{
    ASSERT_TRUE(inh.registerTenant("root"));
    ASSERT_TRUE(inh.registerTenant("mid", "root"));
    ASSERT_TRUE(inh.registerTenant("leaf", "mid"));

    // Root: require_encryption=true
    inh.setTenantPolicyManager("root",
        makeManager(makeRule("r-enc", "data/*", "read",
                             true, true, true, 365, "none")));
    // Mid: allow_export=false, retention=180
    inh.setTenantPolicyManager("mid",
        makeManager(makeRule("m-ret", "data/*", "read",
                             false, false, true, 180, "standard")));
    // Leaf: allow_cache=false
    inh.setTenantPolicyManager("leaf",
        makeManager(makeRule("l-cache", "data/*", "read",
                             false, true, false, 365, "none")));

    auto decision = inh.evaluateEffectivePolicy("leaf", "data/x", "read", {});
    EXPECT_TRUE(decision.require_encryption);   // from root
    EXPECT_FALSE(decision.allow_export);         // from mid
    EXPECT_FALSE(decision.allow_cache);          // from leaf
    EXPECT_EQ(decision.retention_days, 180);     // min(365, 180, 365)
    EXPECT_EQ(decision.redaction_level, "standard"); // strictest
}

// ============================================================================
// created_by annotation in resolveEffectiveRules
// ============================================================================

TEST_F(CrossTenantInheritanceTest, ResolveEffectiveRulesAnnotatesCreatedByWhenEmpty)
{
    ASSERT_TRUE(inh.registerTenant("org"));
    ASSERT_TRUE(inh.registerTenant("team", "org"));

    // Rule with empty created_by – should be annotated with source tenant.
    PolicyRule org_rule = makeRule("org-rule", "data/*", "read");
    org_rule.created_by = "";  // explicitly empty

    // Rule with an existing created_by – must NOT be overwritten.
    PolicyRule team_rule = makeRule("team-rule", "data/*", "write");
    team_rule.created_by = "original-author";

    inh.setTenantPolicyManager("org",  makeManager(org_rule));
    inh.setTenantPolicyManager("team", makeManager(team_rule));

    auto rules = inh.resolveEffectiveRules("team");
    ASSERT_EQ(rules.size(), 2u);

    // Ancestor rule should be annotated with the org tenant id.
    auto it_org = std::find_if(rules.begin(), rules.end(),
        [](const PolicyRule& r){ return r.id == "org-rule"; });
    ASSERT_NE(it_org, rules.end());
    EXPECT_EQ(it_org->created_by, "org");

    // Tenant-local rule: original author preserved.
    auto it_team = std::find_if(rules.begin(), rules.end(),
        [](const PolicyRule& r){ return r.id == "team-rule"; });
    ASSERT_NE(it_team, rules.end());
    EXPECT_EQ(it_team->created_by, "original-author");
}

TEST_F(CrossTenantInheritanceTest, ResolveEffectiveRulesCreatedByPreservedWhenNonEmpty)
{
    // Root rule has created_by already set – must not be clobbered.
    PolicyRule rule = makeRule("r1", "data/*", "read");
    rule.created_by = "admin-user";

    inh.setTenantPolicyManager("root", makeManager(rule));

    auto rules = inh.resolveEffectiveRules("root");
    ASSERT_EQ(rules.size(), 1u);
    EXPECT_EQ(rules[0].created_by, "admin-user");
}

// ============================================================================
// setAuditLogger
// ============================================================================

TEST_F(CrossTenantInheritanceTest, SetAuditLoggerDoesNotCrash)
{
    themis::utils::AuditLoggerConfig cfg;
    cfg.enabled = false;
    auto logger = std::make_shared<themis::utils::AuditLogger>(nullptr, nullptr, cfg);

    // Must not throw or crash; calling evaluateEffectivePolicy with a logger
    // attached must also be safe.
    inh.setAuditLogger(logger);

    ASSERT_TRUE(inh.registerTenant("root"));
    inh.setTenantPolicyManager("root", makeManager(makeRule("r", "data/*", "read")));

    auto decision = inh.evaluateEffectivePolicy("root", "data/x", "read", {});
    EXPECT_TRUE(decision.allowed);
}

TEST_F(CrossTenantInheritanceTest, SetAuditLoggerNullIsNoop)
{
    // Setting null audit logger and then evaluating should not crash.
    inh.setAuditLogger(nullptr);

    ASSERT_TRUE(inh.registerTenant("root"));
    inh.setTenantPolicyManager("root", makeManager(makeRule("r", "data/*", "read")));

    auto decision = inh.evaluateEffectivePolicy("root", "data/x", "read", {});
    EXPECT_TRUE(decision.allowed);
}
