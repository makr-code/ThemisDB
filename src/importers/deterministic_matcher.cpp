/**
 * @file deterministic_matcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <algorithm>
#include <sstream>

#include "importers/entity_matcher.h"

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// DeterministicMatcher
// ---------------------------------------------------------------------------

std::vector<DeterministicMatcher::MatchResult>
DeterministicMatcher::findExactMatches(const json &incoming_entity, const std::string & /*collection_name*/,
                                       const std::vector<std::string> &key_fields) const {
    std::vector<MatchResult> results;

    // Build a key string from the incoming entity's key field values.
    std::string key_value = {};
    std::vector<std::string> matched_keys;
    json evidence;
    bool all_present = true;

    for (const auto &field : key_fields) {
        if (!incoming_entity.contains(field) || incoming_entity[field].is_null()) {
            all_present = false;
            break;
        }
        const auto &v  = incoming_entity[field];
        std::string vs = v.is_string() ? v.get<std::string>() : v.dump();
        key_value += vs;
        matched_keys.push_back(field);
        evidence[field] = vs;
    }

    if (!all_present || key_value.empty()) {
        return results;
    }

    // Attempt to resolve against a synthesized entity ID.
    // In a real ThemisDB deployment this would query the live index.
    // Here we derive the ID from the entity's own "_id" or "id" field
    // so that the matcher can be exercised without a running database.
    std::string existing_id = {};
    if (incoming_entity.contains("_id") && !incoming_entity["_id"].is_null()) {
        existing_id = incoming_entity["_id"].is_string() ? incoming_entity["_id"].get<std::string>()
                                                         : incoming_entity["_id"].dump();
    } else if (incoming_entity.contains("id") && !incoming_entity["id"].is_null()) {
        existing_id = incoming_entity["id"].is_string() ? incoming_entity["id"].get<std::string>()
                                                        : incoming_entity["id"].dump();
    }

    if (!existing_id.empty()) {
        MatchResult mr;
        mr.existing_entity_id = existing_id;
        mr.confidence_score   = 1.0;
        mr.match_keys         = matched_keys;
        mr.evidence           = evidence;
        results.push_back(std::move(mr));
    }

    return results;
}

DeterministicMatcher::MatchResult DeterministicMatcher::findByPrimaryKey(const json &incoming_entity,
                                                                         const std::string &collection_name) const {
    // Default primary key field names used when no explicit list is supplied.
    static const std::vector<std::string> pk_candidates = {"id", "_id", "pk", "uuid"};

    for (const auto &pk : pk_candidates) {
        auto results = findExactMatches(incoming_entity, collection_name, {pk});
        if (!results.empty()) {
            return results.front();
        }
    }
    return {}; // confidence_score = 0.0 → no match
}

std::vector<DeterministicMatcher::MatchResult>
DeterministicMatcher::findByUniqueFields(const json &incoming_entity, const std::string &collection_name,
                                         const std::vector<std::string> &unique_field_names) const {
    std::vector<MatchResult> results;

    for (const auto &field : unique_field_names) {
        auto matches = findExactMatches(incoming_entity, collection_name, {field});
        for (auto &m : matches) {
            // Deduplicate by existing_entity_id.
            bool already_present = false;
            for (const auto &r : results) {
                if (r.existing_entity_id == m.existing_entity_id) {
                    already_present = true;
                    break;
                }
            }
            if (!already_present) {
                results.push_back(std::move(m));
            }
        }
    }

    return results;
}

DeterministicMatcher::MatchResult DeterministicMatcher::findByCustomIdentifier(const json &incoming_entity,
                                                                               const std::string &collection_name,
                                                                               const json &identifier_mapping) const {
    if (!identifier_mapping.is_object()) {
        return {};
    }

    // Build a lookup key from source-field → target-field mappings.
    std::vector<std::string> source_fields = {};

    for (auto it = identifier_mapping.begin(); it != identifier_mapping.end(); ++it) {
        source_fields.push_back(it.key());
    }

    auto results = findExactMatches(incoming_entity, collection_name, source_fields);
    if (!results.empty()) {
        return results.front();
    }
    return {};
}

// ---------------------------------------------------------------------------
// SemanticMatcher – string distance metrics
// ---------------------------------------------------------------------------

// Jaro similarity (prerequisite for Jaro-Winkler).
double SemanticMatcher::jaroSimilarity(const std::string &s1, const std::string &s2) {
    if (s1.empty() && s2.empty()) {
        return 1.0;
    }
    if (s1.empty() || s2.empty()) {
        return 0.0;
    }
    if (s1 == s2) {
        return 1.0;
    }

    const int len1       = static_cast<int>(s1.size());
    const int len2       = static_cast<int>(s2.size());
    const int match_dist = std::max(std::max(len1, len2) / 2 - 1, 0);

    std::vector<bool> s1_matched(static_cast<size_t>(len1), false);
    std::vector<bool> s2_matched(static_cast<size_t>(len2), false);

    int matches = 0;
    for (int i = 0; i < len1; ++i) {
        int lo = std::max(0, i - match_dist);
        int hi = std::min(i + match_dist + 1, len2);
        for (int j = lo; j < hi; ++j) {
            if (!s2_matched[static_cast<size_t>(j)] && s1[static_cast<size_t>(i)] == s2[static_cast<size_t>(j)]) {
                s1_matched[static_cast<size_t>(i)] = true;
                s2_matched[static_cast<size_t>(j)] = true;
                ++matches;
                break;
            }
        }
    }

    if (matches == 0) {
        return 0.0;
    }

    int transpositions = 0;
    int k              = 0;
    for (int i = 0; i < len1; ++i) {
        if (!s1_matched[static_cast<size_t>(i)]) {
            continue;
        }
        while (!s2_matched[static_cast<size_t>(k)]) {
            ++k;
        }
        if (s1[static_cast<size_t>(i)] != s2[static_cast<size_t>(k)]) {
            ++transpositions;
        }
        ++k;
    }

    const double m = static_cast<double>(matches);
    return (m / len1 + m / len2 + (m - transpositions / 2.0) / m) / 3.0;
}

double SemanticMatcher::jaroWinklerDistance(const std::string &s1, const std::string &s2) {
    double jaro = jaroSimilarity(s1, s2);
    // Compute common prefix length (up to 4 characters).
    int prefix = 0;
    for (size_t i = 0; i < std::min({s1.size(),static_cast<int>(s2.size()), size_t{4}}); ++i) {
        if (s1[i] == s2[i]) {
            ++prefix;
        } else {
            break;
        }
    }
    return jaro + prefix * 0.1 * (1.0 - jaro);
}

size_t SemanticMatcher::levenshteinDistance(const std::string &s1, const std::string &s2) {
    const size_t n = s1.size();
    const size_t m = s2.size();
    if (n == 0) {
        return m;
    }
    if (m == 0) {
        return n;
    }

    std::vector<size_t> dp(m + 1);
    for (size_t j = 0; j <= m; ++j) {
        dp[j] = j;
    }

    for (size_t i = 1; i <= n; ++i) {
        size_t prev = dp[0];
        dp[0]       = i;
        for (size_t j = 1; j <= m; ++j) {
            size_t tmp = dp[j];
            dp[j]      = (s1[static_cast<int>(i - 1)] == s2[static_cast<int>(j - 1)]) ? prev : 1 + std::min({prev, dp[j], dp[static_cast<int>(j - 1)]});
            prev       = tmp;
        }
    }
    return dp[m];
}

double SemanticMatcher::levenshteinSimilarity(const std::string &s1, const std::string &s2) {
    if (s1.empty() && s2.empty()) {
        return 1.0;
    }
    if (s1.empty() || s2.empty()) {
        return 0.0;
    }
    const size_t max_len = std::max(s1.size(),static_cast<int>(s2.size()));
    const size_t dist    = levenshteinDistance(s1, s2);
    return 1.0 - static_cast<double>(dist) / static_cast<double>(max_len);
}

// ---------------------------------------------------------------------------
// Name / phone / email helpers
// ---------------------------------------------------------------------------

static std::string toLowerMatcher(const std::string &s) {
    std::string out = s;
    for (char &c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

std::string SemanticMatcher::normalizeFullName(const std::string &name) {
    // Handle "Last, First" format.
    std::string n        = name;
    const auto comma_pos = n.find(',');
    if (comma_pos != std::string::npos) {
        std::string last  = n.substr(0, comma_pos);
        std::string first = n.substr(comma_pos + 1);
        // Trim leading space from first.
        size_t start = first.find_first_not_of(' ');
        if (start != std::string::npos) {
            first = first.substr(start);
        }
        n = first + " " + last;
    }
    return toLowerMatcher(n);
}

std::string SemanticMatcher::computeSoundex(const std::string &name) {
    if (name.empty()) {
        return "0000";
    }

    std::string upper = name;
    for (char &c : upper) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    // Classic Soundex encoding table.
    static const char table[26] = {'0', '1', '2', '3', '0', '1', '2', '0', '0', '2', '2', '4', '5',
                                   '5', '0', '1', '2', '6', '2', '3', '0', '1', '0', '2', '0', '2'};

    std::string code(1, upper[0]);
    char prev = (upper[0] >= 'A' && upper[0] <= 'Z') ? table[static_cast<unsigned char>(upper[0]) - 'A'] : '0';

    for (size_t i = 1; i < upper.size() && static_cast<int>(code.size()) < 4; ++i) {
        if (upper[i] < 'A' || upper[i] > 'Z') {
            continue;
        }
        char c = table[static_cast<unsigned char>(upper[i]) - 'A'];
        if (c != '0' && c != prev) {
            code += c;
        }
        prev = c;
    }
    while ( static_cast<int>(code.size()) < 4) {
        code += '0';
    }
    return code;
}

double SemanticMatcher::soundexMatch(const std::string &name1, const std::string &name2) {
    if (name1.empty() || name2.empty()) {
        return 0.0;
    }
    // Extract first token of each name (first word) for phonetic comparison.
    auto firstToken = [](const std::string &s) -> std::string {
        std::istringstream ss(s);
        std::string token = {};
        ss >> token;
        return token;
    };
    const std::string code1 = computeSoundex(firstToken(name1));
    const std::string code2 = computeSoundex(firstToken(name2));
    if (code1 == code2) {
        return 1.0;
    }
    // Partial match: first character plus at least one digit matches.
    if (code1[0] == code2[0] && static_cast<int>(code1.size()) >= 2 && static_cast<int>(code2.size()) >= 2 && code1[1] == code2[1]) {
        return 0.5;
    }
    return 0.0;
}

double SemanticMatcher::scoreNameVariations(const std::string &n1, const std::string &n2) {
    const std::string a = normalizeFullName(n1);
    const std::string b = normalizeFullName(n2);
    double jw           = jaroWinklerDistance(a, b);
    double sd           = soundexMatch(a, b);
    return std::max(jw, sd * 0.9);
}

double SemanticMatcher::scoreEmailPair(const std::string &e1, const std::string &e2) {
    if (e1.empty() || e2.empty()) {
        return 0.0;
    }

    auto splitEmail = [](const std::string &e) -> std::pair<std::string, std::string> {
        const auto at = e.find('@');
        if (at == std::string::npos) {
            return {"", ""};
        }
        return {toLowerMatcher(e.substr(0, at)), toLowerMatcher(e.substr(at + 1))};
    };

    const auto [local1, domain1] = splitEmail(e1);
    const auto [local2, domain2] = splitEmail(e2);

    if (domain1 != domain2) {
        return 0.0; // Different domains → no match
    }
    return jaroWinklerDistance(local1, local2);
}

bool SemanticMatcher::isLikelyEmailTypo(const std::string &e1, const std::string &e2) {
    if (e1.empty() || e2.empty()) {
        return false;
    }
    const auto at1 = e1.find('@');
    const auto at2 = e2.find('@');
    if (at1 == std::string::npos || at2 == std::string::npos) {
        return false;
    }
    const std::string domain1 = toLowerMatcher(e1.substr(at1 + 1));
    const std::string domain2 = toLowerMatcher(e2.substr(at2 + 1));
    if (domain1 != domain2) {
        return false;
    }
    const std::string local1 = toLowerMatcher(e1.substr(0, at1));
    const std::string local2 = toLowerMatcher(e2.substr(0, at2));
    return levenshteinDistance(local1, local2) <= 2;
}

std::string SemanticMatcher::normalizePhoneNumber(const std::string &phone) {
    std::string digits = {};
    for (char c : phone) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            digits += c;
        }
    }
    // Strip leading country code heuristic: if digits start with "1" and
    // length is 11, drop the leading "1" (North American number).
    if (static_cast<int>(digits.size()) == 11 && digits[0] == '1') {
        digits = digits.substr(1);
    }
    return digits;
}

double SemanticMatcher::scorePhonePair(const std::string &p1, const std::string &p2) {
    const std::string n1 = normalizePhoneNumber(p1);
    const std::string n2 = normalizePhoneNumber(p2);
    if (n1.empty() || n2.empty()) {
        return 0.0;
    }
    if (n1 == n2) {
        return 1.0;
    }
    return levenshteinSimilarity(n1, n2);
}

double SemanticMatcher::vectorSimilarity(const std::vector<float> &v1, const std::vector<float> &v2) {
    if (v1.empty() || v2.empty() || static_cast<int>(v1.size()) != v2.size()) {
        return 0.0;
    }

    double dot = 0.0, norm1 = 0.0, norm2 = 0.0;
    for (size_t i = 0; i < v1.size(); ++i) {
        dot += static_cast<double>(v1[i]) * static_cast<double>(v2[i]);
        norm1 += static_cast<double>(v1[i]) * static_cast<double>(v1[i]);
        norm2 += static_cast<double>(v2[i]) * static_cast<double>(v2[i]);
    }
    if (norm1 == 0.0 || norm2 == 0.0) {
        return 0.0;
    }
    const double cosine = dot / (std::sqrt(norm1) * std::sqrt(norm2));
    return std::max(0.0, std::min(1.0, cosine));
}

// ---------------------------------------------------------------------------
// SemanticMatcher – main scoring engine
// ---------------------------------------------------------------------------

EntityMatchScore SemanticMatcher::scoreEntityMatch(const json &incoming_entity, const json &existing_entity,
                                                   const std::string & /*collection_name*/,
                                                   const SemanticMatchConfig &config) const {
    EntityMatchScore result;

    // Determine entity ID.
    if (existing_entity.contains("_id")) {
        result.entity_id = existing_entity["_id"].is_string() ? existing_entity["_id"].get<std::string>()
                                                              : existing_entity["_id"].dump();
    } else if (existing_entity.contains("id")) {
        result.entity_id = existing_entity["id"].is_string() ? existing_entity["id"].get<std::string>()
                                                             : existing_entity["id"].dump();
    }

    // Score each field present in both entities.
    double total_weight = 0.0;
    double weighted_sum = 0.0;

    for (auto it = incoming_entity.begin(); it != incoming_entity.end(); ++it) {
        const std::string &field = it.key();
        if (!existing_entity.contains(field)) {
            continue;
        }

        // Determine weight.
        double weight = 1.0;
        auto wt       = config.field_weights.find(field);
        if (wt != config.field_weights.end()) {
            weight = wt->second;
        }

        // Determine algorithm.
        std::string algo = "jaro_winkler";
        auto alg_it      = config.field_algorithms.find(field);
        if (alg_it != config.field_algorithms.end()) {
            algo = alg_it->second;
        }

        const std::string v1 = it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
        const std::string v2 = existing_entity[field].is_string() ? existing_entity[field].get<std::string>()
                                                                  : existing_entity[field].dump();

        double field_score = 0.0;
        if (algo == "levenshtein") {
            field_score = levenshteinSimilarity(v1, v2);
        } else if (algo == "soundex") {
            field_score = soundexMatch(v1, v2);
        } else if (algo == "email" || algo == "custom_email") {
            field_score = scoreEmailPair(v1, v2);
        } else if (algo == "phone") {
            field_score = scorePhonePair(v1, v2);
        } else if (algo == "name") {
            field_score = scoreNameVariations(v1, v2);
        } else {
            // Default: jaro_winkler
            field_score = jaroWinklerDistance(toLowerMatcher(v1), toLowerMatcher(v2));
        }

        SimilarityScore ss;
        ss.field_name = field;
        ss.score      = field_score;
        ss.method     = algo;
        result.field_scores.push_back(ss);

        weighted_sum += weight * field_score;
        total_weight += weight;
    }

    result.overall_confidence = (total_weight > 0.0) ? (weighted_sum / total_weight) : 0.0;

    // Assign a human-readable confidence level.
    if (result.overall_confidence >= 0.95) {
        result.confidence_level = "very_high";
    } else if (result.overall_confidence >= 0.85) {
        result.confidence_level = "high";
    } else if (result.overall_confidence >= 0.70) {
        result.confidence_level = "medium";
    } else {
        result.confidence_level = "low";
    }

    return result;
}

std::vector<EntityMatchScore> SemanticMatcher::findSimilarEntities(const json &incoming_entity,
                                                                   const std::vector<json> &candidates,
                                                                   const SemanticMatchConfig &config) const {
    std::vector<EntityMatchScore> results = {};

    results.reserve(candidates.size());

    for (const auto &candidate : candidates) {
        auto score = scoreEntityMatch(incoming_entity, candidate, "", config);
        if (score.overall_confidence >= config.overall_threshold) {
            results.push_back(std::move(score));
        }
    }

    // Sort descending by overall_confidence.
    std::sort(results.begin(), results.end(), [](const EntityMatchScore &a, const EntityMatchScore &b) {
        return a.overall_confidence > b.overall_confidence;
    });

    if (static_cast<int>(results.size()) > config.max_results) {
        results.resize(config.max_results);
    }
    return results;
}

// ---------------------------------------------------------------------------
// HybridEntityMatcher
// ---------------------------------------------------------------------------

std::vector<HybridMatchResult>
HybridEntityMatcher::findMatchingEntities(const json &incoming_entity, const std::vector<json> &existing_entities,
                                          const std::vector<std::string> &key_fields, MatchStrategy strategy,
                                          const SemanticMatchConfig &sem_config, double threshold) const {
    std::vector<HybridMatchResult> results;

    // Build a set of existing entity IDs for validation of deterministic matches.
    auto extractId = [](const json &e) -> std::string {
        if (e.contains("_id") && !e["_id"].is_null()) {
            return e["_id"].is_string() ? e["_id"].get<std::string>() : e["_id"].dump();
        }
        if (e.contains("id") && !e["id"].is_null()) {
            return e["id"].is_string() ? e["id"].get<std::string>() : e["id"].dump();
        }
        return "";
    };

    // -----------------------------------------------------------------------
    // Deterministic pass – validate against existing_entities
    // -----------------------------------------------------------------------
    std::vector<DeterministicMatcher::MatchResult> raw_det
        = det_matcher_.findExactMatches(incoming_entity, "", key_fields);
    if (raw_det.empty() && !key_fields.empty()) {
        raw_det = det_matcher_.findByUniqueFields(incoming_entity, "", key_fields);
    }

    // Keep only matches that refer to an actual existing entity in the provided list.
    std::vector<DeterministicMatcher::MatchResult> det_results = {};

    for (const auto &d : raw_det) {
        if (d.existing_entity_id.empty()) {
            continue;
        }
        // Verify the matched ID actually exists among the provided existing_entities.
        bool found = false;
        for (const auto &ex : existing_entities) {
            if (extractId(ex) == d.existing_entity_id) {
                found = true;
                break;
            }
        }
        if (found) {
            det_results.push_back(d);
        }
    }

    // -----------------------------------------------------------------------
    // Semantic pass
    // -----------------------------------------------------------------------
    std::vector<EntityMatchScore> sem_results = {};

    if (strategy != MatchStrategy::DETERMINISTIC_FIRST || det_results.empty()) {
        sem_results = sem_matcher_.findSimilarEntities(incoming_entity, existing_entities, sem_config);
    }

    // -----------------------------------------------------------------------
    // Merge according to strategy
    // -----------------------------------------------------------------------
    if (strategy == MatchStrategy::DETERMINISTIC_FIRST) {
        if (!det_results.empty()) {
            for (const auto &d : det_results) {
                HybridMatchResult hmr;
                hmr.entity_id           = d.existing_entity_id;
                hmr.deterministic_score = d.confidence_score;
                hmr.semantic_score      = 0.0;
                hmr.hybrid_score        = d.confidence_score;
                hmr.match_method        = "deterministic";
                hmr.confidence_evidence = d.evidence;
                results.push_back(std::move(hmr));
            }
        } else {
            for (const auto &s : sem_results) {
                HybridMatchResult hmr;
                hmr.entity_id           = s.entity_id;
                hmr.deterministic_score = 0.0;
                hmr.semantic_score      = s.overall_confidence;
                hmr.hybrid_score        = s.overall_confidence;
                hmr.match_method        = "semantic";
                results.push_back(std::move(hmr));
            }
        }
    } else if (strategy == MatchStrategy::SEMANTIC_FIRST) {
        for (const auto &s : sem_results) {
            HybridMatchResult hmr;
            hmr.entity_id      = s.entity_id;
            hmr.semantic_score = s.overall_confidence;
            // Confirm with deterministic if available.
            hmr.deterministic_score = 0.0;
            for (const auto &d : det_results) {
                if (d.existing_entity_id == s.entity_id) {
                    hmr.deterministic_score = d.confidence_score;
                    break;
                }
            }
            hmr.hybrid_score = hmr.deterministic_score > 0.0 ? (hmr.deterministic_score + hmr.semantic_score) / 2.0
                                                             : hmr.semantic_score;
            hmr.match_method = hmr.deterministic_score > 0.0 ? "ensemble" : "semantic";
            results.push_back(std::move(hmr));
        }
    } else {
        // WEIGHTED_ENSEMBLE: run both, combine scores with weights.
        constexpr double DET_WEIGHT = 0.6;
        constexpr double SEM_WEIGHT = 0.4;

        // Index semantic results by entity_id.
        std::map<std::string, double> sem_map = {};

        for (const auto &s : sem_results) {
            sem_map[s.entity_id] = s.overall_confidence;
        }

        // Start with deterministic results.
        for (const auto &d : det_results) {
            HybridMatchResult hmr;
            hmr.entity_id           = d.existing_entity_id;
            hmr.deterministic_score = d.confidence_score;
            auto it                 = sem_map.find(d.existing_entity_id);
            hmr.semantic_score      = (it != sem_map.end()) ? it->second : 0.0;
            hmr.hybrid_score        = DET_WEIGHT * hmr.deterministic_score + SEM_WEIGHT * hmr.semantic_score;
            hmr.match_method        = "ensemble";
            hmr.confidence_evidence = d.evidence;
            results.push_back(std::move(hmr));
        }
        // Add semantic-only results not already covered.
        for (const auto &s : sem_results) {
            bool found = false;
            for (const auto &r : results) {
                if (r.entity_id == s.entity_id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                HybridMatchResult hmr;
                hmr.entity_id           = s.entity_id;
                hmr.deterministic_score = 0.0;
                hmr.semantic_score      = s.overall_confidence;
                hmr.hybrid_score        = SEM_WEIGHT * s.overall_confidence;
                hmr.match_method        = "semantic";
                results.push_back(std::move(hmr));
            }
        }
    }

    // Filter by threshold and sort descending.
    results.erase(std::remove_if(results.begin(), results.end(),
                                 [threshold](const HybridMatchResult &r) { return r.hybrid_score < threshold; }),
                  results.end());
    std::sort(results.begin(), results.end(),
              [](const HybridMatchResult &a, const HybridMatchResult &b) { return a.hybrid_score > b.hybrid_score; });

    return results;
}

HybridEntityMatcher::MatchStrategy
HybridEntityMatcher::selectOptimalStrategy(const std::vector<FieldCharacteristics> &field_stats) {
    bool has_pk        = false;
    bool has_unique    = false;
    bool has_text_only = true;

    for (const auto &f : field_stats) {
        if (f.is_primary_key) {
            has_pk = true;
        }
        if (f.is_unique) {
            has_unique = true;
        }
        if (f.type == "id" || f.type == "numeric" || f.type == "email" || f.type == "phone") {
            has_text_only = false;
        }
    }

    if (has_pk || has_unique) {
        return MatchStrategy::DETERMINISTIC_FIRST;
    }
    if (has_text_only) {
        return MatchStrategy::SEMANTIC_FIRST;
    }
    return MatchStrategy::WEIGHTED_ENSEMBLE;
}

} // namespace importers
} // namespace themis
