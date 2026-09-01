/**
 * @file vcc_vpb_importer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=13, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    vcc_vpb_importer.cpp
 * Module:  src/process/
 * Purpose: Import VCC-VPB YAML process definitions into ThemisDB.
 *
 * VCC-VPB (Visual Change Control – Visual Process Builder) is the native
 * process authoring tool for the ThemisDB ecosystem.  It produces YAML files
 * using the schema documented in `config/process_models/README.md`.
 *
 * Implementation note: the YAML parser is a minimal hand-written parser that
 * covers the subset of YAML used by VCC-VPB (single-level keys, simple lists,
 * indented activity/edge blocks). Full YAML 1.2 constructs (anchors, custom
 * tags, multi-document streams) are not supported.
 */

#include "process/vcc_vpb_importer.h"
#include "utils/logger.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <stdexcept>

namespace themis {
namespace process {

namespace fs = std::filesystem;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Domain mapping
// ---------------------------------------------------------------------------

ProcessDomain VccVpbImporter::domainStringToEnum_(std::string_view domain) {
    std::string d(domain);
    std::transform(d.begin(), d.end(), d.begin(), ::tolower);

    if (d == "bauwesen"   || d == "verwaltung" || d == "beschaffung" ||
        d == "personal"   || d == "haushalt"   || d == "administration")
        return ProcessDomain::ADMINISTRATION;
    if (d == "it"         || d == "it_service" || d == "itsm")
        return ProcessDomain::IT_SERVICE;
    if (d == "gesundheit" || d == "healthcare")
        return ProcessDomain::HEALTHCARE;
    if (d == "finanzen"   || d == "finance")
        return ProcessDomain::FINANCE;
    if (d == "kundenservice" || d == "customer_service")
        return ProcessDomain::CUSTOMER_SERVICE;
    if (d == "business")
        return ProcessDomain::BUSINESS;
    return ProcessDomain::CUSTOM;
}

// ---------------------------------------------------------------------------
// Activity type mapping
// ---------------------------------------------------------------------------

BPMNNodeType VccVpbImporter::activityTypeToNodeType_(std::string_view type_str) {
    std::string t(type_str);
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);

    if (t == "start")   return BPMNNodeType::START_EVENT;
    if (t == "end")     return BPMNNodeType::END_EVENT;
    if (t == "gateway" || t == "xor_gateway") return BPMNNodeType::EXCLUSIVE_GATEWAY;
    if (t == "parallel_gateway" || t == "and_gateway")
                        return BPMNNodeType::PARALLEL_GATEWAY;
    if (t == "inclusive_gateway" || t == "or_gateway")
                        return BPMNNodeType::INCLUSIVE_GATEWAY;
    if (t == "subprocess" || t == "sub_process")
                        return BPMNNodeType::SUBPROCESS;
    if (t == "user_task" || t == "user")
                        return BPMNNodeType::TASK; // subtype=USER_TASK
    if (t == "service_task" || t == "service")
                        return BPMNNodeType::TASK; // subtype=SERVICE_TASK
    return BPMNNodeType::TASK; // default
}

// ---------------------------------------------------------------------------
// Edge type mapping
// ---------------------------------------------------------------------------

ProcessEdgeType VccVpbImporter::edgeTypeToProcessEdgeType_(std::string_view type_str) {
    std::string t(type_str);
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);

    if (t == "conditional")    return ProcessEdgeType::CONDITIONAL_FLOW;
    if (t == "default")        return ProcessEdgeType::DEFAULT_FLOW;
    if (t == "exception"  || t == "error")
                               return ProcessEdgeType::EXCEPTION_FLOW;
    if (t == "message")        return ProcessEdgeType::MESSAGE_FLOW;
    return ProcessEdgeType::SEQUENCE_FLOW;
}

// ---------------------------------------------------------------------------
// Minimal YAML-to-JSON converter
// ---------------------------------------------------------------------------
//
// VCC-VPB YAML is a well-structured subset. We convert it to JSON so that
// the rest of the code can use nlohmann::json idioms uniformly.
//
// Handles:
//   - Top-level key: value pairs
//   - Nested mappings (indented with 2/4 spaces)
//   - Lists (- item)
//   - Quoted and unquoted strings
//   - Inline lists: [a, b, c]
//   - Numbers

namespace {

std::string trimStr(const std::string& s) {
    std::string out = s;
    out.erase(0, out.find_first_not_of(" \t\r\n"));
    out.erase(out.find_last_not_of(" \t\r\n") + 1);
    return out;
}

std::string unquote(const std::string& s) {
    if (s.size() >= 2) {
        if ((s.front() == '"' && s.back() == '"') ||
            (s.front() == '\'' && s.back() == '\'')) {
            return s.substr(1, s.size() - 2);
        }
    }
    return s;
}

// Very simple YAML → JSON conversion for VCC-VPB format
// This is NOT a full YAML parser; it handles the specific schema used.
json parseVccVpbYaml(const std::string& yaml_text) {
    // We handle three cases:
    // 1. The document starts with a top-level mapping (process definition)
    // 2. The document starts with a list key (e.g. administrative_models:)
    //    containing an array of process definitions
    //
    // Strategy: split into lines and build a JSON object iteratively.

    std::vector<std::string> lines;
    std::istringstream ss(yaml_text);
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }

    // We build the result as a JSON object.
    // For simplicity we use a stack-based approach.
    json result = json::object();

    // State machine — extremely simplified
    struct Frame {
        json* target;       // JSON object/array to write into
        int   indent;       // indentation level that opened this frame
        bool  is_array;     // true if we're inside a list
        std::string list_key; // key that holds the list
    };

    std::vector<Frame> stack;
    stack.push_back({&result, -1, false, ""});

    std::string current_list_key;

    auto indentOf = [](const std::string& l) -> int {
        int i = 0;
        while (i < (int)l.size() && (l[i] == ' ' || l[i] == '\t')) ++i;
        return i;
    };

    // Two-pass: first collect all top-level keys and their raw blocks
    // Pass 1: simple key: scalar extraction + special handling of list blocks

    // We'll parse the most important fields from VCC-VPB directly
    // using a robust line-by-line approach.

    // Because writing a full YAML parser is out of scope, we use regex patterns
    // for the key fields that VCC-VPB uses and build a JSON manually.

    json doc = json::object();

    // Top-level scalar fields
    auto extractTopLevel = [&](const std::string& key) -> std::string {
        std::regex re("^" + key + R"(\s*:\s*["\']?([^"\'\n]+)["\']?\s*$)");
        for (const auto& l : lines) {
            std::smatch m;
            if (std::regex_match(l, m, re)) {
                return unquote(trimStr(m[1].str()));
            }
        }
        return {};
    };

    doc["id"]          = extractTopLevel("id");
    doc["name"]        = extractTopLevel("name");
    doc["name_en"]     = extractTopLevel("name_en");
    doc["domain"]      = extractTopLevel("domain");
    doc["domain_en"]   = extractTopLevel("domain_en");
    doc["description"] = extractTopLevel("description");
    doc["description_en"] = extractTopLevel("description_en");
    doc["version"]     = extractTopLevel("version");
    doc["owner"]       = extractTopLevel("owner");

    // Compliance list: compliance: ["a", "b", "c"] or multi-line
    {
        json comp_list = json::array();
        
        // Pre-compile regexes outside loops (performance optimization)
        // These static const regexes are compiled once at first use, not per iteration
        static const std::regex inline_re(R"(^compliance\s*:\s*\[([^\]]*)\])");  // NOLINT(readability-static-accessed-through-instance)
        static const std::regex item_re(R"(["\']([^"\']+)["\']|(\w[^\s,\]]*))");  // NOLINT(readability-static-accessed-through-instance)
        static const std::regex multiline_header_re(R"(^compliance\s*:)");  // NOLINT(readability-static-accessed-through-instance)
        static const std::regex multiline_item_re(R"(^\s*-\s*["\']?([^"\']+)["\']?\s*$)");  // NOLINT(readability-static-accessed-through-instance)
        
        // Try inline list first
        bool found_inline = false;
        for (const auto& l : lines) {
            std::smatch m;
            if (std::regex_search(l, m, inline_re)) {  // NOLINT(clang-diagnostic-error) - regex is static const
                std::string items = m[1].str();
                auto it  = std::sregex_iterator(items.begin(), items.end(), item_re);  // NOLINT(clang-diagnostic-error) - regex is static const
                auto end = std::sregex_iterator();
                for (; it != end; ++it) {
                    std::string val = (*it)[1].matched ? (*it)[1].str() : (*it)[2].str();
                    if (!val.empty()) comp_list.push_back(val);
                }
                found_inline = true;
                break;
            }
        }

        if (!found_inline) {
            // Look for multi-line compliance list
            bool in_compliance = false;
            for (const auto& l : lines) {
                if (std::regex_match(l, multiline_header_re)) {  // NOLINT(clang-diagnostic-error) - regex is static const
                    in_compliance = true;
                    continue;
                }
                if (in_compliance) {
                    // Input validation: check bounds before accessing array index
                    if (l.empty() || (l.size() > 0 && l[0] != ' ')) { 
                        in_compliance = false; 
                        continue; 
                    }
                    std::smatch m;
                    if (std::regex_match(l, m, multiline_item_re)) {  // NOLINT(clang-diagnostic-error) - regex is static const
                        comp_list.push_back(trimStr(m[1].str()));
                    }
                }
            }
        }
        doc["compliance"] = comp_list;
    }

    // Parse activities block
    {
        json activities = json::array();
        bool in_activities = false;
        bool in_activity   = false;
        json current_activity = json::object();
        
        // Pre-compile regexes for activities block (static const, compiled once)
        static const std::regex activities_header_re(R"(^activities\s*:)");  // NOLINT(readability-static-accessed-through-instance)
        static const std::regex activity_dash_re(R"(^-\s*$)");  // NOLINT(readability-static-accessed-through-instance)
        static const std::regex activity_kv_re(R"((\w+)\s*:\s*["\']?([^"\'\n]+)["\']?)");  // NOLINT(readability-static-accessed-through-instance)

        for (size_t i = 0; i < lines.size(); ++i) {
            const auto& l = lines[i];
            std::string trimmed = trimStr(l);
            int indent = indentOf(l);

            if (std::regex_match(l, activities_header_re)) {  // NOLINT(clang-diagnostic-error) - regex is static const
                in_activities = true;
                continue;
            }

            if (!in_activities) continue;

            // Detect end of activities block (new top-level key with indent 0)
            if (indent == 0 && !trimmed.empty() && trimmed[0] != '-') {
                if (in_activity && !current_activity.empty()) {
                    activities.push_back(current_activity);
                    current_activity = json::object();
                    in_activity = false;
                }
                in_activities = false;
                continue;
            }

            // New activity item
            if (std::regex_match(trimmed, activity_dash_re) ||  // NOLINT(clang-diagnostic-error) - regex is static const
                (trimmed.size() >= 2 && trimmed.substr(0, 2) == "- ")) {

                if (in_activity && !current_activity.empty()) {
                    activities.push_back(current_activity);
                    current_activity = json::object();
                }
                in_activity    = true;

                // Check if first key is on same line: "- id: foo"
                std::string rest = (trimmed.size() > 2) ? trimStr(trimmed.substr(2)) : "";
                if (!rest.empty()) {
                    std::smatch m;
                    if (std::regex_search(rest, m, activity_kv_re)) {  // NOLINT(clang-diagnostic-error) - regex is static const
                        current_activity[m[1].str()] = unquote(trimStr(m[2].str()));
                    }
                }
                continue;
            }

            if (in_activity) {
                // Parse key: value inside activity
                std::smatch m;
                if (std::regex_search(trimmed, m, activity_kv_re)) {  // NOLINT(clang-diagnostic-error) - regex is static const
                    current_activity[m[1].str()] = unquote(trimStr(m[2].str()));
                }
            }
        }

        if (in_activity && !current_activity.empty()) {
            activities.push_back(current_activity);
        }
        doc["activities"] = activities;
    }

    // Parse edges block (same pattern as activities)
    {
        json edges = json::array();
        bool in_edges  = false;
        bool in_edge   = false;
        json current_edge = json::object();
        
        // Pre-compile regexes for edges block (static const, compiled once)
        static const std::regex edges_header_re(R"(^edges\s*:)");  // NOLINT(readability-static-accessed-through-instance)
        static const std::regex edge_dash_re(R"(^-\s*$)");  // NOLINT(readability-static-accessed-through-instance)
        static const std::regex edge_kv_re(R"((\w+)\s*:\s*["\']?([^"\'\n]+)["\']?)");  // NOLINT(readability-static-accessed-through-instance)

        for (const auto& l : lines) {
            std::string trimmed = trimStr(l);
            int indent = indentOf(l);

            if (std::regex_match(l, edges_header_re)) {  // NOLINT(clang-diagnostic-error) - regex is static const
                in_edges = true;
                continue;
            }

            if (!in_edges) continue;

            if (indent == 0 && !trimmed.empty() && trimmed[0] != '-') {
                if (in_edge && !current_edge.empty()) {
                    edges.push_back(current_edge);
                    current_edge = json::object();
                    in_edge = false;
                }
                in_edges = false;
                continue;
            }

            if (std::regex_match(trimmed, edge_dash_re) ||  // NOLINT(clang-diagnostic-error) - regex is static const
                (trimmed.size() >= 2 && trimmed.substr(0, 2) == "- ")) {

                if (in_edge && !current_edge.empty()) {
                    edges.push_back(current_edge);
                    current_edge = json::object();
                }
                in_edge = true;

                std::string rest = (trimmed.size() > 2) ? trimStr(trimmed.substr(2)) : "";
                if (!rest.empty()) {
                    std::smatch m;
                    if (std::regex_search(rest, m, edge_kv_re)) {  // NOLINT(clang-diagnostic-error) - regex is static const
                        current_edge[m[1].str()] = unquote(trimStr(m[2].str()));
                    }
                }
                continue;
            }

            if (in_edge) {
                std::smatch m;
                if (std::regex_search(trimmed, m, edge_kv_re)) {  // NOLINT(clang-diagnostic-error) - regex is static const
                    current_edge[m[1].str()] = unquote(trimStr(m[2].str()));
                }
            }
        }

        if (in_edge && !current_edge.empty()) {
            edges.push_back(current_edge);
        }
        doc["edges"] = edges;
    }

    return doc;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// parseModelNode_  (internal)
// ---------------------------------------------------------------------------

VccVpbImporter::ImportResult VccVpbImporter::parseModelNode_(
    const json& doc,
    const ProcessModelRecord& meta_defaults)
{
    ImportResult result;

    ProcessModelRecord& rec = result.record;

    // Apply defaults first
    rec = meta_defaults;

    // Override with document data
    std::string id = doc.value("id", "");
    if (!id.empty()) rec.id = id;
    if (rec.id.empty()) {
        result.ok      = false;
        result.message = "VCC-VPB model is missing 'id' field";
        return result;
    }

    std::string name = doc.value("name", "");
    if (!name.empty()) rec.name = name;
    rec.name_en     = doc.value("name_en", rec.name_en);
    rec.description = doc.value("description", rec.description);
    rec.description_en = doc.value("description_en", rec.description_en);
    rec.version     = doc.value("version", rec.version.empty() ? "1.0.0" : rec.version);
    rec.owner       = doc.value("owner", rec.owner);
    rec.notation    = ProcessNotation::VCC_VPB;

    std::string domain_str = doc.value("domain", "");
    if (!domain_str.empty()) {
        rec.domain = domainStringToEnum_(domain_str);
    }

    if (doc.contains("compliance") && doc["compliance"].is_array()) {
        rec.compliance_tags.clear();
        for (const auto& tag : doc["compliance"]) {
            if (tag.is_string()) rec.compliance_tags.push_back(tag.get<std::string>());
        }
    }

    // Build nodes and edges
    std::vector<ProcessNodeInfo> nodes;
    std::vector<ProcessEdgeInfo> edges;

    if (doc.contains("activities") && doc["activities"].is_array()) {
        for (const auto& act : doc["activities"]) {
            ProcessNodeInfo n;
            n.node_id     = act.value("id", "");
            n.name        = act.value("name", n.node_id);
            n.description = act.value("description", "");
            n.subtype     = "";

            if (n.node_id.empty()) continue;

            std::string type_str = act.value("type", "task");
            BPMNNodeType btype   = activityTypeToNodeType_(type_str);
            n.node_type  = btype;

            // Determine subtype for task variants
            std::string t_lower = type_str;
            std::transform(t_lower.begin(), t_lower.end(), t_lower.begin(), ::tolower);
            if (t_lower == "user_task" || t_lower == "user")      n.subtype = "USER_TASK";
            else if (t_lower == "service_task" || t_lower == "service") n.subtype = "SERVICE_TASK";

            // SLA → timeout
            if (act.contains("sla_hours")) {
                double sla_h = 0.0;
                if (act["sla_hours"].is_number()) {
                    sla_h = act["sla_hours"].get<double>();
                } else if (act["sla_hours"].is_string()) {
                    try { sla_h = std::stod(act["sla_hours"].get<std::string>()); }
                    catch (...) {}
                }
                if (sla_h > 0) {
                    n.timeout = std::chrono::milliseconds(
                        static_cast<int64_t>(sla_h * 3600000.0));
                }
            }

            // Responsible role → store in description
            std::string role = act.value("responsible_role", "");
            if (!role.empty()) {
                if (n.description.empty()) n.description = "Role: " + role;
                else                       n.description += " | Role: " + role;
            }

            nodes.push_back(std::move(n));
        }
    }

    if (doc.contains("edges") && doc["edges"].is_array()) {
        int edge_counter = 0;
        for (const auto& e : doc["edges"]) {
            ProcessEdgeInfo edge;
            edge.edge_id   = e.value("id", "edge_" + std::to_string(++edge_counter));
            edge.from_node = e.value("from", "");
            edge.to_node   = e.value("to",   "");
            if (edge.from_node.empty() || edge.to_node.empty()) continue;

            edge.edge_type = edgeTypeToProcessEdgeType_(e.value("type", "sequence"));

            std::string cond = e.value("condition", "");
            if (!cond.empty()) edge.condition_expression = cond;

            edges.push_back(std::move(edge));
        }
    }

    // Build normalised JSON graph
    json normalized;
    normalized["process_id"] = rec.id;
    normalized["name"]       = rec.name;
    normalized["domain"]     = std::string(toString(rec.domain));
    normalized["notation"]   = "VCC_VPB";

    json jnodes = json::array();
    for (const auto& n : nodes) {
        json jn;
        jn["id"]   = n.node_id;
        jn["name"] = n.name;
        jn["description"] = n.description;
        jn["subtype"] = n.subtype;

        if (std::holds_alternative<BPMNNodeType>(n.node_type)) {
            auto t = std::get<BPMNNodeType>(n.node_type);
            switch (t) {
                case BPMNNodeType::START_EVENT:        jn["type"] = "START_EVENT"; break;
                case BPMNNodeType::END_EVENT:          jn["type"] = "END_EVENT"; break;
                case BPMNNodeType::EXCLUSIVE_GATEWAY:  jn["type"] = "EXCLUSIVE_GATEWAY"; break;
                case BPMNNodeType::PARALLEL_GATEWAY:   jn["type"] = "PARALLEL_GATEWAY"; break;
                case BPMNNodeType::INCLUSIVE_GATEWAY:  jn["type"] = "INCLUSIVE_GATEWAY"; break;
                case BPMNNodeType::SUBPROCESS:         jn["type"] = "SUBPROCESS"; break;
                default:                               jn["type"] = "TASK"; break;
            }
        } else {
            jn["type"] = "TASK";
        }
        jn["notation"] = "BPMN";

        if (n.timeout) {
            jn["timeout_ms"] = n.timeout->count();
        }
        jnodes.push_back(std::move(jn));
    }
    normalized["nodes"] = std::move(jnodes);

    json jedges = json::array();
    for (const auto& e : edges) {
        json je;
        je["id"]   = e.edge_id;
        je["from"] = e.from_node;
        je["to"]   = e.to_node;
        je["condition"] = e.condition_expression.value_or("");
        switch (e.edge_type) {
            case ProcessEdgeType::CONDITIONAL_FLOW: je["type"] = "CONDITIONAL_FLOW"; break;
            case ProcessEdgeType::DEFAULT_FLOW:     je["type"] = "DEFAULT_FLOW"; break;
            case ProcessEdgeType::EXCEPTION_FLOW:   je["type"] = "EXCEPTION_FLOW"; break;
            case ProcessEdgeType::MESSAGE_FLOW:     je["type"] = "MESSAGE_FLOW"; break;
            default:                                je["type"] = "SEQUENCE_FLOW"; break;
        }
        jedges.push_back(std::move(je));
    }
    normalized["edges"] = std::move(jedges);

    rec.normalized = normalized;

    result.ok      = true;
    result.message = "OK";
    return result;
}

// ---------------------------------------------------------------------------
// importYaml
// ---------------------------------------------------------------------------

VccVpbImporter::ImportResult VccVpbImporter::importYaml(
    std::string_view      yaml_text,
    const ProcessModelRecord& meta_defaults)
{
    try {
        json doc = parseVccVpbYaml(std::string(yaml_text));

        // Store raw payload in the record
        ImportResult result = parseModelNode_(doc, meta_defaults);
        if (result.ok) {
            result.record.raw_payload = std::string(yaml_text);
        }
        return result;
    } catch (const std::exception& ex) {
        return {false, std::string("YAML parse error: ") + ex.what(), {}};
    }
}

// ---------------------------------------------------------------------------
// importYamlList
// ---------------------------------------------------------------------------

std::vector<VccVpbImporter::ImportResult> VccVpbImporter::importYamlList(
    std::string_view          yaml_text,
    std::string_view          list_key,
    const ProcessModelRecord& meta_defaults)
{
    std::vector<ImportResult> results;

    // Locate list_key block and extract individual model definitions
    // Exception-safe: all allocations are RAII-managed (std::string, std::vector)
    std::string text(yaml_text);  // RAII-managed string copy
    std::string key_pattern = std::string(list_key) + ":";  // RAII-managed string

    size_t list_start = text.find(key_pattern);
    if (list_start == std::string::npos) {
        results.push_back({false, "List key '" + std::string(list_key) + "' not found", {}});
        return results;
    }

    // Each model is introduced by "  - id:" or "  -\n    id:"
    // Split the block into individual model YAML chunks.
    std::string block = text.substr(list_start + key_pattern.size());  // RAII-managed substring
    std::istringstream ss(block);
    std::string line;  // RAII-managed line buffer
    std::vector<std::string> model_chunks;  // RAII-managed vector
    std::string current_chunk;  // RAII-managed current chunk

    // Limit to prevent resource exhaustion (DoS protection)
    static constexpr size_t MAX_LINES = 100000;
    size_t line_count = 0;

    while (std::getline(ss, line) && line_count < MAX_LINES) {
        ++line_count;
         
        // Input validation: explicit bounds checking on user-controlled input
        const size_t line_len = line.size();
         
        // Check for start of new model (line starts with 2 spaces + "- ")
        // Defensive bounds checking to prevent out-of-bounds access
        bool is_new_model = (line_len >= 4 && 
                            line[0] == ' ' && line[1] == ' ' &&
                            line[2] == '-' && line[3] == ' ');
         
        // Also detect "  -\n" (just the dash, id on next line)
        bool is_dash_only = (line_len >= 3 && 
                            line[0] == ' ' && line[1] == ' ' &&
                            line[2] == '-' && 
                            (line_len == 3 || line[3] == '\r'));

        try {
            if ((is_new_model || is_dash_only) && !current_chunk.empty()) {
                model_chunks.push_back(current_chunk);
                current_chunk.clear();
            }

            if (is_new_model) {
                // Remove the leading "  - " (safe after bounds check)
                if (line_len > 4) {
                    current_chunk += line.substr(4) + "\n";
                } else {
                    current_chunk += "\n";
                }
            } else if (is_dash_only) {
                // Just a dash; content on subsequent lines
            } else if (!line.empty() && line_len >= 2 && 
                       line[0] == ' ' && line[1] == ' ') {
                // Indented content belonging to current model — remove 4 spaces indent
                if (line_len > 4) {
                    current_chunk += line.substr(4) + "\n";
                } else if (line_len > 2) {
                    current_chunk += line.substr(2) + "\n";
                } else {
                    current_chunk += "\n";
                }
            } else if (line.empty()) {
                if (!current_chunk.empty()) current_chunk += "\n";
            } else {
                // Top-level key outside the list — stop
                break;
            }
        } catch (const std::exception& e) {
            SPDLOG_WARN("[vcc_vpb_importer] Exception processing line {}: {}", line_count, e.what());
            continue;
        }
    }

    if (!current_chunk.empty()) {
        model_chunks.push_back(current_chunk);
    }

    if (model_chunks.empty()) {
        results.push_back({false, "No models found under key '" + std::string(list_key) + "'", {}});
        return results;
    }

    for (const auto& chunk : model_chunks) {
        results.push_back(importYaml(chunk, meta_defaults));
    }

    return results;
}

// ---------------------------------------------------------------------------
// importDirectory
// ---------------------------------------------------------------------------

std::vector<VccVpbImporter::ImportResult> VccVpbImporter::importDirectory(
    std::string_view          directory_path,
    const ProcessModelRecord& meta_defaults)
{
    std::vector<ImportResult> results;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(
                                     std::string(directory_path))) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            if (ext != ".yaml" && ext != ".yml") continue;

            std::ifstream f(entry.path());
            if (!f.is_open()) {
                results.push_back({false, "Cannot open: " + entry.path().string(), {}});
                continue;
            }

            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());

            // Try as list first, then as single model
            auto list_results = importYamlList(content, "administrative_models", meta_defaults);
            if (!list_results.empty() && list_results[0].ok) {
                for (auto& r : list_results) results.push_back(std::move(r));
            } else {
                results.push_back(importYaml(content, meta_defaults));
            }
        }
    } catch (const std::exception& ex) {
        results.push_back({false, std::string("Directory scan error: ") + ex.what(), {}});
    }

    return results;
}

} // namespace process
} // namespace themis

