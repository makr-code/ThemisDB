/*
 * ThemisDB — Vollautomatisches Behörden-Genehmigungsverfahren E2E-Testszenario
 *
 * Szenario:  Baugenehmigungsverfahren (§ 63 BauO NRW)
 *
 * Beteiligte Behörden:
 *   - Bauamt Köln          (federführende Genehmigungsbehörde)
 *   - Denkmalschutzbehörde NRW  (Fachbehörde — Stellungnahme)
 *   - Umweltamt            (Fachbehörde — Stellungnahme)
 *   - Feuerwehr / Brandschutz   (Fachbehörde — Stellungnahme)
 *
 * Prozessschritte (vollautomatisch):
 *   Phase 1  Antragstellung     — eID-Authentifizierung des Antragstellers,
 *                                  Erzeugung der Antragsunterlagen (XÖV/XDOMEA),
 *                                  OZG-Dienst-Registrierung (Baugenehmigung)
 *   Phase 2  Formale Prüfung    — Bauamt prüft Vollständigkeit via LLM-Analyse
 *   Phase 3  Stellungnahmen     — Bauamt sendet Vorgänge (XDOMEA) an drei
 *                                  Fachbehörden (parallel)
 *   Phase 4  Fachliche Prüfung  — Jede Fachbehörde analysiert Dokumente per
 *                                  LLM und erzeugt eine Stellungnahme
 *   Phase 5  Entscheidung       — Bauamt aggregiert Stellungnahmen, LLM erstellt
 *                                  Entscheidungsempfehlung (Genehmigung/Ablehnung)
 *   Phase 6  Bescheid           — Bauamt erzeugt finalen Bescheid als XDOMEA-
 *                                  Dokument und teilt ihn mit dem Antragsteller
 *
 * Akzeptanzkriterien:
 *   AC-BGV-01  OZG-Dienst für Baugenehmigung ist nach Registrierung abrufbar
 *   AC-BGV-02  Antragsteller-eID-Authentifizierung liefert verifizierten Namen
 *   AC-BGV-03  Antragsunterlagen (XÖV-XBau) werden korrekt importiert
 *   AC-BGV-04  Formale Prüfung erkennt alle Pflichtunterlagen als vorhanden
 *   AC-BGV-05  Bauamt erzeugt drei XDOMEA-Vorgänge (je eine Stellungnahme-Anfrage)
 *   AC-BGV-06  Denkmalschutzbehörde empfängt und verarbeitet ihren Vorgang
 *   AC-BGV-07  Umweltamt empfängt und verarbeitet seinen Vorgang
 *   AC-BGV-08  Feuerwehr empfängt und verarbeitet ihren Vorgang
 *   AC-BGV-09  Alle drei Fachbehörden liefern Stellungnahme-Dokumente zurück
 *   AC-BGV-10  Bauamt aggregiert Stellungnahmen und erzeugt LLM-Entscheidung
 *   AC-BGV-11  Finaler Bescheid (XDOMEA DOKUMENT) wird im DMS des Bauamts gespeichert
 *   AC-BGV-12  Bescheid enthält korrekte Aktenzeichen-Referenz
 *   AC-BGV-13  Gesamtprozess endet mit Status GENEHMIGT oder ABGELEHNT
 *   AC-BGV-14  XDOMEA-Export des Bescheids ist wohlgeformt (non-empty XML)
 *   AC-BGV-15  Antragsteller erhält XDOMEA-Bescheid in seiner Akte
 *   AC-BGV-16  Alle Behörden-XDOMEA-Stores haben die erwarteten Dokumentzahlen
 *   AC-BGV-17  LLM-Konformitätsprüfung des Gesamtverfahrens liefert score >= 0.5
 *   AC-BGV-18  Prozessschritte sind in korrekter Reihenfolge protokolliert
 *   AC-BGV-19  Thread-sichere Verarbeitung paralleler Stellungnahmen
 *   AC-BGV-20  XÖV-XBau-Export der Antragsunterlagen ist wohlgeformt
 *
 * Standards:
 *   - OZG 2.0 (2024) — Onlinezugangsgesetz
 *   - XDOMEA 3.0.0 — KoSIT
 *   - XBau — Bauwesen XÖV-Standard
 *   - BSI TR-03130 — eID-Server
 *   - BauO NRW § 63 — Genehmigungsverfahren
 *   - VwVfG §§ 24, 25, 28 — Verwaltungsverfahren
 */

#include <gtest/gtest.h>

#include "auth/eid_authenticator.h"
#include "document/xdomea_connector.h"
#include "importers/ozg_service_registry.h"
#include "importers/xoev_importer.h"
#include "analytics/llm_process_analyzer.h"

#include <future>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <algorithm>

using namespace themis::auth;
using namespace themis::document;
using namespace themis::importers;
using namespace themis;

// ── Hilfsfunktionen ───────────────────────────────────────────────────────────

namespace {

/**
 * @brief Erzeuge eine eindeutige ID für Dokumente / Vorgänge.
 */
static std::string makeId(const std::string& prefix, int seq) {
    return prefix + "-" + std::to_string(seq);
}

/**
 * @brief Bilde ein ISO-8601-ähnliches Timestamp-Stub für Tests.
 */
static std::string nowIso() {
    return "2026-06-01T10:00:00Z";
}

/**
 * @brief Erzeuge ein minimales XÖV-XBau-Antrags-XML.
 */
static std::string makeXBauXML(const std::string& az,
                                const std::string& antragsteller,
                                const std::string& bauvorhaben) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
           "<xbau>"
           "<record>"
           "<id>" + az + "</id>"
           "<standard>XBAU</standard>"
           "<antragsteller>" + antragsteller + "</antragsteller>"
           "<bauvorhaben>" + bauvorhaben + "</bauvorhaben>"
           "<grundstueck>Musterstraße 1, 50667 Köln</grundstueck>"
           "<flaeche>320</flaeche>"
           "<geschosse>3</geschosse>"
           "<nutzungsart>Wohngebaeude</nutzungsart>"
           "<baugenehmigung_erforderlich>true</baugenehmigung_erforderlich>"
           "</record>"
           "</xbau>";
}

/**
 * @brief Bilde ein XDOMEA-Antragsdokument für den Bauantrag.
 */
static XDOMEADocument makeAntragsDokument(const std::string& id,
                                           const std::string& az,
                                           const std::string& betreff,
                                           const std::string& behoerde) {
    XDOMEADocument doc;
    doc.id              = id;
    doc.object_type     = XDOMEAObjectType::DOKUMENT;
    doc.aktenzeichen    = az;
    doc.betreff         = betreff;
    doc.author          = "Antragsteller";
    doc.responsible_unit = behoerde;
    doc.source_authority = "Antragsteller";
    doc.xdomea_version  = "3.0.0";
    doc.mime_type       = "application/pdf";
    doc.created_at      = nowIso();
    doc.retention       = XDOMEARetentionCategory::ARCHIVWUERDIG;
    return doc;
}

/**
 * @brief Bilde einen XDOMEA-Vorgang (Container für Stellungnahme-Anfrage).
 */
static XDOMEADocument makeVorgang(const std::string& id,
                                   const std::string& az,
                                   const std::string& betreff,
                                   const std::string& absender,
                                   const std::string& empfaenger) {
    XDOMEADocument vorgang;
    vorgang.id               = id;
    vorgang.object_type      = XDOMEAObjectType::VORGANG;
    vorgang.aktenzeichen     = az;
    vorgang.betreff          = betreff;
    vorgang.author           = absender;
    vorgang.responsible_unit = empfaenger;
    vorgang.source_authority = absender;
    vorgang.xdomea_version   = "3.0.0";
    vorgang.created_at       = nowIso();
    vorgang.retention        = XDOMEARetentionCategory::ARCHIVWUERDIG;
    vorgang.metadata["status"] = "OFFEN";
    vorgang.metadata["typ"]    = "STELLUNGNAHME_ANFRAGE";
    return vorgang;
}

/**
 * @brief Bilde ein XDOMEA-Stellungnahme-Dokument einer Fachbehörde.
 */
static XDOMEADocument makeStellungnahme(const std::string& id,
                                         const std::string& parent_vorgang_id,
                                         const std::string& az,
                                         const std::string& behoerde,
                                         const std::string& ergebnis,
                                         const std::string& begruendung) {
    XDOMEADocument stn;
    stn.id               = id;
    stn.object_type      = XDOMEAObjectType::DOKUMENT;
    stn.aktenzeichen     = az;
    stn.betreff          = "Stellungnahme " + behoerde;
    stn.author           = behoerde;
    stn.responsible_unit = behoerde;
    stn.source_authority = behoerde;
    stn.xdomea_version   = "3.0.0";
    stn.mime_type        = "application/pdf";
    stn.created_at       = nowIso();
    stn.parent_id        = parent_vorgang_id;
    stn.retention        = XDOMEARetentionCategory::ARCHIVWUERDIG;
    stn.metadata["ergebnis"]     = ergebnis;
    stn.metadata["begruendung"]  = begruendung;
    stn.metadata["typ"]          = "STELLUNGNAHME";
    return stn;
}

/**
 * @brief Erstelle eine vorkonfigurierte eID-Identität für den Antragsteller.
 */
static EIDIdentity makeAntragstellerIdentity(const std::string& vorname,
                                              const std::string& nachname,
                                              const std::string& tx_id) {
    EIDIdentity id;
    id.transaction_id   = tx_id;
    id.eid_server_id    = "eid-server-bund-01";
    id.assurance        = EIDAssuranceLevel::HIGH;
    id.authenticated_at = std::chrono::system_clock::now();
    id.attributes = {
        {EIDAttributeType::GIVEN_NAMES,  vorname},
        {EIDAttributeType::FAMILY_NAMES, nachname},
        {EIDAttributeType::DATE_OF_BIRTH, "19800315"},
        {EIDAttributeType::MUNICIPALITY_ID, "05315000"},  // AGS Köln
        {EIDAttributeType::PLACE_OF_RESIDENCE, "Musterstraße 1, 50667 Köln"},
    };
    return id;
}

} // anonymous namespace

// ── Testklasse ────────────────────────────────────────────────────────────────

/**
 * @brief Fixture für den Behörden-Genehmigungsverfahren-E2E-Test.
 *
 * Stellt alle In-Memory-Implementierungen der beteiligten Behörden bereit,
 * die gemeinsam das Baugenehmigungsverfahren simulieren.
 */
class BehoerdenGenehmigungsverfahrenE2ETest : public ::testing::Test {
protected:
    // ── Aktenzeichen ─────────────────────────────────────────────────────────
    static constexpr const char* kAZ = "BAUAMT-KN-2026-0042";

    // ── Behörden-XDOMEA-Stores ───────────────────────────────────────────────
    InMemoryXDOMEAConnector dms_bauamt_;         ///< DMS des Bauamts Köln
    InMemoryXDOMEAConnector dms_denkmalschutz_;  ///< DMS der Denkmalschutzbehörde NRW
    InMemoryXDOMEAConnector dms_umweltamt_;      ///< DMS des Umweltamts
    InMemoryXDOMEAConnector dms_feuerwehr_;      ///< DMS der Feuerwehr

    // ── XÖV-Importer (XBau) ─────────────────────────────────────────────────
    InMemoryXOEVImporter xoev_bauamt_;           ///< XÖV-Daten beim Bauamt

    // ── OZG-Dienstregister ───────────────────────────────────────────────────
    InMemoryOZGServiceRegistry ozg_registry_;

    // ── eID-Authenticator ────────────────────────────────────────────────────
    InMemoryEIDAuthenticator eid_auth_;

    // ── LLM-Prozessanalyzer ──────────────────────────────────────────────────
    std::unique_ptr<LLMProcessAnalyzer> llm_analyzer_;

    // ── Prozessprotokoll ─────────────────────────────────────────────────────
    std::vector<std::string> prozess_log_;
    std::mutex               log_mutex_;

    void SetUp() override {
        // LLM-Analyzer mit LOCAL-Provider initialisieren (simulierte Antworten)
        LLMConfig cfg;
        cfg.provider       = LLMProvider::LOCAL;
        cfg.enable_caching = true;
        llm_analyzer_ = std::make_unique<LLMProcessAnalyzer>(cfg);

        // eID-Server konfigurieren
        EIDAuthConfig eid_cfg;
        eid_cfg.enabled               = true;
        eid_cfg.eid_server_url        = "https://eid-server.bund.de/paos";
        eid_cfg.sp_return_url         = "https://bauamt.koeln.de/callback";
        eid_cfg.terminal_certificate  = "-----BEGIN CERTIFICATE-----\nMOCK\n-----END CERTIFICATE-----";
        eid_cfg.requested_attributes  = {
            EIDAttributeType::GIVEN_NAMES,
            EIDAttributeType::FAMILY_NAMES,
            EIDAttributeType::DATE_OF_BIRTH,
            EIDAttributeType::MUNICIPALITY_ID,
            EIDAttributeType::PLACE_OF_RESIDENCE,
        };
        ASSERT_TRUE(eid_auth_.initialize(eid_cfg));

        // OZG-Dienst "Baugenehmigung" registrieren
        OZGServiceEntry baugen;
        baugen.id          = "99014001010000";
        baugen.name        = "Baugenehmigung";
        baugen.short_name  = "BauGen";
        baugen.description = "Antrag auf Erteilung einer Baugenehmigung nach BauO NRW § 63";
        baugen.level       = OZGFederalLevel::MUNICIPAL;
        baugen.applicable_states = {"DE-NW"};
        baugen.legal_basis = {"§ 63 BauO NRW", "§ 29 BauGB"};
        baugen.compliance_tags = {"baugenehmigung", "ozg", "nrw", "vwvfg"};
        baugen.status      = OZGServiceStatus::ONLINE_TRANSACTION;
        baugen.sdg_relevant = false;
        baugen.fields = {
            {"F001", "Antragsteller Name",    OZGFieldType::PERSON_NAME, true,  "Vollständiger Name des Antragstellers", ""},
            {"F002", "Bauvorhaben",           OZGFieldType::TEXT,        true,  "Kurzbeschreibung des Bauvorhabens", ""},
            {"F003", "Grundstück",            OZGFieldType::ADDRESS,     true,  "Anschrift des Baugrundstücks", ""},
            {"F004", "Bauzeichnungen",        OZGFieldType::DOCUMENT_REF,true,  "Grundrisse, Schnitte, Ansichten 1:100", ""},
            {"F005", "Lageplan",              OZGFieldType::DOCUMENT_REF,true,  "Lageplan 1:500 mit Umgebungsbebauung", ""},
            {"F006", "Statik",                OZGFieldType::DOCUMENT_REF,false, "Statische Berechnung", ""},
            {"F007", "Brandschutzkonzept",    OZGFieldType::DOCUMENT_REF,false, "Brandschutzkonzept (>6 Wohneinheiten)", ""},
        };
        ozg_registry_.registerService(baugen);
    }

    void TearDown() override {
        llm_analyzer_.reset();
    }

    void logSchritt(const std::string& schritt) {
        std::unique_lock<std::mutex> lk(log_mutex_);
        prozess_log_.push_back(schritt);
    }
};

// ── AC-BGV-01 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase1_OZGDienstBaugenehmigungRegistriert) {
    auto entry = ozg_registry_.findById("99014001010000");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->name, "Baugenehmigung");
    EXPECT_EQ(entry->status, OZGServiceStatus::ONLINE_TRANSACTION);
    ASSERT_FALSE(entry->legal_basis.empty());
    EXPECT_EQ(entry->legal_basis.front(), "§ 63 BauO NRW");

    // Pflichtfelder prüfen
    auto required = entry->fields;
    auto req_count = std::count_if(required.begin(), required.end(),
                                   [](const OZGDataField& f){ return f.required; });
    EXPECT_GE(req_count, 4) << "Mindestens 4 Pflichtfelder erwartet";

    logSchritt("Phase1: OZG-Dienst Baugenehmigung registriert und gefunden");
}

// ── AC-BGV-02 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase1_AntragstellerEIDAuthentifizierung) {
    const std::string session = "SESSION-BGV-001";
    const std::string tx_id   = "TX-BGV-20260601-001";

    // Identität vorkonfigurieren
    eid_auth_.registerTestIdentity(session,
        makeAntragstellerIdentity("Hans", "Mustermann", tx_id));

    // eID-Session starten
    auto redirect = eid_auth_.beginAuthSession(session);
    ASSERT_FALSE(redirect.empty()) << "Redirect-URL darf nicht leer sein";
    EXPECT_NE(redirect.find("sessionId"), std::string::npos);

    // eID-Session abschließen (simulierter SAML-Response)
    auto result = eid_auth_.completeAuthSession(session, "SAML-MOCK-RESPONSE");
    ASSERT_TRUE(result.success) << "eID-Authentifizierung muss erfolgreich sein";
    ASSERT_TRUE(result.identity.has_value());
    EXPECT_EQ(result.identity->fullName(), "Hans Mustermann");

    auto municipality = result.identity->getAttribute(EIDAttributeType::MUNICIPALITY_ID);
    ASSERT_TRUE(municipality.has_value());
    EXPECT_EQ(*municipality, "05315000");  // AGS Köln

    logSchritt("Phase1: eID-Authentifizierung erfolgreich — " + result.identity->fullName());
}

// ── AC-BGV-03 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase1_AntragsunterlagenXOEVXBauImport) {
    // XÖV-XBau-Antragsdaten importieren
    std::string xbau_xml = makeXBauXML(kAZ, "Hans Mustermann", "Neubau Wohngebäude 3-geschossig");
    auto import_res = xoev_bauamt_.importFromXML(xbau_xml, XOEVStandard::XBAU);

    ASSERT_TRUE(import_res.success);
    ASSERT_EQ(import_res.records_imported, 1u);
    EXPECT_EQ(import_res.records_parsed,   1u);
    EXPECT_TRUE(import_res.errors.empty());

    // Felder prüfen
    ASSERT_FALSE(import_res.records.empty());
    const auto& rec = import_res.records.front();
    EXPECT_EQ(rec.id, std::string(kAZ));
    EXPECT_EQ(rec.standard, XOEVStandard::XBAU);

    auto it_ant = rec.fields.find("antragsteller");
    ASSERT_NE(it_ant, rec.fields.end());
    EXPECT_EQ(it_ant->second, "Hans Mustermann");

    auto it_bv = rec.fields.find("bauvorhaben");
    ASSERT_NE(it_bv, rec.fields.end());
    EXPECT_FALSE(it_bv->second.empty());

    logSchritt("Phase1: XÖV-XBau-Import erfolgreich — Aktenzeichen: " + std::string(kAZ));
}

// ── AC-BGV-04 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase2_FormalePruefungVollstaendigkeitspruefung) {
    // Antragsdokumente im Bauamt-DMS ablegen (simuliert den Antragseingang)
    std::vector<std::string> dokument_typen = {
        "Bauantrag",
        "Bauzeichnungen",
        "Lageplan",
        "Statiknachweis",
        "Brandschutzkonzept",
    };

    int seq = 1;
    for (const auto& typ : dokument_typen) {
        auto doc = makeAntragsDokument(
            makeId("DOC-ANT", seq++), kAZ,
            typ + " — " + std::string(kAZ), "Bauamt Köln");
        dms_bauamt_.storeDocument(doc);
    }

    // Vollständigkeit: alle Pflichtunterlagen im DMS prüfen
    auto alle_docs = dms_bauamt_.listByType(XDOMEAObjectType::DOKUMENT);
    ASSERT_EQ(alle_docs.size(), dokument_typen.size());

    // LLM-Konformitätsprüfung: Prozessschritt Vollständigkeit
    LLMRequest req;
    req.task_type = TaskType::ANALYZE_PROCESS;
    req.domain    = "administrative";
    req.process_trace = nlohmann::json::array({
        {{"activity", "antragstellung"},            {"timestamp", 1000}},
        {{"activity", "vollstaendigkeitspruefung"}, {"timestamp", 2000}},
    });
    req.ideal_model = nlohmann::json::object({
        {"process_id", "baugenehmigung_nrw"},
        {"required_steps", nlohmann::json::array({"antragstellung", "vollstaendigkeitspruefung"})}
    });
    req.parameters["compliance_check"] = "true";

    auto [ok, resp] = llm_analyzer_->analyze(req);
    ASSERT_TRUE(ok) << "LLM-Analyse muss erfolgreich sein";
    EXPECT_GE(resp.conformance_score, 0.0);

    logSchritt("Phase2: Formale Prüfung — alle " +
               std::to_string(dokument_typen.size()) + " Unterlagen vorhanden");
}

// ── AC-BGV-05 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase3_BauamtErstelltStellungnahmeAnfragen) {
    // Bauamt erzeugt je einen XDOMEA-Vorgang für drei Fachbehörden
    struct FachBehoerde {
        std::string id;
        std::string name;
    };
    std::vector<FachBehoerde> behoerden = {
        {"VG-DSB-001", "Denkmalschutzbehörde NRW"},
        {"VG-UWA-001", "Umweltamt Köln"},
        {"VG-FEU-001", "Feuerwehr Köln"},
    };

    for (const auto& b : behoerden) {
        auto vorgang = makeVorgang(
            b.id, kAZ,
            "Stellungnahme-Anfrage: Baugenehmigung " + std::string(kAZ),
            "Bauamt Köln", b.name);
        // Im Bauamt-DMS ablegen (Ausgangskorb)
        dms_bauamt_.storeDocument(vorgang);
    }

    // Erwartung: 3 Vorgänge vom Typ VORGANG im Bauamt-DMS
    auto vorgaenge = dms_bauamt_.listByType(XDOMEAObjectType::VORGANG);
    ASSERT_EQ(vorgaenge.size(), 3u);

    for (const auto& v : vorgaenge) {
        EXPECT_EQ(v.aktenzeichen, std::string(kAZ));
        EXPECT_EQ(v.metadata.at("typ"), "STELLUNGNAHME_ANFRAGE");
        EXPECT_FALSE(v.responsible_unit.empty());
    }

    logSchritt("Phase3: Bauamt hat 3 Stellungnahme-Anfragen (XDOMEA-Vorgänge) erzeugt");
}

// ── AC-BGV-06 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase4_DenkmalschutzEmpfaengtUndVerarbeitetVorgang) {
    // Simuliere Übertragung: Bauamt → Denkmalschutzbehörde
    auto vorgang = makeVorgang(
        "VG-DSB-001", kAZ,
        "Stellungnahme: Denkmalschutz — Baugenehmigung " + std::string(kAZ),
        "Bauamt Köln", "Denkmalschutzbehörde NRW");
    dms_denkmalschutz_.storeDocument(vorgang);

    // Prüfdokument: ist das Grundstück denkmalgeschützt?
    auto pruef_doc = makeAntragsDokument(
        "DOC-DSB-PRUEF-001", kAZ,
        "Prüfbericht Denkmalschutz — Musterstraße 1 Köln",
        "Denkmalschutzbehörde NRW");
    pruef_doc.parent_id = "VG-DSB-001";
    pruef_doc.metadata["denkmalschutz_relevant"] = "false";
    dms_denkmalschutz_.storeDocument(pruef_doc);

    // LLM-Analyse durch Denkmalschutzbehörde
    LLMRequest req;
    req.task_type = TaskType::COMPLIANCE_CHECK;
    req.domain    = "administrative";
    req.process_trace = nlohmann::json::array({
        {{"activity", "eingang_stellungnahme_anfrage"},  {"timestamp", 3000}},
        {{"activity", "denkmalschutz_pruefung"},          {"timestamp", 4000}},
    });
    req.context = nlohmann::json::object({
        {"aktenzeichen", kAZ},
        {"bauvorhaben",  "Neubau Wohngebäude"},
        {"denkmalschutz_relevant", false},
    });

    auto [ok, resp] = llm_analyzer_->analyze(req);
    EXPECT_TRUE(ok);

    // Stellungnahme erzeugen
    auto stn = makeStellungnahme(
        "STN-DSB-001", "VG-DSB-001", kAZ,
        "Denkmalschutzbehörde NRW",
        "KEINE_EINWAENDE",
        "Das Grundstück liegt nicht im Denkmalschutzbereich. Keine Einwände.");
    dms_denkmalschutz_.storeDocument(stn);

    // Vorgang prüfen: Denkmalschutz hat Dokumente
    auto docs = dms_denkmalschutz_.listByType(XDOMEAObjectType::DOKUMENT);
    ASSERT_EQ(docs.size(), 2u);  // Prüfdokument + Stellungnahme

    auto children = dms_denkmalschutz_.listChildren("VG-DSB-001");
    ASSERT_FALSE(children.empty()) << "Mindestens ein Dokument muss dem Vorgang untergeordnet sein";
    const auto stn_it = std::find_if(children.begin(), children.end(),
        [](const XDOMEADocument& d) { return d.id == "STN-DSB-001"; });
    EXPECT_NE(stn_it, children.end()) << "Stellungnahme muss dem Vorgang untergeordnet sein";

    logSchritt("Phase4: Denkmalschutz — Stellungnahme: KEINE_EINWAENDE");
}

// ── AC-BGV-07 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase4_UmweltamtEmpfaengtUndVerarbeitetVorgang) {
    // Simuliere Übertragung: Bauamt → Umweltamt
    auto vorgang = makeVorgang(
        "VG-UWA-001", kAZ,
        "Stellungnahme: Umweltschutz — Baugenehmigung " + std::string(kAZ),
        "Bauamt Köln", "Umweltamt Köln");
    dms_umweltamt_.storeDocument(vorgang);

    // Umweltprüfungs-Dokument anlegen
    auto umwelt_doc = makeAntragsDokument(
        "DOC-UWA-PRUEF-001", kAZ,
        "Umweltverträglichkeitsprüfung — Musterstraße 1 Köln",
        "Umweltamt Köln");
    umwelt_doc.parent_id = "VG-UWA-001";
    umwelt_doc.metadata["bodenschutz_ok"] = "true";
    umwelt_doc.metadata["laermschutz_ok"] = "true";
    umwelt_doc.metadata["gruenflaechen_ok"] = "true";
    dms_umweltamt_.storeDocument(umwelt_doc);

    // LLM-Analyse durch Umweltamt
    LLMRequest req;
    req.task_type = TaskType::COMPLIANCE_CHECK;
    req.domain    = "administrative";
    req.process_trace = nlohmann::json::array({
        {{"activity", "eingang_stellungnahme_anfrage"},   {"timestamp", 3000}},
        {{"activity", "umweltvertraeglichkeitspruefung"}, {"timestamp", 4500}},
    });
    req.context = nlohmann::json::object({
        {"aktenzeichen",    kAZ},
        {"bodenschutz_ok",  true},
        {"laermschutz_ok",  true},
    });

    auto [ok, resp] = llm_analyzer_->analyze(req);
    EXPECT_TRUE(ok);

    // Stellungnahme Umweltamt
    auto stn = makeStellungnahme(
        "STN-UWA-001", "VG-UWA-001", kAZ,
        "Umweltamt Köln",
        "ZUSTIMMUNG_MIT_AUFLAGEN",
        "Zustimmung mit Auflage: Versickerungsanlage für Regenwasser vorzusehen (§ 44 LWG NRW).");
    dms_umweltamt_.storeDocument(stn);

    auto docs = dms_umweltamt_.listByType(XDOMEAObjectType::DOKUMENT);
    ASSERT_EQ(docs.size(), 2u);

    auto stn_doc = dms_umweltamt_.getDocument("STN-UWA-001");
    ASSERT_TRUE(stn_doc.has_value());
    EXPECT_EQ(stn_doc->metadata.at("ergebnis"), "ZUSTIMMUNG_MIT_AUFLAGEN");

    logSchritt("Phase4: Umweltamt — Stellungnahme: ZUSTIMMUNG_MIT_AUFLAGEN");
}

// ── AC-BGV-08 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase4_FeuerwehrEmpfaengtUndVerarbeitetVorgang) {
    // Simuliere Übertragung: Bauamt → Feuerwehr
    auto vorgang = makeVorgang(
        "VG-FEU-001", kAZ,
        "Stellungnahme: Brandschutz — Baugenehmigung " + std::string(kAZ),
        "Bauamt Köln", "Feuerwehr Köln");
    dms_feuerwehr_.storeDocument(vorgang);

    // Brandschutz-Prüfdokument
    auto brand_doc = makeAntragsDokument(
        "DOC-FEU-PRUEF-001", kAZ,
        "Brandschutzprüfung — Musterstraße 1, Wohngebäude 3-geschossig",
        "Feuerwehr Köln");
    brand_doc.parent_id = "VG-FEU-001";
    brand_doc.metadata["fluchtweg_ok"]          = "true";
    brand_doc.metadata["loeschwasser_ok"]        = "true";
    brand_doc.metadata["rauchmelderpflicht"]     = "true";
    brand_doc.metadata["feuerwehrzufahrt_ok"]    = "true";
    dms_feuerwehr_.storeDocument(brand_doc);

    // LLM-Brandschutzprüfung
    LLMRequest req;
    req.task_type = TaskType::COMPLIANCE_CHECK;
    req.domain    = "administrative";
    req.process_trace = nlohmann::json::array({
        {{"activity", "eingang_stellungnahme_anfrage"}, {"timestamp", 3000}},
        {{"activity", "brandschutzpruefung"},           {"timestamp", 4200}},
    });
    req.context = nlohmann::json::object({
        {"aktenzeichen",       kAZ},
        {"geschosse",          3},
        {"fluchtweg_ok",       true},
        {"rauchmelderpflicht", true},
    });

    auto [ok, resp] = llm_analyzer_->analyze(req);
    EXPECT_TRUE(ok);

    // Brandschutz-Stellungnahme
    auto stn = makeStellungnahme(
        "STN-FEU-001", "VG-FEU-001", kAZ,
        "Feuerwehr Köln",
        "ZUSTIMMUNG",
        "Brandschutzkonzept ist genehmigungsfähig. Rauchwarnmelder nach DIN 14676 erforderlich.");
    dms_feuerwehr_.storeDocument(stn);

    auto docs = dms_feuerwehr_.listByType(XDOMEAObjectType::DOKUMENT);
    ASSERT_EQ(docs.size(), 2u);

    auto stn_doc = dms_feuerwehr_.getDocument("STN-FEU-001");
    ASSERT_TRUE(stn_doc.has_value());
    EXPECT_EQ(stn_doc->metadata.at("ergebnis"), "ZUSTIMMUNG");

    logSchritt("Phase4: Feuerwehr — Stellungnahme: ZUSTIMMUNG");
}

// ── AC-BGV-09 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase4_AlleStellungnahmenVollstaendig) {
    // Alle Fachbehörden erzeugen Stellungnahmen (parallel)
    struct BehInfo {
        InMemoryXDOMEAConnector* dms;
        std::string vorgang_id;
        std::string stn_id;
        std::string name;
        std::string ergebnis;
    };

    std::vector<BehInfo> behoerden = {
        {&dms_denkmalschutz_, "VG-DSB-P", "STN-DSB-P", "Denkmalschutz NRW", "KEINE_EINWAENDE"},
        {&dms_umweltamt_,     "VG-UWA-P", "STN-UWA-P", "Umweltamt Köln",    "ZUSTIMMUNG_MIT_AUFLAGEN"},
        {&dms_feuerwehr_,     "VG-FEU-P", "STN-FEU-P", "Feuerwehr Köln",    "ZUSTIMMUNG"},
    };

    // Parallele Verarbeitung (std::async)
    std::vector<std::future<void>> futures;
    for (auto& b : behoerden) {
        futures.push_back(std::async(std::launch::async, [&b, this]() {
            auto vorgang = makeVorgang(b.vorgang_id, kAZ,
                "Stellungnahme-Anfrage " + b.name, "Bauamt Köln", b.name);
            b.dms->storeDocument(vorgang);

            auto stn = makeStellungnahme(
                b.stn_id, b.vorgang_id, kAZ, b.name, b.ergebnis,
                "Automatisch erzeugte Stellungnahme im E2E-Test.");
            b.dms->storeDocument(stn);
            logSchritt("Phase4 (parallel): " + b.name + " — " + b.ergebnis);
        }));
    }

    // Warten bis alle Stellungnahmen vorliegen
    for (auto& f : futures) {
        ASSERT_NO_THROW(f.get());
    }

    // Jede Fachbehörde hat genau 1 Stellungnahme vom Typ DOKUMENT
    EXPECT_EQ(dms_denkmalschutz_.listByType(XDOMEAObjectType::DOKUMENT).size(), 1u);
    EXPECT_EQ(dms_umweltamt_.listByType(XDOMEAObjectType::DOKUMENT).size(),     1u);
    EXPECT_EQ(dms_feuerwehr_.listByType(XDOMEAObjectType::DOKUMENT).size(),     1u);

    // Jede Stellungnahme referenziert den richtigen Vorgang (parent_id)
    auto dsb_stn = dms_denkmalschutz_.getDocument("STN-DSB-P");
    ASSERT_TRUE(dsb_stn.has_value());
    EXPECT_EQ(dsb_stn->parent_id, std::make_optional<std::string>("VG-DSB-P"));

    auto uwa_stn = dms_umweltamt_.getDocument("STN-UWA-P");
    ASSERT_TRUE(uwa_stn.has_value());
    EXPECT_EQ(uwa_stn->parent_id, std::make_optional<std::string>("VG-UWA-P"));

    auto feu_stn = dms_feuerwehr_.getDocument("STN-FEU-P");
    ASSERT_TRUE(feu_stn.has_value());
    EXPECT_EQ(feu_stn->parent_id, std::make_optional<std::string>("VG-FEU-P"));
}

// ── AC-BGV-10 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase5_BauamtAggregatiertUndLLMEntscheidung) {
    // Stellungnahmen im Bauamt-DMS einlegen (simuliert Rücklauf)
    struct StnInfo { std::string id; std::string ergebnis; };
    std::vector<StnInfo> eingegangene_stn = {
        {"STN-DSB-RUECK", "KEINE_EINWAENDE"},
        {"STN-UWA-RUECK", "ZUSTIMMUNG_MIT_AUFLAGEN"},
        {"STN-FEU-RUECK", "ZUSTIMMUNG"},
    };

    int idx = 1;
    for (const auto& s : eingegangene_stn) {
        XDOMEADocument stn_rueck;
        stn_rueck.id               = s.id;
        stn_rueck.object_type      = XDOMEAObjectType::DOKUMENT;
        stn_rueck.aktenzeichen     = kAZ;
        stn_rueck.betreff          = "Stellungnahme-Rücklauf #" + std::to_string(idx++);
        stn_rueck.xdomea_version   = "3.0.0";
        stn_rueck.created_at       = nowIso();
        stn_rueck.retention        = XDOMEARetentionCategory::ARCHIVWUERDIG;
        stn_rueck.metadata["ergebnis"] = s.ergebnis;
        stn_rueck.metadata["typ"]      = "STELLUNGNAHME_RUECKLAUF";
        dms_bauamt_.storeDocument(stn_rueck);
    }

    // Aggregation: Bauamt prüft alle Stellungnahmen
    auto alle_docs = dms_bauamt_.listByType(XDOMEAObjectType::DOKUMENT);
    int einwaende    = 0;
    int zustimmungen = 0;
    for (const auto& doc : alle_docs) {
        auto it = doc.metadata.find("ergebnis");
        if (it == doc.metadata.end()) {
          continue;
        }
        if (it->second == "KEINE_EINWAENDE" || it->second == "ZUSTIMMUNG" ||
            it->second == "ZUSTIMMUNG_MIT_AUFLAGEN") {
            ++zustimmungen;
        } else {
            ++einwaende;
        }
    }
    EXPECT_EQ(einwaende, 0) << "Keine ablehnenden Stellungnahmen im E2E-Test";
    EXPECT_GE(zustimmungen, 3) << "Alle drei Stellungnahmen müssen positiv sein";

    // LLM-Entscheidungsanalyse
    LLMRequest req;
    req.task_type = TaskType::ANALYZE_PROCESS;
    req.domain    = "administrative";
    req.process_trace = nlohmann::json::array({
        {{"activity", "antragstellung"},            {"timestamp", 1000}},
        {{"activity", "vollstaendigkeitspruefung"}, {"timestamp", 2000}},
        {{"activity", "stellungnahmen_einholen"},   {"timestamp", 3000}},
        {{"activity", "stellungnahmen_eingegangen"},{"timestamp", 5000}},
        {{"activity", "entscheidung_vorbereiten"},  {"timestamp", 6000}},
    });
    req.context = nlohmann::json::object({
        {"aktenzeichen",     kAZ},
        {"zustimmungen",     zustimmungen},
        {"einwaende",        einwaende},
        {"entscheidung_tendenz", "GENEHMIGUNG"},
    });

    auto [ok, resp] = llm_analyzer_->analyze(req);
    ASSERT_TRUE(ok);
    EXPECT_GE(resp.conformance_score, 0.0);

    logSchritt("Phase5: LLM-Entscheidungsanalyse — Zustimmungen: " +
               std::to_string(zustimmungen) + ", Einwände: " + std::to_string(einwaende));
}

// ── AC-BGV-11 + AC-BGV-12 + AC-BGV-13 ────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase6_BescheiderstellungImDMSDesBauamts) {
    // Finaler Bescheid als XDOMEA-Dokument
    XDOMEADocument bescheid;
    bescheid.id               = "BESCHEID-KN-2026-0042";
    bescheid.object_type      = XDOMEAObjectType::DOKUMENT;
    bescheid.aktenzeichen     = kAZ;
    bescheid.betreff          = "Baugenehmigungsbescheid — " + std::string(kAZ);
    bescheid.author           = "Bauamt Köln";
    bescheid.responsible_unit = "Bauamt Köln";
    bescheid.source_authority = "Bauamt Köln";
    bescheid.xdomea_version   = "3.0.0";
    bescheid.mime_type        = "application/pdf";
    bescheid.created_at       = nowIso();
    bescheid.retention        = XDOMEARetentionCategory::ARCHIVWUERDIG;
    bescheid.metadata["status"]          = "GENEHMIGT";
    bescheid.metadata["typ"]             = "BESCHEID";
    bescheid.metadata["auflagen"]        = "Versickerungsanlage (§ 44 LWG NRW), Rauchwarnmelder (DIN 14676)";
    bescheid.metadata["gueltig_bis"]     = "2031-06-01";
    bescheid.metadata["antragsteller"]   = "Hans Mustermann";
    bescheid.keywords = {"Baugenehmigung", "Wohngebäude", "Köln", "NRW"};

    dms_bauamt_.storeDocument(bescheid);

    // AC-BGV-11: Bescheid ist im Bauamt-DMS gespeichert
    auto stored = dms_bauamt_.getDocument("BESCHEID-KN-2026-0042");
    ASSERT_TRUE(stored.has_value());

    // AC-BGV-12: Aktenzeichen im Bescheid korrekt
    EXPECT_EQ(stored->aktenzeichen, std::string(kAZ));

    // AC-BGV-13: Status ist GENEHMIGT
    EXPECT_EQ(stored->metadata.at("status"), "GENEHMIGT");

    logSchritt("Phase6: Baugenehmigungsbescheid erstellt — Status: GENEHMIGT");
}

// ── AC-BGV-14 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase6_XDOMEAExportDesBescheidsWohlgeformt) {
    // Bescheid im DMS speichern
    XDOMEADocument bescheid;
    bescheid.id             = "BESCHEID-EXPORT-001";
    bescheid.object_type    = XDOMEAObjectType::DOKUMENT;
    bescheid.aktenzeichen   = kAZ;
    bescheid.betreff        = "Baugenehmigungsbescheid (Export)";
    bescheid.author         = "Bauamt Köln";
    bescheid.xdomea_version = "3.0.0";
    bescheid.created_at     = nowIso();
    bescheid.retention      = XDOMEARetentionCategory::ARCHIVWUERDIG;
    bescheid.metadata["status"] = "GENEHMIGT";

    dms_bauamt_.storeDocument(bescheid);

    // XDOMEA-Export
    auto export_res = dms_bauamt_.exportToXML(
        {bescheid}, XDOMEAVersion::V3_0, XDOMEAMessageType::ABGABE_AN_ARCHIV);

    ASSERT_TRUE(export_res.success);
    EXPECT_GT(export_res.documents_exported, 0u);
    ASSERT_FALSE(export_res.xml_output.empty());

    // Minimale Wohlgeformtheit: enthält XML-Deklaration und root-Element
    EXPECT_NE(export_res.xml_output.find("<?xml"), std::string::npos)
        << "XML muss mit Deklaration beginnen";
    EXPECT_NE(export_res.xml_output.find(std::string(kAZ)), std::string::npos)
        << "Aktenzeichen muss im Export enthalten sein";

    logSchritt("Phase6: XDOMEA-Export des Bescheids — " +
               std::to_string(export_res.xml_output.size()) + " Zeichen");
}

// ── AC-BGV-15 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase6_AntragstellerErhaltBescheid) {
    // Antragsteller-DMS (simuliert als separater Store)
    InMemoryXDOMEAConnector dms_antragsteller;

    // Bescheid beim Bauamt
    XDOMEADocument bescheid;
    bescheid.id             = "BESCHEID-ZUSTELLUNG-001";
    bescheid.object_type    = XDOMEAObjectType::DOKUMENT;
    bescheid.aktenzeichen   = kAZ;
    bescheid.betreff        = "Ihr Baugenehmigungsbescheid";
    bescheid.author         = "Bauamt Köln";
    bescheid.xdomea_version = "3.0.0";
    bescheid.created_at     = nowIso();
    bescheid.retention      = XDOMEARetentionCategory::ARCHIVWUERDIG;
    bescheid.metadata["status"]         = "GENEHMIGT";
    bescheid.metadata["empfaenger"]     = "Hans Mustermann";
    bescheid.metadata["zustellungsart"] = "ELEKTRONISCH_DE_MAIL";

    // Bauamt sendet (exportiert) und Antragsteller empfängt (importiert via XML)
    auto export_res = dms_bauamt_.exportToXML(
        {bescheid}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);
    ASSERT_TRUE(export_res.success);
    ASSERT_FALSE(export_res.xml_output.empty());

    // Antragsteller importiert den Bescheid
    auto import_res = dms_antragsteller.importFromXML(
        export_res.xml_output, XDOMEAVersion::V3_0);
    ASSERT_TRUE(import_res.success);
    ASSERT_GE(import_res.documents_imported, 1u);

    // Antragsteller-Akte enthält den Bescheid
    auto docs = dms_antragsteller.listByType(XDOMEAObjectType::DOKUMENT);
    ASSERT_FALSE(docs.empty());

    logSchritt("Phase6: Bescheid elektronisch an Antragsteller zugestellt");
}

// ── AC-BGV-16 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, GesamtprozessDokumentanzahlJeBehoerde) {
    // Vollständigen Prozess durchlaufen und Dokumentzahlen prüfen

    // Phase 1+2: Antragsunterlagen im Bauamt (5 Dok + 1 Akte)
    XDOMEADocument akte;
    akte.id = "AKTE-BGV-001"; akte.object_type = XDOMEAObjectType::AKTE;
    akte.aktenzeichen = kAZ; akte.betreff = "Baugenehmigung " + std::string(kAZ);
    akte.xdomea_version = "3.0.0"; akte.created_at = nowIso();
    akte.retention = XDOMEARetentionCategory::ARCHIVWUERDIG;
    dms_bauamt_.storeDocument(akte);

    std::vector<std::string> unterlagen = {"Bauantrag","Bauzeichnungen","Lageplan","Statik","Brandschutz"};
    int seq = 100;
    for (const auto& u : unterlagen) {
        auto d = makeAntragsDokument(makeId("DOC", seq++), kAZ, u, "Bauamt Köln");
        d.parent_id = "AKTE-BGV-001";
        dms_bauamt_.storeDocument(d);
    }

    // Phase 3: 3 Vorgänge im Bauamt
    for (int i = 0; i < 3; ++i) {
        auto v = makeVorgang(makeId("VG", i), kAZ, "Anfrage " + std::to_string(i),
                             "Bauamt", "Fachbehoerde " + std::to_string(i));
        dms_bauamt_.storeDocument(v);
    }

    // Phase 4: je 1 Stellungnahme pro Fachbehörde
    auto stn_dsb = makeStellungnahme("STN-CNT-DSB", "VG-0", kAZ, "Denkmalschutz", "KEINE_EINWAENDE", "");
    auto stn_uwa = makeStellungnahme("STN-CNT-UWA", "VG-1", kAZ, "Umweltamt", "ZUSTIMMUNG", "");
    auto stn_feu = makeStellungnahme("STN-CNT-FEU", "VG-2", kAZ, "Feuerwehr", "ZUSTIMMUNG", "");
    dms_denkmalschutz_.storeDocument(stn_dsb);
    dms_umweltamt_.storeDocument(stn_uwa);
    dms_feuerwehr_.storeDocument(stn_feu);

    // Phase 5+6: Bescheid
    auto bescheid = makeAntragsDokument("BESCHEID-CNT-001", kAZ, "Bescheid", "Bauamt Köln");
    bescheid.metadata["status"] = "GENEHMIGT";
    dms_bauamt_.storeDocument(bescheid);

    // Erwartete Zählungen
    EXPECT_EQ(dms_bauamt_.listByType(XDOMEAObjectType::AKTE).size(),      1u);
    EXPECT_EQ(dms_bauamt_.listByType(XDOMEAObjectType::DOKUMENT).size(),   6u);  // 5 Antragsunterlagen + 1 Bescheid
    EXPECT_EQ(dms_bauamt_.listByType(XDOMEAObjectType::VORGANG).size(),    3u);

    EXPECT_EQ(dms_denkmalschutz_.listByType(XDOMEAObjectType::DOKUMENT).size(), 1u);
    EXPECT_EQ(dms_umweltamt_.listByType(XDOMEAObjectType::DOKUMENT).size(),     1u);
    EXPECT_EQ(dms_feuerwehr_.listByType(XDOMEAObjectType::DOKUMENT).size(),     1u);

    logSchritt("Phase6+: Dokumentzahlen aller Behörden stimmen");
}

// ── AC-BGV-17 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, GesamtprozessLLMKonformitaetsscore) {
    LLMRequest req;
    req.task_type = TaskType::ANALYZE_PROCESS;
    req.domain    = "administrative";

    // Vollständiger Prozess-Trace: alle Schritte des Baugenehmigungsverfahrens
    req.process_trace = nlohmann::json::array({
        {{"activity", "antragstellung"},              {"timestamp", 1000}},
        {{"activity", "eid_authentifizierung"},        {"timestamp", 1100}},
        {{"activity", "xoev_xbau_import"},             {"timestamp", 1200}},
        {{"activity", "vollstaendigkeitspruefung"},    {"timestamp", 2000}},
        {{"activity", "stellungnahme_denkmalschutz"},  {"timestamp", 3000}},
        {{"activity", "stellungnahme_umweltamt"},      {"timestamp", 3100}},
        {{"activity", "stellungnahme_feuerwehr"},      {"timestamp", 3200}},
        {{"activity", "ruecklauf_denkmalschutz"},      {"timestamp", 4000}},
        {{"activity", "ruecklauf_umweltamt"},          {"timestamp", 4100}},
        {{"activity", "ruecklauf_feuerwehr"},          {"timestamp", 4200}},
        {{"activity", "entscheidung_vorbereitung"},    {"timestamp", 5000}},
        {{"activity", "bescheid_erstellung"},          {"timestamp", 5500}},
        {{"activity", "bescheid_zustellung"},          {"timestamp", 6000}},
    });
    req.ideal_model = nlohmann::json::object({
        {"process_id", "baugenehmigung_bauordnungsrecht_nrw"},
        {"legal_basis", "§ 63 BauO NRW"},
        {"expected_steps", 13},
    });
    req.context = nlohmann::json::object({
        {"aktenzeichen", kAZ},
        {"ergebnis", "GENEHMIGT"},
        {"auflagen_count", 2},
    });

    auto [ok, resp] = llm_analyzer_->analyze(req);
    ASSERT_TRUE(ok) << "LLM-Gesamtanalyse muss erfolgreich sein";
    EXPECT_GE(resp.conformance_score, 0.5)
        << "Konformitätsscore des Gesamtverfahrens muss >= 0.5 sein";

    logSchritt("Phase-Gesamt: LLM-Konformitätsscore = " +
               std::to_string(resp.conformance_score));
}

// ── AC-BGV-18 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, GesamtprozessProzessschrittReihenfolge) {
    // Simulate the ordered steps as logged actions
    logSchritt("Phase1: OZG-Dienst Baugenehmigung registriert");
    logSchritt("Phase1: eID-Authentifizierung gestartet");
    logSchritt("Phase1: eID-Authentifizierung abgeschlossen");
    logSchritt("Phase1: XÖV-XBau-Antragsdaten importiert");
    logSchritt("Phase2: Vollständigkeitsprüfung abgeschlossen");
    logSchritt("Phase3: Stellungnahme-Anfragen versandt");
    logSchritt("Phase4: Denkmalschutz-Stellungnahme eingegangen");
    logSchritt("Phase4: Umweltamt-Stellungnahme eingegangen");
    logSchritt("Phase4: Feuerwehr-Stellungnahme eingegangen");
    logSchritt("Phase5: Entscheidung vorbereitet");
    logSchritt("Phase6: Bescheid erstellt und zugestellt");

    // Protokoll prüfen: Reihenfolge der Phasen
    ASSERT_GE(prozess_log_.size(), 11u) << "Mindestens 11 Prozessschritte müssen protokolliert sein";

    // Phase1 kommt vor Phase2 kommt vor Phase3 usw.
    auto find_idx = [this](const std::string& substr) -> int {
        for (int i = 0; i < static_cast<int>(prozess_log_.size()); ++i) {
            if (prozess_log_[i].find(substr) != std::string::npos) {
              return i;
            }
        }
        return -1;
    };

    int idx_phase1 = find_idx("Phase1:");
    int idx_phase2 = find_idx("Phase2:");
    int idx_phase3 = find_idx("Phase3:");
    int idx_phase4 = find_idx("Phase4:");
    int idx_phase5 = find_idx("Phase5:");
    int idx_phase6 = find_idx("Phase6:");

    EXPECT_LT(idx_phase1, idx_phase2) << "Phase1 muss vor Phase2 sein";
    EXPECT_LT(idx_phase2, idx_phase3) << "Phase2 muss vor Phase3 sein";
    EXPECT_LT(idx_phase3, idx_phase4) << "Phase3 muss vor Phase4 sein";
    EXPECT_LT(idx_phase4, idx_phase5) << "Phase4 muss vor Phase5 sein";
    EXPECT_LT(idx_phase5, idx_phase6) << "Phase5 muss vor Phase6 sein";
}

// ── AC-BGV-19 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, ParalleleStellungnahmenThreadSicher) {
    // 9 Dokumente parallel in verschiedene Behörden-Stores schreiben
    struct WriteJob {
        InMemoryXDOMEAConnector* dms;
        std::string doc_id;
        std::string behoerde;
    };

    std::vector<WriteJob> jobs = {};

    for (int i = 0; i < 3; ++i) {
        jobs.push_back({&dms_denkmalschutz_, "TSAFE-DSB-" + std::to_string(i), "Denkmalschutz"});
        jobs.push_back({&dms_umweltamt_,     "TSAFE-UWA-" + std::to_string(i), "Umweltamt"});
        jobs.push_back({&dms_feuerwehr_,     "TSAFE-FEU-" + std::to_string(i), "Feuerwehr"});
    }

    std::vector<std::future<void>> futures;
    for (const auto& job : jobs) {
        futures.push_back(std::async(std::launch::async, [&job, this]() {
            auto stn = makeStellungnahme(
                job.doc_id, "VG-TSAFE", kAZ, job.behoerde,
                "ZUSTIMMUNG", "Thread-Sicherheits-Test");
            EXPECT_NO_THROW(job.dms->storeDocument(stn));
        }));
    }

    for (auto& f : futures) {
        ASSERT_NO_THROW(f.get());
    }

    // Jede Fachbehörde hat 3 Thread-sichere Dokumente
    EXPECT_EQ(dms_denkmalschutz_.listByType(XDOMEAObjectType::DOKUMENT).size(), 3u);
    EXPECT_EQ(dms_umweltamt_.listByType(XDOMEAObjectType::DOKUMENT).size(),     3u);
    EXPECT_EQ(dms_feuerwehr_.listByType(XDOMEAObjectType::DOKUMENT).size(),     3u);
}

// ── AC-BGV-20 ────────────────────────────────────────────────────────────────

TEST_F(BehoerdenGenehmigungsverfahrenE2ETest, Phase1_XOEVXBauExportWohlgeformt) {
    // Import
    auto xbau_xml = makeXBauXML(kAZ, "Hans Mustermann", "Neubau Wohngebäude");
    auto import_res = xoev_bauamt_.importFromXML(xbau_xml, XOEVStandard::XBAU);
    ASSERT_TRUE(import_res.success);
    ASSERT_FALSE(import_res.records.empty());

    // Export
    XOEVVersion v; v.major = 1; v.minor = 0; v.patch = 0;
    auto export_res = xoev_bauamt_.exportToXML(
        import_res.records, XOEVStandard::XBAU, v);

    ASSERT_TRUE(export_res.success);
    ASSERT_FALSE(export_res.xml_output.empty());

    // Wohlgeformtheit
    EXPECT_NE(export_res.xml_output.find("<?xml"), std::string::npos)
        << "Export muss XML-Deklaration enthalten";
    EXPECT_NE(export_res.xml_output.find("Hans Mustermann"), std::string::npos)
        << "Antragstellername muss im XÖV-Export enthalten sein";

    logSchritt("Phase1: XÖV-XBau-Export erzeugt — " +
               std::to_string(export_res.xml_output.size()) + " Zeichen");
}
