/**
 * @file classify_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "aql/classify_bridge.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "aql/docs_assistant_functions.h"
#include "query/functions/function_registry.h"

namespace themis {
namespace aql {

// ---------------------------------------------------------------------------
// Internal keyword-scoring classifier
// ---------------------------------------------------------------------------

namespace {

struct CategorySpec {
    std::string label;
    std::vector<std::string> keywords; // lower-case
    double weight;                     // per-hit weight
};

/// Ordered from most to least specific so that earlier matches dominate ties.
const std::vector<CategorySpec> &categorySpecs() {
    static const std::vector<CategorySpec> specs = {
        {"configuration",
         {"configure",   "config",    "setting",   "setup",   "install",    "enable", "disable",    "create",
          "drop",        "alter",     "schema",    "index",   "collection", "shard",  "sharding",   "replica",
          "replication", "cluster",   "node",      "network", "security",   "auth",   "tls",        "ssl",
          "limit",       "threshold", "parameter", "option",  "flag",       "port",   "host",       "path",
          "directory",   "how do i",  "how to",    "set up",  "tune",       "adjust", "initialise", "initialize"},
         1.0},
        {"troubleshooting",
         {"error",
          "fail",
          "failure",
          "problem",
          "issue",
          "hang",
          "crash",
          "not work",
          "doesn't work",
          "does not work",
          "broken",
          "slow",
          "timeout",
          "exception",
          "bug",
          "debug",
          "trace",
          "stacktrace",
          "stack trace",
          "warning",
          "critical",
          "panic",
          "segfault",
          "oom",
          "out of memory",
          "killed",
          "why is",
          "why does",
          "why can't",
          "cannot connect",
          "connection refused",
          "lost connection"},
         1.0},
        {"search",
         {"search", "search for", "find", "look for", "documentation about", "documentation", "where is", "list",
          "show me", "examples of", "what functions", "all the", "available", "supported", "reference"},
         1.0},
        {
            "general",
            {"explain", "describe", "what is", "what are", "why", "help", "learn", "overview", "introduction",
             "concept", "difference", "compare", "versus", "vs"},
            0.8 // slightly lower so explicit matches win
        },
    };
    return specs;
}

/// Compute a raw score for each category against a lower-cased query.
std::unordered_map<std::string, double> scoreCategories(const std::string &query_lower,
                                                        const std::vector<std::string> &categories) {
    std::unordered_map<std::string, double> scores;
    for (const auto &cat : categories) {
        scores[cat] = 0.0;
    }

    for (const auto &spec : categorySpecs()) {
        // Only score categories the caller asked for.
        if (scores.find(spec.label) == scores.end()) {
            continue;
        }
        for (const auto &kw : spec.keywords) {
            if (query_lower.find(kw) != std::string::npos) {
                scores[spec.label] += spec.weight;
            }
        }
    }
    return scores;
}

/// Softmax over scores so confidence values are in (0, 1) and sum to 1.
std::unordered_map<std::string, double> softmax(const std::unordered_map<std::string, double> &raw) {
    if (raw.empty()) {
        return {};
    }

    double max_val = -std::numeric_limits<double>::infinity();
    for (const auto &[_, v] : raw) {
        max_val = std::max(max_val, v);
    }

    std::unordered_map<std::string, double> exp_vals;
    double total = 0.0;
    for (const auto &[k, v] : raw) {
        double e    = std::exp(v - max_val);
        exp_vals[k] = e;
        total += e;
    }

    std::unordered_map<std::string, double> result;
    for (auto &[k, e] : exp_vals) {
        result[k] = e / total;
    }
    return result;
}

ClassifyResult keywordClassify(const std::string &text, const std::vector<std::string> &categories) {
    if (categories.empty()) {
        return ClassifyResult{};
    }

    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    auto raw = scoreCategories(lower, categories);

    // If no keywords matched at all, there is no signal – return an empty
    // result so detectIntentWithNativeNLP() falls through to the LLM path.
    double max_raw = 0.0;
    for (const auto &[_, v] : raw) {
        max_raw = std::max(max_raw, v);
    }
    if (max_raw == 0.0) {
        return ClassifyResult{};
    }

    auto probs = softmax(raw);

    // Pick the highest-confidence category.
    std::string best;
    double best_conf = -1.0;
    for (const auto &[k, v] : probs) {
        if (v > best_conf) {
            best_conf = v;
            best      = k;
        }
    }

    ClassifyResult result;
    result.category   = best;
    result.confidence = best_conf;
    result.scores     = probs;
    return result;
}

} // anonymous namespace

// Margin above the uniform baseline (1.0/N) required to accept a
// FunctionRegistry CLASSIFY result over the keyword fallback.  A result
// at or near uniform is likely the registry's default distribution and
// should be discarded in favour of the keyword classifier.
static constexpr double kRegistryConfidenceMargin = 0.15;

// ---------------------------------------------------------------------------
// AQLFunctionClassifyBridge
// ---------------------------------------------------------------------------

ClassifyResult AQLFunctionClassifyBridge::classify(const std::string &text,
                                                   const std::vector<std::string> &categories) const {
    // 1. Try the FunctionRegistry CLASSIFY function when registered.
    try {
        using namespace themis::query::functions;
        auto &reg = FunctionRegistry::instance();
        if (reg.hasFunction("CLASSIFY") && !categories.empty()) {
            // Build category array as nlohmann::json
            nlohmann::json cats = nlohmann::json::array();
            for (const auto &c : categories) {
                cats.push_back(c);
            }

            FunctionContext ctx;
            nlohmann::json result = reg.call("CLASSIFY", {text, cats}, ctx);

            if (result.is_object() && result.contains("category") && result.contains("confidence")) {
                ClassifyResult cr;
                cr.category   = result["category"].get<std::string>();
                cr.confidence = result["confidence"].get<double>();
                if (result.contains("scores") && result["scores"].is_object()) {
                    for (auto &[k, v] : result["scores"].items()) {
                        cr.scores[k] = v.get<double>();
                    }
                }
                // Accept registry result only when it is meaningfully above the
                // uniform-distribution baseline (1/N), indicating real signal.
                const double uniform_baseline = 1.0 / static_cast<double>(categories.size());
                if (!cr.category.empty() && cr.confidence > uniform_baseline + kRegistryConfidenceMargin) {
                    return cr;
                }
            }
        }
    } catch (...) {
        spdlog::debug("[ClassifyBridge] registry classify call failed; falling through to local classifier");
    }

    // 2. Keyword-based fallback (always produces a result).
    return keywordClassify(text, categories);
}

// ---------------------------------------------------------------------------
// registerClassifyBridge()
// ---------------------------------------------------------------------------

void registerClassifyBridge() {
    static AQLFunctionClassifyBridge bridge;
    getDocsAssistantFunctions().setClassifier(&bridge);
}

} // namespace aql
} // namespace themis

