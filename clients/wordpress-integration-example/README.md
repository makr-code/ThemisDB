# ThemisDB LLM Integration - WordPress Plugin Example

## Übersicht

Dies ist ein **Proof-of-Concept** WordPress Plugin, das demonstriert, wie WordPress mit ThemisDB für erweiterte LLM/AI-Features integriert werden kann.

## Architektur: Hybrid-Ansatz

```
┌─────────────────────────────────────────┐
│         WordPress Frontend              │
│         (Standard WordPress)            │
└──────────────┬──────────────────────────┘
               │
       ┌───────┴───────┐
       │               │
       ▼               ▼
┌─────────────┐  ┌─────────────────┐
│   MySQL/    │  │    ThemisDB     │
│  MariaDB    │  │   (AQL + LLM)   │
│             │  │                 │
│  • Posts    │  │  • Embeddings   │
│  • Users    │  │  • Vector       │
│  • Comments │  │    Search       │
│  • Options  │  │  • RAG Context  │
└─────────────┘  │  • LLM Cache    │
                 └─────────────────┘
```

## Features

### ✅ Implementiert

1. **Automatic Post Synchronization**
   - Posts werden automatisch von MySQL zu ThemisDB synchronisiert
   - Embeddings für Semantic Search werden generiert

2. **Semantic Search**
   - Vektor-basierte Suche über alle Posts
   - Findet semantisch ähnliche Inhalte

3. **AI Chat Widget**
   - RAG (Retrieval-Augmented Generation)
   - Beantwortet Fragen basierend auf WordPress Content
   - Shortcode: `[themis_chat]`

4. **Search Widget**
   - Semantic Search im Frontend
   - Shortcode: `[themis_search]`

5. **Admin Dashboard**
   - Konfiguration von ThemisDB Endpoint
   - Status-Übersicht
   - Sync-Einstellungen

## Installation

### Voraussetzungen

1. **WordPress** >= 5.0
2. **PHP** >= 7.4
3. **ThemisDB Server** (läuft auf localhost:8080 oder remote)
4. **ThemisDB PHP SDK** (via Composer)

### Schritte

1. **Plugin installieren:**
   ```bash
   cd wordpress-integration-example
   composer require themisdb/themisdb-php
   ```

2. **Nach WordPress kopieren:**
   ```bash
   cp -r wordpress-integration-example /path/to/wordpress/wp-content/plugins/themisdb-llm
   ```

3. **Plugin aktivieren:**
   - WordPress Admin → Plugins → ThemisDB LLM Integration → Aktivieren

4. **Konfigurieren:**
   - WordPress Admin → ThemisDB → Settings
   - ThemisDB Endpoint eingeben (z.B. `http://localhost:8080`)
   - LLM Model auswählen (z.B. `llama-2-7b`)
   - Auto Sync aktivieren

5. **Testen:**
   - Neuen Post erstellen und veröffentlichen
   - Prüfen ob Post zu ThemisDB synchronisiert wurde
   - Shortcodes ausprobieren: `[themis_search]` oder `[themis_chat]`

## Verwendung

### Shortcodes

#### Semantic Search Widget
```
[themis_search]
```

Fügt ein Suchfeld ein, das semantische Suche über alle Posts ermöglicht.

#### AI Chat Widget
```
[themis_chat]
```

Fügt einen Chat-Widget ein, der Fragen basierend auf WordPress Content beantwortet.

### AJAX API

#### Semantic Search
```javascript
jQuery.ajax({
    url: themisdb.ajax_url,
    method: 'POST',
    data: {
        action: 'themis_semantic_search',
        nonce: themisdb.search_nonce,
        query: 'machine learning'
    },
    success: function(response) {
        console.log(response.data); // Array of posts
    }
});
```

#### RAG Query
```javascript
jQuery.ajax({
    url: themisdb.ajax_url,
    method: 'POST',
    data: {
        action: 'themis_rag_query',
        nonce: themisdb.rag_nonce,
        query: 'How do I install WordPress?'
    },
    success: function(response) {
        console.log(response.data.answer);
    }
});
```

## Technische Details

### Post Synchronization

Wenn ein Post veröffentlicht wird:

1. **Daten extrahieren:**
   - Titel, Content, Autor, Kategorien, Tags
   
2. **In ThemisDB speichern:**
   ```php
   $client->put('document', 'wordpress_posts', $post_id, $data);
   ```

3. **Embedding generieren:**
   ```aql
   LLM EMBED @content USING MODEL "sentence-transformers"
   ```

4. **Vektor speichern:**
   ```php
   $client->vectorUpsert("post_{$post_id}", $embedding, $metadata);
   ```

### Semantic Search Flow

1. **User Query** → "machine learning tutorials"

2. **Generate Query Embedding:**
   ```aql
   LLM EMBED @query USING MODEL "sentence-transformers"
   ```

3. **Vector Search:**
   ```php
   $results = $client->vectorSearch($embedding, 10);
   ```

4. **Fetch Post Details from MySQL:**
   ```php
   $post = get_post($post_id);
   ```

5. **Return Results** → Array of matching posts with scores

### RAG (Retrieval-Augmented Generation)

1. **User Question** → "How do I install WordPress?"

2. **Search Relevant Content:**
   - Vector Search für relevante Posts
   - Top 5 relevante Dokumente

3. **Generate Answer with LLM:**
   ```aql
   LLM RAG @query FROM COLLECTION wordpress_posts TOP 5 USING MODEL @model
   ```

4. **Return Answer + Sources**

## Dateistruktur

```
wordpress-integration-example/
├── themisdb-llm-integration.php    # Main plugin file
├── README.md                        # This file
├── composer.json                    # PHP dependencies
├── assets/
│   ├── css/
│   │   ├── frontend.css            # Frontend styles
│   │   └── admin.css               # Admin styles
│   └── js/
│       ├── frontend.js             # Frontend JavaScript
│       └── admin.js                # Admin JavaScript
└── templates/                       # (Optional) Template files
```

## Erweiterungsmöglichkeiten

### 1. Content Recommendations
```php
// Empfohlen für aktuellen Post basierend auf Graph-Beziehungen
$related = $client->graphTraverse("post_{$post_id}", 2, 'TAGGED_WITH');
```

### 2. Automated Tagging
```php
// LLM analysiert Post-Content und schlägt Tags vor
$result = $client->query(
    'LLM INFER "Extract 5 relevant tags from this text: @content"',
    ['params' => ['content' => $post_content]]
);
```

### 3. Content Similarity Detection
```php
// Finde ähnliche Posts vor dem Veröffentlichen (Duplicate Detection)
$similar = $client->vectorSearch($post_embedding, 5);
if ($similar['results'][0]['score'] > 0.95) {
    // Warnung: Sehr ähnlicher Content existiert bereits
}
```

### 4. Knowledge Graph
```php
// Erstelle Beziehungen zwischen Posts
$client->put('graph', 'edges', $edge_id, [
    'from' => "post_{$post_id}",
    'to' => "post_{$related_id}",
    'type' => 'RELATED_TO',
    'strength' => 0.85
]);
```

## Performance

### Caching
- WordPress Object Cache für häufige Queries
- ThemisDB integriertes Caching für LLM-Inferenz
- Browser-Caching für Frontend-Assets

### Rate Limiting
- 100 Queries pro Stunde pro User
- Verhindert Missbrauch von LLM-Features

## Sicherheit

### Input Sanitization
- Alle User-Inputs werden mit WordPress-Funktionen gesäubert
- `sanitize_text_field()`, `wp_kses_post()`, etc.

### Nonce Verification
- Alle AJAX-Requests benötigen Nonce-Verification
- `check_ajax_referer()` für jede Anfrage

### Parametrized Queries
- ThemisDB Queries nutzen parametrisierte Abfragen
- Verhindert Injection-Angriffe

## Troubleshooting

### ThemisDB Verbindung fehlgeschlagen
```
Error: Could not connect to ThemisDB
```
**Lösung:**
- Prüfen ob ThemisDB Server läuft: `curl http://localhost:8080/health`
- Firewall-Einstellungen überprüfen
- Endpoint in WordPress Admin korrekt konfigurieren

### Embeddings werden nicht generiert
```
Error: Failed to generate embedding
```
**Lösung:**
- LLM Model muss in ThemisDB geladen sein
- Prüfen mit: `LLM MODEL LIST`
- Model laden: `LLM MODEL LOAD "sentence-transformers"`

### Posts werden nicht synchronisiert
**Lösung:**
- Auto Sync in Settings aktivieren
- WordPress Debug Log prüfen (`wp-content/debug.log`)
- ThemisDB Logs prüfen

## Support

- **GitHub Issues:** https://github.com/makr-code/ThemisDB/issues
- **Documentation:** `/clients/WORDPRESS_THEMISDB_FEASIBILITY_STUDY.md`
- **ThemisDB Docs:** https://makr-code.github.io/ThemisDB/

## Lizenz

MIT License - Copyright (c) 2026 ThemisDB Team

## Related Documentation

- [Feasibility Study](../WORDPRESS_THEMISDB_FEASIBILITY_STUDY.md)
- [ThemisDB PHP SDK](../php/README.md)
- [AQL Reference](../../aql/README.md)
