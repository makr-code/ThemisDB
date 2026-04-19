> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# CRM - Customer Relationship Management mit ThemisDB

![Status](https://img.shields.io/badge/status-ready-brightgreen)
![Difficulty](https://img.shields.io/badge/difficulty-medium-yellow)
![Duration](https://img.shields.io/badge/duration-60--90%20min-blue)

## 📝 Übersicht

Das CRM-System demonstriert Customer Relationship Management mit ThemisDB. Sie lernen:
- Kundenverwaltung mit Kontakthistorie
- Lead-Scoring und Qualification
- Sales-Pipeline Visualisierung
- Activity Timeline
- Email/Call-Logging
- Reporting und Analytics

## ✨ Features

- ✅ **Kundenverwaltung** - Vollständige Kundenprofile
- ✅ **Lead-Management** - Scoring und Qualification
- ✅ **Sales-Pipeline** - Opportunity-Tracking
- ✅ **Activity-Timeline** - Alle Interaktionen
- ✅ **Communication-Log** - Emails, Calls, Meetings
- ✅ **Reporting** - Sales-Analytics
- ✅ **Forecasting** - Revenue-Prognosen
- ✅ **Integration** - Email und Kalender

## 📊 Datenmodell

### Customer

```json
{
  "id": "customer_uuid",
  "name": "Acme Corporation",
  "type": "enterprise",
  "industry": "Technology",
  "revenue": 5000000,
  "employees": 250,
  "contact_person": "Jane Smith",
  "email": "jane@acme.com",
  "phone": "+49 30 12345678",
  "status": "active",
  "lead_score": 85,
  "lifetime_value": 125000,
  "created_at": "2024-01-15"
}
```

### Lead

```json
{
  "id": "lead_uuid",
  "customer_id": "customer_uuid",
  "title": "Q1 2025 Enterprise License",
  "value": 50000,
  "stage": "negotiation",
  "probability": 75,
  "expected_close": "2025-01-31",
  "assigned_to": "sales.rep",
  "source": "website"
}
```

## 🛠️ ThemisDB Features

- **Relational** für Kunden und Leads
- **Time-Series** für Activities
- **Analytics** für Reporting

## 🔗 Navigation

- ⬅️ [16 - Kanban Board](../16_kanban_board/)
- ➡️ [18 - Real-Time Chat](../18_realtime_chat/)
- 🏠 [Übersicht](../README.md)

---

**Status**: Ready | **Letzte Aktualisierung**: 2025-12-22
