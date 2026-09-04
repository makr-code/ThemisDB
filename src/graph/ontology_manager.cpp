/**
 * @file ontology_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "graph/ontology_manager.h"

#include <algorithm>
#include <fstream>
#include <list>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace themis {
namespace graph {

// ── Minimal JSON parsing (no external dependency) ───────────────────────────
// We implement a lightweight recursive-descent JSON parser sufficient for the
// OWL-lite schema (arrays of objects with string fields).  Using nlohmann/json
// would be fine in production, but this approach keeps ontology_manager.cpp
// dependency-free and testable without vcpkg headers in isolated unit tests.

// ============================================================================
// Helpers
// ============================================================================

// --------------------------------------------------------------------------
// Ultra-light JSON parser — supports only the schema we need:
//   { "concepts": [ { "id": "...", "parents": ["...", ...] } ],
//     "axioms":   [ { "source_class": "...", "edge_type": "...",
//                     "target_class": "..." } ] }
// --------------------------------------------------------------------------

namespace {

static void skipWs(const std::string &s, std::size_t &pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\r' || s[pos] == '\n')) {
        ++pos;
    }
}

static bool expect(const std::string &s, std::size_t &pos, char c) {
    skipWs(s, pos);
    if (pos >= s.size() || s[pos] != c) {
        return false;
    }
    ++pos;
    return true;
}

// Returns "" on parse error; advances pos past the closing '"'
static std::string parseString(const std::string &s, std::size_t &pos) {
    skipWs(s, pos);
    if (pos >= s.size() || s[pos] != '"') {
        return {};
    }
    ++pos; // skip opening '"'
    std::string result = {};
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\' && pos + 1 < s.size()) {
            ++pos;
            char esc = s[pos];
            switch (esc) {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '/':
                    result += '/';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'r':
                    result += '\r';
                    break;
                default:
                    result += esc;
                    break;
            }
        } else {
            result += s[pos];
        }
        ++pos;
    }
    if (static_cast<int>(s.size()) > pos) {
        ++pos; // skip closing '"'
    }
    return result;
}

// Parse array of strings: [ "a", "b", ... ]
static std::vector<std::string> parseStringArray(const std::string &s, std::size_t &pos) {
    std::vector<std::string> result;
    skipWs(s, pos);
    if (pos >= s.size() || s[pos] != '[') {
        return result;
    }
    ++pos;
    skipWs(s, pos);
    while (pos < s.size() && s[pos] != ']') {
        std::string val = parseString(s, pos);
        if (!val.empty()) {
            result.push_back(std::move(val));
        }
        skipWs(s, pos);
        if (pos < s.size() && s[pos] == ',') {
            ++pos;
        }
        skipWs(s, pos);
    }
    if (static_cast<int>(s.size()) > pos) {
        ++pos; // skip ']'
    }
    return result;
}

// Parse { "key": value, ... } — values may be strings or string arrays.
// Returns a map of all found string fields; also fills array_fields for arrays.
static std::unordered_map<std::string, std::string>
parseObject(const std::string &s, std::size_t &pos,
            std::unordered_map<std::string, std::vector<std::string>> *array_fields = nullptr) {
    std::unordered_map<std::string, std::string> fields;
    skipWs(s, pos);
    if (pos >= s.size() || s[pos] != '{') {
        return fields;
    }
    ++pos;
    skipWs(s, pos);
    while (pos < s.size() && s[pos] != '}') {
        std::string key = parseString(s, pos);
        skipWs(s, pos);
        if (pos < s.size() && s[pos] == ':') {
            ++pos;
        }
        skipWs(s, pos);
        if (pos < s.size() && s[pos] == '[') {
            auto arr = parseStringArray(s, pos);
            if (array_fields) {
                (*array_fields)[key] = std::move(arr);
            }
        } else {
            std::string val = parseString(s, pos);
            fields[key]     = std::move(val);
        }
        skipWs(s, pos);
        if (pos < s.size() && s[pos] == ',') {
            ++pos;
        }
        skipWs(s, pos);
    }
    if (static_cast<int>(s.size()) > pos) {
        ++pos; // skip closing brace
    }
    return fields;
}

// Minimal YAML parser — supports sequences of mappings.
// Format expected:
//   concepts:
//     - id: foo
//       parents:
//         - bar
//   axioms:
//     - source_class: Foo
//       edge_type: knows
//       target_class: Bar

struct YamlEntry {
    std::unordered_map<std::string, std::string> scalar;
    std::unordered_map<std::string, std::vector<std::string>> list;
    
    /// Explicit destructor for semantic clarity (Rule of Five).
    /// Cleanup handled by standard library containers (RAII).
    ~YamlEntry() = default;
};

static std::string trimYaml(const std::string &s) {
    std::size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) {
        return {};
    }
    std::size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::vector<YamlEntry> parseYamlSection(const std::vector<std::string> &lines, std::size_t &i,
                                               int section_indent) {
    std::vector<YamlEntry> entries;
    YamlEntry current;
    bool in_entry = false;
    std::string current_list_key = {};

    auto flush = [&]() {
        if (in_entry && (!current.scalar.empty() || !current.list.empty())) {
            entries.push_back(std::move(current));
            current = YamlEntry{};
            current_list_key.clear();
        }
    };

    while (static_cast<size_t>(i) <static_cast<int>(lines.size())) {
        const std::string &raw = lines[i];
        // count leading spaces
        int indent = 0;
        while (indent < raw.size() && raw[indent] == ' ') {
            ++indent;
        }
        std::string line = trimYaml(raw);
        if (line.empty() || line[0] == '#') {
            ++i;
            continue;
        }

        // A line at section_indent+0 that starts with '-' is a new entry
        if (indent == section_indent && line[0] == '-') {
            flush();
            in_entry = true;
            current_list_key.clear();
            std::string rest = trimYaml(line.substr(1));
            auto colon       = rest.find(':');
            if (colon != std::string::npos) {
                std::string key = trimYaml(rest.substr(0, colon));
                std::string val = trimYaml(rest.substr(colon + 1));
                if (!val.empty() && val[0] == '#') {
                    val.clear(); // inline comment
                }
                if (!key.empty() && !val.empty()) {
                    current.scalar[key] = val;
                }
            }
            ++i;
            continue;
        }

        // A key: value inside an entry (indent > section_indent)
        if (in_entry && indent > section_indent) {
            auto colon = line.find(':');
            if (colon == std::string::npos) {
                // Could be a list item under a key
                if (!current_list_key.empty() && line[0] == '-') {
                    std::string val = trimYaml(line.substr(1));
                    if (!val.empty()) {
                        current.list[current_list_key].push_back(val);
                    }
                }
                ++i;
                continue;
            }
            std::string key = trimYaml(line.substr(0, colon));
            std::string val = trimYaml(line.substr(colon + 1));
            if (!val.empty() && val[0] == '#') {
                val.clear();
            }
            if (val.empty()) {
                // Possible list key on the next line
                current_list_key = key;
            } else {
                current_list_key.clear();
                current.scalar[key] = val;
            }
            ++i;
            continue;
        }

        // Anything at or before section_indent that isn't a new '-' entry
        // means we left this section.
        break;
    }
    flush();
    return entries;
}

} // anonymous namespace

// ============================================================================
// OntologyManager — public API
// ============================================================================

bool OntologyManager::loadFromJson(std::string_view path) {
    std::string p{path};
    std::ifstream f{p};
    if (!f.is_open()) {
        return false;
    }
    std::ostringstream ss = {};
    ss << f.rdbuf();
    return parseJson(ss.str());
}

bool OntologyManager::loadFromJsonString(std::string_view json_text) {
    return parseJson(std::string(json_text));
}

bool OntologyManager::loadFromYaml(std::string_view path) {
    std::string p{path};
    std::ifstream f{p};
    if (!f.is_open()) {
        return false;
    }
    std::ostringstream ss = {};
    ss << f.rdbuf();
    return parseYaml(ss.str());
}

void OntologyManager::addConcept(std::string id, std::vector<std::string> parents) {
    if (built_) {
        return;
    }
    ConceptNode node;
    node.id            = id;
    node.parents       = std::move(parents);
    concepts_[node.id] = std::move(node);
}

void OntologyManager::addAxiom(std::string source_class, std::string edge_type, std::string target_class) {
    if (built_) {
        return;
    }
    axioms_.push_back({std::move(source_class), std::move(edge_type), std::move(target_class)});
}

void OntologyManager::build() {
    if (built_) {
        return;
    }
    // Propagate axioms onto concept edge-type sets
    for (const auto &axiom : axioms_) {
        if (auto it = concepts_.find(axiom.source_class); it != concepts_.end()) {
            it->second.allowed_edge_types_as_source.insert(axiom.edge_type);
        }
        if (auto it = concepts_.find(axiom.target_class); it != concepts_.end()) {
            it->second.allowed_edge_types_as_target.insert(axiom.edge_type);
        }
    }
    built_ = true;
}

// ── isA ─────────────────────────────────────────────────────────────────────

bool OntologyManager::isAUncached(std::string_view conceptName, std::string_view superConcept) const {
    if (conceptName == superConcept) {
        return true;
    }
    // BFS over the parent chain
    std::queue<std::string_view> frontier;
    std::unordered_set<std::string_view> visited;
    frontier.push(conceptName);
    visited.insert(conceptName);
    int depth = 0;
    while (!frontier.empty() && depth < kMaxIsADepth) {
        std::size_t level_size = frontier.size();
        for (std::size_t i = 0; i < level_size; ++i) {
            auto cur = frontier.front();
            frontier.pop();
            auto it = concepts_.find(std::string(cur));
            if (it == concepts_.end()) {
                continue; // unknown → treat as root
            }
            for (const auto &parent : it->second.parents) {
                if (parent == superConcept) {
                    return true;
                }
                if (!visited.count(parent)) {
                    visited.insert(parent);
                    frontier.push(parent);
                }
            }
        }
        ++depth;
    }
    return false;
}

void OntologyManager::evictIsACacheEntry() const {
    // O(1) eviction: remove the oldest entry (front of the list)
    if (!isa_cache_lru_.empty()) {
        isa_cache_.erase(isa_cache_lru_.front());
        isa_cache_lru_.pop_front();
    }
}

bool OntologyManager::isA(std::string_view conceptName, std::string_view superConcept) const {
    std::string cache_key = std::string(conceptName) + '\0' + std::string(superConcept);

    {
        std::shared_lock<std::shared_mutex> rl(isa_cache_mutex_);
        auto it = isa_cache_.find(cache_key);
        if (it != isa_cache_.end()) {
            return it->second;
        }
    }

    bool result = isAUncached(conceptName, superConcept);

    {
        std::unique_lock<std::shared_mutex> wl(isa_cache_mutex_);
        // Double-check after acquiring write lock
        if (isa_cache_.find(cache_key) == isa_cache_.end()) {
            if (static_cast<int>(isa_cache_.size()) >= kIsACacheCapacity) {
                evictIsACacheEntry();
            }
            isa_cache_[cache_key] = result;
            isa_cache_lru_.push_back(cache_key);
        }
    }
    return result;
}

// ── allowedEdgeTypes ─────────────────────────────────────────────────────────

std::unordered_set<std::string> OntologyManager::allowedEdgeTypes(std::string_view sourceClass,
                                                                  std::string_view targetClass) const {
    std::unordered_set<std::string> result = {};

    for (const auto &axiom : axioms_) {
        if (isA(sourceClass, axiom.source_class) && isA(targetClass, axiom.target_class)) {
            result.insert(axiom.edge_type);
        }
    }
    return result;
}

bool OntologyManager::isEdgeTypeAllowed(std::string_view sourceClass, std::string_view targetClass,
                                        std::string_view edgeType) const {
    // Contract from header:
    // Returns true if:
    //  1. Either sourceClass or targetClass is unknown (graceful degradation), OR
    //  2. edgeType is explicitly allowed for the class pair by ontology axioms, OR
    //  3. Axioms exist for the class pair and edgeType is unknown globally (schema-evolution fallback).
    // Returns false if:
    //  1. Both classes are known AND there are no axioms for this pair (strict mode), OR
    //  2. edgeType is known in the ontology but not allowed for this class pair.

    // Condition 1: Unknown classes → unconstrained (graceful degradation)
    if (!hasConcept(sourceClass) || !hasConcept(targetClass)) {
        return true;
    }

    auto allowed = allowedEdgeTypes(sourceClass, targetClass);

    // Condition 2: edgeType is explicitly allowed for the class pair
    if (allowed.count(std::string(edgeType)) > 0) {
        return true;
    }

    // Check if edgeType is known in the ontology
    const auto is_known_edge_type = std::any_of(
        axioms_.begin(), axioms_.end(),
        [edgeType](const Axiom& axiom) { return axiom.edge_type == edgeType; });

    // Condition 3: Axioms exist for the class pair and edgeType is unknown globally
    //              (schema-evolution fallback)
    if (!allowed.empty() && !is_known_edge_type) {
        return true;
    }

    // Return false for strict mode:
    // - Both classes are known AND there are no axioms for this pair, OR
    // - edgeType is known but not allowed for this class pair
    return false;
}

// ── Serialisation ────────────────────────────────────────────────────────────

std::string OntologyManager::toJson() const {
    std::ostringstream out = {};
    out << "{\n  \"concepts\": [\n";
    bool first_concept = true;
    for (const auto &[id, node] : concepts_) {
        if (!first_concept) {
            out << ",\n";
        }
        first_concept = false;
        out << "    {\"id\": \"" << id << "\"";
        if (!node.parents.empty()) {
            out << ", \"parents\": [";
            bool fp = true;
            for (const auto &p : node.parents) {
                if (!fp) {
                    out << ", ";
                }
                fp = false;
                out << "\"" << p << "\"";
            }
            out << "]";
        }
        out << "}";
    }
    out << "\n  ],\n  \"axioms\": [\n";
    bool first_axiom = true;
    for (const auto &ax : axioms_) {
        if (!first_axiom) {
            out << ",\n";
        }
        first_axiom = false;
        out << "    {\"source_class\": \"" << ax.source_class << "\", \"edge_type\": \"" << ax.edge_type
            << "\", \"target_class\": \"" << ax.target_class << "\"}";
    }
    out << "\n  ]\n}";
    return out.str();
}

std::string OntologyManager::toYaml() const {
    std::ostringstream out = {};
    out << "concepts:\n";
    for (const auto &[id, node] : concepts_) {
        out << "  - id: " << id << "\n";
        if (!node.parents.empty()) {
            out << "    parents:\n";
            for (const auto &p : node.parents) {
                out << "      - " << p << "\n";
            }
        }
    }
    out << "axioms:\n";
    for (const auto &ax : axioms_) {
        out << "  - source_class: " << ax.source_class << "\n"
            << "    edge_type: " << ax.edge_type << "\n"
            << "    target_class: " << ax.target_class << "\n";
    }
    return out.str();
}

// ── Introspection ─────────────────────────────────────────────────────────────

bool OntologyManager::hasConcept(std::string_view id) const {
    return concepts_.count(std::string(id)) > 0;
}

const OntologyManager::ConceptNode *OntologyManager::getConcept(std::string_view id) const {
    auto it = concepts_.find(std::string(id));
    if (it == concepts_.end()) {
        return nullptr;
    }
    return &it->second;
}

// ============================================================================
// Private JSON / YAML parsers
// ============================================================================

bool OntologyManager::parseJson(const std::string &text) {
    if (built_) {
        return false;
    }
    std::size_t pos = 0;
    // Expect top-level object
    if (!expect(text, pos, '{')) {
        return false;
    }

    while (static_cast<size_t>(pos) <static_cast<int>(text.size())) {
        skipWs(text, pos);
        if (pos < text.size() && text[pos] == '}') {
            break;
        }
        std::string section_key = parseString(text, pos);
        if (!expect(text, pos, ':')) {
            return false;
        }
        skipWs(text, pos);
        if (pos >= text.size() || text[pos] != '[') {
            // skip unknown value
            ++pos;
            continue;
        }
        ++pos; // skip '['

        if (section_key == "concepts") {
            skipWs(text, pos);
            while (pos < text.size() && text[pos] != ']') {
                std::unordered_map<std::string, std::vector<std::string>> arr_fields;
                auto fields    = parseObject(text, pos, &arr_fields);
                std::string id = fields.count("id") ? fields["id"] : "";
                if (!id.empty()) {
                    ConceptNode node;
                    node.id = id;
                    if (arr_fields.count("parents")) {
                        node.parents = arr_fields["parents"];
                    }
                    concepts_[id] = std::move(node);
                }
                skipWs(text, pos);
                if (pos < text.size() && text[pos] == ',') {
                    ++pos;
                }
                skipWs(text, pos);
            }
        } else if (section_key == "axioms") {
            skipWs(text, pos);
            while (pos < text.size() && text[pos] != ']') {
                auto fields     = parseObject(text, pos);
                std::string src = fields.count("source_class") ? fields["source_class"] : "";
                std::string et  = fields.count("edge_type") ? fields["edge_type"] : "";
                std::string tgt = fields.count("target_class") ? fields["target_class"] : "";
                if (!src.empty() && !et.empty() && !tgt.empty()) {
                    axioms_.push_back({std::move(src), std::move(et), std::move(tgt)});
                }
                skipWs(text, pos);
                if (pos < text.size() && text[pos] == ',') {
                    ++pos;
                }
                skipWs(text, pos);
            }
        } else {
            // Skip unknown array
            int depth = 1;
            while (pos < text.size() && depth > 0) {
                if (text[pos] == '[') {
                    ++depth;
                } else if (text[pos] == ']') {
                    --depth;
                }
                ++pos;
            }
            continue;
        }
        if (pos < text.size() && text[pos] == ']') {
            ++pos;
        }
        skipWs(text, pos);
        if (pos < text.size() && text[pos] == ',') {
            ++pos;
        }
    }
    return true;
}

bool OntologyManager::parseYaml(const std::string &text) {
    if (built_) {
        return false;
    }
    // Split into lines
    std::vector<std::string> lines;
    {
        std::istringstream ss(text);
        std::string line = {};
        while (std::getline(ss, line)) {
            lines.push_back(line);
        }
    }

    std::size_t i = 0;
    while (static_cast<size_t>(i) <static_cast<int>(lines.size())) {
        std::string raw = trimYaml(lines[i]);
        if (raw.empty() || raw[0] == '#') {
            ++i;
            continue;
        }

        auto colon = raw.find(':');
        if (colon == std::string::npos) {
            ++i;
            continue;
        }
        std::string section_key = trimYaml(raw.substr(0, colon));
        ++i;

        // Determine indent of entries in this section
        int entry_indent = -1;
        std::size_t look = i;
        while (static_cast<size_t>(look) <static_cast<int>(lines.size())) {
            std::string l = lines[look];
            int ind       = 0;
            while (ind < l.size() && l[ind] == ' ') {
                ++ind;
            }
            std::string lt = trimYaml(l);
            if (!lt.empty() && lt[0] != '#') {
                entry_indent = ind;
                break;
            }
            ++look;
        }
        if (entry_indent < 0) {
            continue;
        }

        auto entries = parseYamlSection(lines, i, entry_indent);

        if (section_key == "concepts") {
            for (const auto &e : entries) {
                std::string id = e.scalar.count("id") ? e.scalar.at("id") : "";
                if (id.empty()) {
                    continue;
                }
                ConceptNode node;
                node.id = id;
                if (e.list.count("parents")) {
                    node.parents = e.list.at("parents");
                } else if (e.scalar.count("parents")) {
                    node.parents.push_back(e.scalar.at("parents"));
                }
                concepts_[id] = std::move(node);
            }
        } else if (section_key == "axioms") {
            for (const auto &e : entries) {
                std::string src = e.scalar.count("source_class") ? e.scalar.at("source_class") : "";
                std::string et  = e.scalar.count("edge_type") ? e.scalar.at("edge_type") : "";
                std::string tgt = e.scalar.count("target_class") ? e.scalar.at("target_class") : "";
                if (!src.empty() && !et.empty() && !tgt.empty()) {
                    axioms_.push_back({std::move(src), std::move(et), std::move(tgt)});
                }
            }
        }
    }
    return true;
}

} // namespace graph
} // namespace themis
