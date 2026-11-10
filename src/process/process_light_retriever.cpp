/**
 * @file process_light_retriever.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_light_retriever.cpp
 * Module:  src/process/
 * Purpose: Dual-mode LOCAL/GLOBAL retrieval following the LightRAG approach
 *          (Guo et al., 2024, arXiv:2410.05779). P5 implementation.
 */

#include "process/process_light_retriever.h"
#include <stdexcept>
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <limits>
#include <sstream>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// Constants – global retrieval keywords (case-insensitive matching)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

constexpr const char* kGlobalKeywords[] = {
    "gesamte", "überblick", "alle", "prozess", "ablauf", "workflow",
    "beschreibe", "erklär", "summary", "overview"
};

std::string truncateUtf8(std::string_view text, size_t max_bytes) {
    size_t length = std::min(text.size(), max_bytes);
    if (length < text.size() && length > 0) {
        while (length > 0 &&
               (static_cast<unsigned char>(text[length]) & 0xC0U) == 0x80U) {
            --length;
        }
    }
    return std::string(text.substr(0, length));
}

std::string truncateLines(std::string_view text, size_t max_lines) {
    if (max_lines == 0) {
        return {};
    }

    size_t line_count = 0;
    size_t position = 0;
    while (position < text.size()) {
        ++line_count;
        const size_t newline = text.find('\n', position);
        if (newline == std::string_view::npos) {
            return std::string(text);
        }
        position = newline + 1;
        if (line_count == max_lines) {
            return std::string(text.substr(0, position));
        }
    }
    return std::string(text);
}

/**
 * @brief To Lower.
 * @param[in] sv Input parameter.
 * @return Return value.
 * @details Calls: reserve(), size(), push_back(), std::tolower().
 */
std::string toLower(std::string_view sv) {
    std::string result = {};
    result.reserve(sv.size());
    for (unsigned char c : sv) {
        result.push_back(static_cast<char>(std::tolower(c)));
    }
    return result;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// ProcessLightRetriever
// ─────────────────────────────────────────────────────────────────────────────

ProcessLightRetriever::ProcessLightRetriever(
    RocksDBWrapper&           db,
    ProcessGraphRag&          graph_rag,
    ProcessCommunityDetector& community_detector)
    : graph_rag_(graph_rag)
    , community_detector_(community_detector)
    , db_(db)
{}

RetrievalMode ProcessLightRetriever::classifyQuery(std::string_view query) const {
    const std::string lower = toLower(query);
    for (const char* kw : kGlobalKeywords) {
        if (lower.find(kw) != std::string::npos) {
            return RetrievalMode::GLOBAL;
        }
    }
    return RetrievalMode::LOCAL;
}

LightRetrievalResult ProcessLightRetriever::retrieve(
    std::string_view        query,
    std::string_view        instance_id,
    RetrievalMode           mode,
    const ProcessRagConfig& config) const
{
    using SteadyClock = std::chrono::steady_clock;
    const auto started_at = SteadyClock::now();

    // Resolve effective mode
    const RetrievalMode effective = (mode == RetrievalMode::AUTO)
                                  ? classifyQuery(query)
                                  : mode;

    LightRetrievalResult result{
        .used_mode = RetrievalMode::LOCAL,
        .llm_context = {},
        .community_ids_used = {},
        .instance_id_used = std::string(instance_id),
        .retrieval_time_ms = 0,
        .context_size_bytes = 0,
        .degraded = false,
        .resource_exhaustion_reason = std::nullopt
    };
    std::vector<std::string> degradation_reasons;
    const auto addDegradation = [&degradation_reasons](std::string reason) {
        if (std::find(degradation_reasons.begin(), degradation_reasons.end(), reason) ==
            degradation_reasons.end()) {
            degradation_reasons.push_back(std::move(reason));
        }
    };

    if (effective == RetrievalMode::GLOBAL) {
        // ── GLOBAL path ──────────────────────────────────────────────────────
        // Resolve model_id from the instance record stored under proc:inst:<id>
        std::string model_id = {};
        {
            std::string inst_val = {};
            if (db_.get(std::string("proc:inst:") + std::string(instance_id), inst_val)) {
                try {
                    const auto inst_doc = nlohmann::json::parse(inst_val);
                    model_id = inst_doc.value("model_id",
                               inst_doc.value("process_definition_id", ""));
                } catch (...) {}
            }
        }

        if (!model_id.empty()) {
            auto communities = community_detector_.loadCommunities(model_id);

            if (!communities.empty()) {
                // Sort by modularity score descending, pick top-3
                std::sort(communities.begin(), communities.end(),
                          [](const ProcessCommunity& a, const ProcessCommunity& b) {
                              return a.modularity_score > b.modularity_score;
                          });

                const size_t top_k = std::min({communities.size(), size_t{3},
                                               resource_limits_.max_result_elements});
                if (top_k < std::min(communities.size(), size_t{3})) {
                    addDegradation("max_result_elements_exceeded");
                }

                std::ostringstream ctx = {};

                for (size_t i = 0; i < top_k; ++i) {
                    if (i > 0) {
                        ctx << "\n\n";
                    }
                    ctx << communities[i].report;
                    result.community_ids_used.push_back(communities[i].community_id);
                }

                THEMIS_INFO("ProcessLightRetriever: GLOBAL retrieval for instance '{}' "
                            "using {} communities", instance_id, top_k);

                result.used_mode = RetrievalMode::GLOBAL;
                result.llm_context = ctx.str();
            }
        }

        if (result.used_mode != RetrievalMode::GLOBAL) {
            // Fallback to LOCAL when no communities are persisted
            THEMIS_WARN("ProcessLightRetriever: no persisted communities for model '{}'; "
                        "falling back to LOCAL retrieval", model_id);
        }
    }

    if (result.used_mode != RetrievalMode::GLOBAL) {
        // ── LOCAL path ───────────────────────────────────────────────────────
        ProcessRagConfig bounded_config = config;
        const size_t max_depth_as_int = static_cast<size_t>(std::numeric_limits<int>::max());
        const int max_depth = static_cast<int>(
            std::min(resource_limits_.max_traversal_depth, max_depth_as_int));
        bounded_config.max_subgraph_depth =
            std::max(0, std::min(config.max_subgraph_depth, max_depth));
        const int max_cases = static_cast<int>(
            std::min(resource_limits_.max_result_elements, max_depth_as_int));
        bounded_config.max_similar_cases =
            std::max(0, std::min(config.max_similar_cases, max_cases));

        if (config.max_subgraph_depth > bounded_config.max_subgraph_depth) {
            addDegradation("max_traversal_depth_exceeded");
        }
        if (config.max_similar_cases > bounded_config.max_similar_cases) {
            addDegradation("max_result_elements_exceeded");
        }

        const auto ctx = graph_rag_.retrieve(instance_id, query, bounded_config);
        result.llm_context = ctx.llm_prompt;

        THEMIS_INFO("ProcessLightRetriever: LOCAL retrieval for instance '{}'", instance_id);
    }

    if (result.used_mode == RetrievalMode::GLOBAL) {
        const size_t included_reports = result.community_ids_used.size();
        if (included_reports > resource_limits_.max_result_elements) {
            result.community_ids_used.resize(resource_limits_.max_result_elements);
            addDegradation("max_result_elements_exceeded");
        }
    } else {
        const std::string limited_lines =
            truncateLines(result.llm_context, resource_limits_.max_result_elements);
        if (limited_lines.size() != result.llm_context.size()) {
            addDegradation("max_result_elements_exceeded");
            result.llm_context = limited_lines;
        }
    }

    if (result.llm_context.size() > resource_limits_.max_context_bytes) {
        result.llm_context = truncateUtf8(result.llm_context, resource_limits_.max_context_bytes);
        addDegradation("max_context_size_exceeded");
    }
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        SteadyClock::now() - started_at).count();
    if (elapsed_ms >= resource_limits_.max_retrieval_time_ms) {
        addDegradation("retrieval_timeout");
    }

    result.retrieval_time_ms = elapsed_ms;
    result.context_size_bytes = result.llm_context.size();
    if (!degradation_reasons.empty()) {
        result.degraded = true;
        std::ostringstream reason;
        for (size_t i = 0; i < degradation_reasons.size(); ++i) {
            if (i > 0) {
                reason << "; ";
            }
            reason << degradation_reasons[i];
        }
        result.resource_exhaustion_reason = reason.str();
    }
    return result;
}

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── Phase 2: Stress Scenario Hardening Implementation ─────────────────────────────────────────────────────────────────────────────
 * @param[in] limits Input parameter.
 * @details Implements setResourceLimits without additional internal calls.
 */

void ProcessLightRetriever::setResourceLimits(const ResourceLimits& limits) {
    resource_limits_ = limits;
}

} // namespace process
} // namespace themis
