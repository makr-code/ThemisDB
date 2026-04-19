> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Haushaltsbuch / Expense Tracker - Finanzmanagement mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-easy-green)
![Duration](https://img.shields.io/badge/duration-30--40%20min-blue)

## 📝 Übersicht

Der Expense Tracker ist ein Haushaltsbuch zur Verwaltung persönlicher Finanzen. Sie lernen:
- Einnahmen und Ausgaben zu erfassen
- Transaktionen nach Kategorien zu organisieren
- Budgets zu verwalten und Warnungen zu setzen
- Finanzielle Statistiken und Trends zu visualisieren
- Monatliche und jährliche Auswertungen zu erstellen

Die Anwendung bietet eine Tkinter-GUI mit Diagrammen und Budget-Überwachung.

## ✨ Features

- ✅ **Transaktions-Verwaltung** - Einnahmen und Ausgaben erfassen
- ✅ **Kategorisierung** - Essen, Transport, Wohnen, etc.
- ✅ **Budget-Management** - Budgets setzen mit automatischen Warnungen
- ✅ **Statistiken** - Monatliche und jährliche Auswertungen
- ✅ **Visualisierungen** - Charts für Ausgaben-Verteilung
- ✅ **Trends** - Entwicklung über Zeit
- ✅ **Export** - CSV/PDF-Export für Berichte
- ✅ **Multi-Währung** - Unterstützung für verschiedene Währungen

## 📋 Voraussetzungen

### ThemisDB Server

```bash
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  themisdb/themisdb:latest
```

### Python und Dependencies

- Python 3.8 oder höher
- Tkinter (Standard-Bibliothek)
- matplotlib für Charts

## 🚀 Installation

```bash
cd examples/12_expense_tracker
pip install -r requirements.txt
python main.py
```

## 📊 Datenmodell

### Transaction (Transaktion)

```json
{
  "id": "trans_uuid",
  "type": "expense",
  "amount": 45.50,
  "currency": "EUR",
  "category": "Lebensmittel",
  "description": "Einkauf Supermarkt",
  "date": "2025-12-22",
  "created_at": "2025-12-22T10:00:00Z",
  "tags": ["grocery", "weekly"]
}
```

### Budget

```json
{
  "id": "budget_uuid",
  "category": "Lebensmittel",
  "amount": 400.00,
  "currency": "EUR",
  "period": "monthly",
  "start_date": "2025-12-01",
  "end_date": "2025-12-31",
  "alert_threshold": 0.8
}
```

### Category

```json
{
  "id": "cat_uuid",
  "name": "Lebensmittel",
  "type": "expense",
  "icon": "🛒",
  "color": "#FF6B6B"
}
```

## 🛠️ ThemisDB Features

- **Time-Series Model** für Transaktionen
- **Aggregationen** für Statistiken
- **Dashboard** mit Live-Updates
- **Secondary Indexes** auf Kategorie und Datum

## 📚 Was Sie lernen

1. Time-Series für finanzielle Daten
2. Aggregationen und Statistiken
3. Budget-Tracking und Alerts
4. Datenvisualisierung mit matplotlib

## 🔗 Navigation

- ⬅️ [11 - Blog/Wiki](../11_blog_wiki/)
- ➡️ [13 - Recipe Manager](../13_recipe_manager/)
- 🏠 [Übersicht](../README.md)

---

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-22
