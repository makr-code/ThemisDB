# AQL (Advanced Query Language) - Formale Grammatik

Dieses Verzeichnis enthält die formale Grammatikdefinition von ThemisDB's AQL in EBNF-Notation.

## Dateien

- **AQL_GRAMMAR.ebnf** - Vollständige EBNF-Grammatik für AQL (v1.3.0)
- **AQL_GRAMMAR_EXTENDED_v1.3.1.ebnf** - 📋 PROPOSAL: Erweiterte Grammatik mit OOP-Features (v1.3.1)
- **examples/vision_analysis_oop.aql** - 📋 PROPOSAL: Beispiele für Vision-Analyse mit OOP

## AQL Übersicht

AQL ist ThemisDB's deklarative Abfragesprache für Multi-Model-Datenbank-Operationen:

- **Relational**: Joins, Aggregationen, Gruppierung
- **Graph**: Traversierung (OUTBOUND, INBOUND, ANY), Kürzester Pfad
- **Dokument**: Flexible JSON-Queries
- **Vektor**: Ähnlichkeitssuchen
- **Geo-Spatial**: ST_*-Funktionen
- **Zeitreihen**: Optimierte Speicherung, Gorilla-Kompression, kontinuierliche Aggregation
- **Prozessabbildung**: Event-Log-Extraktion, Process-Discovery, Konformitätsprüfung
- **LLM** (v1.3.0+): Native LLM-Inferenz, RAG, Embeddings

## Sprachfeatures

### Core Clauses
- `FOR` - Iteration über Collections, Ranges, Graph-Traversals
- `LET` - Variable Bindings
- `FILTER` - Prädikate und Bedingungen
- `COLLECT` - Gruppierung und Aggregation
- `SORT` - Sortierung
- `LIMIT` - Pagination
- `RETURN` - Projektion

### DDL (Data Definition Language)
- `CREATE/DROP COLLECTION`
- `CREATE/DROP INDEX` (HASH, SKIPLIST, FULLTEXT, GEO, VECTOR, etc.)
- `CREATE/DROP VIEW`

### DML (Data Manipulation Language)
- `INSERT` - Neue Dokumente einfügen
- `UPDATE` - Dokumente aktualisieren
- `REPLACE` - Dokumente ersetzen
- `REMOVE` - Dokumente löschen
- `UPSERT` - Insert oder Update

### LLM Extensions (v1.3.0+)
- `LLM INFER` - Inferenz mit LLM-Modellen
- `LLM RAG` - Retrieval-Augmented Generation
- `LLM EMBED` - Text-Embeddings generieren
- `LLM MODEL` - Modell-Verwaltung (LOAD, UNLOAD, LIST, INGEST)
- `LLM LORA` - LoRA-Adapter-Verwaltung
- `LLM STATS` - Statistiken
- `LLM CACHE` - Cache-Verwaltung

### 📋 Proposed OOP Extensions (v1.3.1)
- `NAMESPACE` - Hierarchische Organisation von Code
- `TYPE` - User-Defined Types für Typsicherheit
- `FUNCTION` - Wiederverwendbare User-Defined Functions
- `CLASS` - Objektorientierte Workflows mit State Management
- `TRY/CATCH` - Strukturierte Fehlerbehandlung
- `MATCH` - Pattern Matching für komplexe Datenstrukturen
- `ASYNC/AWAIT` - Asynchrone LLM-Operationen
- Pipeline Operator `|>` - Method Chaining für bessere Lesbarkeit

### 📋 Proposed Vision Extensions (v1.3.1)
- `LLM VISION ANALYZE` - Strukturierte Bildanalyse mit Objekterkennung
- `LLM VISION BATCH` - Batch-Verarbeitung mehrerer Bilder
- `LLM VISION QUESTION` - Visual Question Answering (VQA)
- `LLM VISION COMPARE` - Bildvergleich und Ähnlichkeitsanalyse
- `LLM VISION TRANSFORM` - Bildtransformationen
- `LLM VISION RAG` - Multimodales RAG mit Bildern und Text

## Beispiel-Queries

### Einfache Query
```aql
FOR user IN users
  FILTER user.age > 18
  SORT user.name ASC
  LIMIT 10
  RETURN user
```

### Join Query
```aql
FOR user IN users
  FOR order IN orders
    FILTER order.user_id == user._key
    RETURN {
      user_name: user.name,
      order_total: order.total
    }
```

### Graph Traversal
```aql
FOR vertex IN 1..3 OUTBOUND 'users/john' friends
  RETURN vertex.name
```

### Aggregation
```aql
FOR order IN orders
  COLLECT city = order.city
  AGGREGATE total_sales = SUM(order.amount)
  SORT total_sales DESC
  RETURN { city, total_sales }
```

### Vector Search
```aql
FOR doc IN documents
  FILTER SIMILARITY(doc.embedding, @query_vector, 10)
  RETURN doc
```

### LLM Inference (v1.3.0+)
```aql
LLM INFER "Explain the concept of databases"
  USING MODEL "llama-2-7b"
  OPTIONS { max_tokens: 200, temperature: 0.7 }
```

### RAG Query (v1.3.0+)
```aql
LLM RAG "What are the key features of ThemisDB?"
  FROM COLLECTION documents
  TOP 5
  USING LORA "technical-docs"
```

## Weitere Dokumentation

- **Syntax-Guide**: `../docs/de/aql/aql_syntax.md`
- **Funktions-Referenz**: `../docs/de/aql/aql_functions_reference.md`
- **Query Engine**: `../docs/de/aql/aql_query_engine.md`
- **Subqueries**: `../docs/de/aql/aql_subquery_reference.md`
- **Hybrid Queries**: `../docs/de/aql/aql_hybrid_queries.md`
- **LLM-Erweiterungen**: `../docs/de/aql/LLM_GRAMMAR.ebnf`
- **📋 OOP Extension Proposal**: `../docs/de/aql/AQL_OOP_EXTENSION_PROPOSAL.md` (NEU)

## 📋 Vorschlag: OOP & Vision Extensions (v1.3.1)

Siehe detaillierten Vorschlag in `../docs/de/aql/AQL_OOP_EXTENSION_PROPOSAL.md`

Diese Erweiterungen zielen darauf ab:
1. **Modularität** durch Namespaces zu verbessern
2. **Wiederverwendbarkeit** mit User-Defined Functions und Types
3. **Ausdruckskraft** durch Classes und Pattern Matching
4. **Vision-Integration** für llama.cpp vision und multimodale RAG
5. **Best Practices** für Enterprise-Anwendungen zu etablieren

## EBNF-Notation

Die Grammatik verwendet Standard-EBNF-Notation:

- `::=` - Definition
- `|` - Alternative
- `()` - Gruppierung
- `[]` - Optional (0 oder 1)
- `{}` - Wiederholung (0 oder mehr)
- `+` - Wiederholung (1 oder mehr)
- `*` - Wiederholung (0 oder mehr)
- `?` - Optional (0 oder 1)
- `"..."` - Terminal (Literal)
- `(* ... *)` - Kommentar

## Version

- **Version**: 1.3.0 (Aktuell Production-Ready)
- **Version**: 1.3.1 (📋 Vorschlag für OOP & Vision Extensions)
- **Datum**: 22. Dezember 2025
- **Status**: Siehe Proposal-Dokumente für Details

## Lizenz

Copyright (c) 2025 ThemisDB. Alle Rechte vorbehalten.
