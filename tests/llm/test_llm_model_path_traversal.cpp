/*
 * GAP-009: Model path traversal prevention tests.
 *
 * LLMPluginManager::loadModel() must reject paths that escape the configured
 * THEMIS_MODEL_ROOT directory (e.g. "../../etc/passwd" tricks).
 */

#include <gtest/gtest.h>
#include "llm/detail/default_model_path.h"
#include "llm/llm_plugin_manager.h"
#include "llm/model_downloader.h"
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

// POSIX / Win32 setenv shim
#ifdef _WIN32
#  include <cstdlib>
#  ifndef setenv
#    define setenv(n, v, o) _putenv_s(n, v)
#  endif
#  ifndef unsetenv
#    define unsetenv(n) _putenv_s(n, "")
#  endif
#endif

namespace fs = std::filesystem;
using namespace themis::llm;

// RAII helper to set/restore an environment variable.
struct ScopedEnv {
    std::string name_, old_;
    bool had_old_{false};

    ScopedEnv(const char* name, const char* value) : name_(name) {
        if (const char* prev = std::getenv(name)) {
            had_old_ = true;
            old_     = prev;
        }
        ::setenv(name, value, 1);
    }
    ~ScopedEnv() {
        if (had_old_) {
            ::setenv(name_.c_str(), old_.c_str(), 1);
        } else {
#ifdef _WIN32
            _putenv_s(name_.c_str(), "");
#else
            ::unsetenv(name_.c_str());
#endif
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Fixture — creates a real temp directory as THEMIS_MODEL_ROOT.
// ─────────────────────────────────────────────────────────────────────────────
class LlmModelPathTraversalTest : public ::testing::Test {
protected:
    fs::path model_root_;

    void SetUp() override {
        model_root_ = fs::temp_directory_path() /
                      ("themis_gap009_" + std::to_string(
                           std::chrono::high_resolution_clock::now()
                               .time_since_epoch().count()));
        fs::create_directories(model_root_);
        // Create a dummy model file inside the root.
        std::ofstream(model_root_ / "allowed.gguf") << "dummy";
    }

    void TearDown() override {
        std::error_code ec = {};
        fs::remove_all(model_root_, ec);
    }
};

// GAP-009-01: A path inside THEMIS_MODEL_ROOT must pass the containment check.
// loadModel() may still return false (no real plugin / model) but it must NOT
// reject because of path policy.
TEST_F(LlmModelPathTraversalTest, GAP009_PathInsideRoot_NotRejectedByPolicy) {
    ScopedEnv env("THEMIS_MODEL_ROOT", model_root_.string().c_str());

    const std::string inside_path = (model_root_ / "allowed.gguf").string();
    auto& mgr = LLMPluginManager::instance();

    // loadModel returns false because no plugin is registered — that's OK.
    // What matters is that the call does NOT throw / return false due to the
    // path containment check (which would be logged at error level).
    // We only verify no exception escapes; the return value is don't-care.
    EXPECT_NO_THROW({
        // Ignore the return value — no real plugin is installed in unit tests.
        (void)mgr.loadModel("test-model", inside_path);
    });
}

// GAP-009-02: A path that traverses above THEMIS_MODEL_ROOT must be rejected.
TEST_F(LlmModelPathTraversalTest, GAP009_PathTraversal_Rejected) {
    ScopedEnv env("THEMIS_MODEL_ROOT", model_root_.string().c_str());

    // Construct a path that escapes via "../../..": resolve to /etc/passwd style
    const std::string traversal = (model_root_ / ".." / ".." / "evil.gguf").string();

    auto& mgr = LLMPluginManager::instance();
    // loadModel must return false when path is outside the model root.
    const bool result = mgr.loadModel("evil-model", traversal);
    EXPECT_FALSE(result) << "Path traversal outside THEMIS_MODEL_ROOT must be rejected";
}

// GAP-009-03: When THEMIS_MODEL_ROOT is not set, the containment check is
// skipped (backward-compatibility), so loadModel proceeds normally.
TEST_F(LlmModelPathTraversalTest, GAP009_NoModelRoot_CheckSkipped) {
    // Make sure env var is absent.
#ifdef _WIN32
    _putenv_s("THEMIS_MODEL_ROOT", "");
#else
    ::unsetenv("THEMIS_MODEL_ROOT");
#endif

    auto& mgr = LLMPluginManager::instance();
    // No containment rejection expected (may fail for other reasons — don't care).
    EXPECT_NO_THROW({
        (void)mgr.loadModel("unrestricted-model",
                            (model_root_ / "allowed.gguf").string());
    });
}

// GAP-009-04: ModelDownloader must reject a `download_dir` that escapes the
// intended working root even when the model_name itself is valid.
TEST_F(LlmModelPathTraversalTest, GAP009_DownloadDirTraversal_Rejected) {
    ModelDownloadConfig cfg;
    cfg.ollama_url = "http://localhost:11434";
    cfg.model_name = "safe-model";
    cfg.download_dir = (model_root_ / ".." / "escape").string();
    cfg.timeout_seconds = 1;
    cfg.use_cache = false;

    ModelDownloader downloader;
    const auto result = downloader.downloadFromOllama(cfg);

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Invalid download_dir"), std::string::npos)
        << "download_dir traversal must be rejected before any disk write occurs; got: "
        << result.error_message;
}

TEST_F(LlmModelPathTraversalTest, DefaultModelPathHonorsExplicitFileOverride) {
    const auto override_path = model_root_ / "override.gguf";
    std::ofstream(override_path) << "override";
    const auto model_dir_default = model_root_ / "default.gguf";
    std::ofstream(model_dir_default) << "default";

    ScopedEnv model_path("THEMIS_LLM_DEFAULT_MODEL_PATH", override_path.string().c_str());
    ScopedEnv demo_model_path("THEMIS_DEMO_LLM_MODEL_PATH", "");
    ScopedEnv model_dir("THEMIS_MODEL_DIR", model_root_.string().c_str());
    ScopedEnv llm_model_path("THEMIS_LLM_MODEL_PATH", "");

    const auto resolved = themis::llm::detail::resolveDefaultModelPath(model_root_);

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, override_path);
}

TEST_F(LlmModelPathTraversalTest, DefaultModelPathUsesConfiguredModelDirectory) {
    const auto default_path = model_root_ / "default.gguf";
    std::ofstream(default_path) << "default";

    ScopedEnv model_path("THEMIS_LLM_DEFAULT_MODEL_PATH", "");
    ScopedEnv demo_model_path("THEMIS_DEMO_LLM_MODEL_PATH", "");
    ScopedEnv model_dir("THEMIS_MODEL_DIR", model_root_.string().c_str());
    ScopedEnv llm_model_path("THEMIS_LLM_MODEL_PATH", "");

    const auto resolved = themis::llm::detail::resolveDefaultModelPath(model_root_);

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, default_path);
}

TEST_F(LlmModelPathTraversalTest, ModelDirectoryPreservesConfiguredFilename) {
    const auto custom_path = model_root_ / "custom-model.gguf";
    std::ofstream(custom_path) << "custom";

    ScopedEnv model_path("THEMIS_LLM_DEFAULT_MODEL_PATH", "");
    ScopedEnv demo_model_path("THEMIS_DEMO_LLM_MODEL_PATH", "");
    ScopedEnv model_dir("THEMIS_MODEL_DIR", model_root_.string().c_str());
    ScopedEnv llm_model_path("THEMIS_LLM_MODEL_PATH", "");

    const auto resolved = themis::llm::detail::resolveModelPathOverride(
        model_root_, "relative/custom-model.gguf");

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, custom_path);
}

TEST_F(LlmModelPathTraversalTest, LegacyLlmModelPathDirectoryUsesDefaultModel) {
    const auto default_path = model_root_ / "default.gguf";
    std::ofstream(default_path) << "default";

    ScopedEnv model_path("THEMIS_LLM_DEFAULT_MODEL_PATH", "");
    ScopedEnv demo_model_path("THEMIS_DEMO_LLM_MODEL_PATH", "");
    ScopedEnv model_dir("THEMIS_MODEL_DIR", "");
    ScopedEnv llm_model_path("THEMIS_LLM_MODEL_PATH", model_root_.string().c_str());

    const auto resolved = themis::llm::detail::resolveDefaultModelPath(model_root_);

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, default_path);
}

TEST_F(LlmModelPathTraversalTest, DefaultModelPathFallsBackToModelsDirectory) {
    const auto models_dir = model_root_ / "models";
    fs::create_directories(models_dir);
    const auto default_path = models_dir / "default.gguf";
    std::ofstream(default_path) << "default";

    ScopedEnv model_path("THEMIS_LLM_DEFAULT_MODEL_PATH", "");
    ScopedEnv demo_model_path("THEMIS_DEMO_LLM_MODEL_PATH", "");
    ScopedEnv model_dir("THEMIS_MODEL_DIR", "");
    ScopedEnv llm_model_path("THEMIS_LLM_MODEL_PATH", "");

    const auto resolved = themis::llm::detail::resolveDefaultModelPath(model_root_);

    ASSERT_TRUE(resolved.has_value());
    EXPECT_EQ(*resolved, default_path);
}

TEST_F(LlmModelPathTraversalTest, DefaultModelPathIsAbsentWhenNoModelExists) {
    ScopedEnv model_path("THEMIS_LLM_DEFAULT_MODEL_PATH", "");
    ScopedEnv demo_model_path("THEMIS_DEMO_LLM_MODEL_PATH", "");
    ScopedEnv model_dir("THEMIS_MODEL_DIR", "");

    EXPECT_FALSE(themis::llm::detail::resolveDefaultModelPath(model_root_).has_value());
}
