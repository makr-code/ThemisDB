/**
 * @file fim_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: fim_importer.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB - Process Modeling Module
 *
 * File:    fim_importer.h
 * Module:  include/process/
 * Purpose: Import process models from the German FIM (Föderales
 *          Informationsmanagement) Prozessbibliothek (FITKO 2024).
 *
 * FIM defines a reference catalogue of administrative process models in BPMN
 * 2.0 XML.  This importer fetches or parses catalogue documents and converts
 * them to ProcessModelRecord objects ready for storage by ProcessModelManager.
 */

#pragma once

#include "index/process_graph.h"
#include "process/bpmn_serializer.h"
#include "process/process_model_manager.h"
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace process {

/**
 * @brief Result of a single FIM model import operation.
 */
struct FimModelResult {
    bool              ok{false};      ///< True if import succeeded
    std::string       message;        ///< Error description on failure
    ProcessModelRecord record;        ///< Populated on success
};

/**
 * @brief Imports process models from the FIM Prozessbibliothek (FITKO 2024).
 *
 * ## Supported input formats
 * - FIM catalogue XML (`<prozessbibliothek>` root element) containing one or
 *   more `<prozess>` entries whose body is a BPMN 2.0 XML fragment.
 * - Individual FIM BPMN 2.0 XML files (delegated to BpmnSerializer).
 * - FITKO REST API responses (JSON envelope wrapping a BPMN XML payload).
 *
 * ## Usage
 * ```cpp
 * FimImporter imp;
 * auto results = imp.importFimCatalogue(xml_string, ProcessDomain::ADMINISTRATION);
 * for (auto& r : results) {
 *     if (r.ok) manager.save(r.record);
 * }
 * ```
 */
class FimImporter {
public:
    /**
     * @brief Callback type for HTTP GET operations used by importFromFitkoApi().
     *
     * The function receives the full URL string and returns the response body
     * as a UTF-8 string.  An empty string or thrown exception is treated as a
     * fetch failure.  Callers inject a libcurl-backed implementation at startup;
     * unit tests inject a scripted response.
     */
    using HttpFetchFn = std::function<std::string(std::string_view url)>;

    FimImporter() = default;

    /**
     * @brief Inject an HTTP fetch backend for importFromFitkoApi().
     *
     * When set, importFromFitkoApi() delegates the HTTP GET call to @p fn
     * instead of returning the "not yet implemented" error result.
     *
     * @param fn  Callable that performs the HTTP GET and returns the response body.
     */
    void setHttpFetchFn(HttpFetchFn fn) { http_fetch_fn_ = std::move(fn); }

    /**
     * @brief Create a libcurl-backed HttpFetchFn for use with setHttpFetchFn().
     *
     * When built with `THEMIS_HAS_CURL` defined (libcurl linked), returns a
     * concrete HttpFetchFn that performs a real HTTP GET using libcurl with a
     * 10-second timeout and TLS peer verification enabled.  The returned fn is
     * safe to use from multiple threads (each call owns its own CURL handle).
     *
     * When built without libcurl, returns an empty `std::function` so that
     * importFromFitkoApi() falls through to the "not implemented" fallback.
     *
     * **Usage at startup:**
     * @code
     *   importer.setHttpFetchFn(FimImporter::makeCurlHttpFetchFn());
     * @endcode
     *
     * @return A ready-to-use HttpFetchFn or an empty function when libcurl
     *         is unavailable.
     */
    static HttpFetchFn makeCurlHttpFetchFn();

    /**
     * @brief Parse a FIM Prozessbibliothek XML catalogue document.
     *
     * The catalogue may contain multiple `<prozess>` elements.  Each element
     * is converted to a separate ProcessModelRecord with notation BPMN_2_0 and
     * the supplied @p domain classification.
     *
     * @param catalogue_xml  Raw catalogue XML string.
     * @param domain         Domain to assign to imported models.
     * @return One FimModelResult per `<prozess>` element found.
     */
    std::vector<FimModelResult> importFimCatalogue(
        std::string_view catalogue_xml,
        ProcessDomain    domain = ProcessDomain::ADMINISTRATION);

    /**
     * @brief Import a single FIM BPMN 2.0 XML document.
     *
     * Delegates BPMN parsing to BpmnSerializer and wraps the result in a
     * ProcessModelRecord.
     *
     * @param bpmn_xml  BPMN 2.0 XML string (FIM-compliant).
     * @param domain    Domain classification for the model.
     * @return FimModelResult with populated record on success.
     */
    FimModelResult importSingleModel(
        std::string_view bpmn_xml,
        ProcessDomain    domain = ProcessDomain::ADMINISTRATION);

    /**
     * @brief Fetch and import process models from the FITKO REST API.
     *
     * When an HttpFetchFn has been injected via setHttpFetchFn(), calls
     * @p api_base_url + "/prozesse" and parses the JSON response envelope
     * (expected: `{"items": [{"bpmnXml": "..."}]}`), importing each BPMN
     * payload via importSingleModel().  Without an injected fetch function,
     * returns a single error result explaining that HTTP transport is not
     * configured.
     *
     * @param api_base_url  Base URL of the FITKO FIM API (without trailing slash).
     * @param domain        Domain classification.
     * @return One FimModelResult per model retrieved.
     *
     * @note This function emits SPDLOG_WARN entries for non-fatal API errors
     *       (e.g. individual model payloads that fail to parse).
     */
    std::vector<FimModelResult> importFromFitkoApi(
        std::string_view api_base_url,
        ProcessDomain    domain = ProcessDomain::ADMINISTRATION);

private:
    HttpFetchFn http_fetch_fn_;  ///< Optional HTTP GET backend; null = stub path.

    // Internal helper — wraps a BpmnSerializer::ImportResult into a ProcessModelRecord.
    static ProcessModelRecord buildRecord_(
        const BpmnSerializer::ImportResult& ir,
        ProcessDomain domain);
};

} // namespace process
} // namespace themis
