# WordPress Category Extractor

Ein intelligentes Tool zur Extraktion sinnvoller Kategorien und Tags aus ThemisDB-Dokumentation für WordPress-Import.

## Problem

Das bisherige WordPress-Plugin für Tags/Kategorien generierte unsinnige Kategorien wie:
- `📁2026 Https,9 2026,governance,Januar 9,Januar 9 2026,knownlegde,Kritische Infrastrukturen,Multi Model,Themis,use`

Diese enthielten:
- Datumsangaben (2026, Januar 9)
- Ordnerpfade (📁)
- Zahlen ohne Kontext (9)
- Rechtschreibfehler (knownlegde)

## Lösung

Dieses Tool extrahiert **semantisch sinnvolle** Kategorien und Tags durch:

1. **Intelligente Pfad-Analyse**: Mapping von Ordnernamen zu semantischen Kategorien
2. **Filterung ungültiger Kategorien**: Entfernung von Daten, Zahlen, Monaten, etc.
3. **Content-basierte Tag-Extraktion**: Erkennung relevanter Themen im Dokumentinhalt
4. **WordPress-Integration**: Prüfung existierender Kategorien vor Erstellung neuer
5. **YAML Frontmatter Support**: Unterstützung expliziter Metadaten in Markdown-Dateien

## Verwendung

### Basis-Verwendung

```bash
# Kategorien aus docs/ extrahieren
python3 tools/wordpress_category_extractor.py \
  --docs-path docs \
  --output wordpress_categories.json
```

### Mit WordPress-Integration

```bash
# Existierende WordPress-Kategorien prüfen
python3 tools/wordpress_category_extractor.py \
  --docs-path docs \
  --output wordpress_categories.json \
  --check-wordpress \
  --wp-url https://ihre-wordpress-seite.com
```

### Optionen

- `--docs-path`: Pfad zum Dokumentationsverzeichnis (Standard: `docs`)
- `--output`: Ausgabe-JSON-Datei (Standard: `wordpress_categories.json`)
- `--check-wordpress`: Existierende WordPress-Kategorien/Tags prüfen
- `--wp-url`: WordPress-Site-URL für API-Zugriff
- `--wp-user`: WordPress-Benutzername (optional, für geschützte APIs)
- `--wp-password`: WordPress-Passwort (optional)

## Ausgabe

Das Tool erzeugt eine JSON-Datei mit:

```json
{
  "metadata": {
    "generated_at": "2026-01-09T12:36:12.509502",
    "total_documents": 1033,
    "source_path": "docs"
  },
  "categories": {
    "Security": 86,
    "Development": 63,
    "LLM Integration": 58,
    "Features": 50,
    "Guides": 44,
    ...
  },
  "tags": {
    "Ai": 912,
    "Performance": 639,
    "Api": 539,
    ...
  },
  "documents": [
    {
      "file_path": "docs/de/security/...",
      "title": "Security Guide",
      "categories": ["Security", "Guides"],
      "tags": ["Encryption", "Authentication"],
      "language": "de",
      ...
    }
  ]
}
```

## Features

### 1. Semantisches Kategorie-Mapping

Das Tool mappt Ordnernamen zu sinnvollen Kategorien:

```python
'architecture' → 'Architecture'
'aql' → 'AQL Query Language'
'llm' → 'LLM Integration'
'observability' → 'Monitoring & Observability'
```

### 2. Intelligente Filterung

Entfernt automatisch:
- Reine Zahlen (`2026`, `9`)
- Jahreszahlen und Daten
- Monate in Deutsch/Englisch (`Januar`, `January`)
- Versionsnummern (`v1.3.4`)
- Sprachcodes (`de`, `en`, `fr`)
- Protokollnamen (`http`, `https`)
- Generische Wörter (`use`, `readme`)

### 3. Content-basierte Tag-Extraktion

Erkennt relevante Themen:
- Technical: `vector search`, `graph database`, `time-series`
- Infrastructure: `docker`, `kubernetes`, `monitoring`
- Security: `encryption`, `authentication`, `compliance`
- Features: `llm`, `ai`, `machine learning`, `raid`, `replication`

### 4. YAML Frontmatter Support

```markdown
---
title: "Security Guide"
categories: ["Security", "Enterprise"]
tags: ["encryption", "compliance", "GDPR"]
language: de
---

# Security Guide

Content here...
```

## Integration mit WordPress

### Schritt 1: Kategorien extrahieren

```bash
python3 tools/wordpress_category_extractor.py \
  --docs-path docs \
  --output wordpress_categories.json \
  --check-wordpress \
  --wp-url https://ihre-seite.com
```

### Schritt 2: Import-Script erstellen

Die generierte JSON-Datei kann verwendet werden, um:

1. **Neue Kategorien anzulegen**: Nur die Kategorien, die noch nicht existieren
2. **Posts zu erstellen**: Mit korrekten Kategorien und Tags
3. **Batch-Import**: Alle Dokumente auf einmal importieren

Beispiel WordPress-Import-Script:

```python
import json
import requests

# Lade extrahierte Daten
with open('wordpress_categories.json') as f:
    data = json.load(f)

# WordPress API-Endpoint
wp_url = "https://ihre-seite.com/wp-json/wp/v2"
auth = ("username", "password")

# Erstelle fehlende Kategorien
existing_cats = set(...)  # Von WordPress API holen
for category in data['categories'].keys():
    if category not in existing_cats:
        requests.post(
            f"{wp_url}/categories",
            auth=auth,
            json={"name": category}
        )

# Importiere Dokumente
for doc in data['documents']:
    # Erstelle WordPress-Post mit Kategorien und Tags
    # ...
```

## Konfiguration anpassen

### Eigene Kategorie-Mappings hinzufügen

Bearbeiten Sie `CATEGORY_MAPPING` in der Datei:

```python
CATEGORY_MAPPING = {
    'your_folder': 'Your Custom Category',
    'another_folder': 'Another Category',
    ...
}
```

### Zusätzliche Key Topics definieren

Fügen Sie Themen zu `KEY_TOPICS` hinzu:

```python
KEY_TOPICS = [
    'your topic', 'another topic',
    ...
]
```

### Exclude-Patterns erweitern

Fügen Sie Regex-Patterns zu `EXCLUDE_PATTERNS` hinzu:

```python
EXCLUDE_PATTERNS = [
    r'^your_pattern$',
    ...
]
```

## Vorteile

✅ **Semantisch sinnvoll**: Kategorien basieren auf Inhalt und Struktur, nicht auf Pfaden  
✅ **Wartungsarm**: Automatische Extraktion ohne manuelle Kategorisierung  
✅ **Konsistent**: Gleiche Regeln für alle Dokumente  
✅ **WordPress-freundlich**: Prüft existierende Kategorien vor Erstellung  
✅ **Mehrsprachig**: Erkennt Dokumentensprache automatisch  
✅ **Erweiterbar**: Einfach neue Mappings und Regeln hinzufügen  

## Beispiele

### Vorher (mit altem Plugin)

```
Kategorien: 📁2026 Https,9 2026,governance,Januar 9,Januar 9 2026,knownlegde,Kritische Infrastrukturen,Multi Model,Themis,use
```

### Nachher (mit diesem Tool)

```
Kategorien: Governance, Security, Infrastructure
Tags: Multi-Model, Critical Infrastructure, Knowledge Management
```

## Troubleshooting

### "No markdown files found"

- Prüfen Sie, ob `--docs-path` korrekt ist
- Stellen Sie sicher, dass `.md` Dateien vorhanden sind

### "Failed to fetch WordPress categories"

- Prüfen Sie die WordPress-URL (inkl. `https://`)
- Stellen Sie sicher, dass die WordPress REST API aktiviert ist
- Bei geschützten APIs: Username/Password angeben

### "Categories contain dates/numbers"

- Passen Sie `EXCLUDE_PATTERNS` an
- Erweitern Sie `CATEGORY_MAPPING` für spezifische Ordner

## Dependencies

- Python 3.8+
- Optional: `PyYAML` für Frontmatter-Support
- Optional: `requests` für WordPress-API-Integration

```bash
pip install pyyaml requests
```

## Lizenz

MIT - Teil des ThemisDB-Projekts
