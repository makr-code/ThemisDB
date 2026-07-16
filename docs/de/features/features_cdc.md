---

---
category: "🔄 Data Operations"
version: "v1.5.0"
status: "✅"
date: "2026-03-09"
---

# 📊 CDC Module

Change Data Capture Implementation mit Sequence-tracking, SSE/WebSocket-Streaming und Enterprise-Integration.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

## 📋 Übersicht
**Version:** 1.5.0  
**Kategorie:** Features

---

Die vollständige, aktuell gegen den Sourcecode abgeglichene Dokumentation befindet sich in:

- **Ausführliche Anleitung (DE):** [`features_change_data_capture.md`](features_change_data_capture.md)
- **Primäre Modul-Doku:** [`src/cdc/README.md`](../../../src/cdc/README.md)
- **Architektur:** [`src/cdc/ARCHITECTURE.md`](../../../src/cdc/ARCHITECTURE.md)
- **Operations Runbook:** [`docs/CDC_OPERATIONS_RUNBOOK.md`](../../CDC_OPERATIONS_RUNBOOK.md)

## ✨ Features (Kurzübersicht)

| Feature | Status |
|---------|--------|
| Changefeed Engine (insert/update/delete) | ✅ Production |
| SSE-Streaming | ✅ Production |
| WebSocket-Streaming | ✅ Production |
| Consumer Groups mit Offset-Tracking | ✅ Production |
| At-least-once Delivery + DLQ | ✅ Production |
| Kafka-Produzent (Debezium-Format) | ✅ Production (opt-in) |
| Transactional Outbox Pattern | ✅ Production |
| Cross-Collection Streams | ✅ Production |
| CDC-gesteuerte Materialized Views | ✅ Production |
| GDPR-konforme PII-Redaktion | ✅ Production |
| Change Stream Kompression | ✅ Production |

## 📚 Siehe auch

- [features_change_data_capture.md](features_change_data_capture.md) — Vollständige End-User-Dokumentation
- [CDC Operations Runbook](../../CDC_OPERATIONS_RUNBOOK.md) — Betrieb & Troubleshooting
- [src/cdc/ROADMAP.md](../../../src/cdc/ROADMAP.md) — Statusübersicht aller Features
- [Fehlende Implementierungen (Befund-Report)](../cdc/MISSING_IMPLEMENTATIONS.md) — Reality-Check-Befunde

