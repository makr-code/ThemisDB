// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file benchmark_artifact_preflight.h
 * @brief Standardised preflight-check utilities for LLM / LoRA / gguf benchmarks.
 *
 * Maßnahme #6 – PERFORMANCE_EXPECTATIONS.md §1.4
 *
 * Problem addressed:
 *   Several LLM/RAG/LoRA benchmarks failed with opaque "file not found" errors
 *   because the required model artefacts (gguf files, LoRA adapters) were absent.
 *   The root cause was missing artefact setup before the benchmark run.
 *
 * Solution:
 *   This header provides lightweight preflight utilities that every LLM / LoRA /
 *   RAG benchmark fixture can include.  They:
 *     1. Read the standard environment variables (THEMIS_MODEL_DIR,
 *        THEMIS_LLM_STUB_MODELS) to resolve model / adapter paths.
 *     2. Emit a clear, actionable error message (via benchmark::State::SkipWithError
 *        or SkipWithMessage) when an artefact is absent.
 *     3. Expose a macro (THEMIS_BENCH_REQUIRE_ARTIFACT) for one-line guards in
 *        benchmark body functions.
 *
 * Usage example:
 * @code
 *   #include "benchmark_artifact_preflight.h"
 *
 *   class MyLLMFixture : public benchmark::Fixture {
 *   public:
 *       void SetUp(const ::benchmark::State& state) override {
 *           auto preflight = themis::bench::LLMArtifactPreflight::create();
 *           if (!preflight.ok()) {
 *               preflight_error_ = preflight.errorMessage();
 *               return;
 *           }
 *           model_path_ = preflight.modelPath();
 *           lora_path_  = preflight.loraPath();
 *       }
 *
 *   protected:
 *       bool ensureReady(benchmark::State& state) const {
 *           if (!preflight_error_.empty()) {
 *               state.SkipWithError(preflight_error_.c_str());
 *               return false;
 *           }
 *           return true;
 *       }
 *       std::string model_path_;
 *       std::string lora_path_;
 *       std::string preflight_error_;
 *   };
 *
 *   BENCHMARK_F(MyLLMFixture, BM_Inference)(benchmark::State& state) {
 *       if (!ensureReady(state)) return;
 *       // ... benchmark body ...
 *   }
 * @endcode
 *
 * For simple benchmark functions (not fixtures) use the convenience macro:
 * @code
 *   static void BM_SimpleInference(benchmark::State& state) {
 *       THEMIS_BENCH_REQUIRE_ARTIFACT(state, themis::bench::resolveModelPath());
 *       std::string model = themis::bench::resolveModelPath();
 *       // ... benchmark body ...
 *   }
 * @endcode
 */

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace themis {
namespace bench {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Default sub-paths within THEMIS_MODEL_DIR
static constexpr const char* kDefaultStubModelRelPath =
    "gguf/tinyllama-1.1b-chat-v1.0.Q4_K_M.gguf";
static constexpr const char* kDefaultRealModelRelPath =
    "gguf/Meta-Llama-3-8B-Instruct.Q4_K_M.gguf";
static constexpr const char* kDefaultStubLoraRelPath =
    "lora/legal_lora_stub.bin";

/// Fallback model directory when THEMIS_MODEL_DIR is not set
static constexpr const char* kFallbackModelDirSuffix =
    ".local/share/themis/models";

/// Optional explicit LoRA path override used by local benchmark runs.
static constexpr const char* kEnvLoraPath = "THEMIS_LLM_LORA_PATH";

/// Local workspace fallback location used by Windows/VS Code runs.
static constexpr const char* kRepoLocalModelsDir = "models";

// ---------------------------------------------------------------------------
// Path resolution helpers
// ---------------------------------------------------------------------------

/**
 * Returns the repository-local model directory when running from repo root.
 */
inline std::string repoLocalModelDir() {
    std::error_code ec = {};
    const auto local = std::filesystem::current_path(ec) / kRepoLocalModelsDir;
    if (ec) {
        return "";
    }
    if (std::filesystem::exists(local, ec) && std::filesystem::is_directory(local, ec)) {
        return local.string();
    }
    return "";
}

/**
 * Returns the effective model base directory.
 * Priority: THEMIS_MODEL_DIR env var > $HOME/.local/share/themis/models
 */
inline std::string modelBaseDir() {
    const char* env = std::getenv("THEMIS_MODEL_DIR");
    if (env && *env != '\0') {
        return env;
    }
    const char* home = std::getenv("HOME");
    if (home && *home != '\0') {
        return std::string(home) + "/" + kFallbackModelDirSuffix;
    }
    const auto local = repoLocalModelDir();
    if (!local.empty()) {
        return local;
    }
    return "/tmp/themis/models";
}

/**
 * Returns true when THEMIS_LLM_STUB_MODELS is set to "ON" (case-insensitive).
 */
inline bool stubModelsEnabled() {
    const char* env = std::getenv("THEMIS_LLM_STUB_MODELS");
    if (!env || *env == '\0') {
      return false;
    }
    std::string val(env);
    std::transform(val.begin(), val.end(), val.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return val == "ON" || val == "1" || val == "TRUE" || val == "YES";
}

/**
 * Resolves the primary model path.
 *
 * If THEMIS_LLM_MODEL_PATH is set and the file exists, that path is returned.
 * Otherwise falls back to the stub or real model under THEMIS_MODEL_DIR,
 * depending on the THEMIS_LLM_STUB_MODELS flag.
 *
 * Returns an empty string when no model file is found.
 */
inline std::string resolveModelPath() {
    // 1. Explicit override via THEMIS_LLM_MODEL_PATH
    const char* explicit_path = std::getenv("THEMIS_LLM_MODEL_PATH");
    if (explicit_path && *explicit_path != '\0' &&
        std::filesystem::exists(explicit_path)) {
        auto p = std::filesystem::path(explicit_path);
        p.make_preferred();
        return p.string();
    }

    const auto base = std::filesystem::path(modelBaseDir());
    const bool use_stub = stubModelsEnabled();

    // 2. Check stub model first when stub mode is active
    if (use_stub) {
        auto stub = base / kDefaultStubModelRelPath;
        stub.make_preferred();
        if (std::filesystem::exists(stub)) {
            return stub.string();
        }
    }

    // 3. Real model
    auto real = base / kDefaultRealModelRelPath;
    real.make_preferred();
    if (std::filesystem::exists(real)) {
        return real.string();
    }

    // 4. Fallback: stub model even outside explicit stub mode
    auto stub = base / kDefaultStubModelRelPath;
    stub.make_preferred();
    if (std::filesystem::exists(stub)) {
        return stub.string();
    }

    return "";
}

/**
 * Resolves the primary LoRA adapter path.
 *
 * Returns the path if the file exists, otherwise returns an empty string.
 */
inline std::string resolveLoraPath() {
    const char* explicit_path = std::getenv(kEnvLoraPath);
    if (explicit_path && *explicit_path != '\0' &&
        std::filesystem::exists(explicit_path)) {
        auto p = std::filesystem::path(explicit_path);
        p.make_preferred();
        return p.string();
    }

    const auto base = std::filesystem::path(modelBaseDir());
    const auto path = base / kDefaultStubLoraRelPath;
    auto normalized_path = path;
    normalized_path.make_preferred();
    if (std::filesystem::exists(normalized_path)) {
        return normalized_path.string();
    }

    return "";
}

// ---------------------------------------------------------------------------
// LLMArtifactPreflight – RAII preflight result object
// ---------------------------------------------------------------------------

/**
 * @class LLMArtifactPreflight
 * @brief Preflight check result for LLM/LoRA artefacts.
 *
 * Call LLMArtifactPreflight::create() in a benchmark fixture's SetUp().
 * If ok() is false, pass errorMessage() to state.SkipWithError() and return
 * early.
 */
class LLMArtifactPreflight {
public:
    /**
     * Runs the preflight check and returns the result.
     *
     * @param require_lora  When true the check also requires a LoRA adapter.
     */
    static LLMArtifactPreflight create(bool require_lora = false) {
        LLMArtifactPreflight result;
        result.model_path_ = resolveModelPath();
        result.lora_path_  = resolveLoraPath();

        if (result.model_path_.empty()) {
            result.error_message_ =
                "LLM artefact preflight FAILED: no model file found. "
                "Run 'scripts/download_models.sh --stub-only' or set "
                "THEMIS_MODEL_DIR / THEMIS_LLM_MODEL_PATH. "
                "See docs/BENCHMARK_RUNBOOK.md §\"LLM/LoRA Model Setup\".";
        } else if (require_lora && result.lora_path_.empty()) {
            result.error_message_ =
                "LLM artefact preflight FAILED: no LoRA adapter found at "
                "$THEMIS_MODEL_DIR/lora/. "
                "Run 'scripts/download_models.sh --stub-only' to generate a "
                "stub adapter. "
                "See docs/BENCHMARK_RUNBOOK.md §\"LLM/LoRA Model Setup\".";
        }

        return result;
    }

    /** Returns true when all required artefacts were found. */
    bool ok() const { return error_message_.empty(); }

    /** Human-readable error message suitable for state.SkipWithError(). */
    const std::string& errorMessage() const { return error_message_; }

    /** Absolute path to the resolved model file (empty on failure). */
    const std::string& modelPath() const { return model_path_; }

    /** Absolute path to the resolved LoRA adapter (may be empty). */
    const std::string& loraPath() const { return lora_path_; }

    /**
     * Returns the effective THEMIS_MODEL_DIR for use in diagnostic messages.
     */
    static std::string effectiveModelDir() { return modelBaseDir(); }

private:
    LLMArtifactPreflight() = default;
    std::string model_path_;
    std::string lora_path_;
    std::string error_message_;
};

} // namespace bench
} // namespace themis

// ---------------------------------------------------------------------------
// Convenience macros
// ---------------------------------------------------------------------------

/**
 * THEMIS_BENCH_REQUIRE_ARTIFACT(state, path_expr)
 *
 * Skips the benchmark with a clear error message if the given path is empty.
 * Intended for use at the top of benchmark body functions.
 *
 * @param state      benchmark::State& reference
 * @param path_expr  std::string expression that evaluates to the artefact path
 *
 * Example:
 * @code
 *   static void BM_Foo(benchmark::State& state) {
 *       THEMIS_BENCH_REQUIRE_ARTIFACT(state, themis::bench::resolveModelPath());
 *       std::string model = themis::bench::resolveModelPath();
 *       for (auto _ : state) { ... }
 *   }
 * @endcode
 */
#define THEMIS_BENCH_REQUIRE_ARTIFACT(state, path_expr)                        \
    do {                                                                        \
        const std::string _themis_artifact_path = (path_expr);                 \
        if (_themis_artifact_path.empty()) {                                    \
            (state).SkipWithError(                                              \
                "LLM artefact preflight FAILED: required artefact not found. " \
                "Run 'scripts/download_models.sh --stub-only' or set "          \
                "THEMIS_MODEL_DIR. "                                            \
                "See docs/BENCHMARK_RUNBOOK.md §\"LLM/LoRA Model Setup\".");   \
            return;                                                             \
        }                                                                       \
    } while (0)

/**
 * THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, path_expr, artifact_label)
 *
 * Variant that includes an artefact label in the error message.
 *
 * Example:
 * @code
 *   THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state,
 *       themis::bench::resolveLoraPath(), "LoRA adapter");
 * @endcode
 */
#define THEMIS_BENCH_SKIP_IF_ARTIFACT_MISSING(state, path_expr, artifact_label) \
    do {                                                                          \
        const std::string _themis_artifact_path = (path_expr);                   \
        if (_themis_artifact_path.empty()) {                                      \
            (state).SkipWithError(                                                \
                "LLM artefact preflight FAILED: " artifact_label " not found. "  \
                "Run 'scripts/download_models.sh --stub-only' or set "            \
                "THEMIS_MODEL_DIR. "                                              \
                "See docs/BENCHMARK_RUNBOOK.md §\"LLM/LoRA Model Setup\".");     \
            return;                                                               \
        }                                                                         \
    } while (0)
