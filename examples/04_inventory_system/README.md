> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Inventarsystem - Lagerverwaltung mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-medium-orange)
![Duration](https://img.shields.io/badge/duration-30--40%20min-blue)

## 📝 Übersicht

Ein vollständiges Inventarsystem mit Produktverwaltung, Bestandsverfolgung und Lieferanten-Beziehungen. Demonstriert Multi-Model Features (Relational + Graph).

## ✨ Features

- ✅ **Produktverwaltung** - SKU, Name, Beschreibung, Preis
- ✅ **Bestandsverfolgung** - Aktuelle Menge, Mindestbestand, Lagerort
- ✅ **Lieferanten-Graph** - Beziehungen zwischen Produkten und Lieferanten
- ✅ **Bestandshistorie** - Chronologische Aufzeichnung aller Bewegungen
- ✅ **Warnungen** - Automatische Alerts bei niedrigem Bestand
- ✅ **Dashboard** - Statistiken und Charts
- ✅ **Transaktionen** - ACID-Garantien für Bestandsänderungen
- ✅ **Barcode-Support** - QR-Code-Generierung

## 🖼️ Screenshots

*Screenshots werden nach Implementierung hinzugefügt*

## 📋 Voraussetzungen

- ThemisDB Server
- Python 3.8+
- matplotlib (für Charts)

## 🚀 Installation

```bash
cd examples/04_inventory_system
pip install -r requirements.txt
python main.py
```

## 📊 Datenmodell

### Produkt (Relational)
```python
{
    "id": "product_uuid",
    "sku": "PROD-001",
    "name": "Produktname",
    "description": "Beschreibung",
    "price": 29.99,
    "quantity": 100,
    "min_quantity": 20,
    "location": "Lager A, Regal 3",
    "category": "Elektronik",
    "created_at": "2025-12-22T10:00:00Z"
}
```

### Lieferant-Beziehung (Graph)
```python
# Edge: Product -> Supplier
{
    "from": "product_uuid",
    "to": "supplier_uuid",
    "relationship": "supplied_by",
    "lead_time_days": 14,
    "unit_price": 19.99
}
```

### Bestandsbewegung (Time-Series)
```python
{
    "id": "movement_uuid",
    "product_id": "product_uuid",
    "type": "in",  # in, out, adjustment
    "quantity": 50,
    "reason": "Lieferung von Lieferant XYZ",
    "timestamp": "2025-12-22T10:00:00Z",
    "user": "admin"
}
```

## 🔧 Verwendung

Siehe [HOW_TO.md](HOW_TO.md) und [DATA_MODEL.md](DATA_MODEL.md) für Details.

## 📚 Was Sie lernen

- **Multi-Model** - Kombination von Relational, Graph, Time-Series
- **Transaktionen** - Atomare Bestandsänderungen
- **Graph-Queries** - Lieferanten-Pfade finden
- **Aggregationen** - Statistiken berechnen
- **Charts** - Daten mit matplotlib visualisieren
- **Complex UI** - Tabbed Interface mit tkinter

## 📄 Dateien

- `README.md` - Diese Datei
- `HOW_TO.md` - Bedienungsanleitung
- `DATA_MODEL.md` - Datenmodell-Dokumentation
- `API_USAGE.md` - Code-Beispiele
- `main.py` - Hauptanwendung
- `models.py` - Datenmodelle
- `graph_manager.py` - Graph-Operationen
- `statistics.py` - Statistik-Berechnungen

## 🎯 Use Cases

1. **Wareneingang buchen**
2. **Warenausgang verbuchen**
3. **Lieferanten verwalten**
4. **Mindestbestände überwachen**
5. **Bestandshistorie anzeigen**
6. **Statistiken und Reports**

## 🔗 Weiterführend

- Vorheriges Beispiel: [Kontaktmanager](../03_contact_manager/)
- Nächstes Beispiel: [Zeitreihen-Monitor](../05_time_series_monitor/)

---

**Status**: Geplant | Implementierung steht bevor
