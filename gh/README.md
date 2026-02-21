# GitHub Issue Manager - AI Edition

Vollautomatisches System zur Verwaltung von GitHub Issues aus ROADMAP.md Dateien mit KI-Unterstützung durch Ollama.

## 📋 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Features](#features)
- [Voraussetzungen](#voraussetzungen)
- [Installation](#installation)
- [Schnellstart](#schnellstart)
- [Konfiguration](#konfiguration)
- [Verwendung](#verwendung)
- [ROADMAP Format](#roadmap-format)
- [AI-Integration](#ai-integration)
- [Workflows](#workflows)
- [Troubleshooting](#troubleshooting)
- [Best Practices](#best-practices)
- [API-Referenz](#api-referenz)
- [FAQ](#faq)
- [Lizenz](#lizenz)

---

## 🎯 Übersicht

Das **GitHub Issue Manager - AI Edition** ist ein PowerShell-basiertes System, das automatisch GitHub Issues aus strukturierten ROADMAP.md Dateien erstellt. Es nutzt lokale KI-Modelle (Ollama) zur Optimierung von Issue-Titeln und -Beschreibungen.

### Hauptziele

- **Automatisierung**: Reduziert manuelle Arbeit bei der Issue-Erstellung
- **Konsistenz**: Einheitliche Issue-Struktur über alle Module
- **KI-Optimierung**: Verbesserte Titel und Beschreibungen durch lokale LLMs
- **Intelligentes Matching**: Vermeidet Duplikate durch Similarity-Algorithmen
- **Flexibilität**: Unterstützt verschiedene ROADMAP-Formate

---

## ✨ Features

### Core Features

- ✅ **Interaktives CLI-Interface** - Benutzerfreundliche Menüführung
- ✅ **Universal ROADMAP Parser** - Unterstützt verschiedene Markdown-Formate
- ✅ **Intelligente Duplikat-Erkennung** - 85% Similarity-Threshold
- ✅ **Batch-Issue-Erstellung** - Mehrere Issues mit einem Klick
- ✅ **Dry-Run Modus** - Vorschau ohne echte Änderungen
- ✅ **Multi-Modul Support** - Verwaltung mehrerer Repositories/Module

### AI-Features (Optional)

- 🤖 **Ollama-Integration** - Lokale LLM-Unterstützung
- 🤖 **Titel-Optimierung** - KI-verbesserte Issue-Titel
- 🤖 **Kontext-Bewusst** - Modul-spezifische Optimierungen
- 🤖 **Offline-fähig** - Keine Cloud-API benötigt

### Sicherheit & Qualität

- 🔒 **GitHub CLI Authentication** - Sichere Token-basierte Auth
- 🔒 **Input-Validation** - Escaped JSON für API-Calls
- 🔒 **Error-Handling** - Robuste Fehlerbehandlung
- 🔒 **PowerShell 5.1 Kompatibel** - Läuft auf Windows Server

---

## 📦 Voraussetzungen

### System-Anforderungen

| Komponente | Version | Erforderlich | Download |
|------------|---------|--------------|----------|
| **Windows** | 10/11 oder Server 2016+ | ✅ Ja | - |
| **PowerShell** | 5.1 oder höher | ✅ Ja | [Download](https://aka.ms/powershell) |
| **GitHub CLI** | 2.0+ | ✅ Ja | [Download](https://cli.github.com/) |
| **Ollama** | 0.1.0+ | ❌ Optional | [Download](https://ollama.ai/) |
| **Git** | 2.30+ | ⚠️ Empfohlen | [Download](https://git-scm.com/) |

### Berechtigungen

- **GitHub**: Schreibzugriff auf das Ziel-Repository
- **Dateisystem**: Lesezugriff auf ROADMAP.md Dateien
- **Netzwerk**: Zugriff auf GitHub API (api.github.com)
- **Ollama** (optional): Zugriff auf localhost:11434

---

## 🚀 Installation

### Schritt 1: GitHub CLI installieren

```powershell
# Windows (via winget)
winget install GitHub.cli

# Oder manuell von https://cli.github.com/ herunterladen