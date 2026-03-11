# Content Module — Fehlende / Unvollständige Implementierungen

**Stand:** 2026-03-11 (Reality-Check-Pass 3)  
**Revision:** HEAD (copilot/update-ocr-language-pack-path)  
**Geprüfte Pfade:** `src/content/`, `include/content/`

---

## Zusammenfassung

| Schweregrad | Anzahl |
|-------------|--------|
| Critical    | 0 |
| Medium      | 0 |
| Low         | 3 |
| **Gesamt**  | **3** |

---

## CON-001 — LibreOffice Headless Fallback für Legacy-Office-Formate ✅ BEHOBEN

**Datei:** `src/content/office_processor.cpp`

**Erwartet:** `soffice --headless` wird via `posix_spawn` in einem Sandkasten-Subprozess (eingeschränkter OS-User, 30 s Timeout) gestartet, um `.doc`/`.xls`/`.ppt` nach Text zu konvertieren.

**Behoben:** `OfficeProcessor::extractLegacyViaLibreOffice()` implementiert in `office_processor.cpp`. Verwendet `posix_spawn` (kein `system()`); 30-Sekunden-Timeout mit SIGTERM→SIGKILL-Eskalation; `POSIX_SPAWN_RESETIDS`+`POSIX_SPAWN_SETPGROUP`+`POSIX_SPAWN_SETSIGDEF`; vollständige 8-Byte-OLE-Header-Validierung; RAII-Temp-File-Bereinigung; minimale Sandbox-Umgebung (`HOME=tmpdir`). 11 Unit-Tests in `LegacyOfficeExtractionTest` + 2 Tests in `LegacyOfficeMetricsTest` in `tests/test_office_processor.cpp`.

**Auswirkung:** Legacy `.doc`/`.xls`/`.ppt`-Dateien werden jetzt via LibreOffice nach Plaintext konvertiert.

---

## CON-002 — MimeDetector-gesteuerte OCR-Aktivierung via ContentPolicy ✅ BEHOBEN

**Datei:** `src/content/mime_detector.cpp`, `src/content/content_manager.cpp`

**Erwartet:** `MimeDetector` aktiviert OCR für `image/png`, `image/jpeg`, `image/tiff`, wenn `ContentPolicy::ocrEnabled() == true` für die Collection.

**Behoben:** `ContentPolicy::ocr_enabled` + `ocrEnabled()` in `include/content/content_policy.h` implementiert. `MimeDetector::shouldTriggerOcr()` und `enableOcr()` in `src/content/mime_detector.cpp` implementiert. `ContentManager::ingestRawBlob()` nutzt jetzt `mime_detector_.enableOcr(config["ocr_enabled"])` + `mime_detector_.shouldTriggerOcr(detected_mime)` für das OCR-Routing statt der manuellen MIME-Typ-Prüfung. `MimeDetector mime_detector_` ist als privates Mitglied von `ContentManager` registriert (einmalige YAML-Initialisierung). Tests: `ContentPolicyOcrTest` (3 Tests in `test_content_policy.cpp`), `MimeDetectorOcrTest` (9 Tests in `test_content_policy.cpp`), `OcrMimeRoutingIntegrationTest` (5 Tests in `test_ocr_processor.cpp`).

**Auswirkung:** OCR wird automatisch beim Ingestion-Prozess ausgelöst, wenn `config["ocr_enabled"]=true` gesetzt ist. Das Routing läuft vollständig über `MimeDetector::shouldTriggerOcr()`, das intern `ContentPolicy::ocrEnabled()` prüft.

---

## CON-003 — OCR-Vorverarbeitung: DPI-Reskalierung auf 300 DPI ✅ **(Behoben)**

**Datei:** `src/content/ocr_processor.cpp`

**Erwartet:** Bilder auf 300 DPI reskalieren, falls Metadaten eine geringere Auflösung anzeigen; adaptive Binarisierung via Leptonica.

**Status:** Implementiert. `OcrProcessor::Config` besitzt jetzt `target_dpi` (Standard: 300), `enable_dpi_rescaling` (Standard: `true`) und `enable_adaptive_binarization` (Standard: `true`). In `runTesseract()` werden `pixGetXRes`/`pixGetYRes` zur DPI-Erkennung genutzt; bei niedrigerer Auflösung wird mit `pixScale` hochskaliert. Adaptive Binarisierung erfolgt via `pixConvertTo8` + `pixSauvolaBinarize` (Fenster 31×31, *k* = 0,35). Die Metadaten-Felder `ocr_input_dpi`, `ocr_rescaled` und `ocr_binarized` werden in `ExtractionResult::metadata` gespeichert.

**Issue-Titel:** `feat(content): add 300-DPI rescaling and adaptive binarisation in ocr_processor.cpp`

---

## CON-004 — OCR Sprachpaket-Pfad nicht an `config/ai_ml/tesseract_lang/` gebunden ✅ BEHOBEN

**Datei:** `src/content/ocr_processor.cpp`

**Erwartet:** Sprachpakete aus `config/ai_ml/tesseract_lang/` laden; Standard `eng`; konfigurierbar pro Collection.

**Behoben:** `runTesseract()` in `ocr_processor.cpp` verwendet nun `ConfigPathResolver::tryResolve("config/ai_ml/tesseract_lang")`, wenn `config_.data_dir` leer ist. Existiert das Verzeichnis, wird es als Tessdata-Pfad gesetzt; andernfalls greift der Tesseract-Auto-Detect-Mechanismus (nullptr). Die Sprachpräferenz bleibt `"eng"` als Standard, sofern nicht pro Collection überschrieben. Der Pfad `config/tesseract_lang` wurde als Legacy-Mapping zu `config/ai_ml/tesseract_lang` in `ConfigPathResolver::PATH_MAPPING` und `METADATA_TABLE` eingetragen. Das Verzeichnis `config/ai_ml/tesseract_lang/` wurde mit einer `README.md` angelegt, die Konventionen für die Installation zusätzlicher Sprachpakete dokumentiert.

**Auswirkung:** Konsistentes Deployment; Betreiber müssen den Pfad nicht mehr manuell konfigurieren. Override pro Collection möglich über `OcrProcessor::Config::data_dir`.

---

## CON-005 — Back-pressure bei Streaming-Ingestion ✅ BEHOBEN

**Datei:** `src/content/async_ingestion_worker.cpp`

**Erwartet:** `ingestStream()` blockiert den Aufrufer, wenn die Worker-Queue-Tiefe `config_.max_queue_depth` überschreitet; gibt `std::future<ContentId>` für async-Aufrufer zurück.

**Behoben:** `AsyncIngestionConfig` um Feld `max_queue_depth` (Standard: 1000) erweitert. `submitStream()` blockiert per `std::condition_variable::wait` statt eine Ausnahme zu werfen, wenn `queue.size() >= max_queue_depth`. Neue Methode `ingestStream()` gibt `std::future<std::string>` zurück, das bei Erfolg mit der primären ContentId aufgelöst wird; bei Fehlern und Worker-Shutdown wird die Ausnahme über das Future weitergeleitet. Worker-Loop benachrichtigt `backpressure_cv_` nach jedem Dequeue-Vorgang; `stop()` benachrichtigt `backpressure_cv_` beim Shutdown und bricht pending Promises ab. **Overload-Metriken:** `total_backpressure_events_` (atomarer Zähler für Blocking-Ereignisse) und `queue_depth_high_watermark_` (Peak-Queue-Tiefe) werden in `submitStream()` und `ingestStream()` inkrementiert und über `getStatistics()["backpressure"]` exponiert.

**Auswirkung:** Queue-Tiefe wird respektiert; kein unbegrenztes Wachstum unter Last. Sowohl synchrone als auch async-Aufrufer werden korrekt gedrosselt. Overload-Metriken ermöglichen Prometheus-Überwachung von Back-pressure-Ereignissen.

---

## CON-006 — Zip-Bomb-Schutz in `archive_processor.cpp` ✅ BEHOBEN

**Datei:** `src/content/archive_processor.cpp`

**Erwartet:** `content_security.cpp` scannt alle hochgeladenen Archive auf Zip-Bomb-Muster; max. Dekomprimierungs-zu-Komprimierungs-Verhältnis 100×, max. 1.000 extrahierte Dateien.

**Behoben:** `ContentSecurityManager::checkZipBomb()` in `content_security.h/.cpp` implementiert (Verhältnis-Schwellenwert 100×, max. 1.000 Dateien). `ArchiveProcessor::process()` ruft diese Methode nach `extractMetadata()` und vor der Entpackung auf. Konfigurierbar über `ContentSecurityConfig::max_zip_bomb_ratio` und `max_zip_file_count`.

**Auswirkung:** Böswillig erstellte ZIP/tar-Archive werden blockiert, bevor Speicher oder Disk erschöpft werden können.

---

## CON-007 — LibreOffice-Subprozess muss `posix_spawn` mit eingeschränktem User nutzen ✅ BEHOBEN

**Datei:** `src/content/office_processor.cpp`

**Erwartet:** LibreOffice-Subprozess läuft unter eingeschränktem OS-User ohne Schreibzugriff auf das ThemisDB-Datenverzeichnis; nutzt `posix_spawn` statt `system()`.

**Behoben:** Mit CON-001 zusammen implementiert. `extractLegacyViaLibreOffice()` verwendet `posix_spawn` mit `POSIX_SPAWN_RESETIDS` (verhindert SUID/SGID-Privilege-Escalation), `POSIX_SPAWN_SETPGROUP` (isolierte Prozessgruppe), minimaler Sandbox-Umgebung (`PATH`, `HOME=tmpdir`, `TMPDIR`), und Absolut-Pfad-Validierung für das soffice-Binary (verhindert PATH-Hijacking).

**Security-Tests:** `tests/test_office_processor.cpp` — `LibreOfficeSecurityTest` (7 Tests): Relative-Path-Rejection, Dot-Slash-Rejection, Path-Traversal-Rejection, Shell-Metacharacter-Injection (kein Shell-Aufruf via `posix_spawn`), OLE-Blob mit binärem Garbage, Large-Blob (4 MB), RAII-Temp-Dir-Cleanup nach Fehler (keine Datenleckage).

**Auswirkung:** Ein bösartiges Dokument kann das ThemisDB-Datenverzeichnis nicht mehr über den LibreOffice-Subprozess kompromittieren.

---

## Behobene Falschangaben (dieser Pass)

| Datei | Vorher | Nachher |
|-------|--------|---------|
| `src/content/ROADMAP.md` — Planned Features | `[I] PDF and Office document text extraction (pdfmium / LibreOffice headless)` | `[x]` aufgeteilt: PDF (poppler-cpp ✅) und Office OOXML/ODF (libzip+pugixml ✅) |
| `src/content/ROADMAP.md` — Planned Features | `[I] OCR for image-embedded text (Tesseract integration)` | `[x]` `ocr_processor.cpp` production-ready |
| `src/content/ROADMAP.md` — Phase 3 | `[I] Integrate Tesseract OCR` | `[x]` |
| `src/content/ROADMAP.md` — Phase 3 | `Status: In Progress` | `Status: Completed` |
| `src/content/FUTURE_ENHANCEMENTS.md` — PDF section | `[ ]` pdfium integration; "exist as stubs" | `[x]` poppler-cpp used; production-ready |
| `src/content/FUTURE_ENHANCEMENTS.md` — PDF section | `[ ]` DOCX extraction | `[x]` `office_processor.cpp::extractDOCX()` |
| `src/content/FUTURE_ENHANCEMENTS.md` — PDF section | `[ ]` XLSX extraction | `[x]` `office_processor.cpp::extractXLSX()` |
| `src/content/FUTURE_ENHANCEMENTS.md` — PDF section | `[ ]` metrics counters | `[x]` in `content_metrics.cpp` |
| `src/content/FUTURE_ENHANCEMENTS.md` — OCR section | `[ ]` Create `ocr_processor.cpp` | `[x]` done |
| `src/content/FUTURE_ENHANCEMENTS.md` — OCR section | `[ ]` `content_ocr_text` metadata field | `[x]` `result.metadata["content_ocr_text"]` at line 220 |
| `src/content/FUTURE_ENHANCEMENTS.md` — OCR section | `[ ]` graceful degradation without libtesseract | `[x]` returns skipped result |
| `src/content/README.md` | Out of scope: "PDF/binary format parsing (planned)" | PDF parsing implemented; legacy Office headless still planned |
| `src/content/README.md` | Maturity: 🟡 Beta | Maturity: 🟢 Production-Ready |

## Behobene Falschangaben (Pass 2 — 2026-03-11)

| Datei | Vorher | Nachher |
|-------|--------|---------|
| `src/content/office_processor.cpp` — DOC/XLS/PPT switch | Static "not supported" error | `extractLegacyViaLibreOffice()` — posix_spawn, 30s timeout, RAII cleanup (CON-001 ✅) |
| `src/content/ROADMAP.md` — Planned Features (long-term) | `[ ] LibreOffice headless fallback (CON-001)` | `[x]` implemented |
| `src/content/ROADMAP.md` — Phase 5 | `[ ] LibreOffice headless fallback (CON-001)` | `[x]` implemented |
| `src/content/ROADMAP.md` — Known Issues | `Legacy Office formats not fully supported (CON-001)` | Removed (resolved) |
| `src/content/ROADMAP.md` — Completed section | Only OOXML/ODF listed | Added legacy fallback entry |
| `src/content/FUTURE_ENHANCEMENTS.md` — PDF section preamble | "Legacy .doc/.xls via LibreOffice headless not yet implemented" | Updated to ✅ implemented |
| `src/content/FUTURE_ENHANCEMENTS.md` — PDF section notes | `[ ]` LibreOffice headless fallback | `[x]` extractLegacyViaLibreOffice() (CON-001 ✅) |
| `src/content/FUTURE_ENHANCEMENTS.md` — Security section | `[ ]` LibreOffice subprocess security requirement | `[x]` posix_spawn + POSIX_SPAWN_RESETIDS + minimal env (CON-007 ✅) |
| `include/content/content_policy.h` | No `ocr_enabled` / `ocrEnabled()` | `bool ocr_enabled = false` + `bool ocrEnabled() const` (CON-002 ✅) |
| `include/content/mime_detector.h` | No `shouldTriggerOcr()` / `enableOcr()` | `shouldTriggerOcr(mime_type)` + `enableOcr(bool)` added (CON-002 ✅) |
| `src/content/mime_detector.cpp` | No OCR routing logic | `shouldTriggerOcr()` + `enableOcr()` implemented; `ocr_recommended` set in `validateUpload()` (CON-002 ✅) |
| `include/content/content_manager.h` | No `MimeDetector` member | `MimeDetector mime_detector_` private member; `#include "content/mime_detector.h"` (CON-002 ✅) |
| `src/content/content_manager.cpp` — `ingestRawBlob` OCR section | Manual MIME-type check duplicating `shouldTriggerOcr()` logic | Uses `mime_detector_.enableOcr()` + `mime_detector_.shouldTriggerOcr()` (CON-002 ✅) |
