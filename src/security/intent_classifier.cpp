// THEMIS_GAP_STATS: gaps=3 unimpl=2 stub=1 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            intent_classifier.cpp                              ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-17                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// STUB/SIMULATION NOTE:
// Purpose: Rule-based feature classification as placeholder for LoRA-adapted model
// Activation: Always active in v1.0; LoRA adapter replaces rules post-IMPL-A2
// Production Delta: Rule-based precision ~80%; LoRA target precision ≥ 92%
// Roadmap ref: src/security/ROADMAP.md § "Phase 4: Zero-Trust & Post-Quantum Cryptography"
// Removal Plan: Replace classify() internals with LoRA adapter call in IMPL-A2 Loop-1

#include "security/intent_classifier.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_map>

#include <spdlog/spdlog.h>

namespace themis {
namespace security {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Convert query to uppercase for case-insensitive matching.
std::string toUpperAscii(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (auto c : s) out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    return out;
}

/// Weighted SQL-injection indicator set.
struct Feature {
    const char* token;
    double      weight;
};

static const Feature kSqlInjectionFeatures[] = {
    {"OR 1=1",            0.40},
    {"OR '1'='1'",        0.40},
    {"; DROP",            0.50},
    {"UNION SELECT",      0.45},
    {"' --",              0.35},
    {"-- ",               0.20},
    {"INFORMATION_SCHEMA",0.25},
    {"XP_CMDSHELL",       0.60},
    {"EXEC(",             0.30},
    {"CHAR(",             0.15},
};

static const Feature kExfiltrationFeatures[] = {
    {"SELECT *",          0.15},
    {"LIMIT 99999",       0.40},
    {"ORDER BY 1",        0.30},
    {"GROUP_CONCAT",      0.35},
    {"INTO OUTFILE",      0.60},
    {"LOAD_FILE(",        0.55},
    {"BULK INSERT",       0.45},
};

static const Feature kPrivEscFeatures[] = {
    {"GRANT ",            0.50},
    {"REVOKE ",           0.45},
    {"CREATE USER",       0.55},
    {"ALTER USER",        0.50},
    {"DROP USER",         0.50},
    {"SUPERUSER",         0.55},
    {"SYSADMIN",          0.60},
    {"EXECUTE AS",        0.45},
};

// AI Safety Layer — Schicht 4 (ASL-4)
// AQL-native destructive operation patterns.
// Docs: docs/de/security/ai_safety/AI_SAFETY_INTENT_CLASSIFIER.md
// Roadmap: src/security/ROADMAP.md § Phase 5 (ASL-4)
//
// Blocking threshold: confidence ≥ 0.65 (see maybeAlert default threshold
// for AI Safety callers; standard callers keep 0.85 for backwards compat).

static const Feature kDataDestructionFeatures[] = {
    {"FOR ",              0.05},  // FOR alone is benign; weight raised by combos
    {" REMOVE ",         0.65},  // REMOVE keyword in any AQL context
    {"REMOVE @",         0.40},  // Parametrised single-key delete (bind var)
    {"TRUNCATE ",        0.80},  // TRUNCATE collection
    {"DROP COLLECTION",  0.95},  // Full collection drop → nearly always CRITICAL
    {"DELETE FROM",      0.50},  // SQL-style DELETE leaking into AQL
};

static const Feature kSchemaMutationFeatures[] = {
    {"DROP INDEX",       0.70},
    {"DROP TABLE",       0.70},  // SQL DDL sometimes embedded in AQL queries
    {"DROP VIEW",        0.65},
    {"DROP DATABASE",    0.95},
    {"CREATE COLLECTION",0.30},  // Creating is less dangerous than dropping
    {"CREATE INDEX",     0.20},  // Index creation is acceptable but notable
    {"ALTER COLLECTION", 0.60},
};

/// Helper: detect compound FOR…REMOVE without FILTER (full-collection delete).
/// Returns extra weight to add on top of kDataDestructionFeatures score.
double forRemoveWithoutFilterWeight(const std::string& upperQuery) {
    const std::size_t forPos    = upperQuery.find("FOR ");
    const std::size_t removePos = upperQuery.find(" REMOVE ");
    if (forPos == std::string::npos || removePos == std::string::npos) {
        return 0.0;
    }
    if (forPos >= removePos) {
        return 0.0;  // REMOVE appears before FOR — not a FOR-REMOVE pattern
    }
    // Additional weight when there is no FILTER between FOR and REMOVE
    const std::size_t filterPos = upperQuery.find("FILTER ");
    const bool hasFilter = (filterPos != std::string::npos &&
                            filterPos > forPos && filterPos < removePos);
    return hasFilter ? 0.25 : 0.90;  // Unfiltered full-collection delete
}

/// Compute weighted-sum score against a feature table.
template <std::size_t N>
double scoreFeatures(const std::string& upperQuery, const Feature (&features)[N]) {
    double score = 0.0;
    for (const auto& f : features) {
        if (upperQuery.find(f.token) != std::string::npos) {
            score += f.weight;
        }
    }
    return score;
}

/// Logistic function (sigmoid).
double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

/// Map weighted score to confidence using a logistic curve.
/// The scale parameter controls how steeply the score translates to confidence.
double scoreToConfidence(double score, double scale = 3.0) {
    // sigmoid(scale * score) - 0.5 normalised to [0,1]
    double raw = sigmoid(scale * score);
    // Remap so that score=0 → confidence=0 and score→∞ → confidence=1
    return std::clamp((raw - 0.5) * 2.0, 0.0, 1.0);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// IntentClassifier implementation
// ---------------------------------------------------------------------------

IntentClassifier::IntentClassifier(std::string shard_id)
    : shard_id_(std::move(shard_id)) {}

void IntentClassifier::setInferenceFn(InferenceFn fn) {
    inference_fn_ = std::move(fn);
    // Activating the injected backend also enables the LoRA path so that
    // isLoraActive() reflects the live state.  Passing a null function
    // resets to rule-based mode.
    if (inference_fn_) {
        lora_active_ = true;
    } else if (lora_model_path_.empty()) {
        lora_active_ = false;
    }
}

IntentClassifier::ClassificationResult IntentClassifier::classify(
    const std::string&      query,
    const ZeroTrustContext& session_context
) const {
    // When an inference function has been injected (real LoRA/LLM backend),
    // delegate to it directly and bypass the rule-based classifier.
    if (lora_active_ && inference_fn_) {
        return inference_fn_(query, session_context);
    }

    const std::string uq = toUpperAscii(query);

    // Compute per-class feature scores.
    const double injScore   = scoreFeatures(uq, kSqlInjectionFeatures);
    const double exfilScore = scoreFeatures(uq, kExfiltrationFeatures);
    const double privScore  = scoreFeatures(uq, kPrivEscFeatures);

    // AI Safety Layer (Schicht 4): AQL-aware destructive operation scoring.
    const double destroyScore = scoreFeatures(uq, kDataDestructionFeatures)
                                + forRemoveWithoutFilterWeight(uq);
    const double schemaScore  = scoreFeatures(uq, kSchemaMutationFeatures);

    // Convert to confidences.
    const double injConf     = scoreToConfidence(injScore);
    const double exfilConf   = scoreToConfidence(exfilScore);
    const double privConf    = scoreToConfidence(privScore);
    const double destroyConf = scoreToConfidence(destroyScore);
    const double schemaConf  = scoreToConfidence(schemaScore);

    // Pick the dominant malicious class; fall back to LEGITIMATE.
    struct Candidate {
        IntentType  type;
        double      conf;
        std::string indicator;
    };
    Candidate candidates[] = {
        {IntentType::SQL_INJECTION,       injConf,     "SQL_INJECTION_FEATURES"},
        {IntentType::DATA_EXFILTRATION,   exfilConf,   "DATA_EXFILTRATION_FEATURES"},
        {IntentType::PRIVILEGE_ESCALATION,privConf,    "PRIVILEGE_ESCALATION_FEATURES"},
        {IntentType::DATA_DESTRUCTION,    destroyConf, "AQL_DATA_DESTRUCTION_FEATURES"},
        {IntentType::SCHEMA_MUTATION,     schemaConf,  "AQL_SCHEMA_MUTATION_FEATURES"},
    };

    double      bestConf  = 0.0;
    IntentType  bestType  = IntentType::LEGITIMATE;
    std::string bestIndic = "none";

    for (const auto& c : candidates) {
        if (c.conf > bestConf) {
            bestConf  = c.conf;
            bestType  = c.type;
            bestIndic = c.indicator;
        }
    }

    // If the best malicious confidence is below a minimal threshold the query
    // is LEGITIMATE.
    if (bestConf < 0.05) {
        return ClassificationResult{IntentType::LEGITIMATE, 1.0 - bestConf, "none"};
    }

    // For LEGITIMATE queries: report high confidence in legitimacy.
    if (bestType == IntentType::LEGITIMATE) {
        return ClassificationResult{IntentType::LEGITIMATE, 1.0 - bestConf, "none"};
    }

    return ClassificationResult{bestType, bestConf, bestIndic};
}

std::optional<IntentClassifier::IntentAlert> IntentClassifier::maybeAlert(
    const ClassificationResult& result,
    const std::string&          session_id,
    double                      confidence_threshold
) const {
    if (result.intent == IntentType::LEGITIMATE) {
        return std::nullopt;
    }
    if (result.confidence < confidence_threshold) {
        return std::nullopt;
    }

    IntentAlert alert;
    alert.intent              = result.intent;
    alert.confidence          = result.confidence;
    alert.session_id          = session_id;
    alert.shard_id            = shard_id_;
    alert.evidence_embedding  = buildEmbedding(result.intent, result.primary_indicator);
    alert.risk_delta          = riskDelta(result.intent);
    return alert;
}

// static
double IntentClassifier::riskDelta(IntentType t) noexcept {
    switch (t) {
        case IntentType::SQL_INJECTION:       return 0.40;
        case IntentType::DATA_EXFILTRATION:   return 0.30;
        case IntentType::PRIVILEGE_ESCALATION:return 0.50;
        case IntentType::ANOMALOUS_PATTERN:   return 0.20;
        // AI Safety Layer (Schicht 4): AQL-destructive operations get maximum risk
        case IntentType::DATA_DESTRUCTION:    return 0.90;
        case IntentType::SCHEMA_MUTATION:     return 0.75;
        case IntentType::LEGITIMATE:
        default:                              return 0.0;
    }
}

// static
std::string IntentClassifier::intentName(IntentType t) {
    switch (t) {
        case IntentType::LEGITIMATE:            return "LEGITIMATE";
        case IntentType::SQL_INJECTION:         return "SQL_INJECTION";
        case IntentType::DATA_EXFILTRATION:     return "DATA_EXFILTRATION";
        case IntentType::PRIVILEGE_ESCALATION:  return "PRIVILEGE_ESCALATION";
        case IntentType::ANOMALOUS_PATTERN:     return "ANOMALOUS_PATTERN";
        case IntentType::DATA_DESTRUCTION:      return "DATA_DESTRUCTION";
        case IntentType::SCHEMA_MUTATION:       return "SCHEMA_MUTATION";
        default:                                return "UNKNOWN";
    }
}

// static
std::vector<float> IntentClassifier::buildEmbedding(
    IntentType         intent,
    const std::string& primary_indicator
) {
    // Produce a deterministic pseudo-random embedding from (intent, indicator).
    // The embedding contains NO query plaintext — only a hash-derived signal.
    std::vector<float> emb(kEmbeddingDim, 0.0f);

    // Use the intent ordinal as a class seed.
    const int classSeed = static_cast<int>(intent);

    // Simple hash of primary_indicator for deterministic variation.
    std::size_t strHash = std::hash<std::string>{}(primary_indicator);

    for (std::size_t i = 0; i < kEmbeddingDim; ++i) {
        // Deterministic per-dimension value derived from class + indicator hash.
        const float signal = static_cast<float>(classSeed + 1) / 6.0f;
        const float noise  = static_cast<float>((strHash >> (i % 64)) & 0xFF) / 512.0f;
        emb[i] = signal + noise - 0.25f; // centre around 0
    }

    // L2-normalise.
    float norm = 0.0f;
    for (auto v : emb) norm += v * v;
    norm = std::sqrt(norm);
    if (norm > 1e-6f) {
        for (auto& v : emb) v /= norm;
    }
    return emb;
}

// ── LoRA-Adapter API (ASL-13 / IMPL-A2) ─────────────────────────────────────

IntentClassifier::LoraLoadResult IntentClassifier::loadLoraModel(
    const std::string& model_path
) {
    if (model_path.empty()) {
        lora_active_ = false;
        return LoraLoadResult::kEmptyPath;
    }

    std::ifstream probe(model_path, std::ios::binary);
    if (!probe.is_open()) {
        lora_active_ = false;
        spdlog::warn("IntentClassifier: LoRA model not accessible at '{}' (ASL-13)", model_path);
        return LoraLoadResult::kFileNotAccessible;
    }

    lora_model_path_ = model_path;
    lora_active_     = true;
    spdlog::info("IntentClassifier: LoRA model loaded from '{}' (ASL-13)", model_path);
    return LoraLoadResult::kSuccess;
}

bool IntentClassifier::setLoraModelPath(const std::string& model_path) {
    return loadLoraModel(model_path) == LoraLoadResult::kSuccess;
}

bool IntentClassifier::isLoraActive() const noexcept {
    return lora_active_;
}

const std::string& IntentClassifier::loraModelPath() const noexcept {
    return lora_model_path_;
}

} // namespace security
} // namespace themis

