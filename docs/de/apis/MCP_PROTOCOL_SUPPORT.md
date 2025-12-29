# 🔗 Model Context Protocol (MCP) Support

> **Kategorie:** Enterprise Feature  
> **Seit Version:** 1.3.0  
> **Status:** ✅ Stable  
> **Aktualisiert:** 22. Dezember 2025

---

## 📋 Inhaltsverzeichnis

- [🎯 Übersicht](#-übersicht)
- [📊 MCP Architektur](#-mcp-architektur)
- [🚀 Erste Schritte](#-erste-schritte)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)

---

## 🎯 Übersicht

Das **Model Context Protocol (MCP)** ist ein offenes Protokoll, das eine nahtlose Integration zwischen LLM-Anwendungen und externen Datenquellen ermöglicht. ThemisDB implementiert MCP, um KI-gesteuerte Datenbankinteraktionen bereitzustellen.

## What is MCP?

MCP is a standardized protocol developed by Anthropic that allows:
- **Context Sharing**: Applications can expose data and context to LLMs
- **Tool Integration**: LLMs can invoke database operations as tools
- **Bidirectional Communication**: Real-time interaction between AI and database
- **Standardization**: Universal protocol for LLM-database integration

### MCP vs SSE (Server-Sent Events)

| Feature | MCP | SSE |
|---------|-----|-----|
| **Purpose** | LLM-database integration protocol | Server-to-client real-time updates |
| **Direction** | Bidirectional (request/response) | Unidirectional (server → client) |
| **Use Case** | AI queries, tool calling, context | CDC, notifications, live updates |
| **Protocol** | JSON-RPC over stdio/HTTP/WebSocket | HTTP with text/event-stream |
| **Complexity** | Higher (tool definitions, schemas) | Lower (simple event streaming) |
| **Target** | LLM applications (Claude, GPT, etc.) | Web browsers, dashboards |

**Both protocols are complementary:**
- **MCP**: For AI-powered interactions (natural language → database operations)
- **SSE**: For real-time data updates (database changes → UI)

## Architecture

```
┌─────────────────┐
│   LLM Client    │
│ (Claude/GPT/etc)│
└────────┬────────┘
         │ MCP Protocol
         │ (JSON-RPC)
         ▼
┌─────────────────┐
│  MCP Server     │
│  (ThemisDB)     │
├─────────────────┤
│ • Tools         │ ← Query, PutEntity, GetEntity, etc.
│ • Resources     │ ← Schema, Stats, Metadata
│ • Prompts       │ ← Query templates
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   ThemisDB      │
│   Core Engine   │
└─────────────────┘
```

## Features

### 1. **Tools** - Database Operations as LLM Tools

MCP exposes ThemisDB operations as callable tools:

**Available Tools:**
- `query`: Execute Cypher/SQL queries with natural language
- `put_entity`: Create or update entities
- `get_entity`: Retrieve entities by ID
- `delete_entity`: Delete entities
- `create_index`: Create database indexes
- `get_schema`: Retrieve database schema
- `get_stats`: Get database statistics

**Example Tool Call:**
```json
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "query",
    "arguments": {
      "query": "MATCH (u:User) WHERE u.age > 25 RETURN u.name",
      "language": "cypher"
    }
  }
}
```

### 2. **Resources** - Database Context for LLMs

Resources provide read-only context to LLMs:

**Available Resources:**
- `schema://database`: Database schema (nodes, edges, properties)
- `stats://database`: Performance statistics
- `metadata://database`: Database metadata
- `examples://queries`: Example query patterns

**Example Resource Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "resources/read",
  "params": {
    "uri": "schema://database"
  }
}
```

### 3. **Prompts** - Query Templates

Pre-defined prompts for common operations:

- `analyze_data`: Analyze dataset and provide insights
- `optimize_query`: Suggest query optimizations
- `schema_design`: Help design database schema
- `migration_plan`: Create migration strategies

## Implementation Details

### Transport Layers

MCP supports multiple transport mechanisms:

1. **stdio** (Standard Input/Output)
   - Best for local CLI tools
   - Direct process communication
   
2. **HTTP/SSE** (Server-Sent Events)
   - Best for web applications
   - Reuses existing HTTP infrastructure
   
3. **WebSocket**
   - Best for real-time bidirectional communication
   - Low latency

### ThemisDB MCP Server

```cpp
// MCP Server Handler (Pseudocode)
class McpServer {
public:
    // Initialize MCP server
    void initialize(const Config& config);
    
    // Handle MCP requests
    json handleRequest(const json& request);
    
    // Tool handlers
    json executeTool(const string& toolName, const json& args);
    
    // Resource handlers
    json readResource(const string& uri);
    
    // Prompt handlers
    json getPrompt(const string& promptName, const json& args);
};
```

## Configuration

Add MCP configuration to `themis.json`:

```json
{
  "enable_mcp": true,
  "mcp_transport": "websocket",
  "mcp_port": 8085,
  "mcp_max_context_size": 1000000,
  "mcp_enable_schema_context": true,
  "mcp_enable_stats_context": true,
  "mcp_tools": [
    "query",
    "put_entity",
    "get_entity",
    "delete_entity",
    "get_schema"
  ]
}
```

## Build Configuration

MCP support is **disabled by default** (opt-in for security):

```bash
# Build with MCP support
cmake -B build -S . -DTHEMIS_ENABLE_MCP=ON
cmake --build build -j8
```

## Usage Examples

### Example 1: Python Client with MCP

```python
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

async def main():
    server_params = StdioServerParameters(
        command="themisdb-mcp-server",
        args=["--config", "themis.json"]
    )
    
    async with stdio_client(server_params) as (read, write):
        async with ClientSession(read, write) as session:
            await session.initialize()
            
            # Execute natural language query
            result = await session.call_tool(
                "query",
                {
                    "query": "Find all users who signed up last week",
                    "language": "natural"
                }
            )
            print(result)
```

### Example 2: Claude Desktop Integration

```json
{
  "mcpServers": {
    "themisdb": {
      "command": "themisdb-mcp-server",
      "args": ["--config", "/path/to/themis.json"],
      "env": {
        "THEMIS_API_KEY": "your-api-key"
      }
    }
  }
}
```

### Example 3: WebSocket MCP Client (JavaScript)

```javascript
const ws = new WebSocket('ws://localhost:8085/mcp');

ws.onopen = () => {
  // Initialize MCP session
  ws.send(JSON.stringify({
    jsonrpc: '2.0',
    method: 'initialize',
    params: {
      protocolVersion: '2024-11-05',
      capabilities: {
        tools: {},
        resources: {}
      },
      clientInfo: {
        name: 'ThemisDB Web Client',
        version: '1.0.0'
      }
    },
    id: 1
  }));
};

ws.onmessage = (event) => {
  const response = JSON.parse(event.data);
  console.log('MCP Response:', response);
};

// Call a tool
function queryDatabase(naturalLanguageQuery) {
  ws.send(JSON.stringify({
    jsonrpc: '2.0',
    method: 'tools/call',
    params: {
      name: 'query',
      arguments: {
        query: naturalLanguageQuery,
        language: 'natural'
      }
    },
    id: 2
  }));
}

queryDatabase('Show me all active users');
```

## Security Considerations

### 1. **Authentication**
- API key required for MCP connections
- JWT token support for session management
- OAuth2 integration for enterprise deployments

### 2. **Authorization**
- Role-based access control (RBAC)
- Tool-level permissions
- Resource-level permissions
- Query complexity limits

### 3. **Rate Limiting**
- Per-client rate limits
- Tool invocation limits
- Context size limits

### 4. **Sandboxing**
- Query execution timeouts
- Memory limits
- Restricted operations

## Performance

### Benchmarks

| Operation | Latency (p50) | Latency (p99) | Throughput |
|-----------|---------------|---------------|------------|
| Tool Call (query) | 15ms | 50ms | 2000 req/s |
| Resource Read | 5ms | 20ms | 5000 req/s |
| Prompt Generation | 10ms | 30ms | 3000 req/s |

### Optimization Tips

1. **Connection Pooling**: Reuse MCP sessions
2. **Context Caching**: Cache schema and stats
3. **Batch Operations**: Combine multiple tool calls
4. **Streaming**: Use streaming for large results

## Integration with Other Protocols

### MCP + SSE
```javascript
// Subscribe to CDC via SSE
const sse = new EventSource('/cdc/stream');
sse.onmessage = (event) => {
  const change = JSON.parse(event.data);
  
  // Use MCP to analyze the change
  mcpClient.callTool('analyze_change', { change });
};
```

### MCP + WebSocket
```javascript
// Real-time MCP over WebSocket
const ws = new WebSocket('ws://localhost:8085/mcp');

// Bidirectional communication
ws.send(JSON.stringify({
  method: 'tools/call',
  params: { name: 'query', arguments: { query: 'MATCH (n) RETURN count(n)' }}
}));
```

### MCP + HTTP/2
```bash
# HTTP/2 Server Push for proactive MCP updates
curl --http2 https://localhost:8443/mcp/tools/call \
  -H "Content-Type: application/json" \
  -d '{"name": "query", "arguments": {"query": "MATCH (n) RETURN n LIMIT 10"}}'
```

## Comparison: MCP vs Direct API

| Aspect | MCP | Direct REST API |
|--------|-----|-----------------|
| **Learning Curve** | Low (natural language) | High (learn syntax) |
| **Flexibility** | High (AI interprets) | Medium (fixed endpoints) |
| **Context Awareness** | High (schema, stats) | Low (manual context) |
| **Error Handling** | AI-assisted recovery | Manual error handling |
| **Use Case** | AI-powered apps | Traditional apps |

## Office Plugins Integration

See [MCP_OFFICE_PLUGINS.md](./MCP_OFFICE_PLUGINS.md) for detailed information about using MCP with Microsoft Office plugins (Word, Excel, Outlook).

## Troubleshooting

### Common Issues

**1. Connection Refused**
```bash
# Check if MCP server is running
curl http://localhost:8085/mcp/health

# Verify configuration
cat themis.json | grep mcp
```

**2. Tool Not Found**
```json
// Ensure tool is enabled in config
{
  "mcp_tools": ["query", "put_entity", "get_entity"]
}
```

**3. Context Too Large**
```json
// Reduce context size
{
  "mcp_max_context_size": 500000
}
```

## Future Enhancements

- [ ] Natural Language to Cypher translation
- [ ] AI-powered query optimization
- [ ] Automated schema migrations
- [ ] Intelligent caching strategies
- [ ] Anomaly detection
- [ ] Predictive analytics
- [ ] Multi-modal support (images, documents)

## Resources

- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [Anthropic MCP SDK](https://github.com/anthropics/anthropic-sdk-python)
- [MCP Client Examples](https://modelcontextprotocol.io/examples)
- [ThemisDB MCP Server](https://github.com/makr-code/ThemisDB/tree/main/src/mcp)

## See Also

- [HTTP/2 and HTTP/3 Support](./HTTP2_HTTP3_PROTOCOL_SUPPORT.md)
- [WebSocket Support](./ADDITIONAL_PROTOCOLS.md#websocket)
- [SSE Support](./ADDITIONAL_PROTOCOLS.md#sse)
- [Protocol Build Switches](./PROTOCOL_BUILD_SWITCHES.md)
