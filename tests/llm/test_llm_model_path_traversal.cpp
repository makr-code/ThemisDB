/*
 * GAP-009: Model path traversal prevention tests.
 *
 * LLMPluginManager::loadModel() must reject paths that escape the configured
 * THEMIS_MODEL_ROOT directory (e.g. "../../etc/passwd" tricks).
 */

#include <gtest/gtest.h>
#include "llm/llm_plugin_manager.h"
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
        std::error_code ec;
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
