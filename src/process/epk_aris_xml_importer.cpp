/**
 * @file epk_aris_xml_importer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    epk_aris_xml_importer.cpp
 * Module:  src/process/
 * Purpose: EPK import from ARIS Markup Language (AML) XML.
 *
 * Implementation uses the same hand-written, regex-free XML tokenizer
 * pattern as BpmnSerializer.  No external XML library is required.
 */

#include "process/epk_aris_xml_importer.h"
#include <stdexcept>
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <unordered_map>

namespace themis {
namespace process {

// ---------------------------------------------------------------------------
// Internal XML tokenizer (shared with BpmnSerializer pattern)
// ---------------------------------------------------------------------------

namespace {

/// Strip XML character entities and surrounding whitespace.
std::string unescapeAml(std::string_view s) {
    std::string out = {};
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ) {
        if (s[i] != '&') { out += s[i++]; continue; }
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
        else if (static_cast<int>(ent.size()) > 2 && ent[1] == '#') {
            // Numeric character reference: &#N; or &#xNN;
            std::string_view num = ent.substr(2, static_cast<int>(ent.size()) - 3);
            try {
                unsigned long cp = ((!num.empty()) && (num[0] == 'x' || num[0] == 'X'))
                    ? std::stoul(std::string(num.substr(1)), nullptr, 16)
                    : std::stoul(std::string(num), nullptr, 10);
                if (cp < 0x80u) {
                    out += static_cast<char>(cp);
                } else if (cp < 0x800u) {
                    out += static_cast<char>(0xC0u | (cp >> 6));
                    out += static_cast<char>(0x80u | (cp & 0x3Fu));
                } else if (cp < 0x10000u) {
                    out += static_cast<char>(0xE0u | (cp >> 12));
                    out += static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu));
                    out += static_cast<char>(0x80u | (cp & 0x3Fu));
                } else {
                    out += std::string(ent); // keep as-is for supplementary planes
                }
            } catch (...) {
                out += std::string(ent);
            }
        }
        else                      out += std::string(ent);
        i = semi + 1;
    }
    // trim
    size_t a = out.find_first_not_of(" \t\n\r");
    if (a == std::string::npos) return {};
    size_t b = out.find_last_not_of(" \t\n\r");
    return out.substr(a, b - a + 1);
}

/// Strip namespace prefix: "Group.ID" stays; "ns:Model" → "Model".
std::string_view stripNs(std::string_view name) {
    auto colon = name.rfind(':');
    return (colon != std::string_view::npos) ? name.substr(colon + 1) : name;
}

struct XmlTag {
    std::string name;
    std::map<std::string, std::string> attrs;
    bool self_closing{false};
    bool is_close{false};
};

void parseAttrs(std::string_view src,
                std::map<std::string, std::string>& out)
{
    size_t i = 0;
    const size_t n = src.size();
    while (i < n) {
        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) {
          ++i;
        }
        if (i >= n || src[i] == '/' || src[i] == '>') {
          break;
        }

        size_t ns = i;
        while (i < n && src[i] != '=' && src[i] != '/' && src[i] != '>' &&
               !std::isspace(static_cast<unsigned char>(src[i]))) ++i;
        if (i <= ns) {
          break;
        }
        // Keep full attribute name including dots (e.g. "Group.ID", "ObjDef.IdRef")
        std::string attr_name = std::string(src.substr(ns, i - ns));

        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) {
          ++i;
        }
        if (i >= n || src[i] != '=') {
            if (!attr_name.empty()) {
              out.emplace(std::move(attr_name), "true");
            }
            continue;
        }
        ++i;

        while (i < n && std::isspace(static_cast<unsigned char>(src[i]))) {
          ++i;
        }
        if (i >= n) {
          break;
        }

        std::string attr_val = {};
        if (src[i] == '"' || src[i] == '\'') {
            char q = src[i++];
            size_t vs = i;
            while (i < n && src[i] != q) {
              ++i;
            }
            attr_val = unescapeAml(src.substr(vs, i - vs));
            if (i < n) {
              ++i;
            }
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

template<typename TagCb, typename TextCb>
bool tokenizeXml(std::string_view xml,
                 size_t max_bytes,
                 TagCb tag_cb, TextCb text_cb)
{
    if (static_cast<int>(xml.size()) > max_bytes) {
      return false;
    }

    size_t i = 0;
    const size_t n = xml.size();

    while (i < n) {
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
        // <!... >
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

        bool is_close = false;
        if (xml[i] == '/') { is_close = true; ++i; }

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

        if (self_close) {
          i += 2;
        }
        else if (i < n) ++i;

        tag_cb(tag);
    }
    return true;
}

// ---------------------------------------------------------------------------
// AML model extraction
// ---------------------------------------------------------------------------

/// ARIS model representation before post-processing.
struct ArisModel {
    std::string model_id;
    std::string model_type;   // e.g. "EPK"
    std::string model_name;

    // ObjOcc.ID → ObjDef.ID mapping (occurrence in the model)
    std::map<std::string, std::string> occ_to_def;
    // ObjOcc.ID → SymbolNum
    std::map<std::string, int> occ_sym;

    // CxnOcc: (from_occ_id, to_occ_id, cxn_def_id)
    struct CxnOcc {
        std::string cxn_occ_id;
        std::string from_occ_id;
        std::string to_occ_id;
        std::string cxn_def_id;
    };
    std::vector<CxnOcc> cxn_occs;
};

/// Per-Group / whole-file ObjDef registry.
struct ObjDefInfo {
    int type_num{0};
    std::string name;
};

/// Parse all AML models + ObjDef/CxnDef from an XML string.
struct AmlParseResult {
    std::vector<ArisModel> models;
    std::map<std::string, ObjDefInfo> obj_defs; // ObjDef.ID → info
    bool ok{true};
    std::string message;
};

AmlParseResult parseAml(std::string_view xml, size_t max_bytes)
{
    AmlParseResult result;

    // State machine
    enum class Context {
        NONE,
        MODEL,          // inside <Model>
        MODEL_NAME,     // inside <Model.Name>
        OBJ_DEF,        // inside <ObjDef>
        OBJ_DEF_NAME,   // inside <ObjDef.Name>
    };

    Context ctx = Context::NONE;
    ArisModel current_model;
    ObjDefInfo current_obj;
    std::string current_obj_id = {};
    bool in_epk_model = false;

    auto flush_model = [&]() {
        if (!current_model.model_id.empty()) {
            result.models.push_back(current_model);
            current_model = {};
        }
        in_epk_model = false;
    };

    auto flush_obj_def = [&]() {
        if (!current_obj_id.empty()) {
            result.obj_defs[current_obj_id] = current_obj;
            current_obj_id.clear();
            current_obj = {};
        }
    };

    auto tag_cb = [&]([[maybe_unused]] const XmlTag& tag) {
        if (tag.is_close) {
            if (tag.name == "Model") {
                flush_model();
                ctx = Context::NONE;
            } else if (tag.name == "Model.Name") {
                ctx = (in_epk_model) ? Context::MODEL : Context::NONE;
            } else if (tag.name == "ObjDef") {
                flush_obj_def();
                ctx = Context::NONE;
            } else if (tag.name == "ObjDef.Name") {
                ctx = Context::OBJ_DEF;
            }
            return;
        }

        if (tag.name == "Model") {
            flush_model();
            auto it_id   = tag.attrs.find("Model.ID");
            auto it_type = tag.attrs.find("Model.Type");
            current_model.model_id   = (it_id   != tag.attrs.end()) ? it_id->second   : "";
            current_model.model_type = (it_type != tag.attrs.end()) ? it_type->second : "";
            // Normalise type to upper-case
            std::transform(current_model.model_type.begin(),
                           current_model.model_type.end(),
                           current_model.model_type.begin(),
                           [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
            in_epk_model = (current_model.model_type == "EPK" ||
                            current_model.model_type.find("EPK") != std::string::npos);
            ctx = (in_epk_model) ? Context::MODEL : Context::NONE;
            return;
        }

        if (tag.name == "Model.Name" && ctx == Context::MODEL) {
            ctx = Context::MODEL_NAME;
            return;
        }

        if (tag.name == "ObjOcc" && ctx == Context::MODEL) {
            auto it_id  = tag.attrs.find("ObjOcc.ID");
            auto it_ref = tag.attrs.find("ObjDef.IdRef");
            auto it_sym = tag.attrs.find("SymbolNum");
            if (it_id != tag.attrs.end() && it_ref != tag.attrs.end()) {
                current_model.occ_to_def[it_id->second] = it_ref->second;
                if (it_sym != tag.attrs.end()) {
                    try {
                        current_model.occ_sym[it_id->second] =
                            std::stoi(it_sym->second);
                    } catch (...) {}
                }
            }
            return;
        }

        if (tag.name == "CxnOcc" && ctx == Context::MODEL) {
            auto it_id   = tag.attrs.find("CxnOcc.ID");
            auto it_from = tag.attrs.find("FromObjOcc.IdRef");
            auto it_to   = tag.attrs.find("ToObjOcc.IdRef");
            auto it_def  = tag.attrs.find("CxnDef.IdRef");
            if (it_from != tag.attrs.end() && it_to != tag.attrs.end()) {
                ArisModel::CxnOcc cx;
                cx.cxn_occ_id = (it_id  != tag.attrs.end()) ? it_id->second  : "";
                cx.from_occ_id = it_from->second;
                cx.to_occ_id   = it_to->second;
                cx.cxn_def_id  = (it_def != tag.attrs.end()) ? it_def->second : "";
                current_model.cxn_occs.push_back(std::move(cx));
            }
            return;
        }

        if (tag.name == "ObjDef") {
            flush_obj_def();
            auto it_id  = tag.attrs.find("ObjDef.ID");
            auto it_num = tag.attrs.find("TypeNum");
            current_obj_id = (it_id != tag.attrs.end()) ? it_id->second : "";
            if (it_num != tag.attrs.end()) {
                try { current_obj.type_num = std::stoi(it_num->second); }
                catch (...) {}
            }
            ctx = Context::OBJ_DEF;
            return;
        }

        if (tag.name == "ObjDef.Name" && ctx == Context::OBJ_DEF) {
            ctx = Context::OBJ_DEF_NAME;
            return;
        }
    };

    std::string pending_text = {};
    auto text_cb = [&]([[maybe_unused]] std::string_view text) {
        if (ctx == Context::MODEL_NAME) {
            current_model.model_name += std::string(text);
        } else if (ctx == Context::OBJ_DEF_NAME) {
            current_obj.name += std::string(text);
        }
    };

    if (!tokenizeXml(xml, max_bytes, tag_cb, text_cb)) {
        result.ok = false;
        result.message = "AML document exceeds maximum allowed size";
        return result;
    }

    // Flush any trailing model or ObjDef
    flush_model();
    flush_obj_def();

    // Clean up names
    for (auto& m : result.models) {
        size_t a = m.model_name.find_first_not_of(" \t\n\r");
        if (a != std::string::npos) {
            size_t b = m.model_name.find_last_not_of(" \t\n\r");
            m.model_name = m.model_name.substr(a, b - a + 1);
        }
    }
    for (auto& [id, info] : result.obj_defs) {
        size_t a = info.name.find_first_not_of(" \t\n\r");
        if (a != std::string::npos) {
            size_t b = info.name.find_last_not_of(" \t\n\r");
            info.name = info.name.substr(a, b - a + 1);
        }
    }

    return result;
}

/// Convert a parsed ArisModel + ObjDef registry into an ImportResult.
EpkArisXmlImporter::ImportResult buildImportResult(
    const ArisModel& model,
    const std::map<std::string, ObjDefInfo>& obj_defs)
{
    EpkArisXmlImporter::ImportResult res;
    res.ok          = true;
    res.process_id  = model.model_id;
    res.process_name = model.model_name;
    if (res.process_name.empty()) {
        res.process_name = "EPK_" + model.model_id;
    }

    // Build nodes: one node per ObjOcc in model order.
    for (const auto& [occ_id, def_id] : model.occ_to_def) {
        ProcessNodeInfo node;
        node.node_id = occ_id; // use occurrence ID as unique node ID

        // Resolve name and TypeNum from ObjDef registry
        auto def_it = obj_defs.find(def_id);
        if (def_it != obj_defs.end()) {
            node.name = def_it->second.name;
            node.node_type = EpkArisXmlImporter::typeNumToEpkNodeType(
                def_it->second.type_num);
        } else {
            // Fall back to SymbolNum if ObjDef not found (cross-group reference)
            auto sym_it = model.occ_sym.find(occ_id);
            int sym = (sym_it != model.occ_sym.end()) ? sym_it->second : 1;
            node.node_type = EpkArisXmlImporter::typeNumToEpkNodeType(sym);
        }

        // Override node type with SymbolNum when available (SymbolNum is
        // more specific than TypeNum for occurrence-level rendering).
        auto sym_it = model.occ_sym.find(occ_id);
        if (sym_it != model.occ_sym.end()) {
            node.node_type = EpkArisXmlImporter::typeNumToEpkNodeType(
                sym_it->second);
        }

        res.nodes.push_back(std::move(node));
    }

    // Build edges from CxnOcc.
    int edge_counter = 0;
    for (const auto& cx : model.cxn_occs) {
        ProcessEdgeInfo edge;
        edge.edge_id   = cx.cxn_occ_id.empty()
                         ? ("e" + std::to_string(++edge_counter))
                         : cx.cxn_occ_id;
        edge.from_node = cx.from_occ_id;
        edge.to_node   = cx.to_occ_id;
        edge.edge_type = ProcessEdgeType::CONTROL_FLOW;
        res.edges.push_back(std::move(edge));
    }

    return res;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// EpkArisXmlImporter – public API
// ---------------------------------------------------------------------------

EPKNodeType EpkArisXmlImporter::typeNumToEpkNodeType([[maybe_unused]] int type_num) {
    switch (type_num) {
        case  1: return EPKNodeType::FUNCTION;
        case 14: return EPKNodeType::EVENT;
        case 13: return EPKNodeType::AND_CONNECTOR;
        case 12: return EPKNodeType::OR_CONNECTOR;
        case 11: return EPKNodeType::XOR_CONNECTOR;
        case 18: return EPKNodeType::ORGANIZATIONAL_UNIT;
        case 15: return EPKNodeType::INFORMATION_OBJECT;
        case 40: return EPKNodeType::APPLICATION_SYSTEM;
        case 16: return EPKNodeType::PROCESS_PATH;
        default: return EPKNodeType::FUNCTION;
    }
}

std::string_view EpkArisXmlImporter::typeNumToLabel([[maybe_unused]] int type_num) {
    switch (type_num) {
        case  1: return "Funktion";
        case 14: return "Ereignis";
        case 13: return "UND-Verknüpfungsoperator";
        case 12: return "ODER-Verknüpfungsoperator";
        case 11: return "XOR-Verknüpfungsoperator";
        case 18: return "Organisationseinheit";
        case 15: return "Informationsobjekt";
        case 40: return "Anwendungssystem";
        case 16: return "Prozesswegweiser";
        default: return "Unbekannt";
    }
}

EpkArisXmlImporter::ImportResult
EpkArisXmlImporter::importAml(std::string_view aml_xml)
{
    if (aml_xml.empty()) {
        ImportResult r;
        r.ok = false;
        r.message = "AML XML is empty";
        return r;
    }

    auto parsed = parseAml(aml_xml, kMaxAmlBytes);
    if (!parsed.ok) {
        ImportResult r;
        r.ok = false;
        r.message = parsed.message;
        return r;
    }

    // Find the first EPK model
    for (const auto& m : parsed.models) {
        if (m.model_type == "EPK" ||
            m.model_type.find("EPK") != std::string::npos)
        {
            auto res = buildImportResult(m, parsed.obj_defs);
            if (res.nodes.empty() && res.edges.empty()) {
                SPDLOG_WARN("[process] ARIS-XML model '{}' has no nodes/edges",
                            m.model_id);
            }
            return res;
        }
    }

    ImportResult r;
    r.ok = false;
    r.message = "No EPK model found in AML document";
    return r;
}

std::vector<EpkArisXmlImporter::ImportResult>
EpkArisXmlImporter::importAllAml(std::string_view aml_xml)
{
    std::vector<ImportResult> results;

    if (aml_xml.empty()) {
      return results;
    }

    auto parsed = parseAml(aml_xml, kMaxAmlBytes);
    if (!parsed.ok) {
      return results;
    }

    for (const auto& m : parsed.models) {
        if (m.model_type == "EPK" ||
            m.model_type.find("EPK") != std::string::npos)
        {
            results.push_back(buildImportResult(m, parsed.obj_defs));
        }
    }

    return results;
}

} // namespace process
} // namespace themis


