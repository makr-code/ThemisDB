# Scraper Plugin

Agentic AI module for gap-detection-driven web scraping in ThemisDB.

## Overview

The scraper plugin fills data gaps by:

1. **Searching** – submitting gap keywords to configured web portals via
   HTML search forms (e.g. openjur.de, Gesetze im Internet)
2. **Scraping** – fetching and extracting text from result pages
3. **Rendering** – using a headless browser for JavaScript-rendered SPAs
   (React, Vue, Next.js, webpack-dev-server)
4. **Querying APIs** – calling JSON REST endpoints (Bundestag DIP, EUR-Lex)
5. **Evaluating** – scoring each document against the gap context via LLM
   (or heuristic keyword-density fallback)
6. **Writing** – storing relational records, graph nodes/edges, and vector
   records to the ThemisDB metadata layer

## Provenance Flag – Mandatory Ingestion Marker

**Every** document, record, node, and vector produced by this plugin carries
three provenance fields that serve as an authoritative ingestion trail:

| Field | Value | Where |
|-------|-------|-------|
| `is_scraper_ingested` | `true` (bool) | `ScrapedDocument`, `ScraperRelationalRecord`, `ScraperVectorRecord` |
| `ingestion_source_type` | `"SCRAPER"` (string) | all three + graph-node property |
| `ingestion_plugin_version` | semver e.g. `"1.0.0"` | all three + graph-node property |

These fields default to the correct values and are explicitly set by
`ScraperRecordBuilder` — **they must never be cleared or overridden**.

Consumers (query layers, review pipelines) can use these fields to:
- Require human review before using scraped data in legal proceedings
- Filter or report on automatically ingested content
- Trace any DB record back to its automated source

```aql
// AQL example: find all scraper-ingested documents
FOR doc IN scraper_documents
  FILTER doc.is_scraper_ingested == true
  RETURN doc
```

## Knowledge Sources

The plugin ships a comprehensive catalog of reliable sources in
`config/knowledge_sources.yaml`:

| Category | Count | Examples |
|----------|-------|---------|
| EU Law (`LAW_EU`) | 7 | EUR-Lex, CURIA, EP Open Data, HUDOC |
| EU Data (`DATA`) | 3 | Eurostat, EU Open Data Portal, EEA |
| Bund Law/Courts (`LAW_BUND`, `CASE_BUND`) | 10 | BGH, BVerwG, BFH, BVerfG, Gesetze im Internet |
| Bund Data (`DATA`) | 4 | GovData, Destatis, UBA, BAuA |
| Bundesländer (`LAW_LAND_*`) | 16 | All 16 state law portals |
| Standards (`STANDARD`) | 7 | DIN, ISO, IEC, VDE, ETSI, NIST, VDI |
| General knowledge (`WIKI`) | 5 | Wikipedia DE/EN, Wikidata, DBpedia, Bundestag Lexikon |
| Scientific (`SCIENTIFIC`) | 4 | OpenAlex, Europe PMC, arXiv, DOAJ |

**Total: 56 authoritative sources** — all with explicit license information.



The built-in catalog covers **29 official sources**:

| Group | Count | Examples |
|-------|-------|---------|
| Bund (Federal) | 8 | openjur.de, Gesetze im Internet, Bundesanzeiger, Bundestag DIP, BVerfG |
| Bundesländer (16 states) | 16 | gesetze-by.de, landesrecht-bw.de, recht.nrw.de, … |
| EU | 5 | EUR-Lex, CURIA, European Parliament, Publications Office |

Extend the catalog by adding entries to `config/gov_sources.yaml`.

## Configuration

Copy `config/scraper_urls.yaml` and adapt for your gap:

```yaml
gap_context:
  gap_id: "GAP-001"
  description: "Fehlende Daten zu Baugenehmigungsverfahren Bayern"
  keywords: ["Baugenehmigung", "BauGB", "Bebauungsplan"]

gov_sources:
  bund_enabled: true
  eu_enabled: true
  source_ids: ["openjur", "gesetze_im_internet", "eurlex"]
```

## Render Modes

| Mode | Description |
|------|-------------|
| `static` | Plain HTTP GET + HTML parsing |
| `js_rendered` | External headless browser (Puppeteer/Playwright) |
| `api_json` | JSON REST API endpoint |
| `api_graphql` | GraphQL POST endpoint |

For `js_rendered`, configure `js_renderer_cmd` with the path to your renderer:

```yaml
crawl_options:
  render_mode: js_rendered
  js_renderer_cmd: "node /opt/themis/scripts/renderer.js"
```

## Building

```bash
# With all features enabled:
cmake -DTHEMIS_PLUGIN_SCRAPER=ON \
      -DTHEMIS_ENABLE_YAML=ON \
      -DTHEMIS_ENABLE_CURL=ON \
      -DTHEMIS_ENABLE_LLM=ON \
      ..
```

Dependencies: `yaml-cpp`, `pugixml`, `libcurl`, `nlohmann_json`
Optional: `llama.cpp` (via `THEMIS_ENABLE_LLM`)

## Usage (C++ API)

```cpp
#include "scraper_plugin.h"
using namespace themis::scraper;

ScraperConfig cfg = ScraperConfig::loadFromFile("config/scraper_urls.yaml");
ScraperPlugin plugin;
plugin.initialize(cfg);
const ScraperRunStats stats = plugin.scrape();
// stats.docs_accepted, stats.docs_discarded, stats.elapsed_ms
```

## Tests

60 unit tests in `tests/test_scraper_plugin.cpp` (`ScraperPluginFocusedTests`):

- **Group A** (6) – UrlPolicy
- **Group B** (5) – ScraperConfig YAML loading
- **Group C** (5) – GovSourceCatalog (Bund, Länder, EU, YAML overlay)
- **Group D** (5) – HtmlSearchEngine (form discovery, result-list parsing)
- **Group E** (4) – ApiClient (page/cursor pagination)
- **Group F** (4) – LLM Evaluator (LLM path, heuristic fallback, threshold)
- **Group G** (4) – MetadataWriter (relational, graph, vector)
- **Group H** (7) – ScraperPlugin integration (init, search, API, JS, blacklist)
- **Group I** (6) – Provenance flags on relational, graph, vector records and `ScrapedDocument` *(v1.1.0)*
- **Group J** (5) – Knowledge source catalog completeness (Bundestag, EU, 16 Länder, license coverage, unique IDs) *(v1.1.0)*
- **Group K** (5) – End-to-end provenance propagation through `scrape()` *(v1.1.0)*
- **Group L** (4) – Provenance immutability (default values, cross-record consistency, custom version propagation) *(v1.1.0)*
