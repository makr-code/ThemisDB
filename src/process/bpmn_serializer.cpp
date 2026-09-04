/**
 * @file bpmn_serializer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=30, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    bpmn_serializer.cpp
 * Module:  src/process/
 * Purpose: BPMN 2.0 XML import and export for process model definitions.
 *
 * Implementation note: we use a minimal hand-written XML parser for import
 * (no external XML library dependency) and produce standards-compliant XML on
 * export.  BPMNDI (diagram interchange) BPMNShape bounds (x/y/width/height)
 * are parsed on import and stored in ProcessNodeInfo::metadata["layout"].
 * BPMNDI data is not emitted on export because ThemisDB stores processes as
 * graph data, not as graphical diagrams.
 */

#include "process/bpmn_serializer.h"
#include "process/serializer_hardening.h"
#include "process/process_diagnostics.h"
#include "process/process_common.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace process {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Minimal SAX-like XML tokenizer – no regex, no external library
// ---------------------------------------------------------------------------

namespace {

/// Maximum BPMN XML document size accepted (10 MiB security guard).
static constexpr size_t kMaxBpmnXmlBytes = 10u * 1024u * 1024u;

/// Strip XML character entities and leading/trailing whitespace.
std::string unescapeXml(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ) {
        if (s[i] != '&') { out += s[i++]; continue; }
        // entity reference
        size_t semi = s.find(';', i + 1);
        if (semi == std::string_view::npos) { out += s[i++]; continue; }
        std::string_view ent = s.substr(i, semi - i + 1);
        if      (ent == "&amp;") {
          out += '&';
        }
        else if (ent == "<")   out += '<';
        else if (ent == ">")   out += '>';
        else if (ent == "&quot;") out += '"';
        else if (ent == "&apos;") out += '\'';
        else                      out += std::string(ent); // unknown – keep as-is
        i = semi + 1;
    }
    // trim
    size_t a = out.find_first_not_of(" \t\n\r");
    if (a == std::string::npos) return {};
    size_t b = out.find_last_not_of(" \t\n\r");
    return out.substr(a, b - a + 1);
}

/// Strip XML namespace prefix: "bpmn2:startEvent" → "startEvent".
std::string_view stripNs(std::string_view name) {
    auto colon = name.rfind(':');
    return (colon != std::string_view::npos) ? name.substr(colon + 1) : name;
}

/// Parsed representation of a single XML element tag.
struct XmlTag {
    std::string name;       ///< Local element name (namespace stripped).
    std::map<std::string, std::string> attrs; ///< Attribute map.
    bool self_closing{false};
    bool is_close{false};   ///< True for </tag>.
};

/// Parse attributes from the raw text between the tag name and '>' / '/>'
/// (no regex; handles single- and double-quoted values).
void parseAttrs(std::string_view src,
                std::map<std::string, std::string>& out)
{
    size_t i = 0;
    const size_t n = src.size();
    while (i < n) {
        // skip whitespace
        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) {
          ++i;
        }
        if (i >= n || src[i] == '/' || src[i] == '>') {
          break;
        }

        // attribute name
        size_t ns = i;
        while (i < n && src[i] != '=' && src[i] != '/' && src[i] != '>' &&
               !std::isspace(static_cast<unsigned char>(src[i]))) ++i;
        if (i <= ns) {
          break;
        }
        std::string attr_name = std::string(stripNs(src.substr(ns, i - ns)));

        // skip whitespace
        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) {
          ++i;
        }
        if (i >= n || src[i] != '=') {
            // valueless attribute (e.g., isExecutable without value)
            if (!attr_name.empty()) {
              out.emplace(std::move(attr_name), "true");
            }
            continue;
        }
        ++i; // skip '='

        // skip whitespace
        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) {
          ++i;
        }
        if (i >= n) {
          break;
        }

        std::string attr_val;
        if (src[i] == '"' || src[i] == '\'') {
            char q = src[i++];
            size_t vs = i;
            while (i < n && src[i] != q) {
              ++i;
            }
            attr_val = unescapeXml(src.substr(vs, i - vs));
            if (i < n) ++i; // skip closing quote
        } else {
            size_t vs = i;
            while (i < n && !std::isspace(static_cast<unsigned char>(src[i])) &&
                   src[i] != '>' && src[i] != '/') ++i;
            attr_val = std::string(src.substr(vs, i - vs));
        }
        if (!attr_name.empty()) {
          out.emplace(std::move(attr_name), std::move(attr_val));
        }
    }
}

/// Walk every XML token in @p xml, calling @p tag_cb for each element tag and
/// @p text_cb for each text node.  Skips comments, PIs, DOCTYPE, and CDATA.
/// Returns false if @p xml exceeds kMaxBpmnXmlBytes.
template<typename TagCb, typename TextCb>
bool tokenizeXml(std::string_view xml, TagCb tag_cb, TextCb text_cb)
{
    if (xml.size() > kMaxBpmnXmlBytes) {
      return false;
    }

    size_t i = 0;
    const size_t n = xml.size();

    while (i < n) {
        // text node
        size_t ts = i;
        while (i < n && xml[i] != '<') {
          ++i;
        }
        if (i > ts) {
          text_cb(xml.substr(ts, i - ts));
        }
        if (i >= n) {
          break;
        }

        // '<' found
        ++i;
        if (i >= n) {
          break;
        }

        // <!-- comment -->
        if (i + 2 < n && xml[i] == '!' && xml[i+1] == '-' && xml[i+2] == '-') {
            i += 3;
            while (i + 2 < n &&
                   !(xml[i] == '-' && xml[i+1] == '-' && xml[i+2] == '>')) ++i;
            if (i + 2 < n) {
              i += 3;
            }
            continue;
        }
        // <![CDATA[...]]>
        if (i + 7 < n && xml.substr(i, 8) == "![CDATA[") {
            i += 8;
            size_t cs = i;
            while (i + 2 < n &&
                   !(xml[i] == ']' && xml[i+1] == ']' && xml[i+2] == '>')) ++i;
            text_cb(xml.substr(cs, i - cs));
            if (i + 2 < n) {
              i += 3;
            }
            continue;
        }
        // <? PI ?>
        if (xml[i] == '?') {
            while (i + 1 < n && !(xml[i] == '?' && xml[i+1] == '>')) {
              ++i;
            }
            if (i + 1 < n) {
              i += 2;
            }
            continue;
        }
        // <!DOCTYPE …> or other <! constructs (non-recursive depth tracking)
        if (xml[i] == '!') {
            int depth = 1; ++i;
            while (i < n && depth > 0) {
                if (xml[i] == '<') {
                  ++depth;
                }
                else if (xml[i] == '>') --depth;
                ++i;
            }
            continue;
        }

        // closing tag </name>
        bool is_close = false;
        if (xml[i] == '/') { is_close = true; ++i; }

        // tag name
        size_t name_s = i;
        while (i < n && xml[i] != '>' && xml[i] != '/' &&
               !std::isspace(static_cast<unsigned char>(xml[i]))) ++i;
        if (i <= name_s) {
            while (i < n && xml[i] != '>') {
              ++i;
            }
            if (i < n) {
              ++i;
            }
            continue;
        }
        std::string local_name =
            std::string(stripNs(xml.substr(name_s, i - name_s)));

        if (is_close) {
            while (i < n && xml[i] != '>') {
              ++i;
            }
            if (i < n) {
              ++i;
            }
            XmlTag t; t.name = std::move(local_name); t.is_close = true;
            tag_cb(t);
            continue;
        }

        // attribute section — must handle quoted '>' inside values
        size_t attr_s = i;
        bool in_dq = false, in_sq = false, self_close = false;
        size_t tag_end = i;
        while (i < n) {
            char c = xml[i];
            if (in_dq) { if (c == '"')  in_dq = false; }
            else if (in_sq) { if (c == '\'') in_sq = false; }
            else if (c == '"')  { in_dq = true; }
            else if (c == '\'') { in_sq = true; }
            else if (c == '>' ) { tag_end = i; break; }
            else if (c == '/' && i + 1 < n && xml[i+1] == '>') {
                self_close = true; tag_end = i; break;
            }
            ++i;
        }

        XmlTag tag;
        tag.name = std::move(local_name);
        tag.self_closing = self_close;
        parseAttrs(xml.substr(attr_s, tag_end - attr_s), tag.attrs);

        if (self_close) i += 2; // skip '/>'
        else if (i < n) ++i;    // skip '>'

        tag_cb(tag);
    }
    return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// BpmnSerializer::escapeXml_
// ---------------------------------------------------------------------------

std::string BpmnSerializer::escapeXml_(std::string_view s) {
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

// ---------------------------------------------------------------------------
// BpmnSerializer::nodeTypeToXmlTag_
// ---------------------------------------------------------------------------

std::string BpmnSerializer::nodeTypeToXmlTag_(BPMNNodeType t) {
    switch (t) {
        case BPMNNodeType::START_EVENT:         return "startEvent";
        case BPMNNodeType::END_EVENT:           return "endEvent";
        case BPMNNodeType::INTERMEDIATE_EVENT:  return "intermediateCatchEvent";
        case BPMNNodeType::BOUNDARY_EVENT:      return "boundaryEvent";
        case BPMNNodeType::TASK:                return "task";
        case BPMNNodeType::SUBPROCESS:          return "subProcess";
        case BPMNNodeType::CALL_ACTIVITY:       return "callActivity";
        case BPMNNodeType::EXCLUSIVE_GATEWAY:   return "exclusiveGateway";
        case BPMNNodeType::PARALLEL_GATEWAY:    return "parallelGateway";
        case BPMNNodeType::INCLUSIVE_GATEWAY:   return "inclusiveGateway";
        case BPMNNodeType::EVENT_BASED_GATEWAY: return "eventBasedGateway";
        case BPMNNodeType::COMPLEX_GATEWAY:     return "complexGateway";
        case BPMNNodeType::POOL:                return "participant";
        case BPMNNodeType::LANE:                return "lane";
        case BPMNNodeType::DATA_OBJECT:         return "dataObjectReference";
        case BPMNNodeType::DATA_STORE:          return "dataStoreReference";
        case BPMNNodeType::GROUP:               return "group";
        case BPMNNodeType::ANNOTATION:          return "textAnnotation";
        default:                                return "task";
    }
}

// ---------------------------------------------------------------------------
// BpmnSerializer::xmlTagToNodeType_
// ---------------------------------------------------------------------------

BPMNNodeType BpmnSerializer::xmlTagToNodeType_(std::string_view tag) {
    if (tag == "startEvent") {
      return BPMNNodeType::START_EVENT;
    }
    if (tag == "endEvent") {
      return BPMNNodeType::END_EVENT;
    }
    if (tag == "intermediateCatchEvent" ||
        tag == "intermediateThrowEvent") return BPMNNodeType::INTERMEDIATE_EVENT;
    if (tag == "boundaryEvent") {
      return BPMNNodeType::BOUNDARY_EVENT;
    }
    if (tag == "subProcess") {
      return BPMNNodeType::SUBPROCESS;
    }
    if (tag == "callActivity") {
      return BPMNNodeType::CALL_ACTIVITY;
    }
    if (tag == "exclusiveGateway") {
      return BPMNNodeType::EXCLUSIVE_GATEWAY;
    }
    if (tag == "parallelGateway") {
      return BPMNNodeType::PARALLEL_GATEWAY;
    }
    if (tag == "inclusiveGateway") {
      return BPMNNodeType::INCLUSIVE_GATEWAY;
    }
    if (tag == "eventBasedGateway") {
      return BPMNNodeType::EVENT_BASED_GATEWAY;
    }
    if (tag == "complexGateway") {
      return BPMNNodeType::COMPLEX_GATEWAY;
    }
    if (tag == "participant") {
      return BPMNNodeType::POOL;
    }
    if (tag == "lane") {
      return BPMNNodeType::LANE;
    }
    if (tag == "dataObjectReference") {
      return BPMNNodeType::DATA_OBJECT;
    }
    if (tag == "dataStoreReference") {
      return BPMNNodeType::DATA_STORE;
    }
    if (tag == "group") {
      return BPMNNodeType::GROUP;
    }
    if (tag == "textAnnotation") {
      return BPMNNodeType::ANNOTATION;
    }
    // All task variants → TASK
    if (tag.find("Task") != std::string_view::npos ||
        tag == "task")                   return BPMNNodeType::TASK;
    return BPMNNodeType::TASK;
}

// ---------------------------------------------------------------------------
// BpmnSerializer::ImportResult helper methods (Phase 3)
// ---------------------------------------------------------------------------

BpmnSerializer::ImportResult BpmnSerializer::ImportResult::success(
    std::string_view pid,
    std::string_view pname,
    std::vector<ProcessNodeInfo> n,
    std::vector<ProcessEdgeInfo> e
) {
    ImportResult r;
    r.ok = true;
    r.error_code = ProcessErrorCode::INTERNAL_ERROR; // not checked when ok=true
    r.message = "OK";
    r.process_id = std::string(pid);
    r.process_name = std::string(pname);
    r.nodes = std::move(n);
    r.edges = std::move(e);
    return r;
}

BpmnSerializer::ImportResult BpmnSerializer::ImportResult::failure(
    ProcessErrorCode code,
    std::string_view context,
    std::string_view detail
) {
    ImportResult r;
    r.ok = false;
    r.error_code = code;
    r.message = formatDiagnostic(code, context, detail);
    return r;
}

// ---------------------------------------------------------------------------
// BpmnSerializer::importXml
// ---------------------------------------------------------------------------

BpmnSerializer::ImportResult BpmnSerializer::importXml(std::string_view bpmn_xml) {
    if (bpmn_xml.empty()) {
        return ImportResult::failure(
            ProcessErrorCode::EMPTY_INPUT,
            "import BPMN from XML",
            "input is empty"
        );
    }

    // Phase 3: Unified malformed input detection
    auto validation = SerializerInputValidator::validateInput(bpmn_xml, "BPMN 2.0");
    if (!validation.ok) {
        return ImportResult::failure(
            ProcessErrorCode::MALFORMED_INPUT,
            "import BPMN from XML",
            validation.error_message
        );
    }

    // Security guard: reject oversized documents before any parsing work.
    if (bpmn_xml.size() > kMaxBpmnXmlBytes) {
        return ImportResult::failure(
            ProcessErrorCode::INPUT_TOO_LARGE,
            "import BPMN from XML",
            "XML exceeds 10 MiB size limit"
        );
    }

    ImportResult result;

    // Phase 3: Resource limit enforcement using ParserStateTracker
    ParserStateTracker parser_tracker(
        kMaxModelNestingDepth,
        kMaxModelElements,
        kMaxOperationTimeoutMs
    );

    // Known flow-node local names.
    static const std::set<std::string> kFlowNodeTags = {
        "startEvent", "endEvent",
        "intermediateCatchEvent", "intermediateThrowEvent",
        "boundaryEvent",
        "task", "userTask", "serviceTask", "scriptTask",
        "sendTask", "receiveTask", "manualTask", "businessRuleTask",
        "subProcess", "adHocSubProcess", "transaction", "callActivity",
        "exclusiveGateway", "parallelGateway", "inclusiveGateway",
        "eventBasedGateway", "complexGateway",
        "dataObjectReference", "dataStoreReference",
        "textAnnotation", "participant", "lane",
    };

    // State for conditionExpression child collection.
    bool        in_seq_flow{false};
    bool        in_cond_expr{false};
    std::string sf_id, sf_src, sf_tgt, sf_name, cond_text;

    // ── BPMNDI state ──────────────────────────────────────────────────────
    // Track BPMNShape elements: bpmnElement → {x, y, width, height}
    struct BpmnBounds { float x{0}, y{0}, width{0}, height{0}; };
    std::map<std::string, BpmnBounds> shape_bounds;
    bool in_bpmndi{false};         ///< Inside BPMNDiagram element
    bool in_shape{false};          ///< Inside BPMNShape element
    std::string shape_elem_ref;    ///< bpmnElement attr of current BPMNShape

    // Deduplication guard (duplicate IDs can appear in sub-process copies).
    std::set<std::string> seen_node_ids;

    // ── BPMN-S state ──────────────────────────────────────────────────────
    std::string current_flow_node_id;   ///< Non-self-closing flow node being parsed
    bool        in_extension_elements{false};
    std::map<std::string, ProcessNodeInfo::DsgvoAnnotation> dsgvo_map;

    auto tag_cb = [&]([[maybe_unused]] const XmlTag& t) {
        // Phase 3: Check timeout before processing tag
        if (parser_tracker.hasTimedOut()) {
            SPDLOG_WARN("[bpmn_serializer] Timeout during BPMN parsing at tag: {}", t.name);
            return;  // Stop processing
        }

        // Helper lambda for resource limit checks before adding edges
        auto checkAndAddEdge = [&]([[maybe_unused]] const ProcessEdgeInfo& edge) {
            if (!edge.from_node.empty() && !edge.to_node.empty()) {
                if (!parser_tracker.recordElement()) {
                    SPDLOG_WARN("[bpmn_serializer] Element limit exceeded while adding edge '{}'", edge.edge_id);
                    return false;  // Couldn't add
                }
                result.edges.push_back(edge);
            }
            return true;
        };

        const std::string& tn = t.name;

        // ── Closing tags ──────────────────────────────────────────────────
        if (t.is_close) {
            if (tn == "sequenceFlow" && in_seq_flow) {
                ProcessEdgeInfo edge;
                edge.edge_id   = sf_id;
                edge.from_node = sf_src;
                edge.to_node   = sf_tgt;
                edge.edge_type = ProcessEdgeType::SEQUENCE_FLOW;
                if (!cond_text.empty())
                    edge.condition_expression = unescapeXml(cond_text);
                else if (!sf_name.empty())
                    edge.condition_expression = sf_name;
                checkAndAddEdge(edge);
                in_seq_flow  = false;
                in_cond_expr = false;
                cond_text.clear();
            }
            if (tn == "conditionExpression") {
              in_cond_expr = false;
            }
            // ── BPMNDI closing tags ───────────────────────────────────────
            if (tn == "BPMNDiagram" || tn == "BPMNPlane") {
              in_bpmndi = false;
            }
            if (tn == "BPMNShape") { in_shape = false; shape_elem_ref.clear(); }
            // ── BPMN-S closing tags ───────────────────────────────────────
            if (tn == "extensionElements") { in_extension_elements = false; return; }
            if (!current_flow_node_id.empty() && kFlowNodeTags.count(tn)) {
                // Phase 3: Exit scope for subProcess closing tags
                if (tn == "subProcess") {
                    if (!parser_tracker.exitScope()) {
                        SPDLOG_WARN("[bpmn_serializer] Scope underflow when closing subProcess '{}'", 
                                   current_flow_node_id);
                    }
                }
                current_flow_node_id.clear();
                return;
            }
            return;
        }

        // ── Inside a sequenceFlow: look for conditionExpression child ─────
        if (in_seq_flow && tn == "conditionExpression") {
            if (!t.self_closing) {
                in_cond_expr = true;
                cond_text.clear();
            }
            return;
        }

        // ── BPMNDI: BPMNDiagram / BPMNPlane / BPMNShape / Bounds ─────────
        if (tn == "BPMNDiagram" || tn == "BPMNPlane") { in_bpmndi = true; return; }

        if (tn == "BPMNShape" && in_bpmndi) {
            in_shape = false;
            shape_elem_ref.clear();
            auto it = t.attrs.find("bpmnElement");
            if (it != t.attrs.end() && !it->second.empty()) {
                in_shape       = true;
                shape_elem_ref = it->second;
            }
            return;
        }

        // <dc:Bounds x="…" y="…" width="…" height="…" /> inside BPMNShape
        if (in_shape && (tn == "Bounds" || tn == "bounds") &&
            !shape_elem_ref.empty()) {
            BpmnBounds b;
            auto parseAttr = [&]([[maybe_unused]] const char* key) -> float {
                auto it = t.attrs.find(key);
                if (it == t.attrs.end()) {
                  return 0.f;
                }
                try { return std::stof(it->second); } catch (...) { return 0.f; }
            };
            b.x      = parseAttr("x");
            b.y      = parseAttr("y");
            b.width  = parseAttr("width");
            b.height = parseAttr("height");
            shape_bounds[shape_elem_ref] = b;
            return;
        }

        // ── <process> (first occurrence wins) ────────────────────────────
        if (tn == "process") {
            if (result.process_id.empty()) {
                auto it_id = t.attrs.find("id");
                auto it_nm = t.attrs.find("name");
                result.process_id   = (it_id != t.attrs.end()) ? it_id->second : "imported_process";
                result.process_name = (it_nm != t.attrs.end()) ? it_nm->second : result.process_id;
            }
            return;
        }

        // ── extensionElements / BPMN-S SecurityAnnotation ─────────────────
        if (tn == "extensionElements" && !current_flow_node_id.empty()) {
            if (!t.self_closing) {
              in_extension_elements = true;
            }
            return;
        }
        if (in_extension_elements && tn == "SecurityAnnotation") {
            auto get = [&]([[maybe_unused]] const char* k) -> std::string {
                auto it = t.attrs.find(k);
                return (it != t.attrs.end()) ? it->second : std::string{};
            };
            ProcessNodeInfo::DsgvoAnnotation ann;
            ann.data_category    = get("dataCategory");
            ann.legal_basis      = get("legalBasis");
            std::string rd       = get("retentionDays");
            if (!rd.empty()) {
                try { ann.retention_days = std::stoi(rd); } catch (...) {}
            }
            ann.requires_consent = (get("requiresConsent") == "true");
            dsgvo_map[current_flow_node_id] = std::move(ann);
            return;
        }

        // ── sequenceFlow ──────────────────────────────────────────────────
        if (tn == "sequenceFlow") {
            auto get = [&]([[maybe_unused]] const char* k) -> std::string {
                auto it = t.attrs.find(k);
                return (it != t.attrs.end()) ? it->second : std::string{};
            };
            if (t.self_closing) {
                ProcessEdgeInfo edge;
                edge.edge_id   = get("id");
                edge.from_node = get("sourceRef");
                edge.to_node   = get("targetRef");
                edge.edge_type = ProcessEdgeType::SEQUENCE_FLOW;
                std::string cond = get("name");
                if (!cond.empty()) {
                  edge.condition_expression = cond;
                }
                checkAndAddEdge(edge);
            } else {
                in_seq_flow = true;
                sf_id  = get("id");
                sf_src = get("sourceRef");
                sf_tgt = get("targetRef");
                sf_name= get("name");
                cond_text.clear();
            }
            return;
        }

        // ── messageFlow ───────────────────────────────────────────────────
        if (tn == "messageFlow") {
            auto get = [&]([[maybe_unused]] const char* k) -> std::string {
                auto it = t.attrs.find(k);
                return (it != t.attrs.end()) ? it->second : std::string{};
            };
            ProcessEdgeInfo edge;
            edge.edge_id   = get("id");
            edge.from_node = get("sourceRef");
            edge.to_node   = get("targetRef");
            edge.edge_type = ProcessEdgeType::MESSAGE_FLOW;
            checkAndAddEdge(edge);
            return;
        }

        // ── association / dataInputAssociation ────────────────────────────
        if (tn == "association" || tn == "dataInputAssociation" ||
            tn == "dataOutputAssociation") {
            auto get = [&]([[maybe_unused]] const char* k) -> std::string {
                auto it = t.attrs.find(k);
                return (it != t.attrs.end()) ? it->second : std::string{};
            };
            ProcessEdgeInfo edge;
            edge.edge_id   = get("id");
            edge.from_node = get("sourceRef");
            edge.to_node   = get("targetRef");
            edge.edge_type = (tn == "association")
                             ? ProcessEdgeType::ASSOCIATION
                             : ProcessEdgeType::DATA_ASSOCIATION;
            checkAndAddEdge(edge);
            return;
        }

        // ── Flow nodes ────────────────────────────────────────────────────
        if (kFlowNodeTags.count(tn)) {
            auto it_id = t.attrs.find("id");
            if (it_id == t.attrs.end() || it_id->second.empty()) {
              return;
            }
            const std::string& nid = it_id->second;
            if (!seen_node_ids.insert(nid).second) return; // duplicate

            auto it_nm = t.attrs.find("name");

            ProcessNodeInfo node;
            node.node_id  = nid;
            node.name     = (it_nm != t.attrs.end() && !it_nm->second.empty())
                             ? it_nm->second : nid;
            node.node_type = xmlTagToNodeType_(tn);

            if      (tn == "userTask") {
              node.subtype = "USER_TASK";
            }
            else if (tn == "serviceTask")       node.subtype = "SERVICE_TASK";
            else if (tn == "scriptTask")        node.subtype = "SCRIPT_TASK";
            else if (tn == "sendTask")          node.subtype = "SEND_TASK";
            else if (tn == "receiveTask")       node.subtype = "RECEIVE_TASK";
            else if (tn == "manualTask")        node.subtype = "MANUAL_TASK";
            else if (tn == "businessRuleTask")  node.subtype = "BUSINESS_RULE_TASK";

            // async markers from common BPMN engine extensions
            for (const char* k : {"isAsync", "asyncBefore", "async"}) {
                auto it = t.attrs.find(k);
                if (it != t.attrs.end() &&
                    (it->second == "true" || it->second == "TRUE")) {
                    node.is_async = true;
                    break;
                }
            }

            result.nodes.push_back(std::move(node));

            // Phase 3: Resource limit enforcement
            if (!parser_tracker.recordElement()) {
                result.ok = false;
                result.message = "Maximum element count (" + std::to_string(parser_tracker.getElementCount()) + 
                                ") exceeded during BPMN import";
                auto incident = ProcessDiagnostics::createResourceIncident(
                    ProcError::kExecutionTimeout,
                    "bpmn_import",
                    parser_tracker.getDiagnosticMessage()
                );
                SPDLOG_WARN("[bpmn_serializer] {}", incident.toFormattedMessage());
                return;
            }

            // Check for timeout
            if (parser_tracker.hasTimedOut()) {
                result.ok = false;
                result.message = "BPMN import operation exceeded timeout (" + 
                                std::to_string(parser_tracker.getElapsedMs()) + "ms)";
                auto incident = ProcessDiagnostics::createResourceIncident(
                    ProcError::kExecutionTimeout,
                    "bpmn_import",
                    parser_tracker.getDiagnosticMessage()
                );
                SPDLOG_WARN("[bpmn_serializer] {}", incident.toFormattedMessage());
                return;
            }

            // Track non-self-closing flow nodes so extensionElements children can
            // be associated with this node (e.g. BPMN-S SecurityAnnotation).
            if (!t.self_closing) {
                current_flow_node_id = nid;
                // Phase 3: Track nesting depth for subProcess
                if (tn == "subProcess") {
                    if (!parser_tracker.enterScope()) {
                        result.ok = false;
                        result.message = "Maximum nesting depth exceeded in BPMN sub-processes";
                        auto incident = ProcessDiagnostics::createResourceIncident(
                            ProcError::kExecutionTimeout,
                            "bpmn_import",
                            parser_tracker.getDiagnosticMessage()
                        );
                        SPDLOG_WARN("[bpmn_serializer] {}", incident.toFormattedMessage());
                        return;
                    }
                }
            }
        }
    };

    auto text_cb = [&]([[maybe_unused]] std::string_view txt) {
        if (in_cond_expr) {
          cond_text += std::string(txt);
        }
    };

    // Post-pass: apply BPMNDI layout hints after parsing is complete.
    // (shape_bounds is populated by the BPMNDI callbacks above before nodes
    //  are pushed; this lambda runs after tokenizeXml returns.)
    auto applyBpmndiLayout = [&]() {
        if (shape_bounds.empty()) {
          return;
        }
        for (auto& node : result.nodes) {
            auto it = shape_bounds.find(node.node_id);
            if (it == shape_bounds.end()) {
              continue;
            }
            const auto& b = it->second;
            nlohmann::json layout;
            layout["x"]      = b.x;
            layout["y"]      = b.y;
            layout["width"]  = b.width;
            layout["height"] = b.height;
            node.metadata["layout"] = std::move(layout);
        }
    };

    if (!tokenizeXml(bpmn_xml, tag_cb, text_cb)) {
        return ImportResult::failure(
            ProcessErrorCode::MALFORMED_INPUT,
            "import BPMN from XML",
            "XML tokenization failed (invalid syntax or structure)"
        );
    }

    // Apply BPMNDI graphical layout hints (x/y/width/height) to nodes.
    applyBpmndiLayout();

    // Apply BPMN-S DSGVO annotations to nodes; also persist in metadata JSON
    // so the annotation survives serialisation to the normalized graph in RocksDB.
    for (auto& node : result.nodes) {
        auto it = dsgvo_map.find(node.node_id);
        if (it == dsgvo_map.end()) {
          continue;
        }
        node.dsgvo_annotation = it->second;
        nlohmann::json ann_json;
        ann_json["data_category"]    = it->second.data_category;
        ann_json["legal_basis"]      = it->second.legal_basis;
        ann_json["requires_consent"] = it->second.requires_consent;
        if (it->second.retention_days.has_value()) {
            ann_json["retention_days"] = *it->second.retention_days;
        }
        node.metadata["dsgvo_annotation"] = std::move(ann_json);
    }

    if (result.process_id.empty()) {
        result.process_id   = "imported_process";
        result.process_name = "Imported Process";
    }

    if (result.nodes.empty() && result.edges.empty()) {
        return ImportResult::failure(
            ProcessErrorCode::EMPTY_INPUT,
            "import BPMN from XML",
            "no process nodes or edges found"
        );
    }

    // Validate structure before returning success
    std::string validation_error = validateStructure(result.nodes, result.edges);
    if (!validation_error.empty()) {
        return ImportResult::failure(
            ProcessErrorCode::SEMANTIC_VIOLATION,
            "import BPMN from XML",
            validation_error
        );
    }

    // Return success with populated nodes, edges, and metadata
    return ImportResult::success(
        result.process_id,
        result.process_name,
        std::move(result.nodes),
        std::move(result.edges)
    );
}

// ---------------------------------------------------------------------------
// BpmnSerializer::importFile
// ---------------------------------------------------------------------------

BpmnSerializer::ImportResult BpmnSerializer::importFile(std::string_view file_path) {
    std::ifstream f{std::string(file_path)};
    if (!f.is_open()) {
        return ImportResult::failure(
            ProcessErrorCode::FILE_READ_ERROR,
            "import BPMN from file",
            std::string(file_path)
        );
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return importXml(content);
}

// ---------------------------------------------------------------------------
// BpmnSerializer::exportXml
// ---------------------------------------------------------------------------

std::string BpmnSerializer::exportXml(
    std::string_view                    process_id,
    std::string_view                    process_name,
    const std::vector<ProcessNodeInfo>& nodes,
    const std::vector<ProcessEdgeInfo>& edges)
{
    std::ostringstream xml;

    xml << R"(<?xml version="1.0" encoding="UTF-8"?>)" << "\n";
    xml << R"(<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL")" << "\n";
    xml << R"(             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance")" << "\n";
    xml << R"(             targetNamespace="http://themis.db/process")" << "\n";
    xml << R"(             id="definitions_)" << escapeXml_(process_id) << R"(">)" << "\n\n";

    xml << R"(  <process id=")" << escapeXml_(process_id)
        << R"(" name=")" << escapeXml_(process_name)
        << R"(" isExecutable="true">)" << "\n";

    // Nodes
    for (const auto& n : nodes) {
        if (!std::holds_alternative<BPMNNodeType>(n.node_type)) {
            continue; // Skip EPK nodes in BPMN export
        }
        auto btype = std::get<BPMNNodeType>(n.node_type);

        // Determine actual tag (subtype overrides for tasks)
        std::string tag = nodeTypeToXmlTag_(btype);
        if (btype == BPMNNodeType::TASK && !n.subtype.empty()) {
            if      (n.subtype == "USER_TASK") {
              tag = "userTask";
            }
            else if (n.subtype == "SERVICE_TASK")      tag = "serviceTask";
            else if (n.subtype == "SCRIPT_TASK")       tag = "scriptTask";
            else if (n.subtype == "SEND_TASK")         tag = "sendTask";
            else if (n.subtype == "RECEIVE_TASK")      tag = "receiveTask";
            else if (n.subtype == "MANUAL_TASK")       tag = "manualTask";
            else if (n.subtype == "BUSINESS_RULE_TASK")tag = "businessRuleTask";
        }

        xml << "    <" << tag
            << " id=\"" << escapeXml_(n.node_id) << "\""
            << " name=\"" << escapeXml_(n.name) << "\"";

        if (n.is_async) {
            xml << " themis:isAsync=\"true\"";
        }

        if (n.dsgvo_annotation.has_value()) {
            xml << ">\n";
            xml << "      <extensionElements>\n";
            xml << "        <bpmns:SecurityAnnotation"
                << " xmlns:bpmns=\"http://bpmn-s.org/schema\""
                << " dataCategory=\"" << escapeXml_(n.dsgvo_annotation->data_category) << "\""
                << " legalBasis=\"" << escapeXml_(n.dsgvo_annotation->legal_basis) << "\"";
            if (n.dsgvo_annotation->retention_days.has_value()) {
                xml << " retentionDays=\"" << *n.dsgvo_annotation->retention_days << "\"";
            }
            xml << " requiresConsent=\"" << (n.dsgvo_annotation->requires_consent ? "true" : "false") << "\"";
            xml << "/>\n";
            xml << "      </extensionElements>\n";
            xml << "    </" << tag << ">\n";
        } else {
            xml << "/>\n";
        }
    }

    // Edges
    for (const auto& e : edges) {
        std::string tag;
        switch (e.edge_type) {
            case ProcessEdgeType::MESSAGE_FLOW:    tag = "messageFlow";    break;
            case ProcessEdgeType::ASSOCIATION:     tag = "association";    break;
            case ProcessEdgeType::DATA_ASSOCIATION:tag = "dataInputAssociation"; break;
            default:                               tag = "sequenceFlow";   break;
        }

        xml << "    <" << tag
            << " id=\"" << escapeXml_(e.edge_id) << "\""
            << " sourceRef=\"" << escapeXml_(e.from_node) << "\""
            << " targetRef=\"" << escapeXml_(e.to_node) << "\"";

        if (e.condition_expression) {
            xml << " name=\"" << escapeXml_(*e.condition_expression) << "\"";
        }
        xml << "/>\n";
    }

    xml << "  </process>\n";
    xml << "</definitions>\n";

    return xml.str();
}

// ---------------------------------------------------------------------------
// BpmnSerializer::exportFromJson
// ---------------------------------------------------------------------------

std::string BpmnSerializer::exportFromJson(const json& g) {
    if (g.is_null()) return {};

    std::string pid  = g.value("process_id", "process");
    std::string name = g.value("name", pid);

    std::vector<ProcessNodeInfo> nodes;
    std::vector<ProcessEdgeInfo> edges;

    if (g.contains("nodes")) {
        for (const auto& jn : g["nodes"]) {
            ProcessNodeInfo n;
            n.node_id  = jn.value("id",   "");
            n.name     = jn.value("name", "");
            n.subtype  = jn.value("subtype", "");

            std::string type_str = jn.value("type", "TASK");
            n.node_type = xmlTagToNodeType_(type_str);
            nodes.push_back(std::move(n));
        }
    }

    if (g.contains("edges")) {
        for (const auto& je : g["edges"]) {
            ProcessEdgeInfo e;
            e.edge_id   = je.value("id",   "");
            e.from_node = je.value("from", "");
            e.to_node   = je.value("to",   "");
            std::string cond = je.value("condition", "");
            if (!cond.empty()) {
              e.condition_expression = cond;
            }
            e.edge_type = ProcessEdgeType::SEQUENCE_FLOW;
            edges.push_back(std::move(e));
        }
    }

    return exportXml(pid, name, nodes, edges);
}

// ---------------------------------------------------------------------------
// BpmnSerializer::validateStructure
// ---------------------------------------------------------------------------

std::string BpmnSerializer::validateStructure(
    const std::vector<ProcessNodeInfo>& nodes,
    const std::vector<ProcessEdgeInfo>& edges)
{
    // Resource constraints
    constexpr size_t MAX_NODES = 10000;
    constexpr size_t MAX_EDGES = 50000;

    // 1. Check node count bounds
    if (nodes.size() > MAX_NODES) {
        return "Node count (" + std::to_string(nodes.size()) +
               ") exceeds maximum (" + std::to_string(MAX_NODES) + ")";
    }

    // 2. Check edge count bounds
    if (edges.size() > MAX_EDGES) {
        return "Edge count (" + std::to_string(edges.size()) +
               ") exceeds maximum (" + std::to_string(MAX_EDGES) + ")";
    }

    // 3. Build set of node IDs for validation
    std::set<std::string> node_ids = {};

    for (const auto& node : nodes) {
        if (node.node_id.empty()) {
            return "Node with empty id encountered";
        }
        if (node_ids.count(node.node_id)) {
            return "Duplicate node id: " + node.node_id;
        }
        node_ids.insert(node.node_id);
    }

    // 4. Validate all edges reference existing nodes
    for (const auto& edge : edges) {
        if (edge.from_node.empty() || edge.to_node.empty()) {
            return "Edge with empty from or to node encountered";
        }
        if (node_ids.find(edge.from_node) == node_ids.end()) {
            return "Edge references non-existent source node: " + edge.from_node;
        }
        if (node_ids.find(edge.to_node) == node_ids.end()) {
            return "Edge references non-existent target node: " + edge.to_node;
        }
    }

    // 5. Check for self-loops (allowed but unusual)
    size_t self_loop_count = 0;
    for (const auto& edge : edges) {
        if (edge.from_node == edge.to_node) {
            self_loop_count++;
        }
    }
    if (self_loop_count > nodes.size() / 10) {
        // More than 10% self-loops is suspicious, but not necessarily invalid
        SPDLOG_WARN("[bpmn] High self-loop ratio: {}/{}", self_loop_count, edges.size());
    }

    // 6. Validate node names are reasonable length
    for (const auto& node : nodes) {
        if (node.name.length() > 1000) {
            return "Node name exceeds maximum length: " + node.node_id;
        }
    }

    return "";  // Validation passed
}

} // namespace process
} // namespace themis


