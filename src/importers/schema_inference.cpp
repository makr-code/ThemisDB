/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            schema_inference.cpp                               ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 18:48:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     294                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9efa3acd76  2026-03-11  feat(importers): add PostgreSQL Importer v2.1+ with 12 ne... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/schema_inference.h"
#include <algorithm>
#include <cmath>
#include <regex>
#include <set>
#include <unordered_set>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SchemaInferenceEngine::SchemaInferenceEngine(Config cfg)
    : config_(std::move(cfg)) {}

// ---------------------------------------------------------------------------
// Algorithm 1: implicit relationship discovery
// ---------------------------------------------------------------------------

bool SchemaInferenceEngine::columnNameSimilar(const std::string& a,
                                               const std::string& b) const {
    // Strip common suffixes (_id, _fk, _key) and compare stems
    auto stripSuffix = [](std::string s) -> std::string {
        for (const auto& suf : {"_id", "_fk", "_key", "_ref"}) {
            if (s.size() > std::strlen(suf) &&
                s.compare(s.size() - std::strlen(suf), std::strlen(suf), suf) == 0) {
                s.resize(s.size() - std::strlen(suf));
            }
        }
        return s;
    };
    return stripSuffix(a) == stripSuffix(b);
}

double SchemaInferenceEngine::jaccardSimilarity(const std::vector<std::string>& a,
                                                 const std::vector<std::string>& b) const {
    if (a.empty() && b.empty()) return 1.0;
    if (a.empty() || b.empty()) return 0.0;

    std::unordered_set<std::string> setA(a.begin(), a.end());
    std::unordered_set<std::string> setB(b.begin(), b.end());

    size_t intersection = 0;
    for (const auto& v : setA) {
        if (setB.count(v)) ++intersection;
    }
    size_t union_size = setA.size() + setB.size() - intersection;
    return union_size == 0 ? 0.0 : static_cast<double>(intersection) / union_size;
}

std::vector<SchemaInferenceEngine::InferredSchema>
SchemaInferenceEngine::inferImplicitRelationships(
    const std::vector<InferenceTableSchema>& schemas,
    const std::map<std::string, ColumnStatistics>& stats)
{
    std::vector<InferredSchema> results;

    // Build a lookup of column → sample values from stats
    // (We use distinct_count / total_rows as a proxy for value sets)
    for (const auto& schema : schemas) {
        InferredSchema inferred;
        inferred.table_name = schema.name;
        inferred.recommendations = json::object();

        for (const auto& col : schema.columns) {
            // Look for a matching column (by name stem) in another table's PKs
            for (const auto& other : schemas) {
                if (other.name == schema.name) continue;

                for (const auto& pk : other.primary_keys) {
                    if (columnNameSimilar(col, pk) || columnNameSimilar(col, other.name + "_id")) {
                        std::string rel =
                            schema.name + "." + col + " -> " + other.name + "." + pk;

                        // Compute a simple confidence from distinct counts
                        double confidence = 0.8; // default heuristic
                        std::string col_key = schema.name + "." + col;
                        std::string pk_key  = other.name  + "." + pk;
                        if (stats.count(col_key) && stats.count(pk_key)) {
                            const auto& cStat = stats.at(col_key);
                            const auto& pkStat = stats.at(pk_key);
                            if (pkStat.distinct_count > 0) {
                                confidence = std::min(1.0,
                                    static_cast<double>(cStat.distinct_count) /
                                    static_cast<double>(pkStat.distinct_count));
                            }
                        }

                        if (confidence >= config_.relationship_confidence_threshold) {
                            inferred.likely_relationships.push_back(
                                rel + " [conf=" + std::to_string(confidence).substr(0, 4) + "]");
                        }
                    }
                }
            }

            // Record cardinality distribution
            std::string key = schema.name + "." + col;
            if (stats.count(key)) {
                const auto& st = stats.at(key);
                inferred.cardinality_distribution[col] =
                    st.total_rows > 0
                        ? static_cast<double>(st.distinct_count) / st.total_rows
                        : 0.0;
            }
        }

        // Denormalization candidates: columns with very high distinct ratio
        for (const auto& [col, ratio] : inferred.cardinality_distribution) {
            if (ratio > 0.95) {
                inferred.denormalization_candidates.push_back(col);
            }
        }

        inferred.recommendations["relationship_count"] =
            inferred.likely_relationships.size();
        inferred.recommendations["denorm_candidates"] =
            inferred.denormalization_candidates.size();

        results.push_back(std::move(inferred));
    }

    return results;
}

// ---------------------------------------------------------------------------
// Algorithm 2: semantic type detection
// ---------------------------------------------------------------------------

SchemaInferenceEngine::SemanticType
SchemaInferenceEngine::detectSingleColumn(
    const std::vector<std::string>& values) const
{
    if (values.empty()) return SemanticType::UNKNOWN;

    // Simple regex patterns for common semantic types
    static const std::regex re_email(R"(^[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}$)");
    static const std::regex re_uuid(R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)");
    static const std::regex re_ipv4(R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$)");
    static const std::regex re_sha256(R"(^[0-9a-fA-F]{64}$)");
    static const std::regex re_url(R"(^https?://.+)");
    static const std::regex re_iso8601(R"(^\d{4}-\d{2}-\d{2}(T\d{2}:\d{2}(:\d{2})?)?$)");
    static const std::regex re_phone(R"(^\+?[\d\s\-\(\)]{7,20}$)");
    static const std::regex re_currency(R"(^[-]?\d+(\.\d{2})?$)");
    static const std::regex re_coord(R"(^-?\d+(\.\d+)?$)");

    size_t sample_size = std::min(values.size(), config_.max_sample_values);
    std::map<SemanticType, size_t> votes;

    for (size_t i = 0; i < sample_size; ++i) {
        const auto& v = values[i];
        if (std::regex_match(v, re_email))   { votes[SemanticType::EMAIL]++; continue; }
        if (std::regex_match(v, re_uuid))    { votes[SemanticType::UUID]++; continue; }
        if (std::regex_match(v, re_sha256))  { votes[SemanticType::HASH_SHA256]++; continue; }
        if (std::regex_match(v, re_url))     { votes[SemanticType::URL]++; continue; }
        if (std::regex_match(v, re_iso8601)) { votes[SemanticType::ISO8601_DATETIME]++; continue; }
        if (std::regex_match(v, re_ipv4))    { votes[SemanticType::IP_ADDRESS]++; continue; }
        if (std::regex_match(v, re_phone))   { votes[SemanticType::PHONE]++; continue; }
        if (std::regex_match(v, re_currency)){ votes[SemanticType::CURRENCY]++; continue; }
    }

    if (votes.empty()) return SemanticType::UNKNOWN;

    auto best = std::max_element(votes.begin(), votes.end(),
        [](const auto& a, const auto& b){ return a.second < b.second; });

    // Require at least 70 % agreement
    if (best->second * 100 / sample_size >= 70) return best->first;
    return SemanticType::UNKNOWN;
}

std::map<std::string, SchemaInferenceEngine::SemanticType>
SchemaInferenceEngine::detectSemanticTypes(
    const std::vector<InferenceTableSchema>& schemas,
    const std::vector<SampleData>& samples)
{
    std::map<std::string, SemanticType> result;
    if (!config_.enable_semantic_detection) return result;

    // Index samples by "table.column"
    std::map<std::string, std::vector<std::string>> sample_index;
    for (const auto& s : samples) {
        sample_index[s.table_name + "." + s.column_name] = s.values;
    }

    for (const auto& schema : schemas) {
        for (const auto& col : schema.columns) {
            std::string key = schema.name + "." + col;
            auto it = sample_index.find(key);
            if (it != sample_index.end()) {
                result[key] = detectSingleColumn(it->second);
            }
        }
    }
    return result;
}

std::string SchemaInferenceEngine::semanticTypeToString(SemanticType t) {
    switch (t) {
        case SemanticType::EMAIL:            return "EMAIL";
        case SemanticType::PHONE:            return "PHONE";
        case SemanticType::CURRENCY:         return "CURRENCY";
        case SemanticType::LOCATION_COORD:   return "LOCATION_COORD";
        case SemanticType::ISO8601_DATETIME: return "ISO8601_DATETIME";
        case SemanticType::UUID:             return "UUID";
        case SemanticType::HASH_SHA256:      return "HASH_SHA256";
        case SemanticType::IP_ADDRESS:       return "IP_ADDRESS";
        case SemanticType::URL:              return "URL";
        default:                             return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// Algorithm 3: cardinality estimation
// ---------------------------------------------------------------------------

std::vector<SchemaInferenceEngine::CardinalityEstimate>
SchemaInferenceEngine::estimateCardinalities(
    const std::vector<InferenceTableSchema>& schemas,
    const std::map<std::string, ColumnStatistics>& stats)
{
    std::vector<CardinalityEstimate> estimates;

    for (const auto& schema : schemas) {
        for (const auto& fk : schema.foreign_keys) {
            const std::string& local_col = fk.first;
            const std::string& ref = fk.second; // "other_table.other_col"

            std::string local_key = schema.name + "." + local_col;

            CardinalityEstimate est;
            est.relationship_id = local_key + " -> " + ref;

            auto it = stats.find(local_key);
            if (it != stats.end()) {
                const auto& st = it->second;
                // Harmonic mean estimator: avg children per parent
                est.one_to_many_ratio = st.distinct_count > 0
                    ? static_cast<double>(st.total_rows) / st.distinct_count
                    : 1.0;

                // Selectivity: fraction of rows with a non-null FK
                est.selectivity = st.total_rows > 0
                    ? static_cast<double>(st.total_rows - st.null_count) / st.total_rows
                    : 1.0;

                // 95 % CI using Wilson score approximation (simplified)
                double p = est.selectivity;
                double n = static_cast<double>(st.total_rows);
                double z = 1.96; // 95 % CI
                double margin = n > 0 ? z * std::sqrt(p * (1.0 - p) / n) : 0.1;
                est.confidence_interval = {
                    std::max(0.0, p - margin),
                    std::min(1.0, p + margin)
                };
            } else {
                est.one_to_many_ratio = 1.0;
                est.selectivity = 1.0;
                est.confidence_interval = {0.0, 1.0};
            }

            estimates.push_back(std::move(est));
        }
    }

    return estimates;
}

} // namespace importers
} // namespace themis
