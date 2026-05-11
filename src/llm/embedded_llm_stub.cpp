/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            embedded_llm_stub.cpp                              ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:49:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     208                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// STUB/SIMULATION NOTE:
// Purpose: Provide a no-op EmbeddedLLM implementation that compiles and links when
//          THEMIS_ENABLE_LLM=OFF (i.e. the llama.cpp / LlamaWrapper dependency is
//          absent).  All generate/embed calls return empty or a "LLM disabled" string
//          so that the rest of the server stack can boot without a model file.
// Activation: Compiled when THEMIS_ENABLE_LLM is NOT defined (default CI/dev builds).
//             Build with -DTHEMIS_ENABLE_LLM=ON to compile embedded_llm.cpp instead,
//             which provides full inference via LlamaWrapper.
// Production Delta: No actual token generation; embed() always returns {}.
//                   isReady() always returns false; getStats() returns llm_enabled=false.
//                   Any request relying on LLM output will receive the static fallback
//                   string "LLM disabled at build time (THEMIS_ENABLE_LLM=OFF)."
// Removal Plan: This file is permanently retained as the no-LLM build path.
//               It is NOT removed when LLM is enabled — the build system selects
//               either this file or embedded_llm.cpp via CMake source-list logic.
// Roadmap ref: src/llm/ROADMAP.md § "Phase 1: EmbeddedLLM stub → full LlamaWrapper"

#include "llm/embedded_llm.h"

#include <stdexcept>

namespace themis {
namespace llm {

EmbeddedLLM::EmbeddedLLM()
    : EmbeddedLLM(Config{}) {}

EmbeddedLLM::EmbeddedLLM(const Config& config)
    : config_(config) {}

EmbeddedLLM::~EmbeddedLLM() = default;

void EmbeddedLLM::setGenerateFullFn(GenerateFullFn fn) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    generate_full_fn_ = std::move(fn);
}

void EmbeddedLLM::setEmbedFn(EmbedFn fn) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    embed_fn_ = std::move(fn);
}

std::string EmbeddedLLM::generate(const std::string& prompt, int max_tokens) {
    auto req = createRequest(prompt, max_tokens);
    return generateFull(req).text;
}

std::string EmbeddedLLM::generateWithParams(
    const std::string& prompt,
    float temperature,
    float top_p,
    int max_tokens
) {
    auto req = createRequest(prompt, max_tokens, temperature, top_p);
    return generateFull(req).text;
}

std::string EmbeddedLLM::chat(
    const std::vector<ChatMessage>& messages,
    [[maybe_unused]] ChatFormat format
) {
    std::string merged;
    for (const auto& m : messages) {
        if (!merged.empty()) {
            merged.push_back('\n');
        }
        merged.append(m.role);
        merged.append(": ");
        merged.append(m.content);
    }
    return generate(merged);
}

std::string EmbeddedLLM::chatSimple(
    const std::string& system_prompt,
    const std::string& user_message
) {
    return chat({{"system", system_prompt}, {"user", user_message}});
}

std::vector<float> EmbeddedLLM::embed([[maybe_unused]] const std::string& text) {
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (embed_fn_) {
            try {
                auto result = embed_fn_(text);
                if (!result.empty()) {
                    return result;
                }
            } catch (...) {
                // Intentional fail-closed behavior: misconfigured bridge must not
                // make the no-LLM build throw.
            }
        }
    }
    return {};
}

std::vector<std::vector<float>> EmbeddedLLM::embedBatch(const std::vector<std::string>& texts) {
    std::vector<std::vector<float>> out;
    out.reserve(texts.size());
    for ([[maybe_unused]] const auto& t : texts) {
        out.push_back({});
    }
    return out;
}

std::string EmbeddedLLM::generateStreaming(
    const std::string& prompt,
    std::function<void(const std::string& token)> callback,
    int max_tokens
) {
    auto text = generate(prompt, max_tokens);
    if (callback) {
        callback(text);
    }
    return text;
}

std::string EmbeddedLLM::generateStreamingSSE(
    const std::string& prompt,
    std::function<void(const std::string& sse_event)> callback,
    const std::string& request_id,
    int max_tokens
) {
    auto text = generate(prompt, max_tokens);
    if (callback) {
        callback("event: done\ndata: {\"request_id\":\"" + request_id + "\",\"text\":\"\"}\n\n");
    }
    return text;
}

json EmbeddedLLM::generateAsMCP(const std::string& prompt, int max_tokens) {
    auto text = generate(prompt, max_tokens);
    return json{{"text", text}, {"llm_enabled", false}};
}

json EmbeddedLLM::generateAsJsonMarkdown(const std::string& prompt, int max_tokens) {
    auto text = generate(prompt, max_tokens);
    return json{{"markdown", text}, {"llm_enabled", false}};
}

InferenceResponse EmbeddedLLM::generateFull(const InferenceRequest& request) {
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (generate_full_fn_) {
            try {
                auto response = generate_full_fn_(request);
                if (request.stream_callback && !response.text.empty()) {
                    try {
                        request.stream_callback(response.text);
                    } catch (...) {
                        // Ignore callback failures on the bridge path.
                    }
                }
                return response;
            } catch (...) {
                // Fall through to the disabled stub response.
            }
        }
    }

    InferenceResponse resp;
    resp.request_id = request.request_id;
    resp.model_id = request.model_id;
    resp.model_used = request.model_id;
    resp.trace_id = request.trace_id;
    resp.span_id = request.span_id;
    resp.text = "LLM disabled at build time (THEMIS_ENABLE_LLM=OFF).";
    resp.metadata = json{{"llm_enabled", false}};
    return resp;
}

bool EmbeddedLLM::isReady() const {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    return static_cast<bool>(generate_full_fn_) || static_cast<bool>(embed_fn_);
}

std::string EmbeddedLLM::getModelInfo() const {
    return "LLM disabled";
}

json EmbeddedLLM::getStats() const {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    return json{
        {"llm_enabled", static_cast<bool>(generate_full_fn_)},
        {"embedding_enabled", static_cast<bool>(embed_fn_)},
        {"initialized", static_cast<bool>(generate_full_fn_) || static_cast<bool>(embed_fn_)}
    };
}

void EmbeddedLLM::clearCache() {}

EthicalGuidelinesManager* EmbeddedLLM::getEthicalGuidelines() {
    return nullptr;
}

bool EmbeddedLLM::hasEthicalGuidelines() const {
    return false;
}

InferenceRequest EmbeddedLLM::createRequest(
    const std::string& prompt,
    int max_tokens,
    float temperature,
    float top_p
) {
    InferenceRequest req;
    req.prompt = prompt;
    req.model_id = config_.model_id;
    req.max_tokens = max_tokens;
    req.temperature = temperature;
    req.top_p = top_p;
    return req;
}

std::string EmbeddedLLM::applyEthicalGuidelines(
    const std::string& prompt,
    [[maybe_unused]] const std::string& context_text
) {
    return prompt;
}

EmbeddedLLMManager& EmbeddedLLMManager::instance() {
    static EmbeddedLLMManager mgr;
    return mgr;
}

void EmbeddedLLMManager::initialize(const EmbeddedLLM::Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    llm_ = std::make_unique<EmbeddedLLM>(config);
    initialized_ = true;
}

EmbeddedLLM& EmbeddedLLMManager::get() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!llm_) {
        llm_ = std::make_unique<EmbeddedLLM>();
    }
    initialized_ = true;
    return *llm_;
}

bool EmbeddedLLMManager::isInitialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_ && static_cast<bool>(llm_);
}

} // namespace llm
} // namespace themis
