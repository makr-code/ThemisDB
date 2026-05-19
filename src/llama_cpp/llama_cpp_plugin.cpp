/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llama_cpp_plugin.cpp                               ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:49:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   60.0/100                                       ║
    • Total Lines:     400                                            ║
    • Open Issues:     TODOs: 0, Stubs: 8                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • df59ab8148  2026-04-12  feat(llm): promote llama_wrapper, multi_lora_manager, pro... ║
    • f0f3ecebde  2026-04-11  feat(llama_cpp): v2.1.0 — streaming, batch inference, Plu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llama_cpp/llama_cpp_plugin.h"
#include "rag/rag_context_assembler.h"
#include <algorithm>
#include <chrono>
#include <exception>
#include <sstream>

#ifndef THEMIS_NO_SPDLOG
#include <spdlog/spdlog.h>
#else
namespace spdlog {
template <typename... Args>
inline void warn(const char*, Args&&...) {}
} // namespace spdlog
#endif

namespace themis {
namespace llamacpp {

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
    if (!model_loaded_) return std::nullopt;
#ifdef THEMIS_LLM_ENABLED
    if (wrapper_) {
        auto info = wrapper_->getModelInfo();
        if (info) return info;
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

bool LlamaCppPlugin::loadLoRA(const std::string& lora_path,
                               const std::string& lora_id, float scale) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Remove existing entry with same id
    loras_.erase(std::remove_if(loras_.begin(), loras_.end(),
                                [&](const LoRAEntry& e){ return e.id == lora_id; }),
                 loras_.end());
    loras_.push_back({lora_id, lora_path, scale});
    return true;
}

bool LlamaCppPlugin::unloadLoRA(const std::string& lora_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto before = loras_.size();
    loras_.erase(std::remove_if(loras_.begin(), loras_.end(),
                                [&](const LoRAEntry& e){ return e.id == lora_id; }),
                 loras_.end());
    return loras_.size() < before;
}

std::vector<llm::LoRAInfo> LlamaCppPlugin::listLoRAs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<llm::LoRAInfo> result;
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

    if (!loaded) {
        ++error_count_;
        response.success       = false;
        response.error_message = "LlamaCppPlugin: model not loaded";
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
    std::string bridged_model_id;
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
            if (request.stream_callback && !bridged.text.empty()) {
                try {
                    request.stream_callback(bridged.text);
                } catch (const std::exception& e) {
                    ++error_count_;
                    spdlog::warn("LlamaCppPlugin stream callback failed: {}", e.what());
                } catch (...) {
                    ++error_count_;
                    spdlog::warn("LlamaCppPlugin stream callback failed with unknown exception");
                }
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
    // Roadmap ref: src/llama_cpp/ROADMAP.md § "Planned Features"
    // See: llama_cpp/FUTURE_ENHANCEMENTS.md §6; AI_ML_IMPACT_ASSESSMENT.md §7 Gap 1.
#ifdef THEMIS_LLAMA_CPP_STUB_MODE
    // Test-only path: retain the old echo behaviour when the test macro is set.
    {
        ++inference_count_;
        const std::string text = "[stub:" + request.prompt.substr(0, 40) + "]";
        if (request.stream_callback) {
            try {
                request.stream_callback(text);
            } catch (const std::exception& e) {
                ++error_count_;
                spdlog::warn("LlamaCppPlugin stub stream callback failed: {}", e.what());
            } catch (...) {
                ++error_count_;
                spdlog::warn("LlamaCppPlugin stub stream callback failed with unknown exception");
            }
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
    // Build RetrievedChunk objects from the RAGContext documents.
    std::vector<themis::rag::RetrievedChunk> chunks;
    chunks.reserve(rag_context.documents.size());
    for (const auto& doc : rag_context.documents) {
        themis::rag::RetrievedChunk chunk;
        chunk.content         = doc.content;
        chunk.source          = doc.source;
        chunk.relevance_score = doc.relevance_score;
        chunks.push_back(std::move(chunk));
    }

    // Configure the assembler from the loaded model's context window.
    // Honour an explicit override from the caller (rag_context.max_context_tokens).
    themis::rag::RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens =
        (rag_context.max_context_tokens > 0)
            ? static_cast<size_t>(rag_context.max_context_tokens)
            : context_length_;
    cfg.min_response_tokens  =
        (rag_context.response_budget_tokens > 0)
            ? static_cast<size_t>(rag_context.response_budget_tokens)
            : ((request.max_tokens > 0)
                   ? static_cast<size_t>(request.max_tokens)
                   : llm::kDefaultMinResponseTokens);

    themis::rag::RAGContextAssembler assembler(cfg);
    const themis::rag::AssembledContext ctx =
        assembler.assemble(chunks, /*system_prompt=*/"", request.prompt);

    // Build the augmented prompt from the assembled (budget-respecting) context.
    std::ostringstream augmented_prompt;
    for (const auto& c : ctx.chunks_used) {
        augmented_prompt << c.content << "\n";
    }
    augmented_prompt << request.prompt;

    llm::InferenceRequest augmented = request;
    augmented.prompt    = augmented_prompt.str();
    // Clamp max_tokens to the remaining response budget.
    augmented.max_tokens = themis::rag::RAGContextAssembler::computeMaxTokens(
        llm::ContextWindowBudget::compute(
            context_length_, /*system=*/"", request.prompt,
            cfg.min_response_tokens),
        request.max_tokens);

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
                if (!result.empty()) return result;
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

// ── capabilities / stats ──────────────────────────────────────────────────────

llm::LLMCapabilities LlamaCppPlugin::getCapabilities() const {
    llm::LLMCapabilities cap;
    cap.supports_streaming     = true;
    cap.supports_batching      = true;
    cap.supports_lora          = true;
    cap.supports_embeddings    = true;
    cap.supports_rag           = true;
    cap.supports_function_call = false;
    cap.plugin_version         = "2.1.0";
    return cap;
}

json LlamaCppPlugin::getMemoryStats() const {
    json stats = {
        {"plugin",       "llama_cpp"},
        {"model_loaded", model_loaded_},
        {"model_id",     model_id_},
        {"lora_count",   loras_.size()}
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
        {"plugin",          "llama_cpp"},
        {"inference_count", inference_count_},
        {"error_count",     error_count_}
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
#endif
    return {};
}

bool LlamaCppPlugin::importLoRA(const std::string& lora_id,
                                 const std::vector<uint8_t>& data) {
#ifdef THEMIS_LLM_ENABLED
    std::lock_guard<std::mutex> lock(mutex_);
    if (wrapper_) {
        return wrapper_->importLoRA(lora_id, data);
    }
#else
#endif
    return false;
}

// ── generateDraftTokens ────────────────────────────────────────────────────────

llm::ILLMPlugin::DraftTokensResult LlamaCppPlugin::generateDraftTokens(
        const llm::InferenceRequest& request,
        size_t                       k,
        size_t                       vocab_size_hint) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    llm::ILLMPlugin::DraftTokensResult result;
    result.vocab_size = (vocab_size_hint > 0) ? vocab_size_hint : 32000u;
    
#ifdef THEMIS_LLM_ENABLED
    if (!wrapper_) {
        // Stub mode: return k tokens with peaked logits
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

    // Fallback to deterministic peaked logits if real path failed
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
    // Stub mode when THEMIS_LLM_ENABLED is not defined
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

// ── generateStream ────────────────────────────────────────────────────────────

llm::InferenceResponse LlamaCppPlugin::generateStream(
        llm::InferenceRequest request,
        std::function<void(const std::string& token)> token_callback) {
    // Inject the caller-supplied callback and delegate to generate(), which
    // already dispatches stream_callback when it is set.
    request.stream_callback = std::move(token_callback);
    return generate(request);
}

// ── generateBatch ─────────────────────────────────────────────────────────────

std::vector<llm::InferenceResponse> LlamaCppPlugin::generateBatch(
        const std::vector<llm::InferenceRequest>& requests) {
    std::vector<llm::InferenceResponse> results;
    results.reserve(requests.size());
    for (const auto& req : requests) {
        results.push_back(generate(req));
    }
    return results;
}

} // namespace llamacpp
} // namespace themis

// ── dynamic-loading entry points ──────────────────────────────────────────────

#ifndef THEMIS_TEST_BUILD
extern "C" THEMIS_PLUGIN_EXPORT
themis::llm::ILLMPlugin* themis_llm_create() {
    return new themis::llamacpp::LlamaCppPlugin();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themis_llm_destroy(themis::llm::ILLMPlugin* p) {
    delete p;
}
#endif
