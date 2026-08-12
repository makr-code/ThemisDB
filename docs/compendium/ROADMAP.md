# ThemisDB Kompendium — ROADMAP

**Version:** v1.5.0-dev  
**Stand:** 2026-08-12  
**Gouvernance:** [DOCUMENTATION_GOVERNANCE.md](../../../DOCUMENTATION_GOVERNANCE.md) Level 3/4

---

## Current Status

- [x] Phase 1 — YAML-gesteuertes Build-System (mkdocs-nav.yml, 11 Section Pages) — **COMPLETE** (Jan 2026)
- [x] Phase 2 — PDF Enhancement (Abbildungsverzeichnis, Header/Footer, Bookmarks) — **COMPLETE** (Jan 2026)
- [x] Phase 3 Kapitel 16 — Sharding mit RAID-äquivalenten Redundanzmodi — **COMPLETE** (Feb 2026)
- [x] Phase 3 Hochprioritäts-Kapitel (17, 29, 31, 40) — docs/de ↔ Kompendium-Sync — **COMPLETE** (Aug 2026)
- [x] Phase 3 Medium-Priority-Kapitel (10 verbleibende) — **COMPLETE** (Aug 2026)
- [ ] Phase 4 — Cross-Reference-Implementierung in allen Kapiteln — **PENDING** (Target: 2026-Q4)
- [ ] Phase 5 — v1.5.0 GA Build & QA Validation — **PENDING** (Target: 2026-Q4)

---

## Completed (Q3 2026)

- [x] Kapitel 17 (LLM Integration) — Cross-References zu docs/de/llm/, lora/, rag/ ergänzt (Aug 2026)
- [x] Kapitel 29 (Analytics & Process Mining) — AQL-Beispiele aus PROCESS_MINING_AQL_EXAMPLES.md integriert (Aug 2026)
- [x] Kapitel 31 (API-Protokolle) — Cross-References zu docs/de/apis/, rpc_grpc/ ergänzt (Aug 2026)
- [x] Kapitel 40 (Data Governance) — BSI C5/ISO 27001/DSGVO/SOC 2 Compliance-Matrix integriert (Aug 2026)
- [x] Kapitel 2 (Architektur) — docs/de/architecture/ Phase-3-Sync (Aug 2026)
- [x] Kapitel 19 (Monitoring) — docs/de/observability/ Phase-3-Sync (Aug 2026)
- [x] Kapitel 21 (Auth) — docs/de/auth/ Phase-3-Sync (Aug 2026)
- [x] Kapitel 22 (Encryption) — vollständiges Kapitel 22b aus docs/de/security/ (Aug 2026)
- [x] Kapitel 30 (Deployment) — docs/de/deployment/ Phase-3-Sync (Aug 2026)
- [x] Kapitel 34 (Query Optimization) — docs/de/query/ Phase-3-Sync (Aug 2026)
- [x] Kapitel 36 (Security Hardening) — docs/de/security/ Phase-3-Sync (Aug 2026)
- [x] Kapitel 38 (Observability SRE) — docs/de/observability/ Phase-3-Sync (Aug 2026)
- [x] Kapitel 39 (Performance Tuning) — docs/de/performance/ Phase-3-Sync (Aug 2026)
- [x] Appendix H (Glossar) — v1.5.0 Begriffe ergänzt (Aug 2026)
- [x] Appendix F (AQL-Cheatsheet) — v1.5.0 Patterns ergänzt (Aug 2026)
- [x] docs/compendium/VERSION auf v1.5.0-dev aktualisiert (Aug 2026)

---

## Planned Features (Q3–Q4 2026)

### Hochprioritäts-Deliverables

- [ ] Kapitel 17: LoRA-Finetuning-Prozess vollständig dokumentiert (Target: 2026-Q3)
  - Inputs: docs/de/lora/, docs/de/llm/LLAMA_CPP_FEATURE_IMPLEMENTATION_GUIDE.md
  - Outputs: Finetuning-Workflow, Konfigurations-Referenz, Troubleshooting-Abschnitt
- [ ] Kapitel 40: EU AI Act Mapping-Tabelle ergänzen (Target: 2026-Q3)
  - Inputs: docs/de/compliance/compliance_full_checklist.md §EU AI Act
- [ ] Kapitel 29: BPMN-Integration-Kapitel (Target: 2026-Q4)
  - Inputs: docs/de/analytics/BPMN_FUTURE_ENHANCEMENTS.md

### Medium-Priority Kapitel (Phase 3 Rest)

- [x] Kapitel 2 (Architektur) — docs/de/architecture/ integriert (Aug 2026)
- [x] Kapitel 19 (Monitoring) — docs/de/observability/ integriert (Aug 2026)
- [x] Kapitel 21 (Auth) — docs/de/auth/ integriert (Aug 2026)
- [x] Kapitel 22 (Encryption) — vollständiges Kapitel aus docs/de/security/ (Aug 2026)
- [x] Kapitel 36 (Security Hardening) — docs/de/security/ integriert (Aug 2026)
- [x] Kapitel 34 (Query Optimization) — docs/de/query/ integriert (Aug 2026)
- [x] Kapitel 38 (Observability SRE) — docs/de/observability/ integriert (Aug 2026)
- [x] Kapitel 39 (Performance Tuning) — docs/de/performance/ integriert (Aug 2026)
- [x] Kapitel 30 (Deployment) — docs/de/deployment/ integriert (Aug 2026)
- [x] Appendix Update — Glossar + AQL-Cheatsheet v1.5.0-Update (Aug 2026)

### Qualitätssicherung & Abnahme

- [ ] Build-Validierung nach Content-Integration (mkdocs-nav.yml, step1–5 Python-Skripte) (Target: 2026-Q4)
- [ ] HTML + PDF Output QA (Seitenanzahl, Bookmarks, Cross-Links) (Target: 2026-Q4)
- [ ] Broken-Link-Scan für alle bidirektionalen Cross-References (Target: 2026-Q4)
- [ ] v1.5.0-dev → v1.5.0 Release-Bump nach Human-Review (Target: 2026-Q4)

---

## Implementation Phases

### Phase 1: YAML Build-System
- [x] YAML-gesteuertes Build-System implementiert (mkdocs-nav.yml, 11 Section Pages)
- [x] Automatische Section Pages (Teil I-X + Anhänge)
- [x] Abbildungsverzeichnis (101 Diagramme)
- [x] Kapitel-Nummerierung und Appendix-Integration

### Phase 2: PDF Enhancement
- [x] Vollständiges Abbildungsverzeichnis
- [x] Header/Footer-Generierung
- [x] PDF-Bookmarks (wkhtmltopdf Enhancement)
- [x] Build-Zeit: ~2 Minuten

### Phase 3: docs/de ↔ Kompendium-Sync
- [x] Kapitel 16 Sharding — RAID-Redundanzmodi vollständig
- [~] Hochprioritäts-Kapitel 17, 29, 31, 40 — Cross-References + Content-Integration (Aug 2026)
- [x] 10 Medium-Priority-Kapitel (Aug 2026) — Phase-3-Sync vollständig
- [ ] Bidirektionale Cross-Reference-Links in allen Kapiteln

### Phase 4: Cross-Reference-Vollständigkeit
- [ ] Alle Kapitel mit bidirektionalen Links zu docs/de/-Quellen
- [ ] docs/de/-Dateien mit Rückverweisen auf Kompendium-Kapitel
- [ ] Kaputte Links per Link-Checker validieren

### Phase 5: v1.5.0 GA Build & Release
- [ ] Build-Validierung (HTML + PDF)
- [ ] Content-Review durch Human-Maintainer
- [ ] VERSION auf v1.5.0 bumpen
- [ ] Release-Tag setzen

---

## Production Readiness Checklist

- [x] YAML-gesteuertes Build-System funktionsfähig
- [x] 43 Kapitel + 7 Anhänge strukturiert
- [x] PDF-Build validiert (1.7 MB HTML + 6.9 MB PDF)
- [~] Phase-3-Sync der 4 Hochprioritäts-Kapitel abgeschlossen
- [ ] Alle 64 Kapitel mit vollständigen Cross-References
- [ ] Broken-Link-Scan bestanden
- [ ] Human-Review für v1.5.0-Release

---

## Known Issues & Limitations

- [!] 10 Medium-Priority-Kapitel — Phase-3-Sync COMPLETE (Aug 2026)
- [!] Kapitel-Nummerierungs-Duplikate vorhanden (chapter_08_storage_layer + chapter_08_vector; chapter_16_ml + chapter_16_sharding; etc.) — zu bereinigen vor v1.5.0 GA
- [!] Build-Skripte (step1–5) nicht automatisiert via CI — manueller Build-Trigger erforderlich

## Breaking Changes

- Keine Breaking Changes für Endnutzer.
- `VERSION` von `v1.4.0` auf `v1.5.0-dev` angehoben (2026-08-12).
