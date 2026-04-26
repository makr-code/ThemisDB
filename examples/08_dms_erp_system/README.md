> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# DMS/ERP-System - Dokumentenmanagement

![Status](https://img.shields.io/badge/status-planned-yellow)
![Difficulty](https://img.shields.io/badge/difficulty-complex-red)
![Duration](https://img.shields.io/badge/duration-60--90%20min-blue)

## 📝 Übersicht

Vollständiges Document Management System (DMS) mit ERP-Features: Dokumentenverwaltung, Versionierung, Workflows, RBAC und Audit-Logging.

## ✨ Features

- ✅ **Dokumenten-Upload** - Alle Dateiformate mit Preview
- ✅ **Versionierung** - Automatische Versionshistorie
- ✅ **Metadaten & Tagging** - Flexible Kategorisierung
- ✅ **RBAC** - Rollen-basierte Zugriffskontrolle
- ✅ **Workflow-Engine** - Genehmigungsprozesse definieren
- ✅ **Volltext & Vector Search** - Hybrid-Suche
- ✅ **Audit-Log** - Vollständige Nachvollziehbarkeit
- ✅ **E-Signatur** - Digitale Signaturen
- ✅ **OCR** - Texterkennung in Bildern
- ✅ **Multi-Tenant** - Mandantenfähig

## 📊 Datenmodell

### Dokument
```python
{
    "id": "doc_uuid",
    "title": "Rechnung_2025_001.pdf",
    "type": "invoice",
    "version": 3,
    "current_version_id": "version_uuid",
    "metadata": {
        "invoice_number": "2025-001",
        "amount": 1299.99,
        "customer": "Firma XYZ",
        "date": "2025-12-22"
    },
    "tags": ["rechnung", "2025", "gezahlt"],
    "owner": "user_uuid",
    "permissions": [
        {"user": "user1", "role": "read"},
        {"user": "user2", "role": "write"}
    ],
    "workflow_state": "approved",
    "created": "2025-12-22T10:00:00Z"
}
```

### Workflow
```python
{
    "id": "workflow_uuid",
    "name": "Rechnungs-Genehmigung",
    "steps": [
        {
            "name": "Erfassung",
            "role": "employee",
            "actions": ["submit"]
        },
        {
            "name": "Prüfung",
            "role": "manager",
            "actions": ["approve", "reject"]
        },
        {
            "name": "Freigabe",
            "role": "director",
            "actions": ["sign", "reject"]
        }
    ]
}
```

## 🔧 Installation

```bash
cd examples/08_dms_erp_system
pip install -r requirements.txt
python main.py
```

## 📚 Dokumentation

- [README.md](README.md) - Diese Datei
- [HOW_TO.md](HOW_TO.md) - Benutzer-Handbuch
- [ADMIN_GUIDE.md](ADMIN_GUIDE.md) - Administrator-Anleitung
- [SECURITY.md](SECURITY.md) - Sicherheitskonzept
- [WORKFLOW_DESIGN.md](WORKFLOW_DESIGN.md) - Workflow-Konfiguration
- [API_REFERENCE.md](API_REFERENCE.md) - API-Dokumentation

## 📚 Was Sie lernen

- **Multi-Model Integration** - Alle ThemisDB-Features kombiniert
- **Transaktionen** - ACID für Workflows
- **Security** - RBAC, Encryption, Audit
- **Graph für Workflows** - Prozess-Modellierung
- **Vector Search** - Dokumenten-Ähnlichkeit
- **Complex UI** - Enterprise-Grade Interface

## 🎯 Use Cases

1. **Rechnungsverarbeitung**
2. **Vertragsmanagement**
3. **HR-Dokumenten-Verwaltung**
4. **Projektdokumentation**
5. **Compliance-Archivierung**

---

**Status**: Geplant | Enterprise-Grade Beispiel
