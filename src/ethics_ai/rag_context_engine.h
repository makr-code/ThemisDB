/**
 * @file rag_context_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include "argument_store.h"
#include <memory>
#include <mutex>

namespace themis {
namespace plugins {
namespace ethics {

class RAGContextEngine {
public:
    /**
     * @brief RAGContext Engine.
     * @param[in] store Input parameter.
     * @return Return value.
     */
    explicit RAGContextEngine(std::shared_ptr<ArgumentStore> store);
    ~RAGContextEngine() = default;
    
    std::variant<RAGContext, Status> buildContext(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category
    );
    
    std::variant<std::vector<std::string>, Status> findSimilarDilemmas(
        const std::string& query_text,
        double threshold,
        size_t limit
    );
    
    std::variant<std::vector<std::string>, Status> getBestPractices(
        const std::string& category,
        double min_satisfaction,
        size_t limit
    );
    
    std::variant<std::vector<std::pair<std::string, double>>, Status> 
    vectorSemanticSearch(
        const std::vector<float>& query_embedding,
        const std::string& philosophy_school,
        size_t limit
    );
    
    std::variant<std::vector<std::string>, Status> traverseArgumentChain(
        const std::string& start_argument_id,
        size_t max_depth,
        const std::string& direction
    );

    [[nodiscard]] LegalGrounding retrieveLegalGrounding(
        const std::string& dilemma_description) const;

    /**
     * @brief Set Legal Db Available.
     * @param[in] available Input parameter.
     * @note Exception safety: noexcept.
     */
    void setLegalDbAvailable(bool available) noexcept;
    
private:
    std::shared_ptr<ArgumentStore> store_;
    bool legal_db_available_{true};
    
    mutable std::mutex store_access_mutex_;
    
    // Helper methods
    /**
     * @brief Calculate Text Similarity.
     * @param[in] text1 Input parameter.
     * @param[in] text2 Input parameter.
     * @return Return value.
     */
    double calculateTextSimilarity(const std::string& text1, const std::string& text2);
    /**
     * @brief Generate Embedding.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<float> generateEmbedding(const std::string& text);
};

} // namespace ethics
} // namespace plugins
} // namespace themis
