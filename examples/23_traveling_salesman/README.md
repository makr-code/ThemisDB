> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Traveling Salesman Problem (TSP) - Routenoptimierung

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-medium-orange)
![Duration](https://img.shields.io/badge/duration-40--50%20min-blue)

## 📝 Übersicht

Dieses Beispiel zeigt, wie das klassische **Traveling Salesman Problem (TSP)** mit ThemisDB gelöst werden kann. Das TSP ist ein bekanntes Optimierungsproblem, bei dem ein Handelsreisender die kürzeste Route finden muss, um mehrere Städte zu besuchen und zum Ausgangspunkt zurückzukehren.

## ✨ Features

- ✅ **Graphenmodellierung** - Städte als Knoten, Straßen als gewichtete Kanten
- ✅ **Mehrere Algorithmen** - Brute Force, Greedy, 2-Opt Heuristik
- ✅ **Interaktive Visualisierung** - Matplotlib-Integration für Routen
- ✅ **Routenvergleich** - Vergleich verschiedener Lösungsansätze
- ✅ **Performance-Metriken** - Laufzeit und Routenlänge
- ✅ **Anpassbare Daten** - Eigene Städte und Distanzen hinzufügen
- ✅ **Export-Funktion** - Ergebnisse als JSON oder CSV exportieren

## 📊 Problem-Beschreibung

### Das Traveling Salesman Problem

Ein Handelsreisender muss **n Städte** besuchen und zum Ausgangspunkt zurückkehren. Die Herausforderung:

- Jede Stadt soll **genau einmal** besucht werden
- Die **Gesamtdistanz** soll minimal sein
- Es gibt **n!** mögliche Routen (faktorielles Wachstum)

### Beispiel

```
Gegeben: 4 Städte (A, B, C, D) mit Distanzen

    A ---10--- B
    |          |
    15        20
    |          |
    C ---25--- D

Mögliche Routen: 4! = 24 Routen
Optimale Route: A → B → D → C → A (Länge: 70)
```

### Komplexität

- **Exakte Lösung**: O(n!) - praktisch nur für kleine n
- **Heuristische Ansätze**: O(n²) bis O(n³) - für größere Probleme
- **Approximation**: Garantie von z.B. 1.5× optimal (Christofides)

## 🖼️ Screenshots

*Screenshots der Anwendung werden nach Implementierung hinzugefügt*

## 📋 Voraussetzungen

### ThemisDB Server

```bash
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  themisdb/themisdb:latest

# Server-Status prüfen
curl http://localhost:8080/health
```

### Python und Dependencies

- Python 3.8 oder höher
- Tkinter (Standard-Bibliothek)
- matplotlib, numpy (für Visualisierung)

## 🚀 Installation

1. **Navigieren Sie zum Beispiel-Verzeichnis**:
   ```bash
   cd examples/23_traveling_salesman
   ```

2. **Installieren Sie die Abhängigkeiten**:
   ```bash
   pip install -r requirements.txt
   ```

## 🎮 Verwendung

### Anwendung starten

```bash
python main.py
```

### Grundlegende Operationen

1. **Städte verwalten**:
   - Neue Stadt hinzufügen mit Namen und Koordinaten
   - Städte anzeigen und bearbeiten
   - Distanzen werden automatisch berechnet (euklidische Distanz)

2. **Routen berechnen**:
   - **Brute Force**: Findet garantiert die optimale Lösung (nur für kleine n)
   - **Greedy**: Schneller Algorithmus, wählt immer die nächste Stadt
   - **2-Opt**: Verbessert eine gegebene Route iterativ
   - **Nearest Neighbor**: Beginnt bei einer Stadt, besucht immer die nächste

3. **Ergebnisse visualisieren**:
   - Route auf Karte anzeigen
   - Vergleich verschiedener Algorithmen
   - Performance-Metriken (Zeit, Distanz)

4. **Daten exportieren**:
   - Route als JSON exportieren
   - Distanzmatrix als CSV speichern

Siehe [HOW_TO.md](HOW_TO.md) für detaillierte Schritt-für-Schritt-Anleitungen.

## 📊 Datenmodell

### Stadt (City)
```python
{
    "id": "city_uuid",
    "name": "Berlin",
    "x": 52.52,          # Breitengrad
    "y": 13.405,         # Längengrad
    "country": "Deutschland"
}
```

### Verbindung (Connection)
```python
{
    "from": "city1_id",
    "to": "city2_id",
    "distance": 350.5,   # in km
    "type": "road"
}
```

### Route
```python
{
    "id": "route_uuid",
    "cities": ["city1_id", "city2_id", ...],
    "total_distance": 1250.5,
    "algorithm": "2-opt",
    "computation_time": 0.125  # in Sekunden
}
```

## 🏗️ Architektur

```
┌─────────────────────┐
│   Tkinter GUI       │  ← Benutzeroberfläche
│  - Stadt-Manager    │
│  - Algorithmus-Wahl │
│  - Visualisierung   │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│  TSP Algorithms     │  ← Lösungsverfahren
│  - Brute Force      │
│  - Greedy           │
│  - 2-Opt            │
│  - Nearest Neighbor │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│ ThemisDB Client     │  ← Graph-Operationen
│  - Knoten (Städte)  │
│  - Kanten (Wege)    │
│  - Distanz-Queries  │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│  ThemisDB Server    │  ← Property Graph
└─────────────────────┘
```

## 📁 Dateien

- `main.py` - Hauptanwendung mit Tkinter-UI
- `themis_client.py` - ThemisDB-Client für Graph-Operationen
- `tsp_algorithms.py` - TSP-Lösungsverfahren
- `models.py` - Datenmodelle (City, Route)
- `ALGORITHM.md` - Detaillierte Algorithmen-Erklärung
- `README.md` - Diese Datei
- `HOW_TO.md` - Bedienungsanleitung
- `requirements.txt` - Python-Abhängigkeiten

## 📚 Was Sie lernen

- **Graph-Theorie** - Gewichtete Graphen, Hamilton-Kreise
- **Optimierung** - Kombinatorische Optimierungsprobleme
- **Heuristiken** - Approximationsalgorithmen für NP-schwere Probleme
- **ThemisDB Property Graph** - Knoten mit Labels, gewichtete Kanten
- **Algorithmen-Vergleich** - Trade-offs zwischen Laufzeit und Qualität
- **Visualisierung** - Matplotlib-Integration mit Tkinter

## 🎓 Algorithmen

### 1. Brute Force (Exakt)
- **Komplexität**: O(n!)
- **Garantie**: Optimale Lösung
- **Geeignet für**: n ≤ 10 Städte

### 2. Greedy / Nearest Neighbor
- **Komplexität**: O(n²)
- **Garantie**: Keine (oft 25% über optimal)
- **Geeignet für**: Schnelle Näherungslösung

### 3. 2-Opt Heuristik
- **Komplexität**: O(n²) pro Iteration
- **Garantie**: Lokales Optimum
- **Geeignet für**: Verbesserung von Greedy-Lösungen

### 4. Christofides Algorithmus (Bonus)
- **Komplexität**: O(n³)
- **Garantie**: Maximal 1.5× optimal
- **Geeignet für**: Gute Approximation mit Garantie

## 🔧 Konfiguration

Die Standard-Konfiguration in `main.py`:

```python
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
DEFAULT_ALGORITHM = "2-opt"
WINDOW_WIDTH = 1200
WINDOW_HEIGHT = 800
```

## 🐛 Troubleshooting

### Server nicht erreichbar

```
Error: Connection refused
```

**Lösung**: ThemisDB-Server starten
```bash
docker ps  # Prüfen ob Server läuft
docker start themisdb  # Oder neu starten
```

### Matplotlib/Tkinter Fehler

```
Error: No module named 'matplotlib'
```

**Lösung**: 
```bash
pip install matplotlib numpy
```

### Lange Berechnungszeit bei Brute Force

**Lösung**: Brute Force ist nur für n ≤ 10 Städte geeignet. Verwenden Sie Heuristiken für größere Probleme.

## 🔗 Verwandte Beispiele

- [06 - Soziales Netzwerk](../06_graph_social_network/) - Graph-Traversierung und Algorithmen
- [04 - Inventarsystem](../04_inventory_system/) - Graph-Beziehungen
- [19 - Recommendation Engine](../19_recommendation_engine/) - Optimierungsalgorithmen

## 📚 Weiterführende Ressourcen

- [ThemisDB Graph Features](../../docs/de/features/features_property_graph.md)
- [AQL Graph Queries](../../docs/de/aql/aql_syntax.md)
- [TSP Wikipedia](https://en.wikipedia.org/wiki/Travelling_salesman_problem)
- [Christofides Algorithm](https://en.wikipedia.org/wiki/Christofides_algorithm)

## 🤝 Beitragen

Verbesserungsvorschläge und zusätzliche Algorithmen sind willkommen! Öffnen Sie ein Issue oder Pull Request auf GitHub.

## 📄 Lizenz

Dieses Beispiel ist unter der MIT-Lizenz lizenziert - siehe [LICENSE](../../LICENSE) für Details.

---

**Nächste Schritte**: Erkunden Sie [ALGORITHM.md](ALGORITHM.md) für detaillierte Erklärungen der TSP-Algorithmen.
