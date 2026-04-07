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

    model_loaded_ = true;
    return true;
}

void LlamaCppPlugin::unloadModel() {
    std::lock_guard<std::mutex> lock(mutex_);
    model_loaded_ = false;
    model_id_.clear();
    model_path_.clear();
    loras_.clear();
}

// ── getModelInfo ──────────────────────────────────────────────────────────────

std::optional<llm::ModelInfo> LlamaCppPlugin::getModelInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!model_loaded_) return std::nullopt;
    llm::ModelInfo info;
    info.model_id      = model_id_;
    info.model_path    = model_path_;
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
        info.lora_path = e.path;
        info.scale     = e.scale;
        result.push_back(std::move(info));
    }
    return result;
}

// ── generate ──────────────────────────────────────────────────────────────────

llm::InferenceResponse LlamaCppPlugin::generate(const llm::InferenceRequest& request) {
    llm::InferenceResponse response;
    if (!model_loaded_) {
        ++error_count_;
        response.success = false;
        response.error_message = "LlamaCppPlugin: model not loaded";
        return response;
    }
    // Stub: echo back the prompt as the generated text
    ++inference_count_;
    response.text    = "[stub:" + request.prompt.substr(0, 40) + "]";
    response.success = true;
    return response;
}

llm::InferenceResponse LlamaCppPlugin::generateRAG(
        const llm::InferenceRequest& request,
        const std::vector<std::string>& context_docs) {
    // Build RetrievedChunk objects from the raw string documents.
    std::vector<themis::rag::RetrievedChunk> chunks;
    chunks.reserve(context_docs.size());
    for (size_t i = 0; i < context_docs.size(); ++i) {
        themis::rag::RetrievedChunk chunk;
        chunk.content         = context_docs[i];
        chunk.relevance_score = static_cast<float>(context_docs.size() - i);
        chunks.push_back(std::move(chunk));
    }

    // Configure the assembler from the loaded model's context window.
    themis::rag::RAGContextAssemblerConfig cfg;
    cfg.model_context_tokens = context_length_;
    cfg.min_response_tokens  =
        (request.max_tokens > 0)
            ? static_cast<size_t>(request.max_tokens)
            : llm::kDefaultMinResponseTokens;

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
    // Stub: return a fixed-size zero vector
    (void)text;
    return std::vector<float>(384, 0.0f);
}

// ── capabilities / stats ──────────────────────────────────────────────────────

llm::LLMCapabilities LlamaCppPlugin::getCapabilities() const {
    llm::LLMCapabilities cap;
    cap.supports_streaming     = false;
    cap.supports_lora          = true;
    cap.supports_embeddings    = true;
    cap.supports_rag           = true;
    cap.supports_function_call = false;
    cap.plugin_version         = "2.0.0";
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

// ── LoRA import/export (stubs) ────────────────────────────────────────────────

std::vector<uint8_t> LlamaCppPlugin::exportLoRA(const std::string& /*lora_id*/) {
    return {};
}

bool LlamaCppPlugin::importLoRA(const std::string& /*lora_data_base64*/,
                                 const std::string& /*lora_id*/) {
    return false;
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
