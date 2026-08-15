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
    bool loadDatabase(const std::string& path) { return impl ? impl->loadDatabase(path) : false; }
    bool isReady() const { return impl ? impl->isReady() : false; }
    DocsQueryResult queryResult(const std::string& q) {
        DocsQueryResult res;
        if (!impl) return res;
        res.generated_answer = impl->query(q);
        res.total_docs_searched = 0;
        return res;
    }
    std::vector<DocumentEntry> searchDocs(const std::string& q, int max_results) { return {}; }
    void clearCache() { if (impl) impl->clearCache(); }
    DocsQueryResult getConfigHelp(const std::string& topic) {
        DocsQueryResult res;
        if (!impl) return res;
        return impl->getConfigHelp(topic);
    }
    DocsQueryResult getTroubleshootingHelp(const std::string& topic) {
        DocsQueryResult res;
        if (!impl) return res;
        return impl->getTroubleshootingHelp(topic);
    }
    nlohmann::json getStats() const { return impl ? impl->getStats() : nlohmann::json::object(); }
};

DocsAssistant::DocsAssistant(const DocsAssistantConfig& config) : impl_(std::make_unique<Impl>()) {
    (void)config; // adapter ignores config; real implementation may use it
}
DocsAssistant::~DocsAssistant() = default;

bool DocsAssistant::loadDatabase(const std::string& path) { return impl_->loadDatabase(path); }
bool DocsAssistant::isReady() const { return impl_->isReady(); }
DocsQueryResult DocsAssistant::query(const std::string& query) { return impl_->queryResult(query); }
std::vector<DocumentEntry> DocsAssistant::searchDocs(const std::string& q, int max_results) { return impl_->searchDocs(q, max_results); }
void DocsAssistant::clearCache() { impl_->clearCache(); }

DocsQueryResult DocsAssistant::getConfigHelp(const std::string& topic) { return impl_->getConfigHelp(topic); }
DocsQueryResult DocsAssistant::getTroubleshootingHelp(const std::string& topic) { return impl_->getTroubleshootingHelp(topic); }
nlohmann::json DocsAssistant::getStats() const { return impl_->getStats(); }

} // namespace llm
} // namespace themis
