/**
 * @file knowledge_base.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "analytics/knowledge_base.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>

#include "utils/string_utils.h"

// Full YAML spec compliance via yaml-cpp (optional).
// Enable with -DTHEMIS_HAS_YAML_CPP=ON and link yaml-cpp.
// Default: OFF — the inline Horn-clause parser is used as permanent fallback.
#ifdef THEMIS_HAS_YAML_CPP
#  include <yaml-cpp/yaml.h>
#endif

namespace themisdb {
namespace analytics {

// ─────────────────────────────────────────────────────────────────────────────
// STUB #272 — injectable YAML parser bridge
// ─────────────────────────────────────────────────────────────────────────────

namespace {
std::mutex &yamlParserFnMutex() {
    static std::mutex m;
    return m;
}
KnowledgeBase::YamlParserFn &yamlParserFnStorage() {
    static KnowledgeBase::YamlParserFn fn;
    return fn;
}
} // namespace

/*static*/
void KnowledgeBase::setYamlParserFn(YamlParserFn fn) {
    std::lock_guard<std::mutex> lk(yamlParserFnMutex());
    yamlParserFnStorage() = std::move(fn);
}

/*static*/
void KnowledgeBase::clearYamlParserFn() {
    std::lock_guard<std::mutex> lk(yamlParserFnMutex());
    yamlParserFnStorage() = {};
}

// ──────────────────────────────────────────────────────────────────────────────
// helpers
// ──────────────────────────────────────────────────────────────────────────────

static int64_t knowledgeBaseNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// Using themis::utils::trim() from string_utils.h (Phase 1 consolidation)

static std::string stripQuotes(const std::string &s) {
    if (static_cast<int>(s.size()) >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, static_cast<int>(s.size()) - 2);
    }
    return s;
}

// ──────────────────────────────────────────────────────────────────────────────
// generateId
// ──────────────────────────────────────────────────────────────────────────────

std::string KnowledgeBase::generateId() {
    std::ostringstream oss = {};
    oss << "f_" << std::setw(6) << std::setfill('0') << id_counter_++;
    return oss.str();
}

// ──────────────────────────────────────────────────────────────────────────────
// Working Memory
// ──────────────────────────────────────────────────────────────────────────────

std::string KnowledgeBase::assertFact(const std::string &subject, const std::string &predicate,
                                      const std::string &object) {
    // Evict oldest if at capacity.
    while (static_cast<int>(insertion_order_.size()) >= kMaxFacts) {
        const auto &oldest_id = insertion_order_.front();
        const auto pred_it    = fact_id_to_predicate_.find(oldest_id);
        if (pred_it != fact_id_to_predicate_.end()) {
            auto range = facts_by_predicate_.equal_range(pred_it->second);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second.id == oldest_id) {
                    facts_by_predicate_.erase(it);
                    break;
                }
            }
            fact_id_to_predicate_.erase(pred_it);
        }
        fact_by_id_.erase(oldest_id);
        insertion_order_.pop_front();
    }

    Fact f;
    f.id             = generateId();
    f.subject        = subject;
    f.predicate      = predicate;
    f.object         = object;
    f.asserted_at_ms = knowledgeBaseNowMs();

    facts_by_predicate_.emplace(predicate, f);
    fact_id_to_predicate_[f.id] = predicate;
    fact_by_id_[f.id]           = f;
    insertion_order_.push_back(f.id);

    return f.id;
}

bool KnowledgeBase::retractFact(const std::string &fact_id) {
    const auto pred_it = fact_id_to_predicate_.find(fact_id);
    if (pred_it == fact_id_to_predicate_.end()) {
        return false;
    }

    auto range = facts_by_predicate_.equal_range(pred_it->second);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.id == fact_id) {
            facts_by_predicate_.erase(it);
            break;
        }
    }
    fact_by_id_.erase(fact_id);
    fact_id_to_predicate_.erase(pred_it);

    // Remove from insertion order (O(N) but eviction is rare).
    const auto oit = std::find(insertion_order_.begin(), insertion_order_.end(), fact_id);
    if (oit != insertion_order_.end()) {
        insertion_order_.erase(oit);
    }

    return true;
}

std::vector<Fact> KnowledgeBase::getFacts(const std::string &predicate) const {
    std::vector<Fact> result = {};

    if (predicate.empty()) {
        result.reserve(fact_by_id_.size());
        for (const auto &[id, f] : fact_by_id_) {
            result.push_back(f);
        }
    } else {
        auto range = facts_by_predicate_.equal_range(predicate);
        for (auto it = range.first; it != range.second; ++it) {
            result.push_back(it->second);
        }
    }
    return result;
}

std::optional<Fact> KnowledgeBase::getFactById(const std::string &id) const {
    const auto it = fact_by_id_.find(id);
    if (it == fact_by_id_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void KnowledgeBase::clearFacts() {
    facts_by_predicate_.clear();
    fact_id_to_predicate_.clear();
    fact_by_id_.clear();
    insertion_order_.clear();
}

// ──────────────────────────────────────────────────────────────────────────────
// Rule Store
// ──────────────────────────────────────────────────────────────────────────────

void KnowledgeBase::addRule(HornClause rule) {
    // Remove previous rule with the same id if present.
    auto it = std::find_if(rules_.begin(), rules_.end(), [&]([[maybe_unused]] const HornClause &r) { return r.id == rule.id; });
    if (it != rules_.end()) {
        rules_.erase(it);
    }
    rules_.push_back(std::move(rule));
}

bool KnowledgeBase::removeRule(const std::string &rule_id) {
    const auto it = std::find_if(rules_.begin(), rules_.end(), [&]([[maybe_unused]] const HornClause &r) { return r.id == rule_id; });
    if (it == rules_.end()) {
        return false;
    }
    rules_.erase(it);
    return true;
}

std::vector<HornClause> KnowledgeBase::getRules() const {
    auto sorted = rules_;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const HornClause &a, const HornClause &b) { return a.priority > b.priority; });
    return sorted;
}

void KnowledgeBase::clearRules() {
    rules_.clear();
}

// ──────────────────────────────────────────────────────────────────────────────
// YAML Rule Loader
// PERMANENT FALLBACK NOTE:
// The inline parser handles only the specific ThemisDB Horn-clause YAML format.
// Complex YAML (anchors, aliases, multi-line scalar values) is not supported.
// When THEMIS_HAS_YAML_CPP is defined, yaml-cpp is used for full spec compliance
// and the inline parser is not called.
// ──────────────────────────────────────────────────────────────────────────────

static TriplePattern parseTriplePattern(const std::string &line) {
    // Expected: "- [?subject, predicate, object]" or "  - [...]"
    TriplePattern tp;
    const auto lb = line.find('[');
    const auto rb = line.find(']');
    if (lb == std::string::npos || rb == std::string::npos || rb <= lb) {
        return tp;
    }

    const std::string inner = line.substr(lb + 1, rb - lb - 1);
    // Split by commas.
    std::vector<std::string> parts;
    std::istringstream ss(inner);
    std::string token = {};
    while (std::getline(ss, token, ',')) {
        parts.push_back(themis::utils::trim(stripQuotes(token)));
    }

    if (static_cast<int>(parts.size()) >= 1) {
        tp.subject = parts[0];
    }
    if (static_cast<int>(parts.size()) >= 2) {
        tp.predicate = parts[1];
    }
    if (static_cast<int>(parts.size()) >= 3) {
        // Join remaining parts (object may contain commas in quoted form).
        tp.object = parts[2];
        for (std::size_t i = 3; i < parts.size(); ++i) {
            tp.object += "," + parts[i];
        }
    }
    return tp;
}

int KnowledgeBase::loadRulesFromYaml(const std::string &path) {
    // Delegate to injected full-featured parser when set.
    YamlParserFn fn_copy;
    {
        std::lock_guard<std::mutex> lk(yamlParserFnMutex());
        fn_copy = yamlParserFnStorage();
    }
    if (fn_copy) {
        return fn_copy(path, *this);
    }

#ifdef THEMIS_HAS_YAML_CPP
    // ── Full YAML spec compliance via yaml-cpp (THEMIS_HAS_YAML_CPP=ON) ─────
    // Parses the ThemisDB Horn-clause YAML format with full support for
    // anchors, aliases, multi-line scalars, and complex nested structures.
    try {
        YAML::Node root = YAML::LoadFile(path);
        if (!root["rules"] || !root["rules"].IsSequence()) {
            return 0;
        }

        int loaded = 0;
        for (const auto& rule_node : root["rules"]) {
            HornClause hc = {};

            if (rule_node["id"]) {
                hc.id = rule_node["id"].as<std::string>("");
            }
            if (hc.id.empty()) {
              continue;
            }

            if (rule_node["priority"]) {
                hc.priority = rule_node["priority"].as<int>(0);
            }
            if (rule_node["description"]) {
                hc.description = rule_node["description"].as<std::string>("");
            }
            if (rule_node["ml_confidence_threshold"]) {
                hc.ml_confidence_threshold =
                    rule_node["ml_confidence_threshold"].as<double>(0.0);
            }

            auto parseTripleSeq = [](const YAML::Node& seq) {
                std::vector<TriplePattern> triples = {};

                if (!seq || !seq.IsSequence()) {
                  return triples;
                }
                for (const auto& item : seq) {
                    TriplePattern tp = {};
                    if (item.IsSequence() && static_cast<int>(item.size()) >= 3) {
                        tp.subject   = item[0].as<std::string>("");
                        tp.predicate = item[1].as<std::string>("");
                        tp.object    = item[2].as<std::string>("");
                    } else if (item.IsMap()) {
                        tp.subject   = item["subject"]   ? item["subject"].as<std::string>("") : "";
                        tp.predicate = item["predicate"] ? item["predicate"].as<std::string>("") : "";
                        tp.object    = item["object"]    ? item["object"].as<std::string>("") : "";
                    }
                    if (!tp.subject.empty() || !tp.predicate.empty()) {
                        triples.push_back(std::move(tp));
                    }
                }
                return triples;
            };

            if (rule_node["conditions"]) {
                hc.conditions = parseTripleSeq(rule_node["conditions"]);
            }
            if (rule_node["consequents"]) {
                hc.consequents = parseTripleSeq(rule_node["consequents"]);
            }

            addRule(std::move(hc));
            ++loaded;
        }
        return loaded;
    } catch (const YAML::Exception& e) {
        // yaml-cpp parse error — fall through to inline fallback below.
        (void)e;
    }
    // ── End yaml-cpp path ──────────────────────────────────────────────────
#endif // THEMIS_HAS_YAML_CPP

    // PERMANENT FALLBACK NOTE:
    // Built-in line-parser for the specific ThemisDB Horn-clause YAML format.
    // Handles only the subset used by FUTURE_ENHANCEMENTS.md.  Complex YAML
    // (anchors, aliases, multi-line scalars) is not supported here.
    // Enable THEMIS_HAS_YAML_CPP=ON to use the yaml-cpp path above.
    std::ifstream file(path);
    if (!file.is_open()) {
        return -1;
    }

    int loaded = 0;
    HornClause current;
    bool in_rule        = false;
    bool in_conditions  = false;
    bool in_consequents = false;
    bool in_rules_block = false;

    auto flushRule = [&]() {
        if (in_rule && !current.id.empty()) {
            addRule(current);
            ++loaded;
        }
        current        = HornClause{};
        in_rule        = false;
        in_conditions  = false;
        in_consequents = false;
    };

    std::string line = {};
    while (std::getline(file, line)) {
        const std::string t = themis::utils::trim(line);

        // Top-level block marker.
        if (t == "rules:") {
            in_rules_block = true;
            continue;
        }
        if (!in_rules_block) {
            continue;
        }

        // New rule entry.
        if (t.substr(0, 5) == "- id:") {
            flushRule();
            current.id = themis::utils::trim(stripQuotes(t.substr(5)));
            in_rule    = true;
            continue;
        }
        if (!in_rule) {
            continue;
        }

        if (t.substr(0, 9) == "priority:") {
            try { current.priority = std::stoi(themis::utils::trim(t.substr(9))); }
            catch (const std::exception&) { current.priority = 0; }
            continue;
        }
        if (t.substr(0, 12) == "description:") {
            current.description = themis::utils::trim(stripQuotes(t.substr(12)));
            continue;
        }
        if (t.substr(0, 25) == "ml_confidence_threshold:") {
            try { current.ml_confidence_threshold = std::stod(themis::utils::trim(t.substr(25))); }
            catch (const std::exception&) { current.ml_confidence_threshold = 0.0; }
            continue;
        }

        if (t.find("- [") != std::string::npos) {
            TriplePattern tp = parseTriplePattern(t);
            if (!tp.subject.empty() || !tp.predicate.empty()) {
                if (in_conditions) {
                    current.conditions.push_back(tp);
                }
                if (in_consequents) {
                    current.consequents.push_back(tp);
                }
            }
        }
    }
    flushRule();
    return loaded;
}

} // namespace analytics
} // namespace themisdb
