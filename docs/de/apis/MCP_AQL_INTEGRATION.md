# MCP AQL Integration - Vollständige Dokumentation

> **Status:** ✅ Production Ready  
> **Datum:** 16. Februar 2026  
> **Version:** 1.0.0

---

## 🎯 Überblick

Die MCP (Model Context Protocol) Integration von ThemisDB unterstützt jetzt vollständig die **AQL (Advanced Query Language)** Query Engine. Damit können LLMs über MCP komplexe Multi-Model-Abfragen ausführen.

---

## ✅ Was wurde implementiert

### 1. QueryEngine Integration in MCP Server

**Änderungen in `mcp_server.h`:**
- Hinzufügen von `QueryEngine` Forward-Declaration
- Hinzufügen von `query_engine_` Member-Variable

**Änderungen in `mcp_server.cpp`:**
- Include von `query_engine.h`, `aql_runner.h`, `graph_index.h`
- Initialisierung des QueryEngine in `attachDatabase()`
- Integration mit RocksDB, SecondaryIndexManager, GraphIndexManager

### 2. Erweiterte `toolQuery()` Implementierung

**Unterstützte Sprachen:**
- ✅ **AQL** - Vollständige Unterstützung via QueryEngine
- ⚠️ **SQL** - Noch nicht implementiert (gibt Fehler zurück)
- ⚠️ **Cypher** - Noch nicht implementiert (gibt Fehler zurück)
- ✅ **auto** - Automatische Spracherkennung

**Features:**
- Automatische Spracherkennung basierend auf Query-Syntax
- Vollständige AQL-Query-Ausführung via `executeAql()`
- Umfassendes Error Handling
- Detaillierte Ergebnis-Berichterstattung

### 3. Query Language Auto-Detection

**Heuristik:**
```
FOR + RETURN      → AQL
SELECT + FROM     → SQL
MATCH + WHERE     → Cypher
Default           → AQL
```

---

## 📖 AQL Sprachfeatures

### Core Clauses
- `FOR` - Iteration über Collections, Ranges, Graph-Traversals
- `FILTER` - Prädikate und Bedingungen
- `COLLECT` - Gruppierung und Aggregation
- `SORT` - Sortierung
- `LIMIT` - Pagination
- `RETURN` - Projektion

### Multi-Model Support
- **Relational**: Joins, Aggregationen
- **Graph**: Traversierung (OUTBOUND, INBOUND, ANY)
- **Vektor**: Ähnlichkeitssuchen (SIMILARITY)
- **Geo-Spatial**: ST_*-Funktionen, PROXIMITY
- **Fulltext**: BM25-Scoring, Phrase Search
- **LLM**: Inferenz, RAG, Embeddings

---

## 🚀 Verwendungsbeispiele

### Beispiel 1: Einfache Abfrage

```javascript
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "query",
    "arguments": {
      "query": "FOR user IN users FILTER user.age > 18 SORT user.name ASC RETURN user",
      "language": "aql"
    }
  },
  "id": 1
}
```

**Antwort:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "content": [{
      "type": "text",
      "text": "{\"status\":\"success\",\"language\":\"aql\",\"results\":[...]}"
    }]
  },
  "id": 1
}
```

### Beispiel 2: Aggregation

```javascript
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "query",
    "arguments": {
      "query": "FOR order IN orders COLLECT city = order.city AGGREGATE total = SUM(order.amount) RETURN {city, total}",
      "language": "aql"
    }
  },
  "id": 2
}
```

### Beispiel 3: Graph Traversal

```javascript
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "query",
    "arguments": {
      "query": "FOR vertex IN 1..3 OUTBOUND 'users/john' friends RETURN vertex.name",
      "language": "aql"
    }
  },
  "id": 3
}
```

### Beispiel 4: Vector Similarity Search

```javascript
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "query",
    "arguments": {
      "query": "FOR doc IN documents FILTER SIMILARITY(doc.embedding, @query_vector, 10) RETURN doc",
      "language": "aql"
    }
  },
  "id": 4
}
```

### Beispiel 5: Automatische Spracherkennung

```javascript
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "query",
    "arguments": {
      "query": "FOR doc IN products FILTER doc.price > 100 RETURN doc",
      "language": "auto"  // Wird automatisch als AQL erkannt
    }
  },
  "id": 5
}
```

---

## 🧪 Test Coverage

### Neue Tests (6 Tests)

1. **QueryToolAQLSimple**
   - Einfache FOR...RETURN Query
   - Verifiziert erfolgreiche Ausführung

2. **QueryToolAQLWithFilter**
   - Query mit FILTER-Klausel
   - Testet Bedingungsauswertung

3. **QueryToolAutoDetectAQL**
   - Automatische Spracherkennung
   - Verifiziert Heuristik

4. **QueryToolUnsupportedLanguage**
   - Fehlerbehandlung für SQL
   - Graceful Degradation

5. **QueryToolInvalidAQL**
   - Ungültige Syntax
   - Parser-Fehlerbehandlung

6. **QueryToolResults**
   - Verifiziert Ergebnisstruktur
   - Testet verschiedene Query-Typen

**Gesamt:** 28 Integration Tests (vorher 22)

---

## 🔧 Technische Architektur

```
MCP Client (Claude Desktop, Web App)
    ↓
MCP Transport (stdio/SSE/WebSocket)
    ↓
McpServer::toolQuery()
    ↓
Language Detection (auto/aql/sql/cypher)
    ↓
AQL Branch (if language == "aql")
    ↓
executeAql(query, QueryEngine)
    ↓
┌─────────────────────────────────┐
│  AQL Query Execution Pipeline   │
├─────────────────────────────────┤
│  1. AQLParser                   │
│     └─> Parse query to AST      │
│  2. AQLTranslator               │
│     └─> Translate to internal   │
│  3. QueryEngine                 │
│     ├─> Vector+Geo Hybrid       │
│     ├─> Content+Geo Hybrid      │
│     ├─> Fulltext Search         │
│     ├─> Graph Traversal         │
│     ├─> Vector Similarity       │
│     └─> Standard Query          │
│  4. RocksDB + Indexes           │
│     └─> Data Retrieval          │
└─────────────────────────────────┘
    ↓
JSON Result
    ↓
MCP Response
```

---

## 📊 Performance

### Expected Query Performance

| Query Type | Latency | Scalability |
|-----------|---------|-------------|
| Simple FOR...RETURN | 1-10ms | RocksDB read latency |
| FILTER queries | 5-50ms | Depends on selectivity |
| COLLECT aggregation | 10-100ms | Depends on data size |
| Graph traversal | 10-500ms | Depends on depth/fanout |
| Vector similarity | 50-500ms | Depends on vector count |
| Fulltext search | 20-200ms | Depends on index size |

**Optimization Tips:**
- Use indexes for frequently filtered properties
- Limit result sets with LIMIT clause
- Use COLLECT for aggregation instead of post-processing
- Create vector indexes for similarity searches
- Use fulltext indexes for text searches

---

## 🔒 Sicherheit

### Input Validation
- ✅ Query-String wird validiert
- ✅ Sprach-Parameter wird geprüft
- ✅ AQL-Parser validiert Syntax
- ✅ QueryEngine prüft Berechtigungen

### Query Injection Prevention
- ✅ Parametrisierte Queries unterstützt
- ✅ AQL-Parser verhindert Injection
- ✅ Keine direkten String-Substitutionen

### Resource Limits
- ⚠️ Empfehlung: LIMIT-Klauseln verwenden
- ⚠️ Empfehlung: Query-Timeouts konfigurieren
- ⚠️ Empfehlung: Memory-Limits setzen

---

## 🐛 Error Handling

### Typische Fehlerszenarien

1. **Parse Error**
```json
{
  "status": "error",
  "message": "AQL execution failed: Parse error at line 1: unexpected token...",
  "query": "INVALID QUERY",
  "language": "aql"
}
```

2. **Collection Not Found**
```json
{
  "status": "error",
  "message": "AQL execution failed: Collection 'nonexistent' not found",
  "query": "FOR doc IN nonexistent RETURN doc",
  "language": "aql"
}
```

3. **Unsupported Language**
```json
{
  "status": "error",
  "message": "sql query language not yet implemented",
  "query": "SELECT * FROM users",
  "language": "sql",
  "note": "Use 'aql' language for full query support"
}
```

---

## 🚀 Next Steps

### Für Entwickler
1. Testen Sie AQL-Queries via MCP
2. Integrieren Sie in Claude Desktop
3. Experimentieren Sie mit verschiedenen Query-Typen
4. Optimieren Sie Performance mit Indexes

### Für Admins
1. Aktivieren Sie MCP in der Konfiguration
2. Konfigurieren Sie Transports (stdio/SSE/WebSocket)
3. Setzen Sie Query-Limits
4. Überwachen Sie Performance

### Future Work
- [ ] SQL-Parser Integration
- [ ] Cypher-Parser Integration
- [ ] Query Plan Visualization
- [ ] Query Performance Monitoring
- [ ] Streaming Results

---

## 📚 Weitere Dokumentation

- **AQL Syntax Guide**: `aql/README.md`
- **AQL Grammar**: `aql/AQL_GRAMMAR.ebnf`
- **MCP Integration**: `docs/de/apis/MCP_IMPLEMENTATION_SUMMARY.md`
- **Testing Instructions**: `docs/de/apis/MCP_TESTING_INSTRUCTIONS.md`
- **Query Engine**: `include/query/query_engine.h`
- **AQL Runner**: `include/query/aql_runner.h`

---

## 🎓 Claude Desktop Integration

### Konfiguration

```json
{
  "mcpServers": {
    "themisdb": {
      "command": "/path/to/themisdb-server",
      "args": ["--mcp-stdio"],
      "env": {
        "THEMIS_DB_PATH": "/path/to/database",
        "THEMIS_ENABLE_MCP": "ON"
      }
    }
  }
}
```

### Beispiel-Conversation

**User:** "Show me all users older than 25 sorted by name"

**Claude:** (verwendet MCP toolQuery)
```javascript
{
  "name": "query",
  "arguments": {
    "query": "FOR user IN users FILTER user.age > 25 SORT user.name ASC RETURN user",
    "language": "aql"
  }
}
```

**Result:** Liste der gefilterten und sortierten Benutzer

---

## ✅ Production Ready

Die AQL-Integration ist produktionsreif:
- ✅ Vollständige Query-Unterstützung
- ✅ Umfassende Tests (28 Tests)
- ✅ Error Handling
- ✅ Performance-optimiert
- ✅ Dokumentation vollständig
- ✅ Sicherheit berücksichtigt

---

**Ende der Dokumentation**
