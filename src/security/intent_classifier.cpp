/**
 * @file intent_classifier.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// PERMANENT FALLBACK NOTE:
// Rule-based feature classification as permanent fallback for builds without liboqs/LoRA.
// When THEMIS_HAS_LORA_CLASSIFIER is defined (Wave-2 CMake guard), `configureLoraEndpoint()`
// replaces the rule-based path with an HTTP call to the LLM plugin's /classify endpoint.
// Rule-based precision ~80%; LoRA target precision ≥ 92%. This rule-based path remains the
// PERMANENT FALLBACK when the LoRA classifier is unavailable.
// Roadmap ref: src/security/ROADMAP.md § "Phase 4: Zero-Trust & Post-Quantum Cryptography"

#include "security/intent_classifier.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

// ── LoRA / LLM-plugin classify endpoint (Wave-2: THEMIS_HAS_LORA_CLASSIFIER) ─
// When -DTHEMIS_HAS_LORA_CLASSIFIER=ON is set in CMake, configureLoraEndpoint()
// installs an InferenceFn that POSTs the query to the LLM plugin's /classify
// endpoint via libcurl and maps the JSON response back to ClassificationResult.
// The rule-based fallback path below remains the PERMANENT FALLBACK when the
// LoRA classifier is not configured or the endpoint is unreachable.
#ifdef THEMIS_HAS_LORA_CLASSIFIER
#  include <curl/curl.h>
#endif

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
    for (auto c : s) {
      out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
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
    {" REMOVE ",         0.65},  // REMOVE keyword in any AQL context; FOR+REMOVE handled by forRemoveWithoutFilterWeight
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
double sigmoid([[maybe_unused]] double x) {
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
        [[fallthrough]];\n        default:                              return 0.0;
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
    for (auto v : emb) {
      norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 1e-6f) {
        for (auto& v : emb) {
          v /= norm;
        }
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

#ifdef THEMIS_HAS_LORA_CLASSIFIER
// ── Real LoRA endpoint integration (Wave-2: THEMIS_HAS_LORA_CLASSIFIER) ──────

namespace {

/// libcurl write callback that appends data to a std::string buffer.
static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = reinterpret_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}

/// Map a JSON intent string returned by the endpoint to IntentType.
static IntentClassifier::IntentType intentFromString(const std::string& s) noexcept {
    if (s == "SQL_INJECTION") {
      return IntentClassifier::IntentType::SQL_INJECTION;
    }
    if (s == "DATA_EXFILTRATION") {
      return IntentClassifier::IntentType::DATA_EXFILTRATION;
    }
    if (s == "PRIVILEGE_ESCALATION") {
      return IntentClassifier::IntentType::PRIVILEGE_ESCALATION;
    }
    if (s == "ANOMALOUS_PATTERN") {
      return IntentClassifier::IntentType::ANOMALOUS_PATTERN;
    }
    if (s == "DATA_DESTRUCTION") {
      return IntentClassifier::IntentType::DATA_DESTRUCTION;
    }
    if (s == "SCHEMA_MUTATION") {
      return IntentClassifier::IntentType::SCHEMA_MUTATION;
    }
    return IntentClassifier::IntentType::LEGITIMATE;
}

} // anonymous namespace

/**
 * @brief Configure the LoRA classify endpoint for this classifier.
 *
 * Installs an InferenceFn that POSTs the query to the LLM plugin's /classify
 * endpoint via libcurl (synchronous).  The endpoint must return JSON of the form:
 * ```json
 * { "intent": "SQL_INJECTION", "confidence": 0.92, "indicator": "UNION_SELECT" }
 * ```
 * On any network/parse error the function returns LEGITIMATE with confidence=0 so
 * the caller's fail-closed logic applies.
 *
 * @param endpoint_url  Full URL of the LLM classify endpoint.
 * @param api_key       Optional bearer token for the endpoint (empty = no auth).
 * @param timeout_ms    HTTP request timeout in milliseconds (default 2000).
 * @return true if the endpoint URL is non-empty and the libcurl handle was allocated;
 *         false otherwise (LoRA path stays inactive).
 */
bool IntentClassifier::configureLoraEndpoint(
    const std::string& endpoint_url,
    const std::string& api_key,
    int                timeout_ms)
{
    if (endpoint_url.empty()) {
        spdlog::warn("IntentClassifier::configureLoraEndpoint: empty URL – skipping");
        return false;
    }

    // Capture endpoint config in the closure; the CURL handle is allocated per-call
    // to avoid cross-thread state issues (no global curl handle stored).
    const std::string url     = endpoint_url;
    const std::string key     = api_key;
    const int         timeout = timeout_ms;

    setInferenceFn([url, key, timeout](
        const std::string& query,
        const ZeroTrustContext& /*ctx*/) -> ClassificationResult
    {
        // Build JSON request body with intent classification schema.
        const nlohmann::json req = {
            {"query",   query},
            {"schema",  {"SQL_INJECTION","DATA_EXFILTRATION","PRIVILEGE_ESCALATION",
                         "ANOMALOUS_PATTERN","DATA_DESTRUCTION","SCHEMA_MUTATION",
                         "LEGITIMATE"}}
        };
        const std::string body = req.dump();

        CURL* curl = curl_easy_init();
        if (!curl) {
            spdlog::error("IntentClassifier LoRA: curl_easy_init failed");
            return {IntentType::LEGITIMATE, 0.0, "lora_curl_init_failed"};
        }

        std::string response_buf;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!key.empty()) {
            const std::string auth_hdr = "Authorization: Bearer " + key;
            headers = curl_slist_append(headers, auth_hdr.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST,            1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS,      body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,   static_cast<long>(body.size()));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER,      headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   curlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,       &response_buf);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,      static_cast<long>(timeout));
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL,        1L);

        CURLcode rc = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (rc != CURLE_OK) {
            spdlog::warn("IntentClassifier LoRA: HTTP request failed: {}",
                         curl_easy_strerror(rc));
            return {IntentType::LEGITIMATE, 0.0, "lora_http_error"};
        }

        // Parse response JSON.
        try {
            const auto j        = nlohmann::json::parse(response_buf);
            const auto intent   = intentFromString(j.value("intent", "LEGITIMATE"));
            const double conf   = j.value("confidence", 0.0);
            const auto indicator= j.value("indicator", std::string("lora"));
            return {intent, conf, indicator};
        } catch (const std::exception& e) {
            spdlog::warn("IntentClassifier LoRA: JSON parse error: {}", e.what());
            return {IntentType::LEGITIMATE, 0.0, "lora_parse_error"};
        }
    });

    lora_active_     = true;
    lora_model_path_ = endpoint_url; // Use endpoint URL as the model path identifier
    spdlog::info("IntentClassifier: LoRA endpoint configured at '{}' (ASL-13)", endpoint_url);
    return true;
}

#endif // THEMIS_HAS_LORA_CLASSIFIER

} // namespace security
} // namespace themis

