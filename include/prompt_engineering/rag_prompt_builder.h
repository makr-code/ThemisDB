/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rag_prompt_builder.h                               ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 07:08:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     209                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 01a86c4f10  2026-04-07  Changes before error encountered        ║
    • d135ff3ad9  2026-03-09  feat(prompt_engineering): implement ChainOfThoughtBuilder... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file rag_prompt_builder.h
 * @brief RAG (Retrieval-Augmented Generation) prompt construction helpers.
 *
 * Assembles LLM prompts that incorporate retrieved document chunks as
 * grounding context.  Key capabilities:
 *
 *  - Rank-aware chunk selection — greedily fills the context budget in
 *    descending relevance-score order.
 *  - Flexible template injection — replaces a configurable placeholder
 *    token in a base template string with the assembled context block.
 *  - Source citation — optionally prefixes every chunk with its source ID.
 *  - Full prompt assembly — combines system instruction, context, and query.
 *
 * No LLM inference or network I/O is performed; this is a pure
 * string-assembly utility.
 */

#pragma once

#include <string>
#include <vector>

namespace themis {
namespace prompt_engineering {

/**
 * @brief A single retrieved document chunk with metadata.
 */
struct RetrievedChunk {
    std::string content;            ///< Chunk text content
    std::string source;             ///< Source document identifier (file, URL, …)
    double      relevance_score = 1.0; ///< Relevance score in [0, 1]
    std::string chunk_id;           ///< Unique chunk identifier (optional)
};

/**
 * @brief Configuration for RAGPromptBuilder.
 */
struct RAGPromptConfig {
    /// Maximum total character length for the assembled context section.
    /// For token-precise control prefer RAGContextAssembler, which derives
    /// this value automatically from the model's context window.
    size_t max_context_length = 4000;

    /// Header prepended to the context section.
    std::string context_header = "Retrieved Context:";

    /// Footer appended after the last context chunk (empty by default).
    std::string context_footer;

    /// Separator inserted between consecutive chunks.
    std::string chunk_separator = "\n\n---\n\n";

    /// Whether to prefix each chunk with "[Source: <source>]\n".
    bool include_source_citations = true;

    /// Whether to sort candidates by relevance_score descending before
    /// selecting.  Set to false to preserve the caller-supplied order.
    bool rank_by_relevance = true;

    /// Placeholder token inside base templates that will be replaced with
    /// the assembled context block.
    std::string template_placeholder = "{context}";

    /// Minimum tokens reserved for the model's answer when this config is
    /// used in conjunction with RAGContextAssembler.  The assembler converts
    /// the token budget to characters for max_context_length.
    size_t reserved_response_tokens = 512;
};

/**
 * @brief Builds RAG prompts by injecting retrieved context into templates.
 *
 * Usage – template injection:
 * @code
 * RAGPromptBuilder builder;
 * std::string prompt = builder.build(
 *     "Answer based on the following:\n{context}\n\nQuestion: {query}",
 *     user_question,
 *     retrieved_chunks);
 * @endcode
 *
 * Usage – full prompt assembly:
 * @code
 * std::string prompt = builder.buildFullPrompt(
 *     "You are a helpful legal assistant.",
 *     user_question,
 *     retrieved_chunks);
 * @endcode
 */
class RAGPromptBuilder {
public:
    /**
     * @brief Construct a builder with the given configuration.
     * @param config RAG prompt generation settings.
     */
    explicit RAGPromptBuilder(const RAGPromptConfig& config = RAGPromptConfig{});

    /**
     * @brief Build a prompt by injecting context into a template string.
     *
     * Steps performed:
     *  1. Select chunks that fit within `config_.max_context_length`.
     *  2. Assemble the context block (header + chunks + footer).
     *  3. Replace `config_.template_placeholder` in @p base_template with
     *     the context block.
     *  4. Replace `{query}` in the result with @p query (if present).
     *
     * @param base_template  Template string; must contain
     *                       `config_.template_placeholder`.
     * @param query          The user query (substituted for `{query}`).
     * @param chunks         Candidate retrieved chunks.
     * @return The assembled prompt string.
     */
    std::string build(const std::string& base_template,
                      const std::string& query,
                      const std::vector<RetrievedChunk>& chunks) const;

    /**
     * @brief Assemble only the context section string.
     *
     * Produces: `<context_header>\n<chunk1><sep><chunk2>…\n<context_footer>`.
     *
     * @param chunks  Chunks to include (assumed already budget-filtered).
     * @return The context section string.
     */
    std::string buildContextSection(
        const std::vector<RetrievedChunk>& chunks) const;

    /**
     * @brief Build a complete prompt with system instruction, context, and query.
     *
     * The output format is:
     * @code
     * <system_instruction>
     *
     * <context_header>
     * <chunk1>
     * ---
     * <chunk2>
     * …
     *
     * Question: <query>
     * Answer:
     * @endcode
     *
     * @param system_instruction  Role/persona instruction prepended to the prompt.
     * @param query               The user query.
     * @param chunks              Candidate retrieved chunks.
     * @return The fully assembled prompt string.
     */
    std::string buildFullPrompt(const std::string& system_instruction,
                                const std::string& query,
                                const std::vector<RetrievedChunk>& chunks) const;

    /**
     * @brief Select chunks that fit within a total character budget.
     *
     * Iterates chunks in their supplied order (or relevance order if
     * `config_.rank_by_relevance` is true) and greedily includes each chunk
     * until @p max_total_length would be exceeded.
     *
     * @param candidates       All candidate chunks.
     * @param max_total_length Character budget for the context section.
     * @return The subset of chunks that fits.
     */
    std::vector<RetrievedChunk> selectChunks(
        const std::vector<RetrievedChunk>& candidates,
        size_t max_total_length) const;

    /** @brief Return a read-only reference to the current configuration. */
    const RAGPromptConfig& getConfig() const;

    /** @brief Replace the current configuration. */
    void setConfig(const RAGPromptConfig& config);

private:
    RAGPromptConfig config_;

    /// Format a single chunk for inclusion in the context section.
    std::string formatChunk(const RetrievedChunk& chunk, size_t index) const;
};

} // namespace prompt_engineering
} // namespace themis
