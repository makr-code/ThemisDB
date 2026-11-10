/**
 * @file peft_subprocess_backend.h
 * @brief ILoRATrainingBackend that runs real causal-LM LoRA/AdaLoRA
 *        fine-tuning in a Python (transformers + peft) subprocess.
 *
 * The backend drives `scripts/lora/themis_lora_train.py`:
 *  - train(): validates the request, splits the samples deterministically
 *    into train/holdout, writes `train.jsonl`, `eval.jsonl` and `job.json`
 *    into a private (0700) input directory below the job output directory,
 *    spawns `python <script> --config job.json` WITHOUT a shell and parses
 *    the stdout line protocol:
 *      - `THEMIS_PROGRESS {progress,step,total_steps,train_loss,eval_loss,phase}`
 *      - `THEMIS_RESULT {success,error,gguf_adapter_path,safetensors_path,
 *         peft_adapter_dir,manifest_path,train_loss,eval_loss,eval_perplexity,
 *         baseline_eval_loss,train_samples,eval_samples,rank_pattern,...}`
 *    The input directory (it contains database-derived training text) is
 *    removed after the run.
 *  - convertSafetensorsToGguf(): runs the same script with `--convert-only`
 *    to convert a safetensors adapter package into a standalone GGUF LoRA
 *    adapter (llama.cpp `convert_lora_to_gguf.py`) and embeds every given
 *    `themis.*` key unchanged into the GGUF KV store.
 *
 * Artifact policy: adapters are never merged into the base weights. The
 * distribution artifact is the safetensors package (path reported in
 * `raw_report["safetensors_path"]`), the serving artifact is the GGUF adapter
 * (`gguf_adapter_path`), loaded dynamically by llama.cpp at runtime. The
 * convenience manifest path is reported in `raw_report["manifest_path"]`.
 *
 * Metrics are never invented: metrics the trainer did not report (JSON null
 * or missing) are returned as quiet NaN.
 *
 * Process handling (POSIX): fork + execvp with an argv vector, the child is
 * placed in its own process group so that cancellation/timeout reaches the
 * converter grandchild as well (SIGTERM, then SIGKILL after a grace period).
 * Windows: CreateProcessW inside a kill-on-close Job object.
 *
 * Thread safety: the backend holds only immutable configuration; every call
 * is self-contained (own directories, pipes and child process), so
 * concurrent calls on one instance are safe as long as they use distinct
 * output directories.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "llm/lora_framework/lora_training_backend.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Static configuration of PeftSubprocessTrainingBackend.
 */
struct PeftSubprocessBackendConfig {
    /// Python interpreter (absolute path, or a name resolved via PATH).
    std::string python_executable = "python3";
    /// Absolute path of scripts/lora/themis_lora_train.py.
    std::string script_path;
    /// Absolute path of llama.cpp's convert_lora_to_gguf.py (required).
    std::string convert_script_path;
    /// Default HF base model (local HF directory or model id) when the request
    /// leaves TrainingJobRequest::base_model empty; also used for conversion.
    std::string base_model;
    /// Default GGUF base model file name/path when the request leaves it empty.
    std::string base_model_gguf;
    /// Root directory; every job output directory must lie strictly inside it.
    std::string work_root;
    /// GGUF output type passed to the converter (f32|f16|bf16|q8_0|auto).
    std::string gguf_outtype = "f16";
    /// Wall-clock limit of a training run.
    std::chrono::seconds timeout{std::chrono::hours(6)};
    /// Wall-clock limit of a --convert-only run.
    std::chrono::seconds convert_timeout{std::chrono::minutes(30)};
    /// Delay between SIGTERM and SIGKILL on cancel/timeout.
    std::chrono::seconds kill_grace{10};
    /// Number of trailing stderr bytes kept for error messages.
    std::size_t stderr_tail_bytes = 16 * 1024;
};

/**
 * @brief One decoded line of the trainer's stdout protocol.
 */
struct ProtocolMessage {
    enum class Kind { Progress, Result };
    Kind kind = Kind::Progress;
    nlohmann::json payload;  ///< Always a JSON object.
};

/**
 * @brief Deterministic train/holdout split (indices into the sample vector).
 */
struct TrainEvalSplit {
    std::vector<std::size_t> train;
    std::vector<std::size_t> eval;
};

/**
 * @brief Outcome of one child-process run (exposed for diagnostics/tests).
 */
struct SubprocessOutcome {
    bool launched = false;             ///< false: spawn failed (see launch_error).
    std::string launch_error;
    int exit_code = -1;                ///< Exit status if exited normally, else -1.
    int term_signal = 0;               ///< Terminating signal (POSIX), else 0.
    bool timed_out = false;
    bool cancelled = false;
    std::string stderr_tail;           ///< Last bytes of stderr (plus stray stdout lines).
    std::optional<nlohmann::json> result;  ///< Payload of the last THEMIS_RESULT line.
};

/**
 * @brief Real LoRA/AdaLoRA fine-tuning through a Python PEFT subprocess.
 */
class PeftSubprocessTrainingBackend final : public ILoRATrainingBackend {
public:
    /// Line prefixes of the stdout protocol (shared with the Python script).
    static constexpr std::string_view kProgressPrefix = "THEMIS_PROGRESS ";
    static constexpr std::string_view kResultPrefix = "THEMIS_RESULT ";
    /// Upper bound for one protocol line; longer lines are discarded.
    static constexpr std::size_t kMaxProtocolLineBytes = 4 * 1024 * 1024;

    /**
     * @brief Construct with an immutable configuration.
     * @param config Backend configuration (validated per call, not here).
     */
    explicit PeftSubprocessTrainingBackend(PeftSubprocessBackendConfig config);

    /**
     * @brief Train an adapter (blocking until the subprocess finished).
     * @param request     Job description (samples, hyperparameters, paths).
     * @param on_progress Called from the calling thread for every progress
     *                    line; exceptions thrown by it are swallowed.
     * @param cancel      Polled every 200 ms; when set the child process
     *                    group gets SIGTERM, then SIGKILL after kill_grace.
     * @return success=true only if the script exited 0, reported success and
     *         the GGUF and safetensors artifacts exist. raw_report holds the
     *         full THEMIS_RESULT object (incl. safetensors_path, manifest_path).
     */
    [[nodiscard]] TrainingJobResult train(const TrainingJobRequest& request,
                                          const ProgressCallback& on_progress,
                                          const std::atomic<bool>& cancel) override;

    /// @brief Backend identifier: "peft-subprocess".
    [[nodiscard]] std::string name() const override;

    /**
     * @brief Convert a safetensors adapter package into a GGUF LoRA adapter.
     *
     * Runs `python <script> --convert-only --config <tmp.json>`. The script
     * rejects pickle checkpoints, validates the safetensors header and
     * `themis.adapter.sha256_tensors`, runs convert_lora_to_gguf.py and embeds
     * all `themis.*` keys (package header merged with @p themis_metadata,
     * which wins on collision) unchanged into the GGUF KV store.
     *
     * @param peft_package_dir Directory with adapter_model.safetensors and
     *                         adapter_config.json.
     * @param themis_metadata  JSON object of `themis.*` keys (string, number
     *                         or bool values), e.g. signed provenance and
     *                         snapshot fingerprint keys.
     * @param output_gguf_path Destination `.gguf` file (overwritten atomically).
     * @param cancel           Cancellation flag (same semantics as train()).
     * @return success with gguf_adapter_path set, or an error; raw_report
     *         holds the script result (metadata, gguf_sha256, ...).
     */
    [[nodiscard]] TrainingJobResult convertSafetensorsToGguf(const std::string& peft_package_dir,
                                                             const nlohmann::json& themis_metadata,
                                                             const std::string& output_gguf_path,
                                                             const std::atomic<bool>& cancel);

    /// @brief The configuration this backend was constructed with.
    [[nodiscard]] const PeftSubprocessBackendConfig& config() const noexcept { return config_; }

    // ── Static helpers (pure; exposed for unit tests) ─────────────────────

    /**
     * @brief Decode one stdout line of the protocol.
     * @param line One line without or with trailing "\r"/"\n".
     * @return The message, or std::nullopt for non-protocol or malformed lines
     *         (malformed JSON / non-object payload).
     */
    [[nodiscard]] static std::optional<ProtocolMessage> parseProtocolLine(std::string_view line);

    /// @brief Map a THEMIS_PROGRESS payload to TrainingProgress (lenient on types).
    [[nodiscard]] static TrainingProgress progressFromJson(const nlohmann::json& payload);

    /**
     * @brief Map a THEMIS_RESULT payload to TrainingJobResult.
     *
     * Missing or null metrics become quiet NaN (never 0 or invented values);
     * raw_report is the full payload. Does not check files on disk.
     */
    [[nodiscard]] static TrainingJobResult resultFromJson(const nlohmann::json& payload);

    /// @brief true iff @p id matches [A-Za-z0-9_.-]{1,128}.
    [[nodiscard]] static bool isValidAdapterId(std::string_view id) noexcept;

    /**
     * @brief true iff @p candidate lies strictly inside @p root after
     *        std::filesystem::weakly_canonical (symlinks of existing
     *        components resolved, ".." collapsed).
     */
    [[nodiscard]] static bool isWithinRoot(const std::filesystem::path& candidate,
                                           const std::filesystem::path& root);

    /**
     * @brief Validate a request against a configuration.
     * @return Empty string if valid, otherwise a precise error message.
     *         Does not touch the filesystem except for path canonicalisation
     *         and existence checks of the configured scripts.
     */
    [[nodiscard]] static std::string validateRequest(const TrainingJobRequest& request,
                                                     const PeftSubprocessBackendConfig& config);

    /**
     * @brief Deterministic shuffled split.
     *
     * Fisher–Yates with std::mt19937_64(seed) and rejection sampling (no
     * implementation-defined distributions, identical on all platforms).
     * eval size = round(n * holdout) clamped to [1, n-1]; both index lists
     * are sorted ascending. Requires n >= 2 and holdout in (0, 0.5].
     */
    [[nodiscard]] static TrainEvalSplit splitTrainEval(std::size_t n, double holdout_fraction,
                                                       std::uint64_t seed);

    /**
     * @brief JSONL object for one sample: {"input","output"} plus "context"
     *        (string array from metadata["context"]) and "system" (string
     *        from metadata["system"]) when present.
     */
    [[nodiscard]] static nlohmann::json sampleToJson(const TrainingDataSample& sample);

    /**
     * @brief Write the selected samples as UTF-8 JSONL (one object per line).
     * @param error Receives the reason on failure (I/O, invalid UTF-8, index).
     * @return true on success.
     */
    [[nodiscard]] static bool writeJsonl(const std::filesystem::path& path,
                                         const std::vector<TrainingDataSample>& samples,
                                         const std::vector<std::size_t>& indices,
                                         std::string* error);

    /**
     * @brief Build job.json for the training script.
     *
     * `themis.*` keys of request.data.metadata are forwarded in
     * "themis_metadata" (string/number/bool values) so that e.g.
     * themis.training.snapshot.* reach the adapter metadata unchanged.
     */
    [[nodiscard]] static nlohmann::json buildJobJson(const TrainingJobRequest& request,
                                                     const PeftSubprocessBackendConfig& config,
                                                     const std::filesystem::path& train_file,
                                                     const std::filesystem::path& eval_file,
                                                     const std::filesystem::path& output_dir);

    /// @brief argv of the child: {python, script, "--config", file[, "--convert-only"]}.
    [[nodiscard]] static std::vector<std::string> buildArgv(const PeftSubprocessBackendConfig& config,
                                                            const std::filesystem::path& config_file,
                                                            bool convert_only);

    /**
     * @brief Spawn @p argv without a shell and run the protocol loop.
     * @param on_line Called for every decoded protocol message.
     */
    [[nodiscard]] static SubprocessOutcome runProtocolProcess(
        const std::vector<std::string>& argv,
        const std::function<void(const ProtocolMessage&)>& on_line,
        const std::atomic<bool>& cancel, std::chrono::seconds timeout,
        std::chrono::seconds kill_grace, std::size_t stderr_tail_bytes);

private:
    TrainingJobResult trainImpl(const TrainingJobRequest& request, const ProgressCallback& on_progress,
                                const std::atomic<bool>& cancel);
    TrainingJobResult convertImpl(const std::string& peft_package_dir, const nlohmann::json& themis_metadata,
                                  const std::string& output_gguf_path, const std::atomic<bool>& cancel);

    PeftSubprocessBackendConfig config_;
};

}  // namespace lora
}  // namespace llm
}  // namespace themis
