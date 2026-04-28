/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_light_retriever.cpp
 * Module:  src/process/
 * Purpose: Dual-mode LOCAL/GLOBAL retrieval following the LightRAG approach
 *          (Guo et al., 2024, arXiv:2410.05779). P5 implementation.
 */

#include "process/process_light_retriever.h"
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
                    RetrievalMode::GLOBAL,
                    ctx.str(),
                    std::move(used_ids),
                    std::string(instance_id)
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
        RetrievalMode::LOCAL,
        ctx.llm_prompt,
        {},
        std::string(instance_id)
    };
}

} // namespace process
} // namespace themis
