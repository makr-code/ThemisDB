/**
 * @file ingestion_quality_judge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB – Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_quality_judge.cpp                        ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-15                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Implementation: IngestionQualityJudge and ReIngestionController
 *
 * LLM prompts follow a fixed structure so that the score parser is robust:
 *
 *   SCORE: <float 0.0–1.0>
 *   RATIONALE: <one sentence>
 *   MISSING: (optional bullet list)
 *   - item 1
 *   - item 2
 *   UNGROUNDED: (optional bullet list)
 *   - claim 1
 *   HINTS: (optional bullet list)
 *   - suggestion
 *
 * This format is enforced in the system part of each prompt.
 */

#include "ingestion/ingestion_quality_judge.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace themis {
namespace ingestion {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// Return up to @p max_chars from @p s, appending "..." if truncated.
std::string truncate(const std::string& s, size_t max_chars) {
    if (static_cast<int>(s.size()) <= max_chars) {
      return s;
    }
    return s.substr(0, max_chars) + "...";
}

/// Join entity labels in @p ctx to a comma-separated summary string.
std::string entitySummary(const ExtractionContext& ctx, size_t max = 30) {
    std::ostringstream oss = {};
    size_t n = 0;
    for (const auto& e : ctx.entities) {
        if (n++ >= max) { oss << ", ..."; break; }
        if (n > 1) {
          oss << ", ";
        }
        if (!e.text.empty()) {
            oss << e.text;
        } else {
            oss << e.id;
        }
    }
    return oss.str();
}

/// Build a concise relation summary (subject → predicate → object).
std::string relationSummary(const ExtractionContext& ctx, size_t max = 20) {
    std::ostringstream oss = {};
    size_t n = 0;
    for (const auto& r : ctx.relations) {
        if (n++ >= max) { oss << "\n  ..."; break; }
        oss << "\n  " << r.from_id << " →[" << static_cast<int>(r.relation_type) << "]→ " << r.to_id;
    }
    return oss.str();
}

// ---- System preamble injected into every prompt -------------------------

constexpr const char* kSystemPreamble =
    "You are a neutral ingestion-quality auditor for a legal-knowledge database.\n"
    "Your job is to evaluate a text-extraction result against the original source.\n"
    "Always respond in EXACTLY this format (no markdown, no code blocks):\n"
    "SCORE: <float between 0.00 and 1.00>\n"
    "RATIONALE: <one sentence max 120 chars>\n"
    "MISSING:\n"
    "- <item> (repeat for each missing item; omit section if none)\n"
    "UNGROUNDED:\n"
    "- <claim> (repeat; omit if none)\n"
    "HINTS:\n"
    "- <suggestion> (repeat; omit if none)\n"
    "\n"
    "Scoring scale:\n"
    "  1.00 = perfect\n"
    "  0.75 = acceptable\n"
    "  0.50 = partial\n"
    "  0.25 = poor\n"
    "  0.00 = completely missing\n";

} // anonymous namespace

// ============================================================================
// IngestionQualityJudge – construction / destruction
// ============================================================================

IngestionQualityJudge::IngestionQualityJudge(
    std::shared_ptr<ITextGenerationBackend> backend,
    IngestionJudgeConfig                    config)
    : backend_(std::move(backend))
    , config_(std::move(config))
{
    if (!backend_)
        throw std::invalid_argument("IngestionQualityJudge: backend must not be null");
}

IngestionQualityJudge::~IngestionQualityJudge() = default;

// ============================================================================
// Configuration / observers
// ============================================================================

const IngestionJudgeConfig& IngestionQualityJudge::config() const noexcept {
    return config_;
}

void IngestionQualityJudge::setConfig(const IngestionJudgeConfig& cfg) {
    config_ = cfg;
}

void IngestionQualityJudge::addObserver(
    std::shared_ptr<IIngestionQualityObserver> observer)
{
    if (!observer) {
      return;
    }
    std::lock_guard<std::mutex> lk([[maybe_unused]] observer_mutex_);
    observers_.push_back([[maybe_unused]] std::move(observer));
}

void IngestionQualityJudge::removeObserver(
    const IIngestionQualityObserver* observer)
{
    if (!observer) {
      return;
    }
    std::lock_guard<std::mutex> lk([[maybe_unused]] observer_mutex_);
    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
            [observer]([[maybe_unused]] const std::shared_ptr<IIngestionQualityObserver>& sp) {
                return sp.get() == observer;
            }),
        observers_.end());
}

// ============================================================================
// Evaluation – public API
// ============================================================================

IngestionQualityReport IngestionQualityJudge::evaluate(
    const ExtractionContext& ctx) const
{
    return evaluate(ctx, ctx.raw_text);
}

IngestionQualityReport IngestionQualityJudge::evaluate(
    const ExtractionContext& ctx,
    const std::string&       source_text) const
{
    const auto t0 = std::chrono::steady_clock::now();

    IngestionQualityReport report;
    report.doc_id       = !ctx.manifest.file_id.empty() ? ctx.manifest.file_id : ctx.manifest.original_path;
    report.judge_backend = backend_->description();

    // ---- Fail-open when context is too sparse or backend unavailable ----
    const bool sparse_context =
        static_cast<int>(source_text.size()) < config_.min_text_bytes_for_eval ||
        static_cast<int>(ctx.entities.size()) < config_.min_entities_for_eval;

    if (sparse_context || !backend_->isAvailable()) {
        report.passed = true;   // fail-open; scores remain -1.0 (not evaluated)
        return report;
    }

    // ---- Completeness ----
    if (config_.evaluate_completeness) {
        const auto prompt = buildCompletenessPrompt(ctx, source_text);
        const auto resp = backend_->generate(prompt,
                                             config_.max_tokens,
                                             config_.temperature,
                                             config_.lora_adapter);
        report.completeness_score     = parseScore(resp);
        report.completeness_rationale = parseRationale(resp);
        auto missing = parseBulletList(resp, "MISSING:");
        report.missing_entities.insert(report.missing_entities.end(),
                                       missing.begin(), missing.end());
        auto hints = parseBulletList(resp, "HINTS:");
        report.improvement_hints.insert(report.improvement_hints.end(),
                                        hints.begin(), hints.end());
    }

    // ---- Groundedness ----
    if (config_.evaluate_groundedness) {
        const auto prompt = buildGroundednessPrompt(ctx, source_text);
        const auto resp = backend_->generate(prompt,
                                             config_.max_tokens,
                                             config_.temperature,
                                             config_.lora_adapter);
        report.groundedness_score     = parseScore(resp);
        report.groundedness_rationale = parseRationale(resp);
        auto ungrounded = parseBulletList(resp, "UNGROUNDED:");
        report.ungrounded_claims.insert(report.ungrounded_claims.end(),
                                        ungrounded.begin(), ungrounded.end());
        auto hints = parseBulletList(resp, "HINTS:");
        report.improvement_hints.insert(report.improvement_hints.end(),
                                        hints.begin(), hints.end());
    }

    // ---- Entity coverage ----
    if (config_.evaluate_entity_coverage) {
        const auto prompt = buildEntityCoveragePrompt(ctx, source_text);
        const auto resp = backend_->generate(prompt,
                                             config_.max_tokens,
                                             config_.temperature,
                                             config_.lora_adapter);
        report.entity_coverage_score     = parseScore(resp);
        report.entity_coverage_rationale = parseRationale(resp);
        auto missing = parseBulletList(resp, "MISSING:");
        report.missing_entities.insert(report.missing_entities.end(),
                                       missing.begin(), missing.end());
    }

    // ---- Relation coherence ----
    if (config_.evaluate_relation_coherence) {
        const auto prompt = buildRelationCoherencePrompt(ctx);
        const auto resp = backend_->generate(prompt,
                                             config_.max_tokens,
                                             config_.temperature,
                                             config_.lora_adapter);
        report.relation_coherence_score     = parseScore(resp);
        report.relation_coherence_rationale = parseRationale(resp);
        auto hints = parseBulletList(resp, "HINTS:");
        report.improvement_hints.insert(report.improvement_hints.end(),
                                        hints.begin(), hints.end());
    }

    // ---- Aggregate ----
    report.overall_score     = computeOverallScore(report);
    report.passed            = checkThresholds(report);
    report.recommended_steps = computeRecommendedSteps(report);

    const auto t1 = std::chrono::steady_clock::now();
    report.evaluation_time = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);

    // Deduplicate improvement hints
    {
        auto& hints = report.improvement_hints;
        std::sort(hints.begin(), hints.end());
        hints.erase(std::unique(hints.begin(), hints.end()), hints.end());
    }

    notifyEvaluated(report.doc_id, report);
    return report;
}

// ============================================================================
// Prompt builders
// ============================================================================

std::string IngestionQualityJudge::buildCompletenessPrompt(
    const ExtractionContext& ctx,
    const std::string&       source) const
{
    std::ostringstream oss;
    oss << kSystemPreamble << "\n"
        << "TASK: Evaluate COMPLETENESS of the following extraction.\n"
        << "Completeness = how much of the important information in the source "
           "is represented in the extracted data.\n\n"
        << "--- SOURCE TEXT (first 2000 chars) ---\n"
        << truncate(source, 2000) << "\n\n"
        << "--- EXTRACTED ENTITIES (" <<static_cast<int>(ctx.entities.size()) << " total) ---\n"
        << entitySummary(ctx) << "\n\n"
        << "--- EXTRACTED CHUNKS (" <<static_cast<int>(ctx.chunks.size()) << ") ---\n";
    for (size_t i = 0; i < std::min(ctx.chunks.size(), size_t{5}); ++i)
        oss << "Chunk " << i << ": " << truncate(ctx.chunks[i].text, 200) << "\n";
    oss << "\nRate the completeness (SCORE) and list important information "
           "NOT captured in the extraction (MISSING).";
    return oss.str();
}

std::string IngestionQualityJudge::buildGroundednessPrompt(
    const ExtractionContext& ctx,
    const std::string&       source) const
{
    std::ostringstream oss;
    oss << kSystemPreamble << "\n"
        << "TASK: Evaluate GROUNDEDNESS of the following extraction.\n"
        << "Groundedness = every entity / claim in the extraction can be traced "
           "back to a verbatim passage in the source text.\n\n"
        << "--- SOURCE TEXT (first 2000 chars) ---\n"
        << truncate(source, 2000) << "\n\n"
        << "--- EXTRACTED ENTITIES (" <<static_cast<int>(ctx.entities.size()) << ") ---\n"
        << entitySummary(ctx) << "\n\n"
        << "--- EXTRACTED RELATIONS (" <<static_cast<int>(ctx.relations.size()) << ") ---\n"
        << truncate(relationSummary(ctx), 800) << "\n\n"
        << "Rate groundedness (SCORE) and list claims NOT supported by the "
           "source text (UNGROUNDED).";
    return oss.str();
}

std::string IngestionQualityJudge::buildEntityCoveragePrompt(
    const ExtractionContext& ctx,
    const std::string&       source) const
{
    std::ostringstream oss;
    oss << kSystemPreamble << "\n"
        << "TASK: Evaluate ENTITY COVERAGE of the following extraction.\n"
        << "Entity coverage = all named entities (persons, organizations, "
           "laws, norms, locations, dates) that appear in the source are "
           "present in the extracted entity list.\n\n"
        << "--- SOURCE TEXT (first 2000 chars) ---\n"
        << truncate(source, 2000) << "\n\n"
        << "--- EXTRACTED ENTITIES (" <<static_cast<int>(ctx.entities.size()) << ") ---\n"
        << entitySummary(ctx, 50) << "\n\n"
        << "Rate entity coverage (SCORE) and list entity LABELS that appear "
           "in the source but are absent from the extraction (MISSING).";
    return oss.str();
}

std::string IngestionQualityJudge::buildRelationCoherencePrompt(
    const ExtractionContext& ctx) const
{
    std::ostringstream oss;
    oss << kSystemPreamble << "\n"
        << "TASK: Evaluate RELATION COHERENCE of the following extraction.\n"
        << "Relation coherence = the extracted subject–predicate–object triples "
           "are semantically valid (both endpoints exist as entities, the "
           "predicate is appropriate for the entity types).\n\n"
        << "--- EXTRACTED ENTITIES (" <<static_cast<int>(ctx.entities.size()) << ") ---\n"
        << entitySummary(ctx) << "\n\n"
        << "--- EXTRACTED RELATIONS (" <<static_cast<int>(ctx.relations.size()) << ") ---\n"
        << truncate(relationSummary(ctx), 1200) << "\n\n"
        << "Rate relation coherence (SCORE) and list incoherent or implausible "
           "relations as HINTS for improvement.";
    return oss.str();
}

// ============================================================================
// Response parsers
// ============================================================================

double IngestionQualityJudge::parseScore(const std::string& response) noexcept {
    // Look for "SCORE: 0.82" (case-insensitive, optional spaces).
    const std::string tag = "SCORE:";
    auto pos = response.find(tag);
    if (pos == std::string::npos) {
        // Try lowercase fallback.
        const std::string ltag = "score:";
        auto lpos = response.find(ltag);
        if (lpos == std::string::npos) {
          return -1.0;
        }
        pos = lpos;
    }
    pos += tag.size();
    while (pos < response.size() && (response[pos] == ' ' || response[pos] == '\t'))
        ++pos;
    // Read digits / dot until whitespace or newline.
    std::string num = {};
    while (pos < response.size() &&
           (std::isdigit(static_cast<unsigned char>(response[pos])) ||
            response[pos] == '.'))
    {
        num += response[pos++];
    }
    if (num.empty()) {
      return -1.0;
    }
    try {
        double v = std::stod(num);
        if (v < 0.0) {
          v = 0.0;
        }
        if (v > 1.0) {
          v = 1.0;
        }
        return v;
    } catch (...) {
        return -1.0;
    }
}

std::string IngestionQualityJudge::parseRationale(
    const std::string& response) noexcept
{
    const std::string tag = "RATIONALE:";
    auto pos = response.find(tag);
    if (pos == std::string::npos) return {};
    pos += tag.size();
    while (pos < response.size() && response[pos] == ' ') {
      ++pos;
    }
    auto end = response.find('\n', pos);
    if (end == std::string::npos) {
      end = response.size();
    }
    std::string result = response.substr(pos, end - pos);
    // Truncate to 200 chars for safety.
    if (static_cast<int>(result.size()) > 200) {
      result.resize(200);
    }
    return result;
}

std::vector<std::string> IngestionQualityJudge::parseBulletList(
    const std::string& response,
    const std::string& section_tag) noexcept
{
    std::vector<std::string> items;
    auto pos = response.find(section_tag);
    if (pos == std::string::npos) {
        // Case-insensitive fallback: just skip, not critical.
        return items;
    }
    pos += section_tag.size();
    // Read lines until we hit the next section header (all-caps word followed
    // by ':') or end of string.
    std::istringstream ss(response.substr(pos));
    std::string line = {};
    while (std::getline(ss, line)) {
        // Strip leading whitespace.
        auto start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) {
          continue;
        }
        line = line.substr(start);
        if (line.empty()) {
          continue;
        }
        // New section header → stop.
        if (static_cast<int>(line.size()) > 2 && std::isupper(static_cast<unsigned char>(line[0]))
            && line.find(':') != std::string::npos
            && line.find(':') < 20)
        {
            break;
        }
        // Bullet marker.
        if (line[0] == '-' || line[0] == '*' || line[0] == '\xe2' /* UTF-8 bullet */) {
            auto content_start = line.find_first_not_of("-* \t", 1);
            if (content_start == std::string::npos) {
              continue;
            }
            items.push_back(line.substr(content_start));
        }
    }
    return items;
}

// ============================================================================
// Aggregation
// ============================================================================

double IngestionQualityJudge::computeOverallScore(
    const IngestionQualityReport& r) const noexcept
{
    double weighted_sum = 0.0;
    double weight_sum   = 0.0;

    auto add = [&](double score, double weight) {
        if (score < 0.0) return; // not evaluated → skip
        weighted_sum += score * weight;
        weight_sum   += weight;
    };

    add(r.completeness_score,       config_.completeness_weight);
    add(r.groundedness_score,       config_.groundedness_weight);
    add(r.entity_coverage_score,    config_.entity_coverage_weight);
    add(r.relation_coherence_score, config_.relation_coherence_weight);

    if (weight_sum < 1e-9) return 1.0; // nothing evaluated → pass
    return weighted_sum / weight_sum;
}

bool IngestionQualityJudge::checkThresholds(
    const IngestionQualityReport& r) const noexcept
{
    // Overall threshold.
    if (r.overall_score >= 0.0 &&
        r.overall_score < config_.overall_threshold) return false;

    // Per-dimension thresholds (only when dimension was evaluated).
    if (r.completeness_score >= 0.0 &&
        r.completeness_score < config_.completeness_threshold) return false;
    if (r.groundedness_score >= 0.0 &&
        r.groundedness_score < config_.groundedness_threshold) return false;
    if (r.entity_coverage_score >= 0.0 &&
        r.entity_coverage_score < config_.entity_coverage_threshold) return false;
    if (r.relation_coherence_score >= 0.0 &&
        r.relation_coherence_score < config_.relation_coherence_threshold) return false;

    return true;
}

std::vector<std::string> IngestionQualityJudge::computeRecommendedSteps(
    const IngestionQualityReport& r) const
{
    std::vector<std::string> steps;

    if (r.completeness_score >= 0.0 &&
        r.completeness_score < config_.completeness_threshold) {
        // More thorough text chunking and LLM extraction help completeness.
        steps.emplace_back("builtin.chunk_text");
        steps.emplace_back("builtin.llm_extract");
    }
    if (r.groundedness_score >= 0.0 &&
        r.groundedness_score < config_.groundedness_threshold) {
        // Re-run LLM extraction with a lower hallucination risk setting.
        steps.emplace_back("builtin.llm_extract");
    }
    if (r.entity_coverage_score >= 0.0 &&
        r.entity_coverage_score < config_.entity_coverage_threshold) {
        // Re-run NER and LLM extraction to catch missed entities.
        steps.emplace_back("builtin.ner_de");
        steps.emplace_back("builtin.llm_extract");
    }
    if (r.relation_coherence_score >= 0.0 &&
        r.relation_coherence_score < config_.relation_coherence_threshold) {
        // Re-run the reference extraction and base entity assembler.
        steps.emplace_back("builtin.reference_extract");
        steps.emplace_back("builtin.base_entity_assembler");
    }

    // Deduplicate, preserving insertion order.
    std::vector<std::string> unique_steps = {};

    for (const auto& s : steps) {
        if (std::find(unique_steps.begin(), unique_steps.end(), s)
            == unique_steps.end())
        {
            unique_steps.push_back(s);
        }
    }
    return unique_steps;
}

// ============================================================================
// Observer dispatch
// ============================================================================

void IngestionQualityJudge::notifyEvaluated(
    const std::string&            doc_id,
    const IngestionQualityReport& report) const noexcept
{
    std::vector<std::shared_ptr<IIngestionQualityObserver>> snapshot;
    {
        std::lock_guard<std::mutex> lk([[maybe_unused]] observer_mutex_);
        snapshot = observers_;
    }
    for (const auto& obs : snapshot) {
        try { obs->onQualityEvaluated(doc_id, report); } catch (...) {}
    }
}

// ============================================================================
// ReIngestionController – construction / destruction
// ============================================================================

ReIngestionController::ReIngestionController(
    std::shared_ptr<WorkflowEngine>        engine,
    std::shared_ptr<IngestionQualityJudge> judge)
    : engine_(std::move(engine))
    , judge_(std::move(judge))
{
    if (!engine_)
        throw std::invalid_argument("ReIngestionController: engine must not be null");
    if (!judge_)
        throw std::invalid_argument("ReIngestionController: judge must not be null");
}

ReIngestionController::~ReIngestionController() = default;

// ============================================================================
// Configuration / observers
// ============================================================================

void ReIngestionController::setReIngestionProfile(
    const std::string& profile_name)
{
    reingestion_profile_ = profile_name;
}

void ReIngestionController::addObserver(
    std::shared_ptr<IIngestionQualityObserver> observer)
{
    if (!observer) {
      return;
    }
    observers_.push_back([[maybe_unused]] std::move(observer));
}

void ReIngestionController::removeObserver(
    const IIngestionQualityObserver* observer)
{
    if (!observer) {
      return;
    }
    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
            [observer]([[maybe_unused]] const std::shared_ptr<IIngestionQualityObserver>& sp) {
                return sp.get() == observer;
            }),
        observers_.end());
}

// ============================================================================
// process() — the quality-controlled ingestion loop
// ============================================================================

ReIngestionController::RunResult ReIngestionController::process(
    const FileManifest& manifest)
{
    RunResult result;
    result.quality_met = false;

    const int max_attempts = judge_->config().max_reingestion_attempts + 1;
    // attempt 0 = first pass (not a re-ingestion)
    // attempts 1..max_reingestion_attempts = re-ingestion passes

    double best_score = -1.0;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        // ---- Run the workflow engine ----
        const std::string profile = (attempt > 0 && !reingestion_profile_.empty())
            ? reingestion_profile_ : "";

        ExtractionContext ctx;
        ctx.manifest = manifest;

        // Execute workflow. On failure continue to evaluate what we got.
        if (!profile.empty()) {
            (void)engine_->executeWithProfile(profile, ctx);
        } else {
            (void)engine_->execute(ctx);
        }

        // ---- Evaluate quality ----
        IngestionQualityReport report = judge_->evaluate(ctx);
        report.attempt = attempt;

        result.attempts = attempt + 1;
        result.history.push_back(report);

        // Track best context.
        if (report.overall_score > best_score || best_score < 0.0) {
            best_score = report.overall_score;
            result.best_context = ctx;
            result.final_report = report;
        }

        if (report.passed) {
            result.quality_met  = true;
            result.final_report = report;
            result.best_context = ctx;
            break;
        }

        // ---- Should we re-ingest? ----
        if (attempt >= max_attempts - 1) break; // exhausted

        // Notify observers before triggering re-ingestion.
        std::vector<std::string> reasons;
        const auto& cfg = judge_->config();
        if (report.completeness_score >= 0.0 &&
            report.completeness_score < cfg.completeness_threshold)
            reasons.emplace_back("completeness");
        if (report.groundedness_score >= 0.0 &&
            report.groundedness_score < cfg.groundedness_threshold)
            reasons.emplace_back("groundedness");
        if (report.entity_coverage_score >= 0.0 &&
            report.entity_coverage_score < cfg.entity_coverage_threshold)
            reasons.emplace_back("entity_coverage");
        if (report.relation_coherence_score >= 0.0 &&
            report.relation_coherence_score < cfg.relation_coherence_threshold)
            reasons.emplace_back("relation_coherence");

        const std::string doc_id = !manifest.file_id.empty() ? manifest.file_id : manifest.original_path;
        notifyTriggered(doc_id, attempt + 1, reasons);

        // Determine improvement for the upcoming pass notification.
        bool improved_over_prev = false;
        if (static_cast<int>(result.history.size()) >= 2) {
            improved_over_prev = isImprovement(
                result.history[result.history.size() - 2], report);
        }
        notifyComplete(doc_id, attempt, improved_over_prev);
    }

    return result;
}

// ============================================================================
// Helpers
// ============================================================================

bool ReIngestionController::isImprovement(
    const IngestionQualityReport& a,
    const IngestionQualityReport& b) noexcept
{
    // Treat -1 (not evaluated) as 0 for comparison purposes.
    const double sa = (a.overall_score >= 0.0) ? a.overall_score : 0.0;
    const double sb = (b.overall_score >= 0.0) ? b.overall_score : 0.0;
    return sb > sa + 0.01; // require at least 1 pp improvement
}

void ReIngestionController::notifyTriggered(
    const std::string&              doc_id,
    int                             attempt,
    const std::vector<std::string>& reasons) noexcept
{
    for ([[maybe_unused]] const auto& obs : observers_) {
        try { obs->onReIngestionTriggered(doc_id, attempt, reasons); } catch (...) {}
    }
    // Forward to judge observers as well.
}

void ReIngestionController::notifyComplete(
    const std::string& doc_id,
    int                attempt,
    bool               improved) noexcept
{
    for ([[maybe_unused]] const auto& obs : observers_) {
        try { obs->onReIngestionComplete(doc_id, attempt, improved); } catch (...) {}
    }
}

} // namespace ingestion
} // namespace themis


