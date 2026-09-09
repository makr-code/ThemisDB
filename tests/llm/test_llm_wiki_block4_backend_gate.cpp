// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "wikipedia/llm_wiki_plugin_impl.h"

#include <filesystem>
#include <fstream>

namespace {

using themis::plugins::llm_wiki::LLMWikiPluginImpl;
using themis::plugins::llm_wiki::Status;

#ifndef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
TEST(LLMWikiBlock4BackendGateTest, InitializeDeniedWhenEditionGateClosed) {
    LLMWikiPluginImpl plugin;
    const Status st = plugin.initialize(std::string("{}"));
    EXPECT_EQ(st.code, Status::Code::PermissionDenied);
}
#else
TEST(LLMWikiBlock4BackendGateTest, RocksDbConfiguredFailsClosedByDefault) {
    LLMWikiPluginImpl plugin;
    const std::string cfg =
        R"({"rocksdb_dir":"/proc/themisdb_block4_wiki_store"})";
    const Status st = plugin.initialize(cfg);
    EXPECT_FALSE(st.ok());
    EXPECT_EQ(st.code, Status::Code::Error);
}

TEST(LLMWikiBlock4BackendGateTest, RocksDbConfiguredAllowsExplicitDegradedMode) {
    LLMWikiPluginImpl plugin;
    const std::string cfg =
        R"({"rocksdb_dir":"/proc/themisdb_block4_wiki_store","fail_open":true})";
    const Status st = plugin.initialize(cfg);
    ASSERT_TRUE(st.ok()) << st.message;

    const auto stats = plugin.stats({});
    EXPECT_FALSE(stats.rocksdb_backed);
}

TEST(LLMWikiBlock4BackendGateTest, WikipediaDumpRequiresRuntimeLicenseFlag) {
    LLMWikiPluginImpl plugin;
    ASSERT_TRUE(plugin.initialize(std::string("{}")).ok());

    const auto res = plugin.ingestWikipediaDump("/tmp/nonexistent_dump.xml.bz2", {});
    EXPECT_GT(res.errors, 0);
    ASSERT_FALSE(res.failed_files.empty());
    EXPECT_NE(res.failed_files.front().find("permission_denied"), std::string::npos);
}

TEST(LLMWikiBlock4BackendGateTest, InvalidProcessPolicyFailsClosedAtInitialize) {
    const auto dir = std::filesystem::temp_directory_path() / "themisdb_llm_wiki_policy_init";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    const auto policy_path = dir / "policy.yaml";
    std::ofstream out(policy_path.string());
    out << "version: 1\npolicy_id: invalid\norchestration:\n  second_planner_allowed: true\n";
    out.close();

    LLMWikiPluginImpl plugin;
    const std::string cfg = std::string("{\"process_policy_path\":\"") +
        policy_path.string() + "\"}";
    const Status st = plugin.initialize(cfg);
    EXPECT_FALSE(st.ok());
}

TEST(LLMWikiBlock4BackendGateTest, HotReloadRetainsLastKnownGoodOnInvalidUpdate) {
    const auto dir = std::filesystem::temp_directory_path() / "themisdb_llm_wiki_policy_reload";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    const auto policy_path = dir / "policy.yaml";
    std::ofstream out(policy_path.string());
    out << "version: 1\n"
           "policy_id: test_policy\n"
           "mode: shadow\n"
           "orchestration:\n"
           "  planner_owner: prompt_engineering\n"
           "  second_planner_allowed: false\n"
           "  interactive_timeout_ms: 1500\n"
           "stages:\n"
           "  ingest: { enabled: true, schedule: near_realtime }\n"
           "  extract: { enabled: true, schedule: near_realtime }\n"
           "  synthesize: { enabled: true, schedule: interactive, max_evidence_items: 8, min_provenance_confidence: 0.7 }\n"
           "  validate: { enabled: true, schedule: interactive }\n"
           "  re_anchor: { enabled: true, schedule: batch }\n"
           "governance:\n"
           "  policy_snapshot_required: true\n"
           "  require_reason_codes: true\n"
           "ml_control:\n"
           "  adjustable_knobs: [synthesize.max_evidence_items]\n"
           "  hard_bounds:\n"
           "    synthesize.max_evidence_items: { min: 8, max: 64 }\n"
           "  safety:\n"
           "    never_adjust: [governance.policy_snapshot_required, orchestration.second_planner_allowed, stages.validate.fail_closed, stages.ingest.requires]\n";
    out.close();

    LLMWikiPluginImpl plugin;
    const std::string cfg = std::string("{\"process_policy_path\":\"") +
        policy_path.string() + "\",\"process_policy_hot_reload\":true}";
    ASSERT_TRUE(plugin.initialize(cfg).ok());

    std::ofstream bad(policy_path.string(), std::ios::trunc);
    bad << "version: 1\npolicy_id: bad\norchestration:\n  second_planner_allowed: true\n";
    bad.close();

    const auto result = plugin.query("safe query");
    EXPECT_FALSE(result.query_flagged_for_prompt_injection);
}
#endif

}  // namespace
