# Scraper-Modul

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/scraper/README.md · ../../../include/scraper/README.md -->

**Stand:** 13. Mai 2026<br>
**Version:** 1.0.0<br>
**Kategorie:** Datenerfassung / Web-Scraper<br>
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Modul `scraper` stellt ThemisDBs agentischen Web-Scraper bereit. Es automatisiert
die Erfassung öffentlich zugänglicher Rechts- und Behördendaten aus deutschen
Bundes- und Landesportalen sowie EU-Quellen. Der Scraper führt für jeden Lauf eine
vollständige Pipeline aus: HTTP/JS-Seitenabfrage, HTML-Formularsuch-Discovery,
JSON-REST/GraphQL-API-Crawl, LLM-basierte Qualitätsbewertung und Multi-Modell-Persistenz
(relational, Property-Graph, Vektor).

**Primäre Quelle:** [`src/scraper/`](../../../src/scraper/)

---

## Hauptkomponenten

| Komponente | Source | Beschreibung |
|---|---|---|
| `ScraperPlugin` | `src/scraper/scraper_plugin.h/.cpp` | Agentische Orchestrierung des gesamten Scraper-Laufs |
| `ScraperConfig` / `UrlPolicy` | `src/scraper/scraper_config.h/.cpp` | YAML-Konfiguration, URL-Filterregeln, Render-Modi |
| `HtmlSearchEngine` | `src/scraper/scraper_search_engine.h/.cpp` | HTML-Formular-Discovery und Ergebnisseiten-Parsing (pugixml) |
| `HttpScraperApiClient` | `src/scraper/scraper_api_client.h/.cpp` | REST/GraphQL-JSON-API-Client mit automatischer Paginierung |
| `ScraperLLMEvaluator` | `src/scraper/scraper_llm_evaluator.h/.cpp` | Qualitäts- und Lücken-Relevanzbewertung (LLM + Heuristik-Fallback) |
| `SubprocessJSRenderer` | `src/scraper/scraper_js_renderer.h/.cpp` | Headless-Browser-Rendering über externen Subprocess |
| `GovSourceCatalog` | `src/scraper/gov_source_catalog.h/.cpp` | Eingebautes Verzeichnis mit Bund-, Länder- und EU-Portalen |
| `ScraperRecordBuilder` | `src/scraper/scraper_metadata_writer.h/.cpp` | Erzeugt relationale, Graph- und Vektor-Records mit Provenance-Feldern |

---

## Public API & Konfiguration

- Public-API-Guide: [`include/scraper/README.md`](../../../include/scraper/README.md)
- Wichtigster Einstiegspunkt: `scraper/scraper_plugin.h`
- Relevante Konfigurationsschlüssel:
  - `gap_context.gap_id`, `gap_context.keywords`
  - `crawl_options.render_mode` (`STATIC` | `JS_RENDERED` | `API_JSON` | `API_GRAPHQL`)
  - `crawl_options.max_pages` (Standard: 500), `request_delay_ms` (Standard: 250)
  - `llm_options.quality_threshold` (Standard: 0,65)
  - `gov_sources.bund_enabled`, `bundeslaender_enabled`, `eu_enabled`, `source_ids`
  - `whitelist`, `blacklist`, `seed_urls`

---

## Agentischer Scraper-Ablauf

```
1. Seed-URLs sammeln (config + GovSourceCatalog)
2. Je Seed-URL:
   a. Seite laden (STATIC / JS_RENDERED / API_JSON)
   b. UrlPolicy-Check (Whitelist/Blacklist, SSRF-Guard)
   c. HTML-Formulare suchen und mit Gap-Keywords abschicken
   d. Ergebnislisten paginieren; je Ergebnis-URL: Dokument laden
   e. API-Crawl (bei API_JSON / API_GRAPHQL)
3. Je Dokument:
   a. Text extrahieren
   b. LLM-Bewertung (Qualität + Gap-Relevanz) oder Heuristik-Fallback
   c. Unter Schwellwert → verwerfen; sonst → ScraperRecordBuilder
   d. Relational + Graph + Vektor persistieren
```

---

## Laufzeitverhalten, Fehlerfälle und Grenzen

- `scrape()` ohne vorheriges `initialize()` wirft `std::logic_error`.
- HTTP-Fehler (Timeout, DNS-Fehler, Non-2xx) werden pro URL abgefangen und geloggt;
  der Lauf bricht nicht ab.
- LLM-Auswertungsfehler fallen automatisch auf die Heuristik zurück.
- JS-Rendering-Timeout → `JsRenderResult{success=false}` für die betroffene URL;
  Lauf wird fortgesetzt.
- Nicht-http/https-Schemata werden vom SSRF-Guard in `UrlPolicy` lautlos abgelehnt.
- `max_pages` (Standard: 500) und `request_delay_ms` (Standard: 250 ms) begrenzen
  unkontrolliertes Crawlen.
- `ScraperRecordBuilder` setzt die drei Provenance-Felder (`is_scraper_ingested`,
  `ingestion_source_type`, `ingestion_plugin_version`) zwingend auf jeden Record.
  Diese Felder dürfen downstream nicht überschrieben werden.

---

## Behördenquellen-Katalog (`GovSourceCatalog`)

Der eingebaute Katalog enthält:

| Gruppe | Quellen |
|---|---|
| **Bund** | 8 Portale (u.a. `gesetze_im_internet`, `bundesanzeiger`, `bgbl`) |
| **Bundesländer** | je 1 Landesrechtsportal für alle 16 Bundesländer |
| **EU** | 5 Portale (EUR-Lex, CURIA, europarl.europa.eu, ec.europa.eu, publications.europa.eu) |

Quellen werden über `gov_sources.bund_enabled`, `bundeslaender_enabled`, `eu_enabled`
oder eine explizite `source_ids`-Liste aktiviert. Ein eigenes YAML-Overlay kann
den Katalog erweitern oder überschreiben.

---

## Kurzbeispiel

```cpp
#include "scraper/scraper_plugin.h"
#include "scraper/scraper_config.h"

using namespace themis::scraper;

ScraperConfig cfg = ScraperConfig::loadFromFile("/etc/themis/scraper.yaml");
ScraperPlugin plugin;

if (!plugin.initialize(cfg)) {
    throw std::runtime_error("Scraper-Initialisierung fehlgeschlagen");
}

ScraperRunStats stats = plugin.scrape();

for (const ScrapedDocument& doc : plugin.getResults()) {
    if (!doc.discarded) {
        std::cout << doc.title << " (" << doc.quality_score << ")\n";
    }
}
```

---

## Performance-Erwartungen

| Ziel | Erwartungswert |
|---|---|
| Text-Extraktion Throughput | ≥ 50 MB/s bei Standarddokumenten |
| End-to-End Extraktion P95 | ≤ 50 ms je Dokument |
| JS-Renderer-Overhead | ≤ 20 % ggü. Non-JS-Pfad |
| Metadaten-Schreibpfad P99 | ≤ 15 ms |
| Throughput-Regression unter Last | ≤ 10 % ggü. Baseline |

---

## Troubleshooting

| Symptom | Ursache | Lösung |
|---|---|---|
| Kein Dokument akzeptiert | `quality_threshold` zu hoch | `llm_options.quality_threshold` senken; `discard_reason` prüfen |
| JS_RENDERED-Seiten leer | `js_renderer_cmd` nicht gesetzt | `crawl_options.js_renderer_cmd` auf Renderer-Skript setzen |
| `write_errors > 0` | DB-Verbindung nicht verfügbar | DB-Verbindung prüfen; `IScraperMetadataWriter` injizieren |
| Alle Seeds geblockt | Whitelist zu restriktiv | `whitelist` erweitern oder `UrlPolicy` debuggen |
| LLM-Bewertung inaktiv | `THEMIS_ENABLE_LLM` nicht gesetzt | Heuristik-Fallback aktiv; GGUF-Modell laden falls LLM gewünscht |

---

## Installation

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}          # include/
    ${THEMISDB_SOURCE_DIR}/src       # src/
)
target_compile_definitions(your_target PRIVATE
    THEMIS_ENABLE_CURL     # libcurl-HTTP-Backend
    THEMIS_ENABLE_PUGIXML  # pugixml-HTML-Parser
    THEMIS_ENABLE_LLM      # LLM-Bewertungspfad
)
```

---

## Usage

Vollständige Verwendungsbeispiele inkl. Dependency-Injection für Tests:
[`include/scraper/README.md`](../../../include/scraper/README.md)

---

## Weiterführende Links

- [`src/scraper/README.md`](../../../src/scraper/README.md) — Implementierungsübersicht
- [`include/scraper/README.md`](../../../include/scraper/README.md) — Public-API-Referenz
- [`src/scraper/ROADMAP.md`](../../../src/scraper/ROADMAP.md) — Lieferstatus und geplante Features
- [`src/scraper/FUTURE_ENHANCEMENTS.md`](../../../src/scraper/FUTURE_ENHANCEMENTS.md) — Geplante Erweiterungen
- [`src/scraper/PERFORMANCE_EXPECTATIONS.md`](../../../src/scraper/PERFORMANCE_EXPECTATIONS.md) — Performance-Ziele
