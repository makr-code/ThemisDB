# ThemisDB Schulungsbeispiele

Praxisnahe, lauffähige Code-Beispiele für die ThemisDB-Schulung.

## Voraussetzungen

```bash
# ThemisDB starten
docker run -d --name themisdb -p 8080:8080 themisdb/themisdb:latest

# Python-Client installieren
pip install themis-client
```

## Verzeichnisstruktur

| Verzeichnis | Thema | Schwierigkeit |
|---|---|---|
| `01_grundlegende_operationen/` | CRUD, Collections, Transaktionen | ⭐ Einsteiger |
| `02_aql_queries/` | Filters, Joins, Aggregation, Subqueries | ⭐⭐ Fortgeschritten |
| `03_graph_daten/` | Graphmodell, Traversierungen, Pfade | ⭐⭐ Fortgeschritten |
| `04_multimodell_anwendung/` | Vollständige Multi-Model-Anwendung | ⭐⭐⭐ Experte |

## Reihenfolge

1. `01_grundlegende_operationen/` — Basis-Setup und CRUD
2. `02_aql_queries/` — AQL vertiefen
3. `03_graph_daten/` — Graph-Features erkunden
4. `04_multimodell_anwendung/` — Alles zusammen

## Ausführen

```bash
cd schulung/examples/01_grundlegende_operationen
python main.py

# Oder interaktiv im Jupyter-Notebook (falls vorhanden)
jupyter notebook
```

## Hilfe

- [AQL Referenz](../dokumente/02_aql_referenz_kurzuebersicht.md)
- [Übungsaufgaben](../dokumente/04_uebungsaufgaben.md)
- [Vollständige Beispiele](../../examples/)
