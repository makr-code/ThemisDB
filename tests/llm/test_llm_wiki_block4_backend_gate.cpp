// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <chrono>
#include <thread>

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

std::string policyWithDisabledStage(const std::string& stage_name) {
    std::string yaml = validPolicyYaml();
    const std::string needle = "  " + stage_name + ":\n    enabled: true";
    const auto pos = yaml.find(needle);
    if (pos != std::string::npos) {
        yaml.replace(pos, needle.size(),
                     "  " + stage_name + ":\n    enabled: false");
    }
    return yaml;
}

std::string policyWithStageSchedule(const std::string& stage_name,
                                    const std::string& schedule) {
    std::string yaml = validPolicyYaml();
    const std::string needle =
        "  " + stage_name + ":\n    enabled: true\n    schedule: near_realtime";
    const std::string replacement =
        "  " + stage_name + ":\n    enabled: true\n    schedule: " + schedule;
    const auto pos = yaml.find(needle);
    if (pos != std::string::npos) {
        yaml.replace(pos, needle.size(), replacement);
    }
    return yaml;
}

std::filesystem::path makeTempWorkspaceRoot() {
    const auto dir = std::filesystem::temp_directory_path() /
                     "themisdb_llm_wiki_block4_workspace";
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    return dir;
}

std::filesystem::path writeTempMarkdownSource(const std::filesystem::path& root) {
    const auto source_dir = root / "source";
    std::error_code ec;
    std::filesystem::create_directories(source_dir, ec);
    const auto file_path = source_dir / "doc.md";
    std::ofstream out(file_path);
    out << "# Test\n\nalpha beta gamma";
    out.close();
    return file_path;
}

std::string readFileOrEmpty(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(in),
                       std::istreambuf_iterator<char>());
}

void overwriteFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::trunc);
    out << content;
    out.close();
}

std::string makePluginConfigJson(const std::filesystem::path& workspace_root,
                                 const std::filesystem::path& policy_path) {
    return std::string("{\"workspace_root\":\"") + workspace_root.string() +
           "\",\"process_policy_path\":\"" + policy_path.string() + "\"}";
}

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
    ASSERT_TRUE(plugin.initialize(std::string("{}")).ok());

    const auto res = plugin.ingestWikipediaDump("/tmp/nonexistent_dump.xml.bz2", {});
    EXPECT_GT(res.errors, 0);
    ASSERT_FALSE(res.failed_files.empty());
    EXPECT_NE(res.failed_files.front().find("permission_denied"), std::string::npos);
}

TEST(LLMWikiBlock4BackendGateTest, IngestGateDenyPersistsReasonCodeEvidence) {
    const auto workspace = makeTempWorkspaceRoot();
    const auto policy_path = makeTempPolicyFile(policyWithDisabledStage("ingest"));
    const auto source_file = writeTempMarkdownSource(workspace);
    LLMWikiPluginImpl plugin;
    const std::string cfg = makePluginConfigJson(workspace, policy_path);
    ASSERT_TRUE(plugin.initialize(cfg).ok());

    const auto ingest_res = plugin.ingest(source_file.string(), {});
    EXPECT_EQ(ingest_res.errors, 1);
    ASSERT_FALSE(ingest_res.failed_files.empty());
    EXPECT_NE(ingest_res.failed_files.front().find("LLMWIKI_DENY_STAGE_INGEST_DISABLED"),
              std::string::npos);

    const auto evidence_path = workspace / "wiki" / "governance_evidence.jsonl";
    const auto evidence = readFileOrEmpty(evidence_path);
    EXPECT_NE(evidence.find("LLMWIKI_DENY_STAGE_INGEST_DISABLED"), std::string::npos);
}

TEST(LLMWikiBlock4BackendGateTest, ExtractValidateSynthesizeDenyPersistReasonCodeEvidence) {
    const auto workspace = makeTempWorkspaceRoot();
    const auto source_file = writeTempMarkdownSource(workspace);
    (void)source_file;
    {
        const auto policy_path = makeTempPolicyFile(policyWithDisabledStage("extract"));
        LLMWikiPluginImpl plugin;
        const std::string cfg = makePluginConfigJson(workspace, policy_path);
        ASSERT_TRUE(plugin.initialize(cfg).ok());
        const auto query_res = plugin.query("alpha", {});
        EXPECT_TRUE(query_res.candidates.empty());
    }

    {
        const auto policy_path = makeTempPolicyFile(policyWithDisabledStage("validate"));
        LLMWikiPluginImpl plugin;
        const std::string cfg = makePluginConfigJson(workspace, policy_path);
        ASSERT_TRUE(plugin.initialize(cfg).ok());
        const auto query_res = plugin.query("alpha", {});
        EXPECT_TRUE(query_res.candidates.empty());
    }

    {
        const auto policy_path = makeTempPolicyFile(policyWithDisabledStage("synthesize"));
        LLMWikiPluginImpl plugin;
        const std::string cfg = makePluginConfigJson(workspace, policy_path);
        ASSERT_TRUE(plugin.initialize(cfg).ok());
        const auto query_res = plugin.query("alpha", {});
        EXPECT_TRUE(query_res.candidates.empty());
    }

    const auto evidence_path = workspace / "wiki" / "governance_evidence.jsonl";
    const auto evidence = readFileOrEmpty(evidence_path);
    EXPECT_NE(evidence.find("LLMWIKI_DENY_STAGE_EXTRACT_DISABLED"), std::string::npos);
    EXPECT_NE(evidence.find("LLMWIKI_DENY_STAGE_VALIDATE_DISABLED"), std::string::npos);
    EXPECT_NE(evidence.find("LLMWIKI_DENY_STAGE_SYNTHESIZE_DISABLED"), std::string::npos);
}

TEST(LLMWikiBlock4BackendGateTest, BatchScheduleDeniesImmediateExtractExecution) {
    const auto workspace = makeTempWorkspaceRoot();
    const auto policy_path =
        makeTempPolicyFile(policyWithStageSchedule("extract", "batch"));
    LLMWikiPluginImpl plugin;
    ASSERT_TRUE(plugin.initialize(makePluginConfigJson(workspace, policy_path)).ok());

    const auto query_res = plugin.query("alpha", {});
    EXPECT_TRUE(query_res.candidates.empty());

    const auto evidence_path = workspace / "wiki" / "governance_evidence.jsonl";
    const auto evidence = readFileOrEmpty(evidence_path);
    EXPECT_NE(evidence.find("LLMWIKI_DENY_STAGE_EXTRACT_DISABLED_SCHEDULE_BATCH_ONLY"),
              std::string::npos);
}

TEST(LLMWikiBlock4BackendGateTest, HotReloadAppliesChangedPolicyAndEnforcesNewStageGate) {
    const auto workspace = makeTempWorkspaceRoot();
    const auto source_file = writeTempMarkdownSource(workspace);
    const auto policy_path = makeTempPolicyFile(validPolicyYaml());

    LLMWikiPluginImpl plugin;
    ASSERT_TRUE(plugin.initialize(makePluginConfigJson(workspace, policy_path)).ok());
    const auto ingest_res = plugin.ingest(source_file.string(), {});
    EXPECT_EQ(ingest_res.errors, 0);

    auto query_res = plugin.query("alpha", {});
    EXPECT_FALSE(query_res.candidates.empty());

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    overwriteFile(policy_path, policyWithDisabledStage("extract"));

    query_res = plugin.query("alpha", {});
    EXPECT_TRUE(query_res.candidates.empty());

    const auto evidence_path = workspace / "wiki" / "governance_evidence.jsonl";
    const auto evidence = readFileOrEmpty(evidence_path);
    EXPECT_NE(evidence.find("LLMWIKI_DENY_STAGE_EXTRACT_DISABLED"), std::string::npos);
}

TEST(LLMWikiBlock4BackendGateTest, HotReloadRejectsInvalidPolicyFailClosed) {
    const auto workspace = makeTempWorkspaceRoot();
    const auto source_file = writeTempMarkdownSource(workspace);
    const auto policy_path = makeTempPolicyFile(validPolicyYaml());

    LLMWikiPluginImpl plugin;
    ASSERT_TRUE(plugin.initialize(makePluginConfigJson(workspace, policy_path)).ok());
    const auto ingest_res = plugin.ingest(source_file.string(), {});
    EXPECT_EQ(ingest_res.errors, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    overwriteFile(policy_path, "invalid_yaml: [");

    const auto query_res = plugin.query("alpha", {});
    EXPECT_TRUE(query_res.candidates.empty());

    const auto evidence_path = workspace / "wiki" / "governance_evidence.jsonl";
    const auto evidence = readFileOrEmpty(evidence_path);
    EXPECT_NE(evidence.find("LLMWIKI_DENY_POLICY_RELOAD_INVALID"), std::string::npos);
}
#endif

}  // namespace
