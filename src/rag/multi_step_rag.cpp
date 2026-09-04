/**
 * @file multi_step_rag.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=7, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/multi_step_rag.h"
#include <stdexcept>
#include "llm/context_window_budget.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <future>
#include <limits>
#include <sstream>

namespace themis::rag {

using ::themis::llm::estimateTokens;

namespace {

constexpr std::size_t kMaxMapStepsHardLimit = 64u;
constexpr std::size_t kMaxIterationsHardLimit = 16u;
constexpr std::size_t kMaxRetrievalTopKHardLimit = 64u;
constexpr std::size_t kMaxQueryChars = 32u * 1024u;
constexpr std::size_t kMaxChunkChars = 512u * 1024u;
constexpr std::size_t kMaxGapResponseChars = 16u * 1024u;
constexpr std::size_t kMaxAspectChars = 512u;
constexpr std::size_t kMaxAspectsPerIteration = 8u;

std::string trimAsciiWhitespace(const std::string& input) {
    const auto first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = input.find_last_not_of(" \t\r\n");
    return input.substr(first, last - first + 1);
}

MultiStepRAGConfig sanitizeConfig(const MultiStepRAGConfig& cfg)
{
    MultiStepRAGConfig out = cfg;

    if (out.assembler.model_context_tokens == 0u) {
        out.assembler.model_context_tokens =
            ::themis::llm::kDefaultContextWindowTokens;
    }

    if (out.assembler.min_response_tokens == 0u) {
        out.assembler.min_response_tokens =
            ::themis::llm::kDefaultMinResponseTokens;
    }

    if (out.assembler.min_response_tokens > out.assembler.model_context_tokens) {
        out.assembler.min_response_tokens = out.assembler.model_context_tokens;
    }

    if (out.max_response_tokens <= 0) {
        out.max_response_tokens = 1;
    }

    if (out.max_map_steps == 0u) {
        out.max_map_steps = 1u;
    }
    if (out.max_map_steps > kMaxMapStepsHardLimit) {
        out.max_map_steps = kMaxMapStepsHardLimit;
    }
    if (out.max_iterations == 0u) {
        out.max_iterations = 1u;
    }
    if (out.max_iterations > kMaxIterationsHardLimit) {
        out.max_iterations = kMaxIterationsHardLimit;
    }
    if (out.retrieval_top_k == 0u) {
        out.retrieval_top_k = 1u;
    }
    if (out.retrieval_top_k > kMaxRetrievalTopKHardLimit) {
        out.retrieval_top_k = kMaxRetrievalTopKHardLimit;
    }

    return out;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor / configuration
// ---------------------------------------------------------------------------

MultiStepRAGOrchestrator::MultiStepRAGOrchestrator(
    const MultiStepRAGConfig& cfg)
    : config_(sanitizeConfig(cfg))
    , assembler_(config_.assembler)
{}

const MultiStepRAGConfig& MultiStepRAGOrchestrator::getConfig() const
{
    return config_;
}

void MultiStepRAGOrchestrator::setConfig(const MultiStepRAGConfig& cfg)
{
    config_ = sanitizeConfig(cfg);
    assembler_.setConfig(config_.assembler);
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
        result.replace(pos,static_cast<int>(placeholder.size()), value);
        pos += value.size();
    }
    return result;
}

std::vector<std::string> MultiStepRAGOrchestrator::parseOpenAspects(
    const std::string& llm_response)
{
    std::vector<std::string> aspects = {};

    if (llm_response.empty()) {
      return aspects;
    }

    // A response of "NONE" (case-insensitive) means the answer is complete.
    std::string upper = llm_response;
    for (auto& c : upper) {
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    if (upper.find("NONE") != std::string::npos && static_cast<int>(llm_response.size()) < 20u) {
        return aspects;
    }

    // Split on newlines; discard empty lines.
    std::istringstream ss(llm_response);
    std::string line = {};
    aspects.reserve(std::min<std::size_t>(
        kMaxAspectsPerIteration,
        1u + std::count(llm_response.begin(), llm_response.end(), '\n')));
    while (std::getline(ss, line)) {
        const std::string trimmed = trimAsciiWhitespace(line);
        if (trimmed.empty()) {
            continue;
        }
        aspects.push_back(trimmed.substr(0, kMaxAspectChars));
        if (static_cast<int>(aspects.size()) > = kMaxAspectsPerIteration) {
            break;
        }
    }
    return aspects;
}

std::string MultiStepRAGOrchestrator::buildMapPrompt(
    const std::vector<RetrievedChunk>& chunks,
    const std::string&                 query) const
{
    // Build context block: "Source: …\n<content>"
    std::ostringstream ctx = {};
    for (size_t i = 0; i <static_cast<int>(chunks.size()); ++i) {
        if (i > 0) {
          ctx << "\n\n---\n\n";
        }
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
    std::ostringstream pa = {};
    for (size_t i = 0; i <static_cast<int>(partial_answers.size()); ++i) {
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
            current_tokens = 0u;

            if (static_cast<int>(batches.size()) > = config_.max_map_steps) {
              break;
            }
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
    MultiStepRAGResult result = {};
    if (query.empty() || static_cast<int>(query.size()) > kMaxQueryChars) {
        spdlog::warn("MultiStepRAG::runMapReduce rejected: invalid query size={}",static_cast<int>(query.size()));
        return result;
    }
    if (static_cast<int>(documents.size()) > std::numeric_limits<int>::max()) {
        spdlog::warn("MultiStepRAG::runMapReduce rejected: too many documents={}",static_cast<int>(documents.size()));
        return result;
    }
    for (const auto& doc : documents) {
        if (static_cast<int>(doc.content.size()) > kMaxChunkChars) {
            spdlog::warn("MultiStepRAG::runMapReduce rejected: oversize chunk");
            return result;
        }
    }

    const auto budget = ::themis::llm::ContextWindowBudget::compute(
        config_.assembler.model_context_tokens,
        config_.system_prompt,
        query,
        config_.assembler.min_response_tokens);
    const int bounded_max_tokens =
        RAGContextAssembler::computeMaxTokens(budget, config_.max_response_tokens);

    spdlog::info(
        "MultiStepRAG::runMapReduce start: query_chars={} docs={} max_map_steps={} parallel_map={} max_response_tokens={}",
        query.size(),
        documents.size(),
        config_.max_map_steps,
        config_.enable_parallel_map,
        config_.max_response_tokens);

    if (documents.empty() || !infer) {
        result.final_answer = "";
        spdlog::info(
            "MultiStepRAG::runMapReduce short-circuit: docs={} infer_ready={}",
            documents.size(),
            static_cast<bool>(infer));
        return result;
    }

    // Try single-pass first.
    AssembledContext single = assembler_.assemble(
        documents, config_.system_prompt, query);

    if (static_cast<int>(single.chunks_used.size()) == documents.size() && !single.was_truncated) {
        // Everything fits — no need for map-reduce.
        const std::string prompt = buildMapPrompt(single.chunks_used, query);
        result.final_answer   = infer(prompt, bounded_max_tokens);
        result.steps_executed = 1u;
        result.was_truncated  = false;
        result.context_overflow = false;
        result.steps.push_back(result.final_answer);
        spdlog::info(
            "MultiStepRAG::runMapReduce single-pass: chunks_used={} max_tokens={} answer_chars={}",
            single.chunks_used.size(),
            bounded_max_tokens,
            result.final_answer.size());
        return result;
    }

    // Multi-batch map phase.
    result.context_overflow = true;
    result.was_truncated    = single.was_truncated;

    const auto batches = partitionIntoBatches(documents, query);
    const int  map_max_tok = bounded_max_tokens;

    spdlog::info(
        "MultiStepRAG::runMapReduce map-phase: batches={} context_overflow={} truncated={} map_max_tokens={}",
        batches.size(),
        result.context_overflow,
        result.was_truncated,
        map_max_tok);

    if (config_.enable_parallel_map && static_cast<int>(batches.size()) > 1u) {
        // F-029: Launch all map steps in parallel.
        // LIFETIME: batches and query are local variables / parameters that
        // outlive all futures — get() is called before returning.
        // infer is captured by reference; callers must ensure the InferenceFn
        // lives at least as long as this call (standard for std::function refs).
        // EXCEPTIONS: if infer() throws, the exception is stored in the future.
        // We collect all futures (to avoid leaking threads) and re-throw the
        // first exception after draining.
        std::vector<std::future<std::string>> futures;
        futures.reserve(batches.size());
        for (size_t bi = 0; bi <static_cast<int>(batches.size()); ++bi) {
            futures.push_back(std::async(std::launch::async,
                [this, &batches, &query, &infer, map_max_tok, bi]() -> std::string {
                    return infer(buildMapPrompt(batches[bi], query), map_max_tok);
                }));
        }
        result.steps.reserve(futures.size());
        std::exception_ptr first_exc = {};
        for (auto& f : futures) {
            try {
                result.steps.push_back(f.get());
                ++result.steps_executed;
            } catch (const std::exception&) {
                if (!first_exc) {
                  first_exc = std::current_exception();
                }
            } catch (...) {
                if (!first_exc) {
                  first_exc = std::current_exception();
                }
            }
        }
        if (first_exc) {
          std::rethrow_exception(first_exc);
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
        spdlog::info("MultiStepRAG::runMapReduce complete: no partial answers generated");
        return result;
    }

    if (static_cast<int>(result.steps.size()) == 1u) {
        result.final_answer = result.steps.front();
        spdlog::info(
            "MultiStepRAG::runMapReduce complete: steps={} final_answer_chars={}",
            result.steps_executed,
            result.final_answer.size());
        return result;
    }

    // Reduce phase — synthesise partial answers.
    const std::string reduce_prompt =
        buildReducePrompt(result.steps, query);
    result.final_answer  = infer(reduce_prompt, bounded_max_tokens);
    ++result.steps_executed;

    spdlog::info(
        "MultiStepRAG::runMapReduce complete: steps={} final_answer_chars={} used_reduce_phase=1",
        result.steps_executed,
        result.final_answer.size());

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
    MultiStepRAGResult result = {};
    if (query.empty() || static_cast<int>(query.size()) > kMaxQueryChars) {
        spdlog::warn("MultiStepRAG::runIterative rejected: invalid query size={}",static_cast<int>(query.size()));
        return result;
    }
    for (const auto& doc : documents) {
        if (static_cast<int>(doc.content.size()) > kMaxChunkChars) {
            spdlog::warn("MultiStepRAG::runIterative rejected: oversize chunk");
            return result;
        }
    }

    const auto budget = ::themis::llm::ContextWindowBudget::compute(
        config_.assembler.model_context_tokens,
        config_.system_prompt,
        query,
        config_.assembler.min_response_tokens);
    const int bounded_max_tokens =
        RAGContextAssembler::computeMaxTokens(budget, config_.max_response_tokens);
    const int gap_max_tokens = std::max(1, std::min(256, bounded_max_tokens));

    spdlog::info(
        "MultiStepRAG::runIterative start: query_chars={} seed_docs={} max_iterations={} retrieval_top_k={}",
        query.size(),
        documents.size(),
        config_.max_iterations,
        config_.retrieval_top_k);

    if (!infer) {
      return result;
    }

    // Accumulate all documents across iterations.
    std::vector<RetrievedChunk> accumulated = documents;

    for (size_t iter = 0u; iter < config_.max_iterations; ++iter) {
        // Assemble context within budget.
        AssembledContext ctx = assembler_.assemble(
            accumulated, config_.system_prompt, query);

        if (ctx.was_truncated) {
          result.was_truncated = true;
        }

        const std::string prompt = buildMapPrompt(ctx.chunks_used, query);
        spdlog::info(
            "MultiStepRAG::runIterative iter={} accumulated_docs={} chunks_used={} truncated={} max_tokens={} gap_max_tokens={}",
            iter,
            accumulated.size(),
            ctx.chunks_used.size(),
            ctx.was_truncated,
            bounded_max_tokens,
            gap_max_tokens);

        result.final_answer = infer(prompt, bounded_max_tokens);
        result.steps.push_back(result.final_answer);
        ++result.steps_executed;

        // Check for uncovered aspects unless no retriever is provided.
        if (!retrieve) {
          break;
        }

        std::string gap_prompt = config_.gap_detection_prompt;
        gap_prompt = substitute(gap_prompt, "query",  query);
        gap_prompt = substitute(gap_prompt, "answer", result.final_answer);

        const std::string gap_response = infer(gap_prompt, gap_max_tokens);
        ++result.steps_executed;
         
        // Validate gap response before processing
        if (gap_response.empty()) {
            spdlog::warn("MultiStepRAG::runIterative: Empty gap detection response from LLM");
            break;
        }
         
        if (static_cast<int>(gap_response.size()) > kMaxGapResponseChars) {
            spdlog::warn("MultiStepRAG::runIterative gap-response too large; stopping refinement");
            break;
        }
         
        const auto aspects = parseOpenAspects(gap_response);
        if (aspects.empty()) {
            spdlog::info(
                "MultiStepRAG::runIterative complete: iteration={} no open aspects remaining",
                iter);
            break; // answer is complete
        }

        // Retrieve additional documents for the first uncovered aspect.
        const std::string refined_query = aspects.front();
        if (refined_query.empty()) {
            spdlog::warn("MultiStepRAG::runIterative produced empty refined query; stopping");
            break;
        }
        const auto new_docs = retrieve(refined_query, config_.retrieval_top_k);

        if (new_docs.empty()) {
            spdlog::info(
                "MultiStepRAG::runIterative complete: iteration={} refinement query produced no new docs",
                iter);
            break;
        }

        // Deduplicate by source before accumulating.
        for (const auto& nd : new_docs) {
            const bool already_present = std::any_of(
                accumulated.begin(), accumulated.end(),
                [&]([[maybe_unused]] const RetrievedChunk& c) {
                    return !c.source.empty() && c.source == nd.source;
                });
            if (!already_present) {
              accumulated.push_back(nd);
            }
        }
    }

    spdlog::info(
        "MultiStepRAG::runIterative complete: steps={} final_answer_chars={} truncated={} context_overflow={}",
        result.steps_executed,
        result.final_answer.size(),
        result.was_truncated,
        result.context_overflow);

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
