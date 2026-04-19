> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Rezeptverwaltung / Recipe Manager - Kulinarisches Management mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-easy-green)
![Duration](https://img.shields.io/badge/duration-30--40%20min-blue)

## 📝 Übersicht

Der Recipe Manager ist eine Anwendung zur Verwaltung von Rezepten und Zutaten. Sie lernen:
- Rezepte mit Zutaten-Listen zu erstellen
- Nach Zutaten und Tags zu suchen
- Nährwerte zu berechnen
- Einkaufslisten zu generieren
- Favoriten zu verwalten

## ✨ Features

- ✅ **Rezeptverwaltung** - Erstellen, Bearbeiten, Löschen
- ✅ **Zutaten-Listen** - Mit Mengen und Einheiten
- ✅ **Kategorien** - Vorspeise, Hauptgericht, Dessert
- ✅ **Tag-System** - Vegetarisch, Vegan, Glutenfrei
- ✅ **Suche** - Nach Zutaten, Tags, Name
- ✅ **Nährwertberechnung** - Kalorien, Protein, etc.
- ✅ **Einkaufsliste** - Automatische Generierung
- ✅ **Favoriten** - Lieblingsrezepte markieren
- ✅ **Portionsanpassung** - Automatische Umrechnung

## 📊 Datenmodell

### Recipe

```json
{
  "id": "recipe_uuid",
  "name": "Spaghetti Carbonara",
  "category": "Hauptgericht",
  "cuisine": "Italienisch",
  "tags": ["pasta", "schnell", "einfach"],
  "servings": 4,
  "prep_time_minutes": 10,
  "cook_time_minutes": 20,
  "ingredients": [
    {"name": "Spaghetti", "amount": 400, "unit": "g"},
    {"name": "Eier", "amount": 4, "unit": "Stück"},
    {"name": "Parmesan", "amount": 100, "unit": "g"}
  ],
  "instructions": ["Schritt 1...", "Schritt 2..."],
  "nutrition": {
    "calories": 550,
    "protein": 25,
    "carbs": 65,
    "fat": 18
  }
}
```

## 🔗 Navigation

- ⬅️ [12 - Expense Tracker](../12_expense_tracker/)
- ➡️ [14 - E-Commerce](../14_ecommerce_catalog/)
- 🏠 [Übersicht](../README.md)

---

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-22
