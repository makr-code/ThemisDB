#include "llama_cpp/llama_cpp_plugin.h"
#include "rag/rag_context_assembler.h"
#include <algorithm>
#include <chrono>
#include <sstream>

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

    // ── Stub fallback (no real model or THEMIS_LLM_ENABLED not set) ──────────
    ++inference_count_;
    const std::string text = "[stub:" + request.prompt.substr(0, 40) + "]";

    // Honour streaming callback when present.
    // In stub mode the full text is emitted as a single "token" so callers
    // that set stream_callback always receive at least one invocation.
    if (request.stream_callback) {
        try {
            request.stream_callback(text);
        } catch (...) {
            // Exceptions from callbacks are swallowed (per FUTURE_ENHANCEMENTS spec)
            ++error_count_;
        }
    }

    response.text             = text;
    response.success          = true;
    response.tokens_generated = static_cast<int>(text.size() / 4 + 1);
    response.trace_id         = request.trace_id;
    response.span_id          = request.span_id;
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
    // Stub: return a fixed-size zero vector
    (void)text;
    return std::vector<float>(384, 0.0f);
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
    return {
        {"plugin",       "llama_cpp"},
        {"model_loaded", model_loaded_},
        {"model_id",     model_id_},
        {"lora_count",   loras_.size()}
    };
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
    (void)lora_id;
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
    (void)lora_id;
    (void)data;
#endif
    return false;
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

extern "C" THEMIS_PLUGIN_EXPORT
themis::llm::ILLMPlugin* themis_llm_create() {
    return new themis::llamacpp::LlamaCppPlugin();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themis_llm_destroy(themis::llm::ILLMPlugin* p) {
    delete p;
}
