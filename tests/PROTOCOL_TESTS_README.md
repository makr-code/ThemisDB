> **Build + Test:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release && ctest --preset linux-ninja-release`

# Network Protocol Test Suite

This directory contains basic smoke tests for ThemisDB's network protocols.

## Test Files

### Protocol Tests
- **test_http2_protocol.cpp** - HTTP/2 ALPN negotiation, stream handling, configuration tests
- **test_websocket_cdc.cpp** - WebSocket CDC/Changefeed subscription, filtering, message format tests
- **test_mqtt_protocol.cpp** - MQTT packet types, QoS levels, topic wildcards, metrics tests
- **test_postgres_wire.cpp** - PostgreSQL wire protocol, SQL-to-Cypher translation, schema introspection tests

## Running Tests

### Build with Protocol Support
```bash
# Build with all protocols enabled
cmake -B build -S . \
  -DTHEMIS_ENABLE_HTTP2=ON \
  -DTHEMIS_ENABLE_WEBSOCKET=ON \
  -DTHEMIS_ENABLE_MQTT=ON \
  -DTHEMIS_ENABLE_POSTGRES_WIRE=ON \
  -DTHEMIS_BUILD_TESTS=ON

cmake --build build -j8
```

### Run Tests
```bash
# Run all tests
cd build && ctest

# Run specific protocol tests
./build/themis_tests --gtest_filter="HTTP2ProtocolTest.*"
./build/themis_tests --gtest_filter="WebSocketCDCTest.*"
./build/themis_tests --gtest_filter="MQTTProtocolTest.*"
./build/themis_tests --gtest_filter="PostgresWireTest.*"
```

## Test Coverage

### HTTP/2 Tests
- ✅ ALPN negotiation (h2, http/1.1)
- ✅ Stream ID validation (client=odd, server=even)
- ✅ Max concurrent streams configuration
- ✅ HPACK header compression
- ✅ Supported HTTP methods (GET, POST, PUT, DELETE, etc.)
- ✅ Configuration defaults (security-first, OFF by default)

### WebSocket CDC Tests
- ✅ Subscribe/unsubscribe message format
- ✅ CDC event message structure
- ✅ Key prefix filtering
- ✅ Sequence-based filtering
- ✅ Text and binary message support
- ✅ Ping/pong keepalive mechanism
- ✅ Configuration defaults

### MQTT Tests
- ✅ MQTT 3.1.1 packet types (CONNECT, PUBLISH, SUBSCRIBE, etc.)
- ✅ QoS levels (0, 1, 2) with four-way handshake validation
- ✅ Topic wildcards (single-level +, multi-level #)
- ✅ Shared subscriptions ($share/shareName/topic)
- ✅ Rate limiting configuration
- ✅ Metrics structure (messages, bytes, QoS counters)
- ✅ MQTT 5.0 properties (Content-Type, Response-Topic, User Properties)

### PostgreSQL Wire Tests
- ✅ Protocol message types (Query, Parse, Bind, Execute, etc.)
- ✅ SQL-to-Cypher translation (SELECT, WHERE, ORDER BY, LIMIT)
- ✅ Aggregate functions (COUNT, SUM, AVG, MIN, MAX, GROUP BY)
- ✅ pg_catalog schema introspection
- ✅ INFORMATION_SCHEMA queries
- ✅ PostgreSQL function emulation (version(), current_database())
- ✅ Transaction stubs (BEGIN, COMMIT, ROLLBACK)
- ✅ BI tool compatibility (Tableau, Power BI, Metabase)

## Test Philosophy

These tests follow a **basic foundation** approach:
1. **Smoke tests** - Verify basic functionality works
2. **Configuration tests** - Validate default settings and security
3. **Format tests** - Check message/packet structures
4. **Example tests** - Provide patterns for expansion

## Expanding Tests

Each test file is designed to be easily expandable:

```cpp
// Add new tests by following the existing pattern:
TEST(ProtocolNameTest, NewFeatureTest) {
    // Arrange - set up test data
    
    // Act - perform operation
    
    // Assert - verify results
    EXPECT_EQ(actual, expected);
}
```

## CI/CD Integration

Tests are automatically run in CI when:
- Pull requests are created/updated
- Commits are pushed to main branch
- Protocol build flags are enabled

## Future Enhancements

- [ ] Integration tests with actual protocol connections
- [ ] Load testing and stress tests
- [ ] Edge case coverage
- [ ] Performance benchmarks
- [ ] Protocol interoperability tests
- [ ] Security vulnerability tests

## Dependencies

- Google Test (gtest) - Test framework
- nlohmann/json - JSON parsing (for WebSocket tests)
- Protocol implementations must be built with corresponding flags

## Notes

- Tests will be skipped (GTEST_SKIP) if the protocol is not enabled at build time
- Each protocol has security-first defaults (OFF by default)
- Tests validate both functionality AND security configuration
