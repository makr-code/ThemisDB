# Testing Instructions for MCP Complete Integration

## Prerequisites

1. **Build with MCP Enabled:**
   ```bash
   # For Community Edition with MCP
   cmake --preset community-debug \
     -DTHEMIS_ENABLE_MCP=ON \
     -DTHEMIS_BUILD_TESTS=ON
   
   cmake --build build-community-debug
   ```

2. **Run MCP Tests:**
   ```bash
   cd build-community-debug
   
   # Run all MCP tests
   ctest -R mcp -V
   
   # Or run specific test files
   ./tests/test_mcp_protocol
   ./tests/test_mcp_integration
   ```

## Manual Testing with Claude Desktop

### 1. Configure Claude Desktop

Edit your Claude Desktop configuration (`~/Library/Application Support/Claude/claude_desktop_config.json` on macOS):

```json
{
  "mcpServers": {
    "themisdb": {
      "command": "/path/to/ThemisDB/build-community-debug/themisdb-server",
      "args": ["--mcp-stdio", "--db-path", "/tmp/test_themis_db"],
      "env": {
        "THEMIS_ENABLE_MCP": "ON"
      }
    }
  }
}
```

### 2. Test Entity Operations

Ask Claude:
- "Store a user entity with key 'user:alice' containing name 'Alice', email 'alice@example.com', and age 30"
- "Retrieve the user entity with key 'user:alice'"
- "Delete the user entity with key 'user:alice'"

### 3. Test Index Management

Ask Claude:
- "Create a regular unique index on the users table for the email column"
- "Create a fulltext index on the articles table for the content column with English language support"
- "List all indexes in the database"
- "Drop the index on users.email"

### 4. Test Schema Discovery

Ask Claude:
- "Show me the database schema"
- "What tables exist in the database?"
- "What properties does the users table have?"

### 5. Test Statistics

Ask Claude:
- "Get database statistics"
- "How many tables are there?"
- "What is the database version?"

## Expected Results

### Index Creation Success
```json
{
  "status": "success",
  "message": "Index created successfully on users.email",
  "table": "users",
  "column": "email",
  "index_type": "regular",
  "unique": true
}
```

### Index Listing Success
```json
{
  "status": "success",
  "indexes": [
    {
      "table": "users",
      "column": "email",
      "type": "regular",
      "unique": true,
      "estimated_size_bytes": 1024,
      "entry_count": 0
    }
  ],
  "total_count": 1
}
```

### Schema Discovery Success
```json
{
  "integration_level": "full",
  "nodes": [...],
  "edges": [...],
  "total_tables": 5
}
```

## Testing SSE Transport (Optional)

### 1. Start ThemisDB Server with SSE
```bash
./themisdb-server \
  --enable-mcp \
  --enable-sse \
  --http-port 8080 \
  --db-path /tmp/test_db
```

### 2. Test SSE Endpoints
```bash
# Open SSE stream (in terminal 1)
curl -N http://localhost:8080/mcp/sse/stream

# Send request (in terminal 2)
curl -X POST http://localhost:8080/mcp/sse/request \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "tools/list",
    "id": 1
  }'
```

## Testing WebSocket Transport (Optional)

### 1. Start ThemisDB Server with WebSocket
```bash
./themisdb-server \
  --enable-mcp \
  --enable-websocket \
  --ws-port 8081 \
  --db-path /tmp/test_db
```

### 2. Test WebSocket Connection
```javascript
// In browser console or Node.js
const ws = new WebSocket('ws://localhost:8081/mcp/ws');

ws.onopen = () => {
  ws.send(JSON.stringify({
    jsonrpc: "2.0",
    method: "initialize",
    params: {
      protocolVersion: "2024-11-05",
      clientInfo: { name: "test-client", version: "1.0.0" }
    },
    id: 1
  }));
};

ws.onmessage = (event) => {
  console.log('Received:', event.data);
};
```

## Troubleshooting

### Build Issues
- **Error: "THEMIS_ENABLE_MCP not defined"**
  - Ensure `-DTHEMIS_ENABLE_MCP=ON` is passed to cmake
  
- **Error: "SchemaManager not found"**
  - Ensure you're building Community/Enterprise edition, not Minimal
  
### Runtime Issues
- **Error: "Database not attached"**
  - Check that RocksDB path is valid
  - Ensure database is opened before MCP server starts

- **Error: "Index manager not initialized"**
  - Verify SchemaManager is properly attached to database
  - Check that database is open when attachDatabase() is called

### Test Failures
- **Test: "CreateRegularIndex" fails**
  - Check that SecondaryIndexManager is available
  - Verify RocksDB is writable
  
- **Test: "GetSchema" returns empty**
  - Ensure test database has some entities
  - Check SchemaManager cache timeout

## Performance Benchmarks

Run performance tests to validate:
```bash
# Index creation benchmark
./benchmarks/bench_index_creation

# Schema discovery benchmark  
./benchmarks/bench_schema_discovery

# MCP request throughput
./benchmarks/bench_mcp_throughput
```

Expected results:
- Index creation: <1s for tables with <10k rows
- Schema discovery: <100ms (cached)
- MCP request throughput: >100 req/sec (stdio), >1000 req/sec (HTTP)

## Security Validation

Run security checks:
```bash
# CodeQL analysis (already passed)
codeql database analyze ...

# Valgrind for memory leaks
valgrind --leak-check=full ./tests/test_mcp_integration

# AddressSanitizer for memory safety
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ...
./tests/test_mcp_integration
```

## Success Criteria

✅ All 22 integration tests pass  
✅ Manual testing with Claude Desktop works  
✅ No memory leaks detected  
✅ No security vulnerabilities found  
✅ Performance benchmarks meet expectations  
✅ Documentation is clear and complete  

## Support

If you encounter issues:
1. Check logs for error messages
2. Verify configuration is correct
3. Ensure all dependencies are installed
4. Review documentation in `docs/de/apis/`
5. Check test examples in `tests/test_mcp_integration.cpp`
