/**
 * @file docs_assistant_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

// Forward declarations
namespace themis {
namespace llm {
    struct DocumentEntry;
}
}


#pragma once

#include "aql/classify_bridge.h"
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace aql {

using json = nlohmann::json;

class DocsAssistantFunctions {
public:
    enum class DegradedReason {
        OK,                  ///< All components loaded successfully
        DATABASE_NOT_FOUND,  ///< Documentation database file could not be located
        DATABASE_LOAD_FAILED, ///< Database found but failed to load
        LORA_LOAD_FAILED,    ///< LoRA adapter initialisation failed
    };
    DocsAssistantFunctions();
    
    ~DocsAssistantFunctions();
    
    std::string help(const std::string& query, const std::string& user_id = "anonymous");
    
    /**
     * @brief Docs Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string docsQuery(const std::string& query);
    
    json docsSearch(const std::string& query, int limit = 5);
    
    /**
     * @brief Docs Config Help.
     * @param[in] topic Input parameter.
     * @return Return value.
     */
    std::string docsConfigHelp(const std::string& topic);
    
    /**
     * @brief Docs Troubleshoot.
     * @param[in] error_description Input parameter.
     * @return Return value.
     */
    std::string docsTroubleshoot(const std::string& error_description);
    
    /**
     * @brief Docs Stats.
     * @return Return value.
     */
    json docsStats();
    
    /**
     * @brief Is Ready.
     * @return True when the operation succeeds.
     */
    bool isReady() const;

    /**
     * @brief Is Fully Ready.
     * @return True when the operation succeeds.
     */
    bool isFullyReady() const;

    /**
     * @brief Degraded Reason.
     * @return Return value.
     */
    std::string degradedReason() const;
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    /**
     * @brief Is Lo RAActive.
     * @return True when the operation succeeds.
     */
    bool isLoRAActive() const;
    
    /**
     * @brief Get Performance Metrics.
     * @return Return value.
     */
    json getPerformanceMetrics() const;

    /**
     * @brief Set Classifier.
     * @param[in,out] classifier Input/output parameter.
     */
    void setClassifier(IClassifyFn* classifier);

protected:
    /**
     * @brief Detect Intent With Native NLP.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string detectIntentWithNativeNLP(const std::string& query);

private:
    class Impl;
  /**
   * @brief Ensure Impl.
   * @return Return value.
   */
  Impl& ensureImpl();
  /**
   * @brief Try Get Impl.
   * @return Pointer to the result.
   */
  Impl* tryGetImpl() const;
  mutable std::unique_ptr<Impl> impl_;

    IClassifyFn* classifier_ = nullptr;

    /**
     * @brief Detect Intent With LLM.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string detectIntentWithLLM(const std::string& query);
    
    /**
     * @brief Detect Intent With Regex.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string detectIntentWithRegex(const std::string& query);
    
    /**
     * @brief Extract Topic From Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string extractTopicFromQuery(const std::string& query);
    
    /**
     * @brief Extract Search Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string extractSearchQuery(const std::string& query);
    
    /**
     * @brief Format Search Results.
     * @param[in] docs Input parameter.
     * @return Return value.
     */
    std::string formatSearchResults(const std::vector<llm::DocumentEntry>& docs);
};

/**
 * @brief Get Docs Assistant Functions.
 * @return Return value.
 * @note Exception safety: noexcept.
 */
DocsAssistantFunctions& getDocsAssistantFunctions() noexcept;

} // namespace aql
} // namespace themis
