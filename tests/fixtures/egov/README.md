> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# E-Government Test-Fixtures

Generische Fixtures für datengetriebene E-Gov-Tests (ohne CI-Abhängigkeit).

**Anträge** liegen als **Markdown** (`.md`) mit JSON-Front-Matter vor —  
lesbar als Dokument und maschinell parsbar durch den Test.  
Behörden, Prozesse und Kontrollergebnisse bleiben als **JSON**.

## Verzeichnisstruktur

```
fixtures/egov/
├── behoerden.json                        # 11 generische deutsche Behörden (JSON)
├── antraege/
│   ├── baugenehmigung.md                 # Baugenehmigungsantrag (Markdown + JSON-Front-Matter)
│   └── bimschg.md                        # BImSchG-Antrag      (Markdown + JSON-Front-Matter)
├── prozesse/
│   ├── baugenehmigung_prozess.json       # Prozessdefinition Baugenehmigung (6 Phasen, JSON)
│   └── bimschg_prozess.json              # Prozessdefinition BImSchV (9 Phasen, JSON)
└── expected/
    ├── baugenehmigung_expected.json      # Kontrollergebnisse Baugenehmigung (20 ACs, JSON)
    └── bimschg_expected.json             # Kontrollergebnisse BImSchV (30 ACs, JSON)
```

## Lokal ausführen (ohne CI)

### Voraussetzungen

- CMake ≥ 3.20
- GTest (`libgtest-dev`)
- nlohmann/json (`nlohmann-json3-dev` oder `vcpkg install nlohmann-json`)
- C++17-Compiler (gcc-12+ oder clang-14+)

### Build & Ausführen

```bash
# Aus dem Repository-Root:
cmake -S tests -B build_egov \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DTHEMIS_ROOT_DIR=$(pwd)

cmake --build build_egov --target test_egov_data_driven_focused

cd build_egov
./test_egov_data_driven_focused
```

### Eigene Fixtures verwenden

```bash
export THEMIS_EGOV_FIXTURES_DIR=/pfad/zu/eigenen/fixtures
./test_egov_data_driven_focused
```

### Einzelne Tests auswählen

```bash
./test_egov_data_driven_focused --gtest_filter="*BAUGENEHMIGUNG*"
./test_egov_data_driven_focused --gtest_filter="*BIMSCHV*"
./test_egov_data_driven_focused --gtest_filter="*KontrollergebnisseAusFixture*"
```

## Fixture-Formate

### `behoerden.json`

Definiert alle Behörden, die in Verfahren beteiligt sein können:

```json
{
  "behoerden": [
    {
      "id": "bauamt-koeln",
      "name": "Bauamt Köln",
      "typ": "KOMMUNAL",
      "ozg_level": "MUNICIPAL",
      "rollen": ["FEDERFÜHREND"],
      "dms_id": "dms-bauamt-koeln"
    }
  ]
}
```

### `antraege/*.md` — Markdown mit JSON-Front-Matter

Anträge liegen als **Markdown-Dokument** vor. Das Front-Matter (zwischen den
`---`-Trennern) enthält die maschinenlesbare JSON-Struktur. Der Body ist
menschenlesbares Markdown mit Tabellen und Erläuterungen.

Der Test-Loader liest die `.md`-Datei, extrahiert automatisch das JSON-Front-Matter
und speichert den Prosatext im zusätzlichen Feld `_dokument_text`.

```markdown
---
{
  "aktenzeichen": "BAUAMT-KN-2026-0042",
  "antragsteller": { "vorname": "Hans", "eid_tx_id": "TX-BAU-2026-001", ... },
  "unterlagen": [ { "id": "DOC-BAU-001", "typ": "LAGEPLAN", "pflicht": true } ],
  "xoev_xml_vorlage": { "wurzelelement": "xbau", "felder": { ... } }
}
---

# Bauantrag — Neubau Wohngebäude

**Aktenzeichen:** BAUAMT-KN-2026-0042

## 1 · Antragsteller

| Feld | Wert |
|------|------|
| Name | Hans Mustermann |
...
```

### `prozesse/*.json`

Definiert Phasenreihenfolge, Fachbehörden und Prozessschritte:

```json
{
  "federführende_behoerde": "bauamt-koeln",
  "fachbehoerden": [ { "behoerde_id": "...", "stellungnahme_typ": "..." } ],
  "phasen": [ { "id": "ANTRAGSTELLUNG", "reihenfolge": 1, "schritte": [...] } ]
}
```

### `expected/*.json`

Enthält messbare Kontrollergebnisse als Assertions:

```json
{
  "assertions": [
    {
      "id": "AC-BGV-01",
      "typ": "ozg_service_found",
      "service_id": "DE-NW-BAUGENEHMIGUNG",
      "erwartet": true
    },
    {
      "id": "AC-BGV-13",
      "typ": "verfahrensstatus",
      "erwartet_status": "GENEHMIGT"
    }
  ]
}
```

## Unterstützte Assertion-Typen

| Typ | Beschreibung |
|-----|-------------|
| `ozg_service_found` | OZG-Dienst muss nach Registrierung abrufbar sein |
| `eid_fullname` | eID-Vollname und Assurance-Level |
| `eid_attribut` | Einzelnes eID-Attribut (z.B. AGS) |
| `xoev_import_success` | XÖV-Import erfolgreich |
| `xoev_export_wohlgeformt` | XÖV-Export ist gültiges XML |
| `xdomea_objekte_min` | XDOMEA-Store enthält Mindestanzahl Objekte |
| `xdomea_vorgaenge_min` | Mindestanzahl VORGANG-Objekte |
| `xdomea_stellungnahmen_alle` | Alle Behörden liefern Stellungnahmen |
| `xdomea_bescheid_vorhanden` | Dokument mit bestimmtem Metadatum vorhanden |
| `xdomea_akte_vorhanden` | AKTE mit korrektem Aktenzeichen |
| `xdomea_aktenzeichen` | Aktenzeichen in DMS vorhanden |
| `xdomea_export_wohlgeformt` | XDOMEA-Export ist gültiges XML |
| `xdomea_stores_min` | Gesamtanzahl Objekte in DMS |
| `xdomea_kinder_min` | Mindestanzahl Kind-Dokumente |
| `xdomea_metadaten_vorhanden` | Pflicht-Metadaten vorhanden |
| `xdomea_protokoll_einwendungsreferenzen` | Protokoll hat Einwendungsreferenzen |
| `llm_score_min` | LLM-Konformitätsscore ≥ threshold |
| `llm_summary_nichtleer` | LLM-Summary nicht leer |
| `verfahrensstatus` | Endstatus des Verfahrens |
| `prozess_reihenfolge` | Phasen in korrekter Reihenfolge protokolliert |
| `thread_safe_parallel_writes` | Parallele Schreibvorgänge thread-sicher |
| `xdomea_zustellung` | Bescheid an Empfänger zugestellt |
