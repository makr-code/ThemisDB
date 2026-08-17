# MCP API Specification - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [MCP Architektur](#mcp-architektur)
- [Transport Layers](#transport-layers)
- [Tools (Database Operations)](#tools-database-operations)
- [Resources (Schema & Metadata)](#resources-schema--metadata)
- [Prompts (Query Templates)](#prompts-query-templates)
- [Authentifizierung](#authentifizierung)
- [Beispiele](#beispiele)
- [Best Practices](#best-practices)

---

## Übersicht

Das **Model Context Protocol (MCP)** ermöglicht LLM-Anwendungen (wie Claude, GPT-4, etc.) den direkten Zugriff auf ThemisDB. MCP ist ein offenes Protokoll von Anthropic, das standardisierte Kommunikation zwischen AI und Datenquellen ermöglicht.

### MCP Vorteile

- ✅ **Natural Language Queries**: LLMs können Datenbankabfragen in natürlicher Sprache formulieren
- ✅ **Tool Integration**: Datenbank-Operationen als LLM-Tools
- ✅ **Context Sharing**: Schema und Metadaten für bessere AI-Responses
- ✅ **Standardisiertes Protokoll**: JSON-RPC 2.0 basiert
- ✅ **Multi-Transport**: stdio, HTTP, WebSocket

---

## MCP Architektur

### Komponenten

```
┌─────────────────────┐
│   LLM Application   │ (Claude Desktop, Custom AI App)
│  (MCP Client)       │
└──────────┬──────────┘
           │ MCP Protocol (JSON-RPC 2.0)
           │
┌──────────▼──────────┐
│   MCP Server        │
│   (ThemisDB)        │
├─────────────────────┤
│ Tools:              │ - query, put_entity, get_entity
│ Resources:          │ - schema, statistics, metadata
│ Prompts:            │ - query templates
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│   ThemisDB          │
│   Core Engine       │
└─────────────────────┘
```

### Protocol Flow

1. **Initialize**: Client connects to MCP server
2. **List Capabilities**: Server sends available tools/resources/prompts
3. **Request**: Client calls tools or requests resources
4. **Execute**: Server executes database operations
5. **Response**: Server returns results in JSON format

---

## Transport Layers

### 1. Standard I/O (stdio)

**Verwendung**: Desktop applications, command-line tools

**Konfiguration:**
```json
{
  "mcpServers": {
    "themisdb": {
      "command": "themis-mcp-server",
      "args": ["--database", "mydb", "--host", "localhost"],
      "env": {
        "THEMIS_TOKEN": "your-jwt-token"
      }
    }
  }
}
```

**Start:**
```bash
themis-mcp-server --database mydb --host localhost:7687
```

### 2. HTTP/HTTPS

**Endpoint:**
```
POST https://your-themis-instance.com/mcp
```

**Headers:**
```http
Content-Type: application/json
Authorization: Bearer <your-jwt-token>
```

### 3. WebSocket

**Endpoint:**
```
wss://your-themis-instance.com/mcp/ws
```

**Connection:**
```javascript
const ws = new WebSocket('wss://your-themis-instance.com/mcp/ws');
ws.onopen = () => {
  ws.send(JSON.stringify({
    jsonrpc: "2.0",
    method: "initialize",
    params: {
      protocolVersion: "2024-11-05",
      clientInfo: {
        name: "my-ai-app",
        version: "1.0.0"
      }
    },
    id: 1
  }));
};
```

---

## Tools (Database Operations)

### Tool: query

AQL/Cypher/SQL Queries ausführen.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "query",
    "arguments": {
      "query": "FOR doc IN users FILTER doc.age > @minAge RETURN doc",
      "bindVars": {
        "minAge": 25
      },
      "language": "aql"
    }
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Found 3 users:\n- John (30)\n- Alice (28)\n- Bob (35)"
      },
      {
        "type": "resource",
        "resource": {
          "uri": "themisdb://mydb/query-results/1",
          "mimeType": "application/json",
          "text": "[{\"name\":\"John\",\"age\":30},{\"name\":\"Alice\",\"age\":28},{\"name\":\"Bob\",\"age\":35}]"
        }
      }
    ]
  },
  "id": 1
}
```

### Tool: put_entity

Dokument/Entity erstellen oder aktualisieren.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "put_entity",
    "arguments": {
      "collection": "users",
      "entity": {
        "name": "Jane Doe",
        "email": "jane@example.com",
        "age": 28,
        "role": "developer"
      },
      "metadata": {
        "source": "mcp-api",
        "tags": ["new-user"]
      }
    }
  },
  "id": 2
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "content": [
      {
        "type": "text",
        "text": "Entity created successfully"
      },
      {
        "type": "resource",
        "resource": {
          "uri": "themisdb://mydb/users/user_12345",
          "mimeType": "application/json",
          "text": "{\"id\":\"user_12345\",\"name\":\"Jane Doe\",\"email\":\"jane@example.com\"}"
        }
      }
    ],
    "isError": false
  },
  "id": 2
}
```

### Tool: get_entity

Entity nach ID abrufen.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "get_entity",
    "arguments": {
      "collection": "users",
      "id": "user_12345"
    }
  },
  "id": 3
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "content": [
      {
        "type": "resource",
        "resource": {
          "uri": "themisdb://mydb/users/user_12345",
          "mimeType": "application/json",
          "text": "{\"id\":\"user_12345\",\"name\":\"Jane Doe\",\"email\":\"jane@example.com\",\"age\":28,\"role\":\"developer\"}"
        }
      }
    ]
  },
  "id": 3
}
```

### Tool: delete_entity

Entity löschen.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "delete_entity",
    "arguments": {
      "collection": "users",
      "id": "user_12345"
    }
  },
  "id": 4
}
```

### Tool: create_index

Index erstellen.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "create_index",
    "arguments": {
      "collection": "users",
      "fields": ["email"],
      "type": "hash",
      "unique": true
    }
  },
  "id": 5
}
```

### Tool: vector_search

Vector Similarity Search.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "vector_search",
    "arguments": {
      "collection": "documents",
      "vector": [0.1, 0.2, 0.3, /* ... */],
      "k": 5,
      "metric": "cosine"
    }
  },
  "id": 6
}
```

---

## Resources (Schema & Metadata)

### Resource: get_schema

Datenbank-Schema abrufen.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "resources/read",
  "params": {
    "uri": "themisdb://mydb/schema"
  },
  "id": 7
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "contents": [
      {
        "uri": "themisdb://mydb/schema",
        "mimeType": "application/json",
        "text": "{\"collections\":[{\"name\":\"users\",\"fields\":[\"name\",\"email\",\"age\"],\"indexes\":[{\"name\":\"idx_email\",\"fields\":[\"email\"],\"unique\":true}]}]}"
      }
    ]
  },
  "id": 7
}
```

### Resource: get_stats

Statistiken abrufen.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "resources/read",
  "params": {
    "uri": "themisdb://mydb/stats"
  },
  "id": 8
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "contents": [
      {
        "uri": "themisdb://mydb/stats",
        "mimeType": "application/json",
        "text": "{\"collections\":5,\"documents\":10000,\"indexes\":12,\"size_bytes\":1048576}"
      }
    ]
  },
  "id": 8
}
```

### Resource List

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "resources/list",
  "params": {},
  "id": 9
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "resources": [
      {
        "uri": "themisdb://mydb/schema",
        "name": "Database Schema",
        "description": "Complete database schema including collections and indexes",
        "mimeType": "application/json"
      },
      {
        "uri": "themisdb://mydb/stats",
        "name": "Database Statistics",
        "description": "Collection counts, document counts, and size metrics",
        "mimeType": "application/json"
      }
    ]
  },
  "id": 9
}
```

---

## Prompts (Query Templates)

### Prompt: analyze_data

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "prompts/get",
  "params": {
    "name": "analyze_data",
    "arguments": {
      "collection": "sales",
      "metric": "revenue"
    }
  },
  "id": 10
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "description": "Analyze data in a collection",
    "messages": [
      {
        "role": "user",
        "content": {
          "type": "text",
          "text": "Analyze the revenue data in the sales collection. Calculate total, average, min, and max values. Identify trends and outliers."
        }
      }
    ]
  },
  "id": 10
}
```

### Prompt List

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "prompts/list",
  "params": {},
  "id": 11
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "prompts": [
      {
        "name": "analyze_data",
        "description": "Analyze data in a collection with statistical metrics",
        "arguments": [
          {
            "name": "collection",
            "description": "Collection name to analyze",
            "required": true
          },
          {
            "name": "metric",
            "description": "Metric to analyze",
            "required": true
          }
        ]
      },
      {
        "name": "find_similar",
        "description": "Find similar documents using vector search",
        "arguments": [
          {
            "name": "collection",
            "description": "Collection to search in",
            "required": true
          },
          {
            "name": "query",
            "description": "Search query text",
            "required": true
          }
        ]
      }
    ]
  },
  "id": 11
}
```

---

## Authentifizierung

### JWT Token

**Via Environment Variable:**
```bash
export THEMIS_TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
themis-mcp-server --database mydb
```

**Via Configuration:**
```json
{
  "mcpServers": {
    "themisdb": {
      "command": "themis-mcp-server",
      "env": {
        "THEMIS_TOKEN": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
      }
    }
  }
}
```

**Via HTTP Header:**
```http
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

---

## Beispiele

### Beispiel 1: Claude Desktop Integration

**MCP Configuration (`claude_desktop_config.json`):**
```json
{
  "mcpServers": {
    "themisdb": {
      "command": "themis-mcp-server",
      "args": [
        "--database", "mydb",
        "--host", "localhost:7687"
      ],
      "env": {
        "THEMIS_TOKEN": "${THEMIS_TOKEN}"
      }
    }
  }
}
```

**Claude Conversation:**
```
User: "Can you show me all users older than 25?"

Claude: Let me query the database for you.
[Uses 'query' tool with AQL]

Result: Found 3 users:
- John Doe (30 years old)
- Alice Smith (28 years old)
- Bob Johnson (35 years old)
```

### Beispiel 2: Custom Python MCP Client

```python
import json
import asyncio
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

async def main():
    server_params = StdioServerParameters(
        command="themis-mcp-server",
        args=["--database", "mydb"],
        env={"THEMIS_TOKEN": "your-jwt-token"}
    )
    
    async with stdio_client(server_params) as (read, write):
        async with ClientSession(read, write) as session:
            # Initialize
            await session.initialize()
            
            # List available tools
            tools = await session.list_tools()
            print("Available tools:", [t.name for t in tools])
            
            # Execute query
            result = await session.call_tool("query", {
                "query": "FOR doc IN users FILTER doc.age > 25 RETURN doc",
                "language": "aql"
            })
            
            print("Query result:", result.content[0].text)
            
            # Get schema
            schema = await session.read_resource("themisdb://mydb/schema")
            print("Schema:", schema.contents[0].text)

if __name__ == "__main__":
    asyncio.run(main())
```

### Beispiel 3: Node.js MCP Client

```javascript
const { Client } = require('@modelcontextprotocol/sdk/client/index.js');
const { StdioClientTransport } = require('@modelcontextprotocol/sdk/client/stdio.js');

async function main() {
  const transport = new StdioClientTransport({
    command: 'themis-mcp-server',
    args: ['--database', 'mydb'],
    env: {
      THEMIS_TOKEN: process.env.THEMIS_TOKEN
    }
  });
  
  const client = new Client({
    name: 'my-app',
    version: '1.0.0'
  }, {
    capabilities: {}
  });
  
  await client.connect(transport);
  
  // List tools
  const tools = await client.listTools();
  console.log('Available tools:', tools.tools.map(t => t.name));
  
  // Execute query
  const result = await client.callTool('query', {
    query: 'FOR doc IN users RETURN doc',
    language: 'aql'
  });
  
  console.log('Result:', result.content[0].text);
}

main();
```

### Beispiel 4: LangChain Integration

```python
from langchain.tools import Tool
from langchain.agents import initialize_agent
from langchain.llms import OpenAI
import requests

def themis_query(query: str) -> str:
    """Execute AQL query in ThemisDB via MCP"""
    response = requests.post(
        "https://your-themis-instance.com/mcp",
        headers={
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json"
        },
        json={
            "jsonrpc": "2.0",
            "method": "tools/call",
            "params": {
                "name": "query",
                "arguments": {
                    "query": query,
                    "language": "aql"
                }
            },
            "id": 1
        }
    )
    return response.json()["result"]["content"][0]["text"]

# Create LangChain tool
themis_tool = Tool(
    name="ThemisDB",
    func=themis_query,
    description="Execute queries in ThemisDB. Input should be an AQL query."
)

# Initialize agent
llm = OpenAI(temperature=0)
agent = initialize_agent(
    [themis_tool],
    llm,
    agent="zero-shot-react-description",
    verbose=True
)

# Use agent
result = agent.run("Find all users who are developers and older than 25")
print(result)
```

---

## Best Practices

### 1. Error Handling

```python
try:
    result = await session.call_tool("query", {
        "query": "INVALID SYNTAX",
        "language": "aql"
    })
except Exception as e:
    if hasattr(e, 'error'):
        print(f"MCP Error: {e.error['message']}")
        print(f"Code: {e.error['code']}")
    else:
        print(f"Unexpected error: {e}")
```

### 2. Resource Caching

```python
# Cache schema to avoid repeated requests
schema_cache = {}

async def get_schema(session, database):
    if database not in schema_cache:
        schema = await session.read_resource(f"themisdb://{database}/schema")
        schema_cache[database] = json.loads(schema.contents[0].text)
    return schema_cache[database]
```

### 3. Query Parameterization

```python
# Always use bind variables
result = await session.call_tool("query", {
    "query": "FOR doc IN users FILTER doc.email == @email RETURN doc",
    "bindVars": {"email": user_email},  # Prevents injection
    "language": "aql"
})
```

### 4. Batch Operations

```python
# Use batch operations for multiple inserts
entities = [
    {"name": "User 1", "email": "user1@example.com"},
    {"name": "User 2", "email": "user2@example.com"},
    {"name": "User 3", "email": "user3@example.com"}
]

result = await session.call_tool("batch_insert", {
    "collection": "users",
    "entities": entities
})
```

---

## Fehlerbehandlung

### MCP Error Codes

| Code | Bedeutung | Beschreibung |
|------|-----------|--------------|
| -32700 | Parse error | Ungültiges JSON |
| -32600 | Invalid Request | Request-Format ungültig |
| -32601 | Method not found | Methode existiert nicht |
| -32602 | Invalid params | Ungültige Parameter |
| -32603 | Internal error | Interner Server-Fehler |
| -32000 | Database error | Datenbank-spezifischer Fehler |

### Error Response Beispiel

```json
{
  "jsonrpc": "2.0",
  "error": {
    "code": -32000,
    "message": "Database error: Collection 'users' not found",
    "data": {
      "database": "mydb",
      "collection": "users",
      "error_type": "COLLECTION_NOT_FOUND"
    }
  },
  "id": 1
}
```

---

## Geplante Tool-Erweiterungen (Wave B/C)

> **Vollständiger Plan:** [MCP_TOOL_EXTENSION_PLAN.md](MCP_TOOL_EXTENSION_PLAN.md)

Die folgenden Tool-Gruppen sind geplant und werden auf `develop` implementiert. Aktuell **noch nicht** produktiv verfügbar.

### Gruppe 1: Knowledge Graph (Target: Q4 2026)

| Tool | Beschreibung |
|---|---|
| `kg_neighbours` | Nachbarn eines Knotens abrufen (Tiefe, Kantentypen konfigurierbar) |
| `kg_shortest_path` | Kürzester Pfad zwischen zwei Knoten |
| `kg_subgraph` | Teilgraph um Anker-Knoten extrahieren |
| `kg_node_properties` | Alle Eigenschaften eines Knotens |

### Gruppe 2: Vector Search & RAG (Target: Q4 2026)

| Tool | Beschreibung |
|---|---|
| `semantic_search` | Vektor-basiertes semantisches Retrieval (kNN) |
| `hybrid_search` | BM25 + Vektor kombiniert |
| `rag_retrieve` | Vollständige RAG-Pipeline (embed → search → rerank) |
| `vector_index_list` | Verfügbare Vektor-Indizes auflisten |

### Gruppe 3: Plugin & LLM Management (Target: Q1 2027)

| Tool | Beschreibung |
|---|---|
| `plugin_list` | Geladene Plugins mit Status |
| `plugin_load` | Plugin dynamisch laden (Admin-Scope) |
| `plugin_unload` | Plugin entladen (Admin-Scope) |
| `llm_model_list` | Verfügbare LLM-Modelle |
| `llm_model_status` | Status eines spezifischen Modells |

### Gruppe 4: Operations & Monitoring (Target: Q1 2027)

| Tool | Beschreibung |
|---|---|
| `health_check` | Server-Health inkl. Shard-Status |
| `metrics_snapshot` | Aktuelle Performance-Metriken |
| `shard_status` | Sharding-Topologie und Replikationsstatus |
| `compaction_trigger` | RocksDB Compaction auslösen (Admin-Scope) |
| `connection_pool_status` | Status des Connection Pools |

### Gruppe 5: Updates & Backup (Target: Q1 2027)

| Tool | Beschreibung |
|---|---|
| `update_list_pending` | Ausstehende Migrationen auflisten |
| `update_apply` | Migration anwenden (Admin-Scope) |
| `update_rollback` | Letzten Update zurückrollen (Admin-Scope) |
| `backup_create` | Snapshot/Backup initiieren |
| `backup_list` | Verfügbare Backups auflisten |
| `backup_restore` | Backup wiederherstellen (confirm_token erforderlich) |

### Gruppe 6: Security & Audit (Target: Q1–Q2 2027)

| Tool | Beschreibung |
|---|---|
| `audit_log_query` | Audit-Log abfragen (RBAC-gefiltert) |
| `permission_check` | Berechtigungs-Check für Ressource/Aktion |
| `token_validate` | Auth-Token validieren und Claims inspizieren |
| `security_scan_status` | Letzten Sicherheits-Scan-Status abrufen |

### Gruppe 7: Schema & Query-Plan (Target: Q4 2026)

| Tool | Beschreibung |
|---|---|
| `schema_diff` | Schema-Vergleich zwischen zwei Versionen |
| `schema_validate` | Schema gegen Constraints validieren |
| `explain_query` | Query-Execution-Plan ohne Ausführung |

---

## Siehe auch

- [MCP Tool Extension Plan](MCP_TOOL_EXTENSION_PLAN.md)
- [MCP Protocol Support](MCP_PROTOCOL_SUPPORT.md)
- [REST API Specification](REST_API_SPECIFICATION.md)
- [AQL Syntax Guide](../aql/AQL_SYNTAX_GUIDE.md)
- [LLM Integration Guide](../llm/LLM_INTEGRATION.md)
- [Anthropic MCP Specification](https://modelcontextprotocol.io)
