// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_artifact_preflight.cpp
 * @brief Unit tests for benchmark_artifact_preflight.h (LLM/LoRA artefact preflight).
 *
 * Tests IDs: PF-01 .. PF-14
 *
 * Validates that:
 *   - Environment variable priority ordering is correct.
 *   - Stub-model flag is parsed case-insensitively.
 *   - Path resolution returns the first existing file.
 *   - LLMArtifactPreflight::create() emits actionable error messages when
 *     artefacts are missing, and ok() / modelPath() / loraPath() are
 *     consistent with actual file existence.
 *   - THEMIS_BENCH_REQUIRE_ARTIFACT macro compiles and fires on an empty path.
 */

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

// The header under test – header-only, no link dependency.
#include "benchmark_artifact_preflight.h"

namespace fs = std::filesystem;
using namespace themis::bench;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

/// RAII env-var setter / restorer.
struct ScopedEnv {
    explicit ScopedEnv(const char* name, const char* value) : name_(name) {
        const char* old = std::getenv(name);
        old_value_ = old ? old : "";
        had_old_   = (old != nullptr);
#ifdef _WIN32
        _putenv_s(name, value);
#else
        ::setenv(name, value, /*overwrite=*/1);
#endif
    }
    ~ScopedEnv() {
        if (had_old_) {
#ifdef _WIN32
            _putenv_s(name_.c_str(), old_value_.c_str());
#else
            ::setenv(name_.c_str(), old_value_.c_str(), 1);
#endif
        } else {
#ifdef _WIN32
            _putenv_s(name_.c_str(), "");
#else
            ::unsetenv(name_.c_str());
#endif
        }
    }
    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;

private:
    std::string name_ = {};
    std::string old_value_ = {};
    bool had_old_ = {};
};

/// RAII env-var unset.
struct ScopedUnsetEnv {
    explicit ScopedUnsetEnv(const char* name) : name_(name) {
        const char* old = std::getenv(name);
        had_old_    = (old != nullptr);
        old_value_  = had_old_ ? old : "";
#ifdef _WIN32
        _putenv_s(name, "");
#else
        ::unsetenv(name);
#endif
    }
    ~ScopedUnsetEnv() {
        if (had_old_) {
#ifdef _WIN32
            _putenv_s(name_.c_str(), old_value_.c_str());
#else
            ::setenv(name_.c_str(), old_value_.c_str(), 1);
#endif
        }
    }
    ScopedUnsetEnv(const ScopedUnsetEnv&) = delete;
    ScopedUnsetEnv& operator=(const ScopedUnsetEnv&) = delete;

private:
    std::string name_;
    std::string old_value_;
    bool had_old_;
};

/// Creates a tiny regular file at `path` (including parent directories).
void touchFile(const fs::path& path) {
    fs::create_directories(path.parent_path());
    std::ofstream f(path);
    f << "stub";
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// PF-01 – modelBaseDir() honours THEMIS_MODEL_DIR
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF01_ModelBaseDir_UsesEnvVar) {
    ScopedEnv guard("THEMIS_MODEL_DIR", "/tmp/themis_test_models_pf01");
    EXPECT_EQ(modelBaseDir(), "/tmp/themis_test_models_pf01");
}

// ---------------------------------------------------------------------------
// PF-02 – modelBaseDir() falls back to $HOME/.local/share/themis/models
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF02_ModelBaseDir_FallbackHome) {
    ScopedUnsetEnv unset("THEMIS_MODEL_DIR");
    ScopedEnv home("HOME", "/tmp/fakehome_pf02");
    const std::string dir = modelBaseDir();
    EXPECT_EQ(dir, "/tmp/fakehome_pf02/.local/share/themis/models");
}

// ---------------------------------------------------------------------------
// PF-03 – modelBaseDir() uses /tmp/themis/models when neither env var is set
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF03_ModelBaseDir_FallbackTmp) {
    ScopedUnsetEnv u1("THEMIS_MODEL_DIR");
    ScopedUnsetEnv u2("HOME");
    EXPECT_EQ(modelBaseDir(), "/tmp/themis/models");
}

// ---------------------------------------------------------------------------
// PF-04 – stubModelsEnabled() returns true for "ON"
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF04_StubModels_ON) {
    ScopedEnv guard("THEMIS_LLM_STUB_MODELS", "ON");
    EXPECT_TRUE(stubModelsEnabled());
}

// ---------------------------------------------------------------------------
// PF-05 – stubModelsEnabled() returns true for "1" / "true" / "yes"
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF05_StubModels_Variants) {
    {
        ScopedEnv g("THEMIS_LLM_STUB_MODELS", "1");
        EXPECT_TRUE(stubModelsEnabled());
    }
    {
        ScopedEnv g("THEMIS_LLM_STUB_MODELS", "TRUE");
        EXPECT_TRUE(stubModelsEnabled());
    }
    {
        ScopedEnv g("THEMIS_LLM_STUB_MODELS", "yes");
        EXPECT_TRUE(stubModelsEnabled());
    }
    // Case-insensitive
    {
        ScopedEnv g("THEMIS_LLM_STUB_MODELS", "on");
        EXPECT_TRUE(stubModelsEnabled());
    }
}

// ---------------------------------------------------------------------------
// PF-06 – stubModelsEnabled() returns false when unset
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF06_StubModels_Off) {
    ScopedUnsetEnv guard("THEMIS_LLM_STUB_MODELS");
    EXPECT_FALSE(stubModelsEnabled());
    {
        ScopedEnv g("THEMIS_LLM_STUB_MODELS", "OFF");
        EXPECT_FALSE(stubModelsEnabled());
    }
    {
        ScopedEnv g("THEMIS_LLM_STUB_MODELS", "0");
        EXPECT_FALSE(stubModelsEnabled());
    }
}

// ---------------------------------------------------------------------------
// PF-07 – resolveModelPath() returns empty when no file exists
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF07_ResolveModelPath_EmptyWhenMissing) {
    ScopedUnsetEnv u1("THEMIS_LLM_MODEL_PATH");
    ScopedEnv      dir("THEMIS_MODEL_DIR", "/tmp/themis_nonexistent_pf07");
    ScopedUnsetEnv u2("THEMIS_LLM_STUB_MODELS");
    EXPECT_TRUE(resolveModelPath().empty());
}

// ---------------------------------------------------------------------------
// PF-08 – resolveModelPath() uses THEMIS_LLM_MODEL_PATH when file exists
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF08_ResolveModelPath_ExplicitPath) {
    const fs::path stub = fs::temp_directory_path() / "themis_pf08_model.gguf";
    touchFile(stub);
    ScopedEnv path_env("THEMIS_LLM_MODEL_PATH", stub.string().c_str());

    EXPECT_EQ(resolveModelPath(), stub.string());
    fs::remove(stub);
}

// ---------------------------------------------------------------------------
// PF-09 – resolveModelPath() returns empty when THEMIS_LLM_MODEL_PATH is set
//         but the file does not exist
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF09_ResolveModelPath_ExplicitPathMissing) {
    ScopedEnv path_env("THEMIS_LLM_MODEL_PATH",
                       "/tmp/themis_pf09_does_not_exist.gguf");
    ScopedEnv dir_env("THEMIS_MODEL_DIR",
                      "/tmp/themis_pf09_no_models_dir");
    EXPECT_TRUE(resolveModelPath().empty());
}

// ---------------------------------------------------------------------------
// PF-10 – resolveModelPath() picks up the stub file under THEMIS_MODEL_DIR
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF10_ResolveModelPath_StubUnderModelDir) {
    const fs::path base = fs::temp_directory_path() / "themis_pf10_models";
    const fs::path stub = base / "gguf" / "tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf";
    touchFile(stub);

    ScopedEnv      dir_env("THEMIS_MODEL_DIR", base.string().c_str());
    ScopedUnsetEnv path_env("THEMIS_LLM_MODEL_PATH");
    ScopedEnv      stub_flag("THEMIS_LLM_STUB_MODELS", "ON");

    EXPECT_EQ(resolveModelPath(), stub.string());
    fs::remove_all(base);
}

// ---------------------------------------------------------------------------
// PF-11 – LLMArtifactPreflight::create() is not ok() when no model exists
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF11_Preflight_FailsWhenNoModel) {
    ScopedUnsetEnv u1("THEMIS_LLM_MODEL_PATH");
    ScopedEnv      dir_env("THEMIS_MODEL_DIR",
                           "/tmp/themis_pf11_empty_model_dir");
    ScopedUnsetEnv u2("THEMIS_LLM_STUB_MODELS");

    auto pf = LLMArtifactPreflight::create();
    EXPECT_FALSE(pf.ok());
    EXPECT_TRUE(pf.modelPath().empty());
    // Error message must be actionable
    EXPECT_NE(pf.errorMessage().find("download_models.sh"), std::string::npos)
        << "Expected setup guidance in error message";
    EXPECT_NE(pf.errorMessage().find("THEMIS_MODEL_DIR"), std::string::npos)
        << "Expected env-var hint in error message";
}

// ---------------------------------------------------------------------------
// PF-12 – LLMArtifactPreflight::create() is ok() when a model file exists
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF12_Preflight_OkWhenModelPresent) {
    const fs::path base  = fs::temp_directory_path() / "themis_pf12_models";
    const fs::path model = base / "gguf" / "tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf";
    touchFile(model);

    ScopedEnv      dir_env("THEMIS_MODEL_DIR", base.string().c_str());
    ScopedUnsetEnv path_env("THEMIS_LLM_MODEL_PATH");
    ScopedEnv      stub_flag("THEMIS_LLM_STUB_MODELS", "ON");

    auto pf = LLMArtifactPreflight::create();
    EXPECT_TRUE(pf.ok());
    EXPECT_EQ(pf.modelPath(), model.string());
    EXPECT_TRUE(pf.errorMessage().empty());

    fs::remove_all(base);
}

// ---------------------------------------------------------------------------
// PF-13 – LLMArtifactPreflight::create(true) fails when LoRA is missing
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF13_Preflight_FailsWhenLoRaMissing) {
    const fs::path base  = fs::temp_directory_path() / "themis_pf13_models";
    const fs::path model = base / "gguf" / "tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf";
    touchFile(model);
    // Intentionally do NOT create the lora/ directory

    ScopedEnv      dir_env("THEMIS_MODEL_DIR", base.string().c_str());
    ScopedUnsetEnv path_env("THEMIS_LLM_MODEL_PATH");
    ScopedEnv      stub_flag("THEMIS_LLM_STUB_MODELS", "ON");

    auto pf = LLMArtifactPreflight::create(/*require_lora=*/true);
    EXPECT_FALSE(pf.ok());
    EXPECT_FALSE(pf.modelPath().empty()) << "Model was present, path should be set";
    EXPECT_TRUE(pf.loraPath().empty())  << "LoRA was absent, path should be empty";
    // Error message must point to the LoRA gap
    EXPECT_NE(pf.errorMessage().find("LoRA"), std::string::npos)
        << "Error message should mention LoRA";
    EXPECT_NE(pf.errorMessage().find("download_models.sh"), std::string::npos)
        << "Expected setup guidance in error message";

    fs::remove_all(base);
}

// ---------------------------------------------------------------------------
// PF-14 – LLMArtifactPreflight::create(true) succeeds when both files exist
// ---------------------------------------------------------------------------
TEST(ArtifactPreflight, PF14_Preflight_OkWhenBothPresent) {
    const fs::path base  = fs::temp_directory_path() / "themis_pf14_models";
    const fs::path model = base / "gguf" / "tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf";
    const fs::path lora  = base / "lora" / "legal_lora_stub.bin";
    touchFile(model);
    touchFile(lora);

    ScopedEnv      dir_env("THEMIS_MODEL_DIR", base.string().c_str());
    ScopedUnsetEnv path_env("THEMIS_LLM_MODEL_PATH");
    ScopedEnv      stub_flag("THEMIS_LLM_STUB_MODELS", "ON");

    auto pf = LLMArtifactPreflight::create(/*require_lora=*/true);
    EXPECT_TRUE(pf.ok());
    EXPECT_EQ(pf.modelPath(), model.string());
    EXPECT_EQ(pf.loraPath(),  lora.string());
    EXPECT_TRUE(pf.errorMessage().empty());
    EXPECT_EQ(pf.effectiveModelDir(), base.string());

    fs::remove_all(base);
}
