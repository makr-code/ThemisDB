# Content Module — Fehlende / Unvollständige Implementierungen

**Stand:** 2026-03-09 (Reality-Check-Pass 1)  
**Revision:** HEAD (copilot/check-documentation-against-sourcecode)  
**Geprüfte Pfade:** `src/content/`, `include/content/`

---

## Zusammenfassung

| Schweregrad | Anzahl |
|-------------|--------|
| Critical    | 0 |
| Medium      | 3 |
| Low         | 4 |
| **Gesamt**  | **7** |

---

## CON-001 — LibreOffice Headless Fallback für Legacy-Office-Formate *(Medium)*

**Datei:** `src/content/office_processor.cpp`

**Erwartet:** `soffice --headless` wird via `posix_spawn` in einem Sandkasten-Subprozess (eingeschränkter OS-User, 30 s Timeout) gestartet, um `.doc`/`.xls`/`.ppt` nach Text zu konvertieren.

**Beobachtet:** Keine Implementierung. Kein `soffice`-, `posix_spawn`- oder `LibreOffice`-Aufruf in `office_processor.cpp`.

**Auswirkung:** Legacy `.doc`/`.xls`/`.ppt`-Dateien können nicht extrahiert werden; nur OOXML (DOCX/XLSX/PPTX) und ODF werden verarbeitet.

**Empfohlener Issue-Titel:** `feat(content): implement LibreOffice headless fallback in office_processor.cpp for legacy .doc/.xls/.ppt`

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

## CON-004 — OCR Sprachpaket-Pfad nicht an `config/ai_ml/tesseract_lang/` gebunden *(Low)*

**Datei:** `src/content/ocr_processor.cpp`

**Erwartet:** Sprachpakete aus `config/ai_ml/tesseract_lang/` laden; Standard `eng`; konfigurierbar pro Collection.

**Beobachtet:** `config_.data_dir` ist benutzergesteuert ohne Default-Konvention; kein Rückfall auf `config/ai_ml/tesseract_lang/`.

**Auswirkung:** Inkonsistentes Deployment; Betreiber müssen den Pfad manuell konfigurieren.

**Empfohlener Issue-Titel:** `feat(content): default ocr_processor data_dir to config/ai_ml/tesseract_lang/ via ConfigPathResolver`

---

## CON-005 — Backpressure bei Streaming-Ingestion nicht implementiert *(Low)*

**Datei:** `src/content/async_ingestion_worker.cpp`

**Erwartet:** `ingestStream()` blockiert den Aufrufer, wenn die Worker-Queue-Tiefe `config_.max_queue_depth` überschreitet; gibt `std::future<ContentId>` für async-Aufrufer zurück.

**Beobachtet:** Item in `FUTURE_ENHANCEMENTS.md` noch `[ ]` offen.

**Auswirkung:** Unter hoher Ingestion-Last kann die Worker-Queue unbegrenzt wachsen; Aufrufer werden nicht gedrosselt.

**Empfohlener Issue-Titel:** `feat(content): implement back-pressure in async_ingestion_worker.cpp for ingestStream()`

---

## CON-006 — Zip-Bomb-Schutz in `archive_processor.cpp` fehlt *(Low / Security)*

**Datei:** `src/content/archive_processor.cpp`

**Erwartet:** `content_security.cpp` scannt alle hochgeladenen Archive auf Zip-Bomb-Muster; max. Dekomprimierungs-zu-Komprimierungs-Verhältnis 100×, max. 1.000 extrahierte Dateien.

**Beobachtet:** Item in `FUTURE_ENHANCEMENTS.md` noch `[ ]` offen.

**Auswirkung:** Böswillig erstellte ZIP/tar-Archive könnten Speicher oder Disk erschöpfen.

**Empfohlener Issue-Titel:** `fix(content/security): add zip-bomb protection in archive_processor.cpp via content_security.cpp`

---

## CON-007 — LibreOffice-Subprozess muss `posix_spawn` mit eingeschränktem User nutzen *(Low / Security)*

**Datei:** `src/content/office_processor.cpp`

**Erwartet:** LibreOffice-Subprozess läuft unter eingeschränktem OS-User ohne Schreibzugriff auf das ThemisDB-Datenverzeichnis; nutzt `posix_spawn` statt `system()`.

**Beobachtet:** LibreOffice-Fallback noch nicht implementiert (CON-001); Sicherheitsanforderung muss bei der Implementierung von CON-001 eingehalten werden.

**Auswirkung:** Wenn der LibreOffice-Fallback ohne Sandboxing hinzugefügt wird, könnte ein bösartiges Dokument das ThemisDB-Datenverzeichnis kompromittieren.

**Empfohlener Issue-Titel:** `fix(content/security): ensure LibreOffice subprocess in office_processor.cpp uses posix_spawn with restricted user`

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
