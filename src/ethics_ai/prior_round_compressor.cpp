/**
 * @file prior_round_compressor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/prior_round_compressor.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_map>

namespace themis {
namespace plugins {
namespace ethics {

// ---------------------------------------------------------------------------
// Injection API
// ---------------------------------------------------------------------------

void PriorRoundCompressor::setLlmSummaryFn(LlmSummaryFn fn) {
    llm_summary_fn_ = std::move(fn);
}

// ---------------------------------------------------------------------------
// Private static helpers
// ---------------------------------------------------------------------------

int PriorRoundCompressor::countTokens(const std::string &text) noexcept {
    return static_cast<int>((text.size() + 3) / 4);
}

std::vector<std::string> PriorRoundCompressor::extractPrincipleCitations(const std::string &content) {
    std::vector<std::string> citations;
    std::set<std::string> seen;

    // Pattern 1: thesis_id:word (e.g. "kant:kategorischer_imperativ")
    {
        std::regex re(R"(\b(\w+:\w[\w_]*)\b)");
        auto begin = std::sregex_iterator(content.begin(), content.end(), re);
        for (auto it = begin; it != std::sregex_iterator(); ++it) {
            const std::string match = (*it)[1].str();
            if (seen.insert(match).second) {
                citations.push_back(match);
            }
        }
    }

    // Pattern 2: [word:word] bracket notation (e.g. "[kant:thesis_1]")
    {
        std::regex re(R"(\[(\w+:\w[\w_]*)\])");
        auto begin = std::sregex_iterator(content.begin(), content.end(), re);
        for (auto it = begin; it != std::sregex_iterator(); ++it) {
            const std::string match = (*it)[1].str();
            if (seen.insert(match).second) {
                citations.push_back(match);
            }
        }
    }

    // Pattern 3: words containing underscores (potential thesis ID tokens)
    {
        std::regex re(R"(\b(\w+_\w+)\b)");
        auto begin = std::sregex_iterator(content.begin(), content.end(), re);
        for (auto it = begin; it != std::sregex_iterator(); ++it) {
            const std::string match = (*it)[1].str();
            if (seen.insert(match).second) {
                citations.push_back(match);
            }
        }
    }

    return citations;
}

std::string PriorRoundCompressor::extractVerdict(const std::string &content) {
    std::string upper = content;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    for (const auto &keyword : {"PROHIBIT", "CONDITIONAL", "ABSTAIN", "PERMIT"}) {
        if (upper.find(keyword) != std::string::npos) {
            return keyword;
        }
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Compression modes
// ---------------------------------------------------------------------------

CompressionResult PriorRoundCompressor::compressPrincipleCitationsOnly(const EthicalArgument &arg,
                                                                       const CompressionConfig &config) const {
    CompressionResult result;
    result.original_tokens = countTokens(arg.content);

    std::string verdict = config.keep_verdict ? extractVerdict(arg.content) : std::string{};

    // Collect citations: prefer principle_basis, then extract from content
    std::vector<std::string> citations = arg.principle_basis;
    if (citations.empty()) {
        citations = extractPrincipleCitations(arg.content);
    }

    std::ostringstream oss;
    oss << "[" << arg.philosophy_school << "|R" << "]";
    if (!citations.empty()) {
        oss << " Citations:";
        for (const auto &c : citations) {
            oss << " " << c << ";";
        }
    }
    if (!verdict.empty()) {
        oss << " Verdict: " << verdict << ".";
    }

    std::string text = oss.str();

    // Trim to token budget
    if (countTokens(text) > config.max_tokens_per_round) {
        const int max_chars = config.max_tokens_per_round * 4;
        text                = text.substr(0, static_cast<size_t>(max_chars));
    }

    result.compressed_text          = text;
    result.compressed_tokens        = countTokens(text);
    result.compression_ratio        = result.original_tokens > 0 ? static_cast<float>(result.compressed_tokens)
                                                                       / static_cast<float>(result.original_tokens)
                                                                 : 1.0f;
    result.estimated_dc_loss        = measureDcLoss(arg.content, text);
    result.coherence_anchors_intact = config.keep_thesis_id_anchors && !citations.empty();

    return result;
}

CompressionResult PriorRoundCompressor::compressHeadline(const EthicalArgument &arg,
                                                         const CompressionConfig & /*config*/) const {
    CompressionResult result;
    result.original_tokens = countTokens(arg.content);

    // Ultra-sparse: only "[school: arg_type]"
    const char *type_label = "ARG";
    switch (arg.argument_type) {
        case ArgumentType::PRO:
            type_label = "PRO";
            break;
        case ArgumentType::CONTRA:
            type_label = "CONTRA";
            break;
        case ArgumentType::REBUTTAL:
            type_label = "REBUTTAL";
            break;
        case ArgumentType::SYNTHESIS:
            type_label = "SYNTHESIS";
            break;
        case ArgumentType::QUESTION:
            type_label = "QUESTION";
            break;
        case ArgumentType::CLARIFICATION:
            type_label = "CLARIFICATION";
            break;
    }

    result.compressed_text          = "[" + arg.philosophy_school + ": " + type_label + "]";
    result.compressed_tokens        = countTokens(result.compressed_text);
    result.compression_ratio        = result.original_tokens > 0 ? static_cast<float>(result.compressed_tokens)
                                                                       / static_cast<float>(result.original_tokens)
                                                                 : 1.0f;
    result.estimated_dc_loss        = measureDcLoss(arg.content, result.compressed_text);
    result.coherence_anchors_intact = false;

    return result;
}

CompressionResult PriorRoundCompressor::compressStructuredSummary(const EthicalArgument &arg,
                                                                  const CompressionConfig &config) const {
    // Delegate to the injected LLM summariser when available.
    if (llm_summary_fn_) {
        const std::string llm_text = llm_summary_fn_(arg, config.max_tokens_per_round);
        if (!llm_text.empty()) {
            CompressionResult result;
            result.original_tokens   = countTokens(arg.content);
            result.compressed_text   = llm_text;
            result.compressed_tokens = countTokens(llm_text);
            result.compression_ratio = result.original_tokens > 0 ? static_cast<float>(result.compressed_tokens)
                                                                        / static_cast<float>(result.original_tokens)
                                                                  : 1.0f;
            result.estimated_dc_loss = measureDcLoss(arg.content, llm_text);
            result.coherence_anchors_intact = config.keep_thesis_id_anchors;
            return result;
        }
        // fn returned empty → fall through to extractive path
    }

    // Extractive sentence summarisation: score sentences by TF-weighted importance,
    // boosting those containing principle citations or verdict keywords.  Selects
    // sentences greedily until the token budget is exhausted.
    //
    // This replaces the former delegation to compressPrincipleCitationsOnly() and
    // achieves substantially better DC preservation because full sentences (rather
    // than only citation tokens) are retained.  See STUB_INVENTORY.md entry #235.

    CompressionResult result;
    result.original_tokens = countTokens(arg.content);

    // ── 1. Split content into sentences ──────────────────────────────────────
    std::vector<std::string> sentences;
    {
        std::string current;
        for (char c : arg.content) {
            current += c;
            if (c == '.' || c == '!' || c == '?' || c == '\n') {
                // Trim leading/trailing whitespace
                size_t start = current.find_first_not_of(" \t\r\n");
                if (start != std::string::npos) {
                    size_t end = current.find_last_not_of(" \t\r\n");
                    sentences.push_back(current.substr(start, end - start + 1));
                }
                current.clear();
            }
        }
        if (!current.empty()) {
            size_t start = current.find_first_not_of(" \t\r\n");
            if (start != std::string::npos) {
                size_t end = current.find_last_not_of(" \t\r\n");
                sentences.push_back(current.substr(start, end - start + 1));
            }
        }
    }

    if (sentences.empty()) {
        return compressPrincipleCitationsOnly(arg, config);
    }

    // ── 2. Build word-frequency table (TF) from the full content ─────────────
    std::unordered_map<std::string, int> word_freq;
    {
        std::istringstream iss(arg.content);
        std::string word;
        while (iss >> word) {
            // Normalise: lower-case, strip trailing punctuation
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            while (!word.empty() && !std::isalnum(static_cast<unsigned char>(word.back()))) {
                word.pop_back();
            }
            if (word.size() >= 3) { // ignore very short words
                ++word_freq[word];
            }
        }
    }

    // ── 3. Collect citation and verdict tokens for boosting ───────────────────
    const auto citations = [&]() {
        std::vector<std::string> c = arg.principle_basis;
        if (c.empty()) {
            c = extractPrincipleCitations(arg.content);
        }
        return c;
    }();
    const std::string verdict_upper = [&]() {
        std::string v = arg.content;
        std::transform(v.begin(), v.end(), v.begin(), ::toupper);
        return v;
    }();
    // Verdict keywords: same set as extractVerdict() — keep in sync.
    static const std::array<const char *, 4> kVerdictKeywords{"PROHIBIT", "CONDITIONAL", "ABSTAIN", "PERMIT"};

    // ── 4. Score each sentence ────────────────────────────────────────────────
    std::vector<std::pair<float, size_t>> scored; // (score, sentence_index)
    scored.reserve(sentences.size());

    for (size_t i = 0; i < sentences.size(); ++i) {
        const std::string &sent = sentences[i];
        float score             = 0.f;

        // TF component: sum of word frequencies
        std::istringstream iss(sent);
        std::string word;
        int word_count = 0;
        while (iss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            while (!word.empty() && !std::isalnum(static_cast<unsigned char>(word.back()))) {
                word.pop_back();
            }
            auto it = word_freq.find(word);
            if (it != word_freq.end()) {
                score += static_cast<float>(it->second);
            }
            ++word_count;
        }
        if (word_count > 0) {
            score /= static_cast<float>(word_count); // normalise by length
        }

        // Boost: contains a principle citation
        std::string sent_upper = sent;
        std::transform(sent_upper.begin(), sent_upper.end(), sent_upper.begin(), ::toupper);
        for (const auto &cite : citations) {
            std::string cite_upper = cite;
            std::transform(cite_upper.begin(), cite_upper.end(), cite_upper.begin(), ::toupper);
            if (sent_upper.find(cite_upper) != std::string::npos) {
                score += 2.f;
                break;
            }
        }

        // Boost: contains a verdict keyword
        for (const char *kw : kVerdictKeywords) {
            if (sent_upper.find(kw) != std::string::npos) {
                score += 1.5f;
                break;
            }
        }

        // Slight position bias: first and last sentences often contain thesis / verdict
        if (i == 0 || i == sentences.size() - 1) {
            score += 0.5f;
        }

        scored.emplace_back(score, i);
    }

    // Sort by score descending
    std::sort(scored.begin(), scored.end(), [](const auto &a, const auto &b) { return a.first > b.first; });

    // ── 5. Greedily select sentences within token budget ─────────────────────
    const int budget = std::max(config.max_tokens_per_round, 20);
    std::vector<size_t> selected_indices;
    int used_tokens = 0;

    for (const auto &[sc, idx] : scored) {
        const int t = countTokens(sentences[idx]);
        if (used_tokens + t > budget && !selected_indices.empty()) {
            break;
        }
        selected_indices.push_back(idx);
        used_tokens += t;
        if (used_tokens >= budget) {
            break;
        }
    }

    // Restore original order for readability
    std::sort(selected_indices.begin(), selected_indices.end());

    // ── 6. Build output text with verdict prefix when kept ────────────────────
    std::ostringstream oss;
    oss << "[" << arg.philosophy_school << "|R]";

    if (config.keep_verdict) {
        const std::string verdict = extractVerdict(arg.content);
        if (verdict != "UNKNOWN") {
            oss << " Verdict:" << verdict << ".";
        }
    }

    for (size_t idx : selected_indices) {
        oss << " " << sentences[idx];
    }

    result.compressed_text          = oss.str();
    result.compressed_tokens        = countTokens(result.compressed_text);
    result.compression_ratio        = result.original_tokens > 0 ? static_cast<float>(result.compressed_tokens)
                                                                       / static_cast<float>(result.original_tokens)
                                                                 : 1.0f;
    result.estimated_dc_loss        = measureDcLoss(arg.content, result.compressed_text);
    result.coherence_anchors_intact = config.keep_thesis_id_anchors && !citations.empty();

    return result;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

CompressionResult PriorRoundCompressor::compressPriorRound(const std::vector<EthicalArgument> &round_arguments,
                                                           const CompressionConfig &config, int current_round) const {
    // No compression before trigger_round
    if (current_round < config.trigger_round) {
        std::ostringstream oss;
        for (const auto &arg : round_arguments) {
            oss << arg.content << "\n";
        }
        const std::string original_text = oss.str();
        CompressionResult result;
        result.compressed_text          = original_text;
        result.original_tokens          = countTokens(original_text);
        result.compressed_tokens        = result.original_tokens;
        result.compression_ratio        = 1.0f;
        result.estimated_dc_loss        = 0.0f;
        result.coherence_anchors_intact = true;
        return result;
    }

    // Compress each argument and combine
    std::ostringstream combined;
    int total_original   = 0;
    int total_compressed = 0;

    for (const auto &arg : round_arguments) {
        CompressionResult cr;
        switch (config.mode) {
            case CompressionMode::PRINCIPLE_CITATIONS_ONLY:
                cr = compressPrincipleCitationsOnly(arg, config);
                break;
            case CompressionMode::STRUCTURED_SUMMARY:
                cr = compressStructuredSummary(arg, config);
                break;
            case CompressionMode::HEADLINE:
                cr = compressHeadline(arg, config);
                break;
        }
        combined << cr.compressed_text << "\n";
        total_original += cr.original_tokens;
        total_compressed += cr.compressed_tokens;
    }

    CompressionResult result;
    result.compressed_text   = combined.str();
    result.original_tokens   = total_original;
    result.compressed_tokens = countTokens(result.compressed_text);
    result.compression_ratio
        = total_original > 0 ? static_cast<float>(result.compressed_tokens) / static_cast<float>(total_original) : 1.0f;

    // Collect original full text for DC measurement
    std::ostringstream orig_oss;
    for (const auto &arg : round_arguments) {
        orig_oss << arg.content << "\n";
    }
    result.estimated_dc_loss        = measureDcLoss(orig_oss.str(), result.compressed_text);
    result.coherence_anchors_intact = config.keep_thesis_id_anchors;

    return result;
}

std::string PriorRoundCompressor::buildPriorContext(const std::vector<std::vector<EthicalArgument>> &all_rounds,
                                                    const CompressionConfig &config, int current_round,
                                                    int max_total_tokens) const {
    // Oldest rounds → HEADLINE; recent rounds → configured mode
    // "Old" = more than 2 rounds in the past
    std::ostringstream out;
    int accumulated_tokens = 0;

    for (int i = 0; i < static_cast<int>(all_rounds.size()); ++i) {
        const int round_number = i + 1; // 1-based
        const int rounds_ago   = current_round - round_number;

        CompressionConfig round_config = config;
        if (rounds_ago > 2) {
            round_config.mode = CompressionMode::HEADLINE;
        } else if (rounds_ago > 1) {
            round_config.mode = CompressionMode::PRINCIPLE_CITATIONS_ONLY;
        }
        // rounds_ago == 1 or 0: use config.mode as-is

        const CompressionResult cr = compressPriorRound(all_rounds[i], round_config, current_round);

        if (accumulated_tokens + cr.compressed_tokens > max_total_tokens) {
            // Try headline as a last resort to fit within budget
            if (round_config.mode != CompressionMode::HEADLINE) {
                round_config.mode                   = CompressionMode::HEADLINE;
                const CompressionResult headline_cr = compressPriorRound(all_rounds[i], round_config, current_round);
                if (accumulated_tokens + headline_cr.compressed_tokens <= max_total_tokens) {
                    out << "R" << round_number << ": " << headline_cr.compressed_text;
                    accumulated_tokens += headline_cr.compressed_tokens;
                }
            }
            // Otherwise skip this round to respect the hard budget
            break;
        }

        out << "R" << round_number << ": " << cr.compressed_text;
        accumulated_tokens += cr.compressed_tokens;
    }

    return out.str();
}

float PriorRoundCompressor::measureDcLoss(const std::string &original_arg, const std::string &compressed_arg) const {
    // Jaccard distance on whitespace-tokenized sets
    auto tokenize = [](const std::string &text) -> std::set<std::string> {
        std::set<std::string> tokens;
        std::istringstream iss(text);
        std::string tok;
        while (iss >> tok) {
            tokens.insert(tok);
        }
        return tokens;
    };

    const std::set<std::string> orig_tokens = tokenize(original_arg);
    const std::set<std::string> comp_tokens = tokenize(compressed_arg);

    if (orig_tokens.empty() && comp_tokens.empty()) {
        return 0.0f;
    }

    std::set<std::string> intersection;
    std::set_intersection(orig_tokens.begin(), orig_tokens.end(), comp_tokens.begin(), comp_tokens.end(),
                          std::inserter(intersection, intersection.begin()));

    std::set<std::string> union_set;
    std::set_union(orig_tokens.begin(), orig_tokens.end(), comp_tokens.begin(), comp_tokens.end(),
                   std::inserter(union_set, union_set.begin()));

    if (union_set.empty()) {
        return 0.0f;
    }

    const float jaccard_similarity = static_cast<float>(intersection.size()) / static_cast<float>(union_set.size());
    return 1.0f - jaccard_similarity;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
