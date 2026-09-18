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
    virtual ~IDocsAssistant() = default;
    virtual bool loadDatabase(const std::string& path) = 0;
    virtual bool isReady() const = 0;
    virtual std::string query(const std::string& q) = 0;
    virtual void clearCache() = 0;
    virtual themis::llm::DocsQueryResult getConfigHelp(const std::string& topic) = 0;
    virtual themis::llm::DocsQueryResult getTroubleshootingHelp(const std::string& topic) = 0;
    virtual nlohmann::json getStats() const = 0;
};

class IEmbeddedLLM {
public:
    virtual ~IEmbeddedLLM() = default;
    virtual bool isReady() const = 0;
    virtual std::string generate(const std::string& prompt) = 0;
    virtual void generateStreaming(const std::string& prompt, const std::function<void(const std::string&)>& token_cb) = 0;
    virtual std::vector<float> embed(const std::string& text) = 0;
    virtual std::vector<std::vector<float>> embedBatch(const std::vector<std::string>& texts) = 0;
    virtual std::string generateWithParams(const std::string& prompt, float temperature, float top_p, int max_tokens) = 0;
    virtual std::string chat(const std::vector<ChatMessage>& messages, ChatFormat format) = 0;
    virtual nlohmann::json generateAsMCP(const std::string& prompt, int max_tokens) = 0;
    virtual nlohmann::json generateAsJsonMarkdown(const std::string& prompt, int max_tokens) = 0;
    virtual InferenceResponse generateFull(const InferenceRequest& request) = 0;
    virtual bool hasEthicalGuidelines() const = 0;
    virtual EthicalGuidelinesManager* getEthicalGuidelines() = 0;
    virtual nlohmann::json getStats() const = 0;
    virtual void clearCache() = 0;
    virtual std::string getModelInfo() const = 0;
};

class IThemisHelpLoRA {
public:
    virtual ~IThemisHelpLoRA() = default;
    virtual bool isTrained() const = 0;
    virtual std::string query(const std::string& prompt) = 0;
    virtual applications::PerformanceMetrics getMetrics() const = 0;
    virtual applications::FeedbackStats getFeedbackStats() const = 0;
    virtual std::string getVersion() const = 0;
};

class ILlamaWrapper {
public:
    virtual ~ILlamaWrapper() = default;
    virtual bool loadModel(const std::string& path) = 0;
    virtual bool unloadModel() = 0;
    virtual std::string formatAsMCPResponse(const std::string& raw) = 0;
};

class ILLMModelAuditLogger {
public:
    virtual ~ILLMModelAuditLogger() = default;
    virtual void logRequest(const std::string& req) = 0;
};

} // namespace llm
} // namespace themis
