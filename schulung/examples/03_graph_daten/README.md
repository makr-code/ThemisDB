# Graph-Daten — Beispiele

![Schwierigkeit](https://img.shields.io/badge/schwierigkeit-fortgeschritten-orange)
![Dauer](https://img.shields.io/badge/dauer-30--45%20min-blue)

## Übersicht

Dieses Beispiel demonstriert das Graph-Modell von ThemisDB anhand eines sozialen Netzwerks:

- Vertex- und Edge-Collections erstellen
- Graphen mit mehreren Edge-Definitionen
- OUTBOUND/INBOUND/ANY Traversierungen
- Tiefensteuerung (1..3)
- Kürzeste Pfade (SHORTEST_PATH, ALL_SHORTEST_PATHS)
- Kanten-Filter (Beziehungsgewichte)
- Grad-Analyse (In-/Out-Degree)
- Empfehlungsalgorithmus (Freunde von Freunden)

## Ausführen

```bash
cd schulung/examples/03_graph_daten
python main.py
```

## Netzwerk-Topologie

```
alice ──follows──▶ bob ──follows──▶ david
  │                 │                  │
  │              follows            follows
  │                 │                  │
  ▼                 ▼                  ▼
clara ──follows──▶ frank ◀──follows── eva
```

## Wichtige AQL-Konzepte

### Graph-Traversierung
```aql
FOR vertex, edge, path
  IN min_depth..max_depth
  OUTBOUND|INBOUND|ANY start_vertex
  GRAPH "graph_name"
  [FILTER condition]
  RETURN vertex
```

### Kürzester Pfad
```aql
FOR path IN OUTBOUND SHORTEST_PATH from TO to GRAPH "g"
  RETURN path.vertices[*].name
```

### Kanten-Filter
```aql
FOR v, e IN 1..2 OUTBOUND start GRAPH "g"
  FILTER e.weight > 0.7   -- nur starke Verbindungen
  RETURN v
```

## Weiterführend

- [Nächstes Beispiel: Multi-Model-Anwendung](../04_multimodell_anwendung/)
- [Graph-Beispiel aus den Hauptexamples](../../../examples/06_graph_social_network/)
