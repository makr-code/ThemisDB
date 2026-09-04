// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "wikipedia/llm_wiki_plugin_impl.h"

namespace {

using themis::plugins::llm_wiki::LLMWikiPluginImpl;
using themis::plugins::llm_wiki::Status;

std::filesystem::path makeTempPolicyFile(const std::string& yaml) {
    const auto dir = std::filesystem::temp_directory_path() /
                     "themisdb_llm_wiki_block4_policy";
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    const auto path = dir / "policy.yaml";
    std::ofstream out(path);
    out << yaml;
    out.close();
    return path;
}

std::string validPolicyYaml() {
    return R"YAML(version: 1
policy_id: test_policy_v1
mode: shadow
orchestration:
  planner_owner: prompt_engineering
  second_planner_allowed: false
  interactive_timeout_ms: 1500
stages:
  ingest:
    enabled: true
    schedule: near_realtime
  extract:
    enabled: true
    schedule: near_realtime
  synthesize:
    enabled: true
    schedule: interactive
    max_evidence_items: 8
    min_provenance_confidence: 0.7
  validate:
    enabled: true
    schedule: interactive
  re_anchor:
    enabled: true
    schedule: batch
governance:
  policy_snapshot_required: true
  require_reason_codes: true
ml_control:
  adjustable_knobs:
    - synthesize.max_evidence_items
  hard_bounds:
    synthesize.max_evidence_items:
      min: 8
      max: 64
  safety:
    never_adjust:
      - governance.policy_snapshot_required
      - orchestration.second_planner_allowed
      - stages.validate.fail_closed
      - stages.ingest.requires
)YAML";
}

#ifndef THEMISDB_LLM_WIKI_ENTERPRISE_ENABLED
TEST(LLMWikiBlock4BackendGateTest, InitializeDeniedWhenEditionGateClosed) {
    LLMWikiPluginImpl plugin;
    const Status st = plugin.initialize("{}");
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

TEST(LLMWikiBlock4BackendGateTest, MissingProcessPolicyPathFailsClosed) {
    LLMWikiPluginImpl plugin;
    const std::string cfg = R"({
      "process_policy_path":"/tmp/themisdb_missing_llm_wiki_policy.yaml"
    })";
    const Status st = plugin.initialize(cfg);
    EXPECT_FALSE(st.ok());
    EXPECT_EQ(st.code, Status::Code::Error);
    EXPECT_NE(st.message.find("process policy"), std::string::npos);
}

TEST(LLMWikiBlock4BackendGateTest, ValidProcessPolicyPathAllowsInitialize) {
    const auto policy_path = makeTempPolicyFile(validPolicyYaml());
    LLMWikiPluginImpl plugin;
    const std::string cfg = std::string(R"({
      "process_policy_path":")") + policy_path.string() + R"(",
      "enforce_process_policy":true
    })";
    const Status st = plugin.initialize(cfg);
    EXPECT_TRUE(st.ok()) << st.message;
}

TEST(LLMWikiBlock4BackendGateTest, FailOpenRequiresShadowPolicyMode) {
    auto yaml = validPolicyYaml();
    const auto pos = yaml.find("mode: shadow");
    ASSERT_NE(pos, std::string::npos);
    yaml.replace(pos, std::string("mode: shadow").size(), "mode: production");
    const auto policy_path = makeTempPolicyFile(yaml);

    LLMWikiPluginImpl plugin;
    const std::string cfg = std::string(R"({
      "process_policy_path":")") + policy_path.string() + R"(",
      "fail_open":true
    })";
    const Status st = plugin.initialize(cfg);
    EXPECT_FALSE(st.ok());
    EXPECT_EQ(st.code, Status::Code::Error);
    EXPECT_NE(st.message.find("allowed only when process policy mode is 'shadow'"),
              std::string::npos);
}

TEST(LLMWikiBlock4BackendGateTest, WikipediaDumpRequiresRuntimeLicenseFlag) {
    LLMWikiPluginImpl plugin;
    ASSERT_TRUE(plugin.initialize("{}").ok());

    const auto res = plugin.ingestWikipediaDump("/tmp/nonexistent_dump.xml.bz2", {});
    EXPECT_GT(res.errors, 0);
    ASSERT_FALSE(res.failed_files.empty());
    EXPECT_NE(res.failed_files.front().find("permission_denied"), std::string::npos);
}
#endif

}  // namespace
