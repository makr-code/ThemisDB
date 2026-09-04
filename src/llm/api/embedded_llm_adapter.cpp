#include "themis/llm/llm_factory.h"
#include "llm/embedded_llm.h"
namespace themis {
namespace llm {

EmbeddedLLM::EmbeddedLLM() : EmbeddedLLM(Config{}) {}
EmbeddedLLM::EmbeddedLLM(const Config& cfg) : config_(cfg) {}
EmbeddedLLM::~EmbeddedLLM() = default;

bool EmbeddedLLM::isReady() const {
    auto impl = createEmbeddedLLM();
    return impl ? impl->isReady() : false;
}

std::string EmbeddedLLM::generate(const std::string& prompt, int max_tokens) {
    auto impl = createEmbeddedLLM();
    if (!impl) {
      return std::string();
    }
    return impl->generateWithParams(prompt, 0.7f, 0.9f, max_tokens);
}

std::string EmbeddedLLM::generateWithParams(const std::string& prompt, float temperature, float top_p, int max_tokens) {
    auto impl = createEmbeddedLLM();
    if (!impl) {
      return std::string();
    }
    return impl->generateWithParams(prompt, temperature, top_p, max_tokens);
}

std::string EmbeddedLLM::chat(const std::vector<ChatMessage>& messages, ChatFormat format) {
    auto impl = createEmbeddedLLM();
    return impl ? impl->chat(messages, format) : std::string();
}

std::vector<float> EmbeddedLLM::embed(const std::string& text) {
    auto impl = createEmbeddedLLM();
    return impl ? impl->embed(text) : std::vector<float>();
}

std::vector<std::vector<float>> EmbeddedLLM::embedBatch(const std::vector<std::string>& texts) {
    auto impl = createEmbeddedLLM();
    return impl ? impl->embedBatch(texts) : std::vector<std::vector<float>>();
}

std::string EmbeddedLLM::generateStreaming(const std::string& prompt, std::function<void(const std::string&)> callback, int max_tokens) {
    auto impl = createEmbeddedLLM();
    if (!impl) {
      return std::string();
    }
    (void)max_tokens;
    std::string out;
    impl->generateStreaming(prompt, [&]([[maybe_unused]] const std::string& token){
        out += token;
        callback([[maybe_unused]] token);
    });
    return out;
}

json EmbeddedLLM::generateAsMCP(const std::string& prompt, int max_tokens) {
    auto impl = createEmbeddedLLM();
    if (!impl) {
      return json::object();
    }
    return impl->generateAsMCP(prompt, max_tokens);
}

json EmbeddedLLM::generateAsJsonMarkdown(const std::string& prompt, int max_tokens) {
    auto impl = createEmbeddedLLM();
    if (!impl) {
      return json::object();
    }
    return impl->generateAsJsonMarkdown(prompt, max_tokens);
}

InferenceResponse EmbeddedLLM::generateFull(const InferenceRequest& request) {
    auto impl = createEmbeddedLLM();
    if (!impl) return InferenceResponse{};
    return impl->generateFull(request);
}

bool EmbeddedLLM::hasEthicalGuidelines() const {
    auto impl = createEmbeddedLLM();
    return impl ? impl->hasEthicalGuidelines() : false;
}

EthicalGuidelinesManager* EmbeddedLLM::getEthicalGuidelines() {
    auto impl = createEmbeddedLLM();
    return impl ? impl->getEthicalGuidelines() : nullptr;
}

json EmbeddedLLM::getStats() const {
    auto impl = createEmbeddedLLM();
    return impl ? impl->getStats() : json::object();
}

void EmbeddedLLM::setGenerateFullFn(GenerateFullFn fn) {
    // No-op in API shim; forwarders may ignore overrides.
    (void)fn;
}

void EmbeddedLLM::setEmbedFn(EmbedFn fn) {
    (void)fn;
}

void EmbeddedLLM::clearCache() {
    auto impl = createEmbeddedLLM();
    if (impl) {
      impl->clearCache();
    }
}

std::string EmbeddedLLM::getModelInfo() const {
    auto impl = createEmbeddedLLM();
    return impl ? impl->getModelInfo() : std::string();
}

// Note: `isReady()` const overload is defined above; avoid duplicate non-const overload.

} // namespace llm
} // namespace themis
