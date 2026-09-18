/**
 * @file aql_conversation_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.39
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "aql/llm_aql_handler.h"
#include "aql/llm_token_estimator.h"
#include "aql/i_history_compressor.h"
#include <string>
#include <vector>
#include <memory>
#include <cstddef>
#include <functional>

namespace themis {
namespace aql {

// Forward declarations
struct IHistoryCompressor;

class AQLConversationContext {
public:
    struct Config {
        std::size_t max_turns = 50;

        std::size_t max_history_tokens = 8192;

        std::function<std::string(const std::vector<std::pair<std::string, std::string>>&)>
            llm_executor = nullptr;

        bool enable_episodic_compaction = false;

        int32_t episodic_compaction_trigger_tokens = 0;

        float episodic_compression_gate_similarity = 0.85f;
    };

    /**
     * @brief AQLConversation Context.
     * @param[in,out] handler Input/output parameter.
     * @return Return value.
     */
    explicit AQLConversationContext(LLMAQLHandler& handler);
    
    explicit AQLConversationContext(
      LLMAQLHandler& handler,
      Config config,
      std::unique_ptr<TokenEstimator> estimator = nullptr
    );

    /**
     * @brief AQLConversation Context.
     * @param[in,out] handler Input/output parameter.
     * @param[in] config Input parameter.
     * @param[in] estimator Input parameter.
     * @param[in,out] compressor Input/output parameter.
     * @return Return value.
     */
    explicit AQLConversationContext(
      LLMAQLHandler& handler,
      Config config,
      std::unique_ptr<TokenEstimator> estimator,
      IHistoryCompressor* compressor
    );
    ~AQLConversationContext();

    // Non-copyable, movable
    AQLConversationContext(const AQLConversationContext&) = delete;
    AQLConversationContext& operator=(const AQLConversationContext&) = delete;
    AQLConversationContext(AQLConversationContext&&) noexcept;
    AQLConversationContext& operator=(AQLConversationContext&&) noexcept;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Set Schema Context.
     * @param[in] schema Input parameter.
     */
    void setSchemaContext(const std::string& schema);

    /**
     * @brief Get Schema Context.
     * @return Return value.
     */
    std::string getSchemaContext() const;

    /**
     * @brief Set Compressor.
     * @param[in,out] compressor Input/output parameter.
     */
    void setCompressor(IHistoryCompressor* compressor);

    /**
     * @brief Get Compressor.
     * @return Pointer to the result.
     */
    IHistoryCompressor* getCompressor() const;

    // =========================================================================
    // Conversation
    // =========================================================================

    /**
     * @brief Start.
     * @param[in] intent Input parameter.
     * @return Return value.
     */
    std::string start(const std::string& intent);

    /**
     * @brief Refine.
     * @param[in] instruction Input parameter.
     * @return Return value.
     */
    std::string refine(const std::string& instruction);

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

    // =========================================================================
    // Inspection
    // =========================================================================

    /**
     * @brief Turn Count.
     * @return Return value.
     */
    std::size_t turnCount() const;

    /**
     * @brief Token Count.
     * @return Return value.
     */
    std::size_t tokenCount() const;

    /**
     * @brief Last Query.
     * @return Return value.
     */
    std::string lastQuery() const;

    std::vector<std::pair<std::string, std::string>> getHistory() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
