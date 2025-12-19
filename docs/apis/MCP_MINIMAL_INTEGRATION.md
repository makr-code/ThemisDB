# MCP Server Minimal Integration

## Overview

This document describes the minimal MCP (Model Context Protocol) integration implemented for ThemisDB. This integration provides basic LLM capabilities for database operations through a lightweight, production-ready implementation.

## Integration Level: Minimal (3-5 days)

### What's Implemented

#### 1. Stdio Transport (POSIX Only)
- **Real stdin/stdout communication** for Claude Desktop integration
- **Line-based JSON-RPC 2.0** message handling
- **Async I/O** using Boost.ASIO with select() for non-blocking stdin reads
- **Platform Detection** - POSIX systems only (Linux, macOS)
- **Graceful Handling** - Warns on Windows, no crash

**Implementation Details:**
```cpp
// Uses select() with 100ms timeout for responsive stdin reading
// Handles partial JSON messages across multiple lines
// Posts async task to io_context for non-blocking operation
```

#### 2. Core Tools with RocksDB Integration

**Three Essential Tools Connected to ThemisDB Storage:**

| Tool | Function | Integration Status |
|------|----------|-------------------|
| `put_entity` | Store key-value pairs | ✅ **Fully Integrated** with RocksDBWrapper |
| `get_entity` | Retrieve values by key | ✅ **Fully Integrated** with RocksDBWrapper |
| `delete_entity` | Delete keys | ✅ **Fully Integrated** with RocksDBWrapper |
| `query` | Execute queries | ⚠️ **Limited** - KV operations only, full Cypher/SQL pending |
| `get_schema` | Schema discovery | ⚠️ **Stub** - Requires query engine integration |
| `get_stats` | Database statistics | ⚠️ **Stub** - Returns connection status only |
| `create_index` | Create indexes | ⚠️ **Stub** - Requires query engine integration |

**Usage Examples:**

```javascript
// Store an entity
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "put_entity",
    "arguments": {
      "key": "user:1001",
      "value": {
        "name": "Alice",
        "email": "alice@example.com",
        "age": 30
      }
    }
  }
}

// Retrieve an entity
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "get_entity",
    "arguments": {
      "key": "user:1001"
    }
  }
}

// Delete an entity
{
  "jsonrpc": "2.0",
  "method": "tools/call",
  "params": {
    "name": "delete_entity",
    "arguments": {
      "key": "user:1001"
    }
  }
}
```

#### 3. Resources with Database Status

**Four Resources Providing Context:**

| Resource | URI | Content | Integration |
|----------|-----|---------|-------------|
| Schema | `schema://database` | Database schema | ⚠️ Minimal - Awaiting query engine |
| Stats | `stats://database` | Statistics | ✅ Connection status provided |
| Metadata | `metadata://database` | Server info | ✅ Full integration metadata |
| Examples | `examples://queries` | Query examples | ✅ Static examples |

**Enhanced Metadata Resource:**
```json
{
  "version": "1.0.0",
  "name": "ThemisDB",
  "integration_level": "minimal",
  "supported_operations": ["put_entity", "get_entity", "delete_entity"],
  "pending_operations": ["full_query", "schema_discovery", "advanced_stats"],
  "database_attached": true,
  "database_open": true
}
```

#### 4. Prompts (Stub)

Three prompts remain as stubs (no changes from base implementation):
- `simple_query` - Generate simple Cypher queries
- `complex_query` - Generate complex queries with filters  
- `entity_operation` - Entity operation prompts

These will be enhanced in production integration when query engine is fully connected.

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      MCP Server (Minimal)                    │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  stdio       │  │  SSE         │  │  WebSocket   │      │
│  │  Transport   │  │  Transport   │  │  Transport   │      │
│  │  (POSIX)     │  │  (Stub)      │  │  (Stub)      │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
│         └────────────┬─────────────────┬────┘               │
│                      │                 │                     │
│              ┌───────▼─────────────────▼─────────┐          │
│              │   JSON-RPC 2.0 Handler            │          │
│              │   (Request Router)                 │          │
│              └───────┬────────────────────────────┘          │
│                      │                                        │
│         ┌────────────┼────────────┬─────────────┐           │
│         │            │            │             │           │
│    ┌────▼────┐  ┌───▼────┐  ┌───▼──────┐ ┌───▼──────┐    │
│    │ Tools   │  │Resource│  │ Prompts  │ │Initialize│    │
│    │ Registry│  │Registry│  │ Registry │ │  Handler │    │
│    └────┬────┘  └───┬────┘  └───┬──────┘ └──────────┘    │
│         │           │            │                         │
└─────────┼───────────┼────────────┼─────────────────────────┘
          │           │            │
          │           │            │
┌─────────▼───────────▼────────────▼─────────────────────────┐
│                  RocksDBWrapper                              │
│                                                               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │   PUT    │  │   GET    │  │  DELETE  │  │  Batch   │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└───────────────────────────────────────────────────────────────┘
```

### API Integration

#### Attaching Database

```cpp
// In your server initialization code
auto mcp_server = std::make_shared<McpServer>(io_context);
auto rocks_db = std::make_shared<RocksDBWrapper>(db_config);

// Attach database for tool operations
mcp_server->attachDatabase(rocks_db);

// Optional: Attach HTTP server for SSE/WebSocket transports
mcp_server->attachHttpServer(http_server);

// Start MCP server
mcp_server->start();
```

### Testing

#### Unit Tests
The basic test foundation covers MCP functionality but requires RocksDB to be available:

```bash
# Build with MCP enabled
cmake -B build -S . -DTHEMIS_ENABLE_MCP=ON -DTHEMIS_BUILD_TESTS=ON
cmake --build build

# Run MCP tests (when test file is created)
./build/themis_tests --gtest_filter="MCPServerTest.*"
```

#### Manual Testing with Claude Desktop

1. **Configure Claude Desktop:**

```json
{
  "mcpServers": {
    "themisdb": {
      "command": "/path/to/themis_server",
      "args": ["--mcp-stdio"],
      "env": {
        "THEMIS_DB_PATH": "/path/to/database"
      }
    }
  }
}
```

2. **Test Entity Operations:**

- Ask Claude: "Store a user entity with key 'user:alice' containing name, email, and age"
- Ask Claude: "Retrieve the user entity with key 'user:alice'"
- Ask Claude: "Delete the user entity with key 'user:alice'"

### Limitations & Future Work

#### Current Limitations

1. **Stdio Transport:**
   - ✅ POSIX systems (Linux, macOS)
   - ❌ Windows (warning logged, no crash)
   - Future: Windows implementation using named pipes or async console API

2. **Query Support:**
   - ✅ Key-value operations (put, get, delete)
   - ❌ Full Cypher query execution
   - ❌ SQL query execution (via PostgreSQL Wire protocol)
   - Future: Integration with query engine for full query support

3. **Schema Discovery:**
   - ❌ Automatic schema detection
   - ❌ Node/Edge type enumeration
   - Future: Integration with metadata store for schema introspection

4. **Statistics:**
   - ✅ Connection status
   - ❌ Node/Edge counts
   - ❌ Storage size metrics
   - Future: RocksDB statistics API integration

5. **Transports:**
   - ✅ stdio (POSIX only)
   - ❌ SSE (stub)
   - ❌ WebSocket (stub)
   - Future: HTTP endpoint integration for SSE/WebSocket

#### Roadmap to Production Integration

**Phase 1: Minimal Integration** ✅ (Current - 3-5 days)
- [x] POSIX stdio transport
- [x] RocksDB tool integration (put/get/delete)
- [x] Basic resource handlers
- [x] Connection status reporting

**Phase 2: Enhanced Integration** (1-2 weeks)
- [ ] Query engine integration for full Cypher/SQL support
- [ ] Schema discovery via metadata store
- [ ] Comprehensive statistics from RocksDB
- [ ] Index management integration
- [ ] Windows stdio support

**Phase 3: Production Integration** (2-3 weeks)
- [ ] SSE transport with HTTP server integration
- [ ] WebSocket transport with session management
- [ ] Advanced prompt engineering
- [ ] Transaction support
- [ ] Performance optimizations
- [ ] Comprehensive testing suite

**Phase 4: Advanced Features** (Future)
- [ ] Streaming query results
- [ ] Incremental schema updates
- [ ] Query plan visualization
- [ ] Cost-based query optimization hints
- [ ] Multi-database support

### Error Handling

All tools return consistent error responses:

```json
{
  "status": "error",
  "message": "Descriptive error message",
  "key": "context_information"
}
```

Common error scenarios:
- **Database not attached**: Tool operations fail gracefully with clear message
- **Database not open**: Operations return connection error
- **JSON parse error**: Invalid value format for put operations
- **Key not found**: Get operations return null value with success status

### Security Considerations

1. **Input Validation:**
   - All JSON inputs are validated before processing
   - Keys are sanitized (no special characters that could break storage)
   - Values must be valid JSON objects

2. **Access Control:**
   - MCP server should be restricted to trusted LLM clients only
   - stdio transport inherits process permissions (use with care)
   - Consider authentication for HTTP-based transports (future)

3. **Resource Limits:**
   - No rate limiting in minimal integration (future enhancement)
   - Value size limited by RocksDB configuration
   - Consider implementing max_value_size check

### Performance

**Expected Performance (Minimal Integration):**
- **Put operations:** ~1-10ms (RocksDB write latency)
- **Get operations:** ~0.1-1ms (RocksDB read latency, block cache hit)
- **Delete operations:** ~1-10ms (RocksDB delete latency)
- **JSON serialization:** ~0.1-1ms (nlohmann/json)
- **Stdio transport:** ~1-5ms overhead (select() + parsing)

**Scalability:**
- Single-threaded stdio transport (one LLM client at a time)
- RocksDB concurrent reads supported
- No connection pooling in minimal integration

### Comparison with Base Implementation

| Feature | Base Implementation | Minimal Integration |
|---------|-------------------|-------------------|
| stdio Transport | Stub (no I/O) | ✅ POSIX implementation |
| SSE Transport | Stub | Stub (unchanged) |
| WebSocket Transport | Stub | Stub (unchanged) |
| put_entity | Stub | ✅ RocksDB integrated |
| get_entity | Stub | ✅ RocksDB integrated |
| delete_entity | Stub | ✅ RocksDB integrated |
| query | Stub | Limited (KV only) |
| get_schema | Stub | Enhanced stub with metadata |
| get_stats | Stub | Connection status |
| create_index | Stub | Enhanced stub |
| Resources | Static | Dynamic with DB status |

### Conclusion

The minimal MCP integration provides a **production-ready foundation** for LLM interaction with ThemisDB through basic key-value operations. While full query support requires additional integration with the query engine, the current implementation enables:

1. **Entity management** - Create, read, delete entities via natural language
2. **Database connectivity** - LLMs can verify database status  
3. **Extensibility** - Clear path to full integration
4. **Robustness** - Graceful error handling and platform detection

This serves as a **solid baseline** for LLM-powered database applications while maintaining the architecture for future enhancements.

## References

- [MCP Protocol Specification](MCP_PROTOCOL_SUPPORT.md)
- [RocksDB Wrapper Documentation](../../include/storage/rocksdb_wrapper.h)
- [Model Context Protocol](https://modelcontextprotocol.io)
- [Claude Desktop Integration](https://claude.ai/desktop)
