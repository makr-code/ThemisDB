#include "plugins/ethics_ai/prior_round_compressor.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <sstream>

namespace themis {
namespace plugins {
namespace ethics {

// ---------------------------------------------------------------------------
// Private static helpers
// ---------------------------------------------------------------------------

int PriorRoundCompressor::countTokens(const std::string& text) noexcept {
    return static_cast<int>((text.size() + 3) / 4);
}

std::vector<std::string> PriorRoundCompressor::extractPrincipleCitations(
    const std::string& content)
{
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

std::string PriorRoundCompressor::extractVerdict(const std::string& content) {
    std::string upper = content;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    for (const auto& keyword : {"PROHIBIT", "CONDITIONAL", "ABSTAIN", "PERMIT"}) {
        if (upper.find(keyword) != std::string::npos) {
            return keyword;
        }
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Compression modes
// ---------------------------------------------------------------------------

CompressionResult PriorRoundCompressor::compressPrincipleCitationsOnly(
    const EthicalArgument& arg,
    const CompressionConfig& config) const
{
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
        for (const auto& c : citations) {
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
        text = text.substr(0, static_cast<size_t>(max_chars));
    }

    result.compressed_text = text;
    result.compressed_tokens = countTokens(text);
    result.compression_ratio = result.original_tokens > 0
        ? static_cast<float>(result.compressed_tokens) / static_cast<float>(result.original_tokens)
        : 1.0f;
    result.estimated_dc_loss = measureDcLoss(arg.content, text);
    result.coherence_anchors_intact = config.keep_thesis_id_anchors && !citations.empty();

    return result;
}

CompressionResult PriorRoundCompressor::compressHeadline(
    const EthicalArgument& arg,
    const CompressionConfig& /*config*/) const
{
    CompressionResult result;
    result.original_tokens = countTokens(arg.content);

    // Ultra-sparse: only "[school: arg_type]"
    const char* type_label = "ARG";
    switch (arg.argument_type) {
        case ArgumentType::PRO:           type_label = "PRO";           break;
        case ArgumentType::CONTRA:        type_label = "CONTRA";        break;
        case ArgumentType::REBUTTAL:      type_label = "REBUTTAL";      break;
        case ArgumentType::SYNTHESIS:     type_label = "SYNTHESIS";     break;
        case ArgumentType::QUESTION:      type_label = "QUESTION";      break;
        case ArgumentType::CLARIFICATION: type_label = "CLARIFICATION"; break;
    }

    result.compressed_text = "[" + arg.philosophy_school + ": " + type_label + "]";
    result.compressed_tokens = countTokens(result.compressed_text);
    result.compression_ratio = result.original_tokens > 0
        ? static_cast<float>(result.compressed_tokens) / static_cast<float>(result.original_tokens)
        : 1.0f;
    result.estimated_dc_loss = measureDcLoss(arg.content, result.compressed_text);
    result.coherence_anchors_intact = false;

    return result;
}

CompressionResult PriorRoundCompressor::compressStructuredSummary(
    const EthicalArgument& arg,
    const CompressionConfig& config) const
{
    // STUB/SIMULATION NOTE:
    // Purpose: Real STRUCTURED_SUMMARY sends arg.content to a small LLM and
    //          returns an abstractive summary. Backend not yet integrated.
    // Activation: Always (LLM backend not yet wired in this module).
    // Production Delta: Real impl calls small model tier and returns abstractive
    //                   summary with ~60 % reduction and ΔDC ≤ −0.08.
    // Removal Plan: Replace with real LLM dispatch (§12.2.1, Q3 2026).
    return compressPrincipleCitationsOnly(arg, config);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

CompressionResult PriorRoundCompressor::compressPriorRound(
    const std::vector<EthicalArgument>& round_arguments,
    const CompressionConfig& config,
    int current_round) const
{
    // No compression before trigger_round
    if (current_round < config.trigger_round) {
        std::ostringstream oss;
        for (const auto& arg : round_arguments) {
            oss << arg.content << "\n";
        }
        const std::string original_text = oss.str();
        CompressionResult result;
        result.compressed_text      = original_text;
        result.original_tokens      = countTokens(original_text);
        result.compressed_tokens    = result.original_tokens;
        result.compression_ratio    = 1.0f;
        result.estimated_dc_loss    = 0.0f;
        result.coherence_anchors_intact = true;
        return result;
    }

    // Compress each argument and combine
    std::ostringstream combined;
    int total_original  = 0;
    int total_compressed = 0;

    for (const auto& arg : round_arguments) {
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
        total_original   += cr.original_tokens;
        total_compressed += cr.compressed_tokens;
    }

    CompressionResult result;
    result.compressed_text   = combined.str();
    result.original_tokens   = total_original;
    result.compressed_tokens = countTokens(result.compressed_text);
    result.compression_ratio = total_original > 0
        ? static_cast<float>(result.compressed_tokens) / static_cast<float>(total_original)
        : 1.0f;

    // Collect original full text for DC measurement
    std::ostringstream orig_oss;
    for (const auto& arg : round_arguments) {
        orig_oss << arg.content << "\n";
    }
    result.estimated_dc_loss    = measureDcLoss(orig_oss.str(), result.compressed_text);
    result.coherence_anchors_intact = config.keep_thesis_id_anchors;

    return result;
}

std::string PriorRoundCompressor::buildPriorContext(
    const std::vector<std::vector<EthicalArgument>>& all_rounds,
    const CompressionConfig& config,
    int current_round,
    int max_total_tokens) const
{
    // Oldest rounds → HEADLINE; recent rounds → configured mode
    // "Old" = more than 2 rounds in the past
    std::ostringstream out;
    int accumulated_tokens = 0;

    for (int i = 0; i < static_cast<int>(all_rounds.size()); ++i) {
        const int round_number = i + 1;  // 1-based
        const int rounds_ago = current_round - round_number;

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
                round_config.mode = CompressionMode::HEADLINE;
                const CompressionResult headline_cr = compressPriorRound(
                    all_rounds[i], round_config, current_round);
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

float PriorRoundCompressor::measureDcLoss(
    const std::string& original_arg,
    const std::string& compressed_arg) const
{
    // Jaccard distance on whitespace-tokenized sets
    auto tokenize = [](const std::string& text) -> std::set<std::string> {
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
    std::set_intersection(
        orig_tokens.begin(), orig_tokens.end(),
        comp_tokens.begin(), comp_tokens.end(),
        std::inserter(intersection, intersection.begin()));

    std::set<std::string> union_set;
    std::set_union(
        orig_tokens.begin(), orig_tokens.end(),
        comp_tokens.begin(), comp_tokens.end(),
        std::inserter(union_set, union_set.begin()));

    if (union_set.empty()) {
        return 0.0f;
    }

    const float jaccard_similarity =
        static_cast<float>(intersection.size()) / static_cast<float>(union_set.size());
    return 1.0f - jaccard_similarity;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
