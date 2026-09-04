/**
 * @file multi_hop_reasoner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/multi_hop_reasoner.h"

#include "utils/string_utils.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <unordered_set>

namespace themis::rag::multi_hop {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

MultiHopReasoner::MultiHopReasoner(const MultiHopConfig& config)
    : config_(config)
{}

const MultiHopConfig& MultiHopReasoner::getConfig() const
{
    return config_;
}

void MultiHopReasoner::setConfig(const MultiHopConfig& config)
{
    config_ = config;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/**
 * Trim leading and trailing whitespace from @p s.
 * Using themis::utils::themis::utils::trim() from string_utils.h (Phase 1 consolidation)
 */

/**
 * Replace all occurrences of @p key with @p value in @p tmpl.
 */
std::string substitute(const std::string& tmpl,
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

/**
 * Deduplicate documents by id, preserving first occurrence order.
 */
std::vector<judge::RetrievedDocument> deduplicateDocs(
    const std::vector<judge::RetrievedDocument>& docs)
{
    std::vector<judge::RetrievedDocument> result;
    std::unordered_set<std::string> seen = {};

    result.reserve(docs.size());
    for (const auto& d : docs) {
        if (seen.insert(d.id).second) {
            result.push_back(d);
        }
    }
    return result;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// decomposeQuery
// ---------------------------------------------------------------------------

std::vector<std::string> MultiHopReasoner::parseDecompositionResponse(
    const std::string& response) const
{
    std::vector<std::string> sub_queries;
    std::istringstream ss(response);
    std::string line = {};
    while (std::getline(ss, line)) {
        // Strip leading list markers: "1.", "2.", "-", "*"
        std::string t = themis::utils::trim(line);
        if (t.empty()) {
          continue;
        }
        // Remove leading digit+dot or dash/star
        if ((static_cast<int>(t.size()) >= 2 &&
            ((std::isdigit(static_cast<unsigned char>(t[0])) && t[1] == '.') ||
            t[0] == '-' || t[0] == '*'))) {
            t = themis::utils::trim(t.substr(t.find_first_not_of("0123456789.-* \t")));
        }
        if (!t.empty()) {
          sub_queries.push_back(t);
        }
    }
    // Cap at max_hops
    if (static_cast<int>(sub_queries.size()) > config_.max_hops) {
        sub_queries.resize(config_.max_hops);
    }
    return sub_queries;
}

std::vector<std::string> MultiHopReasoner::heuristicDecompose(
    const std::string& query) const
{
    // Split on "and" / "," at the top level — very lightweight.
    std::vector<std::string> parts;
    const std::string q = themis::utils::trim(query);
    if (q.empty()) {
      return parts;
    }

    // Simple sentence boundary split on ". " or "? "
    std::vector<std::string> sentences = {};

    sentences.reserve(q.size() / 20);  // Estimate: average sentence ~20 chars
    std::string acc = {};
    for (size_t i = 0; i <static_cast<int>(q.size()); ++i) {
        acc += q[i];
        if (((q[i] == '.' || q[i] == '?') &&
            i + 1 <static_cast<int>(q.size()) && q[i + 1] == ' ')) {
            const auto t = themis::utils::trim(acc);
            if (!t.empty()) {
              sentences.push_back(t);
            }
            acc.clear();
        }
    }
    const auto t = themis::utils::trim(acc);
    if (!t.empty()) {
      sentences.push_back(t);
    }

    // If only one sentence, return it as-is (single hop)
    if (static_cast<int>(sentences.size()) <= 1) {
        parts.push_back(q);
        return parts;
    }

    for (const auto& s : sentences) {
        parts.push_back(s);
        if (static_cast<int>(parts.size()) >= config_.max_hops) {
          break;
        }
    }
    return parts;
}

std::vector<std::string> MultiHopReasoner::decomposeQuery(
    const std::string& query,
    InferenceFn inference_fn) const
{
    if (query.empty()) return {};

    if (inference_fn) {
        // Use LLM to decompose
        std::string prompt = substitute(
            config_.decomposition_prompt_template, "query", query);
        const std::string response =
            inference_fn(prompt, config_.max_tokens_per_hop);
        if (!response.empty()) {
            auto sub_queries = parseDecompositionResponse(response);
            if (!sub_queries.empty()) {
              return sub_queries;
            }
        }
    }
    // Fallback: heuristic decomposition
    return heuristicDecompose(query);
}

// ---------------------------------------------------------------------------
// buildHopPrompt
// ---------------------------------------------------------------------------

std::string MultiHopReasoner::buildHopPrompt(
    const std::string& sub_query,
    const std::vector<judge::RetrievedDocument>& documents,
    const std::vector<std::string>& previous_answers) const
{
    std::ostringstream prompt = {};

    // Inject previous answers as context when available
    if (!previous_answers.empty()) {
        prompt << config_.context_prefix;
        for (size_t i = 0; i <static_cast<int>(previous_answers.size()); ++i) {
            if (!previous_answers[i].empty()) {
                prompt << "Step " << (i + 1) << ": " << previous_answers[i];
                if (i + 1 <static_cast<int>(previous_answers.size())) {
                    prompt << config_.answer_separator;
                }
            }
        }
        prompt << "\n\n";
    }

    // Append retrieved documents
    if (!documents.empty()) {
        prompt << "Retrieved documents:\n";
        for (size_t i = 0; i <static_cast<int>(documents.size()); ++i) {
            prompt << "[" << (i + 1) << "] " << documents[i].content << "\n";
        }
        prompt << "\n";
    }

    prompt << "Question: " << sub_query;
    return prompt.str();
}

// ---------------------------------------------------------------------------
// composeAnswer
// ---------------------------------------------------------------------------

std::string MultiHopReasoner::composeAnswer(
    const std::string& original_query,
    const std::vector<HopRecord>& hop_records,
    InferenceFn inference_fn) const
{
    // Gather all non-empty intermediate answers
    std::vector<std::string> partial_answers = {};

    partial_answers.reserve(hop_records.size());  // Upper bound: all hops may have answers
    for (const auto& hr : hop_records) {
        if (hr.succeeded && !hr.intermediate_answer.empty()) {
            partial_answers.push_back(
                "Step " + std::to_string(hr.hop_index + 1) +
                " (" + hr.sub_query + "): " + hr.intermediate_answer);
        }
    }

    if (partial_answers.empty()) return {};

    // If only one hop, its answer IS the final answer
    if (static_cast<int>(partial_answers.size()) == 1) {
      return hop_records[0].intermediate_answer;
    }

    if (!inference_fn) {
        // Without LLM, concatenate partial answers
        std::ostringstream oss = {};
        for (const auto& pa : partial_answers) {
            oss << pa << "\n";
        }
        return oss.str();
    }

    // Assemble composition prompt
    std::ostringstream pa_block = {};
    for (const auto& pa : partial_answers) {
        pa_block << pa << "\n";
    }

    std::string prompt = substitute(
        config_.composition_prompt_template, "partial_answers", pa_block.str());
    prompt = substitute(prompt, "query", original_query);

    const std::string composed = inference_fn(prompt, config_.max_tokens_final);
    return composed.empty() ? partial_answers.back() : composed;
}

// ---------------------------------------------------------------------------
// reason — main entry point
// ---------------------------------------------------------------------------

MultiHopResult MultiHopReasoner::reason(
    const std::string& query,
    RetrievalFn retrieval_fn,
    InferenceFn inference_fn) const
{
    MultiHopResult result = {};

    if (query.empty() || !retrieval_fn || !inference_fn) {
        return result;
    }

    const auto pipeline_start = std::chrono::steady_clock::now();

    // Step 1: Decompose the query into sub-questions
    std::vector<std::string> sub_queries =
        decomposeQuery(query, inference_fn);

    if (sub_queries.empty()) {
      sub_queries.push_back(query);
    }

    // Cap sub-queries at max_hops
    if (static_cast<int>(sub_queries.size()) > config_.max_hops) {
        sub_queries.resize(config_.max_hops);
    }

    // Step 2: Execute one hop per sub-query
    std::vector<std::string> previous_answers = {};

    previous_answers.reserve(sub_queries.size());  // Upper bound: one answer per sub-query
    
    for (size_t i = 0; i <static_cast<int>(sub_queries.size()); ++i) {
        const auto hop_start = std::chrono::steady_clock::now();

        HopRecord hop;
        hop.hop_index  = i;
        hop.sub_query  = sub_queries[i];

        // 2a: Retrieve documents for this sub-query
        hop.documents = retrieval_fn(hop.sub_query, config_.top_k_per_hop);

        // 2b: Build prompt including previous context
        const std::string prompt =
            buildHopPrompt(hop.sub_query, hop.documents, previous_answers);

        // 2c: Generate intermediate answer
        hop.intermediate_answer =
            inference_fn(prompt, config_.max_tokens_per_hop);
        hop.succeeded = !hop.intermediate_answer.empty();

        const auto hop_end = std::chrono::steady_clock::now();
        hop.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            hop_end - hop_start);

        result.hop_records.push_back(hop);
        ++result.hops_executed;

        // Accumulate documents
        for (const auto& doc : hop.documents) {
            result.all_documents.push_back(doc);
        }

        if (hop.succeeded) {
            previous_answers.push_back(hop.intermediate_answer);
        }

        // Early stopping: if this was the only sub-query, stop here
        if (config_.early_stopping && static_cast<int>(sub_queries.size()) == 1) {
            result.early_stopped = true;
            break;
        }
    }

    result.hit_hop_limit = (result.hops_executed >= config_.max_hops &&
                             static_cast<int>(sub_queries.size()) >= config_.max_hops);

    // Step 3: Deduplicate collected documents
    result.all_documents = deduplicateDocs(result.all_documents);

    // Step 4: Compose final answer
    result.final_answer = composeAnswer(query, result.hop_records, inference_fn);

    const auto pipeline_end = std::chrono::steady_clock::now();
    result.total_elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            pipeline_end - pipeline_start);

    return result;
}

// ---------------------------------------------------------------------------
// MultiHopReasonerFactory
// ---------------------------------------------------------------------------

std::unique_ptr<MultiHopReasoner> MultiHopReasonerFactory::createSingleHop()
{
    MultiHopConfig cfg;
    cfg.max_hops        = 1;
    cfg.top_k_per_hop   = 5;
    cfg.early_stopping  = true;
    return std::make_unique<MultiHopReasoner>(cfg);
}

std::unique_ptr<MultiHopReasoner> MultiHopReasonerFactory::createBalanced()
{
    MultiHopConfig cfg;
    cfg.max_hops        = 3;
    cfg.top_k_per_hop   = 5;
    cfg.early_stopping  = true;
    return std::make_unique<MultiHopReasoner>(cfg);
}

std::unique_ptr<MultiHopReasoner> MultiHopReasonerFactory::createDeepReasoning()
{
    MultiHopConfig cfg;
    cfg.max_hops              = 5;
    cfg.top_k_per_hop         = 8;
    cfg.max_tokens_per_hop    = 512;
    cfg.max_tokens_final      = 1024;
    cfg.early_stopping        = false;
    return std::make_unique<MultiHopReasoner>(cfg);
}

} // namespace themis::rag::multi_hop
