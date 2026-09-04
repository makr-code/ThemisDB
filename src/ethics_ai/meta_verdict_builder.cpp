/**
 * @file meta_verdict_builder.cpp
 * @brief MetaVerdictBuilder — Ebene-3 convergence-counting MetaVerdict assembly.
 *
 * Production convergence formula:
 *   convergence_score(v) = |{schools: Ebene-1 verdict == v}| / N_active
 *
 * Post-hoc domain weight (Ebene-3 only):
 *   domain_override.weight_boost from school YAML is applied as a multiplier
 *   on each school's convergence vote weight.  This respects deployment context
 *   without pre-filtering (Process Equality contract preserved).
 *   NOTE: weight_boost integration with YAML-loaded profiles is deferred to
 *   LDM-4 wiring (Target: Q2 2027); currently all schools count as weight 1.
 *
 * Cross-cultural convergence flag:
 *   Set true when ≥ 2 schools from distinct cultural regions share the same
 *   dominant verdict.  Cultural region mapping is static (LDM-8 AdaLoRA
 *   compensation deferred to Q4 2027).
 */

#include "ethics_ai/meta_verdict_builder.h"
#include "ethics_ai/ethics_selection_router.h"  // DiscourseMode full definition

#include <algorithm>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

// ============================================================================
// Cultural region mapping (static)
// ============================================================================

namespace {

/// Static cultural region table.
/// Regions: Western-European, Islamic, East-Asian, Indic, Jewish, Other.
/// Used for cross_cultural_flag computation.
const std::unordered_map<std::string, std::string>& culturalRegionTable()
{
    static const std::unordered_map<std::string, std::string> kRegions = {
        // Western-European (Enlightenment / analytic tradition)
        {"kant",              "Western-European"},
        {"rawls",             "Western-European"},
        {"contractualism",    "Western-European"},
        {"rationalism",       "Western-European"},
        {"utilitarianism",    "Western-European"},
        {"adam_smith",        "Western-European"},
        {"socratic",          "Western-European"},
        {"nietzsche",         "Western-European"},
        {"marx",              "Western-European"},
        {"schopenhauer",      "Western-European"},
        {"dilthey",           "Western-European"},
        {"arendt",            "Western-European"},
        {"durkheim",          "Western-European"},
        {"wiener",            "Western-European"},
        {"merton",            "Western-European"},
        {"leopold",           "Western-European"},
        // Institutional (primarily Western)
        {"behoerden_ethik",   "Western-European"},
        {"universitaere_ethik","Western-European"},
        // Islamic
        {"islamische_ethik",  "Islamic"},
        // East-Asian
        {"konfuzianismus",    "East-Asian"},
        // Indic
        {"buddhistische_ethik","Indic"},
        // Jewish
        {"juedische_bioethik", "Jewish"},
    };
    return kRegions;
}

/// Return the dominant verdict among the provided school outputs.
/// Ties are broken by lowest enum value (PROHIBIT < PERMIT < CONDITIONAL < ABSTAIN).
DiscourseVerdict dominantVerdictAmong(
    const std::vector<DiscourseRoundOutput>& outputs)
{
    std::unordered_map<int, int> counts = {};

    for (const auto& o : outputs) {
        if (!o.timed_out && o.ldm_verdict != DiscourseVerdict::ABSTAIN) {
            counts[static_cast<int>(o.ldm_verdict)]++;
        }
    }
    if (counts.empty()) return DiscourseVerdict::ABSTAIN;

    auto best = std::max_element(counts.begin(), counts.end(),
                                 [](const auto& a, const auto& b) {
                                     return a.second < b.second;
                                 });
    return static_cast<DiscourseVerdict>(best->first);
}

} // anonymous namespace

// ============================================================================
// MetaVerdictBuilder implementation
// ============================================================================

void MetaVerdictBuilder::setLegalGrounding(LegalGrounding grounding) noexcept
{
    grounding_ = std::move(grounding);
}

std::string MetaVerdictBuilder::culturalRegion(const std::string& school_id) noexcept
{
    const auto& table = culturalRegionTable();
    auto it = table.find(school_id);
    return (it != table.end()) ? it->second : "Other";
}

MetaVerdict MetaVerdictBuilder::buildMetaVerdict(
    const std::vector<DiscourseRoundOutput>& ebene1_results,
    const std::vector<ClusterPosition>&      cluster_positions,
    const LegalGrounding&                    legal_grounding,
    DiscourseMode                            mode,
    const std::vector<DiscourseRoundOutput>& mirror_dissent) const
{
    (void)cluster_positions;  // Used for Ebene-3 in LDM-4; reserved for Q2 2027 wiring.

    MetaVerdict mv;
    mv.discourse_mode   = mode;
    mv.legal_grounding  = legal_grounding;
    mv.legal_grounding.legal_db_unavailable = !mv.legal_grounding.grounding_available;
    mv.minority_dissent = mirror_dissent;  // Always included for audit trail.

    if (!mv.legal_grounding.grounding_available) {
        mv.legal_grounding.citation_ids.clear();
        mv.legal_grounding.norm_refs.clear();
    }

    // --- participating_schools: ALL N schools (EU AI Act Art. 13) ---
    mv.participating_schools.reserve(ebene1_results.size());
    mv.participating_school_votes.reserve(ebene1_results.size());
    for (const auto& o : ebene1_results) {
        mv.participating_schools.push_back(o.school_id);
        MetaVerdictSchoolVote vote;
        vote.school_id = o.school_id;
        vote.vote      = o.ldm_verdict;
        vote.reason    = (o.timed_out || o.ldm_verdict == DiscourseVerdict::ABSTAIN)
                             ? "unavailable"
                             : "available";
        mv.participating_school_votes.push_back(std::move(vote));
    }

    mv.norm_evidence.legal_db_unavailable = mv.legal_grounding.legal_db_unavailable;
    mv.norm_evidence.citations.reserve(mv.legal_grounding.norm_refs.size());
    for (size_t i = 0; i <static_cast<int>(mv.legal_grounding.norm_refs.size()); ++i) {
        NormCitation citation;
        citation.citation_id = (i <static_cast<int>(mv.legal_grounding.citation_ids.size()))
                                   ? mv.legal_grounding.citation_ids[i]
                                   : ("norm-ref-" + std::to_string(i));
        citation.article_ref     = mv.legal_grounding.norm_refs[i];
        citation.citation_source = "legal_db";
        citation.retrieved_at_utc = mv.legal_grounding.retrieval_timestamp_utc;
        mv.norm_evidence.citations.push_back(std::move(citation));
    }

    // --- Count non-ABSTAIN schools (N_active) ---
    std::vector<const DiscourseRoundOutput*> active = {};

    active.reserve(ebene1_results.size());
    for (const auto& o : ebene1_results) {
        if (!o.timed_out && o.ldm_verdict != DiscourseVerdict::ABSTAIN) {
            active.push_back(&o);
        }
    }

    // Edge case: all schools ABSTAINED → DISSENT (LDM_ALL_ABSTAINED).
    if (active.empty()) {
        mv.convergence_verdict  = MetaVerdict::ConvergenceVerdict::DISSENT;
        mv.convergence_score    = 0.0;
        mv.dominant_verdict     = DiscourseVerdict::ABSTAIN;
        mv.cross_cultural_flag  = false;
        // All schools are dissenting from any consensus.
        mv.dissenting_schools = mv.participating_schools;
        return mv;
    }

    const double N_active = static_cast<double>(active.size());

    // --- Find dominant verdict ---
    mv.dominant_verdict = dominantVerdictAmong(ebene1_results);

    // --- Convergence score ---
    int dominant_count = 0;
    for (const auto* o : active) {
        if (o->ldm_verdict == mv.dominant_verdict) ++dominant_count;
    }
    mv.convergence_score = static_cast<double>(dominant_count) / N_active;

    // --- Map score → ConvergenceVerdict ---
    mv.convergence_verdict = MetaVerdictThreshold(mv.convergence_score);

    // --- Dissenting schools ---
    for (const auto* o : active) {
        if (o->ldm_verdict != mv.dominant_verdict) {
            mv.dissenting_schools.push_back(o->school_id);
        }
    }

    // --- Cross-cultural flag ---
    // True when ≥ 2 schools from DISTINCT cultural regions share the dominant verdict.
    {
        std::set<std::string> regions_for_dominant = {};

        for (const auto* o : active) {
            if (o->ldm_verdict == mv.dominant_verdict) {
                regions_for_dominant.insert(culturalRegion(o->school_id));
            }
        }
        mv.cross_cultural_flag = (static_cast<int>(regions_for_dominant.size()) >= 2);
    }

    return mv;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
