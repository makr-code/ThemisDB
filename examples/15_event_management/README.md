> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Event Management System - Veranstaltungsmanagement mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-medium-yellow)
![Duration](https://img.shields.io/badge/duration-60%20min-blue)

## 📝 Übersicht

Das Event Management System verwaltet Veranstaltungen und Teilnehmer. Sie lernen:
- Event-Erstellung und -Verwaltung
- Teilnehmer-Registrierung
- Ticketing-System
- Kalender-Integration
- Reminder/Benachrichtigungen
- Check-In Tracking

## ✨ Features

- ✅ **Event-Verwaltung** - Erstellen und Verwalten
- ✅ **Ticketing** - Verschiedene Ticket-Typen
- ✅ **Registrierung** - Online-Anmeldung
- ✅ **Kalender** - Übersicht und Integration
- ✅ **Reminders** - Automatische Benachrichtigungen
- ✅ **Check-In** - Tracking und Statistiken
- ✅ **Kapazitäten** - Limit-Management
- ✅ **Berichte** - Event-Auswertungen

## 📊 Datenmodell

### Event

```json
{
  "id": "event_uuid",
  "title": "Tech Conference 2025",
  "description": "Jährliche Tech-Konferenz",
  "location": "Convention Center Berlin",
  "start_date": "2025-06-15T09:00:00Z",
  "end_date": "2025-06-17T18:00:00Z",
  "capacity": 500,
  "registered": 342,
  "ticket_types": [
    {"type": "Early Bird", "price": 199, "available": 0},
    {"type": "Regular", "price": 299, "available": 158}
  ],
  "tags": ["technology", "networking", "conference"]
}
```

## 🛠️ ThemisDB Features

- **Relational** für Events und Teilnehmer
- **Time-Series** für Check-Ins
- **CEP** für Reminders und Notifications

## 🔗 Navigation

- ⬅️ [14 - E-Commerce](../14_ecommerce_catalog/)
- ➡️ [16 - Kanban Board](../16_kanban_board/)
- 🏠 [Übersicht](../README.md)

---

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-22
