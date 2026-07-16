> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Soziales Netzwerk - Graph-Visualisierung

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-medium-orange)
![Duration](https://img.shields.io/badge/duration-40--50%20min-blue)

## 📝 Übersicht

Soziales Netzwerk mit Freundschafts-Graphen, Community-Erkennung und interaktiver Visualisierung. Zeigt Graph-Features von ThemisDB.

## ✨ Features

- ✅ **Benutzerprofile** - Name, Bio, Interessen
- ✅ **Freundschaften** - Bidirektionale Beziehungen
- ✅ **Graph-Traversierung** - Freunde von Freunden (FoF)
- ✅ **Kürzeste Pfade** - Verbindung zwischen zwei Personen
- ✅ **Community-Erkennung** - Automatische Gruppen-Findung
- ✅ **Empfehlungs-Algorithmus** - Freunde vorschlagen
- ✅ **Interaktive Visualisierung** - NetworkX Integration

## 📊 Datenmodell

### Benutzer (Node)
```python
{
    "id": "user_uuid",
    "name": "Max Mustermann",
    "bio": "Software Engineer",
    "interests": ["Python", "Databases", "AI"],
    "location": "Berlin",
    "joined": "2025-01-15"
}
```

### Freundschaft (Edge)
```python
{
    "from": "user1_uuid",
    "to": "user2_uuid",
    "relationship": "friend",
    "since": "2025-03-20",
    "strength": 0.85  # 0-1, basierend auf Interaktionen
}
```

## 🔧 Verwendung

```bash
cd examples/06_graph_social_network
pip install -r requirements.txt
python main.py
```

Siehe [HOW_TO.md](HOW_TO.md) und [GRAPH_THEORY.md](GRAPH_THEORY.md).

## 📚 Was Sie lernen

- **Graph Model** - Knoten und Kanten
- **BFS/DFS** - Graph-Traversierungs-Algorithmen
- **Dijkstra** - Kürzeste Pfade
- **Community Detection** - Louvain-Algorithmus
- **NetworkX** - Graph-Visualisierung
- **AQL Graph Queries** - Graph-spezifische Queries

---

**Status**: Geplant
