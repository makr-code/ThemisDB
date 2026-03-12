# ThemisDB Taxonomy Manager - Integration Guide

## Übersicht

Dieses Dokument beschreibt, wie die verschiedenen ThemisDB WordPress-Plugins mit dem gemeinsamen Taxonomy Manager integriert wurden.

## Integrierte Plugins

### 1. themisdb-downloads

**Status:** ✅ Integriert

**Änderungen:**
- Verwendet automatisch den shared taxonomy manager, falls aktiv
- Behält Legacy-Funktionalität für Rückwärtskompatibilität
- Zeigt Admin-Benachrichtigung, wenn shared manager aktiv ist

**Code-Integration:**
```php
// In themisdb-downloads.php
if (!function_exists('themisdb_get_taxonomy_manager')) {
    // Legacy-Manager verwenden
    require_once THEMISDB_DOWNLOADS_PLUGIN_DIR . 'includes/class-taxonomy-manager.php';
    new ThemisDB_Downloads_Taxonomy_Manager();
} else {
    // Shared taxonomy manager ist aktiv
    // Keine zusätzliche Initialisierung notwendig
}
```

### 2. themisdb-wiki-integration

**Status:** ✅ Integriert

**Änderungen:**
- `wordpress_doc_importer.php` nutzt hierarchische Kategorien
- Automatische Erkennung des shared managers
- Fallback auf Standard-Implementierung

**Code-Integration:**
```php
// In wordpress_doc_importer.php
private function get_or_create_category($name) {
    // Use shared taxonomy manager if available
    if (function_exists('themisdb_get_taxonomy_manager')) {
        $manager = themisdb_get_taxonomy_manager();
        $hierarchy = $manager->get_hierarchy();
        return $hierarchy->get_or_create_hierarchical_category($name);
    }
    
    // Fallback
    return wp_insert_term($name, 'category');
}
```

## Aktivierungsreihenfolge

### Empfohlene Installation:

1. **ThemisDB Taxonomy Manager** installieren und aktivieren
2. **themisdb-downloads** installieren/aktivieren
3. **themisdb-wiki-integration** nutzen (kein WordPress-Plugin, nur Script)

### Warum diese Reihenfolge?

- Der shared manager muss zuerst aktiv sein
- Andere Plugins erkennen automatisch, ob der shared manager verfügbar ist
- Legacy-Funktionalität wird automatisch deaktiviert

## Funktionsweise

### Mit Shared Taxonomy Manager

```
┌─────────────────────────────────────┐
│  ThemisDB Taxonomy Manager          │
│  (Shared Plugin)                    │
│                                     │
│  - Hierarchische Kategorien         │
│  - Konsolidierung                   │
│  - Dual Extraction                  │
└──────────────┬──────────────────────┘
               │
               │ verwendet
               │
    ┌──────────┴───────────┐
    │                      │
    ▼                      ▼
┌─────────┐          ┌──────────┐
│ themisdb│          │ themisdb-│
│downloads│          │  wiki-   │
│         │          │integration│
└─────────┘          └──────────┘
```

### Ohne Shared Taxonomy Manager (Legacy)

```
┌─────────────────────┐    ┌──────────────────────┐
│  themisdb-downloads │    │ themisdb-wiki-       │
│                     │    │ integration          │
│  Eigener Taxonomy   │    │                      │
│  Manager (Legacy)   │    │ Eigene Implementierung│
└─────────────────────┘    └──────────────────────┘
```

## Migrationsschritte

### Von Legacy zu Shared Manager

**Schritt 1: Backup erstellen**
```bash
# WordPress Datenbank backup
wp db export backup.sql

# Kategorien exportieren
wp term list category --format=json > categories_backup.json
```

**Schritt 2: Shared Manager installieren**
```bash
cd /wp-content/plugins/
cp -r /path/to/themisdb-taxonomy-manager ./
```

**Schritt 3: Plugin aktivieren**
```bash
wp plugin activate themisdb-taxonomy-manager
```

**Schritt 4: Kategorien konsolidieren**
```bash
# Via WP-CLI
wp rest post /themisdb/v1/taxonomy/consolidate

# Oder im WordPress Admin:
# Einstellungen → Taxonomy Manager → Optimization → Run Consolidation
```

**Schritt 5: Andere Plugins neu laden**
```bash
wp plugin deactivate themisdb-downloads
wp plugin activate themisdb-downloads
```

## Testen der Integration

### Test 1: Überprüfen, ob shared manager erkannt wird

```php
// In WordPress Theme oder Plugin
if (function_exists('themisdb_get_taxonomy_manager')) {
    echo "✅ Shared Taxonomy Manager ist aktiv!";
    
    $manager = themisdb_get_taxonomy_manager();
    $extractor = $manager->get_extractor();
    $hierarchy = $manager->get_hierarchy();
    
    echo "Extractor: " . get_class($extractor);
    echo "Hierarchy: " . get_class($hierarchy);
} else {
    echo "❌ Shared Taxonomy Manager nicht gefunden";
}
```

### Test 2: Hierarchische Kategorien erstellen

```php
if (function_exists('themisdb_get_taxonomy_manager')) {
    $manager = themisdb_get_taxonomy_manager();
    
    // Erstelle hierarchische Kategorie
    $cat_id = $manager->get_hierarchy()->get_or_create_hierarchical_category('Authentication');
    
    // Prüfe, ob Eltern-Kategorie erstellt wurde
    $cat = get_term($cat_id, 'category');
    if ($cat->parent > 0) {
        $parent = get_term($cat->parent, 'category');
        echo "✅ Hierarchie: {$parent->name} → {$cat->name}";
    }
}
```

### Test 3: Kategorien konsolidieren

```php
if (function_exists('themisdb_get_taxonomy_manager')) {
    $manager = themisdb_get_taxonomy_manager();
    
    // Hole Empfehlungen
    $recommendations = $manager->get_optimization_recommendations();
    echo "Empfehlungen: " . count($recommendations);
    
    // Führe Konsolidierung aus
    $stats = $manager->consolidate_categories();
    print_r($stats);
}
```

## API-Nutzung

### REST API Endpoints

**1. Taxonomien extrahieren**
```bash
# Von Post
curl -X POST http://yoursite.com/wp-json/themisdb/v1/taxonomy/extract \
  -H "Content-Type: application/json" \
  -d '{"post_id": 123}'

# Von Text
curl -X POST http://yoursite.com/wp-json/themisdb/v1/taxonomy/extract \
  -H "Content-Type: application/json" \
  -d '{
    "text": "Content here...",
    "title": "Title",
    "options": {"max_categories": 5}
  }'
```

**2. Kategorien konsolidieren**
```bash
curl -X POST http://yoursite.com/wp-json/themisdb/v1/taxonomy/consolidate \
  -H "Authorization: Bearer YOUR_TOKEN"
```

**3. Empfehlungen abrufen**
```bash
curl http://yoursite.com/wp-json/themisdb/v1/taxonomy/recommendations \
  -H "Authorization: Bearer YOUR_TOKEN"
```

### PHP API

**Taxonomien aus Post extrahieren:**
```php
$manager = themisdb_get_taxonomy_manager();

$post_id = 123;
$result = $manager->get_extractor()->extract_taxonomies($post_id, array(
    'extract_from_content' => true,
    'extract_from_metadata' => true,
    'max_categories' => 5,
    'max_tags' => 15
));

// $result = array(
//     'categories' => array('Security', 'Authentication'),
//     'tags' => array('Encryption', 'OAuth', 'JWT')
// );
```

**Kategorien mit Hierarchie zuweisen:**
```php
$manager = themisdb_get_taxonomy_manager();

$post_id = 123;
$categories = array('Authentication', 'LLM Integration', 'Performance');

$manager->assign_categories_with_hierarchy($post_id, $categories);

// Erstellt automatisch:
// Security → Authentication
// Features → LLM Integration
// Operations → Performance
```

**Batch-Verarbeitung:**
```php
$manager = themisdb_get_taxonomy_manager();

$post_ids = array(1, 2, 3, 4, 5);
$stats = $manager->batch_assign_taxonomies($post_ids, array(
    'extract_from_content' => true,
    'max_categories' => 5
));

echo "Verarbeitet: {$stats['processed']}";
echo "Kategorien zugewiesen: {$stats['categories_assigned']}";
```

## Troubleshooting

### Problem: "Call to undefined function themisdb_get_taxonomy_manager()"

**Lösung:**
```bash
# Prüfen, ob Plugin aktiv ist
wp plugin list | grep themisdb-taxonomy-manager

# Aktivieren
wp plugin activate themisdb-taxonomy-manager

# Plugin-Ladereihenfolge prüfen
wp plugin list --fields=name,status --status=active
```

### Problem: Legacy-Manager wird weiterhin verwendet

**Lösung:**
```php
// In themisdb-downloads.php oder anderem Plugin
// Sicherstellen, dass Prüfung korrekt ist:
if (function_exists('themisdb_get_taxonomy_manager')) {
    // Shared manager verwenden
} else {
    // Legacy verwenden
}
```

### Problem: Kategorien werden nicht hierarchisch erstellt

**Lösung:**
```bash
# Konsolidierung manuell ausführen
wp eval "
\$manager = themisdb_get_taxonomy_manager();
\$stats = \$manager->consolidate_categories();
print_r(\$stats);
"
```

### Problem: Doppelte Kategorien nach Migration

**Lösung:**
```bash
# Empfehlungen prüfen
wp rest get /themisdb/v1/taxonomy/recommendations

# Konsolidierung ausführen
wp rest post /themisdb/v1/taxonomy/consolidate
```

## Performance-Überlegungen

### Caching

Der shared manager nutzt WordPress Transients für Caching:
- Kategorie-Hierarchie wird gecached
- Konsolidierungs-Regeln werden gecached
- Cache-Dauer: 1 Stunde

```php
// Cache manuell löschen
delete_transient('themisdb_taxonomy_hierarchy');
delete_transient('themisdb_taxonomy_consolidation');
```

### Bulk-Operations

Bei großen Importen:
```php
// Deaktiviere Auto-Extraction temporär
update_option('themisdb_taxonomy_auto_extract', 0);

// Import durchführen
// ...

// Batch-Processing
$manager = themisdb_get_taxonomy_manager();
$post_ids = get_posts(array('fields' => 'ids', 'posts_per_page' => -1));
$manager->batch_assign_taxonomies($post_ids);

// Auto-Extraction wieder aktivieren
update_option('themisdb_taxonomy_auto_extract', 1);
```

## Best Practices

### 1. Immer shared manager verwenden

✅ **Gut:**
```php
if (function_exists('themisdb_get_taxonomy_manager')) {
    $manager = themisdb_get_taxonomy_manager();
    $manager->assign_categories_with_hierarchy($post_id, $categories);
}
```

❌ **Schlecht:**
```php
// Direkt wp_insert_term verwenden ohne Hierarchie-Prüfung
wp_insert_term($category, 'category');
```

### 2. Regelmäßige Konsolidierung

```php
// Cron-Job einrichten
add_action('themisdb_daily_consolidation', function() {
    if (function_exists('themisdb_get_taxonomy_manager')) {
        $manager = themisdb_get_taxonomy_manager();
        $manager->consolidate_categories();
    }
});

if (!wp_next_scheduled('themisdb_daily_consolidation')) {
    wp_schedule_event(time(), 'daily', 'themisdb_daily_consolidation');
}
```

### 3. Hierarchie-Regeln anpassen

```php
// In Theme functions.php oder Plugin
add_filter('themisdb_category_hierarchy_rules', function($rules) {
    $rules['Ihre Kategorie'] = array('Child 1', 'Child 2');
    return $rules;
});
```

## Support

Bei Fragen oder Problemen:
- GitHub Issues: https://github.com/makr-code/wordpressPlugins/issues
- Dokumentation: siehe README.md im Plugin-Verzeichnis
