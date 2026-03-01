# ThemisDB WordPress Category & Tag Management - Update Guide

## Problem behoben / Problem Solved

**Deutsch:**
Das bisherige Plugin extrahierte unsinnige Kategorien aus Dateipfaden und Dateinamen, die Datumsangaben, Zahlen und andere nicht-semantische Informationen enthielten.

**English:**
The previous plugin extracted meaningless categories from file paths and names, containing dates, numbers, and other non-semantic information.

### Beispiel Vorher / Example Before:
```
Kategorien: 📁2026 Https,9 2026,governance,Januar 9,Januar 9 2026,knownlegde,Kritische Infrastrukturen,Multi Model,Themis,use
```

### Beispiel Nachher / Example After:
```
Kategorien: Governance, Security, Infrastructure
Tags: Multi-Model, Critical Infrastructure, Knowledge Management, LLM, Performance
```

## Neue Lösung / New Solution

### 1. Intelligenter Category Extractor (Python)

Ein neues Python-Tool extrahiert semantisch sinnvolle Kategorien und Tags:

```bash
python3 tools/wordpress_category_extractor.py \
  --docs-path docs \
  --output wordpress_categories.json \
  --check-wordpress \
  --wp-url https://ihre-wordpress-seite.com
```

**Features:**
- ✅ Semantisches Mapping von Ordnernamen zu Kategorien
- ✅ Automatische Filterung von Daten, Zahlen, Monaten
- ✅ Content-basierte Tag-Extraktion
- ✅ Prüfung existierender WordPress-Kategorien
- ✅ YAML Frontmatter Support

### 2. WordPress Importer (PHP)

Ein neuer WordPress-Importer nutzt die extrahierten Daten:

**Via WP-CLI:**
```bash
wp eval-file wordpress-plugin/themisdb-wiki-integration/wordpress_doc_importer.php wordpress_categories.json
```

**Via WordPress Admin:**
1. Gehe zu **Tools → ThemisDB Import**
2. Gib den Pfad zur `wordpress_categories.json` an
3. Klicke auf "Run Import"

## Workflow für saubere Kategorien / Workflow for Clean Categories

### Schritt 1: Kategorien extrahieren
```bash
cd /path/to/ThemisDB
python3 tools/wordpress_category_extractor.py \
  --docs-path docs \
  --output wordpress_categories.json \
  --check-wordpress \
  --wp-url https://ihre-seite.com
```

### Schritt 2: JSON-Datei überprüfen
```bash
cat wordpress_categories.json | jq '.categories | keys'
```

Erwartete Kategorien:
- Architecture
- API Reference
- AQL Query Language
- Deployment
- Development
- Enterprise Features
- Features
- Guides
- LLM Integration
- Monitoring & Observability
- Performance
- Security
- etc.

### Schritt 3: In WordPress importieren

**Option A: WP-CLI (empfohlen)**
```bash
# JSON-Datei hochladen
scp wordpress_categories.json user@server:/tmp/

# Import via WP-CLI
ssh user@server
cd /var/www/wordpress
wp eval-file wp-content/plugins/themisdb-wiki-integration/wordpress_doc_importer.php /tmp/wordpress_categories.json
```

**Option B: WordPress Admin**
1. JSON-Datei in WordPress-Theme-Ordner hochladen
2. Tools → ThemisDB Import öffnen
3. Pfad zur JSON-Datei eingeben
4. "Run Import" klicken

### Schritt 4: Ergebnis prüfen

Nach dem Import:
- Gehe zu **Posts → Categories**
- Überprüfe die erstellten Kategorien
- Gehe zu **Posts → Tags**
- Überprüfe die erstellten Tags

## Kategorie-Mapping anpassen / Customize Category Mapping

Du kannst das Kategorie-Mapping in `tools/wordpress_category_extractor.py` anpassen:

```python
CATEGORY_MAPPING = {
    'architecture': 'Architecture',
    'aql': 'AQL Query Language',
    'apis': 'API Reference',
    'deployment': 'Deployment',
    # Füge deine eigenen Mappings hinzu:
    'mein_ordner': 'Meine Custom Kategorie',
    'another_dir': 'Eine andere Kategorie',
}
```

## Ausschlussmuster anpassen / Customize Exclusion Patterns

Füge weitere Muster zu `EXCLUDE_PATTERNS` hinzu:

```python
EXCLUDE_PATTERNS = [
    r'^\d+$',  # Pure numbers
    r'^\d{4}$',  # Years
    r'^v?\d+\.\d+',  # Version numbers
    r'(?i)^(januar|februar|märz|april|mai|juni|juli|august|september|oktober|november|dezember)$',  # Monate
    # Füge eigene Muster hinzu:
    r'^tmp$',  # Temporäre Ordner
    r'^test$',  # Test-Ordner
]
```

## Key Topics erweitern / Extend Key Topics

Definiere weitere Themen für die Tag-Extraktion:

```python
KEY_TOPICS = [
    'vector search', 'graph database', 'time-series',
    'llm', 'ai', 'machine learning',
    'raid', 'replication', 'backup',
    # Füge deine Themen hinzu:
    'mein_thema', 'weiteres_thema',
]
```

## YAML Frontmatter Support

Du kannst auch explizite Metadaten in Markdown-Dateien definieren:

```markdown
---
title: "Security Best Practices"
categories: ["Security", "Enterprise", "Compliance"]
tags: ["encryption", "authentication", "GDPR", "audit"]
language: de
---

# Security Best Practices

Content here...
```

Das Tool verwendet diese Metadaten automatisch und ignoriert automatisch extrahierte Kategorien.

## Troubleshooting

### Problem: Immer noch unsinnige Kategorien

**Lösung:**
1. Prüfe `EXCLUDE_PATTERNS` in `wordpress_category_extractor.py`
2. Füge weitere Ausschlussmuster hinzu
3. Führe das Script erneut aus

### Problem: Fehlende Kategorien

**Lösung:**
1. Prüfe `CATEGORY_MAPPING` für fehlende Ordner
2. Füge Mappings für neue Ordner hinzu
3. Führe das Script erneut aus

### Problem: WordPress Import schlägt fehl

**Lösung:**
1. Prüfe, ob die JSON-Datei gültig ist: `python3 -m json.tool wordpress_categories.json`
2. Prüfe WordPress-Berechtigungen
3. Aktiviere WordPress Debug-Modus: `define('WP_DEBUG', true);`

### Problem: Kategorien werden doppelt angelegt

**Lösung:**
- Der Importer prüft automatisch existierende Kategorien
- Falls dennoch Duplikate entstehen, lösche sie manuell in WordPress
- Führe `--check-wordpress` aus, um existierende Kategorien zu laden

## Performance-Tipps

### Große Dokumentationen (>500 Dateien)

```bash
# Batch-Import in kleineren Gruppen
python3 tools/wordpress_category_extractor.py \
  --docs-path docs/de \
  --output wordpress_categories_de.json

python3 tools/wordpress_category_extractor.py \
  --docs-path docs/en \
  --output wordpress_categories_en.json
```

### Inkrementelle Updates

Der Importer erkennt existierende Posts über:
- Titel
- Content Hash

Bereits importierte Dokumente werden aktualisiert, nicht dupliziert.

## Weitere Dokumentation

- [WordPress Category Extractor README](../../tools/README_WORDPRESS_CATEGORY_EXTRACTOR.md)
- [WordPress Plugin README](README.md)
- [Installation Guide](INSTALLATION.md)

## Support

Bei Fragen oder Problemen:
1. Prüfe die Dokumentation oben
2. Öffne ein Issue auf GitHub
3. Kontaktiere das ThemisDB-Team

## Lizenz

MIT - Teil des ThemisDB-Projekts
