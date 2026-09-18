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

/**
 * @brief Generate.
 * @param[in] prompt Input parameter.
 * @param[in] max_tokens Input parameter.
 * @return Return value.
 * @details Calls: createEmbeddedLLM(), std::string(), generateWithParams().
 */
std::string EmbeddedLLM::generate(const std::string& prompt, int max_tokens) {
    auto impl = createEmbeddedLLM();
    if (!impl) {
      return std::string();
    }
    return impl->generateWithParams(prompt, 0.7f, 0.9f, max_tokens);
}

/**
 * @brief Generate With Params.
 * @param[in] prompt Input parameter.
 * @param[in] temperature Input parameter.
 * @param[in] top_p Input parameter.
 * @param[in] max_tokens Input parameter.
 * @return Return value.
 * @details Calls: createEmbeddedLLM(), std::string().
 */
std::string EmbeddedLLM::generateWithParams(const std::string& prompt, float temperature, float top_p, int max_tokens) {
    auto impl = createEmbeddedLLM();
    if (!impl) {
      return std::string();
    }
    return impl->generateWithParams(prompt, temperature, top_p, max_tokens);
}

/**
 * @brief Chat.
 * @param[in] messages Input parameter.
 * @param[in] format Input parameter.
 * @return Return value.
 * @details Calls: createEmbeddedLLM(), std::string().
 */
std::string EmbeddedLLM::chat(const std::vector<ChatMessage>& messages, ChatFormat format) {
    auto impl = createEmbeddedLLM();
    return impl ? impl->chat(messages, format) : std::string();
}

/**
 * @brief Embed.
 * @param[in] text Input parameter.
 * @return Return value.
 * @details Calls: createEmbeddedLLM().
 */
std::vector<float> EmbeddedLLM::embed(const std::string& text) {
    auto impl = createEmbeddedLLM();
    return impl ? impl->embed(text) : std::vector<float>();
}

/**
 * @brief Embed Batch.
 * @param[in] texts Input parameter.
 * @return Return value.
 * @details Calls: createEmbeddedLLM().
 */
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
    std::string out = {};
    impl->generateStreaming(prompt, [&](const std::string& token){
        out += token;
        callback(token);
    });
    return out;
}

/**
 * @brief Generate As MCP.
 * @param[in] prompt Input parameter.
 * @param[in] max_tokens Input parameter.
 * @return Return value.
 * @details Calls: createEmbeddedLLM(), json::object().
 */
json EmbeddedLLM::generateAsMCP(const std::string& prompt, int max_tokens) {
    auto impl = createEmbeddedLLM();
    if (!impl) {
      return json::object();
    }
    return impl->generateAsMCP(prompt, max_tokens);
}

/**
 * @brief Generate As Json Markdown.
 * @param[in] prompt Input parameter.
 * @param[in] max_tokens Input parameter.
 * @return Return value.
 * @details Calls: createEmbeddedLLM(), json::object().
 */
json EmbeddedLLM::generateAsJsonMarkdown(const std::string& prompt, int max_tokens) {
    auto impl = createEmbeddedLLM();
    if (!impl) {
      return json::object();
    }
    return impl->generateAsJsonMarkdown(prompt, max_tokens);
}

/**
 * @brief Generate Full.
 * @param[in] request Input parameter.
 * @return Return value.
 * @details Calls: createEmbeddedLLM().
 */
InferenceResponse EmbeddedLLM::generateFull(const InferenceRequest& request) {
    auto impl = createEmbeddedLLM();
    if (!impl) return InferenceResponse{};
    return impl->generateFull(request);
}

bool EmbeddedLLM::hasEthicalGuidelines() const {
    auto impl = createEmbeddedLLM();
    return impl ? impl->hasEthicalGuidelines() : false;
}

/**
 * @brief Get Ethical Guidelines.
 * @return Pointer to the result.
 * @details Calls: createEmbeddedLLM().
 */
EthicalGuidelinesManager* EmbeddedLLM::getEthicalGuidelines() {
    auto impl = createEmbeddedLLM();
    return impl ? impl->getEthicalGuidelines() : nullptr;
}

json EmbeddedLLM::getStats() const {
    auto impl = createEmbeddedLLM();
    return impl ? impl->getStats() : json::object();
}

/**
 * @brief Set Generate Full Fn.
 * @param[in] fn Input parameter.
 * @details Implements setGenerateFullFn without additional internal calls.
 */
void EmbeddedLLM::setGenerateFullFn(GenerateFullFn fn) {
    // No-op in API shim; forwarders may ignore overrides.
    (void)fn;
}

/**
 * @brief Set Embed Fn.
 * @param[in] fn Input parameter.
 * @details Implements setEmbedFn without additional internal calls.
 */
void EmbeddedLLM::setEmbedFn(EmbedFn fn) {
    (void)fn;
}

/**
 * @brief Clear Cache.
 * @details Calls: createEmbeddedLLM().
 */
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
