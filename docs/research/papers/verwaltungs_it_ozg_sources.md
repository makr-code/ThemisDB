# Verwaltungs-IT Deutschland: OZG, FIM, XÖV und digitale Verwaltungsstandards

**Metadaten:**
- Author(en): FITKO, BMI, IT-Planungsrat, DigitalService Bund, XÖV-Koordinierungsstelle
- Konferenz/Journal: Gesetze, Normen, technische Standards (Government standards / Law)
- Jahr: 2017–2025 (fortlaufend)
- Link: [FITKO](https://www.fitko.de) · [OZG-Umsetzungskatalog](https://ozg.verdrusssache.de) · [FIM-Portal](https://fimportal.de) · [XÖV-Standards](https://www.xoev.de)
- Zitierweise: `fitko2024fim`; `bmi2017ozg`; `xoev2024`
- Tags: `verwaltungs-it`, `ozg`, `fim`, `xoev`, `xdomea`, `egov`, `bpmn`, `german-egovernment`, `process-db`
- ThemisDB-Versionen: v1.9.0+; primary reference for `src/process/` Verwaltungs-IT features
- Status: [~] In Progress (BPMN/XDOMEA import implemented; FIM process library import planned Q4 2026)

## 📋 Executive Summary

This document compiles the key German administrative IT standards and regulations that govern ThemisDB's Verwaltungs-IT use case. It covers the Onlinezugangsgesetz (OZG), Föderales Informationsmanagement (FIM), XÖV standards (XDOMEA, XPersonenstand, XMeld, XKfz), FITKO API standards, and the Single Digital Gateway Regulation (SDGR). Together, these define the data models, process structures, and legal compliance requirements for ThemisDB when used in German public administration contexts.

Directly referenced in `src/process/FUTURE_ENHANCEMENTS.md` (P8: FIM-Prozessbibliothek-Import, Target Q4 2026, Priority: Sehr Hoch) and `docs/de/process/STATE_OF_THE_ART.md`.

## 🎯 Key Findings

### Onlinezugangsgesetz (OZG) 2017 / OZG 2.0 (2024)
- **Scope**: 575 standardized administrative services (`Verwaltungsleistungen`) to be digitalized; structured as `Leistungen → Lebenslagen → Themenfelder`.
- **OZG-Umsetzungskatalog**: Machine-readable catalog of all 575 services with metadata (Rechtsgrundlagen, Zuständigkeiten, Verweis auf FIM-Leistungen).
- **OZG 2.0 (2024)**: Strengthens interoperability requirements; mandates API-first approach; introduces `Einer-für-Alle (EfA)` shared services model.
- **ThemisDB relevance**: OZG Leistungskatalog can be imported as ThemisDB's process schema catalog; service types → process templates.

### Föderales Informationsmanagement (FIM)
- **Scope**: Bundesweite Bibliothek von ca. 5,000 standardisierten Verwaltungsprozessen in 3 Schichten: **Leistung** (Was) → **Prozess** (Wie) → **Datenfelder** (Womit).
- **FIM-Leistungen**: Process blueprints describing the administrative service workflow; BPMN 2.0-compatible XML format.
- **FIM-Datenfelder**: Canonical field definitions (name, type, validation rules, legal reference) covering ~12,000 data fields across all services.
- **FIM-Portal API**: REST API for accessing FIM library; JSON/XML responses; publicly accessible.
- **ThemisDB relevance**: Import FIM-Leistungen as pre-built `ProcessModel` objects; FIM-Datenfelder as ThemisDB schema field registry.

### XDOMEA 3.0 (Verwaltungsakte)
- **Scope**: Standard for electronic records management (eAkte) in German public authorities; XML schema for document/file exchange between authorities.
- **XDOMEA document structure**: `Akte → Vorgang → Dokument` hierarchy; versioned metadata with `AbgabeNachricht`, `ImportNachricht`, `ExportNachricht` messages.
- **ThemisDB relevance**: Import XDOMEA documents as `ProcessAttachment` objects linked to process instances; `ProcessLinker::attachObject()` with `XDOMEA` link_type.

### XÖV Standards Family
- **XPersonenstand 2.0**: Birth, death, marriage registration data model; used for citizen identity in process contexts.
- **XMeld 3.6**: Municipal registration (Einwohnermeldung) data model; address and citizen identity fields.
- **XBau 2.6**: Building permit application data model; directly relevant to ThemisDB's Bauantrag use case.
- **XKfz 3.0**: Vehicle registration data model.
- **ThemisDB relevance**: XÖV data models as first-class ThemisDB schema types; import/export via `src/importers/` and `src/exporters/`.

### eID (BSI TR-03130 / AusweisApp2)
- **Scope**: German national ID card online authentication; SAML2 / OpenID Connect based; eID-Server API.
- **ThemisDB relevance**: `src/auth/` integration with eID-Server for citizen authentication in public-facing ThemisDB deployments.

## 🔗 Direct Influence on ThemisDB

### Affected Modules

- [x] Process module → `src/process/` (FIM process import, OZG service catalog, BPMN compliance)
- [x] Importers → `src/importers/` (XDOMEA document import, XÖV data model import)
- [x] Exporters → `src/exporters/` (XDOMEA export, XÖV-compatible output)
- [ ] Auth module → `src/auth/` (eID integration — planned)
- [ ] Schema registry → (planned: FIM-Datenfelder as canonical field types)

### What Was Adopted?

1. **BPMN 2.0 compliance target**: ISO/IEC 19510 BPMN 2.0.2 — ThemisDB's process XML export must be importable by Camunda, Signavio.
2. **XDOMEA import connector**: `src/importers/` supports XDOMEA 3.0 XML; maps `Akte/Vorgang/Dokument` to `ProcessInstance/ProcessAttachment`.
3. **FIM-inspired process taxonomy**: ThemisDB process `compliance_tags` follow FIM legal references (§ references to BauO, GWB, DSGVO).
4. **OZG service identifier**: Each Verwaltungsleistung has an OZG-ID; stored as `ProcessModel::ozg_service_id` for cross-system interoperability.

### How Was It Adapted?

| German Standard | ThemisDB Adaptation | Rationale |
|---|---|---|
| FIM 3-layer model | `ProcessModel` (process) + `SchemaField` (data fields) + `ServiceCatalog` (service) | Maps to ThemisDB's internal process graph structure |
| XDOMEA message-based exchange | Direct DB import via `XdomeaImporter` | ThemisDB ingests XDOMEA as a data source, not a transport protocol |
| OZG Leistungskatalog (CSV/JSON API) | Scheduled import job + ThemisDB process schema catalog | FIM-Portal provides REST API; scheduled nightly import |
| eID SAML2 | OpenID Connect bridge (BSI TR-03130 SP profile) | ThemisDB's auth module uses OIDC; eID acts as external OIDC provider |

## ⚠️ Limitations & Open Questions

- FIM process blueprints use BPMN 2.0 with German-specific notation extensions; full round-trip import/export requires German BPMN profile handling.
  - ThemisDB solution: Custom BPMN importer that maps FIM-specific extensions to ThemisDB node types.
- OZG-Umsetzungskatalog is incomplete (only ~35% of services had full digital implementation by 2022).
  - ThemisDB solution: Import available services; flag incomplete services as `ozg_status: partial`.
- XÖV standards have different versioning cycles; XDOMEA 3.0 vs. 2.4 co-exist in practice.
  - ThemisDB solution: Version-aware XDOMEA importer with fallback mapping.

## 🔬 Validation

- [x] BPMN 2.0 compliance reviewed against ISO/IEC 19510
- [x] XDOMEA import tested against real authority XML files
- [ ] FIM-Prozessbibliothek bulk import tested
- [ ] OZG service catalog import validated
- [ ] Auth module eID integration tested
- [ ] Module README linked

## 📚 Related Work

- [Process Mining — van der Aalst (2016)](process_mining_van_der_aalst_2012.md)
- [ProcessGPT — Busch et al. (2023)](processgpt_busch_2023.md)
- [OMG BPMN 2.0.2 Standard](https://www.omg.org/spec/BPMN/2.0.2/)
- [`docs/de/process/STATE_OF_THE_ART.md`](../../de/process/STATE_OF_THE_ART.md)
- [Lucke & Reinermann (2000) — Speyerer Definition von Electronic Government](https://foev.dhv-speyer.de/ruvii/)

---
**Last Updated:** 2026-04-06  
**Next Review:** 2026-09-30
