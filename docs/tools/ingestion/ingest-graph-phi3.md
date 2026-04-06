# ingest_graph_phi3.py

## Zweck

CLI-Werkzeug fuer den Import von Dateien aus `Y:\data` in eine ThemisDB-Testinstanz mit Phi3-gestuetzter Graph-Extraktion.

Seit dieser Version gibt es zusaetzlich:

- eine wiederverwendbare Library `tools/ingest_graph_phi3_lib.py`
- eine lokale GUI `tools/ingest_graph_phi3_gui.py` mit Live-Feedback

Das Skript:

- liest Dateien rekursiv aus einem Quellordner,
- extrahiert per `/api/v1/llm/inference` Knoten und Kanten,
- speichert Graph-Knoten als Entities (`/entities`),
- schreibt Graph-Kanten in den Graph-Index (`/graph/edge`).

## Voraussetzungen

- laufende ThemisDB mit HTTP-API (Standard `http://127.0.0.1:8765`)
- LLM-API aktiv (`/api/v1/llm/*`)
- gueltiger Bearer Token (falls Auth aktiviert)
- Python 3.8+

## Schnellstart

```powershell
python tools/ingest_graph_phi3.py \
  --source Y:\data \
  --themis-url http://127.0.0.1:8765 \
  --model-id phi3 \
  --bearer-token <JWT_TOKEN>
```

## GUI Start

```powershell
python tools/ingest_graph_phi3_gui.py
```

Die GUI zeigt waehrend des Laufs in Echtzeit:

- Fortschrittsbalken (bezogen auf gefundene Kandidatendateien)
- Zaehler fuer Scanned, Processed, Skipped, Failed
- aktuelle Node-/Edge-Anzahl
- Prozess-Feedback und Fehler im Logfenster

Optional kann der Token auch ueber die Umgebungsvariable `THEMIS_BEARER_TOKEN` gesetzt werden.

## Wichtige Optionen

- `--source`: Quellverzeichnis (Default `Y:\data`)
- `--themis-url`: Basis-URL der Test-DB
- `--model-id`: LLM Modell-ID (Default `phi3`)
- `--model-path`: optionaler Modellpfad fuer `/api/v1/llm/models/load`
- `--skip-model-load`: Modell-Load ueberspringen
- `--dry-run`: nur Analyse, keine Schreibvorgaenge
- `--max-file-size-mb`: Dateigroessen-Limit
- `--max-chars-per-file`: Textlaenge pro Datei fuer den LLM-Prompt
- `--max-files`: verarbeitet nur N Dateien (0 = unbegrenzt)

## Dry-Run Beispiel

```powershell
python tools/ingest_graph_phi3.py \
  --source Y:\data \
  --dry-run \
  --bearer-token <JWT_TOKEN>
```

In der GUI kann Dry-Run ueber die Checkbox `Dry Run` aktiviert werden.

## Library Nutzung

Die eigentliche Ingestion-Logik liegt in `tools/ingest_graph_phi3_lib.py`.
CLI und GUI verwenden dieselbe `run_ingestion(...)`-Funktion.

Damit sind folgende Integrationen moeglich:

- eigene Frontends (z. B. WPF, WebUI)
- orchestrierte Batch-Laeufe mit Callback-Feedback
- automatisierte Tests gegen die Ingestion-Pipeline

## Persistenzmodell

- **Nodes**: `PUT /entities/{key}` mit `blob`-Payload
  - Key-Format: `graph.default.nodes:<hash>`
  - Blob enthaelt `vertex_id`, `label`, `type`, `properties`, `source_doc`
- **Edges**: `POST /graph/edge`
  - Pflichtfelder: `id`, `_from`, `_to`
  - optional: `type` und weitere Properties

## Hinweise

- Das Skript verwendet eine robuste JSON-Extraktion aus LLM-Text (auch fuer fenced code blocks).
- Binaere Dateien werden uebersprungen.
- Bei Teilfehlern laeuft die Verarbeitung weiter; Fehler werden im Log ausgegeben.
