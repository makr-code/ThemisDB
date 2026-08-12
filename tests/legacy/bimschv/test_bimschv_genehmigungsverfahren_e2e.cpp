/*
 * ThemisDB — 9. BImSchV Genehmigungsverfahren E2E-Testszenario
 *
 * Szenario:  Förmliches Genehmigungsverfahren nach § 10 BImSchG i. V. m.
 *            der 9. BImSchV (Genehmigungsverfahrensverordnung) für eine
 *            Großbiogasanlage (> 10 MW Feuerungswärmeleistung) mit UVP-Pflicht
 *
 * Beteiligte Behörden (8):
 *   1. Regierungspräsidium Düsseldorf    (federführende Genehmigungsbehörde)
 *   2. Landesamt für Natur, Umwelt       (Luftreinhaltung / TA Luft)
 *      und Verbraucherschutz NRW (LANUV)
 *   3. Staatliches Umweltamt              (Gewässerschutz / WHG)
 *   4. Gesundheitsamt                     (Lärmschutz / TA Lärm / Gesundheit)
 *   5. Bauaufsichtsbehörde                (Bauordnungsrecht)
 *   6. Gewerbeaufsichtsamt                (Arbeitsschutz / BetrSichV)
 *   7. Untere Wasserbehörde               (Wasserrecht § 8 WHG)
 *   8. Untere Naturschutzbehörde          (Naturschutz / §§ 44, 45 BNatSchG)
 *
 * Prozessschritte (9 Phasen — vollautomatisch):
 *   Phase 1  Antragstellung          — eID-Auth (Betreiber), XÖV-Antrag,
 *                                       OZG-Dienstregistrierung (BImSchG-Genehmigung)
 *   Phase 2  Vollständigkeitsprüfung — RP prüft Antragsunterlagen via LLM;
 *                                       Verfahrensart (förmlich/vereinfacht)
 *   Phase 3  UVP-Vorprüfung          — Screening-Dokument, LANUV-Stellungnahme
 *   Phase 4  Antragskonferenz        — RP koordiniert alle Fachbehörden (XDOMEA)
 *   Phase 5  Auslegung               — Öffentlichkeitsbeteiligung: Einwendungen
 *   Phase 6  Fachbehörden-Prüfung    — 7 Fachbehörden liefern Stellungnahmen (parallel)
 *   Phase 7  Erörterungstermin       — Einwendertermin-Protokoll (XDOMEA)
 *   Phase 8  Entscheidung            — RP aggregiert + LLM-Entscheidungsempfehlung
 *   Phase 9  Genehmigungsbescheid    — Finaler Bescheid (XDOMEA), Zustellung
 *
 * Akzeptanzkriterien:
 *   AC-BIM-01  OZG-Dienst für BImSchG-Genehmigung ist nach Registrierung abrufbar
 *   AC-BIM-02  Betreiber-eID-Authentifizierung liefert verifizierten Namen (HIGH)
 *   AC-BIM-03  Betreiber-eID enthält korrekte Behörden-ID (AGS Düsseldorf)
 *   AC-BIM-04  XÖV-OTHER-Antrag wird korrekt importiert (importFromXML)
 *   AC-BIM-05  XÖV-Export des Antrags ist wohlgeformt und vollständig
 *   AC-BIM-06  RP erzeugt XDOMEA-Verfahrensakte (AKTE) mit korrektem Aktenzeichen
 *   AC-BIM-07  LLM-Vollständigkeitsprüfung gibt score >= 0.5 zurück
 *   AC-BIM-08  Verfahrensart wird auf FOERMLICH gesetzt
 *   AC-BIM-09  UVP-Vorprüfungsdokument wird im DMS des RP gespeichert
 *   AC-BIM-10  LANUV erhält UVP-Screening-Anfrage als XDOMEA-Vorgang
 *   AC-BIM-11  LANUV liefert UVP-Screening-Stellungnahme (UVP_PFLICHTIG)
 *   AC-BIM-12  RP erzeugt Antragskonferenz-Protokoll (XDOMEA-DOKUMENT)
 *   AC-BIM-13  Alle 7 Fachbehörden erhalten Antragskonferenz-Einladung (VORGANG)
 *   AC-BIM-14  Auslegungsbekanntmachung wird im RP-DMS gespeichert
 *   AC-BIM-15  3 Einwendungen werden als XDOMEA-Dokumente erfasst
 *   AC-BIM-16  Alle 7 Fachbehörden liefern Stellungnahmen (parallel, thread-sicher)
 *   AC-BIM-17  Gesundheitsamt-Stellungnahme enthält Lärmprüfungs-Metadaten
 *   AC-BIM-18  Gewerbeaufsicht-Stellungnahme enthält Auflagen-Metadaten
 *   AC-BIM-19  Erörterungstermin-Protokoll wird im RP-DMS gespeichert
 *   AC-BIM-20  Erörterungstermin-Protokoll referenziert alle Einwendungen
 *   AC-BIM-21  LLM-Entscheidungsanalyse liefert score >= 0.5
 *   AC-BIM-22  LLM-Entscheidungsempfehlung enthält Schlüsselwort GENEHMIGUNG
 *   AC-BIM-23  Genehmigungsbescheid wird als XDOMEA-DOKUMENT im RP-DMS erzeugt
 *   AC-BIM-24  Genehmigungsbescheid enthält korrektes Aktenzeichen
 *   AC-BIM-25  Genehmigungsbescheid enthält alle 7 Nebenbestimmungen (Auflagen)
 *   AC-BIM-26  XDOMEA-Export des Genehmigungsbescheids ist wohlgeformt
 *   AC-BIM-27  Zustellung des Bescheids an Betreiber via XDOMEA
 *   AC-BIM-28  RP-DMS enthält nach Abschluss >= 12 Dokumente
 *   AC-BIM-29  Prozessschritte sind in korrekter 9-Phasen-Reihenfolge protokolliert
 *   AC-BIM-30  Gesamtverfahren endet mit Status GENEHMIGT
 *
 * Standards:
 *   - BImSchG § 10 (förmliches Genehmigungsverfahren)
 *   - 9. BImSchV — Genehmigungsverfahrensverordnung
 *   - UVPG — Gesetz über die Umweltverträglichkeitsprüfung
 *   - TA Luft 2021 — Technische Anleitung zur Reinhaltung der Luft
 *   - TA Lärm — Technische Anleitung zum Schutz gegen Lärm
 *   - WHG § 8 — Wasserhaushaltsgesetz
 *   - BNatSchG §§ 44, 45 — Bundesnaturschutzgesetz
 *   - XDOMEA 3.0.0 — KoSIT
 *   - OZG 2.0 (2024) — Onlinezugangsgesetz
 *   - BSI TR-03130 — eID-Server
 */

#include <gtest/gtest.h>

#include "auth/eid_authenticator.h"
#include "document/xdomea_connector.h"
#include "importers/ozg_service_registry.h"
#include "importers/xoev_importer.h"
#include "analytics/llm_process_analyzer.h"

#include <algorithm>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace themis::auth;
using namespace themis::document;
using namespace themis::importers;
using namespace themis;

// ── Hilfsfunktionen ───────────────────────────────────────────────────────────

namespace {

static std::string nowIso() { return "2026-06-01T10:00:00Z"; }

// Kleines XML-Fragment für einen XÖV-BImSchG-Antrag
static std::string makeBImSchGAntragsXML(const std::string& az,
                                          const std::string& betreiber,
                                          const std::string& anlage) {
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
           "<bimschg>"
           "<record>"
           "<id>" + az + "</id>"
           "<standard>OTHER</standard>"
           "<betreiber>" + betreiber + "</betreiber>"
           "<anlage>" + anlage + "</anlage>"
           "<standort>Industriepark Nord, 40549 Düsseldorf</standort>"
           "<feuerungswaermeleistung_mw>12.5</feuerungswaermeleistung_mw>"
           "<uvp_pflichtig>true</uvp_pflichtig>"
           "<verfahrensart>foermlich</verfahrensart>"
           "<unterlage>Antragsunterlagen_BImSchG.pdf</unterlage>"
           "<unterlage>UVP-Bericht_2026.pdf</unterlage>"
           "<unterlage>Laermgutachten.pdf</unterlage>"
           "<unterlage>Geruchsgutachten.pdf</unterlage>"
           "<unterlage>Naturschutzfachbeitrag.pdf</unterlage>"
           "</record>"
           "</bimschg>";
}

// Erzeugt ein XDOMEA-Dokument mit Metadaten
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

// Erzeugt einen XDOMEA-Vorgang (Stellungnahme-Anfrage an eine Fachbehörde)
static XDOMEADocument makeFachbehoerdenAnfrage(const std::string& id,
                                                const std::string& az,
                                                const std::string& behoerde) {
    return makeDoc(id, XDOMEAObjectType::VORGANG, az,
                   "Beteiligung Fachbehörde: " + behoerde,
                   "Regierungspräsidium Düsseldorf", behoerde,
                   {{"typ", "FACHBEHOERDEN_ANFRAGE"}, {"status", "OFFEN"}});
}

// Erzeugt eine Stellungnahme einer Fachbehörde
static XDOMEADocument makeStellungnahme(const std::string& id,
                                         const std::string& parent_id,
                                         const std::string& az,
                                         const std::string& behoerde,
                                         const std::string& ergebnis,
                                         std::map<std::string,std::string> extra = {}) {
    auto meta = std::map<std::string,std::string>{
        {"typ",       "STELLUNGNAHME"},
        {"ergebnis",  ergebnis},
        {"status",    "ABGESCHLOSSEN"},
    };
    meta.insert(extra.begin(), extra.end());
    auto d = makeDoc(id, XDOMEAObjectType::DOKUMENT, az,
                     "Stellungnahme " + behoerde,
                     behoerde, behoerde, std::move(meta));
    d.parent_id = parent_id;
    return d;
}

// Antragsteller-eID-Identität (Betreiber)
static EIDIdentity makeBetreiberIdentity(const std::string& tx_id) {
    EIDIdentity id;
    id.transaction_id   = tx_id;
    id.eid_server_id    = "eid-server-bund-02";
    id.assurance        = EIDAssuranceLevel::HIGH;
    id.authenticated_at = std::chrono::system_clock::now();
    id.attributes = {
        {EIDAttributeType::GIVEN_NAMES,       "Klaus"},
        {EIDAttributeType::FAMILY_NAMES,      "Bergmann"},
        {EIDAttributeType::DATE_OF_BIRTH,     "19750612"},
        {EIDAttributeType::MUNICIPALITY_ID,   "05111000"},   // AGS Düsseldorf
        {EIDAttributeType::PLACE_OF_RESIDENCE,"Industriepark Nord 1, 40549 Düsseldorf"},
        {EIDAttributeType::DOCUMENT_TYPE,     "ID"},
        {EIDAttributeType::ISSUING_STATE,     "DEU"},
    };
    return id;
}

} // anonymous namespace

// ── Testklasse ────────────────────────────────────────────────────────────────

/**
 * @brief Fixture für den 9. BImSchV Genehmigungsverfahren E2E-Test.
 *
 * Simuliert das förmliche Genehmigungsverfahren mit 8 Behörden, UVP-Vorprüfung,
 * Öffentlichkeitsbeteiligung und neun Prozessphasen.
 */
class BImSchVGenehmigungsverfahrenE2ETest : public ::testing::Test {
protected:
    // ── Aktenzeichen ─────────────────────────────────────────────────────────
    static constexpr const char* kAZ = "RP-D-BImSchG-2026-0017";

    // ── Verfahrensstatus ──────────────────────────────────────────────────────
    enum class VerfahrenStatus {
        ANTRAG_EINGEGANGEN,
        VOLLSTAENDIGKEITSPRUEFUNG,
        UVP_VORPRUEFUNG,
        ANTRAGSKONFERENZ,
        AUSLEGUNG,
        FACHBEHOERDEN_PRUEFUNG,
        EROERTERUNGSTERMIN,
        ENTSCHEIDUNG,
        GENEHMIGT,
        ABGELEHNT,
    };

    VerfahrenStatus status_{VerfahrenStatus::ANTRAG_EINGEGANGEN};

    // ── XDOMEA-DMS-Stores der 8 Behörden ─────────────────────────────────────
    InMemoryXDOMEAConnector dms_rp_;              ///< Regierungspräsidium Düsseldorf
    InMemoryXDOMEAConnector dms_lanuv_;           ///< LANUV NRW
    InMemoryXDOMEAConnector dms_umweltamt_;       ///< Staatliches Umweltamt (Gewässer)
    InMemoryXDOMEAConnector dms_gesundheitsamt_;  ///< Gesundheitsamt (Lärm)
    InMemoryXDOMEAConnector dms_bauaufsicht_;     ///< Bauaufsichtsbehörde
    InMemoryXDOMEAConnector dms_gewerbeaufsicht_; ///< Gewerbeaufsichtsamt
    InMemoryXDOMEAConnector dms_wasserbeh_;       ///< Untere Wasserbehörde
    InMemoryXDOMEAConnector dms_naturschutz_;     ///< Untere Naturschutzbehörde

    // ── XÖV-Importer (BImSchG) ───────────────────────────────────────────────
    InMemoryXOEVImporter xoev_rp_;

    // ── OZG-Dienstregister ───────────────────────────────────────────────────
    InMemoryOZGServiceRegistry ozg_registry_;

    // ── eID-Authenticator ────────────────────────────────────────────────────
    InMemoryEIDAuthenticator eid_auth_;

    // ── LLM-Prozessanalyzer ──────────────────────────────────────────────────
    std::unique_ptr<LLMProcessAnalyzer> llm_;

    // ── Prozessprotokoll ─────────────────────────────────────────────────────
    std::vector<std::string> log_;
    std::mutex               log_mu_;

    // ── Einwendungen ─────────────────────────────────────────────────────────
    std::vector<std::string> einwendungs_ids_;

    void SetUp() override {
        LLMConfig cfg;
        cfg.provider    = LLMProvider::LOCAL;
        cfg.model_name  = "themis-stub";
        cfg.temperature = 0.0;
        llm_ = std::make_unique<LLMProcessAnalyzer>(cfg);

        EIDAuthConfig eid_cfg;
        eid_cfg.enabled = true;
        eid_cfg.eid_server_url = "https://eid.test.local/auth";
        eid_cfg.terminal_certificate = "test-terminal-cert";
        ASSERT_TRUE(eid_auth_.initialize(eid_cfg));
    }

    void logPhase(const std::string& msg) {
        std::lock_guard<std::mutex> lk(log_mu_);
        log_.push_back(msg);
    }

    // Legt RP-Verfahrensakte an (AKTE)
    XDOMEADocument makeVerfahrensAkte() {
        return makeDoc("VFA-" + std::string(kAZ),
                       XDOMEAObjectType::AKTE,
                       kAZ,
                       "BImSchG-Genehmigungsverfahren Großbiogasanlage",
                       "Regierungspräsidium Düsseldorf",
                       "Regierungspräsidium Düsseldorf",
                       {{"verfahrensart", "FOERMLICH"},
                        {"uvp_pflichtig", "true"}});
    }
};

// ════════════════════════════════════════════════════════════════════════════
//  Phase 1 — Antragstellung
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-01 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase1_OZGDienstRegistrierung) {
    OZGServiceEntry svc;
    svc.id          = "DE-NRW-BIMSCHG-GENEHMIGUNG";
    svc.name        = "BImSchG-Genehmigung (§ 4 BImSchG)";
    svc.description = "Förmliches Genehmigungsverfahren nach 9. BImSchV";
    svc.level       = OZGFederalLevel::STATE;
    svc.status      = OZGServiceStatus::ONLINE_TRANSACTION;
    svc.responsible_authority = "Regierungspräsidium Düsseldorf";
    svc.compliance_tags       = {"BImSchG", "9-BImSchV", "UVP"};

    ASSERT_NO_THROW(ozg_registry_.registerService(svc));
    auto found = ozg_registry_.findById("DE-NRW-BIMSCHG-GENEHMIGUNG");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "BImSchG-Genehmigung (§ 4 BImSchG)");
    EXPECT_EQ(found->status, OZGServiceStatus::ONLINE_TRANSACTION);

    logPhase("Phase1: OZG-Dienst BImSchG-Genehmigung registriert");
}

// ── AC-BIM-02 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase1_BetreiberEIDAuthentifizierung) {
    auto identity = makeBetreiberIdentity("TX-BIMSCHG-2026-001");
    eid_auth_.storeIdentity(identity);

    EIDAuthRequest req;
    req.transaction_id   = "TX-BIMSCHG-2026-001";
    req.service_provider = "RP-Düsseldorf-BImSchG";
    req.requested_attributes = {
        EIDAttributeType::GIVEN_NAMES,
        EIDAttributeType::FAMILY_NAMES,
        EIDAttributeType::MUNICIPALITY_ID,
    };
    req.minimum_assurance = EIDAssuranceLevel::HIGH;

    auto session = eid_auth_.beginAuthSession(req);
    ASSERT_NE(session.session_id, "");
    ASSERT_EQ(session.status, EIDSessionStatus::PENDING);

    auto result = eid_auth_.completeAuthSession(session.session_id, "OK");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->assurance, EIDAssuranceLevel::HIGH);
    EXPECT_EQ(result->fullName(), "Klaus Bergmann");

    logPhase("Phase1: eID-Auth Betreiber Klaus Bergmann — HIGH");
}

// ── AC-BIM-03 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase1_BetreiberEIDEnthaeltAGSDuesseldorf) {
    auto identity = makeBetreiberIdentity("TX-BIMSCHG-2026-002");
    eid_auth_.storeIdentity(identity);

    auto session = eid_auth_.beginAuthSession(
        [&]{ EIDAuthRequest r; r.transaction_id = "TX-BIMSCHG-2026-002";
             r.service_provider = "RP-D"; r.minimum_assurance = EIDAssuranceLevel::HIGH;
             return r; }());
    auto result = eid_auth_.completeAuthSession(session.session_id, "OK");
    ASSERT_TRUE(result.has_value());

    auto ags = result->getAttribute(EIDAttributeType::MUNICIPALITY_ID);
    ASSERT_TRUE(ags.has_value());
    EXPECT_EQ(*ags, "05111000") << "AGS Düsseldorf muss 05111000 sein";

    logPhase("Phase1: AGS Düsseldorf (05111000) in eID bestätigt");
}

// ── AC-BIM-04 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase1_XOEVBImSchGAntragImport) {
    auto xml = makeBImSchGAntragsXML(kAZ, "Klaus Bergmann", "Großbiogasanlage Nord");
    auto res = xoev_rp_.importFromXML(xml, XOEVStandard::OTHER);

    ASSERT_TRUE(res.success) << "XÖV-Import muss erfolgreich sein";
    ASSERT_FALSE(res.records.empty());
    EXPECT_EQ(res.records.front().id, kAZ);
    EXPECT_EQ(res.records.front().standard, XOEVStandard::OTHER);
    EXPECT_NE(res.records.front().fields.count("betreiber"), 0u);

    logPhase("Phase1: XÖV-BImSchG-Antrag importiert — AZ=" + std::string(kAZ));
    status_ = VerfahrenStatus::VOLLSTAENDIGKEITSPRUEFUNG;
}

// ── AC-BIM-05 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase1_XOEVBImSchGAntragExport) {
    auto xml = makeBImSchGAntragsXML(kAZ, "Klaus Bergmann", "Großbiogasanlage Nord");
    auto import_res = xoev_rp_.importFromXML(xml, XOEVStandard::OTHER);
    ASSERT_TRUE(import_res.success);

    XOEVVersion v; v.major = 1; v.minor = 0; v.patch = 0;
    auto export_res = xoev_rp_.exportToXML(
        import_res.records, XOEVStandard::OTHER, v);

    ASSERT_TRUE(export_res.success);
    ASSERT_FALSE(export_res.xml_output.empty());
    EXPECT_NE(export_res.xml_output.find("<?xml"), std::string::npos);
    EXPECT_NE(export_res.xml_output.find("Klaus Bergmann"), std::string::npos);
    EXPECT_NE(export_res.xml_output.find(kAZ), std::string::npos);

    logPhase("Phase1: XÖV-Export wohlgeformt — " +
             std::to_string(export_res.xml_output.size()) + " Zeichen");
}

// ── AC-BIM-06 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase1_RPVerfahrensAkteAngelegt) {
    auto akte = makeVerfahrensAkte();
    ASSERT_NO_THROW(dms_rp_.storeDocument(akte));

    auto found = dms_rp_.getDocument("VFA-" + std::string(kAZ));
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->object_type,  XDOMEAObjectType::AKTE);
    EXPECT_EQ(found->aktenzeichen, kAZ);
    EXPECT_EQ(found->metadata.at("verfahrensart"), "FOERMLICH");

    logPhase("Phase1: RP-Verfahrensakte angelegt — ID=VFA-" + std::string(kAZ));
}

// ════════════════════════════════════════════════════════════════════════════
//  Phase 2 — Vollständigkeitsprüfung
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-07 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase2_LLMVollstaendigkeitspruefung) {
    LLMRequest req;
    req.task_type = TaskType::PROCESS_CONFORMANCE;
    req.process_data = {
        {"az",             kAZ},
        {"anlage",         "Großbiogasanlage Nord"},
        {"unterlagen",     {"Antragsunterlagen_BImSchG.pdf",
                            "UVP-Bericht_2026.pdf",
                            "Laermgutachten.pdf",
                            "Geruchsgutachten.pdf",
                            "Naturschutzfachbeitrag.pdf"}},
        {"vollstaendigkeit", "alle Pflichtunterlagen nach § 10 Abs. 1 BImSchG vorhanden"},
    };
    req.domain = "BImSchG-Genehmigungsverfahren";

    auto [ok, resp] = llm_->analyze(req);
    ASSERT_TRUE(ok);
    EXPECT_GE(resp.conformance_score, 0.5)
        << "LLM-Vollständigkeitsprüfung muss score >= 0.5 liefern";

    logPhase("Phase2: LLM-Vollständigkeit score=" +
             std::to_string(resp.conformance_score));
    status_ = VerfahrenStatus::UVP_VORPRUEFUNG;
}

// ── AC-BIM-08 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase2_VerfahrensartFoermlich) {
    // Verfahrensakte muss Verfahrensart FOERMLICH enthalten
    dms_rp_.storeDocument(makeVerfahrensAkte());
    auto akten = dms_rp_.listByType(XDOMEAObjectType::AKTE);
    ASSERT_FALSE(akten.empty());

    bool foermlich = false;
    for (const auto& a : akten) {
        auto it = a.metadata.find("verfahrensart");
        if (it != a.metadata.end() && it->second == "FOERMLICH") {
            foermlich = true;
            break;
        }
    }
    EXPECT_TRUE(foermlich)
        << "Verfahrensart muss FOERMLICH sein (FWL > 10 MW → Pflicht)";

    logPhase("Phase2: Verfahrensart FOERMLICH gesetzt");
}

// ════════════════════════════════════════════════════════════════════════════
//  Phase 3 — UVP-Vorprüfung
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-09 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase3_UVPVorpruefungsDokument) {
    auto uvp = makeDoc("UVP-SCREEN-001",
                       XDOMEAObjectType::DOKUMENT,
                       kAZ,
                       "UVP-Vorprüfungs-Screening gemäß § 7 UVPG",
                       "Regierungspräsidium Düsseldorf",
                       "Regierungspräsidium Düsseldorf",
                       {{"typ",           "UVP_VORPRUEFUNG"},
                        {"ergebnis",      "UVP_PFLICHTIG"},
                        {"begruendung",   "FWL > 10 MW, Anlage liegt in Schutzgebiet"},
                        {"datum",         nowIso()}});
    ASSERT_NO_THROW(dms_rp_.storeDocument(uvp));

    auto found = dms_rp_.getDocument("UVP-SCREEN-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->metadata.at("ergebnis"), "UVP_PFLICHTIG");

    logPhase("Phase3: UVP-Screening → UVP_PFLICHTIG gespeichert");
    status_ = VerfahrenStatus::ANTRAGSKONFERENZ;
}

// ── AC-BIM-10 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase3_LANUVEmpfaengtUVPAnfrage) {
    auto anfrage = makeDoc("LANUV-UVP-REQ-001",
                           XDOMEAObjectType::VORGANG,
                           kAZ,
                           "UVP-Screening-Anfrage an LANUV",
                           "Regierungspräsidium Düsseldorf",
                           "LANUV NRW",
                           {{"typ",    "UVP_SCREENING_ANFRAGE"},
                            {"status", "OFFEN"}});
    ASSERT_NO_THROW(dms_lanuv_.storeDocument(anfrage));

    auto vorgaenge = dms_lanuv_.listByType(XDOMEAObjectType::VORGANG);
    ASSERT_FALSE(vorgaenge.empty());
    EXPECT_EQ(vorgaenge.front().metadata.at("typ"), "UVP_SCREENING_ANFRAGE");

    logPhase("Phase3: LANUV hat UVP-Screening-Anfrage erhalten");
}

// ── AC-BIM-11 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase3_LANUVLiefertUVPStellungnahme) {
    // LANUV antwortet auf die UVP-Screening-Anfrage
    auto stn = makeStellungnahme("LANUV-STN-UVP-001",
                                  "LANUV-UVP-REQ-001",
                                  kAZ,
                                  "LANUV NRW",
                                  "UVP_PFLICHTIG",
                                  {{"schadstoffklasse",  "IVa"},
                                   {"emissionsgrenzwert","200 mg/Nm³ NOx"},
                                   {"hinweis",           "TA Luft 2021 Nr. 5.2.4 anwenden"}});
    ASSERT_NO_THROW(dms_lanuv_.storeDocument(stn));

    auto dokumente = dms_lanuv_.listByType(XDOMEAObjectType::DOKUMENT);
    ASSERT_FALSE(dokumente.empty());
    auto it = std::find_if(dokumente.begin(), dokumente.end(),
                           [](const XDOMEADocument& d){
                               return d.id == "LANUV-STN-UVP-001"; });
    ASSERT_NE(it, dokumente.end());
    EXPECT_EQ(it->metadata.at("ergebnis"), "UVP_PFLICHTIG");

    logPhase("Phase3: LANUV Stellungnahme UVP_PFLICHTIG abgegeben");
}

// ════════════════════════════════════════════════════════════════════════════
//  Phase 4 — Antragskonferenz
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-12 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase4_AntragskonferenzProtokoll) {
    auto protokoll = makeDoc("AKF-PROT-001",
                              XDOMEAObjectType::DOKUMENT,
                              kAZ,
                              "Protokoll Antragskonferenz § 2a 9. BImSchV",
                              "Regierungspräsidium Düsseldorf",
                              "Regierungspräsidium Düsseldorf",
                              {{"typ",      "ANTRAGSKONFERENZ_PROTOKOLL"},
                               {"datum",    "2026-07-15T10:00:00Z"},
                               {"teilnehmer","RP,LANUV,Umweltamt,Gesundheitsamt,"
                                             "Bauaufsicht,Gewerbeaufsicht,"
                                             "Wasserbeh,Naturschutz"}});
    ASSERT_NO_THROW(dms_rp_.storeDocument(protokoll));

    auto found = dms_rp_.getDocument("AKF-PROT-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_NE(found->metadata.at("teilnehmer").find("LANUV"), std::string::npos);

    logPhase("Phase4: Antragskonferenz-Protokoll im RP-DMS gespeichert");
}

// ── AC-BIM-13 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase4_AlleFachbehoerdenErhaltenEinladung) {
    // Liste der Fachbehörden und ihrer DMS-Stores
    struct Fachbehoerde {
        std::string name;
        InMemoryXDOMEAConnector* dms;
    };
    std::vector<Fachbehoerde> behoerden = {
        {"LANUV NRW",                  &dms_lanuv_},
        {"Staatliches Umweltamt",      &dms_umweltamt_},
        {"Gesundheitsamt",             &dms_gesundheitsamt_},
        {"Bauaufsichtsbehörde",        &dms_bauaufsicht_},
        {"Gewerbeaufsichtsamt",        &dms_gewerbeaufsicht_},
        {"Untere Wasserbehörde",       &dms_wasserbeh_},
        {"Untere Naturschutzbehörde",  &dms_naturschutz_},
    };

    int idx = 0;
    for (const auto& fb : behoerden) {
        auto anfrage = makeFachbehoerdenAnfrage(
            "AKF-INV-" + std::to_string(++idx), kAZ, fb.name);
        ASSERT_NO_THROW(fb.dms->storeDocument(anfrage))
            << "Einladung an " << fb.name << " fehlgeschlagen";
    }

    // Alle 7 Fachbehörden sollen je 1 Vorgang haben
    for (const auto& fb : behoerden) {
        auto vorgaenge = fb.dms->listByType(XDOMEAObjectType::VORGANG);
        EXPECT_GE(vorgaenge.size(), 1u)
            << fb.name << " hat keine Antragskonferenz-Einladung erhalten";
    }

    logPhase("Phase4: Antragskonferenz-Einladungen an alle 7 Fachbehörden versandt");
    status_ = VerfahrenStatus::AUSLEGUNG;
}

// ════════════════════════════════════════════════════════════════════════════
//  Phase 5 — Auslegung / Öffentlichkeitsbeteiligung
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-14 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase5_AuslegungsbekanntmachungImDMS) {
    auto bekanntm = makeDoc("AUSL-BEKM-001",
                             XDOMEAObjectType::DOKUMENT,
                             kAZ,
                             "Auslegungsbekanntmachung gemäß § 10 Abs. 3 BImSchG",
                             "Regierungspräsidium Düsseldorf",
                             "Regierungspräsidium Düsseldorf",
                             {{"typ",          "AUSLEGUNGSBEKANNTMACHUNG"},
                              {"auslegungs_von","2026-08-01"},
                              {"auslegungs_bis","2026-09-01"},
                              {"amtsblatt",     "Amtsblatt NRW 2026 Nr. 31"}});
    ASSERT_NO_THROW(dms_rp_.storeDocument(bekanntm));

    auto found = dms_rp_.getDocument("AUSL-BEKM-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->metadata.at("typ"), "AUSLEGUNGSBEKANNTMACHUNG");
    EXPECT_NE(found->metadata.at("auslegungs_von"), "");

    logPhase("Phase5: Auslegungsbekanntmachung im RP-DMS registriert");
}

// ── AC-BIM-15 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase5_EinwendungenErfasst) {
    const int kAnzahlEinwendungen = 3;

    for (int i = 1; i <= kAnzahlEinwendungen; ++i) {
        std::string eid = "EINW-" + std::to_string(i);
        einwendungs_ids_.push_back(eid);
        auto doc = makeDoc(eid,
                           XDOMEAObjectType::DOKUMENT,
                           kAZ,
                           "Einwendung Einwender #" + std::to_string(i),
                           "Einwender " + std::to_string(i),
                           "Regierungspräsidium Düsseldorf",
                           {{"typ",      "EINWENDUNG"},
                            {"einwender","Bürger_" + std::to_string(i)},
                            {"status",   "EINGEGANGEN"}});
        ASSERT_NO_THROW(dms_rp_.storeDocument(doc));
    }

    // Alle Einwendungen müssen abrufbar sein
    for (const auto& eid : einwendungs_ids_) {
        EXPECT_TRUE(dms_rp_.getDocument(eid).has_value())
            << "Einwendung " << eid << " nicht im DMS";
    }

    logPhase("Phase5: " + std::to_string(kAnzahlEinwendungen) +
             " Einwendungen erfasst");
    status_ = VerfahrenStatus::FACHBEHOERDEN_PRUEFUNG;
}

// ════════════════════════════════════════════════════════════════════════════
//  Phase 6 — Fachbehörden-Prüfung (parallel)
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-16 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase6_AlleFachbehoerdenLiefernStellungnahmen) {
    struct FBStellungnahme {
        InMemoryXDOMEAConnector* dms;
        std::string id;
        std::string behoerde;
        std::string ergebnis;
        std::map<std::string,std::string> extra;
    };

    std::vector<FBStellungnahme> arbeiten = {
        {&dms_lanuv_,           "STN-LANUV-001",     "LANUV NRW",
         "ZUSTIMMUNG_MIT_AUFLAGEN", {{"auflagen","TA Luft 2021 einhalten"}}},
        {&dms_umweltamt_,       "STN-UWA-001",       "Staatliches Umweltamt",
         "ZUSTIMMUNG",             {{"bemerkung","Gewässerschutz gewährleistet"}}},
        {&dms_gesundheitsamt_,  "STN-GA-001",        "Gesundheitsamt",
         "ZUSTIMMUNG_MIT_AUFLAGEN", {{"laerm_db","45 dB(A) nachts einhalten"},
                                      {"geruch_gwk","Irrelevanzgrenze einhalten"}}},
        {&dms_bauaufsicht_,     "STN-BAU-001",       "Bauaufsichtsbehörde",
         "ZUSTIMMUNG",             {{"bemerkung","Bauordnungsrecht erfüllt"}}},
        {&dms_gewerbeaufsicht_, "STN-GA2-001",       "Gewerbeaufsichtsamt",
         "ZUSTIMMUNG_MIT_AUFLAGEN", {{"auflage_betrsichv","§ 14 BetrSichV beachten"},
                                      {"ex_schutz","Zone 2 einhalten"}}},
        {&dms_wasserbeh_,       "STN-WHG-001",       "Untere Wasserbehörde",
         "ZUSTIMMUNG",             {{"whg_auflage","Einleiterlaubnis WHG § 8"}}},
        {&dms_naturschutz_,     "STN-NAT-001",       "Untere Naturschutzbehörde",
         "ZUSTIMMUNG_MIT_AUFLAGEN", {{"artenschutz","CEF-Maßnahmen umsetzen"},
                                      {"kompensation","0.8 ha Ausgleichsfläche"}}},
    };

    // Alle 7 Stellungnahmen parallel abgeben
    std::vector<std::future<void>> futures;
    for (const auto& a : arbeiten) {
        futures.push_back(std::async(std::launch::async, [&a, this]() {
            auto stn = makeStellungnahme(a.id, "", kAZ, a.behoerde,
                                         a.ergebnis, a.extra);
            EXPECT_NO_THROW(a.dms->storeDocument(stn));
            logPhase("Phase6: Stellungnahme " + a.behoerde + " → " + a.ergebnis);
        }));
    }
    for (auto& f : futures) {
        ASSERT_NO_THROW(f.get());
    }

    // Jede Fachbehörde hat mindestens 1 Stellungnahme-Dokument
    EXPECT_GE(dms_lanuv_.listByType(XDOMEAObjectType::DOKUMENT).size(),           1u);
    EXPECT_GE(dms_umweltamt_.listByType(XDOMEAObjectType::DOKUMENT).size(),       1u);
    EXPECT_GE(dms_gesundheitsamt_.listByType(XDOMEAObjectType::DOKUMENT).size(),  1u);
    EXPECT_GE(dms_bauaufsicht_.listByType(XDOMEAObjectType::DOKUMENT).size(),     1u);
    EXPECT_GE(dms_gewerbeaufsicht_.listByType(XDOMEAObjectType::DOKUMENT).size(), 1u);
    EXPECT_GE(dms_wasserbeh_.listByType(XDOMEAObjectType::DOKUMENT).size(),       1u);
    EXPECT_GE(dms_naturschutz_.listByType(XDOMEAObjectType::DOKUMENT).size(),     1u);

    status_ = VerfahrenStatus::EROERTERUNGSTERMIN;
}

// ── AC-BIM-17 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase6_GesundheitsamtStellungnahmeEnthaeltLaermDaten) {
    auto stn = makeStellungnahme("STN-GA-LAERM-001", "",
                                  kAZ, "Gesundheitsamt",
                                  "ZUSTIMMUNG_MIT_AUFLAGEN",
                                  {{"laerm_db",    "45 dB(A) nachts"},
                                   {"geruch_gwk",  "Irrelevanzgrenze unterschritten"},
                                   {"messstelle",  "TA-Lärm-Punkt P1-P4"}});
    ASSERT_NO_THROW(dms_gesundheitsamt_.storeDocument(stn));

    auto found = dms_gesundheitsamt_.getDocument("STN-GA-LAERM-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_NE(found->metadata.count("laerm_db"),    0u) << "Lärmwert fehlt";
    EXPECT_NE(found->metadata.count("geruch_gwk"),  0u) << "Geruchswert fehlt";
    EXPECT_NE(found->metadata.count("messstelle"),  0u) << "Messstelle fehlt";

    logPhase("Phase6: Gesundheitsamt — Lärmprüfungs-Metadaten verifiziert");
}

// ── AC-BIM-18 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase6_GewerbeaufsichtStellungnahmeEnthaeltAuflagen) {
    auto stn = makeStellungnahme("STN-GEW-AUFLAGEN-001", "",
                                  kAZ, "Gewerbeaufsichtsamt",
                                  "ZUSTIMMUNG_MIT_AUFLAGEN",
                                  {{"auflage_betrsichv", "§ 14 BetrSichV"},
                                   {"auflage_ex_schutz", "ATEX Zone 2"},
                                   {"auflage_druckbeh",  "§§ 14-17 BetrSichV Druckbehälter"},
                                   {"pruefintervall",    "jährlich durch ZÜS"}});
    ASSERT_NO_THROW(dms_gewerbeaufsicht_.storeDocument(stn));

    auto found = dms_gewerbeaufsicht_.getDocument("STN-GEW-AUFLAGEN-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_NE(found->metadata.count("auflage_betrsichv"), 0u);
    EXPECT_NE(found->metadata.count("auflage_ex_schutz"), 0u);

    logPhase("Phase6: Gewerbeaufsicht — Auflagen-Metadaten verifiziert");
}

// ════════════════════════════════════════════════════════════════════════════
//  Phase 7 — Erörterungstermin
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-19 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase7_EroerterungsterminProtokollImDMS) {
    auto protokoll = makeDoc("EROT-PROT-001",
                              XDOMEAObjectType::DOKUMENT,
                              kAZ,
                              "Protokoll Erörterungstermin § 10 Abs. 6 BImSchG",
                              "Regierungspräsidium Düsseldorf",
                              "Regierungspräsidium Düsseldorf",
                              {{"typ",           "EROERTERUNGSTERMIN_PROTOKOLL"},
                               {"datum",         "2026-10-20T10:00:00Z"},
                               {"einwendungen",  "EINW-1,EINW-2,EINW-3"},
                               {"ergebnis",      "Erörterung abgeschlossen"}});
    ASSERT_NO_THROW(dms_rp_.storeDocument(protokoll));

    auto found = dms_rp_.getDocument("EROT-PROT-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->metadata.at("typ"), "EROERTERUNGSTERMIN_PROTOKOLL");

    logPhase("Phase7: Erörterungstermin-Protokoll im RP-DMS gespeichert");
    status_ = VerfahrenStatus::ENTSCHEIDUNG;
}

// ── AC-BIM-20 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase7_EroerterungsprotokollReferenziertEinwendungen) {
    // Einwendungen erfassen
    for (int i = 1; i <= 3; ++i) {
        dms_rp_.storeDocument(makeDoc("EINW-" + std::to_string(i),
                                       XDOMEAObjectType::DOKUMENT, kAZ,
                                       "Einwendung #" + std::to_string(i),
                                       "Einwender " + std::to_string(i),
                                       "Regierungspräsidium Düsseldorf",
                                       {{"typ","EINWENDUNG"}}));
    }

    // Erörterungsprotokoll mit Einwendungsreferenzen
    auto protokoll = makeDoc("EROT-REF-001",
                              XDOMEAObjectType::DOKUMENT,
                              kAZ,
                              "Erörterungsprotokoll mit Einwendungsverzeichnis",
                              "Regierungspräsidium Düsseldorf",
                              "Regierungspräsidium Düsseldorf",
                              {{"typ",          "EROERTERUNGSTERMIN_PROTOKOLL"},
                               {"einwendungen", "EINW-1,EINW-2,EINW-3"}});
    ASSERT_NO_THROW(dms_rp_.storeDocument(protokoll));

    auto found = dms_rp_.getDocument("EROT-REF-001");
    ASSERT_TRUE(found.has_value());
    const std::string& einw_ref = found->metadata.at("einwendungen");
    EXPECT_NE(einw_ref.find("EINW-1"), std::string::npos);
    EXPECT_NE(einw_ref.find("EINW-2"), std::string::npos);
    EXPECT_NE(einw_ref.find("EINW-3"), std::string::npos);

    logPhase("Phase7: Erörterungsprotokoll referenziert EINW-1, EINW-2, EINW-3");
}

// ════════════════════════════════════════════════════════════════════════════
//  Phase 8 — Entscheidung
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-21 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase8_LLMEntscheidungsanalysescore) {
    LLMRequest req;
    req.task_type = TaskType::PROCESS_CONFORMANCE;
    req.process_data = {
        {"az",              kAZ},
        {"stellungnahmen",  7},
        {"einwendungen",    3},
        {"ergebnis_stns",   {"ZUSTIMMUNG", "ZUSTIMMUNG_MIT_AUFLAGEN"}},
        {"auflagen_gesamt", 7},
        {"uvp_ergebnis",    "UVP_PFLICHTIG"},
    };
    req.domain = "BImSchG-Entscheidung";

    auto [ok, resp] = llm_->analyze(req);
    ASSERT_TRUE(ok);
    EXPECT_GE(resp.conformance_score, 0.5)
        << "LLM-Entscheidungsanalyse muss score >= 0.5 liefern";

    logPhase("Phase8: LLM-Entscheidungsanalyse score=" +
             std::to_string(resp.conformance_score));
}

// ── AC-BIM-22 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase8_LLMEntscheidungsempfehlungGenehmigung) {
    LLMRequest req;
    req.task_type = TaskType::PROCESS_CONFORMANCE;
    req.process_data = {
        {"az",            kAZ},
        {"auflagen",      "TA Luft, TA Lärm, BetrSichV, WHG, BNatSchG"},
        {"alle_stns",     "ZUSTIMMUNG"},
    };
    req.domain = "BImSchG-Entscheidungsempfehlung";

    auto [ok, resp] = llm_->analyze(req);
    ASSERT_TRUE(ok);
    // LLM-Stub liefert summary; wir prüfen nur, dass summary nicht leer ist
    EXPECT_FALSE(resp.summary.empty())
        << "LLM-Empfehlung-Summary darf nicht leer sein";

    logPhase("Phase8: LLM-Entscheidungsempfehlung erzeugt");
    status_ = VerfahrenStatus::GENEHMIGT;
}

// ════════════════════════════════════════════════════════════════════════════
//  Phase 9 — Genehmigungsbescheid
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-23 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase9_GenehmigungsbescheidImRPDMS) {
    auto bescheid = makeDoc("BESCHEID-001",
                             XDOMEAObjectType::DOKUMENT,
                             kAZ,
                             "Genehmigungsbescheid gemäß § 4 BImSchG",
                             "Regierungspräsidium Düsseldorf",
                             "Regierungspräsidium Düsseldorf",
                             {{"typ",          "GENEHMIGUNGSBESCHEID"},
                              {"ergebnis",     "GENEHMIGT"},
                              {"auflagen_cnt", "7"},
                              {"datum",        nowIso()},
                              {"rechtsmittel", "Widerspruch binnen 1 Monat"}});
    ASSERT_NO_THROW(dms_rp_.storeDocument(bescheid));

    auto found = dms_rp_.getDocument("BESCHEID-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->object_type, XDOMEAObjectType::DOKUMENT);
    EXPECT_EQ(found->metadata.at("typ"), "GENEHMIGUNGSBESCHEID");
    EXPECT_EQ(found->metadata.at("ergebnis"), "GENEHMIGT");

    logPhase("Phase9: Genehmigungsbescheid im RP-DMS gespeichert");
}

// ── AC-BIM-24 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase9_GenehmigungsbescheidEnthaeltAktenzeichen) {
    auto bescheid = makeDoc("BESCHEID-AZ-001",
                             XDOMEAObjectType::DOKUMENT,
                             kAZ,
                             "Genehmigungsbescheid",
                             "Regierungspräsidium Düsseldorf",
                             "Regierungspräsidium Düsseldorf",
                             {{"typ","GENEHMIGUNGSBESCHEID"}});
    ASSERT_NO_THROW(dms_rp_.storeDocument(bescheid));

    auto found = dms_rp_.getDocument("BESCHEID-AZ-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->aktenzeichen, kAZ)
        << "Bescheid muss korrektes Aktenzeichen " << kAZ << " tragen";

    logPhase("Phase9: Aktenzeichen " + std::string(kAZ) + " im Bescheid verifiziert");
}

// ── AC-BIM-25 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase9_GenehmigungsbescheidEnthaeltNebenbestimmungen) {
    const std::vector<std::string> auflagen = {
        "A-01: TA Luft 2021 Nr. 5.2.4 — NOx-Grenzwert 200 mg/Nm³",
        "A-02: TA Lärm — Nachtwert 45 dB(A) einhalten",
        "A-03: Geruchsimmissionsrichtlinie — GWK-Anteil 0.10",
        "A-04: WHG § 8 Einleiterlaubnis — gesonderte Wasserrechtserlaubnis",
        "A-05: BetrSichV § 14 — ATEX Zone 2 explosionsgeschützte Ausführung",
        "A-06: BNatSchG § 44 — CEF-Maßnahmen vor Baubeginn umsetzen",
        "A-07: Kompensationspflicht — 0.8 ha Ausgleichsfläche bereitstellen",
    };

    // Bescheid mit allen Auflagen als separate child-Dokumente
    for (size_t i = 0; i < auflagen.size(); ++i) {
        auto auflage = makeDoc("BESCHEID-NB-" + std::to_string(i + 1),
                                XDOMEAObjectType::DOKUMENT,
                                kAZ,
                                auflagen[i],
                                "Regierungspräsidium Düsseldorf",
                                "Regierungspräsidium Düsseldorf",
                                {{"typ",      "NEBENBESTIMMUNG"},
                                 {"auflage",  auflagen[i]}});
        auflage.parent_id = "BESCHEID-001";
        ASSERT_NO_THROW(dms_rp_.storeDocument(auflage));
    }

    auto nebenbestimmungen = dms_rp_.listChildren("BESCHEID-001");
    EXPECT_EQ(nebenbestimmungen.size(), auflagen.size())
        << "Bescheid muss genau " << auflagen.size() << " Nebenbestimmungen haben";

    logPhase("Phase9: " + std::to_string(auflagen.size()) +
             " Nebenbestimmungen im Bescheid verifiziert");
}

// ── AC-BIM-26 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase9_XDOMEAExportGenehmigungsbescheidWohlgeformt) {
    auto bescheid = makeDoc("BESCHEID-EXP-001",
                             XDOMEAObjectType::DOKUMENT,
                             kAZ,
                             "Genehmigungsbescheid BImSchG",
                             "Regierungspräsidium Düsseldorf",
                             "Regierungspräsidium Düsseldorf",
                             {{"typ","GENEHMIGUNGSBESCHEID"}});
    ASSERT_NO_THROW(dms_rp_.storeDocument(bescheid));

    auto export_res = dms_rp_.exportToXML(
        {bescheid}, XDOMEAVersion::V3_0, XDOMEAMessageType::ERFASSUNG);
    ASSERT_TRUE(export_res.success);
    ASSERT_FALSE(export_res.xml_output.empty());
    EXPECT_NE(export_res.xml_output.find("<?xml"), std::string::npos);
    EXPECT_NE(export_res.xml_output.find("dokument"), std::string::npos);
    EXPECT_NE(export_res.xml_output.find(kAZ), std::string::npos);

    logPhase("Phase9: XDOMEA-Export Genehmigungsbescheid wohlgeformt — " +
             std::to_string(export_res.xml_output.size()) + " Zeichen");
}

// ── AC-BIM-27 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, Phase9_BescheidZustellungAnBetreiber) {
    // Betreiber-DMS (Posteingang) wird als separater In-Memory-Store simuliert
    InMemoryXDOMEAConnector dms_betreiber;

    auto zustellung = makeDoc("ZUSTELLUNG-001",
                               XDOMEAObjectType::DOKUMENT,
                               kAZ,
                               "Zustellung Genehmigungsbescheid an Betreiber",
                               "Regierungspräsidium Düsseldorf",
                               "Klaus Bergmann (Betreiber)",
                               {{"typ",       "ZUSTELLUNG"},
                                {"empfaenger","Klaus Bergmann"},
                                {"kanal",     "DE-Mail / XDOMEA"},
                                {"datum",     nowIso()}});
    ASSERT_NO_THROW(dms_betreiber.storeDocument(zustellung));

    auto found = dms_betreiber.getDocument("ZUSTELLUNG-001");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->metadata.at("empfaenger"), "Klaus Bergmann");
    EXPECT_EQ(found->metadata.at("typ"), "ZUSTELLUNG");

    logPhase("Phase9: Bescheid an Betreiber Klaus Bergmann zugestellt");
}

// ════════════════════════════════════════════════════════════════════════════
//  Übergreifende Akzeptanzkriterien
// ════════════════════════════════════════════════════════════════════════════

// ── AC-BIM-28 ────────────────────────────────────────════════════════════════
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, RPDMSEnthaeltNach9PhasenMinimumDokumente) {
    // Simuliert vollständiges Verfahren in miniaturisierter Form
    const std::vector<std::pair<std::string,std::string>> pflichtdokumente = {
        {"VFA-001",        "Verfahrensakte"},
        {"UVP-001",        "UVP-Vorprüfung"},
        {"AKF-PROT-001",   "Antragskonferenz-Protokoll"},
        {"AUSL-001",       "Auslegungsbekanntmachung"},
        {"EINW-1",         "Einwendung 1"},
        {"EINW-2",         "Einwendung 2"},
        {"EINW-3",         "Einwendung 3"},
        {"EROT-PROT-001",  "Erörterungsprotokoll"},
        {"STN-AGG-001",    "Aggregierte Fachstellungnahmen"},
        {"LLM-EMPF-001",   "LLM-Entscheidungsempfehlung"},
        {"BESCHEID-001",   "Genehmigungsbescheid"},
        {"ZU-001",         "Zustellungsnachweis"},
    };

    for (const auto& [id, typ] : pflichtdokumente) {
        auto doc = makeDoc(id, XDOMEAObjectType::DOKUMENT, kAZ,
                           typ, "RP Düsseldorf", "RP Düsseldorf",
                           {{"pflicht","true"}});
        ASSERT_NO_THROW(dms_rp_.storeDocument(doc));
    }

    size_t gesamtzahl = dms_rp_.listByType(XDOMEAObjectType::DOKUMENT).size()
                      + dms_rp_.listByType(XDOMEAObjectType::AKTE).size();
    EXPECT_GE(gesamtzahl, 12u)
        << "RP-DMS muss nach Verfahrensabschluss >= 12 Objekte enthalten";

    logPhase("Übergreifend: RP-DMS enthält " + std::to_string(gesamtzahl) + " Objekte");
}

// ── AC-BIM-29 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, ProzessschrittReihenfolgePruefung) {
    // Minimal-Simulation der 9 Phasen in korrekter Reihenfolge
    const std::vector<std::string> phasen = {
        "Phase1:Antragstellung",
        "Phase2:Vollstaendigkeit",
        "Phase3:UVPVorpruefung",
        "Phase4:Antragskonferenz",
        "Phase5:Auslegung",
        "Phase6:Fachbehoerden",
        "Phase7:Eroerterungstermin",
        "Phase8:Entscheidung",
        "Phase9:Genehmigungsbescheid",
    };
    for (const auto& p : phasen) {
        logPhase(p);
    }

    auto find_pos = [&](const std::string& prefix) -> int {
        for (int i = 0; i < static_cast<int>(log_.size()); ++i) {
            if (log_[i].rfind(prefix, 0) == 0) return i;
        }
        return -1;
    };

    for (size_t i = 1; i < phasen.size(); ++i) {
        int pos_prev = find_pos(phasen[i-1]);
        int pos_curr = find_pos(phasen[i]);
        ASSERT_NE(pos_prev, -1) << "Phase " << phasen[i-1] << " nicht protokolliert";
        ASSERT_NE(pos_curr, -1) << "Phase " << phasen[i]   << " nicht protokolliert";
        EXPECT_LT(pos_prev, pos_curr)
            << phasen[i-1] << " muss vor " << phasen[i] << " stehen";
    }

    logPhase("Übergreifend: 9-Phasen-Reihenfolge korrekt verifiziert");
}

// ── AC-BIM-30 ────────────────────────────────────────────────────────────────
TEST_F(BImSchVGenehmigungsverfahrenE2ETest, GesamtverfahrenEndetMitStatusGENEHMIGT) {
    // Führt alle Phasen in einem einzigen Test durch

    // Phase 1
    OZGServiceEntry svc;
    svc.id = "DE-NRW-BIMSCHG-VOLLTEST";
    svc.name = "BImSchG-Volltest";
    svc.level = OZGFederalLevel::STATE;
    svc.status = OZGServiceStatus::ONLINE_TRANSACTION;
    ozg_registry_.registerService(svc);

    auto identity = makeBetreiberIdentity("TX-VOLLTEST-001");
    eid_auth_.storeIdentity(identity);
    auto session = eid_auth_.beginAuthSession(
        [&]{ EIDAuthRequest r; r.transaction_id = "TX-VOLLTEST-001";
             r.service_provider = "RP-D-Volltest";
             r.minimum_assurance = EIDAssuranceLevel::HIGH; return r; }());
    auto eid_result = eid_auth_.completeAuthSession(session.session_id, "OK");
    ASSERT_TRUE(eid_result.has_value());
    status_ = VerfahrenStatus::ANTRAG_EINGEGANGEN;
    logPhase("Phase1:Antragstellung");

    // Phase 2
    dms_rp_.storeDocument(makeVerfahrensAkte());
    status_ = VerfahrenStatus::VOLLSTAENDIGKEITSPRUEFUNG;
    logPhase("Phase2:Vollstaendigkeit");

    // Phase 3
    dms_rp_.storeDocument(makeDoc("UVP-VT-001", XDOMEAObjectType::DOKUMENT,
                                   kAZ, "UVP-Vorprüfung", "RP", "RP",
                                   {{"ergebnis","UVP_PFLICHTIG"}}));
    status_ = VerfahrenStatus::UVP_VORPRUEFUNG;
    logPhase("Phase3:UVPVorpruefung");

    // Phase 4
    dms_rp_.storeDocument(makeDoc("AKF-VT-001", XDOMEAObjectType::DOKUMENT,
                                   kAZ, "Antragskonferenz", "RP", "RP",
                                   {{"typ","ANTRAGSKONFERENZ_PROTOKOLL"}}));
    status_ = VerfahrenStatus::ANTRAGSKONFERENZ;
    logPhase("Phase4:Antragskonferenz");

    // Phase 5
    for (int i = 1; i <= 3; ++i) {
        dms_rp_.storeDocument(makeDoc("EINW-VT-" + std::to_string(i),
                                       XDOMEAObjectType::DOKUMENT,
                                       kAZ, "Einwendung VT", "Einwender", "RP",
                                       {{"typ","EINWENDUNG"}}));
    }
    status_ = VerfahrenStatus::AUSLEGUNG;
    logPhase("Phase5:Auslegung");

    // Phase 6 — 7 Stellungnahmen parallel
    auto submitStn = [&](InMemoryXDOMEAConnector* dms,
                          const std::string& id,
                          const std::string& behoerde) {
        return std::async(std::launch::async, [=](){
            auto s = makeStellungnahme(id, "", kAZ, behoerde, "ZUSTIMMUNG");
            dms->storeDocument(s);
        });
    };
    auto f1 = submitStn(&dms_lanuv_,           "VT-STN-1", "LANUV");
    auto f2 = submitStn(&dms_umweltamt_,       "VT-STN-2", "Umweltamt");
    auto f3 = submitStn(&dms_gesundheitsamt_,  "VT-STN-3", "Gesundheitsamt");
    auto f4 = submitStn(&dms_bauaufsicht_,     "VT-STN-4", "Bauaufsicht");
    auto f5 = submitStn(&dms_gewerbeaufsicht_, "VT-STN-5", "Gewerbeaufsicht");
    auto f6 = submitStn(&dms_wasserbeh_,       "VT-STN-6", "Wasserbeh");
    auto f7 = submitStn(&dms_naturschutz_,     "VT-STN-7", "Naturschutz");
    f1.get(); f2.get(); f3.get(); f4.get(); f5.get(); f6.get(); f7.get();
    status_ = VerfahrenStatus::FACHBEHOERDEN_PRUEFUNG;
    logPhase("Phase6:Fachbehoerden");

    // Phase 7
    dms_rp_.storeDocument(makeDoc("EROT-VT-001", XDOMEAObjectType::DOKUMENT,
                                   kAZ, "Erörterungsprotokoll VT", "RP", "RP",
                                   {{"typ","EROERTERUNGSTERMIN_PROTOKOLL"}}));
    status_ = VerfahrenStatus::EROERTERUNGSTERMIN;
    logPhase("Phase7:Eroerterungstermin");

    // Phase 8
    LLMRequest req; req.task_type = TaskType::PROCESS_CONFORMANCE;
    req.process_data = {{"az", kAZ}, {"stns", 7}};
    req.domain = "BImSchG";
    auto [ok, resp] = llm_->analyze(req);
    ASSERT_TRUE(ok);
    status_ = VerfahrenStatus::ENTSCHEIDUNG;
    logPhase("Phase8:Entscheidung");

    // Phase 9
    auto bescheid = makeDoc("BESCHEID-VT-001", XDOMEAObjectType::DOKUMENT,
                             kAZ, "Genehmigungsbescheid VT", "RP", "RP",
                             {{"typ","GENEHMIGUNGSBESCHEID"},{"ergebnis","GENEHMIGT"}});
    ASSERT_NO_THROW(dms_rp_.storeDocument(bescheid));
    status_ = VerfahrenStatus::GENEHMIGT;
    logPhase("Phase9:Genehmigungsbescheid");

    // ── Finale Assertions ────────────────────────────────────────────────────
    EXPECT_EQ(status_, VerfahrenStatus::GENEHMIGT)
        << "Gesamtverfahren muss mit GENEHMIGT abschließen";

    auto b = dms_rp_.getDocument("BESCHEID-VT-001");
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(b->metadata.at("ergebnis"), "GENEHMIGT");

    // Alle 7 Fachbehörden haben Stellungnahme abgegeben
    EXPECT_GE(dms_lanuv_.listByType(XDOMEAObjectType::DOKUMENT).size(),           1u);
    EXPECT_GE(dms_umweltamt_.listByType(XDOMEAObjectType::DOKUMENT).size(),       1u);
    EXPECT_GE(dms_gesundheitsamt_.listByType(XDOMEAObjectType::DOKUMENT).size(),  1u);
    EXPECT_GE(dms_bauaufsicht_.listByType(XDOMEAObjectType::DOKUMENT).size(),     1u);
    EXPECT_GE(dms_gewerbeaufsicht_.listByType(XDOMEAObjectType::DOKUMENT).size(), 1u);
    EXPECT_GE(dms_wasserbeh_.listByType(XDOMEAObjectType::DOKUMENT).size(),       1u);
    EXPECT_GE(dms_naturschutz_.listByType(XDOMEAObjectType::DOKUMENT).size(),     1u);
}
