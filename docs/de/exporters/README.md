# Exporters Module

**Stand:** 6. April 2026  
**Version:** 1.1  
**Kategorie:** Datenexport / LLM-Training  
**Validated:** 2026-03-22 (Reality-Check gegen Sourcecode; siehe [missing-implementations.md](missing-implementations.md))

---

## Übersicht

Das Exporters-Modul stellt Datenexport-Funktionalität für ThemisDB bereit. Es exportiert Dokumente aus beliebigen Kollektionen in Formate, die für LLM-Training, Datenanalyse und ML-Pipelines optimiert sind.

**Wichtigste Eigenschaften:**

- JSONL-Export mit Instruction-Tuning-Formaten (Alpaca, ShareGPT, ChatML, OpenAI)
- Apache Parquet-Export mit konfigurierbarem Arrow-Schema
- Apache Arrow IPC-Export für zero-copy-Pipelines
- Hugging Face Datasets-kompatibler Export (JSONL-Shards + `dataset_card.md` + `dataset_info.json`)
- Streaming-Export für große Kollektionen ohne vollständiges In-Memory-Laden
- Inkrementeller/Delta-Export mit Wasserzeichen-basierter Änderungsverfolgung
- **Cross-Collection-Join-Export** — Hash-Join zweier Kollektionen; Output-Field-Aliasing, AQL-Prädikat, PII-Redaktion
- AQL-Prädikat-Filterung zur Einschränkung exportierter Datensätze
- AES-256-GCM-Verschlüsselung für sensible Exportdaten
- PII-Erkennung und -Redaktion vor dem Export
- Synthetische Datenanreicherung (Data Augmentation) für Trainingsdaten-Diversität

---

## Source-Code Referenz

### Implementierung (`src/exporters/`)

| Datei / Komponente | Rolle |
|---|---|
| `jsonl_llm_exporter.cpp` | JSONL-Export: Instruction/Input/Output-Format, Batching, LoRA-Metadaten |
| `huggingface_exporter.cpp` | Hugging Face Datasets-kompatibler Export (JSONL-Shards + Dataset Card) |
| `parquet_exporter.cpp` | Apache Parquet Columnar-Export (Arrow C++ oder eingebetteter Fallback-Writer) |
| `arrow_ipc_exporter.cpp` | Apache Arrow IPC-Datei (`.arrow`) und Stream (`.arrows`) für zero-copy-Pipelines |
| `streaming_exporter.cpp` | Cursor-basierter Streaming-Export für große Kollektionen |
| `stream_writer.cpp` | Low-Level Streaming-Output-Writer mit konfigurierbarem `max_buffer_bytes` |
| `incremental_exporter.cpp` | Delta-Export: exportiert nur Datensätze, die seit dem letzten Wasserzeichen geändert wurden |
| `aql_predicate_filter.cpp` | AQL-Prädikat-Filterung zur Einschränkung exportierter Datensätze zur Abfragezeit |
| `format_template.cpp` | Instruction-Tuning-Format-Templates: Alpaca, ShareGPT, ChatML, OpenAI |
| `export_encryption.cpp` | AES-256-GCM-Verschlüsselung für Exportdaten; Schlüsselmaterial wird nur per ID referenziert |
| `data_augmentation.cpp` | Synthetische Datenanreicherung (Synonym-Ersetzung, Paraphrase-Varianten) |
| `pii_detector.cpp` | PII-Erkennung (E-Mail, Telefon, SSN, Kreditkarte, IP) und Redaktion vor dem Export |
| `join_exporter.cpp` | Cross-Collection-Hash-Join-Export; Output-Field-Aliasing, AQL-Prädikat, PII-Redaktion |
| `huggingface_hub_client.cpp` | Hugging Face Hub-Upload via libcurl mit PolicyEngine-Autorisierung und Audit-Log |
| `exporter_metrics.cpp` | Export-Durchsatz, Datensatzzahl, PII-Trefferrate als Prometheus-Metriken |

**Gesamt:** 16 Implementierungsdateien, ~8 100 Zeilen

---

## Exporter-Übersicht

| Exporter | Format | Status | Issue |
|---|---|---|---|
| `jsonl_llm_exporter.cpp` | JSONL (Instruction-Tuning) | ✅ Produktionsreif | — |
| `parquet_exporter.cpp` | Apache Parquet (Columnar) | ✅ Produktionsreif | #1710 |
| `arrow_ipc_exporter.cpp` | Apache Arrow IPC | ✅ Produktionsreif | #1714 |
| `huggingface_exporter.cpp` | Hugging Face Datasets | ✅ Produktionsreif | #1711 |
| `streaming_exporter.cpp` | Streaming (cursor-basiert) | ✅ Produktionsreif | — |
| `incremental_exporter.cpp` | Delta/Inkrementell | ✅ Produktionsreif | #1726 |
| `aql_predicate_filter.cpp` | AQL-Filterung | ✅ Produktionsreif | #1715 |
| `format_template.cpp` | Format-Templates | ✅ Produktionsreif | #1727 |
| `export_encryption.cpp` | AES-256-GCM-Verschlüsselung | ✅ Produktionsreif | #1728 |
| `data_augmentation.cpp` | Datenanreicherung | ✅ Produktionsreif | — |
| `huggingface_hub_client.cpp` | Hugging Face Hub API | ✅ Produktionsreif | #1719 |
| `join_exporter.cpp` | Cross-Collection-Join | ✅ Produktionsreif | #1722 |

---

## Laufzeitverhalten

### JSONL-Export (Standardpfad)

```cpp
JSONLLLMExporter exporter(config);
ExportStats stats = exporter.exportEntities(entities, options);
// stats.exported_entities, stats.bytes_written, stats.errors
```

### Streaming-Export für große Kollektionen

```cpp
StreamingExporter exporter(config);
// Exportiert in Seiten (page_size konfigurierbar, Standard: 1 000 Dokumente)
// Maximale Puffergröße: max_buffer_bytes (Standard: 256 MB)
ExportStats stats = exporter.exportEntities(entities, options);
```

### Inkrementeller/Delta-Export

```cpp
IncrementalExporter exporter(config);
// Liest Wasserzeichen aus watermark_path
// Exportiert nur Datensätze mit _seq > letztes Wasserzeichen
// Wasserzeichen-Update: atomisch via .tmp + rename()
ExportStats stats = exporter.exportEntities(entities, options);
```

### Cross-Collection-Join-Export

```cpp
JoinExportConfig cfg;
cfg.left_collection  = "documents";
cfg.right_collection = "annotations";
cfg.left_key_field   = "_key";
cfg.right_key_field  = "doc_id";
cfg.join_predicate   = "doc.score >= 0.5";
cfg.output_fields    = {"_key", "content", "label"};

JoinExporter exporter(cfg);
exporter.setRightCollection(right_entities);  // Lädt Right-Side-Hash-Tabelle

ExportOptions opts;
opts.output_path = "/tmp/joined.jsonl";
ExportStats stats = exporter.exportEntities(left_entities, opts);
// Innerer Hash-Join: nicht übereinstimmende linke Datensätze werden übersprungen
```

### PII-Redaktion vor dem Export

```cpp
PIIDetector detector(pii_config);
std::string redacted = detector.redactPII(raw_text);
// Strategien: MASK (***), HASH (SHA256:...), REMOVE ([REDACTED]), PARTIAL
```

---

## Build-Konfiguration

| CMake-Flag / Compiler-Define | Standard | Beschreibung |
|---|---|---|
| `ARROW_ENABLED` | OFF | Arrow C++ Bibliothek aktivieren (Parquet/Arrow IPC; ohne Fallback auf eingebetteten Writer) |
| — | — | Alle anderen Exporter haben keine Build-Pflichtabhängigkeiten außer OpenSSL |

---

## Performance-Ziele

| Metrik | Aktueller Wert | Zielwert | Methode |
|---|---|---|---|
| JSONL-Export-Durchsatz | ~150 MB/s (vollständiger Batch) | ≥ 200 000 Dok/s (sustained) | `benchmarks/export_bench.cpp` |
| Peak-Speicher (50 GB-Export) | Begrenzt via `StreamingExporter` (≤ 512 MB) | ≤ 512 MB | `/proc/self/status` VmRSS |
| Parquet-Export-Durchsatz | Implementiert (Fallback-Writer) | ≥ 500 MB/s unkomprimiert (Arrow-Pfad) | `benchmarks/export_bench.cpp` |
| Delta-Export-Speedup (0,1 % Änderungsrate) | Implementiert | ≥ 10× vs. vollständiger Export | Integrations-Tests |
| Join-Export-Durchsatz | Implementiert (Hash-Join) | ≥ 50 000 gemischte Dok/s | `tests/exporters/test_join_exporter.cpp` |

---

## Sicherheit

- **PII-Schutz**: `pii_detector.cpp` erkennt und redigiert E-Mail, Telefon, SSN, Kreditkarte und IP-Adressen vor dem Export
- **Verschlüsselung**: `export_encryption.cpp` implementiert AES-256-GCM mit `TENC`-Magic-Header; DEK wird via HKDF-SHA256 aus einem KEK-Referenz abgeleitet — rohe Schlüssel erscheinen niemals in Logs
- **Metriken ohne Datenleckage**: `exporter_metrics.cpp` emittiert keine Dokumenteninhalte oder Feldwerte in Prometheus-Labels
- **Wasserzeichen-Atomizität**: `incremental_exporter.cpp` schreibt Wasserzeichen atomisch via `.tmp` + `rename()`, um Datenverlust bei Absturz zu verhindern

---

## Verwandte Dokumentation

### Primärdokumentation (Quellcode)

- [README (src/exporters)](../../../src/exporters/README.md) — Modulübersicht und Komponentenliste
- [ARCHITECTURE (src/exporters)](../../../src/exporters/ARCHITECTURE.md) — Architektur-Leitfaden mit Datenfluss-Diagramm
- [ROADMAP (src/exporters)](../../../src/exporters/ROADMAP.md) — Entwicklungs-Roadmap und Produktionsreife-Checkliste
- [FUTURE_ENHANCEMENTS (src/exporters)](../../../src/exporters/FUTURE_ENHANCEMENTS.md) — Geplante Features mit Performance-Zielen und wissenschaftlichen Referenzen

### Reality-Check & Offene Implementierungen

- [missing-implementations.md](missing-implementations.md) — Reality-Check-Bericht: fehlende/unvollständige Implementierungen mit Evidence, Impact und Issue-Vorschlägen (Stand 2026-03-09)
- [missing-implementations.json](missing-implementations.json) — Maschinenlesbares Format des obigen Berichts

### Verwandte Module

- [LLM-Modul](../llm/README.md) — LLM-Inferenz und Adapter-Verwaltung, die Exportformate konsumiert
- [LoRA-Modul](../lora/README.md) — LoRA-Adapter-Metadaten-Format, das von `jsonl_llm_exporter.cpp` erzeugt wird
- [Security-Modul](../security/README.md) — `PolicyEngine` für export-berechtigungs-basierte Zugriffskontrolle (geplante Integration)
