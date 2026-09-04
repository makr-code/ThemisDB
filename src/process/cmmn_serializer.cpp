/**
 * @file cmmn_serializer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=22, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    cmmn_serializer.cpp
 * Module:  src/process/
 * Purpose: CMMN 1.1 Case Management Model and Notation import/export.
 */

#include "process/cmmn_serializer.h"
#include "process/serializer_hardening.h"
#include "process/process_diagnostics.h"
#include "process/process_common.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <spdlog/spdlog.h>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// Anonymous-namespace XML helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Maximum CMMN XML document size (10 MiB).
static constexpr size_t kMaxCmmnXmlBytes = 10u * 1024u * 1024u;

/// Strip XML namespace prefix ("cmmn:humanTask" → "humanTask").
static std::string_view stripNs(std::string_view name) {
    auto colon = name.rfind(':');
    return (colon != std::string_view::npos) ? name.substr(colon + 1) : name;
}

static std::string unescapeXml(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ) {
        if (s[i] != '&') { out += s[i++]; continue; }
        size_t semi = s.find(';', i + 1);
        if (semi == std::string_view::npos) { out += s[i++]; continue; }
        std::string_view ent = s.substr(i, semi - i + 1);
        if      (ent == "&amp;")  out += '&';
        else if (ent == "<")   out += '<';
        else if (ent == ">")   out += '>';
        else if (ent == "&quot;") out += '"';
        else if (ent == "&apos;") out += '\'';
        else                      out += std::string(ent);
        i = semi + 1;
    }
    size_t a = out.find_first_not_of(" \t\n\r");
    if (a == std::string::npos) return {};
    size_t b = out.find_last_not_of(" \t\n\r");
    return out.substr(a, b - a + 1);
}

struct XmlTag {
    std::string name;
    std::map<std::string, std::string> attrs;
    bool self_closing{false};
    bool is_close{false};
};

static void parseAttrs(std::string_view src,
                       std::map<std::string, std::string>& out)
{
    size_t i = 0;
    const size_t n = src.size();
    while (i < n) {
        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) ++i;
        if (i >= n || src[i] == '/' || src[i] == '>') break;
        size_t ns = i;
        while (i < n && src[i] != '=' && src[i] != '/' && src[i] != '>' &&
               !std::isspace(static_cast<unsigned char>(src[i]))) ++i;
        if (i <= ns) break;
        std::string attr_name = std::string(stripNs(src.substr(ns, i - ns)));
        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) ++i;
        if (i >= n || src[i] != '=') {
            if (!attr_name.empty()) out.emplace(std::move(attr_name), "true");
            continue;
        }
        ++i;
        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) ++i;
        if (i >= n) break;
        std::string attr_val;
        if (src[i] == '"' || src[i] == '\'') {
            char q = src[i++];
            size_t vs = i;
            while (i < n && src[i] != q) ++i;
            attr_val = unescapeXml(src.substr(vs, i - vs));
            if (i < n) ++i;
        } else {
            size_t vs = i;
            while (i < n && !std::isspace(static_cast<unsigned char>(src[i])) &&
                   src[i] != '>' && src[i] != '/') ++i;
            attr_val = std::string(src.substr(vs, i - vs));
        }
        if (!attr_name.empty()) out.emplace(std::move(attr_name), std::move(attr_val));
    }
}

template<typename TagCb, typename TextCb>
bool tokenizeCmmnXml(std::string_view xml, TagCb tag_cb, TextCb text_cb) {
    if (xml.size() > kMaxCmmnXmlBytes) return false;

    size_t i = 0;
    const size_t n = xml.size();

    while (i < n) {
        size_t ts = i;
        while (i < n && xml[i] != '<') ++i;
        if (i > ts) text_cb(xml.substr(ts, i - ts));
        if (i >= n) break;
        ++i;
        if (i >= n) break;

        if (i + 2 < n && xml[i]=='!' && xml[i+1]=='-' && xml[i+2]=='-') {
            i += 3;
            while (i + 2 < n && !(xml[i]=='-' && xml[i+1]=='-' && xml[i+2]=='>')) ++i;
            if (i + 2 < n) i += 3;
            continue;
        }
        if (i + 7 < n && xml.substr(i, 8) == "![CDATA[") {
            i += 8;
            size_t cs = i;
            while (i + 2 < n && !(xml[i]==']' && xml[i+1]==']' && xml[i+2]=='>')) ++i;
            text_cb(xml.substr(cs, i - cs));
            if (i + 2 < n) i += 3;
            continue;
        }
        if (xml[i] == '?') {
            while (i + 1 < n && !(xml[i]=='?' && xml[i+1]=='>')) ++i;
            if (i + 1 < n) i += 2;
            continue;
        }
        if (xml[i] == '!') {
            int depth = 1; ++i;
            while (i < n && depth > 0) {
                if (xml[i] == '<') ++depth;
                else if (xml[i] == '>') --depth;
                ++i;
            }
            continue;
        }

        bool is_close = false;
        if (xml[i] == '/') { is_close = true; ++i; }

        size_t name_s = i;
        while (i < n && xml[i] != '>' && xml[i] != '/' &&
               !std::isspace(static_cast<unsigned char>(xml[i]))) ++i;
        if (i <= name_s) {
            while (i < n && xml[i] != '>') ++i;
            if (i < n) ++i;
            continue;
        }
        std::string local_name = std::string(stripNs(xml.substr(name_s, i - name_s)));

        if (is_close) {
            while (i < n && xml[i] != '>') ++i;
            if (i < n) ++i;
            XmlTag t; t.name = std::move(local_name); t.is_close = true;
            tag_cb(t);
            continue;
        }

        size_t attr_s = i;
        bool in_dq = false, in_sq = false, self_close = false;
        size_t tag_end = i;
        while (i < n) {
            char c = xml[i];
            if (in_dq)      { if (c == '"')  in_dq = false; }
            else if (in_sq) { if (c == '\'') in_sq = false; }
            else if (c == '"')  { in_dq = true; }
            else if (c == '\'') { in_sq = true; }
            else if (c == '>') { tag_end = i; break; }
            else if (c == '/' && i + 1 < n && xml[i+1] == '>') {
                self_close = true; tag_end = i; break;
            }
            ++i;
        }

        XmlTag tag;
        tag.name = std::move(local_name);
        tag.self_closing = self_close;
        parseAttrs(xml.substr(attr_s, tag_end - attr_s), tag.attrs);

        if (self_close) i += 2;
        else if (i < n) ++i;

        tag_cb(tag);
    }
    return true;
}

/// CMMN plan-item element names that map to ProcessNodeInfo nodes.
static const std::set<std::string> kCmmnTaskTags = {
    "humanTask", "processTask", "caseTask",
    "stage", "casePlanModel", "milestone",
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// CmmnSerializer::escapeXml_
// ─────────────────────────────────────────────────────────────────────────────

std::string CmmnSerializer::escapeXml_(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "<";   break;
            case '>':  out += ">";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += c;        break;
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// CmmnSerializer::importXml
// ─────────────────────────────────────────────────────────────────────────────

CmmnSerializer::ImportResult CmmnSerializer::importXml(std::string_view cmmn_xml) {
    ImportResult result;

    // Phase 3: Validate input before parsing
    auto validation = SerializerInputValidator::validateInput(
        cmmn_xml,
        "CMMN 1.1"
    );
    if (!validation.ok) {
        result.ok      = false;
        result.message = validation.error_message;
        auto incident = ProcessDiagnostics::createMalformedInputIncident(
            ProcError::kDeserialiserFailed,
            "<cmmn_xml>",
            validation.error_message
        );
        SPDLOG_WARN("[cmmn_serializer] {}", incident.toFormattedMessage());
        return result;
    }

    // ── Parser state ──────────────────────────────────────────────────────

    // Current plan-item element being parsed (for criterion association).
    std::string  current_item_id;

    // Sentry tracking:
    //   sentry_id → sourceRef (from <onPart sourceRef="…">)
    std::map<std::string, std::string> sentry_sources;
    //   sentry_id → target plan-item id (from entryCriterion/exitCriterion)
    std::map<std::string, std::string> sentry_targets;

    std::string current_sentry_id;
    bool        in_sentry{false};

    std::set<std::string> seen_node_ids;

    // Phase 3: Initialize parser state tracker for resource limit enforcement
    ParserStateTracker parser_state(static_cast<size_t>(kMaxCmmnXmlBytes));

    auto tag_cb = [&]([[maybe_unused]] const XmlTag& t) {
        const std::string& tn = t.name;

        // ── Closing tags ──────────────────────────────────────────────────
        if (t.is_close) {
            if (tn == "sentry") { in_sentry = false; current_sentry_id.clear(); }
            if (kCmmnTaskTags.count(tn)) { current_item_id.clear(); }
            return;
        }

        // ── <case> root element ───────────────────────────────────────────
        if (tn == "case") {
            // Prefer explicit case metadata over <definitions> fallback data.
            // A valid CMMN document may define both, and test fixtures expect
            // the semantic case identity from the <case> element.
            auto it_id   = t.attrs.find("id");
            auto it_name = t.attrs.find("name");
            if (it_id != t.attrs.end() && !it_id->second.empty()) {
                result.case_id = it_id->second;
            } else if (result.case_id.empty()) {
                result.case_id = "imported_case";
            }
            if (it_name != t.attrs.end() && !it_name->second.empty()) {
                result.case_name = it_name->second;
            } else if (result.case_name.empty()) {
                result.case_name = result.case_id;
            }
            return;
        }

        // ── <definitions> — CMMN XML root (alternative to <case>) ─────────
        if (tn == "definitions" || tn == "Definitions") {
            auto it_id   = t.attrs.find("id");
            auto it_name = t.attrs.find("name");
            if (result.case_id.empty()) {
                result.case_id   = (it_id   != t.attrs.end()) ? it_id->second   : "imported_case";
                result.case_name = (it_name != t.attrs.end()) ? it_name->second : result.case_id;
            }
            return;
        }

        // ── casePlanModel / plan-item task/stage/milestone ────────────────
        if (kCmmnTaskTags.count(tn)) {
            auto it_id = t.attrs.find("id");
            if (it_id == t.attrs.end() || it_id->second.empty()) return;
            const std::string& nid = it_id->second;
            if (!seen_node_ids.insert(nid).second) return;

            auto it_name = t.attrs.find("name");
            ProcessNodeInfo node;
            node.node_id = nid;
            node.name    = (it_name != t.attrs.end() && !it_name->second.empty())
                           ? it_name->second : nid;

            if (tn == "casePlanModel") {
                node.node_type = BPMNNodeType::SUBPROCESS;
                node.subtype   = "CASE_PLAN";
            } else if (tn == "humanTask") {
                node.node_type = BPMNNodeType::TASK;
                node.subtype   = "HUMAN_TASK";
            } else if (tn == "processTask") {
                node.node_type = BPMNNodeType::TASK;
                node.subtype   = "PROCESS_TASK";
            } else if (tn == "caseTask") {
                node.node_type = BPMNNodeType::TASK;
                node.subtype   = "CASE_TASK";
            } else if (tn == "stage") {
                node.node_type = BPMNNodeType::SUBPROCESS;
                node.subtype   = "STAGE";
            } else if (tn == "milestone") {
                node.node_type = BPMNNodeType::INTERMEDIATE_EVENT;
                node.subtype   = "MILESTONE";
            }

            // Store case_id in metadata for traceability.
            node.metadata["cmmn_case_id"] = result.case_id;

            result.nodes.push_back(std::move(node));

            // Track current item for criterion association on non-self-closing elements.
            if (!t.self_closing) current_item_id = nid;
            return;
        }

        // ── <sentry id="…"> ───────────────────────────────────────────────
        if (tn == "sentry") {
            auto it = t.attrs.find("id");
            if (it != t.attrs.end()) {
                current_sentry_id = it->second;
                in_sentry = !t.self_closing;
            }
            return;
        }

        // ── <onPart sourceRef="…"/> inside a sentry ───────────────────────
        if (in_sentry && (tn == "onPart" || tn == "planItemOnPart" ||
                          tn == "caseFileItemOnPart")) {
            auto it = t.attrs.find("sourceRef");
            if (it != t.attrs.end() && !current_sentry_id.empty()) {
                sentry_sources[current_sentry_id] = it->second;
            }
            return;
        }

        // ── <entryCriterion sentryRef="…"/> / <exitCriterion …/> ──────────
        if ((tn == "entryCriterion" || tn == "exitCriterion") &&
            !current_item_id.empty()) {
            auto it = t.attrs.find("sentryRef");
            if (it != t.attrs.end()) {
                // Map this sentry to the current plan item as the edge target.
                sentry_targets[it->second] = current_item_id;
            }
            return;
        }
    };

    auto text_cb = [&]([[maybe_unused]] std::string_view) {};

    if (!tokenizeCmmnXml(cmmn_xml, tag_cb, text_cb)) {
        result.ok      = false;
        result.message = "CMMN XML exceeds maximum allowed size (10 MiB)";
        return result;
    }

    // Build edges from sentry mappings:
    // for each sentry s: if sentry_sources[s] and sentry_targets[s] both exist,
    // create a SEQUENCE_FLOW edge from source to target.
    static int edge_counter = 0;
    for (const auto& [sentry_id, source_ref] : sentry_sources) {
        auto it = sentry_targets.find(sentry_id);
        if (it == sentry_targets.end()) continue;
        const std::string& target_ref = it->second;
        if (source_ref.empty() || target_ref.empty()) continue;

        ProcessEdgeInfo edge;
        edge.edge_id   = "edge_sentry_" + sentry_id + "_" + std::to_string(++edge_counter);
        edge.from_node = source_ref;
        edge.to_node   = target_ref;
        edge.edge_type = ProcessEdgeType::SEQUENCE_FLOW;
        result.edges.push_back(std::move(edge));
    }

    if (result.case_id.empty()) {
        result.case_id   = "imported_case";
        result.case_name = "Imported Case";
    }

    if (result.nodes.empty() && result.edges.empty()) {
        result.ok      = false;
        result.message = "No CMMN plan elements found in XML";
        return result;
    }

    result.ok      = true;
    result.message = "OK";
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// CmmnSerializer::importFile
// ─────────────────────────────────────────────────────────────────────────────

CmmnSerializer::ImportResult CmmnSerializer::importFile(std::string_view file_path) {
    std::ifstream f{std::string(file_path)};
    if (!f.is_open()) {
        ImportResult r;
        r.ok      = false;
        r.message = "Cannot open file: " + std::string(file_path);
        auto incident = ProcessDiagnostics::createImportIncident(
            ProcError::kDeserialiserFailed,
            file_path,
            r.message
        );
        SPDLOG_WARN("[cmmn_serializer] {}", incident.toFormattedMessage());
        return r;
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return importXml(content);
}

// ─────────────────────────────────────────────────────────────────────────────
// CmmnSerializer::exportXml
// ─────────────────────────────────────────────────────────────────────────────

std::string CmmnSerializer::exportXml(
    std::string_view                    case_id,
    std::string_view                    case_name,
    const std::vector<ProcessNodeInfo>& nodes,
    const std::vector<ProcessEdgeInfo>& edges)
{
    static_cast<void>(edges);
    std::ostringstream xml;

    xml << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
    xml << R"(<definitions xmlns="http://www.omg.org/spec/CMMN/20151109/MODEL")" << "\n";
    xml << R"(             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance")" << "\n";
    xml << R"(             targetNamespace="http://themis.db/cmmn")" << "\n";
    xml << R"(             id="definitions_)" << escapeXml_(case_id) << R"(">)" << "\n\n";

    xml << R"(  <case id=")" << escapeXml_(case_id)
        << R"(" name=")" << escapeXml_(case_name) << "\">\n";

    // Find the casePlanModel (if any) to use as the root container.
    std::string plan_model_id;
    for (const auto& n : nodes) {
        if (std::holds_alternative<BPMNNodeType>(n.node_type) &&
            std::get<BPMNNodeType>(n.node_type) == BPMNNodeType::SUBPROCESS &&
            n.subtype == "CASE_PLAN") {
            plan_model_id = n.node_id;
            break;
        }
    }
    if (plan_model_id.empty()) {
        plan_model_id = std::string(case_id) + "_plan";
    }

    xml << R"(    <casePlanModel id=")" << escapeXml_(plan_model_id)
        << R"(" name=")" << escapeXml_(case_name) << "\">\n";

    // Emit plan-item elements.
    for (const auto& n : nodes) {
        if (!std::holds_alternative<BPMNNodeType>(n.node_type)) continue;
        auto btype = std::get<BPMNNodeType>(n.node_type);

        // Skip the casePlanModel itself (already emitted as root).
        if (btype == BPMNNodeType::SUBPROCESS && n.subtype == "CASE_PLAN") continue;

        std::string tag;
        if (btype == BPMNNodeType::TASK) {
            if      (n.subtype == "HUMAN_TASK")    tag = "humanTask";
            else if (n.subtype == "PROCESS_TASK")  tag = "processTask";
            else if (n.subtype == "CASE_TASK")     tag = "caseTask";
            else                                    tag = "humanTask"; // default
        } else if (btype == BPMNNodeType::SUBPROCESS && n.subtype == "STAGE") {
            tag = "stage";
        } else if (btype == BPMNNodeType::INTERMEDIATE_EVENT &&
                   n.subtype == "MILESTONE") {
            tag = "milestone";
        } else {
            continue; // Skip non-CMMN node types
        }

        xml << "      <" << tag
            << " id=\"" << escapeXml_(n.node_id) << "\""
            << " name=\"" << escapeXml_(n.name) << "\"/>\n";
    }

    xml << "    </casePlanModel>\n";
    xml << "  </case>\n";
    xml << "</definitions>\n";

    return xml.str();
}

} // namespace process
} // namespace themis

