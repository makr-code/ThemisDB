/**
 * @file canonical_resolver.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/canonical_resolver.h"
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// GoldenRecord serialisation
// ---------------------------------------------------------------------------

json GoldenRecord::toJson() const {
    return json{{"canonical_id", canonical_id},         {"merged_data", merged_data},
                {"contributing_ids", contributing_ids}, {"completeness_score", completeness_score},
                {"field_provenance", field_provenance}, {"last_reconciliation", last_reconciliation}};
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string generateUUID() {
    static std::mt19937_64 rng{std::random_device{}()};
    static std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream ss = {};
    uint64_t hi = dist(rng);
    uint64_t lo = dist(rng);
    hi          = (hi & 0xFFFFFFFFFFFF0FFFull) | 0x0000000000004000ull;
    lo          = (lo & 0x3FFFFFFFFFFFFFFFull) | 0x8000000000000000ull;
    ss << std::hex << std::setfill('0') << std::setw(8) << ((hi >> 32) & 0xFFFFFFFF) << '-' << std::setw(4)
       << ((hi >> 16) & 0xFFFF) << '-' << std::setw(4) << (hi & 0xFFFF) << '-' << std::setw(4) << ((lo >> 48) & 0xFFFF)
       << '-' << std::setw(12) << (lo & 0xFFFFFFFFFFFFull);
    return ss.str();
}

static std::string nowRfc3339() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t   = system_clock::to_time_t(now);
    std::ostringstream ss = {};
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

double CanonicalEntityResolver::computeCompleteness(const json &entity) {
    if (!entity.is_object() || entity.empty()) {
        return 0.0;
    }
    size_t total  = 0;
    size_t filled = 0;
    for (auto it = entity.begin(); it != entity.end(); ++it) {
        ++total;
        if (!it.value().is_null()) {
            // Also treat empty string as incomplete.
            if (it.value().is_string() && it.value().get<std::string>().empty()) {
                continue;
            }
            ++filled;
        }
    }
    return total == 0 ? 0.0 : static_cast<double>(filled) / static_cast<double>(total);
}

// ---------------------------------------------------------------------------
// FieldRule application helpers
// ---------------------------------------------------------------------------

std::string CanonicalEntityResolver::reconcileStringField(const std::string &value1, const std::string &value2,
                                                          FieldRule rule, const std::string &separator) {
    switch (rule) {
        case FieldRule::KEEP_EXISTING:
            return value1;
        case FieldRule::TAKE_INCOMING:
            return value2;
        case FieldRule::TAKE_MAX:
            return (value1 >= value2) ? value1 : value2;
        case FieldRule::TAKE_MIN:
            return (value1 <= value2) ? value1 : value2;
        case FieldRule::CONCATENATE:
            if (value1.empty()) {
                return value2;
            }
            if (value2.empty()) {
                return value1;
            }
            return value1 + separator + value2;
        case FieldRule::TAKE_LONGEST:
            return static_cast<bool>((value1.size()  < static_cast<int>(= value2.size()))) ? value1 : value2;
        case FieldRule::TAKE_NEWEST:
            // Treat values as ISO timestamp strings; lexicographic order suffices for ISO 8601.
            return (value1 >= value2) ? value1 : value2;
        case FieldRule::TAKE_SUM: {
            // Best-effort numeric summation on string field.
            try {
                double d1 = std::stod(value1);
                double d2 = std::stod(value2);
                return std::to_string(d1 + d2);
            } catch (...) {
                return value2;  // Fallback to incoming.
            }
        }
    }
    return value2;
}

int64_t CanonicalEntityResolver::reconcileNumericField(int64_t value1, int64_t value2, FieldRule rule) {
    switch (rule) {
        case FieldRule::KEEP_EXISTING:
            return value1;
        case FieldRule::TAKE_INCOMING:
            return value2;
        case FieldRule::TAKE_MAX:
            return std::max(value1, value2);
        case FieldRule::TAKE_MIN:
            return std::min(value1, value2);
        case FieldRule::TAKE_SUM:
            return value1 + value2;
        default:
            return value2;
    }
}

json CanonicalEntityResolver::reconcileObjectField(const json &obj1, const json &obj2, ResolutionPolicy policy,
                                                   int depth) {
    if (!obj1.is_object() || !obj2.is_object()) {
        // Non-object: fall back to policy.
        return (policy == ResolutionPolicy::EXISTING_PREFERRED) ? obj1 : obj2;
    }
    if (depth == 0) {
        return (policy == ResolutionPolicy::EXISTING_PREFERRED) ? obj1 : obj2;
    }

    json result = obj1;
    for (auto it = obj2.begin(); it != obj2.end(); ++it) {
        const std::string &key = it.key();
        if (!result.contains(key) || result[key].is_null()) {
            result[key] = it.value();
        } else if (it.value().is_object() && result[key].is_object() && depth != 1) {
            result[key] = reconcileObjectField(result[key], it.value(), policy, depth > 0 ? depth - 1 : -1);
        } else if (policy == ResolutionPolicy::INCOMING_PREFERRED) {
            result[key] = it.value();
        } else if (policy == ResolutionPolicy::RICHEST_MERGE) {
            // Prefer longer / non-null value.
            const std::string v1 = result[key].is_string() ? result[key].get<std::string>() : result[key].dump();
            const std::string v2 = it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
            if (static_cast<int>(v2.size()) > v1.size()) {
                result[key] = it.value();
            }
        }
        // EXISTING_PREFERRED: keep result[key] as-is.
    }
    return result;
}

double CanonicalEntityResolver::scoreFieldQuality(const std::string & /*field_name*/, const std::string &value,
                                                  const FieldQualityPolicy &policy) {
    if (value.empty()) {
        return 0.0;
    }

    double score = 1.0;
    if (policy.min_length > 0 && static_cast<int>(value.size()) < policy.min_length) {
        score *= 0.5;
    }
    if (policy.prefer_digits_only) {
        bool all_digits = std::all_of(value.begin(), value.end(),
                                      [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
        if (!all_digits) {
            score *= 0.7;
        }
    }
    if (policy.prefer_upper_case) {
        bool has_lower = std::any_of(value.begin(), value.end(),
                                     [](char c) { return std::islower(static_cast<unsigned char>(c)); });
        if (has_lower) {
            score *= 0.9;
        }
    }
    return score;
}

std::string CanonicalEntityResolver::bestStringValue(const std::string &v1, const std::string &v2,
                                                     ResolutionPolicy policy) {
    if (v1.empty()) {
        return v2;
    }
    if (v2.empty()) {
        return v1;
    }
    switch (policy) {
        case ResolutionPolicy::NEWEST_FIRST:
            return (v1 >= v2) ? v1 : v2;
        case ResolutionPolicy::MOST_COMPLETE:
        [[fallthrough]];\n        case ResolutionPolicy::RICHEST_MERGE:
            return static_cast<bool>((v1.size()  < static_cast<int>(= v2.size()))) ? v1 : v2;
        case ResolutionPolicy::EXISTING_PREFERRED:
            return v1;
        case ResolutionPolicy::INCOMING_PREFERRED:
            return v2;
        default:
            return v2;
    }
}

// ---------------------------------------------------------------------------
// createGoldenRecord
// ---------------------------------------------------------------------------

GoldenRecord
CanonicalEntityResolver::createGoldenRecord(const std::vector<std::pair<std::string, json>> &linked_entities,
                                            const std::string & /*collection_name*/, ResolutionPolicy policy,
                                            const std::map<std::string, FieldRule> &field_rules,
                                            const std::vector<std::string> &protected_fields) const {
    GoldenRecord gr;
    gr.canonical_id        = generateUUID();
    gr.last_reconciliation = nowRfc3339();

    if (linked_entities.empty()) {
        gr.completeness_score = 0.0;
        return gr;
    }

    // Collect contributing IDs.
    for (const auto &[eid, _] : linked_entities) {
        gr.contributing_ids.push_back(eid);
    }

    // Select the "base" record according to policy.
    size_t base_idx = 0;
    if (policy == ResolutionPolicy::MOST_COMPLETE) {
        double best = -1.0;
        for (size_t i = 0; i < linked_entities.size(); ++i) {
            double c = computeCompleteness(linked_entities[i].second);
            if (c > best) {
                best     = c;
                base_idx = i;
            }
        }
    } else if (policy == ResolutionPolicy::NEWEST_FIRST) {
        std::string newest = {};
        for (size_t i = 0; i < linked_entities.size(); ++i) {
            const auto &e = linked_entities[i].second;
            std::string ts = {};
            if (e.contains("updated_at") && e["updated_at"].is_string()) {
                ts = e["updated_at"].get<std::string>();
            } else if (e.contains("created_at") && e["created_at"].is_string()) {
                ts = e["created_at"].get<std::string>();
            }
            if (ts > newest) {
                newest   = ts;
                base_idx = i;
            }
        }
    } else if (policy == ResolutionPolicy::INCOMING_PREFERRED) {
        base_idx = static_cast<int>(linked_entities.size()) - 1;
    }
    // EXISTING_PREFERRED → base_idx = 0 (already set).
    // RICHEST_MERGE / CUSTOM_RULES → start from 0 and iterate all.

    json merged         = linked_entities[base_idx].second;
    gr.field_provenance = json::object();
    for (auto it = merged.begin(); it != merged.end(); ++it) {
        gr.field_provenance[it.key()] = linked_entities[base_idx].first;
    }

    // Protected fields: always preserve entity[0]'s (original/existing) values,
    // regardless of the chosen base or merge policy.
    // Apply them to the initial merged state before iterating other entities.
    if (!linked_entities.empty() && !protected_fields.empty()) {
        const auto &original        = linked_entities[0].second;
        const std::string &orig_eid = linked_entities[0].first;
        for (const auto &pf : protected_fields) {
            if (original.contains(pf) && !original[pf].is_null()) {
                merged[pf]              = original[pf];
                gr.field_provenance[pf] = orig_eid;
            }
        }
    }

    // Merge remaining records.
    for (size_t i = 0; i < linked_entities.size(); ++i) {
        if (i == base_idx) {
            continue;
        }
        const auto &[eid, entity] = linked_entities[i];

        for (auto it = entity.begin(); it != entity.end(); ++it) {
            const std::string &field = it.key();

            // Protected fields are never overwritten.
            if (std::find(protected_fields.begin(), protected_fields.end(), field) != protected_fields.end()) {
                continue;
            }

            // Per-field rule overrides.
            auto rule_it = field_rules.find(field);
            if (rule_it != field_rules.end()) {
                FieldRule rule = rule_it->second;
                if (!merged.contains(field) || merged[field].is_null()) {
                    merged[field]              = it.value();
                    gr.field_provenance[field] = eid;
                } else {
                    // Apply rule.
                    if (merged[field].is_number_integer() && it.value().is_number_integer()) {
                        merged[field]
                            = reconcileNumericField(merged[field].get<int64_t>(), it.value().get<int64_t>(), rule);
                    } else {
                        const std::string v1
                            = merged[field].is_string() ? merged[field].get<std::string>() : merged[field].dump();
                        const std::string v2
                            = it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
                        merged[field] = reconcileStringField(v1, v2, rule);
                    }
                    gr.field_provenance[field] = eid;
                }
                continue;
            }

            // Default policy-based merge.
            if (!merged.contains(field) || merged[field].is_null()) {
                merged[field]              = it.value();
                gr.field_provenance[field] = eid;
            } else if (policy == ResolutionPolicy::RICHEST_MERGE || policy == ResolutionPolicy::CUSTOM_RULES) {
                if (it.value().is_object() && merged[field].is_object()) {
                    merged[field]              = reconcileObjectField(merged[field], it.value(), policy);
                    gr.field_provenance[field] = eid;
                } else {
                    const std::string v1
                        = merged[field].is_string() ? merged[field].get<std::string>() : merged[field].dump();
                    const std::string v2 = it.value().is_string() ? it.value().get<std::string>() : it.value().dump();
                    std::string best     = bestStringValue(v1, v2, policy);
                    if (best == v2) {
                        merged[field]              = it.value();
                        gr.field_provenance[field] = eid;
                    }
                }
            }
            // EXISTING_PREFERRED / INCOMING_PREFERRED / NEWEST_FIRST → keep base value.
            // (For INCOMING_PREFERRED the base was already selected as the last/newest entity.)
        }
    }

    gr.merged_data        = std::move(merged);
    gr.completeness_score = computeCompleteness(gr.merged_data);
    return gr;
}

} // namespace importers
} // namespace themis

