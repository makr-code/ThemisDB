> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# E-Commerce Produktkatalog - Multi-Model Showcase mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-medium-yellow)
![Duration](https://img.shields.io/badge/duration-60%20min-blue)

## 📝 Übersicht

Der E-Commerce Produktkatalog demonstriert Multi-Model Features von ThemisDB. Sie lernen:
- Produktverwaltung mit Varianten
- Graph-basierte Produktempfehlungen
- Vector Search für ähnliche Produkte
- Bewertungen und Reviews
- Inventory-Integration
- Preishistorie mit Time-Series

## ✨ Features

- ✅ **Produktkatalog** - Mit Varianten (Größe, Farbe)
- ✅ **Shopping Cart** - Warenkorb-Simulation
- ✅ **Empfehlungen** - "Kunden kauften auch..."
- ✅ **Ähnliche Produkte** - Vector Search
- ✅ **Reviews** - Bewertungen und Kommentare
- ✅ **Preishistorie** - Preisentwicklung über Zeit
- ✅ **Inventory** - Bestandsverwaltung
- ✅ **Suche** - Volltext und Filter

## 📊 Datenmodell

### Product

```json
{
  "id": "prod_uuid",
  "name": "T-Shirt Classic",
  "description": "Hochwertiges Baumwoll-T-Shirt",
  "category": "Bekleidung",
  "brand": "FashionBrand",
  "base_price": 29.99,
  "current_price": 24.99,
  "variants": [
    {"sku": "TS-BLK-M", "color": "Schwarz", "size": "M", "stock": 15},
    {"sku": "TS-BLK-L", "color": "Schwarz", "size": "L", "stock": 8}
  ],
  "tags": ["casual", "basic", "unisex"],
  "rating": 4.5,
  "review_count": 127
}
```

## 🛠️ ThemisDB Features

- **Relational** für Produkte und Varianten
- **Graph** für Empfehlungen
- **Vector Search** für Ähnlichkeit
- **Time-Series** für Preishistorie

## 🔗 Navigation

- ⬅️ [13 - Recipe Manager](../13_recipe_manager/)
- ➡️ [15 - Event Management](../15_event_management/)
- 🏠 [Übersicht](../README.md)

---

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-22
