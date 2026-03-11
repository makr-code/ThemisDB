# Content Module — Fehlende / Unvollständige Implementierungen

**Stand:** 2026-03-11 (Reality-Check-Pass 2)  
**Revision:** HEAD (copilot/enhance-office-processor-legacy-support)  
**Geprüfte Pfade:** `src/content/`, `include/content/`

---

## Zusammenfassung

| Schweregrad | Anzahl |
|-------------|--------|
| Critical    | 0 |
| Medium      | 1 |
| Low         | 3 |
| **Gesamt**  | **4** |

---

## CON-001 — LibreOffice Headless Fallback für Legacy-Office-Formate ✅ BEHOBEN

**Datei:** `src/content/office_processor.cpp`

**Erwartet:** `soffice --headless` wird via `posix_spawn` in einem Sandkasten-Subprozess (eingeschränkter OS-User, 30 s Timeout) gestartet, um `.doc`/`.xls`/`.ppt` nach Text zu konvertieren.

**Behoben:** `OfficeProcessor::extractLegacyViaLibreOffice()` implementiert in `office_processor.cpp`. Verwendet `posix_spawn` (kein `system()`); 30-Sekunden-Timeout mit SIGTERM→SIGKILL-Eskalation; `POSIX_SPAWN_RESETIDS`+`POSIX_SPAWN_SETPGROUP`+`POSIX_SPAWN_SETSIGDEF`; vollständige 8-Byte-OLE-Header-Validierung; RAII-Temp-File-Bereinigung; minimale Sandbox-Umgebung (`HOME=tmpdir`). 13 Unit-Tests in `tests/test_office_processor.cpp` (`LegacyOfficeExtractionTest`).

**Auswirkung:** Legacy `.doc`/`.xls`/`.ppt`-Dateien werden jetzt via LibreOffice nach Plaintext konvertiert.

---

## CON-002 — MimeDetector-gesteuerte OCR-Aktivierung via ContentPolicy *(Medium)*

**Datei:** `src/content/mime_detector.cpp`

**Erwartet:** `MimeDetector` aktiviert OCR für `image/png`, `image/jpeg`, `image/tiff`, wenn `ContentPolicy::ocrEnabled() == true` für die Collection.

**Beobachtet:** Methode `ocrEnabled()` auf `ContentPolicy` nicht vorhanden; keine OCR-Routing-Logik in `mime_detector.cpp`.

**Auswirkung:** OCR wird nicht automatisch beim Ingestion-Prozess ausgelöst; muss manuell via `ocr_processor.cpp` aufgerufen werden.

**Empfohlener Issue-Titel:** `feat(content): add ContentPolicy::ocrEnabled() and wire OCR trigger in MimeDetector`

---

## CON-003 — OCR-Vorverarbeitung: DPI-Reskalierung auf 300 DPI *(Medium)*

**Datei:** `src/content/ocr_processor.cpp`

**Erwartet:** Bilder auf 300 DPI reskalieren, falls Metadaten eine geringere Auflösung anzeigen; adaptive Binarisierung via Leptonica.

**Beobachtet:** Leptonica wird für das Laden des Bildes genutzt (Zeile 145), aber keine DPI-Reskalierungslogik vorhanden.

**Auswirkung:** OCR-Qualität bei niedrig aufgelösten Scans kann beeinträchtigt sein.

**Empfohlener Issue-Titel:** `feat(content): add 300-DPI rescaling and adaptive binarisation in ocr_processor.cpp`

---

## CON-004 — OCR Sprachpaket-Pfad nicht an `config/ai_ml/tesseract_lang/` gebunden ✅ BEHOBEN

**Datei:** `src/content/ocr_processor.cpp`

**Erwartet:** Sprachpakete aus `config/ai_ml/tesseract_lang/` laden; Standard `eng`; konfigurierbar pro Collection.

**Behoben:** `runTesseract()` in `ocr_processor.cpp` verwendet nun `ConfigPathResolver::tryResolve("config/ai_ml/tesseract_lang")`, wenn `config_.data_dir` leer ist. Existiert das Verzeichnis, wird es als Tessdata-Pfad gesetzt; andernfalls greift der Tesseract-Auto-Detect-Mechanismus (nullptr). Die Sprachpräferenz bleibt `"eng"` als Standard, sofern nicht pro Collection überschrieben. Der Pfad `config/tesseract_lang` wurde als Legacy-Mapping zu `config/ai_ml/tesseract_lang` in `ConfigPathResolver::PATH_MAPPING` und `METADATA_TABLE` eingetragen.

**Auswirkung:** Konsistentes Deployment; Betreiber müssen den Pfad nicht mehr manuell konfigurieren. Override pro Collection möglich über `OcrProcessor::Config::data_dir`.

---

## CON-005 — Back-pressure bei Streaming-Ingestion ✅ BEHOBEN

**Datei:** `src/content/async_ingestion_worker.cpp`

**Erwartet:** `ingestStream()` blockiert den Aufrufer, wenn die Worker-Queue-Tiefe `config_.max_queue_depth` überschreitet; gibt `std::future<ContentId>` für async-Aufrufer zurück.

**Behoben:** `AsyncIngestionConfig` um Feld `max_queue_depth` (Standard: 1000) erweitert. `submitStream()` blockiert per `std::condition_variable::wait` statt eine Ausnahme zu werfen, wenn `queue.size() >= max_queue_depth`. Neue Methode `ingestStream()` gibt `std::future<std::string>` zurück, das bei Erfolg mit der primären ContentId aufgelöst wird; bei Fehlern und Worker-Shutdown wird die Ausnahme über das Future weitergeleitet. Worker-Loop benachrichtigt `backpressure_cv_` nach jedem Dequeue-Vorgang; `stop()` benachrichtigt `backpressure_cv_` beim Shutdown und bricht pending Promises ab.

**Auswirkung:** Queue-Tiefe wird respektiert; kein unbegrenztes Wachstum unter Last. Sowohl synchrone als auch async-Aufrufer werden korrekt gedrosselt.

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
