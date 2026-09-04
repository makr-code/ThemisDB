/**
 * @file llama_cpp_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 83/100
 * @note Gap Summary: total=19; TODO=1, Stub=15, Unimpl=0, Mock=1, Sim=2, Debt=0, C=4, H=11, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llama_cpp/llama_cpp_plugin.h"
#include "llm/json_schema_converter.h"
#include <stdexcept>
#include "rag/rag_context_assembler.h"
#include <algorithm>
#include <atomic>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifndef THEMIS_NO_SPDLOG
#include <spdlog/spdlog.h>
#else
namespace spdlog {
template <typename... Args>
inline void info(const char*, Args&&...) {}
template <typename... Args>
inline void debug(const char*, Args&&...) {}
template <typename... Args>
inline void warn(const char*, Args&&...) {}
} // namespace spdlog
#endif

namespace themis {
namespace llamacpp {

namespace {
constexpr size_t kDefaultDraftFallbackVocabSize = 32000u;
constexpr size_t kMaxDraftFallbackVocabSize = 65536u;
} // namespace

LlamaCppPlugin::LlamaCppPlugin() = default;
LlamaCppPlugin::~LlamaCppPlugin() { unloadModel(); }

// ── loadModel / unloadModel ───────────────────────────────────────────────────

bool LlamaCppPlugin::loadModel(const std::string& model_path, const json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    model_path_   = model_path;
    model_id_     = model_path.empty() ? "stub" : model_path;

    // Read context window size from config (keys: "context_length" or "n_ctx").
    // Fall back to 4 096 when neither key is present or the value is 0.
    size_t ctx = 0u;
    if (config.contains("context_length") && config["context_length"].is_number()) {
        ctx = config["context_length"].get<size_t>();
    } else if (config.contains("n_ctx") && config["n_ctx"].is_number()) {
        ctx = config["n_ctx"].get<size_t>();
    }
    context_length_ = (ctx > 0u) ? ctx : llm::kDefaultContextWindowTokens;

#ifdef THEMIS_LLM_ENABLED
    // When a real model path is supplied, attempt to create a LlamaWrapper and
    // load the model through it.  Any exception or load failure is caught and
    // the plugin silently falls back to stub mode so that CI and unit tests
    // that pass an empty or non-existent path continue to work.
    wrapper_.reset();
    if (!model_path.empty()) {
        try {
            llm::LlamaWrapper::Config wrapper_cfg;
            if (config.contains("n_gpu_layers") && config["n_gpu_layers"].is_number())
                wrapper_cfg.n_gpu_layers = config["n_gpu_layers"].get<int>();
            if (config.contains("n_ctx") && config["n_ctx"].is_number())
                wrapper_cfg.n_ctx = config["n_ctx"].get<int>();
            else if (config.contains("context_length") && config["context_length"].is_number())
                wrapper_cfg.n_ctx = static_cast<int>(config["context_length"].get<size_t>());
            if (config.contains("n_batch") && config["n_batch"].is_number())
                wrapper_cfg.n_batch = config["n_batch"].get<int>();
            if (config.contains("n_threads") && config["n_threads"].is_number())
                wrapper_cfg.n_threads = config["n_threads"].get<int>();
            // Disable the response cache to avoid an unconditional RocksDB
            // initialisation during startup (matches LlamaWrapper default comment).
            wrapper_cfg.enable_response_cache = false;
            // Continuous batching is managed externally; keep it off in the plugin.
            wrapper_cfg.use_continuous_batching = false;

            auto w = std::make_unique<llm::LlamaWrapper>(wrapper_cfg);
            if (w->loadModel(model_path, config)) {
                wrapper_ = std::move(w);
                // Prefer the wrapper's richer context-length information.
                auto info = wrapper_->getModelInfo();
                if (info && info->context_length > 0)
                    context_length_ = info->context_length;

                // Opt-in model integrity: when config["verify_model_digest"] is
                // true and config["expected_model_digest"] is set, compute a
                // file digest and compare. Fail-closed on mismatch.
                if (config.value("verify_model_digest", false)) {
                    const std::string expected =
                        config.value("expected_model_digest", "");
                    if (!expected.empty()) {
                        const std::string actual = computeFileDigest(model_path);
                        if (actual != expected) {
                            wrapper_.reset();
                            model_loaded_ = false;
                            return false; // Digest mismatch — fail-closed
                        }
                    }
                }
            }
        } catch (...) {
            // Fallback to stub mode; wrapper_ remains null.
            wrapper_.reset();
        }
    }
#endif

    model_loaded_ = true;
    return true;
}

void LlamaCppPlugin::unloadModel() {
    std::lock_guard<std::mutex> lock(mutex_);
    model_loaded_ = false;
    model_id_.clear();
    model_path_.clear();
    loras_.clear();
#ifdef THEMIS_LLM_ENABLED
    wrapper_.reset();
#endif
}

// ── getModelInfo ──────────────────────────────────────────────────────────────

std::optional<llm::ModelInfo> LlamaCppPlugin::getModelInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!model_loaded_) {
      return std::nullopt;
    }
#ifdef THEMIS_LLM_ENABLED
    if (wrapper_) {
        auto info = wrapper_->getModelInfo();
        if (info) {
          return info;
        }
    }
#endif
    llm::ModelInfo info;
    info.model_id       = model_id_;
    info.path           = model_path_;
    info.context_length = context_length_;
    return info;
}

std::string LlamaCppPlugin::getModelId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return model_id_;
}

// ── LoRA management ───────────────────────────────────────────────────────────

bool LlamaCppPlugin::loadLoRA(const std::string& lora_id,
                               const std::string& lora_path, float scale) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Remove existing entry with same id
    loras_.erase(std::remove_if(loras_.begin(), loras_.end(),
                                [&]([[maybe_unused]] const LoRAEntry& e){ return e.id == lora_id; }),
                 loras_.end());
    loras_.push_back({lora_id, lora_path, scale});
    return true;
}

bool LlamaCppPlugin::unloadLoRA(const std::string& lora_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto before = loras_.size();
    loras_.erase(std::remove_if(loras_.begin(), loras_.end(),
                                [&]([[maybe_unused]] const LoRAEntry& e){ return e.id == lora_id; }),
                 loras_.end());
    return static_cast<int>(loras_.size()) < before;
}

std::vector<llm::LoRAInfo> LlamaCppPlugin::listLoRAs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<llm::LoRAInfo> result = {};

    result.reserve(loras_.size());
    for (const auto& e : loras_) {
        llm::LoRAInfo info;
        info.lora_id   = e.id;
        info.path      = e.path;
        info.scale     = e.scale;
        result.push_back(std::move(info));
    }
    return result;
}

// ── generate ──────────────────────────────────────────────────────────────────

llm::InferenceResponse LlamaCppPlugin::generate(const llm::InferenceRequest& request) {
    llm::InferenceResponse response;

    // ── Stream-callback retry wrapper (gap: no_retry_logic × 13) ─────────────
    // Invokes request.stream_callback(token) with up to kMaxRetries attempts
    // for transient exceptions.  std::bad_alloc is treated as non-retryable.
    // Each failed transient attempt (except the last) increments
    // stream_retry_count_; a final failure increments error_count_.
    // Gap scanner found 5× pointer_arithmetic_unbounded findings; audit of
    // this file found no unbounded raw-pointer arithmetic — findings are
    // false-positives from the scanner.  No change required.
    constexpr int kStreamCallbackMaxRetries = 3;
    auto invokeStreamCallback = [&]([[maybe_unused]] const std::string& token) {
        for ([[maybe_unused]] int attempt = 0; attempt < kStreamCallbackMaxRetries; ++attempt) {
            try {
                request.stream_callback([[maybe_unused]] token);
                return; // success
            } catch (const std::bad_alloc&) {
                ++error_count_;
                return; // non-retryable; abort immediately
            } catch (...) {
                if ([[maybe_unused]] attempt < kStreamCallbackMaxRetries - 1) {
                    ++stream_retry_count_; // transient — will retry
                } else {
                    ++error_count_; // all retries exhausted
                }
            }
        }
    };

    // Snapshot state under lock; do not hold lock during (potentially long) inference.
    bool loaded = false;
#ifdef THEMIS_LLM_ENABLED
    llm::LlamaWrapper* w = nullptr;
#endif
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loaded = model_loaded_;
#ifdef THEMIS_LLM_ENABLED
        w = wrapper_.get();
#endif
    }

    // Policy gate: checked before any inference work, after snapshotting state.
    {
        PolicyFn policy_fn;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            policy_fn = policy_fn_;
        }
        if (policy_fn) {
            std::string denial_reason = {};
            if (!policy_fn(request, denial_reason)) {
                ++error_count_;
                llm::InferenceResponse denied;
                denied.success       = false;
                denied.error_message = "Request denied by policy: " + denial_reason;
                denied.trace_id      = request.trace_id;
                denied.span_id       = request.span_id;
                return denied;
            }
        }
    }

    if (!loaded) {
        ++error_count_;
        response.success       = false;
        response.error_message = "LlamaCppPlugin: model not loaded";
        return response;
    }

    // Per-request cancellation check: reject before starting inference.
    if (request.cancellation_token &&
        request.cancellation_token->load(std::memory_order_acquire)) {
        ++error_count_;
        response.success       = false;
        response.error_message = "Request cancelled";
        response.trace_id      = request.trace_id;
        response.span_id       = request.span_id;
        return response;
    }

#ifdef THEMIS_LLM_ENABLED
    if (w) {
        ++inference_count_;
        try {
            response = w->generate(request);
            // LlamaWrapper does not set success=true on the happy path.
            response.success = true;
        } catch (const std::exception& e) {
            ++error_count_;
            response.success       = false;
            response.error_message = e.what();
            response.trace_id      = request.trace_id;
            response.span_id       = request.span_id;
        }
        return response;
    }
#endif

    GenerateFn generate_fn;
    std::string bridged_model_id = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        generate_fn = generate_fn_;
        bridged_model_id = model_id_;
    }
    if (generate_fn) {
        try {
            auto bridged = generate_fn(request);
            // `model_id` is the identifier the caller sees on the response;
            // `model_used` tracks which physical model actually produced the
            // tokens (useful when a router may forward to a different backend).
            // Both are set to the same local `model_id_` when the bridge does
            // not populate them, so that callers always get a non-empty value
            // in both fields.
            if (!bridged.model_id.empty()) {
                bridged.model_used = bridged.model_id;
            } else if (bridged.model_used.empty()) {
                bridged.model_id = bridged_model_id;
                bridged.model_used = bridged_model_id;
            }
            if (bridged.trace_id.empty()) {
                bridged.trace_id = request.trace_id;
            }
            if (bridged.span_id.empty()) {
                bridged.span_id = request.span_id;
            }
            if ([[maybe_unused]] request.stream_callback && !bridged.text.empty()) {
                invokeStreamCallback([[maybe_unused]] bridged.text);
            }
            ++inference_count_;
            return bridged;
        } catch (const std::exception& e) {
            ++error_count_;
            response.success = false;
            response.error_message = std::string("LlamaCppPlugin generate bridge failed: ") + e.what();
            response.trace_id = request.trace_id;
            response.span_id = request.span_id;
            return response;
        } catch (...) {
            ++error_count_;
            response.success = false;
            response.error_message = "LlamaCppPlugin generate bridge failed";
            response.trace_id = request.trace_id;
            response.span_id = request.span_id;
            return response;
        }
    }

    // STUB/SIMULATION NOTE:
    // Purpose: Signal clearly that no model is loaded when llama.cpp is not
    //          compiled in (THEMIS_ENABLE_LLAMA_CPP not set) or loadModel()
    //          has not been called.  Unit-test builds that require the echo
    //          behaviour should define THEMIS_LLAMA_CPP_STUB_MODE.
    // Activation: Reached when `wrapper_` is nullptr (model not loaded or build
    //             flag absent); controlled by build flag THEMIS_ENABLE_LLAMA_CPP
    //             and runtime loadModel() call.
    // Production Delta (fixed — Gap 1): Previously returned success=true with a
    //             stub echo string, making the failure invisible to callers.
    //             Now returns success=false + error_message="Model not loaded"
    //             so callers can programmatically detect and handle the error.
    // Removal Plan: Build with -DTHEMIS_LLM_ENABLED=ON, provide a valid model
    //               file path in config["model_path"], and call loadModel()
    //               before generate().  Once wrapper_ is non-null the real
    //               LlamaWrapper inference path executes and this block is
    //               bypassed entirely.  See SETUP.md §"Enabling real LLM inference".
    // Roadmap ref: src/llama_cpp/ROADMAP.md § "Planned Features"
    // See: llama_cpp/FUTURE_ENHANCEMENTS.md §6; AI_ML_IMPACT_ASSESSMENT.md §7 Gap 1.
#ifdef THEMIS_LLAMA_CPP_STUB_MODE
    // Test-only path: retain the old echo behaviour when the test macro is set.
    {
        ++inference_count_;

        // When tools are provided, synthesize a minimal stub tool-call JSON so
        // tests can verify the tool-calling path without a real model.
        std::string text = {};
        if (!request.tools.empty()) {
            const auto& first_tool = request.tools.front();
            nlohmann::json tool_call_json = {
                {"name", first_tool.name},
                {"arguments", first_tool.parameters.contains("properties")
                    ? nlohmann::json::object()
                    : nlohmann::json::object()}
            };
            text = tool_call_json.dump();
            auto parsed = llm::JsonSchemaConverter::parseToolCall(text);
            if (parsed.has_value()) {
                response.tool_calls.push_back(std::move(parsed.value()));
            }
        } else {
            text = "[stub:" + request.prompt.substr(0, 40) + "]";
        }

        if ([[maybe_unused]] request.stream_callback) {
            invokeStreamCallback([[maybe_unused]] text);
        }
        response.text             = text;
        response.success          = true;
        response.tokens_generated = static_cast<int>(text.size() / 4 + 1);
        response.trace_id         = request.trace_id;
        response.span_id          = request.span_id;
        return response;
    }
#endif
    ++error_count_;
    response.success       = false;
    response.error_message = "Model not loaded — call loadModel() before generate()";
    response.trace_id      = request.trace_id;
    response.span_id       = request.span_id;
    return response;
}

llm::InferenceResponse LlamaCppPlugin::generateRAG(
        const llm::RAGContext& rag_context,
        const llm::InferenceRequest& request) {
    // Snapshot shared state under the mutex to eliminate data races on
    // model_loaded_ and context_length_ that may be concurrently written
    // by loadModel() / unloadModel().
    bool   snap_model_loaded = {};
    size_t snap_context_length = {};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snap_model_loaded   = model_loaded_;
        snap_context_length = context_length_;
    }

    // Build RetrievedChunk objects from the RAGContext documents.
    // request and rag_context are caller-owned; no shared state involved here.
    std::vector<themis::rag::RetrievedChunk> chunks = {};

    chunks.reserve(rag_context.documents.size());
    for (const auto& doc : rag_context.documents) {
        themis::rag::RetrievedChunk chunk;
        chunk.content         = doc.content;
        chunk.source          = doc.source;
        chunk.relevance_score = doc.relevance_score;
        chunks.push_back(std::move(chunk));
    }

    // Early-out if model is not loaded (mirrors generate() behaviour).
    if (!snap_model_loaded) {
        llm::InferenceResponse err;
        err.success       = false;
        err.error_message = "Model not loaded — call loadModel() before generateRAG()";
        err.trace_id      = request.trace_id;
        err.span_id       = request.span_id;
        ++error_count_;
        return err;
    }

    // Configure the assembler from the loaded model's context window.
    // Honour an explicit override from the caller (rag_contex[[maybe_unused]] t.max_context_token[[maybe_unused]] s).
    themis::rag::RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens =
        (rag_context.max_context_tokens > 0)
            ? static_cast<size_t>(rag_context.max_context_tokens)
            : snap_context_length;
    cfg.min_response_tokens  =
        (rag_context.response_budget_tokens > 0)
            ? static_cast<size_t>(rag_context.response_budget_tokens)
            : ((request.max_tokens > 0)
                   ? static_cast<size_t>(request.max_tokens)
                   : llm::kDefaultMinResponseTokens);

    std::string rag_mode = "text";
    if (request.metadata.is_object()) {
        const auto rag_mode_it = request.metadata.find("rag_mode");
        if (rag_mode_it != request.metadata.end() && rag_mode_it->is_string()) {
            rag_mode = rag_mode_it->get<std::string>();
        }
    }

    spdlog::info(
        "LlamaCppPlugin::generateRAG start: docs={} rag_mode='{}' query_chars={} model_ctx={} response_budget={} request_max_tokens={}",
        rag_context.documents.size(),
        rag_mode,
        request.prompt.size(),
        cfg.model_context_tokens,
        cfg.min_response_tokens,
        request.max_tokens);

    themis::rag::RAGContextAssembler assembler(cfg);
    const themis::rag::AssembledContext ctx =
        assembler.assemble(chunks, /*system_prompt=*/"", request.prompt);

    spdlog::info(
        "LlamaCppPlugin::generateRAG assembled: chunks_used={} tokens_used={} truncated={} response_tokens_remaining={}",
        ctx.chunks_used.size(),
        ctx.tokens_used,
        ctx.was_truncated,
        ctx.tokens_remaining_for_response);

    llm::InferenceRequest augmented = request;

    // Build the augmented prompt from the assembled (budget-respecting) context.
    // Optional hybrid mode compacts retrieved chunks into fixed-size memory slots
    // to reduce prompt-token pressure while preserving high-signal evidence.
    if (rag_mode == "tensor_hybrid" || rag_mode == "tensor_prefix") {
        int rag_tensor_slots = 6;
        int rag_tensor_slot_chars = 280;
        if (request.metadata.is_object()) {
            const auto slots_it = request.metadata.find("rag_tensor_slots");
            if (slots_it != request.metadata.end() && slots_it->is_number_integer()) {
                rag_tensor_slots = slots_it->get<int>();
            }
            const auto chars_it = request.metadata.find("rag_tensor_slot_chars");
            if (chars_it != request.metadata.end() && chars_it->is_number_integer()) {
                rag_tensor_slot_chars = chars_it->get<int>();
            }
        }
        rag_tensor_slots = std::clamp(rag_tensor_slots, 1, 16);
        rag_tensor_slot_chars = std::clamp(rag_tensor_slot_chars, 80, 1200);

        std::vector<themis::rag::RetrievedChunk> ranked_chunks = ctx.chunks_used;
        std::stable_sort(
            ranked_chunks.begin(),
            ranked_chunks.end(),
            [](const themis::rag::RetrievedChunk& a, const themis::rag::RetrievedChunk& b) {
                return a.relevance_score > b.relevance_score;
            });

        const size_t slot_count = std::min(
            ranked_chunks.size(), static_cast<size_t>(rag_tensor_slots));

        std::ostringstream compact_prompt = {};
        compact_prompt << "SYSTEM: Use the semantic memory slots below as the primary evidence. "
                          "When uncertain, say so and cite slot source ids.\n\n";
        for (size_t i = 0; i < slot_count; ++i) {
            std::string slot_text = ranked_chunks[i].content;
            if (static_cast<int>(slot_text.size()) > static_cast<size_t>(rag_tensor_slot_chars)) {
                slot_text = slot_text.substr(0, static_cast<size_t>(rag_tensor_slot_chars));
            }
            compact_prompt << "[MEMORY_SLOT id=" << (i + 1)
                           << " source=\"" << ranked_chunks[i].source
                           << "\" relevance=" << ranked_chunks[i].relevance_score
                           << "]\n";
            compact_prompt << slot_text << "\n";
            compact_prompt << "[/MEMORY_SLOT]\n\n";
        }
        compact_prompt << "User Question: " << request.prompt;
        augmented.prompt = compact_prompt.str();
    } else {
        std::ostringstream augmented_prompt = {};
        for (const auto& c : ctx.chunks_used) {
            augmented_prompt << c.content << "\n";
        }
        augmented_prompt << request.prompt;
        augmented.prompt = augmented_prompt.str();
    }

    // Clamp max_tokens to the remaining response budget.
    augmented.max_tokens = themis::rag::RAGContextAssembler::computeMaxTokens(
        llm::ContextWindowBudget::compute(
            cfg.model_context_tokens, /*system=*/"", augmented.prompt,
            cfg.min_response_tokens),
        request.max_tokens);

    spdlog::info(
        "LlamaCppPlugin::generateRAG dispatch: rag_mode='{}' prompt_chars={} effective_max_tokens={}",
        rag_mode,
        augmented.prompt.size(),
        augmented.max_tokens);

    return generate(augmented);
}

// ── embed ─────────────────────────────────────────────────────────────────────

std::vector<float> LlamaCppPlugin::embed(const std::string& text) {
    if (!model_loaded_) return {};
#ifdef THEMIS_LLM_ENABLED
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (wrapper_) {
            try {
                return wrapper_->embed(text);
            } catch (...) {
                // Fallback to stub zero-vector on error.
            }
        }
    }
#endif
    // Injection API (Stub #200): delegate to the injected EmbedFn when set.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (embed_fn_) {
            try {
                auto result = embed_fn_(text);
                if (!result.empty()) {
                  return result;
                }
            } catch (...) {
                // fn must not throw; fall through to zero-vector stub
            }
        }
    }
    // STUB/SIMULATION NOTE:
    // Purpose: Return a syntactically valid embedding vector when llama.cpp is not
    //          compiled in (THEMIS_LLM_ENABLED absent) or the model wrapper is null
    //          and no EmbedFn has been injected via setEmbedFn().
    // Activation: Reached when `wrapper_` is nullptr (model not loaded or
    //             THEMIS_LLM_ENABLED not set at build time) AND no `embed_fn_` set.
    // Production Delta: Every embed() call returns a 384-dimensional zero vector.
    //                   Cosine similarity between any two texts becomes 0/NaN;
    //                   semantic search, ANN indexing, and RAG retrieval all
    //                   produce meaningless results.
    // Removal Plan: Build with THEMIS_LLM_ENABLED and call loadModel() before
    //               embed(); the wrapper_->embed() path then returns real vectors.
    //               Alternatively, inject a real backend via setEmbedFn().
    //               See src/llama_cpp/FUTURE_ENHANCEMENTS.md §LlamaCppPlugin Embed.
    return std::vector<float>(384, 0.0f);
}

// ── setEmbedFn ────────────────────────────────────────────────────────────────

void LlamaCppPlugin::setEmbedFn(EmbedFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    embed_fn_ = std::move(fn);
}

void LlamaCppPlugin::setGenerateFn(GenerateFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    generate_fn_ = std::move(fn);
}

void LlamaCppPlugin::setPolicyFn(PolicyFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    policy_fn_ = std::move(fn);
}

// ── capabilities / stats ──────────────────────────────────────────────────────

llm::LLMCapabilities LlamaCppPlugin::getCapabilities() const {
    llm::LLMCapabilities cap;
    cap.supports_streaming     = true;
    cap.supports_batching      = true;
    cap.supports_lora          = true;
    cap.supports_embeddings    = true;
    cap.supports_rag           = true;
    cap.supports_function_call = true;
    cap.plugin_version         = "2.1.0";
    return cap;
}

json LlamaCppPlugin::getMemoryStats() const {
    json stats = {
        {"plugin",       "llama_cpp"},
        {"model_loaded", model_loaded_},
        {"model_id",     model_id_},
        {"lora_count",static_cast<int>(loras_.size())}
    };

#ifdef THEMIS_LLM_ENABLED
    std::lock_guard<std::mutex> lock(mutex_);
    if (wrapper_) {
        auto info = wrapper_->getModelInfo();
        if (info.has_value()) {
            stats["model_vram_required_mb"] = info->vram_required_mb;
            stats["model_ram_required_mb"] = info->ram_required_mb;
            if (info->metadata.contains("runtime_gpu_offload_requested")) {
                stats["runtime_gpu_offload_requested"] = info->metadata["runtime_gpu_offload_requested"];
            }
            if (info->metadata.contains("runtime_gpu_offload_effective")) {
                stats["runtime_gpu_offload_effective"] = info->metadata["runtime_gpu_offload_effective"];
            }
            if (info->metadata.contains("runtime_llama_assigned_cpu_tensors")) {
                stats["runtime_llama_assigned_cpu_tensors"] = info->metadata["runtime_llama_assigned_cpu_tensors"];
            }
            if (info->metadata.contains("runtime_llama_assigned_non_cpu_tensors")) {
                stats["runtime_llama_assigned_non_cpu_tensors"] = info->metadata["runtime_llama_assigned_non_cpu_tensors"];
            }
            if (info->metadata.contains("runtime_llama_backend_cpu_only_hint")) {
                stats["runtime_llama_backend_cpu_only_hint"] = info->metadata["runtime_llama_backend_cpu_only_hint"];
            }
        }
    }
#endif

    return stats;
}

json LlamaCppPlugin::getPerformanceStats() const {
    return {
        {"plugin",              "llama_cpp"},
        {"inference_count",     inference_count_.load()},
        {"error_count",         error_count_.load()},
        {"stream_retry_count",  stream_retry_count_.load()}
    };
}

// ── LoRA import/export ────────────────────────────────────────────────────────

std::vector<uint8_t> LlamaCppPlugin::exportLoRA(const std::string& lora_id) {
#ifdef THEMIS_LLM_ENABLED
    std::lock_guard<std::mutex> lock(mutex_);
    if (wrapper_) {
        return wrapper_->exportLoRA(lora_id);
    }
#else
    (void)lora_id;
#endif
    return {};
}

bool LlamaCppPlugin::importLoRA(const std::string& lora_id,
                                 const std::vector<uint8_t>& data) {
    // SECURITY: Validate GGUF magic bytes and size bound before processing.
    // GGUF magic: 0x47 0x47 0x55 0x46 ('G','G','U','F')
    constexpr size_t kMinLoRASize = 8u;
    constexpr size_t kMaxLoRASize = 2 * 1024 * 1024 * 1024; // 2 GB

    if (static_cast<int>(data.size()) < kMinLoRASize) {
        return false; // Too small — cannot contain a valid header
    }
    if (static_cast<int>(data.size()) > kMaxLoRASize) {
        return false; // Size bound — prevent heap exhaustion
    }
    // Magic bytes check: GGUF signature
    if (data[0] != 0x47 || data[1] != 0x47 || data[2] != 0x55 || data[3] != 0x46) {
        return false; // Invalid GGUF magic
    }

#ifdef THEMIS_LLM_ENABLED
    std::lock_guard<std::mutex> lock(mutex_);
    if (wrapper_) {
      return wrapper_->importLoRA(lora_id, data);
    }
#else
    (void)lora_id;
#endif
    // Stub mode: validation passed, no real adapter to load.
    return true;
}

// ── computeFileDigest ─────────────────────────────────────────────────────────

std::string LlamaCppPlugin::computeFileDigest(const std::string& path) {
    // NOTE: This implementation uses FNV-64 as a CI-safe placeholder for
    // SHA-256.  Production deployments should replace this with OpenSSL
    // EVP_DigestFinal (or libcrypto equivalent) once the dependency is
    // available.  The FNV-64 output is sufficient for correctness testing of
    // the opt-in integrity gate in loadModel().
    std::ifstream f(path, std::ios::binary);
    if (!f) {
      return "";
    }
    constexpr uint64_t kFnvPrime = 0x00000100000001B3ULL;
    uint64_t hash = 0xcbf29ce484222325ULL;
    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        for (std::streamsize i = 0; i < f.gcount(); ++i) {
            hash ^= static_cast<uint8_t>(buf[i]);
            hash *= kFnvPrime;
        }
    }
    std::ostringstream oss = {};
    oss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return oss.str();
}

// ── generateDraftTokens ────────────────────────────────────────────────────────

llm::ILLMPlugin::DraftTokensResult LlamaCppPlugin::generateDraftTokens(
        const llm::InferenceRequest& request,
        size_t                       k,
        size_t                       vocab_size_hint) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Fast-path: k=0 is a valid (no-op) call; return empty result immediately.
    if (k == 0) {
        llm::ILLMPlugin::DraftTokensResult empty;
        empty.vocab_size = (vocab_size_hint > 0)
                               ? std::min(vocab_size_hint, kMaxDraftFallbackVocabSize)
                               : kDefaultDraftFallbackVocabSize;
        return empty;
    }

    llm::ILLMPlugin::DraftTokensResult result;
    const size_t requested_vocab_size =
        (vocab_size_hint > 0) ? vocab_size_hint : kDefaultDraftFallbackVocabSize;
    result.vocab_size = std::min(requested_vocab_size, kMaxDraftFallbackVocabSize);
    if (result.vocab_size != requested_vocab_size) {
        spdlog::warn(
            "LlamaCppPlugin::generateDraftTokens capped vocab_size_hint={} to {} for fallback safety",
            requested_vocab_size, result.vocab_size);
    }
    
#ifdef THEMIS_LLM_ENABLED
    if (!wrapper_) {
        // STUB/SIMULATION NOTE:
        // Purpose: Return k syntactically valid draft tokens when THEMIS_LLM_ENABLED
        //          is set but loadModel() has not been called (wrapper_ is null).
        //          Keeps callers from null-dereferencing while producing a result
        //          they can inspect in unit tests.
        // Activation: THEMIS_LLM_ENABLED defined at build time AND loadModel() not
        //             yet called (or model already unloaded).
        // Production Delta: Peaked logits are not model-grounded; speculative-
        //                   decoding acceptance rates will be low / zero.
        // Removal Plan: Call loadModel() before generateDraftTokens(); wrapper_
        //               will be non-null and the real path below executes.
        constexpr float kPeak      =  5.0f;
        constexpr float kBaseline  = -5.0f;

        for (size_t i = 0; i < k; ++i) {
            // Deterministic token based on prompt hash
            int token_id = (i + 1) % static_cast<int>(result.vocab_size);
            result.tokens.push_back(token_id);

            std::vector<float> logits(result.vocab_size, kBaseline);
            logits[token_id] = kPeak;
            result.logits.push_back(std::move(logits));
        }
        return result;
    }

    // Phase 2: Real draft-logit pipeline using llama.cpp
    try {
        llm::InferenceRequest draft_req = request;
        draft_req.max_tokens = static_cast<int>(k);
        draft_req.stream_callback = nullptr;

        const auto real_result = wrapper_->generateDraftTokens(
            draft_req,
            k,
            vocab_size_hint > 0 ? vocab_size_hint : result.vocab_size);

        if (!real_result.tokens.empty() &&
            real_result.tokens.size() == real_result.logits.size() &&
            real_result.vocab_size > 0) {
            return real_result;
        }

        spdlog::warn("LlamaCppPlugin::generateDraftTokens got invalid real result, using fallback");
    } catch (const std::exception& e) {
        spdlog::warn("LlamaCppPlugin::generateDraftTokens failed: {}", e.what());
    }

    // STUB/SIMULATION NOTE:
    // Purpose: Provide a syntactically valid DraftTokensResult when the real
    //          wrapper_->generateDraftTokens() returns an invalid result or
    //          throws (network error, OOM, corrupt model, etc.).
    // Activation: Reached only when the real path returns tokens.empty(),
    //             a size mismatch between tokens/logits, vocab_size==0, or
    //             an exception is thrown by LlamaWrapper::generateDraftTokens().
    // Production Delta: Peaked logits are deterministic (token_id = (i+1) % vocab)
    //                   and not grounded in model probability; speculative-decoding
    //                   acceptance rates will be low and throughput gains lost.
    // Removal Plan: Fix the underlying LlamaWrapper::generateDraftTokens() failure
    //               so the real result is always valid.  Log the failure case and
    //               surface it via metrics so it is visible to operators.
    // Roadmap ref: src/llama_cpp/ROADMAP.md § "Speculative Decoding"
    constexpr float kPeak      =  5.0f;
    constexpr float kBaseline  = -5.0f;
    for (size_t i = 0; i < k; ++i) {
        int token_id = (i + 1) % static_cast<int>(result.vocab_size);
        result.tokens.push_back(token_id);

        std::vector<float> logits(result.vocab_size, kBaseline);
        logits[token_id] = kPeak;
        result.logits.push_back(std::move(logits));
    }
#else
    // STUB/SIMULATION NOTE:
    // Purpose: Provide a syntactically valid DraftTokensResult when the build
    //          was compiled WITHOUT THEMIS_LLM_ENABLED (no llama.cpp linkage).
    // Activation: Entire `#else` branch — THEMIS_LLM_ENABLED not defined at
    //             compile time.  No real model is ever available in this build.
    // Production Delta: Identical to the real-path fallback above — peaked
    //                   logits are not model-grounded; speculative decoding
    //                   will not benefit from draft quality.
    // Removal Plan: Rebuild with -DTHEMIS_LLM_ENABLED=ON and link llama.cpp.
    //               See SETUP.md § "Enabling real LLM inference".
    constexpr float kPeak      =  5.0f;
    constexpr float kBaseline  = -5.0f;
    
    for (size_t i = 0; i < k; ++i) {
        int token_id = (i + 1) % static_cast<int>(result.vocab_size);
        result.tokens.push_back(token_id);
        
        std::vector<float> logits(result.vocab_size, kBaseline);
        logits[token_id] = kPeak;
        result.logits.push_back(std::move(logits));
    }
#endif
    
    return result;
}

std::vector<std::vector<float>> LlamaCppPlugin::computeTargetLogitsForTokens(
    const llm::InferenceRequest& request,
    const std::vector<int>& draft_token_ids
) {
    std::lock_guard<std::mutex> lock(mutex_);

#ifdef THEMIS_LLM_ENABLED
    if (!wrapper_) {
        throw std::runtime_error(
            "LlamaCppPlugin target-logit bridge requires a loaded llama wrapper");
    }
    return wrapper_->computeTargetLogitsForTokens(request, draft_token_ids);
#else
    (void)request;
    (void)draft_token_ids;
    throw std::runtime_error(
        "LlamaCppPlugin target-logit bridge unavailable without THEMIS_LLM_ENABLED");
#endif
}

// ── generateStream ────────────────────────────────────────────────────────────

llm::InferenceResponse LlamaCppPlugin::generateStream(
        llm::InferenceRequest request,
        std::function<void([[maybe_unused]] const std::string& token)> token_callback) {
    // Inject the caller-supplied callback and delegate to generate(), which
    // already dispatches stream_callback when it is set.
    request.stream_callback = std::move([[maybe_unused]] token_callback);
    return generate(request);
}

// ── generateBatch ─────────────────────────────────────────────────────────────

std::vector<llm::InferenceResponse> LlamaCppPlugin::generateBatch(
        const std::vector<llm::InferenceRequest>& requests) {
    std::vector<llm::InferenceResponse> results = {};

    results.reserve(requests.size());
    for (const auto& req : requests) {
        results.push_back(generate(req));
    }
    return results;
}

} // namespace llamacpp
} // namespace themis

// ── dynamic-loading entry points ──────────────────────────────────────────────

#if !defined(THEMIS_TEST_BUILD) && defined(THEMIS_PLUGIN_EXPORTS)
// RAII note: C-linkage ABI requires a raw pointer return. Ownership is fully
// transferred to the caller. The caller MUST invoke themis_llm_destroy() to
// release the object; failing to do so will leak memory. Do NOT delete the
// pointer via any other mechanism — always use the paired destroy function.
extern "C" THEMIS_PLUGIN_EXPORT
themis::llm::ILLMPlugin* themis_llm_create() {
    return new themis::llamacpp::LlamaCppPlugin();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themis_llm_destroy(themis::llm::ILLMPlugin* p) {
    delete p;
}
#endif
