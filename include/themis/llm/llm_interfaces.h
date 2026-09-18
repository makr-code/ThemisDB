// Minimal LLM interface declarations used to break link-time dependence
// between query/sharding and the full LLM implementation.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

// Forward declarations for types defined in other LLM headers so this
// lightweight API header does not pull heavy implementation headers.
struct DocsQueryResult;
struct DocumentEntry;
namespace applications { struct PerformanceMetrics; struct FeedbackStats; }
struct ChatMessage;
enum class ChatFormat : int;
struct InferenceRequest;
struct InferenceResponse;
class EthicalGuidelinesManager;


class IDocsAssistant {
public:
    /**
     * @brief TBD: Describe ~IDocsAssistant.
     * @return Return value.
     */
    virtual ~IDocsAssistant() = default;
    /**
     * @brief TBD: Describe loadDatabase.
     * @param[in] path Input parameter.
     * @return True on success.
     */
    virtual bool loadDatabase(const std::string& path) = 0;
    /**
     * @brief TBD: Describe isReady.
     * @return True on success.
     */
    virtual bool isReady() const = 0;
    /**
     * @brief TBD: Describe query.
     * @param[in] q Input parameter.
     * @return Return value.
     */
    virtual std::string query(const std::string& q) = 0;
    /**
     * @brief TBD: Describe clearCache.
     */
    virtual void clearCache() = 0;
    /**
     * @brief TBD: Describe getConfigHelp.
     * @param[in] topic Input parameter.
     * @return Return value.
     */
    virtual themis::llm::DocsQueryResult getConfigHelp(const std::string& topic) = 0;
    /**
     * @brief TBD: Describe getTroubleshootingHelp.
     * @param[in] topic Input parameter.
     * @return Return value.
     */
    virtual themis::llm::DocsQueryResult getTroubleshootingHelp(const std::string& topic) = 0;
    /**
     * @brief TBD: Describe getStats.
     * @return Return value.
     */
    virtual nlohmann::json getStats() const = 0;
};

class IEmbeddedLLM {
public:
    /**
     * @brief TBD: Describe ~IEmbeddedLLM.
     * @return Return value.
     */
    virtual ~IEmbeddedLLM() = default;
    /**
     * @brief TBD: Describe isReady.
     * @return True on success.
     */
    virtual bool isReady() const = 0;
    /**
     * @brief TBD: Describe generate.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    virtual std::string generate(const std::string& prompt) = 0;
    virtual void generateStreaming(const std::string& prompt, const std::function<void(const std::string&)>& token_cb) = 0;
    /**
     * @brief TBD: Describe embed.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    virtual std::vector<float> embed(const std::string& text) = 0;
    /**
     * @brief TBD: Describe embedBatch.
     * @param[in] texts Input parameter.
     * @return Return value.
     */
    virtual std::vector<std::vector<float>> embedBatch(const std::vector<std::string>& texts) = 0;
    /**
     * @brief TBD: Describe generateWithParams.
     * @param[in] prompt Input parameter.
     * @param[in] temperature Input parameter.
     * @param[in] top_p Input parameter.
     * @param[in] max_tokens Input parameter.
     * @return Return value.
     */
    virtual std::string generateWithParams(const std::string& prompt, float temperature, float top_p, int max_tokens) = 0;
    /**
     * @brief TBD: Describe chat.
     * @param[in] messages Input parameter.
     * @param[in] format Input parameter.
     * @return Return value.
     */
    virtual std::string chat(const std::vector<ChatMessage>& messages, ChatFormat format) = 0;
    /**
     * @brief TBD: Describe generateAsMCP.
     * @param[in] prompt Input parameter.
     * @param[in] max_tokens Input parameter.
     * @return Return value.
     */
    virtual nlohmann::json generateAsMCP(const std::string& prompt, int max_tokens) = 0;
    /**
     * @brief TBD: Describe generateAsJsonMarkdown.
     * @param[in] prompt Input parameter.
     * @param[in] max_tokens Input parameter.
     * @return Return value.
     */
    virtual nlohmann::json generateAsJsonMarkdown(const std::string& prompt, int max_tokens) = 0;
    /**
     * @brief TBD: Describe generateFull.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    virtual InferenceResponse generateFull(const InferenceRequest& request) = 0;
    /**
     * @brief TBD: Describe hasEthicalGuidelines.
     * @return True on success.
     */
    virtual bool hasEthicalGuidelines() const = 0;
    /**
     * @brief TBD: Describe getEthicalGuidelines.
     * @return Pointer to the result.
     */
    virtual EthicalGuidelinesManager* getEthicalGuidelines() = 0;
    /**
     * @brief TBD: Describe getStats.
     * @return Return value.
     */
    virtual nlohmann::json getStats() const = 0;
    /**
     * @brief TBD: Describe clearCache.
     */
    virtual void clearCache() = 0;
    /**
     * @brief TBD: Describe getModelInfo.
     * @return Return value.
     */
    virtual std::string getModelInfo() const = 0;
};

class IThemisHelpLoRA {
public:
    /**
     * @brief TBD: Describe ~IThemisHelpLoRA.
     * @return Return value.
     */
    virtual ~IThemisHelpLoRA() = default;
    /**
     * @brief TBD: Describe isTrained.
     * @return True on success.
     */
    virtual bool isTrained() const = 0;
    /**
     * @brief TBD: Describe query.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    virtual std::string query(const std::string& prompt) = 0;
    /**
     * @brief TBD: Describe getMetrics.
     * @return Return value.
     */
    virtual applications::PerformanceMetrics getMetrics() const = 0;
    /**
     * @brief TBD: Describe getFeedbackStats.
     * @return Return value.
     */
    virtual applications::FeedbackStats getFeedbackStats() const = 0;
    /**
     * @brief TBD: Describe getVersion.
     * @return Return value.
     */
    virtual std::string getVersion() const = 0;
};

class ILlamaWrapper {
public:
    /**
     * @brief TBD: Describe ~ILlamaWrapper.
     * @return Return value.
     */
    virtual ~ILlamaWrapper() = default;
    /**
     * @brief TBD: Describe loadModel.
     * @param[in] path Input parameter.
     * @return True on success.
     */
    virtual bool loadModel(const std::string& path) = 0;
    /**
     * @brief TBD: Describe unloadModel.
     * @return True on success.
     */
    virtual bool unloadModel() = 0;
    /**
     * @brief TBD: Describe formatAsMCPResponse.
     * @param[in] raw Input parameter.
     * @return Return value.
     */
    virtual std::string formatAsMCPResponse(const std::string& raw) = 0;
};

class ILLMModelAuditLogger {
public:
    /**
     * @brief TBD: Describe ~ILLMModelAuditLogger.
     * @return Return value.
     */
    virtual ~ILLMModelAuditLogger() = default;
    /**
     * @brief TBD: Describe logRequest.
     * @param[in] req Input parameter.
     */
    virtual void logRequest(const std::string& req) = 0;
};

} // namespace llm
} // namespace themis
