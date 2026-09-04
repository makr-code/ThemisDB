// Copyright 2026 ThemisDB — Licensed under MIT License
// AI Safety Layer — Schichten 1 & 2: AiOperationGuard (DOG + HILG)
// Roadmap: src/security/ROADMAP.md § Phase 2 (ASL-4, ASL-7)
//
// Tests:
//   AOG-01  READ_ONLY tool → no approval required
//   AOG-02  WRITE_SAFE tool → no approval required
//   AOG-03  delete_entity → DESTRUCTIVE, requires_approval=true, op_id non-empty
//   AOG-04  drop_index → DESTRUCTIVE
//   AOG-05  AQL read-only query → READ_ONLY
//   AOG-06  AQL INSERT → WRITE_SAFE
//   AOG-07  AQL REMOVE with FILTER → DESTRUCTIVE
//   AOG-08  AQL REMOVE without FILTER (full-collection) → CRITICAL
//   AOG-09  AQL DROP COLLECTION → CRITICAL
//   AOG-10  AQL TRUNCATE → CRITICAL
//   AOG-11  System collection (_system) → escalates to CRITICAL
//   AOG-12  denied_collections → hard-block (non-empty block_reason)
//   AOG-13  CRITICAL in production, no role → hard-block
//   AOG-14  CRITICAL in production, correct role → approval only (not blocked)
//   AOG-15  CRITICAL in development, no role → approval (not blocked)
//   AOG-16  guard disabled → always READ_ONLY, no approval
//   AOG-17  buildRequiresApprovalResponse returns correct shape
//   AOG-18  buildBlockedResponse returns correct shape
//   AOG-19  operation_id unique across two calls
//   AOG-20  allowed_collections: tool targeting denied collection → hard-block
//   AOG-21  dry_run_preview field defaults to true in Config (ASL-7)
//   AOG-22  dry_run_preview can be set to false (ASL-7)
//   AOG-23  approval_timeout_s is applied from Config (ASL-7)

#include <gtest/gtest.h>
#include "security/ai_operation_guard.h"

#include <string>
#include <unordered_set>

using namespace themis::security;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static AiOperationGuard makeGuard(
    bool                             enabled       = true,
    OperationClass                   threshold     = OperationClass::DESTRUCTIVE,
    const std::string&               env           = "development",
    bool                             block_prod    = true,
    std::vector<std::string>         denied        = {},
    std::vector<std::string>         allowed       = {},
    const std::string&               role          = "AI_DESTRUCTIVE_PRODUCTION_OPS"
) {
    AiOperationGuard::Config cfg;
    cfg.enabled                = enabled;
    cfg.approval_threshold     = threshold;
    cfg.environment            = env;
    cfg.block_critical_in_prod = block_prod;
    cfg.denied_collections     = std::move(denied);
    cfg.allowed_collections    = std::move(allowed);
    cfg.critical_ops_role      = role;
    cfg.approval_timeout_s     = 30;
    cfg.dry_run_preview        = true;
    return AiOperationGuard(std::move(cfg));
}

// ---------------------------------------------------------------------------
// AOG-01  READ_ONLY tool → no approval required
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, ReadOnlyTool) {
    auto guard = makeGuard();
    for (const auto& tool : {"get_entity", "get_schema", "get_stats", "list_indexes"}) {
        const auto d = guard.evaluate(tool, json::object(), "session-1");
        EXPECT_EQ(d.op_class, OperationClass::READ_ONLY) << "tool=" << tool;
        EXPECT_FALSE(d.requires_approval) << "tool=" << tool;
        EXPECT_TRUE(d.operation_id.empty()) << "tool=" << tool;
        EXPECT_TRUE(d.block_reason.empty()) << "tool=" << tool;
    }
}

// ---------------------------------------------------------------------------
// AOG-02  WRITE_SAFE tool → no approval required
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, WriteSafeTool) {
    auto guard = makeGuard();
    for (const auto& tool : {"put_entity", "create_index"}) {
        const auto d = guard.evaluate(tool, json::object(), "session-2");
        EXPECT_EQ(d.op_class, OperationClass::WRITE_SAFE) << "tool=" << tool;
        EXPECT_FALSE(d.requires_approval) << "tool=" << tool;
        EXPECT_TRUE(d.block_reason.empty()) << "tool=" << tool;
    }
}

// ---------------------------------------------------------------------------
// AOG-03  delete_entity → DESTRUCTIVE, requires_approval=true, op_id non-empty
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, DeleteEntityIsDestructive) {
    auto guard = makeGuard();
    const auto d = guard.evaluate("delete_entity",
                                  {{"key", "users:42"}}, "session-3");
    EXPECT_EQ(d.op_class, OperationClass::DESTRUCTIVE);
    EXPECT_TRUE(d.requires_approval);
    EXPECT_FALSE(d.operation_id.empty());
    EXPECT_TRUE(d.block_reason.empty());
    // operation_id has "op-" prefix
    EXPECT_EQ(d.operation_id.substr(0, 3), "op-");
}

// ---------------------------------------------------------------------------
// AOG-04  drop_index → DESTRUCTIVE
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, DropIndexIsDestructive) {
    auto guard = makeGuard();
    const auto d = guard.evaluate("drop_index",
                                  {{"table", "products"}, {"column", "name"}},
                                  "session-4");
    EXPECT_EQ(d.op_class, OperationClass::DESTRUCTIVE);
    EXPECT_TRUE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-05  AQL read-only query → READ_ONLY
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, AqlReadOnlyQuery) {
    auto guard = makeGuard();
    const auto d = guard.evaluate("query",
                                  {{"query", "FOR u IN users FILTER u.age > 18 RETURN u"}},
                                  "session-5");
    EXPECT_EQ(d.op_class, OperationClass::READ_ONLY);
    EXPECT_FALSE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-06  AQL INSERT → WRITE_SAFE
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, AqlInsert) {
    auto guard = makeGuard();
    const auto d = guard.evaluate("query",
                                  {{"query", "INSERT {name: @n} INTO users"}},
                                  "session-6");
    EXPECT_EQ(d.op_class, OperationClass::WRITE_SAFE);
    EXPECT_FALSE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-07  AQL REMOVE with FILTER → DESTRUCTIVE
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, AqlFilteredRemove) {
    auto guard = makeGuard();
    const auto d = guard.evaluate("query",
        {{"query", "FOR u IN users FILTER u._key == @k REMOVE u IN users"}},
        "session-7");
    EXPECT_EQ(d.op_class, OperationClass::DESTRUCTIVE);
    EXPECT_TRUE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-08  AQL REMOVE without FILTER (full-collection) → CRITICAL
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, AqlFullCollectionRemove) {
    auto guard = makeGuard();
    const auto d = guard.evaluate("query",
        {{"query", "FOR x IN orders REMOVE x IN orders"}},
        "session-8");
    EXPECT_EQ(d.op_class, OperationClass::CRITICAL);
    EXPECT_TRUE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-09  AQL DROP COLLECTION → CRITICAL
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, AqlDropCollection) {
    auto guard = makeGuard();
    const auto d = guard.evaluate("query",
        {{"query", "DROP COLLECTION users"}},
        "session-9");
    EXPECT_EQ(d.op_class, OperationClass::CRITICAL);
    EXPECT_TRUE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-10  AQL TRUNCATE → CRITICAL
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, AqlTruncate) {
    auto guard = makeGuard();
    const auto d = guard.evaluate("query",
        {{"query", "TRUNCATE logs"}},
        "session-10");
    EXPECT_EQ(d.op_class, OperationClass::CRITICAL);
    EXPECT_TRUE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-11  System collection (_system) → escalates to CRITICAL
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, SystemCollectionEscalatesToCritical) {
    auto guard = makeGuard();
    // delete_entity with _system collection key
    const auto d = guard.evaluate("delete_entity",
        {{"key", "_system:admin"}}, "session-11");
    EXPECT_EQ(d.op_class, OperationClass::CRITICAL);
}

// ---------------------------------------------------------------------------
// AOG-12  denied_collections → hard-block
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, DeniedCollectionHardBlock) {
    auto guard = makeGuard(true, OperationClass::DESTRUCTIVE, "development",
                           true, {"finance"}, {});
    const auto d = guard.evaluate("delete_entity",
        {{"key", "finance:record_42"}}, "session-12");
    EXPECT_FALSE(d.block_reason.empty());
    EXPECT_FALSE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-13  CRITICAL in production, no role → hard-block
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, CriticalInProductionNoRoleIsBlocked) {
    auto guard = makeGuard(true, OperationClass::DESTRUCTIVE, "production",
                           true, {}, {}, "AI_DESTRUCTIVE_PRODUCTION_OPS");
    const auto d = guard.evaluate("query",
        {{"query", "FOR x IN orders REMOVE x IN orders"}},
        "session-13", /*caller_role=*/"");
    EXPECT_FALSE(d.block_reason.empty());
    EXPECT_FALSE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-14  CRITICAL in production, correct role → approval (not blocked)
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, CriticalInProductionWithRoleRequiresApproval) {
    auto guard = makeGuard(true, OperationClass::DESTRUCTIVE, "production",
                           true, {}, {}, "AI_DESTRUCTIVE_PRODUCTION_OPS");
    const auto d = guard.evaluate("query",
        {{"query", "FOR x IN orders REMOVE x IN orders"}},
        "session-14", "AI_DESTRUCTIVE_PRODUCTION_OPS");
    EXPECT_TRUE(d.block_reason.empty());
    EXPECT_TRUE(d.requires_approval);
    EXPECT_EQ(d.op_class, OperationClass::CRITICAL);
}

// ---------------------------------------------------------------------------
// AOG-15  CRITICAL in development, no role → approval (not blocked)
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, CriticalInDevelopmentNoBlock) {
    auto guard = makeGuard(true, OperationClass::DESTRUCTIVE, "development");
    const auto d = guard.evaluate("query",
        {{"query", "FOR x IN orders REMOVE x IN orders"}},
        "session-15", /*caller_role=*/"");
    EXPECT_TRUE(d.block_reason.empty());
    EXPECT_TRUE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-16  guard disabled → always READ_ONLY, no approval
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, GuardDisabledPassesThrough) {
    auto guard = makeGuard(/*enabled=*/false);
    for (const auto& tool : {"delete_entity", "drop_index", "query"}) {
        json args = {};
        if (std::string(tool) == "query") {
            args["query"] = "FOR x IN col REMOVE x IN col";
        } else {
            args["key"] = "users:1";
        }
        const auto d = guard.evaluate(tool, args, "session-16");
        EXPECT_EQ(d.op_class, OperationClass::READ_ONLY) << "tool=" << tool;
        EXPECT_FALSE(d.requires_approval) << "tool=" << tool;
        EXPECT_TRUE(d.block_reason.empty()) << "tool=" << tool;
    }
}

// ---------------------------------------------------------------------------
// AOG-17  buildRequiresApprovalResponse returns correct shape
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, BuildRequiresApprovalResponse) {
    auto guard = makeGuard();
    const auto d = guard.evaluate("delete_entity",
                                  {{"key", "users:99"}}, "session-17");
    ASSERT_TRUE(d.requires_approval);

    const json resp = guard.buildRequiresApprovalResponse(d);
    EXPECT_EQ(resp["status"].get<std::string>(), "requires_approval");
    EXPECT_EQ(resp["operation_id"].get<std::string>(), d.operation_id);
    EXPECT_EQ(resp["classification"].get<std::string>(), "DESTRUCTIVE");
    EXPECT_EQ(resp["tool"].get<std::string>(), "delete_entity");
    EXPECT_TRUE(resp.contains("preview"));
    EXPECT_TRUE(resp.contains("impact_estimate"));
    EXPECT_TRUE(resp.contains("expires_at"));
    EXPECT_TRUE(resp.contains("approve_url"));
    // approve_url contains the operation_id
    const std::string url = resp["approve_url"].get<std::string>();
    EXPECT_NE(url.find(d.operation_id), std::string::npos);
}

// ---------------------------------------------------------------------------
// AOG-18  buildBlockedResponse returns correct shape
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, BuildBlockedResponse) {
    auto guard = makeGuard(true, OperationClass::DESTRUCTIVE, "production",
                           true, {}, {}, "AI_DESTRUCTIVE_PRODUCTION_OPS");
    const auto d = guard.evaluate("query",
        {{"query", "FOR x IN orders REMOVE x IN orders"}},
        "session-18", "");
    ASSERT_FALSE(d.block_reason.empty());

    const json resp = guard.buildBlockedResponse(d);
    EXPECT_EQ(resp["status"].get<std::string>(), "blocked");
    EXPECT_FALSE(resp["reason"].get<std::string>().empty());
    EXPECT_EQ(resp["classification"].get<std::string>(), "CRITICAL");
}

// ---------------------------------------------------------------------------
// AOG-19  operation_id unique across two calls
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, OperationIdIsUnique) {
    auto guard = makeGuard();
    const auto d1 = guard.evaluate("delete_entity",
                                   {{"key", "users:1"}}, "s1");
    const auto d2 = guard.evaluate("delete_entity",
                                   {{"key", "users:2"}}, "s2");
    ASSERT_TRUE(d1.requires_approval);
    ASSERT_TRUE(d2.requires_approval);
    EXPECT_NE(d1.operation_id, d2.operation_id);
}

// ---------------------------------------------------------------------------
// AOG-20  allowed_collections: tool targeting a non-allowed collection
//          → hard-block
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, AllowedCollectionsEnforced) {
    // Only "users" is allowed; targeting "orders" should hard-block
    auto guard = makeGuard(true, OperationClass::DESTRUCTIVE, "development",
                           false, {}, {"users"});
    const auto d = guard.evaluate("delete_entity",
        {{"key", "orders:5"}}, "session-20");
    EXPECT_FALSE(d.block_reason.empty());
    EXPECT_FALSE(d.requires_approval);
}

// ---------------------------------------------------------------------------
// AOG-21  dry_run_preview field defaults to true in Config (ASL-7)
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, DryRunPreviewDefaultTrue) {
    AiOperationGuard::Config cfg;
    EXPECT_TRUE(cfg.dry_run_preview);
}

// ---------------------------------------------------------------------------
// AOG-22  dry_run_preview can be set to false (ASL-7)
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, DryRunPreviewSetFalse) {
    AiOperationGuard::Config cfg;
    cfg.dry_run_preview = false;
    AiOperationGuard guard(std::move(cfg));
    EXPECT_FALSE(guard.config().dry_run_preview);
}

// ---------------------------------------------------------------------------
// AOG-23  approval_timeout_s is applied from Config (ASL-7)
//          A guard constructed with timeout=120 should report 120.
// ---------------------------------------------------------------------------
TEST(AiOperationGuardTest, ApprovalTimeoutFromConfig) {
    AiOperationGuard::Config cfg;
    cfg.approval_timeout_s = 120;
    AiOperationGuard guard(std::move(cfg));
    EXPECT_EQ(guard.config().approval_timeout_s, 120);
}
