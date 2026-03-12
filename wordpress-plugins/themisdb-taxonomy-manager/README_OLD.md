# ThemisDB Taxonomy Manager

Ein gemeinsames WordPress-Plugin zur intelligenten Verwaltung von Kategorien und Tags für alle ThemisDB-Plugins.

## Übersicht

Dieses Plugin konsolidiert die Taxonomie-Verwaltung aus beiden bisherigen Plugins:
- **themisdb-downloads**: Content-basierte Extraktion (Textanalyse)
- **themisdb-wiki-integration**: Struktur-basierte Extraktion (Dateipfade, Metadaten)

## Hauptfunktionen

### ✅ Duale Extraktion
- **Content-basiert**: Analysiert Beitragsinhalt mit Wortfrequenz und Phrase-Erkennung
- **Struktur-basiert**: Extrahiert aus Dateipfaden, Verzeichnisstruktur und Metadaten
- **Kombiniert**: Nutzt beide Ansätze für optimale Ergebnisse

### ✅ Hierarchische Kategorien
- **Bis zu 3 Ebenen**: Parent → Child → Grandchild Struktur
- **Automatische Hierarchie**: Erstellt automatisch Eltern-Kind-Beziehungen
- **Konsolidierung**: Minimiert redundante Kategorien

### ✅ Intelligente Optimierung
- **Kategorie-Konsolidierung**: Fasst ähnliche Kategorien zusammen
- **Empfehlungen**: Schlägt Optimierungen vor
- **Minimierung**: "So wenig wie möglich, so viel wie nötig"

## Installation

### Methode 1: WordPress Admin

1. Laden Sie das Plugin-Verzeichnis als ZIP-Datei herunter
2. Gehen Sie zu WordPress Admin → Plugins → Installieren
3. Klicken Sie auf "Plugin hochladen"
4. Aktivieren Sie das Plugin

### Methode 2: Manuell

```bash
cd /wp-content/plugins/
cp -r /pfad/zu/themisdb-taxonomy-manager ./
```

## Konfiguration

Gehen Sie zu **Einstellungen → Taxonomy Manager**:

### Grundeinstellungen

| Einstellung | Beschreibung | Standard |
|-------------|--------------|----------|
| Enable Auto-Extraction | Automatische Extraktion bei Post-Speicherung | ✅ An |
| Auto-Assign Categories | Automatische Kategorie-Zuweisung | ✅ An |
| Auto-Assign Tags | Automatische Tag-Zuweisung | ✅ An |
| Maximum Category Depth | Maximale Kategorie-Tiefe (1-5) | 3 |
| Consolidate Categories | Automatische Konsolidierung | ✅ An |

## Kategorie-Hierarchie

Das Plugin definiert eine dreistufige Hierarchie:

```
Documentation (Ebene 1)
├── Guides (Ebene 2)
├── API Reference (Ebene 2)
└── Architecture (Ebene 2)

Security (Ebene 1)
├── Authentication (Ebene 2)
│   └── OAuth (Ebene 3)
├── Encryption (Ebene 2)
└── Compliance (Ebene 2)

Features (Ebene 1)
├── LLM Integration (Ebene 2)
├── Vector Search (Ebene 2)
└── Time-Series (Ebene 2)

Operations (Ebene 1)
├── Deployment (Ebene 2)
├── Monitoring & Observability (Ebene 2)
└── Performance (Ebene 2)
```

### Hierarchie-Regeln anpassen

```php
add_filter('themisdb_category_hierarchy_rules', function($rules) {
    $rules['Ihre Kategorie'] = array('Child 1', 'Child 2');
    return $rules;
});
```

## Konsolidierung

Das Plugin konsolidiert ähnliche Kategorien automatisch:

```
Monitoring → Monitoring & Observability
Observability → Monitoring & Observability
AQL → AQL Query Language
APIs → API Reference
Auth → Authentication
```

### Konsolidierungs-Regeln anpassen

```php
add_filter('themisdb_category_consolidation_rules', function($rules) {
    $rules['Ihre Variante'] = 'Kanonischer Name';
    return $rules;
});
```

## Verwendung für Entwickler

### Programmatische Extraktion

```php
// Hole Taxonomy Manager
$manager = themisdb_get_taxonomy_manager();

// Extrahiere von Post
$post_id = 123;
$result = $manager->get_extractor()->extract_taxonomies($post_id);

// Weise mit Hierarchie zu
$manager->assign_categories_with_hierarchy($post_id, $result['categories']);
$manager->assign_tags($post_id, $result['tags']);
```

### Batch-Verarbeitung

```php
$manager = themisdb_get_taxonomy_manager();

$post_ids = array(1, 2, 3, 4, 5);
$options = array(
    'extract_from_content' => true,
    'extract_from_metadata' => true,
    'max_categories' => 5,
    'max_tags' => 15
);

$stats = $manager->batch_assign_taxonomies($post_ids, $options);

// $stats = array(
//     'processed' => 5,
//     'categories_assigned' => 12,
//     'tags_assigned' => 23,
//     'errors' => 0
// );
```

### Kategorie-Optimierung

```php
$manager = themisdb_get_taxonomy_manager();

// Hole Empfehlungen
$recommendations = $manager->get_optimization_recommendations();

// Führe Konsolidierung aus
$stats = $manager->consolidate_categories();

// $stats = array(
//     'consolidated' => 5,
//     'hierarchized' => 8,
//     'errors' => 0
// );
```

## REST API

Das Plugin stellt REST API Endpoints bereit:

### Taxonomien extrahieren

```bash
POST /wp-json/themisdb/v1/taxonomy/extract
{
  "post_id": 123
}

# Oder mit Text
POST /wp-json/themisdb/v1/taxonomy/extract
{
  "text": "Content here...",
  "title": "Title here...",
  "options": {
    "max_categories": 5,
    "max_tags": 15
  }
}
```

### Kategorien konsolidieren

```bash
POST /wp-json/themisdb/v1/taxonomy/consolidate
```

### Empfehlungen abrufen

```bash
GET /wp-json/themisdb/v1/taxonomy/recommendations
```

## Integration mit anderen Plugins

### themisdb-downloads

```php
// In themisdb-downloads/themisdb-downloads.php
if (function_exists('themisdb_get_taxonomy_manager')) {
    // Verwende shared manager statt eigener Klasse
    // remove_action('save_post', array($old_manager, 'auto_assign_taxonomies'));
}
```

### themisdb-wiki-integration

```php
// In wordpress_doc_importer.php
if (function_exists('themisdb_get_taxonomy_manager')) {
    $manager = themisdb_get_taxonomy_manager();
    
    // Nutze hierarchische Kategorien
    $manager->assign_categories_with_hierarchy($post_id, $categories);
}
```

## Optimierung

### Manuelle Konsolidierung

1. Gehen Sie zu **Einstellungen → Taxonomy Manager**
2. Wechseln Sie zum Tab **Optimization**
3. Klicken Sie auf **Get Recommendations**
4. Prüfen Sie die Vorschläge
5. Klicken Sie auf **Run Consolidation**

### Ergebnis

```
Results:
{
  "consolidated": 5,
  "hierarchized": 8,
  "errors": 0
}
```

## Best Practices

### 1. Minimale Kategorien

✅ **Gut**: `Security → Authentication`
❌ **Schlecht**: `Security`, `Authentication`, `Security and Auth`, `Auth`

### 2. Hierarchische Struktur

✅ **Gut**: 
```
Features
  ├── LLM Integration
  └── Vector Search
```

❌ **Schlecht**: Alle auf einer Ebene

### 3. Konsolidierung

✅ **Gut**: Eine kanonische Kategorie `Monitoring & Observability`
❌ **Schlecht**: Mehrere Varianten: `Monitoring`, `Observability`, `Monitoring/Observability`

## Troubleshooting

### Problem: Kategorien werden nicht automatisch erstellt

**Lösung**: Prüfen Sie, ob "Auto-Assign Categories" in den Einstellungen aktiviert ist.

### Problem: Zu viele Kategorien

**Lösung**: 
1. Reduzieren Sie "Maximum Category Depth"
2. Führen Sie Konsolidierung aus
3. Prüfen Sie Empfehlungen

### Problem: Hierarchie wird nicht angewendet

**Lösung**: Führen Sie manuelle Konsolidierung aus unter **Optimization** Tab.

## Lizenz

MIT - Teil des ThemisDB-Projekts

## Support

- GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- Dokumentation: https://github.com/makr-code/wordpressPlugins/wiki
