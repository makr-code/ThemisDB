# Exporters Module — Missing Implementations Report

**Validiert:** 2026-03-10  
**Geprüfte Revision:** `HEAD`  
**Geprüfte Pfade:** `src/exporters/`, `cmake/CMakeLists.txt`, `cmake/ModularBuild.cmake`, `tests/CMakeLists.txt`  
**Methode:** Reality-Check (Doku ↔ Sourcecode); Suche nach `TODO`, `STUB`, `NOT_IMPLEMENTED`; Zeilenzählung via `wc -l`; Datei-Metadaten-Analyse

---

## Zusammenfassung

| Schwere | Anzahl |
|---------|--------|
| 🔴 Kritisch (Produktionsblocker) | 0 |
| 🟡 Mittel (Funktional eingeschränkt) | 1 |
| 🟢 Gering (Hardening / Optimierung) | 2 |
| ✅ Behoben | 4 |

Alle Kern-Exporter (`jsonl_llm_exporter`, `parquet_exporter`, `arrow_ipc_exporter`, `huggingface_exporter`, `streaming_exporter`, `incremental_exporter`, `aql_predicate_filter`, `format_template`, `export_encryption`, `data_augmentation`, `pii_detector`, `exporter_metrics`, `stream_writer`) haben `Open Issues: TODOs: 0, Stubs: 0` und `Maturity Level: 🟢 PRODUCTION-READY`.

---

## Einträge

### ~~1. PolicyEngine-Integration für Export-Autorisierung~~ ✅ Behoben

`enforceExportPolicy()` ist in allen 6 Exportern aufgerufen (`jsonl_llm_exporter`, `streaming_exporter`, `incremental_exporter`, `parquet_exporter`, `arrow_ipc_exporter`, `huggingface_exporter`). Bei Ablehnung wird `ExporterException(ERR_EXPORT_POLICY_DENIED)` geworfen. Audit-Logging via `AuditLogger`: `EXPORT_DENIED`-Event bei Ablehnung, `BULK_EXPORT`-Event bei Genehmigung — sofern `ExportOptions::audit_logger` gesetzt ist. 9 Unit-Tests in `tests/exporters/test_export_encryption.cpp` (EXP-001).

---

### ~~2. Hub Direct Upload Integration~~ ✅ Behoben (Issue: #1719)

`HuggingFaceHubClient` vollständig implementiert in `src/exporters/huggingface_hub_client.cpp` und
`include/exporters/huggingface_hub_client.h`. Die Implementierung umfasst:

- libcurl-basierter HTTP-Upload mit Retry-Logik und Exponential Backoff
- Bearer-Token-Authentifizierung via `HubUploadConfig::hf_token` oder `HF_TOKEN`-Umgebungsvariable
- Automatische Repository-Erstellung bei `create_repo=true`
- **PolicyEngine-Integration**: `HubUploadConfig::policy_engine` — `uploadDataset()` ruft
  `PolicyEngine::checkExportPermission()` auf; abgelehnte Uploads geben sofort `success=false` zurück
- **Audit-Log-Integration**: `HubUploadConfig::audit_log` — jeder Upload-Versuch (erlaubt,
  abgelehnt, Fehler) wird als `hub_upload`-JSON-Eintrag ins Audit-Log geschrieben
- Unit-Tests in `tests/exporters/test_huggingface_hub_client.cpp` decken Denial, Permit,
  Audit-Log-Writes und Backward-Compatibility ab

---

### ~~3. Stale TODO-Zähler in pii_detector.cpp-Header~~ ✅ Behoben

`src/exporters/pii_detector.cpp` Zeile 14: `TODOs: 1` → `TODOs: 0` korrigiert (kein tatsächlicher TODO-Kommentar im Funktionskörper).

---

### 4. --incremental CLI-Flag fehlt (🟢 Gering)

**Claim-Quelle:** `src/exporters/FUTURE_ENHANCEMENTS.md` → Incremental / Delta Export → Remaining  
**Datei:** `tools/` (CLI-Export-Befehl)

**Erwartet:** `--incremental`-Flag am CLI-Export-Befehl in `tools/`, das `IncrementalExporter` statt `JSONLLLMExporter` aktiviert.

**Beobachtet:** `incremental_exporter.cpp` ist vollständig implementiert. Das CLI-Flag fehlt noch — kein `--incremental`-Argument in den `tools/`-Dateien gefunden.

**Evidence:**
- `src/exporters/FUTURE_ENHANCEMENTS.md`, Abschnitt „~~Incremental / Delta Export~~ ✅ Implemented": `Remaining: --incremental flag on the CLI export command in tools/.`

**Impact:** Benutzer müssen `IncrementalExporter` programmatisch konfigurieren; kein bequemer CLI-Schalter für inkrementelle Exports.

**Issue-Titelvorschlag:** `feat(tools): add --incremental flag to CLI export command`  
**Label-Vorschläge:** `module:exporters`, `kind:usability`, `priority:low`

---

### 5. ExportFormatRegistry nicht implementiert (🟢 Gering)

**Claim-Quelle:** `src/exporters/ROADMAP.md` → Breaking Changes; `src/exporters/FUTURE_ENHANCEMENTS.md` → Parquet → Remaining  
**Datei:** — (kein `export_format_registry.cpp` vorhanden)

**Erwartet:** `ExportFormatRegistry`-Singleton zur Registrierung von Export-Writern nach Format-Typ; ermöglicht additive Erweiterung ohne API-Änderung.

**Beobachtet:** Kein `ExportFormatRegistry` oder vergleichbare Registry-Klasse in `src/exporters/` gefunden. Exporter werden direkt instanziiert.

**Evidence:**
- `src/exporters/ROADMAP.md` → Breaking Changes: `Export format registry will be introduced to add new formats without changing the API signature (additive, non-breaking)`
- `src/exporters/FUTURE_ENHANCEMENTS.md` → Parquet → Remaining: `Register Parquet writer in a formal ExportFormatRegistry`
- Kein `export_format_registry.cpp` oder `export_format_registry.h` in `src/exporters/`

**Impact:** Gering — neue Formate erfordern aktuell direkten Code-Zugriff; additive Erweiterbarkeit noch nicht gegeben.

**Issue-Titelvorschlag:** `feat(exporters): implement ExportFormatRegistry for pluggable format registration`  
**Label-Vorschläge:** `module:exporters`, `kind:architecture`, `priority:low`

---

### ~~6. huggingface_exporter.cpp und data_augmentation.cpp nicht im Build-System~~ ✅ Behoben

`huggingface_exporter.cpp` und `data_augmentation.cpp` zu `cmake/CMakeLists.txt` und `cmake/ModularBuild.cmake` hinzugefügt.

---

### ~~7. Keine Focused-Test-Targets für Exporters-Modul~~ ✅ Behoben

10 Focused-Test-Targets in `tests/CMakeLists.txt` registriert: `JsonlLlmExporterFocusedTests`, `HuggingFaceExporterFocusedTests`, `ParquetExporterFocusedTests`, `ArrowIpcExporterFocusedTests`, `StreamingExporterFocusedTests`, `IncrementalExporterFocusedTests`, `AqlPredicateFilterFocusedTests`, `FormatTemplateFocusedTests`, `ExportEncryptionFocusedTests`, `DataAugmentationFocusedTests`.

---

## Nicht gefundene Probleme

Die folgenden in anderen Modulen häufigen Probleme wurden im Exporters-Modul **nicht** gefunden:

- Keine Stubs in Kern-Exportern (alle: `Stubs: 0`)
- Kein widersprüchlicher Maturity-Level (alle: `🟢 PRODUCTION-READY`)
- Keine fehlenden Dateien, die in README/ARCHITECTURE erwähnt werden (alle 13 Dateien vorhanden)
- Kein Parquet-Exporter ohne Fallback (eingebetteter Fallback-Writer vorhanden, Arrow C++ optional)
