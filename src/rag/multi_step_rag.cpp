/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_step_rag.cpp                                 ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:50:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     384                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 01a86c4f10  2026-04-07  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file multi_step_rag.cpp
 * @brief Multi-step RAG orchestration — Map-Reduce and Iterative strategies.
 *
 * See include/rag/multi_step_rag.h for the public interface.
 */

#include "rag/multi_step_rag.h"
#include "llm/context_window_budget.h"

#include <algorithm>
#include <future>
#include <sstream>

namespace themis::rag {

using ::themis::llm::estimateTokens;

// ---------------------------------------------------------------------------
// Constructor / configuration
// ---------------------------------------------------------------------------

MultiStepRAGOrchestrator::MultiStepRAGOrchestrator(
    const MultiStepRAGConfig& cfg)
    : config_(cfg)
    , assembler_(cfg.assembler)
{}

const MultiStepRAGConfig& MultiStepRAGOrchestrator::getConfig() const
{
    return config_;
}

void MultiStepRAGOrchestrator::setConfig(const MultiStepRAGConfig& cfg)
{
    config_   = cfg;
    assembler_.setConfig(cfg.assembler);
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

std::string MultiStepRAGOrchestrator::substitute(
    const std::string& tmpl,
    const std::string& key,
    const std::string& value)
{
    std::string result = tmpl;
    const std::string placeholder = "{" + key + "}";
    size_t pos = 0;
    while ((pos = result.find(placeholder, pos)) != std::string::npos) {
        result.replace(pos, placeholder.size(), value);
        pos += value.size();
    }
    return result;
}

std::vector<std::string> MultiStepRAGOrchestrator::parseOpenAspects(
    const std::string& llm_response)
{
    std::vector<std::string> aspects;
    if (llm_response.empty()) return aspects;

    // A response of "NONE" (case-insensitive) means the answer is complete.
    std::string upper = llm_response;
    for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (upper.find("NONE") != std::string::npos && llm_response.size() < 20u) {
        return aspects;
    }

    // Split on newlines; discard empty lines.
    std::istringstream ss(llm_response);
    std::string line;
    while (std::getline(ss, line)) {
        // Trim leading/trailing whitespace.
        const auto first = line.find_first_not_of(" \t\r");
        const auto last  = line.find_last_not_of(" \t\r");
        if (first != std::string::npos) {
            aspects.push_back(line.substr(first, last - first + 1));
        }
    }
    return aspects;
}

std::string MultiStepRAGOrchestrator::buildMapPrompt(
    const std::vector<RetrievedChunk>& chunks,
    const std::string&                 query) const
{
    // Build context block: "Source: …\n<content>"
    std::ostringstream ctx;
    for (size_t i = 0; i < chunks.size(); ++i) {
        if (i > 0) ctx << "\n\n---\n\n";
        if (!chunks[i].source.empty()) {
            ctx << "[Source: " << chunks[i].source << "]\n";
        }
        ctx << chunks[i].content;
    }

    std::string prompt = config_.map_prompt_template;
    prompt = substitute(prompt, "context", ctx.str());
    prompt = substitute(prompt, "query",   query);

    if (!config_.system_prompt.empty()) {
        prompt = config_.system_prompt + "\n\n" + prompt;
    }
    return prompt;
}

std::string MultiStepRAGOrchestrator::buildReducePrompt(
    const std::vector<std::string>& partial_answers,
    const std::string&              query) const
{
    std::ostringstream pa;
    for (size_t i = 0; i < partial_answers.size(); ++i) {
        pa << "--- Answer " << (i + 1) << " ---\n" << partial_answers[i] << "\n";
    }

    std::string prompt = config_.reduce_prompt_template;
    prompt = substitute(prompt, "query",           query);
    prompt = substitute(prompt, "partial_answers", pa.str());

    if (!config_.system_prompt.empty()) {
        prompt = config_.system_prompt + "\n\n" + prompt;
    }
    return prompt;
}

// ---------------------------------------------------------------------------
// Batch partitioning
// ---------------------------------------------------------------------------

std::vector<std::vector<RetrievedChunk>>
MultiStepRAGOrchestrator::partitionIntoBatches(
    const std::vector<RetrievedChunk>& documents,
    const std::string&                 query) const
{
    using ::themis::llm::ContextWindowBudget;

    const ContextWindowBudget budget = ContextWindowBudget::compute(
        config_.assembler.model_context_tokens,
        config_.system_prompt,
        query,
        config_.assembler.min_response_tokens);

    const size_t batch_token_limit = budget.available_context_tokens;

    std::vector<std::vector<RetrievedChunk>> batches;
    std::vector<RetrievedChunk>              current_batch;
    size_t                                   current_tokens = 0u;

    for (const auto& doc : documents) {
        const size_t doc_tokens = estimateTokens(doc.content);

        if (!current_batch.empty() &&
            current_tokens + doc_tokens > batch_token_limit)
        {
            // Current batch is full — flush it.
            batches.push_back(std::move(current_batch));
            current_batch.clear();
            current_tokens = 0u;

            if (batches.size() >= config_.max_map_steps) break;
        }

        current_batch.push_back(doc);
        current_tokens += doc_tokens;
    }

    if (!current_batch.empty()) {
        batches.push_back(std::move(current_batch));
    }

    return batches;
}

// ---------------------------------------------------------------------------
// Strategy A: Map-Reduce
// ---------------------------------------------------------------------------

MultiStepRAGResult MultiStepRAGOrchestrator::runMapReduce(
    const std::string&                 query,
    const std::vector<RetrievedChunk>& documents,
    const InferenceFn&                 infer) const
{
    MultiStepRAGResult result;

    if (documents.empty() || !infer) {
        result.final_answer = "";
        return result;
    }

    // Try single-pass first.
    AssembledContext single = assembler_.assemble(
        documents, config_.system_prompt, query);

    if (single.chunks_used.size() == documents.size() && !single.was_truncated) {
        // Everything fits — no need for map-reduce.
        const std::string prompt = buildMapPrompt(single.chunks_used, query);
        const int max_tok = RAGContextAssembler::computeMaxTokens(
            ::themis::llm::ContextWindowBudget::compute(
                config_.assembler.model_context_tokens,
                config_.system_prompt, query,
                config_.assembler.min_response_tokens),
            config_.max_response_tokens);

        result.final_answer   = infer(prompt, max_tok);
        result.steps_executed = 1u;
        result.was_truncated  = false;
        result.context_overflow = false;
        result.steps.push_back(result.final_answer);
        return result;
    }

    // Multi-batch map phase.
    result.context_overflow = true;
    result.was_truncated    = single.was_truncated;

    const auto batches = partitionIntoBatches(documents, query);
    const int  map_max_tok = config_.max_response_tokens;

    if (config_.enable_parallel_map && batches.size() > 1u) {
        // F-029: Launch all map steps in parallel.
        // Requires the InferenceFn to be thread-safe (documented in the config).
        std::vector<std::future<std::string>> futures;
        futures.reserve(batches.size());
        for (size_t bi = 0; bi < batches.size(); ++bi) {
            futures.push_back(std::async(std::launch::async,
                [this, &batches, &query, &infer, map_max_tok, bi]() -> std::string {
                    return infer(buildMapPrompt(batches[bi], query), map_max_tok);
                }));
        }
        result.steps.reserve(futures.size());
        for (auto& f : futures) {
            result.steps.push_back(f.get());
            ++result.steps_executed;
        }
    } else {
        // Sequential map phase (default).
        for (const auto& batch : batches) {
            const std::string map_prompt = buildMapPrompt(batch, query);
            std::string partial          = infer(map_prompt, map_max_tok);
            result.steps.push_back(partial);
            ++result.steps_executed;
        }
    }

    if (result.steps.empty()) {
        result.final_answer = "";
        return result;
    }

    if (result.steps.size() == 1u) {
        result.final_answer = result.steps.front();
        return result;
    }

    // Reduce phase — synthesise partial answers.
    const std::string reduce_prompt =
        buildReducePrompt(result.steps, query);
    result.final_answer  = infer(reduce_prompt, config_.max_response_tokens);
    ++result.steps_executed;

    return result;
}

// ---------------------------------------------------------------------------
// Strategy B: Iterative
// ---------------------------------------------------------------------------

MultiStepRAGResult MultiStepRAGOrchestrator::runIterative(
    const std::string&                 query,
    const std::vector<RetrievedChunk>& documents,
    const InferenceFn&                 infer,
    const RetrievalFn&                 retrieve) const
{
    MultiStepRAGResult result;

    if (!infer) return result;

    // Accumulate all documents across iterations.
    std::vector<RetrievedChunk> accumulated = documents;

    for (size_t iter = 0u; iter < config_.max_iterations; ++iter) {
        // Assemble context within budget.
        AssembledContext ctx = assembler_.assemble(
            accumulated, config_.system_prompt, query);

        if (ctx.was_truncated) result.was_truncated = true;

        const std::string prompt = buildMapPrompt(ctx.chunks_used, query);
        const int max_tok = RAGContextAssembler::computeMaxTokens(
            ::themis::llm::ContextWindowBudget::compute(
                config_.assembler.model_context_tokens,
                config_.system_prompt, query,
                config_.assembler.min_response_tokens),
            config_.max_response_tokens);

        result.final_answer = infer(prompt, max_tok);
        result.steps.push_back(result.final_answer);
        ++result.steps_executed;

        // Check for uncovered aspects unless no retriever is provided.
        if (!retrieve) break;

        std::string gap_prompt = config_.gap_detection_prompt;
        gap_prompt = substitute(gap_prompt, "query",  query);
        gap_prompt = substitute(gap_prompt, "answer", result.final_answer);

        const std::string gap_response = infer(gap_prompt, 256);
        ++result.steps_executed;

        const auto aspects = parseOpenAspects(gap_response);
        if (aspects.empty()) break; // answer is complete

        // Retrieve additional documents for the first uncovered aspect.
        const std::string refined_query = aspects.front();
        const auto new_docs = retrieve(refined_query, config_.retrieval_top_k);

        if (new_docs.empty()) break;

        // Deduplicate by source before accumulating.
        for (const auto& nd : new_docs) {
            const bool already_present = std::any_of(
                accumulated.begin(), accumulated.end(),
                [&](const RetrievedChunk& c) {
                    return !c.source.empty() && c.source == nd.source;
                });
            if (!already_present) accumulated.push_back(nd);
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// MultiStepRAGFactory
// ---------------------------------------------------------------------------

std::unique_ptr<MultiStepRAGOrchestrator>
MultiStepRAGFactory::createSmallContext()
{
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 4096u;
    cfg.assembler.min_response_tokens  = 512u;
    cfg.max_response_tokens            = 512;
    cfg.max_map_steps                  = 3u;
    cfg.max_iterations                 = 3u;
    return std::make_unique<MultiStepRAGOrchestrator>(cfg);
}

std::unique_ptr<MultiStepRAGOrchestrator>
MultiStepRAGFactory::createMediumContext()
{
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 8192u;
    cfg.assembler.min_response_tokens  = 512u;
    cfg.max_response_tokens            = 512;
    cfg.max_map_steps                  = 4u;
    cfg.max_iterations                 = 4u;
    return std::make_unique<MultiStepRAGOrchestrator>(cfg);
}

std::unique_ptr<MultiStepRAGOrchestrator>
MultiStepRAGFactory::createLargeContext()
{
    MultiStepRAGConfig cfg;
    cfg.assembler.model_context_tokens = 32768u;
    cfg.assembler.min_response_tokens  = 1024u;
    cfg.max_response_tokens            = 1024;
    cfg.max_map_steps                  = 8u;
    cfg.max_iterations                 = 5u;
    return std::make_unique<MultiStepRAGOrchestrator>(cfg);
}

std::unique_ptr<MultiStepRAGOrchestrator>
MultiStepRAGFactory::create(const MultiStepRAGConfig& cfg)
{
    return std::make_unique<MultiStepRAGOrchestrator>(cfg);
}

} // namespace themis::rag
