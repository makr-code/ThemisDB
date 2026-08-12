/**
 * @file process_model_generator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB – Process Modeling Module
 *
 * File:    process_model_generator.cpp
 * Module:  src/process/
 * Purpose: LLM-to-BPMN generator implementation.
 */

#include "process/process_model_generator.h"
#include <stdexcept>
#include "utils/logger.h"

#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace process {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Map domain enum to JSON string and back.
std::string domainToStr(ProcessDomain d) {
    switch (d) {
        case ProcessDomain::ADMINISTRATION:   return "ADMINISTRATION";
        case ProcessDomain::BUSINESS:         return "BUSINESS";
        case ProcessDomain::IT_SERVICE:       return "IT_SERVICE";
        case ProcessDomain::HEALTHCARE:       return "HEALTHCARE";
        case ProcessDomain::FINANCE:          return "FINANCE";
        case ProcessDomain::CUSTOMER_SERVICE: return "CUSTOMER_SERVICE";
        default:                              return "CUSTOM";
    }
}

ProcessDomain strToDomain(const std::string& s) {
    if (s == "ADMINISTRATION")   return ProcessDomain::ADMINISTRATION;
    if (s == "IT_SERVICE")       return ProcessDomain::IT_SERVICE;
    if (s == "HEALTHCARE")       return ProcessDomain::HEALTHCARE;
    if (s == "FINANCE")          return ProcessDomain::FINANCE;
    if (s == "CUSTOMER_SERVICE") return ProcessDomain::CUSTOMER_SERVICE;
    return ProcessDomain::BUSINESS;
}

/// Normalise an LLM node type string to the canonical uppercase form used
/// by ProcessModelManager::buildNormalizedGraph_().
std::string normalizeNodeType(const std::string& t) {
    // Already uppercase canonical form
    if (t == "START_EVENT" || t == "END_EVENT" || t == "TASK" ||
        t == "EXCLUSIVE_GATEWAY" || t == "PARALLEL_GATEWAY" ||
        t == "INCLUSIVE_GATEWAY" || t == "SUBPROCESS" ||
        t == "CALL_ACTIVITY" || t == "INTERMEDIATE_EVENT") {
        return t;
    }
    // LLM camelCase → canonical uppercase
    if (t == "startEvent")        return "START_EVENT";
    if (t == "endEvent")          return "END_EVENT";
    if (t == "exclusiveGateway")  return "EXCLUSIVE_GATEWAY";
    if (t == "parallelGateway")   return "PARALLEL_GATEWAY";
    if (t == "inclusiveGateway")  return "INCLUSIVE_GATEWAY";
    if (t == "intermediateEvent") return "INTERMEDIATE_EVENT";
    if (t == "subProcess")        return "SUBPROCESS";
    if (t == "callActivity")      return "CALL_ACTIVITY";
    // userTask, serviceTask, scriptTask, etc. → TASK
    return "TASK";
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// setLlmBackend
// ─────────────────────────────────────────────────────────────────────────────

void ProcessModelGenerator::setLlmBackend(LlmBackend backend) {
    llm_backend_ = std::move(backend);
}

// ─────────────────────────────────────────────────────────────────────────────
// buildGenerationPrompt_
// ─────────────────────────────────────────────────────────────────────────────

std::string ProcessModelGenerator::buildGenerationPrompt_(
    std::string_view description,
    const Config&    cfg) const
{
    std::ostringstream oss;

    if (cfg.language == "de") {
        oss << "Du bist ein BPMN 2.0-Experte für Verwaltungsprozesse.\n"
            << "Erstelle ein vollständiges Prozessmodell als JSON für den folgenden Prozess:\n\n"
            << description << "\n\n"
            << "Domäne: " << domainToStr(cfg.domain) << "\n\n"
            << "Ausgabe NUR als valides JSON-Objekt (kein Markdown, kein Fließtext):\n"
            << "{\n"
            << "  \"id\": \"<slug_ohne_leerzeichen>\",\n"
            << "  \"name\": \"<Prozessname>\",\n"
            << "  \"domain\": \"" << domainToStr(cfg.domain) << "\",\n"
            << "  \"events\": [\n"
            << "    {\"id\": \"s1\", \"type\": \"startEvent\", \"name\": \"Antrag eingegangen\"},\n"
            << "    {\"id\": \"e1\", \"type\": \"endEvent\",   \"name\": \"Bescheid erteilt\"}\n"
            << "  ],\n"
            << "  \"activities\": [\n"
            << "    {\"id\": \"a1\", \"name\": \"Vollständigkeitsprüfung\", \"type\": \"userTask\", \"sla_hours\": 48}\n"
            << "  ],\n"
            << "  \"gateways\": [\n"
            << "    {\"id\": \"g1\", \"name\": \"Vollständig?\", \"type\": \"exclusiveGateway\"}\n"
            << "  ],\n"
            << "  \"edges\": [\n"
            << "    {\"id\": \"f1\", \"from\": \"s1\", \"to\": \"a1\", \"type\": \"sequenceFlow\"}\n"
            << "  ]\n"
            << "}\n"
            << "Pflichtregeln:\n"
            << "- Genau ein startEvent-Knoten\n"
            << "- Mindestens ein endEvent-Knoten\n"
            << "- Keine isolierten Knoten (jeder Knoten hat mind. eine Kante)\n"
            << "- Jedes Gateway hat mind. eine ausgehende Kante\n"
            << "- Mindestens 3 Aktivitäten\n";
    } else {
        oss << "You are a BPMN 2.0 expert for administrative processes.\n"
            << "Create a complete process model as JSON for the following process:\n\n"
            << description << "\n\n"
            << "Domain: " << domainToStr(cfg.domain) << "\n\n"
            << "Output ONLY a valid JSON object (no markdown, no prose):\n"
            << "{\n"
            << "  \"id\": \"<slug_no_spaces>\",\n"
            << "  \"name\": \"<ProcessName>\",\n"
            << "  \"domain\": \"" << domainToStr(cfg.domain) << "\",\n"
            << "  \"events\":     [{\"id\":\"s1\",\"type\":\"startEvent\",\"name\":\"Request received\"},"
            <<                    "{\"id\":\"e1\",\"type\":\"endEvent\",\"name\":\"Decision issued\"}],\n"
            << "  \"activities\": [{\"id\":\"a1\",\"name\":\"Completeness check\",\"type\":\"userTask\",\"sla_hours\":48}],\n"
            << "  \"gateways\":   [{\"id\":\"g1\",\"name\":\"Complete?\",\"type\":\"exclusiveGateway\"}],\n"
            << "  \"edges\":      [{\"id\":\"f1\",\"from\":\"s1\",\"to\":\"a1\",\"type\":\"sequenceFlow\"}]\n"
            << "}\n"
            << "Required rules:\n"
            << "- Exactly one startEvent node\n"
            << "- At least one endEvent node\n"
            << "- No isolated nodes (every node must have at least one edge)\n"
            << "- Every gateway must have at least one outgoing edge\n"
            << "- At least 3 activities\n";
    }

    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// buildFixPrompt_
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ std::string ProcessModelGenerator::buildFixPrompt_(
    const json&                    current_json,
    const std::vector<std::string>& errors,
    std::string_view               language)
{
    std::ostringstream oss;
    const std::string err_prefix =
        (language == "de") ? "Korrigiere folgende Fehler im Prozessmodell:"
                           : "Fix the following errors in the process model:";
    oss << err_prefix << "\n";
    for (const auto& e : errors) oss << "  - " << e << "\n";
    oss << "\nAktuelles Modell:\n" << current_json.dump(2) << "\n\n"
        << "Ausgabe NUR als valides JSON-Objekt (gleiche Struktur, Fehler behoben).\n";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// extractJson_
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ json ProcessModelGenerator::extractJson_(const std::string& llm_text) {
    // Try parsing the whole text first
    try {
        return json::parse(llm_text);
    } catch (...) {}

    // Extract the first JSON object from the text (LLMs often wrap with prose)
    const auto start = llm_text.find('{');
    const auto end   = llm_text.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return json{};
    }
    try {
        return json::parse(llm_text.substr(start, end - start + 1));
    } catch (...) {
        return json{};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// validate
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ ProcessModelGenerator::ValidationResult ProcessModelGenerator::validate(
    const json& normalized_graph)
{
    ValidationResult result;

    if (!normalized_graph.contains("nodes") || !normalized_graph.contains("edges")) {
        result.ok = false;
        result.errors.push_back("Missing 'nodes' or 'edges' key in graph");
        return result;
    }

    bool has_start = false;
    bool has_end   = false;
    std::set<std::string> all_node_ids;
    std::map<std::string, std::string> node_types; // id → type (deterministic)
    std::map<std::string, int>         out_degree;
    std::map<std::string, int>         in_degree;

    for (const auto& n : normalized_graph["nodes"]) {
        std::string nid   = n.value("id", "");
        std::string ntype = n.value("type", "");
        if (nid.empty()) continue;
        all_node_ids.insert(nid);
        node_types[nid] = ntype;
        out_degree[nid]  = 0;
        in_degree[nid]   = 0;

        // Check both canonical (START_EVENT) and LLM camelCase (startEvent) forms
        if (ntype == "startEvent" || ntype == "START_EVENT")  has_start = true;
        if (ntype == "endEvent"   || ntype == "END_EVENT")    has_end   = true;
    }

    for (const auto& e : normalized_graph["edges"]) {
        std::string from = e.value("from", "");
        std::string to   = e.value("to", "");
        if (all_node_ids.count(from)) ++out_degree[from];
        if (all_node_ids.count(to))   ++in_degree[to];
    }

    if (!has_start) {
        result.ok = false;
        result.errors.push_back("No startEvent node found");
    }
    if (!has_end) {
        result.ok = false;
        result.errors.push_back("No endEvent node found");
    }

    for (const auto& nid : all_node_ids) {
        if (out_degree[nid] == 0 && in_degree[nid] == 0) {
            result.ok = false;
            result.errors.push_back("Isolated node: " + nid);
        }
        const auto& nt = node_types[nid];
        const bool is_gateway =
            (nt == "exclusiveGateway" || nt == "parallelGateway" || nt == "inclusiveGateway" ||
             nt == "EXCLUSIVE_GATEWAY" || nt == "PARALLEL_GATEWAY" || nt == "INCLUSIVE_GATEWAY");
        if (is_gateway && out_degree[nid] == 0) {
            result.ok = false;
            result.errors.push_back("Gateway '" + nid + "' has no outgoing edges");
        }
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// fromLlmJson
// ─────────────────────────────────────────────────────────────────────────────

/*static*/ ProcessModelRecord ProcessModelGenerator::fromLlmJson(
    const json&   llm_json,
    ProcessDomain domain)
{
    ProcessModelRecord rec;
    rec.id       = llm_json.value("id", "generated_proc");
    rec.name     = llm_json.value("name", rec.id);
    rec.notation = ProcessNotation::BPMN_2_0;
    rec.state    = ProcessModelState::DRAFT;

    // Domain from JSON or default
    if (llm_json.contains("domain") && llm_json["domain"].is_string()) {
        rec.domain = strToDomain(llm_json["domain"].get<std::string>());
    } else {
        rec.domain = domain;
    }

    // Build normalised graph
    json norm_nodes = json::array();
    json norm_edges = json::array();

    // Events
    if (llm_json.contains("events") && llm_json["events"].is_array()) {
        for (const auto& ev : llm_json["events"]) {
            json node;
            node["id"]   = ev.value("id", "");
            node["name"] = ev.value("name", "");
            node["type"] = normalizeNodeType(ev.value("type", "startEvent"));
            norm_nodes.push_back(node);
        }
    }

    // Activities
    if (llm_json.contains("activities") && llm_json["activities"].is_array()) {
        for (const auto& act : llm_json["activities"]) {
            json node;
            node["id"]   = act.value("id", "");
            node["name"] = act.value("name", "");
            node["type"] = normalizeNodeType(act.value("type", "userTask"));
            if (act.contains("sla_hours")) node["sla_hours"] = act["sla_hours"];
            norm_nodes.push_back(node);
        }
    }

    // Gateways
    if (llm_json.contains("gateways") && llm_json["gateways"].is_array()) {
        for (const auto& gw : llm_json["gateways"]) {
            json node;
            node["id"]   = gw.value("id", "");
            node["name"] = gw.value("name", "");
            node["type"] = normalizeNodeType(gw.value("type", "exclusiveGateway"));
            norm_nodes.push_back(node);
        }
    }

    // Edges
    if (llm_json.contains("edges") && llm_json["edges"].is_array()) {
        for (const auto& e : llm_json["edges"]) {
            json edge;
            edge["id"]   = e.value("id", "");
            edge["from"] = e.value("from", "");
            edge["to"]   = e.value("to", "");
            edge["type"] = e.value("type", "SEQUENCE_FLOW");
            norm_edges.push_back(edge);
        }
    }

    rec.normalized = {{"nodes", norm_nodes}, {"edges", norm_edges}};
    return rec;
}

// ─────────────────────────────────────────────────────────────────────────────
// generateFromDescription
// ─────────────────────────────────────────────────────────────────────────────

std::pair<bool, ProcessModelRecord> ProcessModelGenerator::generateFromDescription(
    std::string_view description,
    const Config&    cfg) const
{
    if (!llm_backend_) {
        SPDLOG_ERROR("[ProcessModelGenerator] No LLM backend configured");
        return {false, {}};
    }

    std::string prompt = buildGenerationPrompt_(description, cfg);
    json current_json;

    for (int attempt = 0; attempt < cfg.max_retries; ++attempt) {
        const std::string llm_response = llm_backend_(prompt);
        if (llm_response.empty()) {
            SPDLOG_WARN("[ProcessModelGenerator] Empty LLM response on attempt {}", attempt + 1);
            continue;
        }

        current_json = extractJson_(llm_response);
        if (current_json.empty()) {
            SPDLOG_WARN("[ProcessModelGenerator] Invalid JSON from LLM on attempt {}", attempt + 1);
            prompt = buildFixPrompt_(current_json,
                                     {"LLM response is not valid JSON"},
                                     cfg.language);
            continue;
        }

        // Convert to ProcessModelRecord and validate
        ProcessModelRecord rec = fromLlmJson(current_json, cfg.domain);
        ValidationResult vr = validate(rec.normalized);
        if (vr.ok) {
            SPDLOG_INFO("[ProcessModelGenerator] Generated model '{}' after {} attempt(s)",
                        rec.id, attempt + 1);
            return {true, std::move(rec)};
        }

        SPDLOG_WARN("[ProcessModelGenerator] Validation failed on attempt {}: {}",
                    attempt + 1, vr.errors.empty() ? "unknown" : vr.errors[0]);
        prompt = buildFixPrompt_(current_json, vr.errors, cfg.language);
    }

    SPDLOG_ERROR("[ProcessModelGenerator] Failed to generate valid model after {} retries",
                 cfg.max_retries);
    return {false, {}};
}

// ─────────────────────────────────────────────────────────────────────────────
// refine
// ─────────────────────────────────────────────────────────────────────────────

std::pair<bool, ProcessModelRecord> ProcessModelGenerator::refine(
    const ProcessModelRecord& existing,
    std::string_view          feedback,
    const Config&             cfg) const
{
    if (!llm_backend_) {
        SPDLOG_ERROR("[ProcessModelGenerator] No LLM backend configured");
        return {false, existing};
    }

    // Build a refinement prompt
    std::ostringstream oss;
    if (cfg.language == "de") {
        oss << "Verbessere das folgende BPMN-Prozessmodell basierend auf dem Feedback:\n\n"
            << "Feedback: " << feedback << "\n\n"
            << "Aktuelles Modell:\n"
            << existing.normalized.dump(2) << "\n\n"
            << "Ausgabe NUR als valides JSON-Objekt (gleiche Struktur).\n";
    } else {
        oss << "Refine the following BPMN process model based on the feedback:\n\n"
            << "Feedback: " << feedback << "\n\n"
            << "Current model:\n"
            << existing.normalized.dump(2) << "\n\n"
            << "Output ONLY a valid JSON object (same structure).\n";
    }

    json current_json;
    std::string prompt = oss.str();

    for (int attempt = 0; attempt < cfg.max_retries; ++attempt) {
        const std::string llm_response = llm_backend_(prompt);
        if (llm_response.empty()) continue;

        current_json = extractJson_(llm_response);
        if (current_json.empty()) {
            prompt = buildFixPrompt_(current_json, {"Invalid JSON response"}, cfg.language);
            continue;
        }

        ProcessModelRecord refined = fromLlmJson(current_json, existing.domain);
        // Preserve the original model ID
        refined.id = existing.id;
        refined.version = existing.version;

        ValidationResult vr = validate(refined.normalized);
        if (vr.ok) return {true, std::move(refined)};

        prompt = buildFixPrompt_(current_json, vr.errors, cfg.language);
    }

    SPDLOG_WARN("[ProcessModelGenerator] Refinement failed; returning original model");
    return {false, existing};
}

} // namespace process
} // namespace themis

