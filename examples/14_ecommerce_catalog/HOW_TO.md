> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# E-Commerce Katalog - Anleitung

## 🚀 Schnellstart

```bash
cd examples/14_ecommerce_catalog
pip install -r requirements.txt
python main.py
```

## 📖 Hauptfunktionen

### Produkte durchsuchen
- Suche nach Name, Kategorie, Marke
- Filter nach Preis, Bewertung
- Sortierung nach Relevanz, Preis, Bewertung

### Empfehlungen
- "Kunden kauften auch..." basiert auf Graph
- "Ähnliche Produkte" nutzt Vector Search
- Personalisierte Empfehlungen basierend auf Historie

### Bewertungen
- Produkte bewerten (1-5 Sterne)
- Reviews schreiben
- Hilfreiche Reviews markieren

---

**Letzte Aktualisierung**: 2025-12-22
