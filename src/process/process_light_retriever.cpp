/**
 * @file process_light_retriever.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
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
#include <sstream>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// Constants – global retrieval keywords (case-insensitive matching)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Keywords that trigger GLOBAL (community-based) retrieval.
constexpr const char* kGlobalKeywords[] = {
    "gesamte", "überblick", "alle", "prozess", "ablauf", "workflow",
    "beschreibe", "erklär", "summary", "overview"
};

/// Convert a string to lower-case (ASCII + common German umlauts normalised
/// by simple lower-case mapping – sufficient for keyword matching).
std::string toLower(std::string_view sv) {
    std::string result;
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
    // Resolve effective mode
    const RetrievalMode effective = (mode == RetrievalMode::AUTO)
                                  ? classifyQuery(query)
                                  : mode;

    if (effective == RetrievalMode::GLOBAL) {
        // ── GLOBAL path ──────────────────────────────────────────────────────
        // Resolve model_id from the instance record stored under proc:inst:<id>
        std::string model_id;
        {
            std::string inst_val;
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

                const int top_k = std::min(static_cast<int>(communities.size()), 3);
                std::ostringstream ctx;
                std::vector<std::string> used_ids;
                for (int i = 0; i < top_k; ++i) {
                    if (i > 0) ctx << "\n\n";
                    ctx << communities[i].report;
                    used_ids.push_back(communities[i].community_id);
                }

                THEMIS_INFO("ProcessLightRetriever: GLOBAL retrieval for instance '{}' "
                            "using {} communities", instance_id, top_k);

                return LightRetrievalResult{
                    .used_mode = RetrievalMode::GLOBAL,
                    .llm_context = ctx.str(),
                    .community_ids_used = std::move(used_ids),
                    .instance_id_used = std::string(instance_id)
                };
            }
        }

        // Fallback to LOCAL when no communities are persisted
        THEMIS_WARN("ProcessLightRetriever: no persisted communities for model '{}'; "
                    "falling back to LOCAL retrieval", model_id);
    }

    // ── LOCAL path ───────────────────────────────────────────────────────────
    const auto ctx = graph_rag_.retrieve(instance_id, query, config);

    THEMIS_INFO("ProcessLightRetriever: LOCAL retrieval for instance '{}'", instance_id);

    return LightRetrievalResult{
        .used_mode = RetrievalMode::LOCAL,
        .llm_context = ctx.llm_prompt,
        .community_ids_used = {},
        .instance_id_used = std::string(instance_id)
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 2: Stress Scenario Hardening Implementation
// ─────────────────────────────────────────────────────────────────────────────

void ProcessLightRetriever::setResourceLimits(const ResourceLimits& limits) {
    resource_limits_ = limits;
}

bool ProcessLightRetriever::isWithinTimeoutBudget(int64_t start_time_ms) const {
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return (now_ms - start_time_ms) < resource_limits_.max_retrieval_time_ms;
}

bool ProcessLightRetriever::isWithinSizeBudget(size_t current_size_bytes) const {
    return current_size_bytes < resource_limits_.max_context_bytes;
}

bool ProcessLightRetriever::isWithinDepthBudget(size_t current_depth) const {
    return current_depth <= resource_limits_.max_traversal_depth;
}

LightRetrievalResult ProcessLightRetriever::createDegradedResult(
    std::string_view reason) const
{
    return LightRetrievalResult{
        RetrievalMode::LOCAL,
        "(retrieval interrupted due to resource constraints)",
        {},
        "",
        0,  // retrieval_time_ms
        0,  // context_size_bytes
        true,  // degraded
        std::string(reason)  // resource_exhaustion_reason
    };
}

} // namespace process
} // namespace themis


