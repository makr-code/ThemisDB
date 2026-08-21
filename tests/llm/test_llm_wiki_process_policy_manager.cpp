// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "llm_wiki/process_policy_manager.h"

namespace {

std::filesystem::path makeTempDir() {
    auto root = std::filesystem::temp_directory_path() /
                "themisdb_llm_wiki_process_policy_tests";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    return root;
}

std::filesystem::path writePolicyFile(const std::filesystem::path& dir,
                                      const std::string& yaml_body) {
    const auto path = dir / "policy.yaml";
    std::ofstream out(path);
    out << yaml_body;
    out.close();
    return path;
}

std::string baselinePolicyYaml() {
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

} // namespace

TEST(LLMWikiProcessPolicyManagerTest, LoadsValidPolicyAndMaterializesFields) {
    const auto dir = makeTempDir();
    const auto path = writePolicyFile(dir, baselinePolicyYaml());

    themis::llm_wiki::LLMWikiProcessPolicy policy;
    const auto status = themis::llm_wiki::ProcessPolicyManager::loadFromYaml(path, policy);

    ASSERT_TRUE(status.ok()) << status.message;
    EXPECT_EQ(policy.version, 1);
    EXPECT_EQ(policy.policy_id, "test_policy_v1");
    EXPECT_EQ(policy.planner_owner, "prompt_engineering");
    EXPECT_FALSE(policy.second_planner_allowed);
    EXPECT_TRUE(policy.validate.enabled);
    EXPECT_TRUE(policy.policy_snapshot_required);
    EXPECT_TRUE(policy.require_reason_codes);
    ASSERT_EQ(policy.adjustable_knobs.size(), 1u);
    EXPECT_EQ(policy.adjustable_knobs.front(), "synthesize.max_evidence_items");
}

TEST(LLMWikiProcessPolicyManagerTest, RejectsSecondPlannerEnablement) {
    auto yaml = baselinePolicyYaml();
    const auto pos = yaml.find("second_planner_allowed: false");
    ASSERT_NE(pos, std::string::npos);
    yaml.replace(pos, std::string("second_planner_allowed: false").size(),
                 "second_planner_allowed: true");

    const auto dir = makeTempDir();
    const auto path = writePolicyFile(dir, yaml);

    themis::llm_wiki::LLMWikiProcessPolicy policy;
    const auto status = themis::llm_wiki::ProcessPolicyManager::loadFromYaml(path, policy);

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code, themis::llm_wiki::ProcessPolicyStatus::Code::ValidationError);
    EXPECT_NE(status.message.find("second_planner_allowed"), std::string::npos);
}

TEST(LLMWikiProcessPolicyManagerTest, RejectsMissingHardBoundForAdjustableKnob) {
    auto yaml = baselinePolicyYaml();
    const auto remove_start = yaml.find("  hard_bounds:");
    const auto remove_end = yaml.find("  safety:");
    ASSERT_NE(remove_start, std::string::npos);
    ASSERT_NE(remove_end, std::string::npos);
    yaml.erase(remove_start, remove_end - remove_start);

    const auto dir = makeTempDir();
    const auto path = writePolicyFile(dir, yaml);

    themis::llm_wiki::LLMWikiProcessPolicy policy;
    const auto status = themis::llm_wiki::ProcessPolicyManager::loadFromYaml(path, policy);

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code, themis::llm_wiki::ProcessPolicyStatus::Code::ValidationError);
    EXPECT_NE(status.message.find("Missing hard bound"), std::string::npos);
}

  TEST(LLMWikiProcessPolicyManagerTest, RejectsUnknownRootPropertyViaSchemaGate) {
    auto yaml = baselinePolicyYaml();
    yaml += "unexpected_root_flag: true\n";

    const auto dir = makeTempDir();
    const auto path = writePolicyFile(dir, yaml);

    themis::llm_wiki::LLMWikiProcessPolicy policy;
    const auto status = themis::llm_wiki::ProcessPolicyManager::loadFromYaml(path, policy);

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code, themis::llm_wiki::ProcessPolicyStatus::Code::ValidationError);
    EXPECT_NE(status.message.find("Schema validation failed"), std::string::npos);
  }
