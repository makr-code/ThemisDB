> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Soziales Netzwerk - Bedienungsanleitung

## 🚀 Start

```bash
python main.py
```

## 📋 Hauptfunktionen

### Profil erstellen

1. "Neuer Benutzer"
2. Name, Bio, Interessen eingeben
3. Profilbild hochladen (optional)
4. Speichern

### Freundschaften

**Freund hinzufügen**:
1. Benutzer suchen
2. "Freundschaft anfragen"
3. Anderer Benutzer bestätigt

**Freunde anzeigen**:
- Liste aller Freunde
- Sortierung nach Name, Datum
- Interaktions-Stärke

### Graph erkunden

**Freunde von Freunden**:
1. Benutzer auswählen
2. "Netzwerk anzeigen"
3. Zeigt Graph bis Tiefe 2-3

**Kürzester Pfad**:
1. Start-Benutzer wählen
2. Ziel-Benutzer wählen
3. "Pfad finden"
4. Zeigt Verbindung über Freunde

**Communities**:
- "Community-Erkennung starten"
- Algorithmus findet Gruppen
- Farbcodierte Darstellung

### Empfehlungen

**Freunde vorschlagen**:
- Basiert auf gemeinsamen Freunden
- Ähnliche Interessen
- Gleicher Standort

## 💡 Algorithmen

- **BFS**: Breiten-Suche
- **Dijkstra**: Kürzeste Pfade
- **Louvain**: Community-Detection

---

**Weitere Details**: Siehe GRAPH_THEORY.md
