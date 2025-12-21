# ThemisDB Client Tests and Benchmarks

This document describes the comprehensive test suites and benchmarks for ThemisDB REST API and wire protocol clients.

## Overview

We have created extensive test suites for:
- **HTTP/REST API layer** - Testing all REST endpoints (CRUD, Query, Graph, Vector operations)
- **Wire Protocol layer** - Testing native binary protocol (serialization, opcodes, performance)

## Go Client

### Location
- Tests: `/clients/go/`
- REST API Tests: `client_rest_test.go`
- Wire Protocol Tests: `wire_protocol_test.go`
- REST API Benchmarks: `client_bench_test.go`
- Wire Protocol Benchmarks: `wire_protocol_bench_test.go`

### Running Tests

```bash
cd clients/go

# Run all tests
go test -v

# Run specific test suites
go test -v -run TestClient_CRUD
go test -v -run TestWireFrame

# Run with coverage
go test -v -cover
go test -v -coverprofile=coverage.out
go tool cover -html=coverage.out
```

### Running Benchmarks

```bash
cd clients/go

# Run all benchmarks
go test -bench=. -benchmem

# Run specific benchmarks
go test -bench=BenchmarkClient_Get -benchmem
go test -bench=BenchmarkWireFrame -benchmem

# Run benchmarks with custom time
go test -bench=. -benchtime=5s -benchmem

# Compare benchmark results
go test -bench=. -benchmem > old.txt
# Make changes
go test -bench=. -benchmem > new.txt
go install golang.org/x/perf/cmd/benchstat@latest
benchstat old.txt new.txt
```

### Test Coverage

**HTTP/REST API Tests:**
- ✅ CRUD operations (Get, Put, Delete)
- ✅ Query execution (AQL)
- ✅ Graph operations (Traverse, ShortestPath, Neighbors)
- ✅ Vector operations (Search, Upsert, Delete)
- ✅ Transaction lifecycle (Begin, Commit, Rollback)
- ✅ Error handling (Network errors, HTTP errors, timeouts)
- ✅ Context cancellation
- ✅ Endpoint selection

**Wire Protocol Tests:**
- ✅ Frame serialization/deserialization
- ✅ All operation codes (Hello, Auth, Get, Put, Delete, Query, Vector, etc.)
- ✅ Payload size handling (0 bytes to 1MB+)
- ✅ Sequence number generation
- ✅ Concurrent access
- ✅ Round-trip encoding

**Benchmarks:**
- 📊 HTTP operations (Get, Put, Delete, Query)
- 📊 Graph operations (Traverse, ShortestPath, Neighbors)
- 📊 Vector operations (Search, Upsert)
- 📊 Transaction operations
- 📊 Parallel requests
- 📊 Large payloads
- 📊 Wire frame serialization/deserialization
- 📊 Different payload sizes
- 📊 Different opcodes
- 📊 Sequence generation

### Benchmark Results (Sample)

```
BenchmarkClient_Get-4                     8694    135688 ns/op    8878 B/op    111 allocs/op
BenchmarkClient_Put-4                     9283    128920 ns/op    7505 B/op     94 allocs/op
BenchmarkClient_Query-4                   7240    157767 ns/op   14252 B/op    214 allocs/op
BenchmarkWireFrame_ToBytes-4           4674678       256 ns/op     128 B/op      8 allocs/op
BenchmarkWireFrame_FromBytes-4         3983923       305 ns/op     160 B/op      9 allocs/op
BenchmarkWireClient_SequenceGeneration  198M         6.1 ns/op       0 B/op      0 allocs/op
```

## Python Client

### Location
- Tests: `/clients/python/tests/`
- REST API Tests: `test_rest_api.py`
- Benchmarks: `test_benchmarks.py`
- Transaction Tests: `test_transaction.py`
- Topology Tests: `test_topology.py`

### Running Tests

```bash
cd clients/python

# Install dependencies
pip install -e ".[dev]"
pip install pytest pytest-asyncio pytest-benchmark httpx

# Run all tests
pytest tests/ -v

# Run specific test suites
pytest tests/test_rest_api.py -v
pytest tests/test_transaction.py -v

# Run with coverage
pytest tests/ --cov=themis --cov-report=html
```

### Running Benchmarks

```bash
cd clients/python

# Install benchmark plugin
pip install pytest-benchmark

# Run all benchmarks
pytest tests/test_benchmarks.py --benchmark-only -v

# Run specific benchmarks
pytest tests/test_benchmarks.py::test_benchmark_get_operation --benchmark-only

# Save benchmark results
pytest tests/test_benchmarks.py --benchmark-only --benchmark-save=baseline

# Compare benchmarks
pytest tests/test_benchmarks.py --benchmark-only --benchmark-compare=baseline
```

### Test Coverage

**HTTP/REST API Tests:**
- ✅ Client creation and configuration
- ✅ CRUD operations (Get, Put, Delete)
- ✅ Query execution
- ✅ Graph operations (optional, if implemented)
- ✅ Vector operations (optional, if implemented)
- ✅ Transaction support
- ✅ Error handling (network, HTTP status codes)
- ✅ Async client operations
- ✅ Multiple endpoint configuration
- ✅ Timeout handling

**Benchmarks:**
- 📊 GET, PUT, DELETE operations
- 📊 Query processing
- 📊 Graph traversal (if available)
- 📊 Vector search (if available)
- 📊 Transaction lifecycle
- 📊 Data serialization/deserialization
- 📊 Different data sizes
- 📊 Different vector dimensions
- 📊 Sequential vs parallel operations

## Ruby Client

### Location
- Client: `/clients/ruby/lib/themisdb.rb`

### Status
⚠️ **Tests need to be created** - Ruby client currently has no test suite.

### Recommended Setup

```bash
cd clients/ruby

# Create test directory
mkdir -p test

# Install test dependencies
gem install minitest minitest-reporters webmock

# Create test files
# - test/test_rest_api.rb - REST API tests
# - test/test_benchmarks.rb - Benchmark tests
```

## Best Practices

### Writing Tests

1. **Use Mock Servers**: Use httptest (Go) or similar libraries to avoid external dependencies
2. **Test Error Cases**: Include tests for network failures, timeouts, and invalid responses
3. **Test Edge Cases**: Empty payloads, large payloads, concurrent access
4. **Separate Integration Tests**: Mark integration tests that require a running server
5. **Use Table-Driven Tests**: For testing multiple scenarios efficiently

### Writing Benchmarks

1. **Warm-up**: Benchmarks should run multiple iterations
2. **Isolate**: Each benchmark should test one thing
3. **Representative Data**: Use realistic data sizes and patterns
4. **Memory Profiling**: Include memory allocations in benchmarks
5. **Compare**: Keep baseline results for comparison

### CI/CD Integration

```yaml
# Example GitHub Actions workflow
- name: Run Go tests
  run: |
    cd clients/go
    go test -v -race -coverprofile=coverage.txt

- name: Run Python tests
  run: |
    cd clients/python
    pip install -e ".[dev]"
    pytest tests/ --cov=themis --cov-report=xml

- name: Upload coverage
  uses: codecov/codecov-action@v3
```

## Performance Targets

Based on benchmark results, we aim for:

- **HTTP GET**: < 200ms per operation
- **HTTP PUT**: < 200ms per operation  
- **Query**: < 500ms for simple queries
- **Wire Frame Serialization**: < 1μs for small payloads
- **Sequence Generation**: < 10ns per sequence number
- **Parallel Requests**: > 30k ops/sec

## Troubleshooting

### Go Tests

**Issue**: Package conflicts
```bash
# Solution: Check for multiple packages in same directory
go list ./...
```

**Issue**: Import errors
```bash
# Solution: Update dependencies
go mod tidy
go get -u ./...
```

### Python Tests

**Issue**: Module not found
```bash
# Solution: Install in development mode
pip install -e .
```

**Issue**: Async tests failing
```bash
# Solution: Install pytest-asyncio
pip install pytest-asyncio
```

## Contributing

When adding new features:

1. Add corresponding tests in `*_test.go` or `test_*.py`
2. Add benchmarks if performance-critical
3. Update this README with new test coverage
4. Ensure all tests pass before submitting PR

## Summary

✅ **Go Client**: Comprehensive tests and benchmarks for both HTTP and wire protocol  
✅ **Python Client**: REST API tests and benchmark framework
⚠️ **Ruby Client**: Needs test implementation

The test suites ensure reliability, performance, and correctness of both the HTTP REST API layer and the native wire protocol layer.
