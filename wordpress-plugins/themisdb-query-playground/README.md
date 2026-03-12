# ThemisDB Query Playground - WordPress Plugin

Interactive AQL query playground for ThemisDB with live query execution, perfect for showcasing Wikipedia Knowledge Graph use case.

## 📋 Overview

This plugin provides an interactive query interface for ThemisDB, designed to demonstrate multi-model capabilities with the German Wikipedia knowledge graph.

- **Shortcode**: `[themisdb_query_playground]`
- **Live Execution**: Execute AQL queries against ThemisDB
- **Multiple Views**: Table, JSON, Chart visualization
- **Example Queries**: Pre-built queries for Wikipedia knowledge graph

## ✨ Features

### Query Editor
- 📝 **CodeMirror Integration**: Syntax highlighting for AQL
- ⌨️ **Keyboard Shortcuts**: Ctrl/Cmd+Enter to execute
- 🎨 **Multiple Themes**: Monokai, Dracula, etc.
- 📊 **Auto-format**: Format queries automatically

### Query Execution
- ⚡ **Live Execution**: Real-time query against ThemisDB
- 🔒 **Read-Only Mode**: Safe public deployment
- 📈 **Performance Metrics**: Execution time tracking
- 🎯 **Result Limiting**: Configurable max results

### Wikipedia Knowledge Graph Showcase
- 📚 **Relational Model**: Article metadata, categories
- 🔗 **Graph Model**: Category relationships, page links
- 🔍 **Vector Search**: Semantic search with embeddings
- 🤖 **LLM Integration**: Natural language queries (llama.cpp without GPU!)

### Result Visualization
- 📋 **Table View**: Tabular data display
- 📄 **JSON View**: Raw JSON output
- 📊 **Chart View**: Visual data representation
- 💾 **Export**: CSV and JSON export

## 🚀 Installation

1. **Copy Plugin**
   ```bash
   cd /path/to/wordpress/wp-content/plugins/
   cp -r /path/to/ThemisDB/tools/query-playground-wordpress ./themisdb-query-playground
   ```

2. **Install ThemisDB PHP Client**
   ```bash
   cd /path/to/ThemisDB/clients/php
   composer install
   ```

3. **Configure Plugin**
   - Go to Settings → Query Playground
   - Set ThemisDB endpoint (e.g., `http://themisdb:8080`)
   - Set PHP client path
   - Enable execution and examples

4. **Activate Plugin**
   - WordPress Admin → Plugins
   - Activate "ThemisDB Query Playground"

## 📖 Usage

### Basic Shortcode
```php
[themisdb_query_playground]
```

### With Default Query
```php
[themisdb_query_playground 
    default_query="SELECT * FROM urn:themis:relational:wikipedia_articles LIMIT 10"]
```

### Custom Height and Theme
```php
[themisdb_query_playground height="600px" theme="dracula"]
```

## 🌐 Wikipedia Knowledge Graph Queries

### Search Articles by Title
```sql
SELECT title, summary, categories 
FROM urn:themis:relational:wikipedia_articles
WHERE title LIKE '%Künstliche Intelligenz%'
LIMIT 10
```

### Find Related Categories (Graph)
```sql
MATCH (cat:Category {name: 'Informatik'})-[:SUBCATEGORY*1..3]->(subcat)
RETURN subcat.name, subcat.article_count
ORDER BY subcat.article_count DESC
LIMIT 20
```

### Semantic Search with Vectors
```sql
SELECT title, summary, 
       VECTOR_SIMILARITY(embedding, @query_embedding) as similarity
FROM urn:themis:vector:wikipedia_embeddings
WHERE VECTOR_SIMILARITY(embedding, @query_embedding) > 0.75
ORDER BY similarity DESC
LIMIT 10
```

### LLM-Powered Natural Language Query
```sql
SELECT title, summary
FROM urn:themis:relational:wikipedia_articles
WHERE LLM_SIMILARITY(summary, 'Erkläre mir maschinelles Lernen') > 0.7
LIMIT 5
```

### Category Statistics (Analytics)
```sql
SELECT category, 
       COUNT(*) as article_count,
       AVG(LENGTH(content)) as avg_length
FROM urn:themis:relational:wikipedia_articles
GROUP BY category
ORDER BY article_count DESC
LIMIT 20
```

## ⚙️ Settings

### Connection Settings
- **ThemisDB Endpoint**: URL to ThemisDB instance
- **Namespace**: Database namespace (default: `default`)
- **Timeout**: Query timeout in seconds
- **PHP Client Path**: Path to ThemisDB PHP client

### Security Settings
- **Enable Execution**: Allow/disallow query execution
- **Read-Only Mode**: Prevent INSERT/UPDATE/DELETE
- **Max Results**: Limit result rows

### Display Settings
- **Enable Examples**: Show example queries
- **Editor Theme**: CodeMirror theme

## 🔒 Security

- **Read-Only Mode**: Blocks write operations
- **Nonce Verification**: AJAX request protection
- **Query Validation**: Sanitize user input
- **Result Limiting**: Prevent large result sets

## 🎯 Wikipedia Use Case

Perfect for demonstrating ThemisDB capabilities:

1. **Multi-Model**: Same data, different models
2. **Vector Search**: Semantic article search
3. **Graph Traversal**: Category relationships
4. **LLM Integration**: Natural language queries
5. **Performance**: Fast queries on large dataset

## 📄 License

MIT License

## 🔗 Links

- **GitHub**: [makr-code/wordpressPlugins](https://github.com/makr-code/wordpressPlugins)
- **ThemisDB PHP Client**: `/clients/php/`

---

**Powered by [ThemisDB](https://github.com/makr-code/wordpressPlugins)** - Phase 2.2 Implementation
