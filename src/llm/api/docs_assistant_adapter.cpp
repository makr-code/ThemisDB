#include "themis/llm/llm_factory.h"
#include "llm/docs_assistant.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

// Define the PImpl type declared in the header so we can implement
// the public methods without exposing internal details.
struct DocsAssistant::Impl {
    Impl() { impl = createDocsAssistant(); }
    std::shared_ptr<IDocsAssistant> impl;
    /**
     * @brief Load Database.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     * @details Implements loadDatabase without additional internal calls.
     */
    bool loadDatabase(const std::string& path) { return impl ? impl->loadDatabase(path) : false; }
    bool isReady() const { return impl ? impl->isReady() : false; }
    /**
     * @brief Query Result.
     * @param[in] q Input parameter.
     * @return Return value.
     * @details Calls: query().
     */
    DocsQueryResult queryResult(const std::string& q) {
        DocsQueryResult res = {};
        if (!impl) {
          return res;
        }
        res.generated_answer = impl->query(q);
        res.total_docs_searched = 0;
        return res;
    }
    /**
     * @brief Search Docs.
     * @param[in] q Input parameter.
     * @param[in] max_results Input parameter.
     * @return Return value.
     * @details Implements searchDocs without additional internal calls.
     */
    std::vector<DocumentEntry> searchDocs(const std::string& q, int max_results) {
        // The lightweight IDocsAssistant interface currently exposes no raw document search endpoint.
        (void)q;
        (void)max_results;
        return {};
    }
    /**
     * @brief Clear Cache.
     * @details Implements clearCache without additional internal calls.
     */
    void clearCache() { if (impl) impl->clearCache(); }
    /**
     * @brief Get Config Help.
     * @param[in] topic Input parameter.
     * @return Return value.
     * @details Implements getConfigHelp without additional internal calls.
     */
    DocsQueryResult getConfigHelp(const std::string& topic) {
        DocsQueryResult res = {};
        if (!impl) {
          return res;
        }
        return impl->getConfigHelp(topic);
    }
    /**
     * @brief Get Troubleshooting Help.
     * @param[in] topic Input parameter.
     * @return Return value.
     * @details Implements getTroubleshootingHelp without additional internal calls.
     */
    DocsQueryResult getTroubleshootingHelp(const std::string& topic) {
        DocsQueryResult res = {};
        if (!impl) {
          return res;
        }
        return impl->getTroubleshootingHelp(topic);
    }
    nlohmann::json getStats() const { return impl ? impl->getStats() : nlohmann::json::object(); }
};

DocsAssistant::DocsAssistant(const DocsAssistantConfig& config) : impl_(std::make_unique<Impl>()) {
    (void)config; // adapter ignores config; real implementation may use it
}
DocsAssistant::~DocsAssistant() = default;

/**
 * @brief Load Database.
 * @param[in] path Input parameter.
 * @return True when the operation succeeds.
 * @details Implements loadDatabase without additional internal calls.
 */
bool DocsAssistant::loadDatabase(const std::string& path) { return impl_->loadDatabase(path); }
bool DocsAssistant::isReady() const { return impl_->isReady(); }
/**
 * @brief Query.
 * @param[in] query Input parameter.
 * @return Return value.
 * @details Calls: queryResult().
 */
DocsQueryResult DocsAssistant::query(const std::string& query) { return impl_->queryResult(query); }
/**
 * @brief Search Docs.
 * @param[in] q Input parameter.
 * @param[in] max_results Input parameter.
 * @return Return value.
 * @details Implements searchDocs without additional internal calls.
 */
std::vector<DocumentEntry> DocsAssistant::searchDocs(const std::string& q, int max_results) { return impl_->searchDocs(q, max_results); }
/**
 * @brief Clear Cache.
 * @details Implements clearCache without additional internal calls.
 */
void DocsAssistant::clearCache() { impl_->clearCache(); }

/**
 * @brief Get Config Help.
 * @param[in] topic Input parameter.
 * @return Return value.
 * @details Implements getConfigHelp without additional internal calls.
 */
DocsQueryResult DocsAssistant::getConfigHelp(const std::string& topic) { return impl_->getConfigHelp(topic); }
/**
 * @brief Get Troubleshooting Help.
 * @param[in] topic Input parameter.
 * @return Return value.
 * @details Implements getTroubleshootingHelp without additional internal calls.
 */
DocsQueryResult DocsAssistant::getTroubleshootingHelp(const std::string& topic) { return impl_->getTroubleshootingHelp(topic); }
nlohmann::json DocsAssistant::getStats() const { return impl_->getStats(); }

} // namespace llm
} // namespace themis
