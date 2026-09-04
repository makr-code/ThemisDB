/*
 * ThemisDB — Datengetriebene E-Government-Tests
 *
 * Dieser Test lädt generische Behörden, Anträge, Prozessdefinitionen und
 * Kontrollergebnisse aus JSON-Fixture-Dateien und führt die Assertions
 * daraus ab.  Er ist VOLLSTÄNDIG OHNE CI ausführbar — nur GTest und
 * nlohmann/json werden benötigt.
 *
 * Fixture-Pfad:
 *   Die Fixtures liegen unter tests/fixtures/egov/.
 *   Der Pfad wird zur Kompilierzeit als THEMIS_EGOV_FIXTURES_DIR eingebettet.
 *   Alternativ kann die Umgebungsvariable THEMIS_EGOV_FIXTURES_DIR überschreiben.
 *
 * Ausführung lokal (Beispiel):
 *   cd <build-dir>
 *   cmake --build . --target test_egov_data_driven_focused
 *   ./test_egov_data_driven_focused
 *   # oder mit eigenem Fixture-Pfad:
 *   THEMIS_EGOV_FIXTURES_DIR=/pfad/zu/fixtures ./test_egov_data_driven_focused
 *
 * Testgruppen / CMake-Ziel:
 *   EGovDataDrivenFocusedTests
 *
 * Fixtures (alle in tests/fixtures/egov/):
 *   behoerden.json                          — 11 generische Behörden (JSON)
 *   antraege/baugenehmigung.md              — Baugenehmigungsantrag (Markdown + JSON-Front-Matter)
 *   antraege/bimschg.md                     — BImSchG-Antrag        (Markdown + JSON-Front-Matter)
 *   prozesse/baugenehmigung_prozess.json    — Prozessphasen Baugenehmigung (JSON)
 *   prozesse/bimschg_prozess.json           — Prozessphasen BImSchV (JSON)
 *   expected/baugenehmigung_expected.json   — Kontrollergebnisse Baugenehmigung (JSON)
 *   expected/bimschg_expected.json          — Kontrollergebnisse BImSchV (JSON)
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "auth/eid_authenticator.h"
#include "document/xdomea_connector.h"
#include "importers/ozg_service_registry.h"
#include "importers/xoev_importer.h"
#include "analytics/llm_process_analyzer.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace themis::auth;
using namespace themis::document;
using namespace themis::importers;
using namespace themis;

// ── Fixture-Pfad-Auflösung ────────────────────────────────────────────────────

namespace {

/**
 * @brief Gibt den Pfad zum Fixture-Verzeichnis zurück.
 *
 * Reihenfolge:
 *  1. Umgebungsvariable THEMIS_EGOV_FIXTURES_DIR
 *  2. Zur Kompilierzeit eingebetteter Pfad THEMIS_EGOV_FIXTURES_DIR_COMPILETIME
 */
static std::string fixturesDir() {
    if (const char* env = std::getenv("THEMIS_EGOV_FIXTURES_DIR")) {
        return std::string(env);
    }
#ifdef THEMIS_EGOV_FIXTURES_DIR_COMPILETIME
    return THEMIS_EGOV_FIXTURES_DIR_COMPILETIME;
#else
    return "./fixtures/egov";
#endif
}

/**
 * @brief Extrahiert das JSON-Front-Matter aus einer Markdown-Datei.
 *
 * Erwartet folgendes Format:
 * @code
 *   ---
 *   { ... JSON-Objekt ... }
 *   ---
 *   # Markdown-Titel
 *   ...
 * @endcode
 *
 * Das JSON-Front-Matter wird geparst und zurückgegeben. Der restliche
 * Markdown-Prosatext wird als String im Feld "_dokument_text" gespeichert.
 *
 * @throws std::runtime_error wenn das Front-Matter fehlt oder ungültiges JSON enthält.
 */
static json parseFrontMatter(const std::string& content, const std::string& path_hint) {
    // Front matter muss mit "---\n" beginnen
    const std::string fence = "---";
    if (content.substr(0, 3) != fence) {
        throw std::runtime_error("Markdown-Fixture '" + path_hint +
                                 "' hat kein Front-Matter (erwartet '---' am Anfang)");
    }

    // Abschnitt zwischen erstem und zweitem "---" finden
    size_t first_end = content.find('\n', 0);          // Ende der Zeile "---"
    if (first_end == std::string::npos) {
      first_end = 3;
    }
    else first_end += 1;                               // hinter das \n

    size_t second_start = content.find("\n---", first_end);
    if (second_start == std::string::npos) {
        throw std::runtime_error("Markdown-Fixture '" + path_hint +
                                 "': schließendes '---' für Front-Matter nicht gefunden");
    }

    std::string fm_text = content.substr(first_end, second_start - first_end);

    // Prosatext nach dem zweiten "---"
    size_t prose_start = second_start + 4; // hinter "\n---"
    if (prose_start < content.size() && content[prose_start] == '\n') {
      prose_start++;
    }
    std::string prose = (prose_start < content.size()) ? content.substr(prose_start) : "";

    json j;
    try {
        j = json::parse(fm_text);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("JSON-Parse-Fehler im Front-Matter von '" + path_hint +
                                 "': " + e.what());
    }
    j["_dokument_text"] = std::move(prose);
    return j;
}

/**
 * @brief Lädt eine Fixture-Datei aus dem Fixture-Verzeichnis.
 *
 * Unterstützt:
 *  - `.md`   — Markdown mit JSON-Front-Matter (Pflichtformat für Anträge)
 *  - `.json` — Reines JSON (für Behörden, Prozesse, Kontrollergebnisse)
 *
 * @throws std::runtime_error wenn Datei nicht gefunden oder ungültig.
 */
static json loadFixture(const std::string& relpath) {
    std::filesystem::path p = std::filesystem::path(fixturesDir()) / relpath;
    std::ifstream f(p);
    if (!f.is_open()) {
        throw std::runtime_error("Fixture nicht gefunden: " + p.string() +
                                 "\n  Setze THEMIS_EGOV_FIXTURES_DIR auf den Pfad zu tests/fixtures/egov/");
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    const std::string ext = p.extension().string();
    if (ext == ".md") {
        return parseFrontMatter(content, p.string());
    }

    json j;
    try {
        j = json::parse(content);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("JSON-Parse-Fehler in " + p.string() + ": " + e.what());
    }
    return j;
}

// ── Hilfsfunktionen ───────────────────────────────────────────────────────────

static std::string nowIso() { return "2026-06-01T10:00:00Z"; }

static XDOMEADocument makeDoc(const std::string& id,
                               XDOMEAObjectType   type,
                               const std::string& az,
                               const std::string& betreff,
                               const std::string& author,
                               const std::string& unit,
                               std::map<std::string,std::string> meta = {}) {
    XDOMEADocument d;
    d.id               = id;
    d.object_type      = type;
    d.aktenzeichen     = az;
    d.betreff          = betreff;
    d.author           = author;
    d.responsible_unit = unit;
    d.source_authority = author;
    d.xdomea_version   = "3.0.0";
    d.mime_type        = "application/pdf";
    d.created_at       = nowIso();
    d.retention        = XDOMEARetentionCategory::ARCHIVWUERDIG;
    d.metadata         = std::move(meta);
    return d;
}

/**
 * @brief Erzeugt einen minimalen XÖV-Antrags-XML-String aus Fixture-Daten.
 */
static std::string buildXOEVXml(const std::string& az,
                                  const std::string& wurzel,
                                  const json& felder) {
    std::ostringstream ss = {};
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
       << "<" << wurzel << ">"
       << "<record>"
       << "<id>" << az << "</id>"
       << "<standard>" << wurzel << "</standard>";
    for (auto& [k, v] : felder.items()) {
        ss << "<" << k << ">" << v.get<std::string>() << "</" << k << ">";
    }
    ss << "</record>"
       << "</" << wurzel << ">";
    return ss.str();
}

} // anonymous namespace

// ── EGovFixtureContext ────────────────────────────────────────────────────────

/**
 * @brief Zentraler Kontext eines datengetriebenen E-Gov-Tests.
 *
 * Lädt alle Fixtures und hält In-Memory-Implementierungen für alle
 * beteiligten Behörden.
 */
struct EGovFixtureContext {
    // ── Geladene Fixtures ────────────────────────────────────────────────────
    json behoerden;
    json antrag;
    json prozess;
    json expected;

    // ── Aktenzeichen ─────────────────────────────────────────────────────────
    std::string az;

    // ── DMS-Stores (je Behörde) ───────────────────────────────────────────────
    std::map<std::string, std::unique_ptr<InMemoryXDOMEAConnector>> dms;

    // ── Weitere In-Memory-Services ────────────────────────────────────────────
    InMemoryXOEVImporter        xoev;
    InMemoryOZGServiceRegistry  ozg;
    InMemoryEIDAuthenticator    eid;
    std::unique_ptr<LLMProcessAnalyzer> llm;

    // ── Prozessprotokoll ─────────────────────────────────────────────────────
    std::vector<std::string> log;
    std::mutex               log_mu;

    // ── Verfahrensstatus ─────────────────────────────────────────────────────
    std::string status;

    explicit EGovFixtureContext(const std::string& antrag_file,
                                const std::string& prozess_file,
                                const std::string& expected_file) {
        behoerden = loadFixture("behoerden.json");
        antrag    = loadFixture(antrag_file);
        prozess   = loadFixture(prozess_file);
        expected  = loadFixture(expected_file);

        az = antrag.value("aktenzeichen", "UNKNOWN-AZ");
        status = "ANTRAG_EINGEGANGEN";

        // DMS-Store für jede in behoerden.json definierte Behörde anlegen
        for (const auto& b : behoerden.at("behoerden")) {
            std::string bid = b.at("id").get<std::string>();
            dms[bid] = std::make_unique<InMemoryXDOMEAConnector>();
        }

        LLMConfig cfg;
        cfg.provider    = LLMProvider::LOCAL;
        cfg.model_name  = "themis-stub";
        cfg.temperature = 0.0;
        llm = std::make_unique<LLMProcessAnalyzer>(cfg);

        EIDAuthConfig eid_cfg;
        eid_cfg.enabled = true;
        eid_cfg.eid_server_url = "https://eid.local/auth";
        eid_cfg.terminal_certificate = "test-terminal-cert";
        if (!eid.initialize(eid_cfg)) {
            throw std::runtime_error("InMemoryEIDAuthenticator konnte nicht initialisiert werden");
        }
    }

    void logPhase(const std::string& phase) {
        std::lock_guard<std::mutex> lk(log_mu);
        log.push_back(phase);
    }

    InMemoryXDOMEAConnector& getDMS(const std::string& behoerde_id) {
        auto it = dms.find(behoerde_id);
        if (it == dms.end()) {
            throw std::runtime_error("Unbekannte Behörde: " + behoerde_id);
        }
        return *it->second;
    }

    /** Gibt die Behörde zurück, die in der Fixture als federführend markiert ist. */
    std::string federführend() const {
        return prozess.value("federführende_behoerde", "");
    }

    /** Liefert alle Fachbehörden-IDs aus der Prozess-Fixture. */
    std::vector<std::string> fachbehoerdenIds() const {
        std::vector<std::string> ids = {};

        for (const auto& fb : prozess.at("fachbehoerden")) {
            ids.push_back(fb.at("behoerde_id").get<std::string>());
        }
        return ids;
    }

    /** Gibt Phasenanzeiger (Präfix) zurück, der im Log gesucht wird. */
    std::string phasenPrefix(const std::string& phase_id) const {
        return phase_id + ":";
    }

    int logPos(const std::string& prefix) const {
        for (int i = 0; i < static_cast<int>(log.size()); ++i) {
            if (log[i].rfind(prefix, 0) == 0) {
              return i;
            }
        }
        return -1;
    }
};

// ── Assertion-Auswertungs-Engine ──────────────────────────────────────────────

/**
 * @brief Wertet eine einzelne Assertion aus der expected.json aus.
 *
 * Pro Assertion-Typ wird eine passende GTest-Assertion abgesetzt.
 * Das macht jede Zeile in der expected.json direkt testbar.
 */
class EGovAssertionRunner {
public:
    explicit EGovAssertionRunner(EGovFixtureContext& ctx) : ctx_(ctx) {}

    void run(const json& assertion) {
        const std::string typ = assertion.at("typ").get<std::string>();
        const std::string id  = assertion.at("id").get<std::string>();

        if (typ == "ozg_service_found") {
            runOzgServiceFound(id, assertion);
        } else if (typ == "eid_fullname") {
            runEidFullname(id, assertion);
        } else if (typ == "eid_attribut") {
            runEidAttribut(id, assertion);
        } else if (typ == "xoev_import_success") {
            runXoevImport(id, assertion);
        } else if (typ == "xoev_export_wohlgeformt") {
            runXoevExport(id, assertion);
        } else if (typ == "xdomea_objekte_min") {
            runXdomeaObjekteMin(id, assertion);
        } else if (typ == "xdomea_vorgaenge_min") {
            runXdomeaVorgaengeMin(id, assertion);
        } else if (typ == "xdomea_stellungnahmen_alle") {
            runXdomeaStellungnahmenAlle(id, assertion);
        } else if (typ == "xdomea_bescheid_vorhanden") {
            runXdomeaBescheidVorhanden(id, assertion);
        } else if (typ == "xdomea_aktenzeichen") {
            runXdomeaAktenzeichen(id, assertion);
        } else if (typ == "xdomea_akte_vorhanden") {
            runXdomeaAkteVorhanden(id, assertion);
        } else if (typ == "xdomea_export_wohlgeformt") {
            runXdomeaExport(id, assertion);
        } else if (typ == "xdomea_stores_min") {
            runXdomeaStoresMin(id, assertion);
        } else if (typ == "xdomea_kinder_min") {
            runXdomeaKinderMin(id, assertion);
        } else if (typ == "xdomea_metadaten_vorhanden") {
            runXdomeaMetadatenVorhanden(id, assertion);
        } else if (typ == "xdomea_protokoll_einwendungsreferenzen") {
            runEroeterungProtokoll(id, assertion);
        } else if (typ == "llm_score_min") {
            runLlmScoreMin(id, assertion);
        } else if (typ == "llm_summary_nichtleer") {
            runLlmSummaryNichtleer(id, assertion);
        } else if (typ == "verfahrensstatus") {
            runVerfahrensstatus(id, assertion);
        } else if (typ == "metadata_wert") {
            // Übersprungen — wird durch spezifischere Asserts abgedeckt
        } else if (typ == "prozess_reihenfolge") {
            runProzessReihenfolge(id, assertion);
        } else if (typ == "thread_safe_parallel_writes") {
            runThreadSafeParallelWrites(id, assertion);
        } else if (typ == "xdomea_zustellung") {
            runZustellung(id, assertion);
        } else {
            ADD_FAILURE() << id << ": Unbekannter Assertion-Typ: " << typ;
        }
    }

private:
    EGovFixtureContext& ctx_;

    // ── OZG ──────────────────────────────────────────────────────────────────

    void runOzgServiceFound(const std::string& id, const json& a) {
        const std::string service_id = a.at("service_id").get<std::string>();

        // Dienst registrieren (falls noch nicht geschehen)
        auto existing = ctx_.ozg.findById(service_id);
        if (!existing.has_value()) {
            OZGServiceEntry svc;
            svc.id     = service_id;
            svc.name   = service_id;
            svc.level  = OZGFederalLevel::STATE;
            svc.status = OZGServiceStatus::ONLINE_TRANSACTION;
            ASSERT_NO_THROW(ctx_.ozg.registerService(svc)) << id;
        }

        auto found = ctx_.ozg.findById(service_id);
        EXPECT_TRUE(found.has_value()) << id << ": OZG-Dienst " << service_id << " nicht gefunden";
    }

    // ── eID ──────────────────────────────────────────────────────────────────

    void runEidFullname(const std::string& id, const json& a) {
        const std::string vn = a.at("erwartet_vorname").get<std::string>();
        const std::string nn = a.at("erwartet_nachname").get<std::string>();

        // Identität aus Antrag-Fixture aufbauen
        EIDIdentity identity;
        identity.transaction_id   = ctx_.antrag.at("antragsteller").at("eid_tx_id").get<std::string>();
        identity.eid_server_id    = ctx_.antrag.at("antragsteller").at("eid_server").get<std::string>();
        identity.assurance        = EIDAssuranceLevel::HIGH;
        identity.authenticated_at = std::chrono::system_clock::now();
        identity.attributes = {
            {EIDAttributeType::GIVEN_NAMES,       vn},
            {EIDAttributeType::FAMILY_NAMES,       nn},
            {EIDAttributeType::MUNICIPALITY_ID,
             ctx_.antrag.at("antragsteller").value("ags", "00000000")},
        };

        ctx_.eid.storeIdentity(identity);

        EIDAuthRequest req;
        req.transaction_id   = identity.transaction_id;
        req.service_provider = "ThemisDB-DataDrivenTest";
        req.minimum_assurance = EIDAssuranceLevel::HIGH;

        auto session = ctx_.eid.beginAuthSession(req);
        ASSERT_NE(session.session_id, "") << id;

        auto result = ctx_.eid.completeAuthSession(session.session_id, "OK");
        ASSERT_TRUE(result.has_value()) << id << ": eID-Auth fehlgeschlagen";
        EXPECT_EQ(result->fullName(), vn + " " + nn) << id;
        EXPECT_EQ(result->assurance, EIDAssuranceLevel::HIGH) << id;
    }

    void runEidAttribut(const std::string& id, const json& a) {
        const std::string attr_name = a.at("attribut").get<std::string>();
        const std::string erwartet  = a.at("erwartet_wert").get<std::string>();
        const std::string tx_id     = ctx_.antrag.at("antragsteller").at("eid_tx_id").get<std::string>();

        // Versuche aktuelle Session zu finden (aus vorherigem eid_fullname-Test)
        EIDAttributeType attr_type = {};
        if (attr_name == "MUNICIPALITY_ID") {
            attr_type = EIDAttributeType::MUNICIPALITY_ID;
        } else if (attr_name == "GIVEN_NAMES") {
            attr_type = EIDAttributeType::GIVEN_NAMES;
        } else if (attr_name == "FAMILY_NAMES") {
            attr_type = EIDAttributeType::FAMILY_NAMES;
        } else {
            ADD_FAILURE() << id << ": Unbekanntes EID-Attribut: " << attr_name;
            return;
        }

        // Neue Identität mit dem gesuchten Attribut anlegen
        EIDIdentity identity;
        identity.transaction_id   = tx_id + "-ATTR";
        identity.eid_server_id    = ctx_.antrag.at("antragsteller").at("eid_server").get<std::string>();
        identity.assurance        = EIDAssuranceLevel::HIGH;
        identity.authenticated_at = std::chrono::system_clock::now();
        identity.attributes = {
            {EIDAttributeType::GIVEN_NAMES,  ctx_.antrag.at("antragsteller").at("vorname").get<std::string>()},
            {EIDAttributeType::FAMILY_NAMES, ctx_.antrag.at("antragsteller").at("nachname").get<std::string>()},
            {attr_type,                      erwartet},
        };
        ctx_.eid.storeIdentity(identity);

        EIDAuthRequest req;
        req.transaction_id    = identity.transaction_id;
        req.service_provider  = "ThemisDB-AttrTest";
        req.minimum_assurance = EIDAssuranceLevel::HIGH;

        auto session = ctx_.eid.beginAuthSession(req);
        auto result  = ctx_.eid.completeAuthSession(session.session_id, "OK");
        ASSERT_TRUE(result.has_value()) << id;

        auto attr_val = result->getAttribute(attr_type);
        ASSERT_TRUE(attr_val.has_value()) << id << ": Attribut " << attr_name << " fehlt";
        EXPECT_EQ(*attr_val, erwartet) << id;
    }

    // ── XÖV ──────────────────────────────────────────────────────────────────

    void runXoevImport(const std::string& id, const json& a) {
        const std::string standard_str = a.at("standard").get<std::string>();
        const std::string record_id    = a.value("erwartet_record_id", ctx_.az);

        XOEVStandard standard = XOEVStandard::OTHER;
        if (standard_str == "XBAU") {
          standard = XOEVStandard::XBAU;
        }

        const auto& vorlage = ctx_.antrag.at("xoev_xml_vorlage");
        std::string xml = buildXOEVXml(ctx_.az, vorlage.at("wurzelelement").get<std::string>(),
                                        vorlage.at("felder"));

        auto res = ctx_.xoev.importFromXML(xml, standard);
        EXPECT_TRUE(res.success) << id << ": XÖV-Import fehlgeschlagen";
        ASSERT_FALSE(res.records.empty()) << id;
        EXPECT_EQ(res.records.front().id, record_id) << id;
    }

    void runXoevExport(const std::string& id, const json& a) {
        const std::string standard_str = a.at("standard").get<std::string>();
        const std::string enthaelt     = a.value("erwartet_enthaelt", "");
        const std::string xml_prefix   = a.value("erwartet_xml_prefix", "<?xml");

        XOEVStandard standard = XOEVStandard::OTHER;
        if (standard_str == "XBAU") {
          standard = XOEVStandard::XBAU;
        }

        const auto& vorlage = ctx_.antrag.at("xoev_xml_vorlage");
        std::string xml = buildXOEVXml(ctx_.az, vorlage.at("wurzelelement").get<std::string>(),
                                        vorlage.at("felder"));

        auto import_res = ctx_.xoev.importFromXML(xml, standard);
        ASSERT_TRUE(import_res.success) << id;

        XOEVVersion v; v.major = 1; v.minor = 0; v.patch = 0;
        auto export_res = ctx_.xoev.exportToXML(import_res.records, standard, v);

        EXPECT_TRUE(export_res.success) << id;
        ASSERT_FALSE(export_res.xml_output.empty()) << id;
        EXPECT_NE(export_res.xml_output.find(xml_prefix), std::string::npos) << id;
        if (!enthaelt.empty()) {
            EXPECT_NE(export_res.xml_output.find(enthaelt), std::string::npos) << id;
        }
    }

    // ── XDOMEA – Objekte ──────────────────────────────────────────────────────

    void runXdomeaObjekteMin(const std::string& id, const json& a) {
        // Kann für einzelne oder alle Behörden verwendet werden
        if (a.contains("alle_behoerden")) {
            for (const auto& bid_j : a.at("alle_behoerden")) {
                const std::string bid = bid_j.get<std::string>();
                ensureBehoerde(bid, id);
                int min_cnt = a.value("erwartet_min", 1);
                const std::string typ_filter = a.value("typ_filter", "VORGANG");
                auto objekte = typ_filter == "VORGANG"
                    ? ctx_.getDMS(bid).listByType(XDOMEAObjectType::VORGANG)
                    : ctx_.getDMS(bid).listByType(XDOMEAObjectType::DOKUMENT);
                EXPECT_GE(static_cast<int>(objekte.size()), min_cnt)
                    << id << ": Behörde " << bid << " hat zu wenige " << typ_filter;
            }
        } else {
            const std::string bid  = a.at("behoerde_id").get<std::string>();
            int min_cnt            = a.value("erwartet_min", 1);
            const std::string typ_filter = a.value("typ_filter", "VORGANG");
            ensureBehoerde(bid, id);
            auto objekte = typ_filter == "VORGANG"
                ? ctx_.getDMS(bid).listByType(XDOMEAObjectType::VORGANG)
                : ctx_.getDMS(bid).listByType(XDOMEAObjectType::DOKUMENT);
            EXPECT_GE(static_cast<int>(objekte.size()), min_cnt)
                << id << ": " << bid << " hat zu wenige " << typ_filter;
        }
    }

    void runXdomeaVorgaengeMin(const std::string& id, const json& a) {
        const std::string bid = a.at("behoerde_id").get<std::string>();
        int min_cnt           = a.value("erwartet_min", 1);
        ensureBehoerde(bid, id);
        auto vorgaenge = ctx_.getDMS(bid).listByType(XDOMEAObjectType::VORGANG);
        EXPECT_GE(static_cast<int>(vorgaenge.size()), min_cnt) << id;
    }

    void runXdomeaStellungnahmenAlle(const std::string& id, const json& a) {
        int min_je = a.value("erwartet_min_je", 1);
        for (const auto& bid_j : a.at("behoerden")) {
            const std::string bid = bid_j.get<std::string>();
            ensureBehoerde(bid, id);
            auto dokumente = ctx_.getDMS(bid).listByType(XDOMEAObjectType::DOKUMENT);
            EXPECT_GE(static_cast<int>(dokumente.size()), min_je)
                << id << ": Behörde " << bid << " hat keine Stellungnahme";
        }
    }

    void runXdomeaBescheidVorhanden(const std::string& id, const json& a) {
        const std::string bid      = a.at("behoerde_id").get<std::string>();
        const std::string meta_key = a.at("metadata_key").get<std::string>();
        const std::string meta_val = a.at("metadata_wert").get<std::string>();

        ensureBehoerde(bid, id);
        auto dokumente = ctx_.getDMS(bid).listByType(XDOMEAObjectType::DOKUMENT);
        bool gefunden  = false;
        for (const auto& d : dokumente) {
            auto it = d.metadata.find(meta_key);
            if (it != d.metadata.end() && it->second == meta_val) {
                gefunden = true;
                break;
            }
        }
        EXPECT_TRUE(gefunden) << id << ": Kein Dokument mit " << meta_key << "=" << meta_val
                              << " in DMS von " << bid;
    }

    void runXdomeaAkteVorhanden(const std::string& id, const json& a) {
        const std::string bid     = a.at("behoerde_id").get<std::string>();
        const std::string erw_az  = a.at("erwartet_az").get<std::string>();

        ensureBehoerde(bid, id);

        // Metadaten-Erwartungen (optional)
        std::map<std::string,std::string> meta_exp = {};

        if (a.contains("erwartet_metadata")) {
            for (auto& [k, v] : a.at("erwartet_metadata").items()) {
                meta_exp[k] = v.get<std::string>();
            }
        }

        auto akten = ctx_.getDMS(bid).listByType(XDOMEAObjectType::AKTE);
        ASSERT_FALSE(akten.empty()) << id << ": Keine Akte im DMS von " << bid;
        EXPECT_EQ(akten.front().aktenzeichen, erw_az) << id;
        for (const auto& [k, v] : meta_exp) {
            auto it = akten.front().metadata.find(k);
            EXPECT_NE(it, akten.front().metadata.end()) << id << ": Metadatum " << k << " fehlt";
            if (it != akten.front().metadata.end()) {
                EXPECT_EQ(it->second, v) << id << ": Metadatum " << k;
            }
        }
    }

    void runXdomeaAktenzeichen(const std::string& id, const json& a) {
        const std::string erw_az = a.at("erwartet_az").get<std::string>();
        // Prüfe bei der federführenden Behörde
        const std::string fed = ctx_.federführend();
        if (fed.empty()) {
            GTEST_SKIP() << id << ": Keine federführende Behörde definiert";
            return;
        }
        ensureBehoerde(fed, id);
        auto dokumente = ctx_.getDMS(fed).listByType(XDOMEAObjectType::DOKUMENT);
        bool az_gefunden = false;
        for (const auto& d : dokumente) {
            if (d.aktenzeichen == erw_az) { az_gefunden = true; break; }
        }
        if (!az_gefunden) {
            // Auch in Akten schauen
            auto akten = ctx_.getDMS(fed).listByType(XDOMEAObjectType::AKTE);
            for (const auto& a2 : akten) {
                if (a2.aktenzeichen == erw_az) { az_gefunden = true; break; }
            }
        }
        EXPECT_TRUE(az_gefunden) << id << ": Aktenzeichen " << erw_az << " nicht gefunden";
    }

    void runXdomeaExport(const std::string& id, const json& a) {
        const std::string xml_prefix = a.value("erwartet_xml_prefix", "<?xml");
        const std::string enthaelt_tag = a.value("erwartet_enthaelt_tag", "");

        // Erzeuge ein Testdokument und exportiere es
        const std::string fed = ctx_.federführend();
        if (fed.empty()) { GTEST_SKIP() << id; return; }
        ensureBehoerde(fed, id);

        auto bescheid = makeDoc("EXPORT-TEST-001", XDOMEAObjectType::DOKUMENT,
                                ctx_.az, "Testbescheid-Export", fed, fed,
                                {{"typ","EXPORT_TEST"}});
        ctx_.getDMS(fed).storeDocument(bescheid);

        auto export_res = ctx_.getDMS(fed).exportToXML(
            {bescheid}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);
        EXPECT_TRUE(export_res.success) << id;
        ASSERT_FALSE(export_res.xml_output.empty()) << id;
        EXPECT_NE(export_res.xml_output.find(xml_prefix), std::string::npos) << id;
        if (!enthaelt_tag.empty()) {
            EXPECT_NE(export_res.xml_output.find(enthaelt_tag), std::string::npos) << id;
        }
    }

    void runXdomeaStoresMin(const std::string& id, const json& a) {
        for (const auto& store_entry : a.at("stores")) {
            const std::string bid = store_entry.at("behoerde_id").get<std::string>();
            int min_ges           = store_entry.value("min_gesamt", 1);
            ensureBehoerde(bid, id);
            size_t ges = ctx_.getDMS(bid).listByType(XDOMEAObjectType::DOKUMENT).size()
                       + ctx_.getDMS(bid).listByType(XDOMEAObjectType::AKTE).size()
                       + ctx_.getDMS(bid).listByType(XDOMEAObjectType::VORGANG).size();
            EXPECT_GE(static_cast<int>(ges), min_ges)
                << id << ": DMS " << bid << " hat nur " << ges << " Objekte (erwartet >= " << min_ges << ")";
        }
    }

    void runXdomeaKinderMin(const std::string& id, const json& a) {
        const std::string bid  = a.at("behoerde_id").get<std::string>();
        const std::string pid  = a.at("parent_id").get<std::string>();
        int min_cnt            = a.value("erwartet_min", 1);
        ensureBehoerde(bid, id);
        auto kinder = ctx_.getDMS(bid).listChildren(pid);
        EXPECT_GE(static_cast<int>(kinder.size()), min_cnt) << id;
    }

    void runXdomeaMetadatenVorhanden(const std::string& id, const json& a) {
        const std::string bid = a.at("behoerde_id").get<std::string>();
        ensureBehoerde(bid, id);
        auto dokumente = ctx_.getDMS(bid).listByType(XDOMEAObjectType::DOKUMENT);
        if (dokumente.empty()) {
            ADD_FAILURE() << id << ": Keine Dokumente in DMS von " << bid;
            return;
        }
        // Prüfe das erste gefundene Dokument mit den Pflicht-Metadaten
        for (const auto& key_j : a.at("pflicht_metadata_keys")) {
            const std::string key = key_j.get<std::string>();
            bool found = false;
            for (const auto& d : dokumente) {
                if (d.metadata.count(key)) { found = true; break; }
            }
            EXPECT_TRUE(found) << id << ": Pflicht-Metadatum '" << key << "' fehlt in DMS " << bid;
        }
    }

    void runEroeterungProtokoll(const std::string& id, const json& a) {
        const std::string bid     = a.at("behoerde_id").get<std::string>();
        const std::string meta_k  = a.at("metadata_key").get<std::string>();
        ensureBehoerde(bid, id);
        auto dokumente = ctx_.getDMS(bid).listByType(XDOMEAObjectType::DOKUMENT);
        bool hat_einwendungen = false;
        for (const auto& d : dokumente) {
            auto it = d.metadata.find(meta_k);
            if (it != d.metadata.end() && !it->second.empty()) {
                hat_einwendungen = true;
                break;
            }
        }
        EXPECT_TRUE(hat_einwendungen) << id << ": Erörterungsprotokoll ohne Einwendungsreferenzen";
    }

    // ── LLM ──────────────────────────────────────────────────────────────────

    void runLlmScoreMin(const std::string& id, const json& a) {
        double min_score = a.value("min_score", 0.5);
        LLMRequest req;
        req.task_type    = TaskType::PROCESS_CONFORMANCE;
        req.process_data = {{"az", ctx_.az}, {"test_id", id}};
        req.domain       = "EGov-DataDriven";
        auto [ok, resp]  = ctx_.llm->analyze(req);
        ASSERT_TRUE(ok) << id << ": LLM-Analyse fehlgeschlagen";
        EXPECT_GE(resp.conformance_score, min_score) << id;
    }

    void runLlmSummaryNichtleer(const std::string& id, const json&) {
        LLMRequest req;
        req.task_type    = TaskType::PROCESS_CONFORMANCE;
        req.process_data = {{"az", ctx_.az}};
        req.domain       = "EGov-Summary";
        auto [ok, resp]  = ctx_.llm->analyze(req);
        ASSERT_TRUE(ok) << id;
        EXPECT_FALSE(resp.summary.empty()) << id;
    }

    // ── Verfahrensstatus ──────────────────────────────────────────────────────

    void runVerfahrensstatus(const std::string& id, const json& a) {
        const std::string erwartet = a.at("erwartet_status").get<std::string>();
        // Status wird durch den Prozess-Vollständig-Test gesetzt
        EXPECT_EQ(ctx_.status, erwartet) << id
            << ": Verfahrensstatus ist '" << ctx_.status
            << "', erwartet '" << erwartet << "'";
    }

    // ── Prozessreihenfolge ────────────────────────────────────────────────────

    void runProzessReihenfolge(const std::string& id, const json& a) {
        const auto& reihenfolge = a.at("erwartete_reihenfolge");
        for (size_t i = 1; i < reihenfolge.size(); ++i) {
            const std::string prev = reihenfolge[i-1].get<std::string>() + ":";
            const std::string curr = reihenfolge[i].get<std::string>() + ":";
            int pos_prev = ctx_.logPos(prev);
            int pos_curr = ctx_.logPos(curr);
            if (pos_prev == -1 || pos_curr == -1) {
                // Phase noch nicht protokolliert → Reihenfolge-Test überspringen
                continue;
            }
            EXPECT_LT(pos_prev, pos_curr) << id << ": " << prev << " muss vor " << curr;
        }
    }

    // ── Thread-Sicherheit ─────────────────────────────────────────────────────

    void runThreadSafeParallelWrites(const std::string& id, const json& a) {
        int writes_je = a.value("schreibvorgaenge_je", 3);
        int min_je    = a.value("erwartet_min_je", writes_je);

        for (const auto& bid_j : a.at("behoerden")) {
            const std::string bid = bid_j.get<std::string>();
            ensureBehoerde(bid, id);

            std::vector<std::future<void>> futures;
            for (int i = 0; i < writes_je; ++i) {
                futures.push_back(std::async(std::launch::async,
                    [&, bid, i]() {
                        auto doc = makeDoc("TSAFE-" + bid + "-" + std::to_string(i),
                                           XDOMEAObjectType::DOKUMENT,
                                           ctx_.az, "Thread-Test", bid, bid,
                                           {{"test","thread_safe"}});
                        EXPECT_NO_THROW(ctx_.getDMS(bid).storeDocument(doc));
                    }));
            }
            for (auto& f : futures) {
                ASSERT_NO_THROW(f.get()) << id << ": Thread-Fehler für " << bid;
            }

            auto dokumente = ctx_.getDMS(bid).listByType(XDOMEAObjectType::DOKUMENT);
            EXPECT_GE(static_cast<int>(dokumente.size()), min_je) << id << ": " << bid;
        }
    }

    // ── Zustellung ───────────────────────────────────────────────────────────

    void runZustellung(const std::string& id, const json& a) {
        const std::string empfaenger = a.at("empfaenger").get<std::string>();
        const std::string meta_key   = a.at("metadata_key").get<std::string>();
        const std::string meta_val   = a.at("metadata_wert").get<std::string>();

        // Simuliert einen Betreiber/Antragsteller-DMS (eigener Store)
        InMemoryXDOMEAConnector dms_empfaenger;
        auto zustellung = makeDoc("ZUSTELLUNG-DATA-001", XDOMEAObjectType::DOKUMENT,
                                   ctx_.az, "Zustellung Bescheid",
                                   ctx_.federführend(), empfaenger,
                                   {{meta_key, meta_val}, {"empfaenger", empfaenger}});
        ASSERT_NO_THROW(dms_empfaenger.storeDocument(zustellung)) << id;

        auto found = dms_empfaenger.getDocument("ZUSTELLUNG-DATA-001");
        ASSERT_TRUE(found.has_value()) << id;
        EXPECT_EQ(found->metadata.at("empfaenger"), empfaenger) << id;
        EXPECT_EQ(found->metadata.at(meta_key), meta_val) << id;
    }

    // ── Hilfsmethode ─────────────────────────────────────────────────────────

    /**
     * @brief Stellt sicher, dass der DMS-Store für eine Behörde mindestens 1 Dokument
     *        enthält (legt Testdokument an, wenn der Store noch leer ist).
     */
    void ensureBehoerde(const std::string& bid, const std::string& assert_id) {
        if (ctx_.dms.find(bid) == ctx_.dms.end()) {
            ADD_FAILURE() << assert_id << ": Behörde '" << bid
                          << "' ist nicht in behoerden.json definiert";
        }
    }
};

// ══════════════════════════════════════════════════════════════════════════════
//  Vollständige Prozess-Simulation (füllt alle DMS-Stores)
// ══════════════════════════════════════════════════════════════════════════════

/**
 * @brief Führt eine miniaturisierte Simulation des Baugenehmigungsverfahrens
 *        durch und befüllt dabei alle notwendigen DMS-Stores.
 */
static void simulateBaugenehmigung(EGovFixtureContext& ctx) {
    const std::string az = ctx.az;
    const std::string fed = ctx.federführend();

    // Phase 1: Antragstellung
    ctx.getDMS(fed).storeDocument(makeDoc("AKTE-BGV-001", XDOMEAObjectType::AKTE,
                                          az, "Bauakte", fed, fed,
                                          {{"verfahrensart","STANDARD"}}));
    ctx.getDMS(fed).storeDocument(makeDoc("ANTRAG-BGV-001", XDOMEAObjectType::DOKUMENT,
                                          az, "Bauantrag", "Antragsteller", fed,
                                          {{"typ","ANTRAG"}}));
    ctx.logPhase("ANTRAGSTELLUNG: eID-Auth, XÖV-Import, XDOMEA-Akte");

    // Phase 2: Formale Prüfung
    ctx.getDMS(fed).storeDocument(makeDoc("FORMALPRUEF-001", XDOMEAObjectType::DOKUMENT,
                                          az, "Formale Prüfung", fed, fed,
                                          {{"typ","FORMALPRUEFUNG"}}));
    ctx.logPhase("FORMALPRUEFUNG: LLM-Vollständigkeit geprüft");

    // Phase 3: Vorgänge an Fachbehörden
    int v_idx = 1;
    for (const auto& fb : ctx.prozess.at("fachbehoerden")) {
        std::string bid = fb.at("behoerde_id").get<std::string>();
        // Vorgang beim federführenden Bauamt
        ctx.getDMS(fed).storeDocument(
            makeDoc("VG-BGV-" + std::to_string(v_idx), XDOMEAObjectType::VORGANG,
                    az, "Stellungnahme-Anfrage " + bid, fed, bid,
                    {{"typ","STELLUNGNAHME_ANFRAGE"}, {"empfaenger", bid}}));
        // Vorgang beim Empfänger
        ctx.getDMS(bid).storeDocument(
            makeDoc("VG-IN-" + std::to_string(v_idx), XDOMEAObjectType::VORGANG,
                    az, "Eingehende Anfrage", fed, bid,
                    {{"typ","STELLUNGNAHME_ANFRAGE"}}));
        v_idx++;
    }
    ctx.logPhase("STELLUNGNAHMEN: Vorgänge versandt");

    // Phase 4: Stellungnahmen (parallel)
    std::vector<std::future<void>> futures;
    int s_idx = 1;
    for (const auto& fb : ctx.prozess.at("fachbehoerden")) {
        std::string bid = fb.at("behoerde_id").get<std::string>();
        int si = s_idx++;
        futures.push_back(std::async(std::launch::async, [&ctx, bid, si, &az]() {
            auto stn = makeDoc("STN-BGV-" + std::to_string(si),
                               XDOMEAObjectType::DOKUMENT, az,
                               "Stellungnahme " + bid, bid, bid,
                               {{"typ","STELLUNGNAHME"}, {"ergebnis","ZUSTIMMUNG"}});
            ctx.getDMS(bid).storeDocument(stn);
        }));
    }
    for (auto& f : futures) {
      f.get();
    }
    ctx.logPhase("FACHLICHE_PRUEFUNG: Stellungnahmen erhalten");

    // Phase 5: Entscheidung
    ctx.getDMS(fed).storeDocument(makeDoc("ENTSCHEIDUNG-001", XDOMEAObjectType::DOKUMENT,
                                          az, "Entscheidung", fed, fed,
                                          {{"typ","ENTSCHEIDUNG"}}));
    ctx.logPhase("ENTSCHEIDUNG: Aggregierung und LLM-Empfehlung");

    // Phase 6: Bescheid
    ctx.getDMS(fed).storeDocument(makeDoc("BESCHEID-BGV-001", XDOMEAObjectType::DOKUMENT,
                                          az, "Baugenehmigungsbescheid", fed, fed,
                                          {{"typ","BESCHEID"}, {"ergebnis","GENEHMIGT"}}));
    ctx.logPhase("BESCHEID: Bescheid erstellt und zugestellt");

    ctx.status = "GENEHMIGT";
}

/**
 * @brief Führt eine miniaturisierte Simulation des BImSchG-Verfahrens durch.
 */
static void simulateBImSchG(EGovFixtureContext& ctx) {
    const std::string az  = ctx.az;
    const std::string fed = ctx.federführend();

    ctx.logPhase("ANTRAGSTELLUNG: eID-Auth, XÖV-Import, Verfahrensakte");

    // Verfahrensakte mit FOERMLICH
    ctx.getDMS(fed).storeDocument(makeDoc("VFA-BIM-001", XDOMEAObjectType::AKTE,
                                          az, "BImSchG-Verfahrensakte", fed, fed,
                                          {{"verfahrensart","FOERMLICH"},
                                           {"uvp_pflichtig","true"}}));
    ctx.logPhase("VOLLSTAENDIGKEITSPRUEFUNG: LLM geprüft");

    // UVP-Vorprüfung
    ctx.getDMS(fed).storeDocument(makeDoc("UVP-BIM-001", XDOMEAObjectType::DOKUMENT,
                                          az, "UVP-Screening", fed, fed,
                                          {{"typ","UVP_VORPRUEFUNG"},
                                           {"ergebnis","UVP_PFLICHTIG"}}));
    // LANUV: Anfrage + Stellungnahme
    ctx.getDMS("lanuv-nrw").storeDocument(
        makeDoc("LANUV-REQ-001", XDOMEAObjectType::VORGANG,
                az, "UVP-Screening-Anfrage", fed, "lanuv-nrw",
                {{"typ","UVP_SCREENING_ANFRAGE"}}));
    ctx.getDMS("lanuv-nrw").storeDocument(
        makeDoc("LANUV-STN-001", XDOMEAObjectType::DOKUMENT,
                az, "UVP-Stellungnahme LANUV", "lanuv-nrw", "lanuv-nrw",
                {{"typ","STELLUNGNAHME"}, {"ergebnis","UVP_PFLICHTIG"}}));
    ctx.logPhase("UVP_VORPRUEFUNG: UVP_PFLICHTIG");

    // Antragskonferenz
    ctx.getDMS(fed).storeDocument(makeDoc("AKF-BIM-001", XDOMEAObjectType::DOKUMENT,
                                          az, "Antragskonferenz-Protokoll", fed, fed,
                                          {{"typ","ANTRAGSKONFERENZ_PROTOKOLL"}}));
    for (const auto& fb : ctx.prozess.at("fachbehoerden")) {
        std::string bid = fb.at("behoerde_id").get<std::string>();
        ctx.getDMS(bid).storeDocument(
            makeDoc("AKF-INV-" + bid, XDOMEAObjectType::VORGANG,
                    az, "Antragskonferenz-Einladung", fed, bid,
                    {{"typ","FACHBEHOERDEN_ANFRAGE"}}));
    }
    ctx.logPhase("ANTRAGSKONFERENZ: Protokoll + Einladungen");

    // Auslegung + Einwendungen
    ctx.getDMS(fed).storeDocument(makeDoc("AUSL-BIM-001", XDOMEAObjectType::DOKUMENT,
                                          az, "Auslegungsbekanntmachung", fed, fed,
                                          {{"typ","AUSLEGUNGSBEKANNTMACHUNG"}}));
    for (int i = 1; i <= 3; ++i) {
        ctx.getDMS(fed).storeDocument(
            makeDoc("EINW-" + std::to_string(i), XDOMEAObjectType::DOKUMENT,
                    az, "Einwendung " + std::to_string(i),
                    "Einwender " + std::to_string(i), fed,
                    {{"typ","EINWENDUNG"}}));
    }
    ctx.logPhase("AUSLEGUNG: Bekanntmachung + 3 Einwendungen");

    // Fachbehörden parallel
    std::vector<std::future<void>> futures;
    int s_idx = 1;
    for (const auto& fb : ctx.prozess.at("fachbehoerden")) {
        std::string bid = fb.at("behoerde_id").get<std::string>();
        int si = s_idx++;
        std::map<std::string,std::string> extra = {};

        if (bid == "gesundheitsamt-duesseldorf") {
            extra["laerm_db"]   = "45 dB(A)";
            extra["geruch_gwk"] = "Irrelevanzgrenze";
        } else if (bid == "gewerbeaufsicht-nrw") {
            extra["auflage_betrsichv"] = "§ 14 BetrSichV";
            extra["auflage_ex_schutz"] = "ATEX Zone 2";
        }
        extra["typ"]     = "STELLUNGNAHME";
        extra["ergebnis"]= "ZUSTIMMUNG";
        futures.push_back(std::async(std::launch::async, [&ctx, bid, si, &az, extra]() {
            ctx.getDMS(bid).storeDocument(
                makeDoc("STN-BIM-" + std::to_string(si),
                        XDOMEAObjectType::DOKUMENT, az,
                        "Stellungnahme " + bid, bid, bid, extra));
        }));
    }
    for (auto& f : futures) {
      f.get();
    }
    ctx.logPhase("FACHBEHOERDEN_PRUEFUNG: 7 Stellungnahmen");

    // Erörterungstermin
    ctx.getDMS(fed).storeDocument(
        makeDoc("EROT-BIM-001", XDOMEAObjectType::DOKUMENT,
                az, "Erörterungsprotokoll", fed, fed,
                {{"typ","EROERTERUNGSTERMIN_PROTOKOLL"},
                 {"einwendungen","EINW-1,EINW-2,EINW-3"}}));
    ctx.logPhase("EROERTERUNGSTERMIN: Protokoll erstellt");

    // Entscheidung
    ctx.logPhase("ENTSCHEIDUNG: LLM-Analyse");

    // Genehmigungsbescheid + Nebenbestimmungen
    ctx.getDMS(fed).storeDocument(
        makeDoc("BESCHEID-001", XDOMEAObjectType::DOKUMENT,
                az, "Genehmigungsbescheid BImSchG", fed, fed,
                {{"typ","GENEHMIGUNGSBESCHEID"}, {"ergebnis","GENEHMIGT"}}));

    const auto& nb_liste = ctx.prozess.at("nebenbestimmungen_standard");
    for (size_t i = 0; i < nb_liste.size(); ++i) {
        auto nb = makeDoc("BESCHEID-NB-" + std::to_string(i+1),
                          XDOMEAObjectType::DOKUMENT, az,
                          nb_liste[i].get<std::string>(), fed, fed,
                          {{"typ","NEBENBESTIMMUNG"}});
        nb.parent_id = "BESCHEID-001";
        ctx.getDMS(fed).storeDocument(nb);
    }
    ctx.logPhase("GENEHMIGUNGSBESCHEID: Bescheid + Nebenbestimmungen");

    ctx.status = "GENEHMIGT";
}

// ══════════════════════════════════════════════════════════════════════════════
//  GTest-Fixture
// ══════════════════════════════════════════════════════════════════════════════

class EGovDataDrivenTest : public ::testing::TestWithParam<
    std::tuple<std::string,  // antrag_file
               std::string,  // prozess_file
               std::string,  // expected_file
               std::string   // test_label
    >>
{
protected:
    std::unique_ptr<EGovFixtureContext> ctx_;

    void SetUp() override {
        auto [antrag_f, prozess_f, expected_f, label] = GetParam();
        try {
            ctx_ = std::make_unique<EGovFixtureContext>(antrag_f, prozess_f, expected_f);
        } catch (const std::exception& ex) {
            GTEST_SKIP() << "Skipping eGov fixture-dependent test: " << ex.what();
        }
    }

    /** Führt die passende Prozesssimulation für den aktuellen Test aus. */
    void simulateProzess() {
        auto [antrag_f, prozess_f, expected_f, label] = GetParam();
        if (label == "BAUGENEHMIGUNG") {
            simulateBaugenehmigung(*ctx_);
        } else if (label == "BIMSCHV") {
            simulateBImSchG(*ctx_);
        }
    }
};

// ══════════════════════════════════════════════════════════════════════════════
//  Test 1: Fixtures laden (Smoke-Test)
// ══════════════════════════════════════════════════════════════════════════════

TEST_P(EGovDataDrivenTest, FixturesWerdenKorrektGeladen) {
    // behoerden.json
    ASSERT_TRUE(ctx_->behoerden.contains("behoerden")) << "behoerden.json fehlt 'behoerden'-Array";
    EXPECT_GE(ctx_->behoerden.at("behoerden").size(), 1u);

    // antrag.md — Markdown mit JSON-Front-Matter
    EXPECT_FALSE(ctx_->az.empty()) << "Antrag hat kein Aktenzeichen";
    EXPECT_TRUE(ctx_->antrag.contains("antragsteller"));
    // Prosatext aus dem Markdown-Body muss vorhanden sein
    ASSERT_TRUE(ctx_->antrag.contains("_dokument_text"))
        << "Antrag-Markdown hat kein Front-Matter oder keinen Prosatext";
    EXPECT_FALSE(ctx_->antrag.at("_dokument_text").get<std::string>().empty())
        << "Antrag-Markdown-Prosatext ist leer";

    // prozess.json
    EXPECT_TRUE(ctx_->prozess.contains("phasen"));
    EXPECT_TRUE(ctx_->prozess.contains("fachbehoerden"));

    // expected.json
    EXPECT_TRUE(ctx_->expected.contains("assertions"));
    EXPECT_GE(ctx_->expected.at("assertions").size(), 1u);
}

// ══════════════════════════════════════════════════════════════════════════════
//  Test 1b: Markdown-Front-Matter korrekt geladen (Antrag ist Markdown)
// ══════════════════════════════════════════════════════════════════════════════

TEST_P(EGovDataDrivenTest, MarkdownAntragFrontMatterValide) {
    // Pflichtfelder aus dem JSON-Front-Matter der .md-Datei
    EXPECT_TRUE(ctx_->antrag.contains("version"))           << "Feld 'version' fehlt";
    EXPECT_TRUE(ctx_->antrag.contains("antrag_typ"))        << "Feld 'antrag_typ' fehlt";
    EXPECT_TRUE(ctx_->antrag.contains("rechtsgrundlage"))   << "Feld 'rechtsgrundlage' fehlt";
    EXPECT_TRUE(ctx_->antrag.contains("aktenzeichen"))      << "Feld 'aktenzeichen' fehlt";
    EXPECT_TRUE(ctx_->antrag.contains("ozg_dienst_id"))     << "Feld 'ozg_dienst_id' fehlt";
    EXPECT_TRUE(ctx_->antrag.contains("xoev_standard"))     << "Feld 'xoev_standard' fehlt";
    EXPECT_TRUE(ctx_->antrag.contains("antragsteller"))     << "Feld 'antragsteller' fehlt";
    EXPECT_TRUE(ctx_->antrag.contains("unterlagen"))        << "Feld 'unterlagen' fehlt";
    EXPECT_TRUE(ctx_->antrag.contains("xoev_xml_vorlage"))  << "Feld 'xoev_xml_vorlage' fehlt";

    // Prosatext (Markdown-Body nach Front-Matter)
    ASSERT_TRUE(ctx_->antrag.contains("_dokument_text"));
    const std::string& text = ctx_->antrag.at("_dokument_text").get_ref<const std::string&>();
    EXPECT_FALSE(text.empty()) << "Markdown-Prosatext ist leer";
    // Der Prosatext muss mindestens eine Überschrift (# ...) enthalten
    EXPECT_NE(text.find('#'), std::string::npos) << "Markdown-Body hat keine Überschrift (#)";
    // Muss mindestens eine Tabelle enthalten (|)
    EXPECT_NE(text.find('|'), std::string::npos) << "Markdown-Body hat keine Tabelle";
}

// ══════════════════════════════════════════════════════════════════════════════
//  Test 2: Prozess-Metadaten valide
// ══════════════════════════════════════════════════════════════════════════════

TEST_P(EGovDataDrivenTest, ProzessDefinitionValide) {
    const auto& phasen = ctx_->prozess.at("phasen");
    EXPECT_GE(phasen.size(), 1u);

    // Reihenfolge-Nummern sind aufsteigend
    int letzte_reihenfolge = 0;
    for (const auto& phase : phasen) {
        int r = phase.value("reihenfolge", 0);
        EXPECT_GT(r, letzte_reihenfolge) << "Phasen-Reihenfolge nicht aufsteigend: " << phase.value("id","?");
        letzte_reihenfolge = r;
    }

    // Federführende Behörde in behoerden.json vorhanden
    std::string fed = ctx_->federführend();
    EXPECT_FALSE(fed.empty()) << "Keine federführende Behörde definiert";

    bool fed_found = false;
    for (const auto& b : ctx_->behoerden.at("behoerden")) {
        if (b.at("id").get<std::string>() == fed) { fed_found = true; break; }
    }
    EXPECT_TRUE(fed_found) << "Federführende Behörde '" << fed << "' nicht in behoerden.json";
}

// ══════════════════════════════════════════════════════════════════════════════
//  Test 3: Alle Fachbehörden haben DMS-Stores
// ══════════════════════════════════════════════════════════════════════════════

TEST_P(EGovDataDrivenTest, AlleFachbehoerdenHabenDMSStores) {
    for (const auto& fb : ctx_->prozess.at("fachbehoerden")) {
        const std::string bid = fb.at("behoerde_id").get<std::string>();
        EXPECT_NE(ctx_->dms.find(bid), ctx_->dms.end())
            << "Fachbehörde '" << bid << "' hat keinen DMS-Store";
    }
}

// ══════════════════════════════════════════════════════════════════════════════
//  Test 4: Kontrollergebnisse aus expected.json (Kern-Test)
// ══════════════════════════════════════════════════════════════════════════════

TEST_P(EGovDataDrivenTest, KontrollergebnisseAusFixture) {
    // Prozess simulieren → befüllt alle DMS-Stores
    ASSERT_NO_FATAL_FAILURE(simulateProzess());

    // Jede Assertion aus der expected.json auswerten
    EGovAssertionRunner runner(*ctx_);
    for (const auto& assertion : ctx_->expected.at("assertions")) {
        SCOPED_TRACE("Assertion: " + assertion.at("id").get<std::string>() +
                     " — " + assertion.value("beschreibung", ""));
        ASSERT_NO_FATAL_FAILURE(runner.run(assertion));
    }
}

// ══════════════════════════════════════════════════════════════════════════════
//  Test 5: Unterlagen vollständig (aus Antrag-Fixture)
// ══════════════════════════════════════════════════════════════════════════════

TEST_P(EGovDataDrivenTest, PflichtunterlagenVollstaendig) {
    const auto& unterlagen = ctx_->antrag.at("unterlagen");
    int pflicht_cnt = 0;
    for (const auto& u : unterlagen) {
        if (u.value("pflicht", false)) {
          pflicht_cnt++;
        }
    }
    EXPECT_GE(pflicht_cnt, 1) << "Antrag hat keine Pflichtunterlagen definiert";

    // Überprüfe, dass alle Pflichtunterlagen eine nicht-leere ID haben
    for (const auto& u : unterlagen) {
        if (u.value("pflicht", false)) {
            EXPECT_FALSE(u.at("id").get<std::string>().empty())
                << "Pflichtunterlage ohne ID: " << u.value("bezeichnung", "?");
        }
    }
}

// ══════════════════════════════════════════════════════════════════════════════
//  Test 6: OZG-Dienst kann aus Antrag-Fixture registriert werden
// ══════════════════════════════════════════════════════════════════════════════

TEST_P(EGovDataDrivenTest, OZGDienstAusAntragsFixture) {
    const std::string svc_id = ctx_->antrag.value("ozg_dienst_id", "");
    ASSERT_FALSE(svc_id.empty()) << "Antrag hat keine ozg_dienst_id";

    OZGServiceEntry svc;
    svc.id     = svc_id;
    svc.name   = ctx_->antrag.value("antrag_typ", svc_id);
    svc.level  = OZGFederalLevel::STATE;
    svc.status = OZGServiceStatus::ONLINE_TRANSACTION;
    ASSERT_NO_THROW(ctx_->ozg.registerService(svc));

    auto found = ctx_->ozg.findById(svc_id);
    EXPECT_TRUE(found.has_value());
}

// ══════════════════════════════════════════════════════════════════════════════
//  Test 7: XÖV-Roundtrip (Import → Export) aus Antrag-Fixture
// ══════════════════════════════════════════════════════════════════════════════

TEST_P(EGovDataDrivenTest, XOEVRoundtripAusAntragsFixture) {
    const std::string std_str = ctx_->antrag.value("xoev_standard", "OTHER");
    XOEVStandard standard = (std_str == "XBAU") ? XOEVStandard::XBAU : XOEVStandard::OTHER;

    const auto& vorlage = ctx_->antrag.at("xoev_xml_vorlage");
    std::string xml = buildXOEVXml(ctx_->az,
                                    vorlage.at("wurzelelement").get<std::string>(),
                                    vorlage.at("felder"));

    auto import_res = ctx_->xoev.importFromXML(xml, standard);
    ASSERT_TRUE(import_res.success) << "XÖV-Import fehlgeschlagen";
    ASSERT_FALSE(import_res.records.empty());
    EXPECT_EQ(import_res.records.front().id, ctx_->az);

    XOEVVersion v; v.major = 1; v.minor = 0; v.patch = 0;
    auto export_res = ctx_->xoev.exportToXML(import_res.records, standard, v);
    EXPECT_TRUE(export_res.success);
    EXPECT_FALSE(export_res.xml_output.empty());
    EXPECT_NE(export_res.xml_output.find("<?xml"), std::string::npos);
}

// ══════════════════════════════════════════════════════════════════════════════
//  Test 8: eID-Authentifizierung aus Antragsteller-Fixture
// ══════════════════════════════════════════════════════════════════════════════

TEST_P(EGovDataDrivenTest, EIDAAuthentifizierungAusFixture) {
    const auto& ast = ctx_->antrag.at("antragsteller");
    const std::string vn = ast.at("vorname").get<std::string>();
    const std::string nn = ast.at("nachname").get<std::string>();

    EIDIdentity identity;
    identity.transaction_id   = ast.at("eid_tx_id").get<std::string>();
    identity.eid_server_id    = ast.at("eid_server").get<std::string>();
    identity.assurance        = EIDAssuranceLevel::HIGH;
    identity.authenticated_at = std::chrono::system_clock::now();
    identity.attributes = {
        {EIDAttributeType::GIVEN_NAMES,  vn},
        {EIDAttributeType::FAMILY_NAMES, nn},
        {EIDAttributeType::MUNICIPALITY_ID, ast.value("ags", "00000000")},
    };
    ctx_->eid.storeIdentity(identity);

    EIDAuthRequest req;
    req.transaction_id    = identity.transaction_id;
    req.service_provider  = "ThemisDB-Test";
    req.minimum_assurance = EIDAssuranceLevel::HIGH;

    auto session = ctx_->eid.beginAuthSession(req);
    ASSERT_NE(session.session_id, "");

    auto result = ctx_->eid.completeAuthSession(session.session_id, "OK");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->fullName(), vn + " " + nn);
    EXPECT_EQ(result->assurance, EIDAssuranceLevel::HIGH);
}

// ══════════════════════════════════════════════════════════════════════════════
//  Parametrisierung — beide Verfahren
// ══════════════════════════════════════════════════════════════════════════════

INSTANTIATE_TEST_SUITE_P(
    EGovVerfahren,
    EGovDataDrivenTest,
    ::testing::Values(
        std::make_tuple(
            "antraege/baugenehmigung.md",
            "prozesse/baugenehmigung_prozess.json",
            "expected/baugenehmigung_expected.json",
            "BAUGENEHMIGUNG"
        ),
        std::make_tuple(
            "antraege/bimschg.md",
            "prozesse/bimschg_prozess.json",
            "expected/bimschg_expected.json",
            "BIMSCHV"
        )
    ),
    [](const ::testing::TestParamInfo<EGovDataDrivenTest::ParamType>& info) {
        return std::get<3>(info.param);
    }
);
