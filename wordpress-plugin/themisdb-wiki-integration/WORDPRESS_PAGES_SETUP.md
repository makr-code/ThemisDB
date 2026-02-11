# WordPress-Seiten Setup für ThemisDB Dokumentation (Option 2)

**Version:** 1.2.0  
**Datum:** 07. Januar 2026  
**Implementierung:** Hybrid-Ansatz mit WordPress-Suche

---

## Übersicht

Dieser Guide erklärt, wie Sie WordPress-Seiten für die ThemisDB-Dokumentation anlegen, damit:
- ✅ WordPress-Suche die Dokumentation findet
- ✅ Inhalte automatisch aus GitHub synchronisiert bleiben
- ✅ Keine manuelle Pflege der Inhalte nötig ist

---

## Konzept: Hybrid-Ansatz

### Statische Komponente (WordPress)
- Seiten-URL (z.B. `/docs/features/`)
- Seitentitel (z.B. "ThemisDB Features")
- Meta-Beschreibung für SEO
- Kategorien/Tags für Organisation

### Dynamische Komponente (GitHub)
- Markdown-Inhalt wird bei jedem Aufruf geladen
- Automatische Updates nach Cache-Ablauf (1 Stunde)
- Single Source of Truth bleibt GitHub

---

## Schritt-für-Schritt-Anleitung

### Schritt 1: Dokumentationsstruktur planen

Basierend auf der `_Sidebar.md` aus GitHub:

```
WordPress-Seiten:
/docs/                          → Home.md
/docs/schnellstart/             → Quick Reference
/docs/features/                 → FEATURES.md
/docs/architektur/              → architecture.md
/docs/installation/             → guides/INSTALLATION.md
/docs/api/                      → apis/openapi.md
/docs/sicherheit/               → security/overview.md
... (weitere Seiten)
```

### Schritt 2: Basis-Seite erstellen

**WordPress Admin → Seiten → Erstellen**

**Beispiel: Features-Seite**

1. **Titel:** `ThemisDB Features`
2. **URL-Slug:** `features`
3. **Inhalt:**
```php
[themisdb_wiki file="FEATURES.md" lang="de" show_toc="yes"]
```

4. **Kategorie:** `Dokumentation`
5. **Tags:** `features`, `database`, `multi-model`
6. **Meta-Beschreibung (SEO):**
   ```
   Überblick über alle Features von ThemisDB: Multi-Model-Datenbank, 
   LLM-Integration, ACID-Transaktionen, Vector Search und mehr.
   ```

### Schritt 3: Seiten-Template (Optional)

Erstellen Sie ein WordPress-Template für konsistente Dokumentationsseiten:

**themes/[your-theme]/page-docs.php:**

```php
<?php
/**
 * Template Name: Dokumentationsseite
 */

get_header(); ?>

<div class="docs-layout">
    <!-- Sidebar mit Navigation -->
    <aside class="docs-sidebar">
        <?php echo do_shortcode('[themisdb_wiki_nav lang="de" style="accordion"]'); ?>
    </aside>
    
    <!-- Hauptinhalt -->
    <main class="docs-content">
        <?php
        while (have_posts()) : the_post();
            the_content(); // Shortcode wird hier gerendert
        endwhile;
        ?>
        
        <!-- Breadcrumb -->
        <nav class="docs-breadcrumb">
            <a href="<?php echo home_url('/'); ?>">Home</a> » 
            <a href="<?php echo home_url('/docs/'); ?>">Dokumentation</a> » 
            <span><?php the_title(); ?></span>
        </nav>
        
        <!-- Footer Navigation -->
        <div class="docs-footer-nav">
            <?php
            // Vorherige/Nächste Seite
            previous_post_link('%link', '← %title');
            next_post_link('%link', '%title →');
            ?>
        </div>
    </main>
</div>

<?php get_footer(); ?>
```

### Schritt 4: Seiten-Erstellung automatisieren (WP-CLI)

Für viele Seiten nutzen Sie WP-CLI:

**seiten-erstellen.sh:**

```bash
#!/bin/bash

# Array mit Dokumentationsseiten
declare -A PAGES=(
    ["features"]="FEATURES.md"
    ["architektur"]="architecture.md"
    ["installation"]="guides/INSTALLATION.md"
    ["api"]="apis/openapi.md"
    ["sicherheit"]="security/overview.md"
    ["aql-syntax"]="aql_syntax.md"
    ["performance"]="performance_benchmarks.md"
)

# Seiten erstellen
for slug in "${!PAGES[@]}"; do
    file="${PAGES[$slug]}"
    
    # Titel aus Slug generieren
    title=$(echo "$slug" | sed 's/-/ /g' | awk '{for(i=1;i<=NF;i++)sub(/./,toupper(substr($i,1,1)),$i)}1')
    
    # Seite erstellen
    wp post create \
        --post_type=page \
        --post_status=publish \
        --post_title="ThemisDB $title" \
        --post_name="$slug" \
        --post_content="[themisdb_wiki file=\"$file\" lang=\"de\" show_toc=\"yes\"]" \
        --page_template=page-docs.php \
        --post_category="Dokumentation"
    
    echo "✓ Seite erstellt: $title"
done

echo "Alle Seiten wurden erstellt!"
```

**Ausführen:**

```bash
chmod +x seiten-erstellen.sh
./seiten-erstellen.sh
```

### Schritt 5: WordPress-Menü konfigurieren

**WordPress Admin → Design → Menüs**

1. Neues Menü erstellen: "Dokumentation"
2. Seiten hinzufügen (aus Schritt 4)
3. Hierarchie erstellen:
   ```
   Dokumentation
   ├── Schnellstart
   ├── Features
   ├── Architektur
   │   ├── Überblick
   │   └── Storage
   ├── Installation
   ├── API Reference
   └── Sicherheit
   ```
4. Menü Position: "Hauptmenü" oder "Sidebar"

### Schritt 6: WordPress-Suche konfigurieren

**Suche funktioniert automatisch**, aber für bessere Ergebnisse:

**Option A: Native WordPress-Suche**
```php
// In functions.php
add_filter('pre_get_posts', 'include_docs_in_search');

function include_docs_in_search($query) {
    if ($query->is_search && !is_admin()) {
        $query->set('post_type', array('post', 'page'));
        $query->set('category_name', 'dokumentation');
    }
    return $query;
}
```

**Option B: Relevanssi Plugin**
1. Plugin installieren: WordPress Admin → Plugins → Relevanssi
2. Einstellungen → Relevanssi
3. "Index neu aufbauen" klicken
4. Dokumentationsseiten werden indiziert

**Option C: SearchWP Plugin**
1. Plugin installieren: SearchWP
2. Engines konfigurieren
3. Seiten mit Kategorie "Dokumentation" priorisieren

### Schritt 7: SEO optimieren

**Yoast SEO / Rank Math:**

1. **Focus Keyword** für jede Seite:
   - Features: "ThemisDB Features"
   - Installation: "ThemisDB Installation"
   - API: "ThemisDB API Dokumentation"

2. **Meta-Beschreibung** (155-160 Zeichen):
   ```
   Vollständige Dokumentation zu ThemisDB Features: 
   Multi-Model-Datenbank, LLM-Integration, ACID-Transaktionen 
   und Vector Search.
   ```

3. **Open Graph Tags**:
   ```
   og:title: ThemisDB Features
   og:description: [Meta-Beschreibung]
   og:image: [ThemisDB Logo]
   ```

---

## Verwendungsbeispiele

### Beispiel 1: Einfache Dokumentationsseite

**Seite:** `/docs/features/`

**Inhalt:**
```php
[themisdb_wiki file="FEATURES.md" lang="de" show_toc="yes"]
```

**Ergebnis:**
- Markdown wird von GitHub geladen
- Inhaltsverzeichnis wird angezeigt
- Bei GitHub-Update: nach 1h automatisch aktualisiert

### Beispiel 2: Mehrsprachige Seite

**Seite EN:** `/en/docs/features/`

**Inhalt:**
```php
[themisdb_wiki file="FEATURES.md" lang="en" show_toc="yes"]
```

**Seite DE:** `/de/docs/features/`

**Inhalt:**
```php
[themisdb_wiki file="FEATURES.md" lang="de" show_toc="yes"]
```

### Beispiel 3: Seite mit Navigation

**Seite:** `/docs/`

**Inhalt:**
```php
<div class="docs-home">
    <h1>ThemisDB Dokumentation</h1>
    <p>Willkommen zur vollständigen Dokumentation von ThemisDB.</p>
    
    <!-- Navigation -->
    [themisdb_wiki_nav lang="de" style="sidebar"]
    
    <!-- Übersicht -->
    [themisdb_wiki file="README.md" lang="de"]
</div>
```

### Beispiel 4: Kombinierte Ansicht

**Seite:** `/docs/architektur/`

**Inhalt:**
```php
<!-- Sidebar Navigation -->
<aside class="sidebar">
    [themisdb_wiki_nav lang="de" style="accordion"]
</aside>

<!-- Hauptinhalt -->
<article class="content">
    [themisdb_wiki file="architecture.md" lang="de" show_toc="yes"]
</article>

<!-- Verwandte Themen -->
<section class="related">
    <h3>Verwandte Themen</h3>
    [themisdb_docs lang="de" layout="grid"]
</section>
```

---

## WordPress-Suche nutzen

### Suchformular einbinden

**Standardmäßig:**
```php
<?php get_search_form(); ?>
```

**Custom Suchformular:**
```html
<form role="search" method="get" action="<?php echo home_url('/'); ?>">
    <input type="search" 
           name="s" 
           placeholder="Dokumentation durchsuchen..." 
           value="<?php echo get_search_query(); ?>">
    <input type="hidden" name="post_type" value="page">
    <button type="submit">Suchen</button>
</form>
```

### Suchergebnisse anpassen

**search.php Template:**
```php
<?php get_header(); ?>

<div class="search-results">
    <h1>Suchergebnisse für: <?php echo get_search_query(); ?></h1>
    
    <?php if (have_posts()) : ?>
        <ul class="search-list">
            <?php while (have_posts()) : the_post(); ?>
                <li class="search-item">
                    <h2><a href="<?php the_permalink(); ?>"><?php the_title(); ?></a></h2>
                    <div class="excerpt"><?php the_excerpt(); ?></div>
                    <a href="<?php the_permalink(); ?>" class="read-more">Weiterlesen →</a>
                </li>
            <?php endwhile; ?>
        </ul>
        
        <?php the_posts_pagination(); ?>
    <?php else : ?>
        <p>Keine Ergebnisse gefunden. Versuchen Sie andere Suchbegriffe.</p>
    <?php endif; ?>
</div>

<?php get_footer(); ?>
```

---

## Wartung & Updates

### Automatische Updates

**Keine Aktion nötig!**
- Markdown-Änderungen in GitHub → nach 1h sichtbar
- Cache manuell leeren: Admin → ThemisDB Wiki → "Sync Now"

### Neue Seiten hinzufügen

**Wenn neue Dokumentation in GitHub erscheint:**

1. WordPress-Seite erstellen (siehe Schritt 2)
2. Shortcode mit neuem File-Pfad einfügen
3. In Menü/Navigation aufnehmen
4. Fertig!

### Seiten löschen

**Wenn Dokumentation aus GitHub entfernt wird:**

1. WordPress-Seite löschen oder auf "Entwurf" setzen
2. Aus Menü entfernen
3. 404-Weiterleitung einrichten (optional)

---

## Vorteile dieser Methode

### ✅ Für Nutzer
- WordPress-Suche funktioniert
- Saubere URLs
- SEO-optimiert
- Schnelle Navigation

### ✅ Für Entwickler
- Single Source of Truth (GitHub)
- Keine Duplikation
- Automatische Updates
- Einfache Wartung

### ✅ Für Content-Manager
- Dokumentation in GitHub pflegen
- Keine WordPress-Kenntnisse nötig
- Git-Workflow für Docs
- Versionskontrolle

---

## Troubleshooting

### Problem: Seite zeigt alten Inhalt

**Lösung:**
```
Admin → ThemisDB Wiki → "Sync Now"
```

### Problem: Suche findet Seiten nicht

**Lösung:**
1. Relevanssi: Index neu aufbauen
2. SearchWP: Re-Index starten
3. Native Suche: WordPress-Cache leeren

### Problem: Layout passt nicht

**Lösung:**
```css
/* In Custom CSS */
.themisdb-wiki-container {
    max-width: 100%;
    padding: 1rem;
}
```

---

## Best Practices

### 1. Konsistente URL-Struktur

```
/docs/               → Dokumentations-Home
/docs/[kategorie]/  → Kategorie-Seite
/docs/[kategorie]/[thema]/ → Detail-Seite
```

### 2. Breadcrumb-Navigation

Jede Seite sollte Breadcrumbs haben:
```
Home » Dokumentation » Architektur » Storage
```

### 3. Related Content

Verwandte Artikel am Seitenende:
```php
[themisdb_docs lang="de" layout="grid"]
```

### 4. Sidebar-Navigation

Konsistente Navigation auf allen Docs-Seiten:
```php
[themisdb_wiki_nav lang="de" style="accordion"]
```

---

## Zusammenfassung

**Setup-Checkliste:**

- [ ] Dokumentationsstruktur planen
- [ ] WordPress-Seiten erstellen (manuell oder per WP-CLI)
- [ ] Shortcodes in Seiteninhalte einfügen
- [ ] Template für Docs-Seiten erstellen (optional)
- [ ] WordPress-Menü konfigurieren
- [ ] Suche testen
- [ ] SEO-Metadaten ergänzen
- [ ] Fertig! Inhalte aktualisieren sich automatisch

**Zeitaufwand:** ~2-4 Stunden für vollständige Dokumentationsstruktur

**Ergebnis:**
- ✅ WordPress-Suche funktioniert
- ✅ Inhalte bleiben automatisch aktuell
- ✅ Keine manuelle Pflege nötig

---

**Version:** 1.2.0  
**Autor:** ThemisDB Team  
**Datum:** 07. Januar 2026
