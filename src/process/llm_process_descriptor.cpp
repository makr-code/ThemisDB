/**
 * @file llm_process_descriptor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    llm_process_descriptor.cpp
 * Module:  src/process/
 * Purpose: Generate LLM-optimised JSON descriptors and prompt strings
 *          for process model records.
 */

#include "process/llm_process_descriptor.h"
#include "process/process_model_manager.h"
#include "utils/logger.h"

#include <sstream>
#include <algorithm>
#include <cmath>

namespace themis {
namespace process {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

std::string LlmProcessDescriptor::truncate_(std::string_view s, size_t max_chars) {
    if (s.size() <= max_chars) {
      return std::string(s);
    }
    return std::string(s.substr(0, max_chars)) + "…";
}

json LlmProcessDescriptor::nodeToJson_(const json& node_doc, const Config& cfg) {
    json n;
    n["id"]          = node_doc.value("id",   "");
    n["name"]        = node_doc.value("name", "");
    n["type"]        = node_doc.value("type", "TASK");
    n["subtype"]     = node_doc.value("subtype", "");
    n["description"] = truncate_(node_doc.value("description", ""), cfg.max_description_chars);

    if (node_doc.contains("timeout_ms")) {
        n["timeout_hours"] = node_doc["timeout_ms"].get<double>() / 3600000.0;
    }
    return n;
}

json LlmProcessDescriptor::edgeToJson_(const json& edge_doc) {
    json e;
    e["from"]      = edge_doc.value("from", "");
    e["to"]        = edge_doc.value("to",   "");
    e["type"]      = edge_doc.value("type", "SEQUENCE_FLOW");
    std::string cond = edge_doc.value("condition", "");
    if (!cond.empty()) {
        e["condition"] = cond;
    }
    return e;
}

// ---------------------------------------------------------------------------
// generate
// ---------------------------------------------------------------------------

json LlmProcessDescriptor::generate(const ProcessModelRecord& record)
{
    return generate(record, Config{});
}

json LlmProcessDescriptor::generate(
    const ProcessModelRecord& record,
    const Config& cfg)
{
    json desc;

    // Identity
    desc["process_id"] = record.id;
    desc["name"]       = cfg.language == "en" && !record.name_en.empty()
                         ? record.name_en : record.name;
    desc["notation"]   = std::string(toString(record.notation));
    desc["domain"]     = std::string(toString(record.domain));
    desc["state"]      = std::string(toString(record.state));
    desc["version"]    = record.version;

    // Description
    std::string summary = cfg.language == "en" && !record.description_en.empty()
                          ? record.description_en : record.description;
    if (summary.empty()) {
      summary = record.long_description;
    }
    desc["summary"] = truncate_(summary, cfg.max_description_chars);

    // Compliance
    desc["compliance"] = record.compliance_tags;

    // Nodes
    json nodes_array = json::array();
    size_t total_sla_ms = 0;
    if (record.normalized.contains("nodes")) {
        for (const auto& jn : record.normalized["nodes"]) {
            nodes_array.push_back(nodeToJson_(jn, cfg));
            if (jn.contains("timeout_ms") && jn["timeout_ms"].is_number()) {
                total_sla_ms += static_cast<size_t>(jn["timeout_ms"].get<double>());
            }
        }
    }
    desc["nodes"] = std::move(nodes_array);

    // Edges
    json edges_array = json::array();
    if (record.normalized.contains("edges")) {
        for (const auto& je : record.normalized["edges"]) {
            edges_array.push_back(edgeToJson_(je));
        }
    }
    desc["edges"] = std::move(edges_array);

    // SLA summary
    if (total_sla_ms > 0) {
        desc["sla_total_hours"] = static_cast<double>(total_sla_ms) / 3600000.0;
    }

    // Ownership / metadata
    desc["owner"]       = record.owner;
    desc["created_at"]  = record.created_at_ms;
    desc["updated_at"]  = record.updated_at_ms;

    if (cfg.include_raw_payload && !record.raw_payload.empty()) {
        desc["raw_payload"] = record.raw_payload;
    }

    if (cfg.include_embedding && !record.embedding.empty()) {
        desc["embedding"] = record.embedding;
    }

    // Build condensed LLM context string
    desc["llm_context"] = buildSystemPrompt(desc);

    return desc;
}

// ---------------------------------------------------------------------------
// buildSystemPrompt
// ---------------------------------------------------------------------------

std::string LlmProcessDescriptor::buildSystemPrompt(const json& descriptor) {
    std::ostringstream prompt = {};

    prompt << "=== Process Model: " << descriptor.value("name", "Unknown") << " ===\n";
    prompt << "ID: "       << descriptor.value("process_id", "") << "\n";
    prompt << "Notation: " << descriptor.value("notation", "BPMN_2_0") << "\n";
    prompt << "Domain: "   << descriptor.value("domain", "BUSINESS") << "\n";
    prompt << "State: "    << descriptor.value("state", "ACTIVE") << "\n";

    if (!descriptor.value("summary", "").empty()) {
        prompt << "Summary: " << descriptor.value("summary", "") << "\n";
    }

    if (descriptor.contains("compliance") && !descriptor["compliance"].empty()) {
        prompt << "Compliance: ";
        bool first = true;
        for (const auto& tag : descriptor["compliance"]) {
            if (!first) {
              prompt << ", ";
            }
            prompt << tag.get<std::string>();
            first = false;
        }
        prompt << "\n";
    }

    if (descriptor.contains("sla_total_hours")) {
        prompt << "Total SLA: " << descriptor.value("sla_total_hours", 0.0) << " hours\n";
    }

    prompt << "\nProcess Steps:\n";
    if (descriptor.contains("nodes")) {
        for (const auto& n : descriptor["nodes"]) {
            prompt << "  [" << n.value("type", "TASK") << "] "
                   << n.value("name", "") << " (id=" << n.value("id", "") << ")";
            std::string desc_str = n.value("description", "");
            if (!desc_str.empty()) {
                prompt << " — " << desc_str;
            }
            if (n.contains("timeout_hours")) {
                prompt << " [SLA: " << n.value("timeout_hours", 0.0) << "h]";
            }
            prompt << "\n";
        }
    }

    prompt << "\nFlow:\n";
    if (descriptor.contains("edges")) {
        for (const auto& e : descriptor["edges"]) {
            prompt << "  " << e.value("from", "?") << " -> " << e.value("to", "?");
            if (e.contains("condition")) {
                prompt << " [IF: " << e.value("condition", "") << "]";
            }
            prompt << "\n";
        }
    }

    return prompt.str();
}

// ---------------------------------------------------------------------------
// summarizeList
// ---------------------------------------------------------------------------

json LlmProcessDescriptor::summarizeList(
    const std::vector<ProcessModelRecord>& records,
    std::string_view                       language)
{
    json arr = json::array();
    for (const auto& r : records) {
        json item;
        item["id"]     = r.id;
        item["name"]   = (language == "en" && !r.name_en.empty()) ? r.name_en : r.name;
        item["domain"] = std::string(toString(r.domain));
        item["state"]  = std::string(toString(r.state));
        item["notation"] = std::string(toString(r.notation));
        item["version"]  = r.version;

        std::string desc = (language == "en" && !r.description_en.empty())
                           ? r.description_en : r.description;
        item["description"] = truncate_(desc, 200);
        item["compliance"]  = r.compliance_tags;

        if (r.normalized.contains("nodes")) {
            item["node_count"] = r.normalized["nodes"].size();
        }
        if (r.normalized.contains("edges")) {
            item["edge_count"] = r.normalized["edges"].size();
        }

        arr.push_back(std::move(item));
    }
    return arr;
}

// ---------------------------------------------------------------------------
// buildConformancePrompt
// ---------------------------------------------------------------------------

std::string LlmProcessDescriptor::buildConformancePrompt(
    const json& descriptor,
    const json& observed_trace)
{
    std::ostringstream prompt = {};

    prompt << "Task: Process Conformance Checking\n\n";
    prompt << "== Expected Process Model ==\n";
    prompt << buildSystemPrompt(descriptor);

    prompt << "\n== Observed Execution Trace ==\n";
    if (observed_trace.is_array()) {
        int step = 1;
        for (const auto& activity : observed_trace) {
            prompt << "  " << step++ << ". ";
            if (activity.is_string()) {
                prompt << activity.get<std::string>();
            } else if (activity.is_object()) {
                prompt << activity.value("activity", "?");
                if (activity.contains("timestamp")) {
                    prompt << " at " << activity.value("timestamp", "");
                }
            }
            prompt << "\n";
        }
    }

    prompt << "\n== Analysis Required ==\n";
    prompt << "1. Calculate fitness score (0.0-1.0): fraction of trace that can be replayed\n";
    prompt << "2. Calculate precision score (0.0-1.0): fraction of model behaviour observed\n";
    prompt << "3. List all deviations from expected model with severity (LOW/MEDIUM/HIGH)\n";
    prompt << "4. Check compliance with: ";
    if (descriptor.contains("compliance") && !descriptor["compliance"].empty()) {
        bool first = true;
        for (const auto& tag : descriptor["compliance"]) {
            if (!first) {
              prompt << ", ";
            }
            prompt << tag.get<std::string>();
            first = false;
        }
    } else {
        prompt << "N/A";
    }
    prompt << "\n";
    prompt << "5. Assess SLA adherence\n";
    prompt << "6. Provide actionable improvement recommendations\n\n";
    prompt << "Output format: JSON with keys: fitness, precision, deviations[], "
              "compliance_violations[], sla_breaches[], recommendations[]\n";

    return prompt.str();
}

} // namespace process
} // namespace themis
