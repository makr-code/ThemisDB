> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Wikipedia to ThemisDB Ingestion Tool

Tool zum Importieren der deutschen Wikipedia in ThemisDB als Multi-Model Wissensgraph.

## 🎯 Ziel

Deutsche Wikipedia in ThemisDB laden und Wissensgraph aufbauen - zeigt ThemisDB Multi-Model Stärken:
- 📊 **Relational**: Artikel-Metadaten  
- 🔗 **Graph**: Kategorie-Hierarchien, Links
- 🔍 **Vector**: Semantische Suche
- 🤖 **LLM**: Natural Language Queries (llama.cpp ohne GPU!)

## 📋 Features

- Wikipedia Dump Download & Parse
- Multi-Model Data Ingestion
- Embedding Generation (Sentence-Transformers)
- LLM Integration (llama.cpp)
- Progress Tracking

## 🚀 Quick Start

```bash
# 1. Download Wikipedia Dump
./scripts/download_wikipedia.sh de latest

# 2. Ingest into ThemisDB
php ingest.php --dump=data/dewiki-latest-pages-articles.xml.bz2

# 3. Generate Embeddings
python generate_embeddings.py
```

## 📊 Data Models

### Relational
```sql
SELECT * FROM urn:themis:relational:wikipedia_articles 
WHERE title LIKE '%KI%' LIMIT 10;
```

### Graph
```sql
MATCH (cat:Category)-[:SUBCATEGORY*1..3]->(sub)
WHERE cat.name = 'Informatik'
RETURN sub.name;
```

### Vector
```sql
SELECT title, VECTOR_SIMILARITY(embedding, @query) as score
FROM urn:themis:vector:wikipedia_embeddings
ORDER BY score DESC LIMIT 10;
```

### LLM
```sql
SELECT * FROM urn:themis:relational:wikipedia_articles
WHERE LLM_SIMILARITY(content, 'Erkläre Berlin') > 0.7;
```

## 📈 Performance

- **Ingestion**: ~1000 Artikel/min
- **Datenvolumen**: ~2.8M Artikel, 15-20GB
- **LLM**: CPU-only, kein GPU/VRAM!

---

**Status**: Konzept & Planung  
**Integration**: Mit Query Playground Plugin
