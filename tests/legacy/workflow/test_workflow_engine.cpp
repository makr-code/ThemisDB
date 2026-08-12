/*
 * ThemisDB — WorkflowEngine + StepRegistry Tests
 *
 * Tests for:
 *   StepRegistry                    (registration/lookup)  SR-01..SR-05
 *   WorkflowEngine profile loading  (YAML/JSON profiles)   WE-01..WE-06
 *   WorkflowEngine profile selection (MIME + glob)         WE-07..WE-10
 *   WorkflowEngine execution        (step dispatch)        WE-11..WE-15
 *   YAML-native features                                   WE-16
 *   DLL sandbox                                            SBX-01..SBX-03
 *
 * Acceptance criteria:
 *
 * StepRegistry (SR-01..SR-05)
 *   SR-01  Empty registry: getStep() returns nullptr for unknown name
 *   SR-02  registerStep() adds the step; hasStep() returns true
 *   SR-03  listSteps() returns all registered step names
 *   SR-04  registerStep() with duplicate name returns an error
 *   SR-05  unloadStep() removes the step; hasStep() returns false afterward
 *
 * WorkflowEngine profile loading (WE-01..WE-06)
 *   WE-01  loadProfile() with nonexistent path returns ERR_WORKFLOW_PROFILE_NOT_FOUND
 *   WE-02  loadProfile() with invalid JSON returns ERR_WORKFLOW_PROFILE_INVALID
 *   WE-03  loadProfile() with missing 'name' field returns ERR_WORKFLOW_PROFILE_INVALID
 *   WE-04  loadProfile() with missing 'steps' field returns ERR_WORKFLOW_PROFILE_INVALID
 *   WE-05  loadProfile() with valid profile succeeds; listProfiles() returns the name
 *   WE-06  loadProfile() with same name twice is idempotent (no duplicate)
 *
 * WorkflowEngine profile selection (WE-07..WE-10)
 *   WE-07  selectProfile() returns nullptr when no profiles are loaded
 *   WE-08  selectProfile() returns matching profile for exact MIME match
 *   WE-09  selectProfile() returns "default" when no specific match found
 *   WE-10  selectProfile() prefers specific profiles over "default"
 *
 * WorkflowEngine execution (WE-11..WE-15)
 *   WE-11  execute() returns ERR_WORKFLOW_NO_MATCHING_PROFILE when no profile matches
 *   WE-12  execute() runs steps in order; step results accumulate in context
 *   WE-13  execute() skips step when canHandle() returns false
 *   WE-14  execute() skips step on failure when on_failure=skip
 *   WE-15  executeWithProfile() uses the named profile regardless of MIME
 *   WE-16  YAML comment lines tolerated when yaml-cpp is available
 *   SBX-01 loadStepPlugin() rejects path outside allowedPaths
 *   SBX-02 loadStepPlugin() rejects missing sidecar when allowedMime is set
 *   SBX-03 empty manifest is permissive; normal load error for missing .so
 */

#include <gtest/gtest.h>

#include "ingestion/workflow_engine.h"
#include "ingestion/extraction_context.h"
#include "ingestion/file_manifest.h"
#include "ingestion/base_entity.h"
#include "utils/error_registry.h"

#include <fstream>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace themis;

using namespace themis::ingestion;
using namespace themis::errors;

// ─────────────────────────────────────────────────────────────────────────────
// Stub step implementations used by tests
// ─────────────────────────────────────────────────────────────────────────────

class AppendTextStep : public IIngestionStep {
public:
    explicit AppendTextStep(std::string append) : append_(std::move(append)) {}

    const char* getName()    const override { return "test.append_text"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx,
                         const StepConfig&) override {
        ctx.raw_text += append_;
        return {};
    }

private:
    std::string append_;
};

class FailStep : public IIngestionStep {
public:
    const char* getName()    const override { return "test.fail"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext&, const StepConfig&) override {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED, "always fails"});
    }
};

class PdfOnlyStep : public IIngestionStep {
public:
    const char* getName()    const override { return "test.pdf_only"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(const char*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    std::vector<std::string> supportedMimeTypes() const override {
        return {"application/pdf"};
    }

    Result<void> execute(ExtractionContext& ctx, const StepConfig&) override {
        ctx.raw_text += "[pdf_only_ran]";
        return {};
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string writeTempProfile(const std::string& content,
                                     const std::string& name = "test_profile.json") {
    const auto path = (std::filesystem::temp_directory_path() / name).string();
    std::ofstream f(path);
    f << content;
    return path;
}

static ExtractionContext makeCtx(const std::string& mime = "",
                                  const std::string& stem = "test",
                                  const std::string& ext  = ".txt") {
    ExtractionContext ctx;
    ctx.manifest.detected_mime = mime;
    ctx.manifest.filename_stem = stem;
    ctx.manifest.extension     = ext;
    ctx.manifest.file_id       = "sha256:aabbcc";
    return ctx;
}

// ─────────────────────────────────────────────────────────────────────────────
// SR — StepRegistry tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(StepRegistry, SR01_EmptyRegistryReturnsNullptr) {
    StepRegistry reg;
    EXPECT_EQ(reg.getStep("unknown"), nullptr);
    EXPECT_FALSE(reg.hasStep("unknown"));
}

TEST(StepRegistry, SR02_RegisterAddsStep) {
    StepRegistry reg;
    auto step = std::make_shared<AppendTextStep>("hello");
    auto res = reg.registerStep("test.hello", step);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_TRUE(reg.hasStep("test.hello"));
    EXPECT_NE(reg.getStep("test.hello"), nullptr);
}

TEST(StepRegistry, SR03_ListStepsReturnsAll) {
    StepRegistry reg;
    reg.registerStep("test.a", std::make_shared<AppendTextStep>("a"));
    reg.registerStep("test.b", std::make_shared<AppendTextStep>("b"));
    const auto list = reg.listSteps();
    EXPECT_EQ(list.size(), 2u);
}

TEST(StepRegistry, SR04_DuplicateRegistrationIsError) {
    StepRegistry reg;
    auto step = std::make_shared<AppendTextStep>("x");
    reg.registerStep("test.dup", step);
    auto res = reg.registerStep("test.dup", step);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code(), ErrorCode::ERR_WORKFLOW_STEP_ALREADY_REGISTERED);
}

TEST(StepRegistry, SR05_UnloadRemovesStep) {
    StepRegistry reg;
    reg.registerStep("test.rm", std::make_shared<AppendTextStep>("rm"));
    ASSERT_TRUE(reg.hasStep("test.rm"));
    auto res = reg.unloadStep("test.rm");
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_FALSE(reg.hasStep("test.rm"));
}

// ─────────────────────────────────────────────────────────────────────────────
// WE — WorkflowEngine profile loading
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowEngine, WE01_NonexistentProfileReturnsError) {
    WorkflowEngine engine;
    auto res = engine.loadProfile("/nonexistent/path/profile.json");
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code(), ErrorCode::ERR_WORKFLOW_PROFILE_NOT_FOUND);
}

TEST(WorkflowEngine, WE02_InvalidJsonReturnsError) {
    const auto path = writeTempProfile("{ not valid json >>>", "bad_json.json");
    WorkflowEngine engine;
    auto res = engine.loadProfile(path);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code(), ErrorCode::ERR_WORKFLOW_PROFILE_INVALID);
}

TEST(WorkflowEngine, WE03_MissingNameReturnsError) {
    const auto path = writeTempProfile(
        R"({"steps":[]})", "no_name.json");
    WorkflowEngine engine;
    auto res = engine.loadProfile(path);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code(), ErrorCode::ERR_WORKFLOW_PROFILE_INVALID);
}

TEST(WorkflowEngine, WE04_MissingStepsReturnsError) {
    const auto path = writeTempProfile(
        R"({"name":"test"})", "no_steps.json");
    WorkflowEngine engine;
    auto res = engine.loadProfile(path);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code(), ErrorCode::ERR_WORKFLOW_PROFILE_INVALID);
}

TEST(WorkflowEngine, WE05_ValidProfileLoadsSuccessfully) {
    const auto path = writeTempProfile(
        R"({"name":"test-profile","steps":[]})", "valid.json");
    WorkflowEngine engine;
    auto res = engine.loadProfile(path);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    const auto profiles = engine.listProfiles();
    EXPECT_EQ(profiles.size(), 1u);
    EXPECT_EQ(profiles[0], "test-profile");
}

TEST(WorkflowEngine, WE06_DuplicateProfileLoadIsIdempotent) {
    const auto path = writeTempProfile(
        R"({"name":"test-profile","steps":[]})", "valid2.json");
    WorkflowEngine engine;
    engine.loadProfile(path);
    engine.loadProfile(path);  // second call
    EXPECT_EQ(engine.listProfiles().size(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// WE — profile selection
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowEngine, WE07_SelectProfileWhenNoneLoaded) {
    WorkflowEngine engine;
    EXPECT_EQ(engine.selectProfile("application/pdf", "test.pdf"), nullptr);
}

TEST(WorkflowEngine, WE08_SelectProfileExactMimeMatch) {
    const auto path = writeTempProfile(
        R"({"name":"pdf-profile","file_patterns":{"mime_types":["application/pdf"]},"steps":[]})",
        "pdf.json");
    WorkflowEngine engine;
    engine.loadProfile(path);
    const auto* p = engine.selectProfile("application/pdf", "test.pdf");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->name, "pdf-profile");
}

TEST(WorkflowEngine, WE09_SelectProfileFallsBackToDefault) {
    const auto path = writeTempProfile(
        R"({"name":"default","steps":[]})", "def.json");
    WorkflowEngine engine;
    engine.loadProfile(path);
    const auto* p = engine.selectProfile("application/pdf", "test.pdf");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->name, "default");
}

TEST(WorkflowEngine, WE10_SpecificProfilePreferredOverDefault) {
    const auto pdf_path = writeTempProfile(
        R"({"name":"pdf-profile","file_patterns":{"mime_types":["application/pdf"]},"steps":[]})",
        "pdf2.json");
    const auto def_path = writeTempProfile(
        R"({"name":"default","steps":[]})", "def2.json");
    WorkflowEngine engine;
    engine.loadProfile(pdf_path);
    engine.loadProfile(def_path);
    const auto* p = engine.selectProfile("application/pdf", "test.pdf");
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->name, "pdf-profile");
}

// ─────────────────────────────────────────────────────────────────────────────
// WE — execution
// ─────────────────────────────────────────────────────────────────────────────

TEST(WorkflowEngine, WE11_ExecuteNoMatchingProfileReturnsError) {
    WorkflowEngine engine;
    auto ctx = makeCtx("application/pdf");
    auto res = engine.execute(ctx);
    ASSERT_FALSE(res.has_value());
    EXPECT_EQ(res.error().code(), ErrorCode::ERR_WORKFLOW_NO_MATCHING_PROFILE);
}

TEST(WorkflowEngine, WE12_ExecuteRunsStepsInOrder) {
    const auto path = writeTempProfile(
        R"({
          "name": "default",
          "steps": [
            {"name":"a","plugin":"test.a","on_failure":"abort"},
            {"name":"b","plugin":"test.b","on_failure":"abort"}
          ]
        })",
        "order.json");
    WorkflowEngine engine;
    engine.loadProfile(path);
    engine.stepRegistry().registerStep("test.a",
        std::make_shared<AppendTextStep>("A"));
    engine.stepRegistry().registerStep("test.b",
        std::make_shared<AppendTextStep>("B"));

    auto ctx = makeCtx("text/plain");
    auto res = engine.execute(ctx);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_EQ(ctx.raw_text, "AB");
}

TEST(WorkflowEngine, WE13_StepSkippedWhenCanHandleReturnsFalse) {
    const auto path = writeTempProfile(
        R"({
          "name": "default",
          "steps": [{"name":"pdf_only","plugin":"test.pdf_only","on_failure":"abort"}]
        })",
        "pdf_only.json");
    WorkflowEngine engine;
    engine.loadProfile(path);
    engine.stepRegistry().registerStep("test.pdf_only",
        std::make_shared<PdfOnlyStep>());

    auto ctx = makeCtx("text/plain");  // not PDF
    auto res = engine.execute(ctx);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    // Step must NOT have run (MIME mismatch)
    EXPECT_EQ(ctx.raw_text, "");
}

TEST(WorkflowEngine, WE14_StepSkippedOnFailureWhenOnFailureSkip) {
    const auto path = writeTempProfile(
        R"({
          "name": "default",
          "steps": [
            {"name":"fail","plugin":"test.fail","on_failure":"skip"},
            {"name":"ok",  "plugin":"test.ok",  "on_failure":"abort"}
          ]
        })",
        "skip.json");
    WorkflowEngine engine;
    engine.loadProfile(path);
    engine.stepRegistry().registerStep("test.fail",
        std::make_shared<FailStep>());
    engine.stepRegistry().registerStep("test.ok",
        std::make_shared<AppendTextStep>("OK"));

    auto ctx = makeCtx();
    auto res = engine.execute(ctx);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_EQ(ctx.raw_text, "OK");
    EXPECT_FALSE(ctx.warnings.empty());
}

TEST(WorkflowEngine, WE15_ExecuteWithProfileByName) {
    const auto path = writeTempProfile(
        R"({
          "name": "custom",
          "steps": [{"name":"a","plugin":"test.custom","on_failure":"abort"}]
        })",
        "custom.json");
    WorkflowEngine engine;
    engine.loadProfile(path);
    engine.stepRegistry().registerStep("test.custom",
        std::make_shared<AppendTextStep>("custom"));

    auto ctx = makeCtx("text/plain");
    auto res = engine.executeWithProfile("custom", ctx);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_EQ(ctx.raw_text, "custom");
}

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowEngineFocusedTests fixture — provides tmp_dir_ + writeFile() helper
// ─────────────────────────────────────────────────────────────────────────────

class WorkflowEngineFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = (std::filesystem::temp_directory_path()
                    / "themis_focused_tests").string();
        std::filesystem::create_directories(tmp_dir_);
        engine_ = std::make_unique<WorkflowEngine>();
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }

    void writeFile(const std::string& path, const std::string& content) {
        std::ofstream f(path);
        f << content;
    }

    std::string tmp_dir_;
    std::unique_ptr<WorkflowEngine> engine_;
};

// WE-16: YAML-native feature — YAML comment lines are tolerated
// (nlohmann/json can't parse them in the YAML node position; yaml-cpp handles them natively)
TEST_F(WorkflowEngineFocusedTests, WE16_YamlNativeCommentToleratedOrFallback) {
    const std::string yaml_content = R"(
# This is a YAML comment — valid YAML, invalid JSON
name: yaml_comment_test
steps:
  - name: step_one
    plugin: builtin.parse_text
)";
    const std::string path = tmp_dir_ + "/yaml_comment_test.yaml";
    writeFile(path, yaml_content);
    auto result = engine_->loadProfile(path);
#ifdef HAVE_YAML_CPP
    EXPECT_TRUE(result.has_value()) << "yaml-cpp should parse YAML comments; error: "
                                    << (result ? "" : result.error().message());
    auto profiles = engine_->listProfiles();
    EXPECT_NE(std::find(profiles.begin(), profiles.end(), "yaml_comment_test"), profiles.end());
#else
    // Without yaml-cpp the comment may cause parse failure — that's acceptable
    [[maybe_unused]] auto& unused = result;
#endif
}

// SBX-01: DLL sandbox — path outside allowedPaths rejected
TEST_F(WorkflowEngineFocusedTests, SBX01_SandboxRejectsPathOutsideAllowedPaths) {
    StepRegistry registry;
    StepPluginManifest manifest;
    manifest.allowed_paths = {"/opt/themis/plugins"};  // not /tmp
    auto result = registry.loadStepPlugin(
        "test.plugin",
        "/tmp/some_plugin.so",
        manifest);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ErrorCode::ERR_WORKFLOW_PLUGIN_LOAD_FAILED);
    EXPECT_NE(result.error().message().find("allowedPaths"), std::string::npos);
}

// SBX-02: DLL sandbox — missing sidecar .manifest.json when allowedMime is set
TEST_F(WorkflowEngineFocusedTests, SBX02_SandboxRejectsMissingSidecar) {
    StepRegistry registry;
    StepPluginManifest manifest;
    // Allow any path but restrict MIME
    manifest.allowed_mime_types = {"application/x-themis-step"};
    auto result = registry.loadStepPlugin(
        "test.plugin",
        "/tmp/no_sidecar_plugin.so",
        manifest);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ErrorCode::ERR_WORKFLOW_PLUGIN_LOAD_FAILED);
    EXPECT_NE(result.error().message().find("manifest.json"), std::string::npos);
}

// SBX-03: DLL sandbox — empty manifest allows any path; normal load error for missing .so
TEST_F(WorkflowEngineFocusedTests, SBX03_EmptyManifestPermissive) {
    StepRegistry registry;
    StepPluginManifest empty_manifest;  // no constraints
    // Sandbox should not reject — normal load failure expected for missing .so
    auto result = registry.loadStepPlugin(
        "test.plugin",
        "/tmp/nonexistent_plugin_xyz.so",
        empty_manifest);
    // Must fail with library load error, NOT sandbox error
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ErrorCode::ERR_WORKFLOW_PLUGIN_LOAD_FAILED);
    // Must NOT mention allowedPaths
    EXPECT_EQ(result.error().message().find("allowedPaths"), std::string::npos);
}
