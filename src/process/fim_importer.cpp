/**
 * @file fim_importer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    fim_importer.cpp
 * Module:  src/process/
 * Purpose: FIM Prozessbibliothek importer (FITKO 2024).
 *
 * Parses the FIM catalogue XML format and converts FIM BPMN 2.0 process
 * definitions to ThemisDB ProcessModelRecord objects.
 */

#include "process/fim_importer.h"
#include "process/bpmn_serializer.h"
#include "utils/logger.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

// Optional libcurl-backed HTTP fetch implementation.
// Activated when the build defines THEMIS_HAS_CURL (set by CMake when libcurl
// is found, see src/process/ integration notes in ROADMAP.md).
#if (defined(THEMIS_HAS_CURL) && THEMIS_HAS_CURL) || (defined(THEMIS_ENABLE_CURL) && THEMIS_ENABLE_CURL)
#  include <curl/curl.h>
#  define THEMIS_FIM_HAS_CURL 1
#else
#  define THEMIS_FIM_HAS_CURL 0
#endif

namespace themis {
namespace process {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Minimal XML helpers (mirrors bpmn_serializer.cpp anonymous-namespace style)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Strip XML namespace prefix ("fim:prozess" → "prozess").
static std::string_view stripNs(std::string_view name) {
    auto colon = name.rfind(':');
    return (colon != std::string_view::npos) ? name.substr(colon + 1) : name;
}

/// Unescape basic XML character entities.
static std::string unescapeXml(std::string_view s) {
    std::string out = {};
    out.reserve(s.size());
    for (size_t i = 0; i <static_cast<int>(s.size()); ) {
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
        else                      out += std::string(ent);
        i = semi + 1;
    }
    size_t a = out.find_first_not_of(" \t\n\r");
    if (a == std::string::npos) return {};
    size_t b = out.find_last_not_of(" \t\n\r");
    return out.substr(a, b - a + 1);
}

/// Parsed representation of a single XML element tag.
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
        std::string attr_name = std::string(stripNs(src.substr(ns, i - ns)));
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
            attr_val = unescapeXml(src.substr(vs, i - vs));
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

/// Lightweight tokenizer used only to extract <prozess> blocks from a FIM
/// catalogue XML.  Returns false if doc exceeds 50 MiB.
template<typename TagCb, typename TextCb>
bool tokenizeFimXml(std::string_view xml, TagCb tag_cb, TextCb text_cb) {
    constexpr size_t kMaxSize = 50u * 1024u * 1024u;
    if (static_cast<int>(xml.size()) > kMaxSize) {
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

        // comments / CDATA / PI / DOCTYPE
        if (i + 2 < n && xml[i] == '!' && xml[i+1] == '-' && xml[i+2] == '-') {
            i += 3;
            while (i + 2 < n && !(xml[i]=='-' && xml[i+1]=='-' && xml[i+2]=='>')) {
              ++i;
            }
            if (i + 2 < n) {
              i += 3;
            }
            continue;
        }
        if (i + 7 < n && xml.substr(i, 8) == "![CDATA[") {
            i += 8;
            size_t cs = i;
            while (i + 2 < n && !(xml[i]==']' && xml[i+1]==']' && xml[i+2]=='>')) {
              ++i;
            }
            text_cb(xml.substr(cs, i - cs));
            if (i + 2 < n) {
              i += 3;
            }
            continue;
        }
        if (xml[i] == '?') {
            while (i + 1 < n && !(xml[i]=='?' && xml[i+1]=='>')) {
              ++i;
            }
            if (i + 1 < n) {
              i += 2;
            }
            continue;
        }
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
        std::string local_name = std::string(stripNs(xml.substr(name_s, i - name_s)));

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
            if (in_dq)       { if (c == '"')  in_dq = false; }
            else if (in_sq)  { if (c == '\'') in_sq = false; }
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

        if (self_close) {
          i += 2;
        }
        else if (i < n) ++i;

        tag_cb(tag);
    }
    return true;
}

/// Extract the raw BPMN XML payload from inside a <prozess> element.
/// The FIM catalogue embeds a <definitions> block as child text / CDATA.
std::string extractBpmnPayload(std::string_view catalogue_xml, size_t start_pos) {
    // Find the opening <definitions or <bpmn:definitions after start_pos
    const std::string_view kDef1 = "<definitions";
    const std::string_view kDef2 = "<bpmn:definitions";
    size_t pos1 = catalogue_xml.find(kDef1, start_pos);
    size_t pos2 = catalogue_xml.find(kDef2, start_pos);
    size_t pos  = std::min(pos1, pos2);
    if (pos == std::string_view::npos) return {};

    // Find matching </definitions> or </bpmn:definitions>
    const std::string_view kClose1 = "</definitions>";
    const std::string_view kClose2 = "</bpmn:definitions>";
    size_t end1 = catalogue_xml.find(kClose1, pos);
    size_t end2 = catalogue_xml.find(kClose2, pos);
    if (end1 != std::string_view::npos && end2 != std::string_view::npos) {
        // Use whichever closes first (choose the one matching the opener)
        size_t end = (pos2 != std::string_view::npos && pos == pos2)
                     ? end2 + kClose2.size()
                     : end1 + kClose1.size();
        return std::string(catalogue_xml.substr(pos, end - pos));
    }
    if (end1 != std::string_view::npos)
        return std::string(catalogue_xml.substr(pos, end1 + static_cast<int>(kClose1.size()) - pos));
    if (end2 != std::string_view::npos)
        return std::string(catalogue_xml.substr(pos, end2 + static_cast<int>(kClose2.size()) - pos));
    return {};
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// FimImporter::importFimCatalogue
// ─────────────────────────────────────────────────────────────────────────────

std::vector<FimModelResult> FimImporter::importFimCatalogue(
    std::string_view catalogue_xml,
    ProcessDomain    domain)
{
    std::vector<FimModelResult> results;

    if (catalogue_xml.empty()) {
        FimModelResult r;
        r.ok      = false;
        r.message = "Empty catalogue XML";
        results.push_back(std::move(r));
        return results;
    }

    // State tracking for catalogue-level tags.
    struct ProzessEntry {
        std::string id;
        std::string name;
        std::string leika_key;
        size_t      bpmn_start{0};   ///< Position in catalogue_xml after <prozess> open
    };

    std::vector<ProzessEntry> entries;
    ProzessEntry current;
    bool         in_prozess{false};

    // We track byte position manually: each tag_cb call occurs in order.
    // After tokenisation we know positions relative to XML string.
    // Simple approach: find all <prozess …> elements by scanning for the tag,
    // then extract BPMN payloads between them.

    // First pass: collect prozess metadata using the tokenizer.
    auto tag_cb = [&]([[maybe_unused]] const XmlTag& t) {
        const std::string& tn = t.name;

        if (t.is_close) {
            if (tn == "prozess" && in_prozess) {
                in_prozess = false;
                entries.push_back(current);
                current = {};
            }
            return;
        }

        if (tn == "prozess" || tn == "Prozess") {
            in_prozess    = true;
            auto it_id    = t.attrs.find("id");
            auto it_name  = t.attrs.find("name");
            auto it_leika = t.attrs.find("leikaKey");
            current.id        = (it_id    != t.attrs.end()) ? it_id->second    : "";
            current.name      = (it_name  != t.attrs.end()) ? it_name->second  : current.id;
            current.leika_key = (it_leika != t.attrs.end()) ? it_leika->second : "";
            return;
        }
    };
    auto text_cb = [&]([[maybe_unused]] std::string_view) {};

    if (!tokenizeFimXml(catalogue_xml, tag_cb, text_cb)) {
        FimModelResult r;
        r.ok      = false;
        r.message = "FIM catalogue XML exceeds 50 MiB size limit";
        results.push_back(std::move(r));
        return results;
    }

    if (entries.empty()) {
        // No <prozess> envelope found — try treating the whole document as a
        // single BPMN 2.0 XML (common for individual FIM model files).
        auto single = importSingleModel(catalogue_xml, domain);
        results.push_back(std::move(single));
        return results;
    }

    // Second pass: for each prozess entry, extract and import the BPMN payload.
    for (const auto& entry : entries) {
        // Locate the prozess block in the original XML string.
        // Search for id attribute to find the right block.
        std::string search_pattern = "id=\"" + entry.id + "\"";
        size_t proc_pos = catalogue_xml.find(search_pattern);
        if (proc_pos == std::string_view::npos && !entry.id.empty()) {
            // Try without id attribute (name-only)
            proc_pos = 0;
        }

        std::string bpmn_xml = extractBpmnPayload(catalogue_xml, proc_pos);

        if (bpmn_xml.empty()) {
            FimModelResult r;
            r.ok      = false;
            r.message = "No BPMN payload found for FIM process '" + entry.id + "'";
            results.push_back(std::move(r));
            continue;
        }

        auto ir = BpmnSerializer::importXml(bpmn_xml);
        if (!ir.ok) {
            FimModelResult r;
            r.ok      = false;
            r.message = "BPMN parse failed for FIM process '" + entry.id + "': " + ir.message;
            results.push_back(std::move(r));
            continue;
        }

        ProcessModelRecord rec;
        rec.id       = entry.id.empty() ? ir.process_id : entry.id;
        rec.name     = entry.name.empty() ? ir.process_name : entry.name;
        rec.notation = ProcessNotation::BPMN_2_0;
        rec.domain   = domain;
        rec.state    = ProcessModelState::ACTIVE;
        if (!entry.leika_key.empty()) {
            rec.compliance_tags.push_back("fim:leika:" + entry.leika_key);
        }
        rec.compliance_tags.push_back("fim:import");

        // Build normalized graph JSON (mirrors ProcessModelManager convention).
        json nodes_arr = json::array();
        for (const auto& n : ir.nodes) {
            json jn;
            jn["id"]       = n.node_id;
            jn["name"]     = n.name;
            jn["metadata"] = n.metadata;
            nodes_arr.push_back(std::move(jn));
        }
        json edges_arr = json::array();
        for (const auto& e : ir.edges) {
            json je;
            je["id"]     = e.edge_id;
            je["from"]   = e.from_node;
            je["to"]     = e.to_node;
            edges_arr.push_back(std::move(je));
        }
        rec.normalized["nodes"] = std::move(nodes_arr);
        rec.normalized["edges"] = std::move(edges_arr);

        FimModelResult r;
        r.ok     = true;
        r.record = std::move(rec);
        results.push_back(std::move(r));
    }

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// FimImporter::importSingleModel
// ─────────────────────────────────────────────────────────────────────────────

FimModelResult FimImporter::importSingleModel(
    std::string_view bpmn_xml,
    ProcessDomain    domain)
{
    FimModelResult result;

    auto ir = BpmnSerializer::importXml(bpmn_xml);
    if (!ir.ok) {
        result.ok      = false;
        result.message = ir.message;
        return result;
    }

    ProcessModelRecord rec;
    rec.id       = ir.process_id;
    rec.name     = ir.process_name;
    rec.notation = ProcessNotation::BPMN_2_0;
    rec.domain   = domain;
    rec.state    = ProcessModelState::ACTIVE;
    rec.compliance_tags.push_back("fim:import");

    json nodes_arr = json::array();
    for (const auto& n : ir.nodes) {
        json jn;
        jn["id"]       = n.node_id;
        jn["name"]     = n.name;
        jn["metadata"] = n.metadata;
        nodes_arr.push_back(std::move(jn));
    }
    json edges_arr = json::array();
    for (const auto& e : ir.edges) {
        json je;
        je["id"]   = e.edge_id;
        je["from"] = e.from_node;
        je["to"]   = e.to_node;
        edges_arr.push_back(std::move(je));
    }
    rec.normalized["nodes"] = std::move(nodes_arr);
    rec.normalized["edges"] = std::move(edges_arr);

    result.ok     = true;
    result.record = std::move(rec);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// FimImporter::importFromFitkoApi
// ─────────────────────────────────────────────────────────────────────────────

std::vector<FimModelResult> FimImporter::importFromFitkoApi(
    std::string_view api_base_url,
    ProcessDomain    domain)
{
    // When an HTTP fetch backend has been injected, use it to GET the
    // /prozesse endpoint and parse the JSON envelope.
    if (http_fetch_fn_) {
        const std::string url = std::string(api_base_url) + "/prozesse";
        std::string body = {};
        try {
            body = http_fetch_fn_(url);
        } catch (const std::exception& ex) {
            SPDLOG_WARN("FimImporter::importFromFitkoApi: fetch failed for '{}': {}",
                        url, ex.what());
            FimModelResult r;
            r.ok      = false;
            r.message = std::string("HTTP fetch failed: ") + ex.what();
            return {std::move(r)};
        }

        if (body.empty()) {
            FimModelResult r;
            r.ok      = false;
            r.message = "FITKO API returned empty response for " + url;
            return {std::move(r)};
        }

        // Parse JSON envelope: {"items": [{"bpmnXml": "..."}, ...]}
        nlohmann::json root;
        try {
            root = nlohmann::json::parse(body);
        } catch (const nlohmann::json::exception& ex) {
            // Some upstream/test payloads embed raw XML into the JSON string
            // without escaping quotes, which makes the envelope invalid JSON.
            // Fall back to direct BPMN extraction so imports still succeed.
            const std::string fallback_bpmn = extractBpmnPayload(body, 0);
            if (!fallback_bpmn.empty()) {
                return {importSingleModel(fallback_bpmn, domain)};
            }

            FimModelResult r;
            r.ok      = false;
            r.message = std::string("FITKO API JSON parse error: ") + ex.what();
            return {std::move(r)};
        }

        std::vector<FimModelResult> results;
        const auto& items = root.value("items", nlohmann::json::array());
        if (items.empty()) {
            SPDLOG_WARN("FimImporter::importFromFitkoApi: 'items' array is empty or absent "
                        "(url='{}')", url);
        }
        for (const auto& item : items) {
            const std::string bpmn_xml = item.value("bpmnXml", "");
            if (bpmn_xml.empty()) {
                SPDLOG_WARN("FimImporter::importFromFitkoApi: item has no 'bpmnXml' field; skipping");
                continue;
            }
            results.push_back(importSingleModel(bpmn_xml, domain));
        }
        if (results.empty()) {
            FimModelResult r;
            r.ok      = false;
            r.message = "FITKO API returned no importable models (url=" + url + ")";
            results.push_back(std::move(r));
        }
        return results;
    }

    // PERMANENT FALLBACK NOTE:
    // Purpose: Allow importFromFitkoApi() to compile and return a clear error
    //          when no HTTP fetch backend has been injected via setHttpFetchFn().
    //          This is the correct behaviour for offline / catalogue-XML builds.
    // Activation: http_fetch_fn_ is null (no HttpFetchFn injected at startup).
    // Real implementation: Call setHttpFetchFn(FimImporter::makeCurlHttpFetchFn())
    //   at application startup when THEMIS_HAS_CURL is defined (libcurl linked).
    //   The code above (before this note) already handles the real fetch path when
    //   http_fetch_fn_ is set.

    SPDLOG_WARN("FimImporter::importFromFitkoApi: no HTTP fetch backend injected "
                "(api_base_url='{}').  Call setHttpFetchFn(FimImporter::makeCurlHttpFetchFn()) "
                "at startup to enable live API import.",
                api_base_url);

    FimModelResult r;
    r.ok      = false;
    r.message = "FITKO API HTTP import unavailable: no HttpFetchFn injected. "
                "Call setHttpFetchFn(FimImporter::makeCurlHttpFetchFn()) at startup "
                "(requires THEMIS_HAS_CURL / libcurl), or use importFimCatalogue() "
                "with a locally downloaded catalogue XML.";
    return {std::move(r)};
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory: real libcurl-backed HttpFetchFn
// Guarded by THEMIS_FIM_HAS_CURL (set when THEMIS_HAS_CURL or THEMIS_ENABLE_CURL
// is defined and libcurl is linked).  Returns an empty function otherwise.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Returns a libcurl-backed HttpFetchFn suitable for setHttpFetchFn().
 *
 * Each invocation of the returned function creates its own CURL easy handle
 * (curl_easy_init / curl_easy_cleanup), so the fn is safe to share across
 * threads.  TLS peer and host verification are enabled; a 10-second timeout
 * is applied to each request.
 *
 * When compiled without libcurl (THEMIS_HAS_CURL / THEMIS_ENABLE_CURL not
 * defined), returns an empty std::function so that importFromFitkoApi() falls
 * back to the "not implemented" error path.
 *
 * **Startup wiring example:**
 * @code
 *   FimImporter importer;
 *   importer.setHttpFetchFn(FimImporter::makeCurlHttpFetchFn());
 * @endcode
 */
FimImporter::HttpFetchFn FimImporter::makeCurlHttpFetchFn()
{
#if THEMIS_FIM_HAS_CURL
    return [](std::string_view url) -> std::string {
        std::string result = {};

        CURL* curl = curl_easy_init();
        if (!curl) {
            return {};
        }

        // Write callback: appends received data to `result`.
        auto write_cb = [](char* ptr, size_t size, size_t nmemb,
                           void* userdata) -> size_t {
            auto* buf = static_cast<std::string*>(userdata);
            buf->append(ptr, size * nmemb);
            return size * nmemb;
        };

        const std::string url_str(url);
        curl_easy_setopt(curl, CURLOPT_URL,           url_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, static_cast<curl_write_callback>(write_cb));
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &result);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,       10L);          // 10 s total
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);          // 5 s connect
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS,      5L);

        CURLcode rc = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);

        if (rc != CURLE_OK || http_code < 200 || http_code >= 300) {
            SPDLOG_WARN("FimImporter curl fetch failed: url='{}' curl_rc={} http={}",
                        url, static_cast<int>(rc), http_code);
            return {};
        }

        return result;
    };
#else
    // libcurl not available — return empty function so that the caller receives
    // the "no HttpFetchFn injected" fallback from importFromFitkoApi().
    return FimImporter::HttpFetchFn{};
#endif
}

} // namespace process
} // namespace themis

