# ThemisDB Examples (`examples/`)

Beispielsammlung für API-, Datenmodell- und Integrations-Workflows.

## Struktur (Top-Level)

- Numerische Lernpfade: `01_...` bis `24_...`
- Themenordner: `llm/`, `geo/`, `security/`, `performance/`, `replication/`, `migration/` u. a.

## Schnellstart (verifiziertes Grundmuster)

```bash
cd examples/01_hello_world
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python main.py
```

> Hinweis: Abhängigkeiten variieren je Beispiel; maßgeblich ist jeweils das lokale `README.md` im Beispielordner.

## Installation

Python-Abhängigkeiten werden pro Beispiel über die jeweilige `requirements.txt` installiert.

## Usage

Starten Sie Beispiele aus dem jeweiligen Beispielordner mit `python main.py` bzw. gemäß lokalem README.

## Navigation

- Gesamtprojekt: [`../README.md`](../README.md)
- Testabdeckung: [`../tests/README.md`](../tests/README.md)
- API-Doku: [`../docs/api/`](../docs/api/)
