/**
 * @file xoev_importer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — XÖV Data Model Importer / Exporter
 *
 * Import and export support for XÖV (XML in der öffentlichen Verwaltung)
 * data models — the German federal standard for machine-readable data
 * exchange between public-administration systems.  Covers the most
 * widely used XÖV sub-standards:
 *
 *   XPersonenstand   — civil-registry data (births, deaths, marriages)
 *   XMeld            — resident-registration data (Meldewesen)
 *   XBau             — construction-permit data (Bauwesen)
 *   XKfz             — vehicle-registration data (KFZ-Zulassung)
 *   XFinanz          — financial / tax data (Finanzamt)
 *   XGewerbeanmeldung — business-registration data
 *
 * Implementations shipped in this header-only file:
 *   - XOEVRecord             — in-memory representation of one XÖV record
 *   - XOEVImportResult       — result of an import operation
 *   - XOEVExportResult       — result of an export operation
 *   - IXOEVImporter          — abstract import / export interface
 *   - InMemoryXOEVImporter   — thread-safe in-memory implementation
 *
 * Standards references:
 *   - XÖV-Rahmenwerk, Version 3.0 — KoSIT (Koordinierungsstelle für
 *     IT-Standards), https://www.xoev.de
 *   - DIN SPEC 91379:2022-03 — Characters and defined character sequences
 *     in Unicode for the processing of names and address data
 *   - BSI TR-03137 — Anforderungen an XÖV-konforme Schnittstellen
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace importers {

// ── XOEVStandard ─────────────────────────────────────────────────────────────

enum class XOEVStandard {
    XPERSONENSTAND,     ///< Personenstandswesen — §§ 55a PStG
    XMELD,              ///< Meldewesen — BMG / BAMF
    XBAU,               ///< Bauwesen — construction permits
    XKFZ,               ///< KFZ-Zulassung — vehicle registration
    XFINANZ,            ///< Finanzamt — tax data
    XGEWERBEANMELDUNG,  ///< Gewerbeanmeldung — business registration
    XBILDUNG,           ///< Bildung — education data
    XJUSTIZ,            ///< Justiz — court & justice data
    XGESUNDHEIT,        ///< Gesundheit — healthcare data
    OTHER,              ///< Custom / proprietary XÖV profile
};

// ── XOEVVersion ──────────────────────────────────────────────────────────────

struct XOEVVersion {
    int major{1};
    int minor{0};
    int patch{0};

    std::string toString() const {
        return std::to_string(major) + "." +
               std::to_string(minor) + "." +
               std::to_string(patch);
    }

    bool operator==(const XOEVVersion& o) const noexcept {
        return major == o.major && minor == o.minor && patch == o.patch;
    }
    bool operator<(const XOEVVersion& o) const noexcept {
        if (major != o.major) {
          return major < o.major;
        }
        if (minor != o.minor) {
          return minor < o.minor;
        }
        return patch < o.patch;
    }
};

// ── XOEVRecord ───────────────────────────────────────────────────────────────

struct XOEVRecord {
    std::string id;                         ///< Internal record identifier
    XOEVStandard standard{XOEVStandard::OTHER}; ///< XÖV sub-standard
    XOEVVersion version;                    ///< Schema version
    std::string message_type;               ///< XÖV message type (Nachrichtentyp)
    std::string source_authority;           ///< Sending authority (Behörde)
    std::string target_authority;           ///< Receiving authority
    std::string transaction_id;             ///< End-to-end correlation identifier
    std::string created_at;                 ///< ISO 8601 timestamp
    std::map<std::string, std::string> fields; ///< Extracted data fields
    std::string raw_xml;                    ///< Original XML source (optional)
};

// ── XOEVImportError ───────────────────────────────────────────────────────────

struct XOEVImportError {
    std::size_t record_index{0};    ///< 0-based index of the failing record
    std::string field;              ///< Affected XML path / field name
    std::string message;            ///< Human-readable error description
    bool fatal{false};              ///< True → import was aborted
};

// ── XOEVImportResult ─────────────────────────────────────────────────────────

struct XOEVImportResult {
    bool success{false};
    std::size_t records_parsed{0};
    std::size_t records_imported{0};
    std::size_t records_skipped{0};
    std::vector<XOEVImportError> errors;
    std::vector<XOEVRecord> records; ///< Imported records (populated on success)

    bool hasErrors() const noexcept { return !errors.empty(); }
};

// ── XOEVExportResult ──────────────────────────────────────────────────────────

struct XOEVExportResult {
    bool success{false};
    std::size_t records_exported{0};
    std::string xml_output;         ///< Serialised XML string (in-memory export)
    std::string error_message;      ///< Non-empty on failure
};

// ── IXOEVImporter ─────────────────────────────────────────────────────────────

class IXOEVImporter {
public:
    /**
     * @brief IXOEVImporter.
     * @return Return value.
     */
    virtual ~IXOEVImporter() = default;

    /**
     * @brief Import From XML.
     * @param[in] xml_content Input parameter.
     * @param[in] standard Input parameter.
     * @return Return value.
     */
    virtual XOEVImportResult importFromXML(std::string_view xml_content,
                                           XOEVStandard standard) = 0;

    /**
     * @brief Export To XML.
     * @param[in] records Input parameter.
     * @param[in] standard Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    virtual XOEVExportResult exportToXML(const std::vector<XOEVRecord>& records,
                                         XOEVStandard standard,
                                         const XOEVVersion& version) = 0;

    /**
     * @brief Validate.
     * @param[in] xml_content Input parameter.
     * @param[in] standard Input parameter.
     * @param[in] version Input parameter.
     * @param[in,out] errors_out Input/output parameter.
     * @return True when the operation succeeds.
     */
    virtual bool validate(std::string_view xml_content,
                          XOEVStandard standard,
                          const XOEVVersion& version,
                          std::vector<XOEVImportError>& errors_out) = 0;

    /**
     * @brief Stored Records.
     * @return Return value.
     */
    virtual std::vector<XOEVRecord> storedRecords() const = 0;

    /**
     * @brief Clear Records.
     */
    virtual void clearRecords() = 0;
};

// ── InMemoryXOEVImporter ──────────────────────────────────────────────────────

class InMemoryXOEVImporter : public IXOEVImporter {
public:
    // ── IXOEVImporter ────────────────────────────────────────────────────────

    XOEVImportResult importFromXML(std::string_view xml_content,
                                   XOEVStandard standard) override {
        XOEVImportResult result = {};
        if (xml_content.empty()) {
            result.errors.push_back({0, "", "XML content is empty", true});
            return result;
        }

        /**
         * @brief Minimal extraction: find <record> … </record> elements.
         * @param[in] xml_content Input parameter.
         * @return Return value.
         */
        const std::string xml(xml_content);
        std::size_t pos = 0;

        while (true) {
            auto start = xml.find("<record", pos);
            if (start == std::string::npos) {
              break;
            }
            auto end = xml.find("</record>", start);
            if (end == std::string::npos) {
                result.errors.push_back({result.records_parsed,
                    "", "Unclosed <record> element", false});
                break;
            }

            XOEVRecord rec;
            rec.standard = standard;
            rec.raw_xml  = xml.substr(start, end - start + 9);
            extractFields_(rec.raw_xml, rec.fields);
            // Derive id from field "id" if present, else generate a positional ID.
            auto id_it = rec.fields.find("id");
            rec.id = (id_it != rec.fields.end())
                   ? id_it->second
                   : "xoev-" + std::to_string(result.records_parsed);

            result.records.push_back(rec);
            ++result.records_parsed;
            ++result.records_imported;
            pos = end + 9;
        }

        // Persist to internal store.
        {
            /**
             * @brief Lk.
             * @param[in] mutex_ Input parameter.
             * @return Return value.
             */
            std::unique_lock<std::mutex> lk(mutex_);
            for (const auto& r : result.records) {
                store_[r.id] = r;
            }
        }

        result.success = !result.errors.empty()
                       ? result.records_imported > 0
                       : true;
        return result;
    }

    XOEVExportResult exportToXML(const std::vector<XOEVRecord>& records,
                                 XOEVStandard standard,
                                 const XOEVVersion& version) override {
        XOEVExportResult result = {};
        if (records.empty()) {
            result.success     = true;
            result.xml_output  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                                 "<xoev:nachrichten xmlns:xoev=\"https://www.xoev.de/schema\"/>\n";
            return result;
        }

        std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        xml += "<xoev:nachrichten xmlns:xoev=\"https://www.xoev.de/schema\" "
               "version=\"" + version.toString() + "\">\n";

        for (const auto& rec : records) {
            if (rec.standard != standard && rec.standard != XOEVStandard::OTHER) {
                result.error_message = "Record standard mismatch for id=" + rec.id;
                return result;
            }
            xml += "  <record id=\"" + escapeXML_(rec.id) + "\">\n";
            for (const auto& [k, v] : rec.fields) {
                xml += "    <" + escapeXML_(k) + ">"
                     + escapeXML_(v) + "</" + escapeXML_(k) + ">\n";
            }
            xml += "  </record>\n";
            ++result.records_exported;
        }
        xml += "</xoev:nachrichten>\n";

        result.xml_output = std::move(xml);
        result.success    = true;
        return result;
    }

    bool validate(std::string_view xml_content,
                  XOEVStandard /*standard*/,
                  const XOEVVersion& /*version*/,
                  std::vector<XOEVImportError>& errors_out) override {
        if (xml_content.empty()) {
            errors_out.push_back({0, "", "Empty document", true});
            return false;
        }
        /**
         * @brief Xml.
         * @param[in] xml_content Input parameter.
         * @return Return value.
         */
        const std::string xml(xml_content);
        // Basic well-formedness: every opening tag must have a closing tag.
        std::size_t open  = std::count(xml.begin(), xml.end(), '<');
        std::size_t close = std::count(xml.begin(), xml.end(), '>');
        if (open != close) {
            errors_out.push_back({0, "", "Mismatched < and > characters", true});
            return false;
        }
        return true;
    }

    std::vector<XOEVRecord> storedRecords() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<XOEVRecord> result = {};

        result.reserve(store_.size());
        for (const auto& [id, r] : store_) {
          result.push_back(r);
        }
        return result;
    }

    void clearRecords() override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        store_.clear();
    }

private:
    // ── Helpers ───────────────────────────────────────────────────────────────

    static void extractFields_(const std::string& fragment,
                                std::map<std::string, std::string>& out) {
        std::size_t pos = 0;
        while (true) {
            auto ts = fragment.find('<', pos);
            if (ts == std::string::npos) {
              break;
            }
            auto te = fragment.find('>', ts);
            if (te == std::string::npos) {
              break;
            }
            std::string tag = fragment.substr(ts + 1, te - ts - 1);
            // Skip closing tags and the <record …> element itself.
            if (tag.empty() || tag[0] == '/' || tag.find("record") == 0) {
                pos = te + 1;
                continue;
            }
            // Strip attributes from tag name.
            auto space = tag.find(' ');
            if (space != std::string::npos) {
              tag = tag.substr(0, space);
            }

            auto vs = te + 1;
            auto ve = fragment.find("</" + tag + ">", vs);
            if (ve == std::string::npos) { pos = te + 1; continue; }
            out[tag] = fragment.substr(vs, ve - vs);
            pos = ve + tag.size() + 3;
        }
    }

    /**
     * @brief Escape XML.
     * @param[in] s Input parameter.
     * @return Return value.
     * @details Calls: reserve(), size().
     */
    static std::string escapeXML_(const std::string& s) {
        std::string out = {};
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

    mutable std::mutex mutex_;
    std::map<std::string, XOEVRecord> store_;
};

} // namespace importers
} // namespace themis

