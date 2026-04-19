> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Kontaktmanager - Adressbuch mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-easy-green)
![Duration](https://img.shields.io/badge/duration-15--20%20min-blue)

## 📝 Übersicht

Der Kontaktmanager ist ein vollwertiges Adressbuch mit Volltext-Suche, Kategorisierung und Export/Import-Funktionen. Demonstriert das Document Model von ThemisDB.

## ✨ Features

- ✅ **Kontakte verwalten** - Name, Email, Telefon, Adresse
- ✅ **Kategorien** - Freunde, Familie, Arbeit, Sonstiges
- ✅ **Volltext-Suche** - Suche über alle Felder
- ✅ **Export/Import** - JSON und CSV Format
- ✅ **Detail-Ansicht** - Ausführliche Kontaktinformationen
- ✅ **Favoriten** - Wichtige Kontakte markieren
- ✅ **Notizen** - Zusätzliche Informationen speichern

## 🖼️ Screenshots

*Screenshots werden nach Implementierung hinzugefügt*

## 📋 Voraussetzungen

- ThemisDB Server
- Python 3.8+

## 🚀 Installation

```bash
cd examples/03_contact_manager
pip install -r requirements.txt
python main.py
```

## 📊 Datenmodell

```python
{
    "id": "contact_uuid",
    "first_name": "Max",
    "last_name": "Mustermann",
    "email": "max@example.com",
    "phone": "+49 123 456789",
    "address": {
        "street": "Musterstraße 1",
        "city": "Berlin",
        "postal_code": "10115",
        "country": "Deutschland"
    },
    "category": "friends",  # friends, family, work, other
    "is_favorite": false,
    "notes": "Notizen zum Kontakt",
    "created_at": "2025-12-22T10:00:00Z",
    "updated_at": "2025-12-22T10:00:00Z"
}
```

## 🔧 Verwendung

Siehe [HOW_TO.md](HOW_TO.md) für detaillierte Anleitungen.

## 📚 Was Sie lernen

- **Document Model** - Flexible JSON-Dokumente
- **Volltext-Suche** - AQL Text-Queries
- **Kategorisierung** - Daten organisieren
- **Export/Import** - Daten austauschen
- **Komplexe Layouts** - Master-Detail-UI

## 📄 Dateien

- `README.md` - Diese Datei
- `HOW_TO.md` - Bedienungsanleitung
- `TUTORIAL.md` - Anfänger-Tutorial
- `main.py` - Hauptanwendung
- `models.py` - Datenmodelle
- `export_handler.py` - Import/Export-Logik

## 🔗 Weiterführend

- Vorheriges Beispiel: [Todo-App](../02_todo_app/)
- Nächstes Beispiel: [Inventarsystem](../04_inventory_system/)

---

**Status**: ✅ Implementiert | Voll funktionsfähig
