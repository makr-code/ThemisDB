#include "llm/ai_orchestrator.h"
#include "llm/skill_delegation_context.h"
#include "llm/skill_registry.h"
#ifdef THEMIS_TEST_NATIVE_TIMESERIES_SKILL
#include "index/graph_index.h"
#include "index/property_graph.h"
#include "llm/asset_impact_analysis_tool.h"
#include "llm/timeseries_analysis_tool.h"
#include "storage/base_entity.h"
#endif

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <vector>

namespace {

namespace fs = std::filesystem;
using themis::errors::ErrorCode;
using themis::llm::ModeSpec;
using themis::llm::SkillBudgetLimits;
using themis::llm::SkillBudgetUsage;
using themis::llm::SkillDelegationContext;
using themis::llm::SkillPermission;
using themis::llm::SkillPermissionSet;
using themis::llm::SkillRegistry;
using themis::llm::SkillRegistryOptions;
using themis::llm::ToolRegistry;
using themis::llm::registerSkillTools;

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        static std::atomic_uint64_t sequence{0};
        const auto timestamp = std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count();
        path_ = fs::temp_directory_path() /
                ("themis_skill_registry_" + std::to_string(timestamp) + "_" +
                 std::to_string(sequence.fetch_add(1)));
        fs::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        fs::remove_all(path_, error);
    }

    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

void writeFile(const fs::path& path, const std::string& contents) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output.is_open()) {
        throw std::runtime_error("test fixture could not be written");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

fs::path writeSkill(const fs::path& root,
                    const std::string& name,
                    const std::string& instructions = "Use the documented process.\n") {
    const auto package = root / name;
    writeFile(package / "SKILL.md",
              "---\nname: " + name + "\ndescription: Guide for " + name +
                  ".\n---\n" + instructions);
    return package;
}

SkillRegistryOptions optionsFor(const fs::path& root) {
    SkillRegistryOptions options;
    options.roots = {root};
    return options;
}

SkillDelegationContext::PermissionAuthorizer allowTestPermissions() {
    return [](std::string_view principal_id,
              std::string_view tenant_id,
              const SkillPermission&) {
        return principal_id == "principal-1" && tenant_id == "tenant-1";
    };
}

TEST(SkillDelegationContextTest, ChildCanOnlyRemoveExactPermissions) {
    const auto expiry = SkillDelegationContext::Clock::now() +
                        std::chrono::minutes(5);
    SkillPermissionSet parent_permissions{
        {"read", "collection:cases"},
        {"read", "collection:reports"},
    };
    auto root = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1", parent_permissions, expiry,
        SkillBudgetLimits{4, 100, 4096}, allowTestPermissions());
    ASSERT_TRUE(root);

    SkillPermissionSet child_permissions{{"read", "collection:cases"}};
    auto child = root->attenuate(child_permissions, expiry);
    ASSERT_TRUE(child);
    EXPECT_EQ(child->principalId(), "principal-1");
    EXPECT_EQ(child->tenantId(), "tenant-1");
    EXPECT_TRUE(child->allows("read", "collection:cases"));
    EXPECT_FALSE(child->allows("read", "collection:reports"));
    EXPECT_FALSE(child->allows("write", "collection:cases"));

    auto grandchild = child->attenuate(
        SkillPermissionSet{{"read", "collection:cases"}}, expiry);
    ASSERT_TRUE(grandchild);
    auto escalated_grandchild = child->attenuate(
        SkillPermissionSet{{"read", "collection:reports"}}, expiry);
    ASSERT_FALSE(escalated_grandchild);
    EXPECT_EQ(escalated_grandchild.error().code(),
              ErrorCode::ERR_QUERY_ACCESS_DENIED);
}

TEST(SkillDelegationContextTest, RejectsPermissionAndExpiryExpansion) {
    const auto expiry = SkillDelegationContext::Clock::now() +
                        std::chrono::minutes(5);
    auto root = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1",
        SkillPermissionSet{{"read", "collection:cases"}}, expiry,
        SkillBudgetLimits{4, 100, 4096}, allowTestPermissions());
    ASSERT_TRUE(root);

    auto extra_permission = root->attenuate(
        SkillPermissionSet{{"read", "collection:cases"},
                           {"write", "collection:cases"}},
        expiry);
    ASSERT_FALSE(extra_permission);
    EXPECT_EQ(extra_permission.error().code(),
              ErrorCode::ERR_QUERY_ACCESS_DENIED);

    auto later_expiry = root->attenuate(
        SkillPermissionSet{{"read", "collection:cases"}},
        expiry + std::chrono::seconds(1));
    ASSERT_FALSE(later_expiry);
    EXPECT_EQ(later_expiry.error().code(),
              ErrorCode::ERR_QUERY_ACCESS_DENIED);
}

TEST(SkillDelegationContextTest, RejectsInvalidRootIdentityAndWildcardGrant) {
    const auto expiry = SkillDelegationContext::Clock::now() +
                        std::chrono::minutes(5);
    auto missing_identity = SkillDelegationContext::createRoot(
        "", "tenant-1", {}, expiry, SkillBudgetLimits{},
        allowTestPermissions());
    ASSERT_FALSE(missing_identity);
    EXPECT_EQ(missing_identity.error().code(),
              ErrorCode::ERR_UTIL_INVALID_ARGUMENT);

    auto wildcard = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1",
        SkillPermissionSet{{"read", "collection:*"}}, expiry,
        SkillBudgetLimits{}, allowTestPermissions());
    ASSERT_FALSE(wildcard);
    EXPECT_EQ(wildcard.error().code(), ErrorCode::ERR_UTIL_INVALID_ARGUMENT);

    auto expired = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1", {},
        SkillDelegationContext::TimePoint::min(), SkillBudgetLimits{},
        allowTestPermissions());
    ASSERT_FALSE(expired);
    EXPECT_EQ(expired.error().code(), ErrorCode::ERR_UTIL_INVALID_ARGUMENT);
}

TEST(SkillDelegationContextTest, DescendantsShareAtomicNonResettableBudget) {
    const auto expiry = SkillDelegationContext::Clock::now() +
                        std::chrono::minutes(5);
    auto root = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1", {}, expiry,
        SkillBudgetLimits{2, 4, 10}, allowTestPermissions());
    ASSERT_TRUE(root);
    auto child = root->attenuate({}, expiry);
    ASSERT_TRUE(child);

    EXPECT_TRUE(child->consumeBudget(SkillBudgetUsage{1, 3, 7}));
    EXPECT_FALSE(root->consumeBudget(SkillBudgetUsage{1, 2, 4}));
    EXPECT_EQ(root->remainingBudget().max_tool_calls, 1);
    EXPECT_EQ(root->remainingBudget().max_candidates, 1);
    EXPECT_EQ(root->remainingBudget().max_content_bytes, 3);

    EXPECT_TRUE(root->consumeBudget(SkillBudgetUsage{1, 1, 3}));
    EXPECT_FALSE(child->consumeBudget(SkillBudgetUsage{1, 0, 0}));
    EXPECT_EQ(child->remainingBudget().max_tool_calls, 0);
    EXPECT_EQ(child->remainingBudget().max_candidates, 0);
    EXPECT_EQ(child->remainingBudget().max_content_bytes, 0);
}

TEST(SkillDelegationContextTest, ConcurrentDescendantsCannotOverspendBudget) {
    const auto expiry = SkillDelegationContext::Clock::now() +
                        std::chrono::minutes(5);
    auto root = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1", {}, expiry,
        SkillBudgetLimits{64, 64, 64}, allowTestPermissions());
    ASSERT_TRUE(root);
    auto child = root->attenuate({}, expiry);
    ASSERT_TRUE(child);

    std::atomic_size_t accepted{0};
    std::vector<std::thread> workers;
    workers.reserve(8);
    for (std::size_t worker = 0; worker < 8; ++worker) {
        workers.emplace_back([&child, &accepted] {
            for (std::size_t attempt = 0; attempt < 50; ++attempt) {
                if (child->consumeBudget(SkillBudgetUsage{1, 1, 1})) {
                    accepted.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    EXPECT_EQ(accepted.load(std::memory_order_relaxed), 64);
    EXPECT_EQ(root->remainingBudget().max_tool_calls, 0);
    EXPECT_EQ(root->remainingBudget().max_candidates, 0);
    EXPECT_EQ(root->remainingBudget().max_content_bytes, 0);
}

TEST(SkillDelegationContextTest, ExpiredContextCannotAuthorizeOrConsumeBudget) {
    const auto expiry = SkillDelegationContext::Clock::now() +
                        std::chrono::milliseconds(1);
    auto root = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1",
        SkillPermissionSet{{"read", "collection:cases"}}, expiry,
        SkillBudgetLimits{2, 4, 10}, allowTestPermissions());
    ASSERT_TRUE(root);

    std::this_thread::sleep_until(expiry + std::chrono::milliseconds(1));
    EXPECT_FALSE(root->isActive());
    EXPECT_FALSE(root->allows("read", "collection:cases"));
    EXPECT_FALSE(root->consumeBudget(SkillBudgetUsage{1, 1, 1}));
    auto child = root->attenuate({}, expiry);
    ASSERT_FALSE(child);
    EXPECT_EQ(child.error().code(), ErrorCode::ERR_QUERY_ACCESS_DENIED);
    EXPECT_EQ(root->remainingBudget().max_tool_calls, 2);
}

TEST(SkillDelegationContextTest, RootIssuanceRequiresPolicyApproval) {
    const auto expiry = SkillDelegationContext::Clock::now() +
                        std::chrono::minutes(5);
    auto missing_authorizer = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1", {}, expiry, SkillBudgetLimits{},
        SkillDelegationContext::PermissionAuthorizer{});
    ASSERT_FALSE(missing_authorizer);
    EXPECT_EQ(missing_authorizer.error().code(),
              ErrorCode::ERR_UTIL_INVALID_ARGUMENT);

    std::size_t authorization_checks = 0;
    SkillDelegationContext::PermissionAuthorizer deny_write =
        [&authorization_checks](std::string_view principal_id,
                                std::string_view tenant_id,
                                const SkillPermission& permission) {
            ++authorization_checks;
            return principal_id == "principal-1" &&
                   tenant_id == "tenant-1" &&
                   permission.action == "read";
        };
    auto denied = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1",
        SkillPermissionSet{{"read", "collection:cases"},
                           {"write", "collection:cases"}},
        expiry, SkillBudgetLimits{1, 1, 1}, deny_write);
    ASSERT_FALSE(denied);
    EXPECT_EQ(denied.error().code(), ErrorCode::ERR_QUERY_ACCESS_DENIED);
    EXPECT_EQ(authorization_checks, 2u);

    auto identity_mismatch = SkillDelegationContext::createRoot(
        "different-principal", "tenant-1",
        SkillPermissionSet{{"read", "collection:cases"}},
        expiry, SkillBudgetLimits{1, 1, 1}, allowTestPermissions());
    ASSERT_FALSE(identity_mismatch);
    EXPECT_EQ(identity_mismatch.error().code(),
              ErrorCode::ERR_QUERY_ACCESS_DENIED);

    auto policy_failure = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1",
        SkillPermissionSet{{"read", "collection:cases"}},
        expiry, SkillBudgetLimits{1, 1, 1},
        [](std::string_view, std::string_view,
           const SkillPermission&) -> bool {
            throw std::runtime_error("policy backend unavailable");
        });
    ASSERT_FALSE(policy_failure);
    EXPECT_EQ(policy_failure.error().code(),
              ErrorCode::ERR_QUERY_ACCESS_DENIED);
}

TEST(SkillRegistryTest, DiscoversAndLoadsShippedSkillsAndReference) {
    constexpr const char* expected_names[] = {
        "aql-mutation-draft", "aql-plan-analysis", "aql-query-review",
        "aql-read-query", "asset-impact-analysis", "evidence-answer", "file-edit",
        "file-import-planning", "file-inspection", "graph-exploration",
        "hybrid-search-diagnostics", "log-analysis", "process-bpmn-mining",
        "rag-evaluation", "rag-retrieval", "similar-case-research", "themisdb-yaml-config",
        "timeseries-window-analysis", "trace-correlation",
    };
    constexpr auto expected_count = sizeof(expected_names) / sizeof(expected_names[0]);
    const auto repo_root =
        fs::path(__FILE__).parent_path().parent_path().parent_path();
    auto skills = std::make_shared<SkillRegistry>(
        optionsFor(repo_root / "config" / "agent_skills"));
    const auto refreshed = skills->refresh();
    ASSERT_TRUE(refreshed);
    EXPECT_EQ(*refreshed, expected_count);

    ToolRegistry tools;
    registerSkillTools(tools, skills);
    ModeSpec mode;
    mode.tools_allowed = {"list_skills", "load_skill", "read_skill_resource"};
    const auto catalog = tools.invokeTool("list_skills", {}, mode);
    ASSERT_TRUE(catalog.contains("skills"));
    ASSERT_EQ(catalog["skills"].size(), expected_count);
    for (std::size_t i = 0; i < expected_count; ++i) {
        const auto& entry = catalog["skills"][i];
        ASSERT_TRUE(entry.contains("name"));
        EXPECT_EQ(entry["name"], expected_names[i]);
        const auto loaded = tools.invokeTool(
            "load_skill", {{"name", entry["name"]}}, mode);
        ASSERT_TRUE(loaded.contains("instructions"));
        EXPECT_FALSE(loaded["instructions"].get<std::string>().empty());
    }

    const auto reference = tools.invokeTool(
        "read_skill_resource",
        {{"name", "evidence-answer"}, {"path", "references/source-checklist.md"}},
        mode);
    ASSERT_TRUE(reference.contains("content"));
    EXPECT_NE(reference["content"].get<std::string>().find("authorized scope"),
              std::string::npos);

    mode.tools_denied = {"load_skill"};
    const auto denied =
        tools.invokeTool("load_skill", {{"name", "aql-read-query"}}, mode);
    EXPECT_TRUE(denied.contains("error"));
}

#ifdef THEMIS_TEST_NATIVE_TIMESERIES_SKILL
TEST(SkillRegistryTest, NativeTimeseriesToolReadsScopedDataWithOptionalGeo) {
    TemporaryDirectory temporary;
    themis::RocksDBWrapper::Config config;
    config.db_path = (temporary.path() / "data").string();
    config.enable_blobdb = false;
    auto db = std::make_shared<themis::RocksDBWrapper>(config);
    ASSERT_TRUE(db->open());
    auto store = std::make_shared<themis::TSStore>(db->getRawDB());
    const auto putEntity = [&](const char* collection, const char* key, std::string_view document) {
        const auto entity = themis::BaseEntity::fromJson(key, document);
        return db->put(themis::KeySchema::makeRelationalKey(collection, key), entity.serialize());
    };
    ASSERT_TRUE(putEntity("assets", "finance", "{}"));
    ASSERT_TRUE(putEntity("assets", "local", R"({"location":{"type":"Point","coordinates":[10,50]}})"));
    ASSERT_TRUE(putEntity("assets", "remote", R"({"location":{"type":"Point","coordinates":[20,50]}})"));
    ASSERT_TRUE(putEntity("outside", "secret", "{}"));
    const std::tuple<const char*, int, double> samples[] = {
        {"local", 1000, 20.0}, {"local", 1999, 40.0}, {"local", 2000, 50.0},
        {"local", 3000, 999.0}, {"finance", 2000, 75.0}, {"secret", 2000, 99.0},
    };
    for (const auto& [entity, timestamp, value] : samples) {
        themis::TSStore::DataPoint point{};
        point.metric = "reading";
        point.entity = entity;
        point.timestamp_ms = timestamp;
        point.value = value;
        ASSERT_TRUE(store->putDataPoint(point));
    }

    ToolRegistry tools;
    themis::llm::registerTimeseriesAnalysisTool(tools, db, store, "assets", "reading");
    ModeSpec mode;
    auto invoke = [&](const themis::llm::json& args) {
        return tools.invokeTool("compare_timeseries_windows", args, mode);
    };
    themis::llm::json args = {
        {"baseline_start_ms", 1000}, {"current_start_ms", 2000}, {"duration_ms", 1000}
    };
    EXPECT_TRUE(invoke(args).contains("error"));
    mode.tools_allowed = {"compare_timeseries_windows"};
    auto result = invoke(args);
    ASSERT_FALSE(result.contains("error")) << result.dump();
    ASSERT_EQ(result["entities"].size(), 3u);
    EXPECT_EQ(result["entities"][0]["entity"], "finance");
    EXPECT_TRUE(result["entities"][0]["delta"].is_null());
    EXPECT_EQ(result["entities"][0]["current_mean"], 75.0);
    EXPECT_EQ(result["entities"][1]["baseline_count"], 2u);
    EXPECT_EQ(result["entities"][1]["baseline_mean"], 30.0);
    EXPECT_EQ(result["entities"][1]["current_count"], 1u);
    EXPECT_EQ(result["entities"][1]["delta"], 20.0);

    args["geo"] = {{"lon", 10}, {"lat", 50}, {"radius_m", 1000}};
    result = invoke(args);
    ASSERT_FALSE(result.contains("error")) << result.dump();
    ASSERT_EQ(result["entities"].size(), 1u);
    EXPECT_EQ(result["entities"][0]["entity"], "local");
    EXPECT_EQ(result["entities"][0]["distance_m"], 0.0);
    EXPECT_EQ(result["missing_location"], 1u);
    args["collection"] = "outside";
    EXPECT_TRUE(invoke(args).contains("error"));
    args.erase("collection");
    args["current_start_ms"] = 1999;
    EXPECT_TRUE(invoke(args).contains("error"));
    args["current_start_ms"] = 2000;
    args["geo"]["radius_m"] = 1'000'001;
    EXPECT_TRUE(invoke(args).contains("error"));
    args["geo"]["radius_m"] = 1000;
    ASSERT_TRUE(putEntity("assets", "invalid", R"({"location":{"type":"Point","coordinates":[10,95]}})"));
    EXPECT_TRUE(invoke(args).contains("error"));
    args.erase("geo");
    for (int i = 0; i < 61; ++i) {
        ASSERT_TRUE(putEntity("assets", ("extra" + std::to_string(i)).c_str(), "{}"));
    }
    const auto limited = invoke(args);
    EXPECT_EQ(limited.value("error", ""), "Entity collection exceeds the 64-document analysis limit");
}
#endif

#ifdef THEMIS_TEST_NATIVE_TIMESERIES_SKILL
TEST(SkillRegistryTest, NativeAssetImpactToolCombinesScopedGraphAndTimeSeries) {
    TemporaryDirectory temporary;
    themis::RocksDBWrapper::Config config;
    config.db_path = (temporary.path() / "data").string();
    config.enable_blobdb = false;
    auto db = std::make_shared<themis::RocksDBWrapper>(config);
    ASSERT_TRUE(db->open());

    constexpr auto graphId = "assets";
    constexpr auto edgeType = "DEPENDS_ON";
    constexpr auto metric = "temperature";
    auto graph = std::make_shared<themis::GraphIndexManager>(*db);
    themis::PropertyGraphManager propertyGraph(*db);
    const auto addNode = [&](const char* id, const char* document) {
        const auto node = themis::BaseEntity::fromJson(id, document);
        return propertyGraph.addNode(node, graphId);
    };
    ASSERT_TRUE(addNode(
        "root", R"({"id":"root","location":{"type":"Point","coordinates":[10,50]}})").ok);
    ASSERT_TRUE(addNode(
        "pump", R"({"id":"pump","location":{"type":"Point","coordinates":[10.001,50]}})").ok);
    ASSERT_TRUE(addNode(
        "remote", R"({"id":"remote","location":{"type":"Point","coordinates":[12,50]}})").ok);

    const auto addEdge = [&](const char* id, const char* from, const char* to,
                             const char* type, const char* scope) {
        themis::BaseEntity edge(id);
        edge.setField("id", id);
        edge.setField("_from", from);
        edge.setField("_to", to);
        edge.setField("_type", type);
        return propertyGraph.addEdge(edge, scope);
    };
    ASSERT_TRUE(addEdge("root-pump", "root", "pump", edgeType, graphId).ok);
    ASSERT_TRUE(addEdge("pump-remote", "pump", "remote", edgeType, graphId).ok);
    ASSERT_TRUE(addEdge("foreign", "root", "remote", edgeType, "other-graph").ok);

    auto store = std::make_shared<themis::TSStore>(db->getRawDB());
    const auto addPoint = [&](const char* entity, int64_t timestamp, double value) {
        themis::TSStore::DataPoint point{};
        point.metric = metric;
        point.entity = entity;
        point.timestamp_ms = timestamp;
        point.value = value;
        return store->putDataPoint(point);
    };
    ASSERT_TRUE(addPoint("root", 1000, 10.0));
    ASSERT_TRUE(addPoint("root", 2000, 15.0));
    ASSERT_TRUE(addPoint("pump", 1000, 20.0));
    ASSERT_TRUE(addPoint("pump", 2000, 30.0));
    ASSERT_TRUE(addPoint("remote", 2000, 50.0));

    ToolRegistry tools;
    themis::llm::registerAssetImpactAnalysisTool(
        tools, db, graph, store, graphId, edgeType, metric);
    ModeSpec mode;
    themis::llm::json args = {
        {"start_asset_id", "root"}, {"max_depth", 2},
        {"baseline_start_ms", 1000}, {"current_start_ms", 2000},
        {"duration_ms", 1000}
    };
    EXPECT_TRUE(tools.invokeTool("analyze_asset_impact", args, mode).contains("error"));

    mode.tools_allowed = {"analyze_asset_impact"};
    auto result = tools.invokeTool("analyze_asset_impact", args, mode);
    ASSERT_FALSE(result.contains("error")) << result.dump();
    ASSERT_EQ(result["assets"].size(), 3u);
    EXPECT_EQ(result["edges_examined"], 2u);
    const auto pump = std::find_if(
        result["assets"].begin(), result["assets"].end(),
        [](const auto& asset) { return asset["asset_id"] == "pump"; });
    ASSERT_NE(pump, result["assets"].end());
    EXPECT_EQ((*pump)["depth"], 1u);
    EXPECT_EQ((*pump)["baseline_mean"], 20.0);
    EXPECT_EQ((*pump)["current_mean"], 30.0);
    EXPECT_EQ((*pump)["delta"], 10.0);
    const auto remote = std::find_if(
        result["assets"].begin(), result["assets"].end(),
        [](const auto& asset) { return asset["asset_id"] == "remote"; });
    ASSERT_NE(remote, result["assets"].end());
    EXPECT_EQ((*remote)["baseline_count"], 0u);
    EXPECT_TRUE((*remote)["baseline_mean"].is_null());
    EXPECT_TRUE((*remote)["delta"].is_null());

    ToolRegistry delegatedTools;
    themis::llm::registerDelegatedAssetImpactAnalysisTool(
        delegatedTools, db, graph, store, graphId, edgeType, metric);
    const SkillPermissionSet delegatedPermissions{
        {"tool.invoke", "analyze_asset_impact"},
        {"graph.read", graphId},
        {"timeseries.read", metric}
    };
    const auto expiresAt = SkillDelegationContext::Clock::now() +
                           std::chrono::minutes(5);
    auto delegatedContext = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1", delegatedPermissions, expiresAt,
        SkillBudgetLimits{1, 100, 1'000'000},
        [](std::string_view, std::string_view, const SkillPermission&) {
            return true;
        });
    ASSERT_TRUE(delegatedContext);
    const auto delegatedResult =
        delegatedTools.invokeTool("analyze_asset_impact", args, mode,
                                  *delegatedContext);
    ASSERT_FALSE(delegatedResult.contains("error")) << delegatedResult.dump();
    const auto remaining = delegatedContext->remainingBudget();
    EXPECT_LT(remaining.max_candidates, 100u);
    EXPECT_LT(remaining.max_content_bytes, 1'000'000u);

    auto missingGraphGrant = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1",
        SkillPermissionSet{{"tool.invoke", "analyze_asset_impact"},
                           {"timeseries.read", metric}},
        SkillDelegationContext::Clock::now() + std::chrono::minutes(5),
        SkillBudgetLimits{1, 100, 1'000'000},
        [](std::string_view, std::string_view, const SkillPermission&) {
            return true;
        });
    ASSERT_TRUE(missingGraphGrant);
    const auto denied =
        delegatedTools.invokeTool("analyze_asset_impact", args, mode,
                                  *missingGraphGrant);
    EXPECT_TRUE(denied.contains("error"));
    EXPECT_EQ(missingGraphGrant->remainingBudget().max_candidates, 100u);

    auto candidateLimited = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1", delegatedPermissions,
        SkillDelegationContext::Clock::now() + std::chrono::minutes(5),
        SkillBudgetLimits{1, 1, 1'000'000},
        [](std::string_view, std::string_view, const SkillPermission&) {
            return true;
        });
    ASSERT_TRUE(candidateLimited);
    const auto candidateLimitedResult =
        delegatedTools.invokeTool("analyze_asset_impact", args, mode,
                                  *candidateLimited);
    EXPECT_TRUE(candidateLimitedResult.contains("error"));
    EXPECT_EQ(candidateLimited->remainingBudget().max_candidates, 0u);

    auto byteLimited = SkillDelegationContext::createRoot(
        "principal-1", "tenant-1", delegatedPermissions,
        SkillDelegationContext::Clock::now() + std::chrono::minutes(5),
        SkillBudgetLimits{1, 100, 1},
        [](std::string_view, std::string_view, const SkillPermission&) {
            return true;
        });
    ASSERT_TRUE(byteLimited);
    const auto byteLimitedResult =
        delegatedTools.invokeTool("analyze_asset_impact", args, mode,
                                  *byteLimited);
    EXPECT_TRUE(byteLimitedResult.contains("error"));
    EXPECT_EQ(byteLimited->remainingBudget().max_content_bytes, 1u);

    args["geo"] = {{"lon", 10}, {"lat", 50}, {"radius_m", 1000}};
    result = tools.invokeTool("analyze_asset_impact", args, mode);
    ASSERT_FALSE(result.contains("error")) << result.dump();
    ASSERT_EQ(result["assets"].size(), 2u);
    EXPECT_EQ(result["missing_location"], 0u);

    args["max_depth"] = 4;
    EXPECT_TRUE(tools.invokeTool("analyze_asset_impact", args, mode).contains("error"));

    themis::TSStore::QueryOptions strictOptions;
    strictOptions.metric = metric;
    strictOptions.entity = "root";
    strictOptions.from_timestamp_ms = 1500;
    strictOptions.to_timestamp_ms = 1500;
    strictOptions.limit = 4;
    const std::string corruptKey = "ts:temperature:root:00000000000000001500";
    ASSERT_TRUE(db->put(corruptKey, std::vector<uint8_t>{'{' }));
    const auto compatibleQuery = store->query(strictOptions);
    ASSERT_TRUE(compatibleQuery);
    EXPECT_TRUE(compatibleQuery->empty());
    const auto strictQuery = store->queryStrict(strictOptions);
    EXPECT_FALSE(strictQuery);

    strictOptions.entity = "pump";
    strictOptions.from_timestamp_ms = 1000;
    strictOptions.to_timestamp_ms = 1999;
    const std::string corruptChunkKey =
        "tsc:temperature:pump:00000000000000001000:00000000000000001999";
    ASSERT_TRUE(db->put(corruptChunkKey, std::vector<uint8_t>{'{' }));
    const auto compatibleChunkQuery = store->query(strictOptions);
    ASSERT_TRUE(compatibleChunkQuery);
    EXPECT_EQ(compatibleChunkQuery->size(), 1u);
    const auto strictChunkQuery = store->queryStrict(strictOptions);
    EXPECT_FALSE(strictChunkQuery);

    strictOptions.entity = "remote";
    const auto corruptPayload = themis::llm::json{
        {"compression", "gorilla"},
        {"data", std::vector<uint8_t>{0x47, 0x4f, 0xff}}
    }.dump();
    const std::string corruptPayloadKey =
        "tsc:temperature:remote:00000000000000001000:00000000000000001999";
    ASSERT_TRUE(db->put(
        corruptPayloadKey,
        std::vector<uint8_t>(corruptPayload.begin(), corruptPayload.end())));
    const auto compatiblePayloadQuery = store->query(strictOptions);
    ASSERT_TRUE(compatiblePayloadQuery);
    EXPECT_TRUE(compatiblePayloadQuery->empty());
    const auto strictPayloadQuery = store->queryStrict(strictOptions);
    EXPECT_FALSE(strictPayloadQuery);

    strictOptions.entity = "empty";
    const auto emptyPayload = themis::llm::json{
        {"compression", "gorilla"},
        {"data", std::vector<uint8_t>{}}
    }.dump();
    const std::string emptyPayloadKey =
        "tsc:temperature:empty:00000000000000001000:00000000000000001999";
    ASSERT_TRUE(db->put(
        emptyPayloadKey,
        std::vector<uint8_t>(emptyPayload.begin(), emptyPayload.end())));
    const auto compatibleEmptyQuery = store->query(strictOptions);
    ASSERT_TRUE(compatibleEmptyQuery);
    EXPECT_TRUE(compatibleEmptyQuery->empty());
    const auto strictEmptyQuery = store->queryStrict(strictOptions);
    EXPECT_FALSE(strictEmptyQuery);

    args["max_depth"] = 2;
    EXPECT_TRUE(tools.invokeTool("analyze_asset_impact", args, mode).contains("error"));
}

TEST(SkillRegistryTest, StrictTimeSeriesQueryMetersCompressedChunkAndDecodedPoints) {
    TemporaryDirectory temporary;
    themis::RocksDBWrapper::Config config;
    config.db_path = (temporary.path() / "data").string();
    config.enable_blobdb = false;
    auto db = std::make_shared<themis::RocksDBWrapper>(config);
    ASSERT_TRUE(db->open());
    themis::TSStore store(db->getRawDB());

    std::vector<themis::TSStore::DataPoint> points;
    for (int i = 0; i < 3; ++i) {
        themis::TSStore::DataPoint point{};
        point.metric = "reading";
        point.entity = "sensor";
        point.timestamp_ms = 1000 + i;
        point.value = static_cast<double>(i);
        points.push_back(std::move(point));
    }
    ASSERT_TRUE(store.putDataPoints(points));

    themis::TSStore::QueryOptions options;
    options.metric = "reading";
    options.entity = "sensor";
    options.from_timestamp_ms = 1000;
    options.to_timestamp_ms = 1002;
    options.limit = 10;
    std::size_t candidates = 0;
    std::size_t contentBytes = 0;
    options.work_meter = [&candidates, &contentBytes](
                             std::size_t newCandidates,
                             std::size_t newContentBytes) {
        candidates += newCandidates;
        contentBytes += newContentBytes;
        return true;
    };

    const auto result = store.queryStrict(options);
    ASSERT_TRUE(result);
    ASSERT_EQ(result->size(), 3u);
    EXPECT_EQ(candidates, 4u);
    EXPECT_GT(contentBytes, 0u);

    options.work_meter = [](std::size_t newCandidates, std::size_t) {
        return newCandidates == 0;
    };
    const auto rejected = store.queryStrict(options);
    EXPECT_FALSE(rejected);
    EXPECT_EQ(rejected.error().code(),
              themis::errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED);

    options.work_meter = [](std::size_t, std::size_t) -> bool {
        throw std::runtime_error("meter failure");
    };
    const auto exceptionRejected = store.queryStrict(options);
    EXPECT_FALSE(exceptionRejected);
    EXPECT_EQ(exceptionRejected.error().code(),
              themis::errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED);
}
#endif

TEST(SkillRegistryTest, DiscoversMetadataAndLoadsInstructionsOnRequest) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "weather");
    writeSkill(root, "alpha", "Alpha instructions.\n");
    writeFile(package / "references" / "format.md", "Use Celsius.\n");

    SkillRegistry registry(optionsFor(root));
    const auto refresh_result = registry.refresh();
    ASSERT_TRUE(refresh_result);
    EXPECT_EQ(*refresh_result, 2u);

    const auto listed = registry.list();
    ASSERT_TRUE(listed);
    ASSERT_EQ(listed->size(), 2u);
    EXPECT_EQ((*listed)[0].name, "alpha");
    EXPECT_EQ((*listed)[0].description, "Guide for alpha.");
    EXPECT_EQ((*listed)[1].name, "weather");
    EXPECT_EQ((*listed)[1].description, "Guide for weather.");

    const auto document = registry.load("weather");
    ASSERT_TRUE(document);
    EXPECT_EQ(document->instructions, "Use the documented process.\n");

    const auto resource = registry.readResource("weather", "references/format.md");
    ASSERT_TRUE(resource);
    EXPECT_EQ(*resource, "Use Celsius.\n");
}

TEST(SkillRegistryTest, DiscoveryDoesNotApplyInstructionBodyLimit) {
    TemporaryDirectory temporary;
    writeSkill(temporary.path() / "skills", "large-body",
               std::string(64 * 1024, 'x'));

    auto options = optionsFor(temporary.path() / "skills");
    options.max_instruction_bytes = 8;
    SkillRegistry registry(std::move(options));

    const auto refreshed = registry.refresh();
    ASSERT_TRUE(refreshed);
    EXPECT_EQ(*refreshed, 1u);
    ASSERT_TRUE(registry.list());

    const auto document = registry.load("large-body");
    ASSERT_FALSE(document);
    EXPECT_EQ(document.error().code(), ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT);
}

TEST(SkillRegistryTest, RejectsOversizedFrontmatterDuringDiscovery) {
    TemporaryDirectory temporary;
    writeSkill(temporary.path() / "skills", "large-metadata",
               "Instructions.\n");

    auto options = optionsFor(temporary.path() / "skills");
    options.max_frontmatter_bytes = 16;
    SkillRegistry registry(std::move(options));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(), ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT);
}

TEST(SkillRegistryTest, ListRequiresSuccessfulRefresh) {
    TemporaryDirectory temporary;
    SkillRegistry registry(optionsFor(temporary.path()));

    const auto listed = registry.list();
    ASSERT_FALSE(listed);
    EXPECT_EQ(listed.error().code(),
              ErrorCode::ERR_LLM_SKILL_REGISTRY_NOT_INITIALIZED);
}

TEST(SkillRegistryTest, InvalidManifestDoesNotReplacePreviousSnapshot) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "valid");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    writeFile(root / "broken" / "SKILL.md", "name: broken\ndescription: missing fences\n");
    const auto refresh_result = registry.refresh();
    ASSERT_FALSE(refresh_result);
    EXPECT_EQ(refresh_result.error().code(),
              ErrorCode::ERR_LLM_SKILL_INVALID_METADATA);

    const auto listed = registry.list();
    ASSERT_TRUE(listed);
    ASSERT_EQ(listed->size(), 1u);
    EXPECT_EQ(listed->front().name, "valid");
}

TEST(SkillRegistryTest, ConcurrentRefreshAndReadersObserveCompleteSnapshots) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "alpha", "Stable instructions.\n");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());
    writeSkill(root, "beta", "New instructions.\n");

    std::atomic<bool> start{false};
    std::atomic<bool> finished{false};
    std::atomic<bool> failed{false};
    std::thread refresher([&] {
        while (!start.load()) {
            std::this_thread::yield();
        }
        for (int attempt = 0; attempt < 32; ++attempt) {
            const auto refreshed = registry.refresh();
            if (!refreshed || *refreshed != 2u) {
                failed.store(true);
                break;
            }
        }
        finished.store(true);
    });

    start.store(true);
    while (!finished.load()) {
        const auto listed = registry.list();
        if (!listed || (listed->size() != 1u && listed->size() != 2u)) {
            failed.store(true);
            break;
        }
        const auto document = registry.load("alpha");
        if (!document || document->instructions != "Stable instructions.\n") {
            failed.store(true);
            break;
        }
        std::this_thread::yield();
    }
    refresher.join();

    EXPECT_FALSE(failed.load());
    const auto listed = registry.list();
    ASSERT_TRUE(listed);
    EXPECT_EQ(listed->size(), 2u);
}

TEST(SkillRegistryTest, DuplicateFrontmatterKeysAreRejected) {
    TemporaryDirectory temporary;
    writeFile(temporary.path() / "skills" / "docs" / "SKILL.md",
              "---\nname: docs\nname: alternate\ndescription: Guide.\n---\n");

    SkillRegistry registry(optionsFor(temporary.path() / "skills"));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(),
              ErrorCode::ERR_LLM_SKILL_INVALID_METADATA);
}

TEST(SkillRegistryTest, InvalidUtf8InIgnoredFrontmatterTextIsRejected) {
    TemporaryDirectory temporary;
    std::string manifest =
        "---\nname: docs\ndescription: Guide.\n# ignored comment ";
    manifest.append("\xc3", 1);
    manifest.append("\n---\nInstructions are loaded later.\n");
    writeFile(temporary.path() / "skills" / "docs" / "SKILL.md", manifest);

    SkillRegistry registry(optionsFor(temporary.path() / "skills"));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(), ErrorCode::ERR_LLM_SKILL_INVALID_CONTENT);
}

TEST(SkillRegistryTest, DuplicateNamesAcrossRootsAreRejected) {
    TemporaryDirectory temporary;
    const auto first_root = temporary.path() / "first";
    const auto second_root = temporary.path() / "second";
    writeSkill(first_root, "shared");
    writeSkill(second_root, "shared");

    auto options = optionsFor(first_root);
    options.roots.push_back(second_root);
    SkillRegistry registry(std::move(options));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(),
              ErrorCode::ERR_LLM_SKILL_DUPLICATE_NAME);
    EXPECT_FALSE(registry.list());
}

TEST(SkillRegistryTest, RejectsTraversalAndOversizedResources) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "docs");
    writeFile(package / "large.md", "123456789");

    auto options = optionsFor(root);
    options.max_resource_bytes = 8;
    SkillRegistry registry(std::move(options));
    ASSERT_TRUE(registry.refresh());

    const auto traversal = registry.readResource("docs", "../outside.md");
    ASSERT_FALSE(traversal);
    EXPECT_EQ(traversal.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);

    const auto absolute = registry.readResource(
        "docs", (temporary.path() / "outside.md").string());
    ASSERT_FALSE(absolute);
    EXPECT_EQ(absolute.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);

    const auto oversized = registry.readResource("docs", "large.md");
    ASSERT_FALSE(oversized);
    EXPECT_EQ(oversized.error().code(), ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT);
}

TEST(SkillRegistryTest, MissingResourceReturnsNotFound) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "docs");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    const auto resource = registry.readResource("docs", "references/missing.md");
    ASSERT_FALSE(resource);
    EXPECT_EQ(resource.error().code(), ErrorCode::ERR_LLM_SKILL_NOT_FOUND);
}

TEST(SkillRegistryTest, RejectsInvalidUtf8AndNulInResourcePaths) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "docs");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    const std::string invalid_utf8_path("\xc3\x28", 2);
    const auto invalid_utf8 = registry.readResource("docs", invalid_utf8_path);
    ASSERT_FALSE(invalid_utf8);
    EXPECT_EQ(invalid_utf8.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);

    const std::string nul_path("missing\0file.md", 15);
    const auto nul = registry.readResource("docs", nul_path);
    ASSERT_FALSE(nul);
    EXPECT_EQ(nul.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsSymlinkedResources) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "docs");
    const auto outside = temporary.path() / "outside.md";
    writeFile(outside, "outside package\n");

    std::error_code error;
    fs::create_symlink(outside, package / "linked.md", error);
    if (error) {
        GTEST_SKIP() << "Symlink creation is not permitted in this environment";
    }

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());
    const auto resource = registry.readResource("docs", "linked.md");
    ASSERT_FALSE(resource);
    EXPECT_EQ(resource.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsSymlinkedPackageDirectoryDuringDiscovery) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto target = writeSkill(temporary.path() / "external", "docs");
    fs::create_directories(root);

    std::error_code error;
    fs::create_directory_symlink(target, root / "docs", error);
    if (error) {
        GTEST_SKIP() << "Symlink creation is not permitted in this environment";
    }

    SkillRegistry registry(optionsFor(root));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsSymlinkedManifestDuringDiscovery) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = root / "docs";
    fs::create_directories(package);
    const auto outside = temporary.path() / "SKILL.md";
    writeFile(outside,
              "---\nname: docs\ndescription: External manifest.\n---\n");

    std::error_code error;
    fs::create_symlink(outside, package / "SKILL.md", error);
    if (error) {
        GTEST_SKIP() << "Symlink creation is not permitted in this environment";
    }

    SkillRegistry registry(optionsFor(root));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsManifestReplacedBySymlinkAfterRefresh) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "docs");
    const auto outside = temporary.path() / "outside.md";
    writeFile(outside, "---\nname: docs\ndescription: outside\n---\nsecret\n");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    std::error_code error;
    fs::remove(package / "SKILL.md", error);
    ASSERT_FALSE(error);
    fs::create_symlink(outside, package / "SKILL.md", error);
    if (error) {
        GTEST_SKIP() << "Symlink creation is not permitted in this environment";
    }

    const auto document = registry.load("docs");
    ASSERT_FALSE(document);
    EXPECT_EQ(document.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsPackageRootReplacedBySymlinkAfterRefresh) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "docs");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    const auto external_root = temporary.path() / "external_skills";
    const auto external_package = writeSkill(external_root, "docs");
    writeFile(external_package / "private.md", "external content\n");

    std::error_code error;
    const auto moved_root = temporary.path() / "original_skills";
    fs::rename(root, moved_root, error);
    if (error) {
        GTEST_SKIP() << "Root replacement is not permitted in this environment";
    }
    fs::create_directory_symlink(external_root, root, error);
    if (error) {
        GTEST_SKIP() << "Directory symlink creation is not permitted in this environment";
    }

    const auto document = registry.load("docs");
    ASSERT_FALSE(document);
    EXPECT_EQ(document.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);

    const auto resource = registry.readResource("docs", "private.md");
    ASSERT_FALSE(resource);
    EXPECT_EQ(resource.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsInvalidUtf8Resources) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "docs");
    writeFile(package / "invalid.txt", std::string("\xc3\x28", 2));

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());
    const auto resource = registry.readResource("docs", "invalid.txt");
    ASSERT_FALSE(resource);
    EXPECT_EQ(resource.error().code(), ErrorCode::ERR_LLM_SKILL_INVALID_CONTENT);
}

TEST(SkillRegistryTest, OptionalToolsRemainGatedByTheActiveMode) {
    TemporaryDirectory temporary;
    const auto package =
        writeSkill(temporary.path() / "skills", "howto", "Do the task.\n");
    writeFile(package / "references" / "guide.md", "Read the guide.\n");
    auto registry = std::make_shared<SkillRegistry>(
        optionsFor(temporary.path() / "skills"));
    ASSERT_TRUE(registry->refresh());

    ToolRegistry tools;
    registerSkillTools(tools, registry);

    ModeSpec mode;
    mode.id = "agent";
    auto result = tools.invokeTool("list_skills", {}, mode);
    EXPECT_TRUE(result.contains("error"));
    result = tools.invokeTool(
        "read_skill_resource",
        {{"name", "howto"}, {"path", "references/guide.md"}}, mode);
    EXPECT_TRUE(result.contains("error"));

    mode.tools_allowed = {"list_skills"};
    result = tools.invokeTool("list_skills", {}, mode);
    ASSERT_TRUE(result.contains("skills"));
    ASSERT_EQ(result["skills"].size(), 1u);
    EXPECT_EQ(result["skills"][0]["name"], "howto");

    result = tools.invokeTool("load_skill", {{"name", "howto"}}, mode);
    EXPECT_TRUE(result.contains("error"));
    result = tools.invokeTool(
        "read_skill_resource",
        {{"name", "howto"}, {"path", "references/guide.md"}}, mode);
    EXPECT_TRUE(result.contains("error"));

    mode.tools_allowed.push_back("load_skill");
    result = tools.invokeTool("load_skill", {{"name", "howto"}}, mode);
    ASSERT_TRUE(result.contains("instructions"));
    EXPECT_EQ(result["instructions"], "Do the task.\n");

    mode.tools_allowed.push_back("read_skill_resource");
    mode.tools_denied = {"read_skill_resource"};
    result = tools.invokeTool(
        "read_skill_resource",
        {{"name", "howto"}, {"path", "references/guide.md"}}, mode);
    EXPECT_TRUE(result.contains("error"));

    mode.tools_denied.clear();
    result = tools.invokeTool(
        "read_skill_resource",
        {{"name", "howto"}, {"path", "references/guide.md"}}, mode);
    ASSERT_TRUE(result.contains("content"));
    EXPECT_EQ(result["content"], "Read the guide.\n");
}

TEST(SkillRegistryTest, ToolNameConflictDoesNotRegisterPartialSkillTools) {
    ToolRegistry tools;
    themis::llm::ToolSpec conflicting;
    conflicting.name = "load_skill";
    tools.registerTool(conflicting, [](const themis::llm::json&,
                                       const ModeSpec&) {
        return themis::llm::json::object();
    });

    auto registry = std::make_shared<SkillRegistry>(SkillRegistryOptions{});
    EXPECT_THROW(registerSkillTools(tools, std::move(registry)),
                 std::invalid_argument);

    const auto names = tools.listTools();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names.front(), "load_skill");
}

} // namespace
