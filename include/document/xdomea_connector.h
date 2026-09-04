/**
 * @file xdomea_connector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — XDOMEA Document Management Connector
 *
 * Connector for the XDOMEA standard (XML Datenaustausch in der öffentlichen
 * Verwaltung) — the authoritative German federal specification for
 * electronic document management (DMS/VBS) and records management (RM)
 * between public authorities, published by the KoSIT (Koordinierungsstelle
 * für IT-Standards).
 *
 * Supported XDOMEA versions:
 *   2.1  — BSI / KoSIT, 2011–2018 (legacy, still in widespread use)
 *   3.0  — KoSIT, 2020+ (current)
 *
 * Object types (Objekttypen):
 *   Schriftgut (records), Akten (dossiers / folders), Vorgänge (processes),
 *   Dokumente (documents), Dateien (electronic files), Benutzer (users).
 *
 * Message types (Nachrichtentypen) supported:
 *   0201 — Anbietung (offer for archival)
 *   0202 — Bewertung (appraisal / retention decision)
 *   0203 — Aussonderung (disposal / transfer to archive)
 *   0401 — Abgabe an Archiv (transfer to federal archive)
 *   0501 — Anforderung (retrieval request)
 *   0601 — Erfassung (ingestion / registration)
 *
 * Implementations shipped in this header-only file:
 *   - XDOMEAVersion               — version identifier
 *   - XDOMEAObjectType            — XDOMEA object classification
 *   - XDOMEARetentionCategory     — retention / archival classification
 *   - XDOMEADocument              — in-memory XDOMEA document record
 *   - XDOMEAImportResult          — result of an import operation
 *   - XDOMEAExportResult          — result of an export operation
 *   - IXDOMEAConnector            — abstract connector interface
 *   - InMemoryXDOMEAConnector     — thread-safe in-memory implementation
 *
 * Standards references:
 *   - XDOMEA 3.0.0 — KoSIT, https://www.xdomea.de
 *   - XDOMEA 2.1.0 — BSI / KoSIT, 2018
 *   - DOMEA®-Konzept 2.0 — Bundesministerium des Innern, 2012
 *   - BSI TR-03138 — RESISCAN — Ersetzendes Scannen
 *   - ISO 15489-1:2016 — Information and documentation – Records management
 *   - DIN 31644:2012-04 — Langzeitarchivierung digitaler Dokumente
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace document {

// ── XDOMEAVersion ─────────────────────────────────────────────────────────────

/**
 * @brief XDOMEA schema version used to encode / decode a message.
 */
enum class XDOMEAVersion {
    V2_1,   ///< XDOMEA 2.1.0 (legacy)
    V3_0,   ///< XDOMEA 3.0.0 (current)
};

// ── XDOMEAObjectType ──────────────────────────────────────────────────────────

/**
 * @brief Classification of the XDOMEA object type.
 */
enum class XDOMEAObjectType {
    AKTE,           ///< Akte (dossier / folder)
    VORGANG,        ///< Vorgang (business process / sub-folder)
    DOKUMENT,       ///< Dokument (document with one or more electronic files)
    DATEI,          ///< Datei (electronic file — binary attachment)
    BENUTZER,       ///< Benutzer (user / principal record)
    SCHRIFTGUT,     ///< Schriftgut-Objekt (generic records object)
};

// ── XDOMEARetentionCategory ───────────────────────────────────────────────────

/**
 * @brief Retention / archival decision for a XDOMEA object.
 *
 * Corresponds to the Bewertungsergebnis element in XDOMEA 3.0.
 */
enum class XDOMEARetentionCategory {
    ARCHIVWUERDIG,      ///< Archivwürdig — to be permanently retained
    NICHT_ARCHIVWUERDIG,///< Nicht archivwürdig — to be destroyed after retention period
    UNGEKLAERT,         ///< Ungeklärt — retention status not yet determined
};

// ── XDOMEAMessageType ─────────────────────────────────────────────────────────

/**
 * @brief XDOMEA message type (Nachrichtentyp) code.
 */
enum class XDOMEAMessageType {
    ANBIETUNG,          ///< 0201 — Offer for archival
    BEWERTUNG,          ///< 0202 — Appraisal / retention decision
    AUSSONDERUNG,       ///< 0203 — Disposal / transfer to archive
    ABGABE_AN_ARCHIV,   ///< 0401 — Transfer to federal / state archive
    ANFORDERUNG,        ///< 0501 — Retrieval request
    ERFASSUNG,          ///< 0601 — Ingestion / registration in DMS
};

// ── XDOMEADocument ────────────────────────────────────────────────────────────

/**
 * @brief In-memory representation of a single XDOMEA object.
 *
 * Models both Akten/Vorgänge (structural objects) and Dokumente/Dateien
 * (content-bearing objects) with a common structure.  The @p object_type
 * field distinguishes them.
 */
struct XDOMEADocument {
    // ── Identity ──────────────────────────────────────────────────────────────
    std::string id;                     ///< Unique XDOMEA object identifier (UUID-style)
    XDOMEAObjectType object_type{XDOMEAObjectType::DOKUMENT};
    std::string aktenzeichen;           ///< File number / reference code
    std::string betreff;                ///< Subject / title of the document or dossier
    std::string xdomea_version;         ///< "2.1.0" or "3.0.0"

    // ── Hierarchy ─────────────────────────────────────────────────────────────
    std::optional<std::string> parent_id; ///< ID of the parent Akte or Vorgang
    std::vector<std::string> child_ids;   ///< IDs of child objects

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    std::string created_at;             ///< ISO 8601 creation timestamp
    std::string modified_at;            ///< ISO 8601 last-modification timestamp
    std::string closed_at;              ///< ISO 8601 closing / completion timestamp
    XDOMEARetentionCategory retention{XDOMEARetentionCategory::UNGEKLAERT};
    std::optional<std::string> retention_end; ///< ISO 8601 end of retention period

    // ── Authoring / provenance ────────────────────────────────────────────────
    std::string author;                 ///< Author (Ersteller)
    std::string responsible_unit;       ///< Responsible organisational unit
    std::string source_authority;       ///< Sending authority (ABS) identifier

    // ── Content (for Dokument / Datei) ────────────────────────────────────────
    std::string mime_type;              ///< MIME type of the primary file
    std::string file_name;             ///< Original file name
    std::string content_hash_sha256;   ///< SHA-256 hex-digest of the file content
    std::string content_base64;        ///< Base64-encoded file content (may be empty for stubs)

    // ── Classification ────────────────────────────────────────────────────────
    std::string classification;        ///< Vertraulichkeitsstufe (VS-NfD, etc.)
    std::vector<std::string> keywords; ///< Free-text keywords / tags

    // ── Extended metadata ────────────────────────────────────────────────────
    std::map<std::string, std::string> metadata; ///< Arbitrary KV metadata
};

// ── XDOMEAImportResult ────────────────────────────────────────────────────────

/**
 * @brief Aggregated result of an XDOMEA import operation.
 */
struct XDOMEAImportResult {
    bool success{false};
    XDOMEAVersion version{XDOMEAVersion::V3_0};
    XDOMEAMessageType message_type{XDOMEAMessageType::ERFASSUNG};
    std::size_t documents_parsed{0};
    std::size_t documents_imported{0};
    std::vector<std::string> errors;
    std::vector<XDOMEADocument> documents;
};

// ── XDOMEAExportResult ────────────────────────────────────────────────────────

/**
 * @brief Aggregated result of an XDOMEA export operation.
 */
struct XDOMEAExportResult {
    bool success{false};
    std::size_t documents_exported{0};
    std::string xml_output;
    std::string error_message;
};

// ── IXDOMEAConnector ──────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for XDOMEA document management connectivity.
 *
 * Implementations MUST be thread-safe.
 */
class IXDOMEAConnector {
public:
    virtual ~IXDOMEAConnector() = default;

    /**
     * @brief Import documents from an XDOMEA XML string.
     *
     * @param xml_content  Raw UTF-8 encoded XDOMEA XML message.
     * @param version      Expected XDOMEA schema version.
     * @return             Import result with all parsed documents and errors.
     */
    virtual XDOMEAImportResult importFromXML(std::string_view xml_content,
                                             XDOMEAVersion version) = 0;

    /**
     * @brief Export a collection of documents to an XDOMEA XML string.
     *
     * @param documents    Documents to serialise.
     * @param version      Target XDOMEA schema version.
     * @param message_type XDOMEA message type for the XML envelope.
     * @return             Export result containing the serialised XML or an error.
     */
    virtual XDOMEAExportResult exportToXML(
        const std::vector<XDOMEADocument>& documents,
        XDOMEAVersion version,
        XDOMEAMessageType message_type) = 0;

    /**
     * @brief Store a document in the connector's repository.
     *
     * @throws std::invalid_argument  if doc.id is empty.
     * @throws std::runtime_error     if a document with the same ID is already
     *                                stored.
     */
    virtual void storeDocument(const XDOMEADocument& doc) = 0;

    /**
     * @brief Retrieve a document by ID.
     *
     * @return The document, or std::nullopt if not found.
     */
    virtual std::optional<XDOMEADocument> getDocument(std::string_view id) const = 0;

    /**
     * @brief List documents of a specific object type.
     */
    virtual std::vector<XDOMEADocument>
    listByType(XDOMEAObjectType type) const = 0;

    /**
     * @brief List documents with a given retention category.
     */
    virtual std::vector<XDOMEADocument>
    listByRetention(XDOMEARetentionCategory retention) const = 0;

    /**
     * @brief List child documents of the given parent ID.
     */
    virtual std::vector<XDOMEADocument>
    listChildren(std::string_view parent_id) const = 0;

    /**
     * @brief Remove a document from the repository.  No-op if not found.
     */
    virtual void removeDocument(std::string_view id) = 0;

    /**
     * @brief Return the total number of stored documents.
     */
    virtual std::size_t count() const = 0;
};

// ── InMemoryXDOMEAConnector ───────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of IXDOMEAConnector.
 *
 * Parses XDOMEA XML with a minimal scanner (no external XML library
 * dependency).  Production deployments SHOULD use a libxml2- or
 * pugixml-backed implementation with full XSD validation.
 */
class InMemoryXDOMEAConnector : public IXDOMEAConnector {
public:
    // ── IXDOMEAConnector ──────────────────────────────────────────────────────

    XDOMEAImportResult importFromXML(std::string_view xml_content,
                                     XDOMEAVersion version) override {
        XDOMEAImportResult result;
        result.version = version;

        if (xml_content.empty()) {
            result.errors.push_back("XML content is empty");
            return result;
        }

        const std::string xml(xml_content);
        std::size_t pos = 0;

        const auto findTagStart = [&xml](const std::string& tag, std::size_t from) -> std::size_t {
            std::size_t cursor = from;
            while (true) {
                auto hit = xml.find(tag, cursor);
                if (hit == std::string::npos) {
                  return std::string::npos;
                }
                const std::size_t next = hit + tag.size();
                if (next >= xml.size() || xml[next] == '>' || xml[next] == ' ') {
                    return hit;
                }
                cursor = hit + 1;
            }
        };

        while (true) {
            auto ds = findTagStart("<dokument", pos);
            if (ds == std::string::npos) {
              break;
            }
            auto de = xml.find("</dokument>", ds);
            if (de == std::string::npos) {
                result.errors.push_back("Unclosed <dokument> element");
                break;
            }

            XDOMEADocument doc;
            doc.xdomea_version = (version == XDOMEAVersion::V3_0) ? "3.0.0" : "2.1.0";
            doc.object_type    = XDOMEAObjectType::DOKUMENT;
            const std::string fragment = xml.substr(ds, de - ds + 11);
            extractField_(fragment, "id",          doc.id);
            extractField_(fragment, "aktenzeichen",doc.aktenzeichen);
            extractField_(fragment, "betreff",     doc.betreff);
            extractField_(fragment, "ersteller",   doc.author);
            extractField_(fragment, "dateiname",   doc.file_name);
            extractField_(fragment, "mimetyp",     doc.mime_type);
            if (doc.id.empty()) {
                doc.id = "xdomea-" + std::to_string(result.documents_parsed);
            }

            result.documents.push_back(doc);
            ++result.documents_parsed;
            ++result.documents_imported;
            pos = de + 11;
        }

        // Also scan for <akte> elements.
        pos = 0;
        while (true) {
            auto as = findTagStart("<akte", pos);
            if (as == std::string::npos) {
              break;
            }
            auto ae = xml.find("</akte>", as);
            if (ae == std::string::npos) {
                result.errors.push_back("Unclosed <akte> element");
                break;
            }
            XDOMEADocument doc;
            doc.xdomea_version = (version == XDOMEAVersion::V3_0) ? "3.0.0" : "2.1.0";
            doc.object_type    = XDOMEAObjectType::AKTE;
            const std::string fragment = xml.substr(as, ae - as + 7);
            extractField_(fragment, "id",          doc.id);
            extractField_(fragment, "aktenzeichen",doc.aktenzeichen);
            extractField_(fragment, "betreff",     doc.betreff);
            if (doc.id.empty()) {
                doc.id = "akte-" + std::to_string(result.documents_parsed);
            }
            result.documents.push_back(doc);
            ++result.documents_parsed;
            ++result.documents_imported;
            pos = ae + 7;
        }

        // Persist.
        {
            std::unique_lock<std::mutex> lk(mutex_);
            for (const auto& d : result.documents) {
              store_[d.id] = d;
            }
        }

        result.success = result.errors.empty();
        return result;
    }

    XDOMEAExportResult exportToXML(const std::vector<XDOMEADocument>& documents,
                                   XDOMEAVersion version,
                                   XDOMEAMessageType message_type) override {
        XDOMEAExportResult result;
        const std::string ver = (version == XDOMEAVersion::V3_0) ? "3.0.0" : "2.1.0";
        const std::string ns  = "https://www.xdomea.de/ns/xdomea/";

        std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        xml += "<xdomea:Nachricht xmlns:xdomea=\"" + ns + "\"\n";
        xml += "  xdomea:version=\"" + ver + "\"\n";
        xml += "  xdomea:nachrichtentyp=\"" + messageTypeCode_(message_type) + "\">\n";

        for (const auto& doc : documents) {
            const std::string tag = objectTypeTag_(doc.object_type);
            xml += "  <" + tag + ">\n";
            xml += "    <id>"           + escapeXML_(doc.id)           + "</id>\n";
            xml += "    <aktenzeichen>" + escapeXML_(doc.aktenzeichen) + "</aktenzeichen>\n";
            xml += "    <betreff>"      + escapeXML_(doc.betreff)      + "</betreff>\n";
            if (!doc.author.empty())
                xml += "    <ersteller>"   + escapeXML_(doc.author)       + "</ersteller>\n";
            if (!doc.file_name.empty())
                xml += "    <dateiname>"   + escapeXML_(doc.file_name)    + "</dateiname>\n";
            if (!doc.mime_type.empty())
                xml += "    <mimetyp>"     + escapeXML_(doc.mime_type)    + "</mimetyp>\n";
            xml += "  </" + tag + ">\n";
            ++result.documents_exported;
        }
        xml += "</xdomea:Nachricht>\n";

        result.xml_output = std::move(xml);
        result.success    = true;
        return result;
    }

    void storeDocument(const XDOMEADocument& doc) override {
        if (doc.id.empty()) {
            throw std::invalid_argument("XDOMEADocument::id must not be empty");
        }
        std::unique_lock<std::mutex> lk(mutex_);
        if (store_.count(doc.id)) {
            throw std::runtime_error("Document already stored: " + doc.id);
        }
        store_[doc.id] = doc;
    }

    std::optional<XDOMEADocument> getDocument(std::string_view id) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        auto it = store_.find(std::string(id));
        if (it == store_.end()) {
          return std::nullopt;
        }
        return it->second;
    }

    std::vector<XDOMEADocument> listByType(XDOMEAObjectType type) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<XDOMEADocument> result = {};

        for (const auto& [id, d] : store_) {
            if (d.object_type == type) {
              result.push_back(d);
            }
        }
        return result;
    }

    std::vector<XDOMEADocument>
    listByRetention(XDOMEARetentionCategory retention) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<XDOMEADocument> result = {};

        for (const auto& [id, d] : store_) {
            if (d.retention == retention) {
              result.push_back(d);
            }
        }
        return result;
    }

    std::vector<XDOMEADocument>
    listChildren(std::string_view parent_id) const override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<XDOMEADocument> result;
        const std::string pid(parent_id);
        for (const auto& [id, d] : store_) {
            if (d.parent_id && *d.parent_id == pid) {
              result.push_back(d);
            }
        }
        return result;
    }

    void removeDocument(std::string_view id) override {
        std::unique_lock<std::mutex> lk(mutex_);
        store_.erase(std::string(id));
    }

    std::size_t count() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return store_.size();
    }

private:
    // ── Helpers ───────────────────────────────────────────────────────────────

    static void extractField_(const std::string& fragment,
                               const std::string& tag,
                               std::string& out) {
        const std::string open  = "<" + tag + ">";
        const std::string close = "</" + tag + ">";
        auto s = fragment.find(open);
        if (s == std::string::npos) {
          return;
        }
        auto e = fragment.find(close, s + open.size());
        if (e == std::string::npos) {
          return;
        }
        out = fragment.substr(s + open.size(), e - s - open.size());
    }

    static std::string escapeXML_(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '&':  out += "&amp;";  break;
                case '<':  out += "&lt;";   break;
                case '>':  out += "&gt;";   break;
                case '"':  out += "&quot;"; break;
                case '\'': out += "&apos;"; break;
                default:   out += c;        break;
            }
        }
        return out;
    }

    static std::string objectTypeTag_(XDOMEAObjectType t) {
        switch (t) {
            case XDOMEAObjectType::AKTE:      return "akte";
            case XDOMEAObjectType::VORGANG:   return "vorgang";
            case XDOMEAObjectType::DOKUMENT:  return "dokument";
            case XDOMEAObjectType::DATEI:     return "datei";
            case XDOMEAObjectType::BENUTZER:  return "benutzer";
            case XDOMEAObjectType::SCHRIFTGUT:return "schriftgut";
            default:                          return "objekt";
        }
    }

    static std::string messageTypeCode_(XDOMEAMessageType t) {
        switch (t) {
            case XDOMEAMessageType::ANBIETUNG:      return "0201";
            case XDOMEAMessageType::BEWERTUNG:      return "0202";
            case XDOMEAMessageType::AUSSONDERUNG:   return "0203";
            case XDOMEAMessageType::ABGABE_AN_ARCHIV: return "0401";
            case XDOMEAMessageType::ANFORDERUNG:    return "0501";
            case XDOMEAMessageType::ERFASSUNG:      return "0601";
            default:                                return "0000";
        }
    }

    mutable std::mutex mutex_;
    std::map<std::string, XDOMEADocument> store_;
};

} // namespace document
} // namespace themis

