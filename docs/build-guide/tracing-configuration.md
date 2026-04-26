# Distributed Tracing Configuration Guide

## Overview

ThemisDB supports distributed tracing using OpenTelemetry with OTLP HTTP export to Jaeger, Grafana Tempo, or any OpenTelemetry-compatible backend.

## Configuration

### Basic Configuration (config.yaml)

```yaml
tracing:
  enabled: true
  service_name: "themis-server"
  otlp_endpoint: "http://localhost:4318"
```

### Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enabled` | boolean | `false` | Enable/disable distributed tracing |
| `service_name` | string | `"themis-server"` | Service name for trace identification |
| `otlp_endpoint` | string | `"http://localhost:4318"` | OTLP HTTP endpoint URL |

## Integration with Backends

### Jaeger

1. **Start Jaeger (Docker)**:
```bash
docker run -d --name jaeger \
  -e COLLECTOR_OTLP_ENABLED=true \
  -p 16686:16686 \
  -p 4318:4318 \
  jaegertracing/all-in-one:latest
```

2. **Configure ThemisDB**:
```yaml
tracing:
  enabled: true
  service_name: "themis-db"
  otlp_endpoint: "http://localhost:4318"
```

3. **Access Jaeger UI**: http://localhost:16686

### Grafana Tempo

1. **Start Tempo (Docker)**:
```bash
docker run -d --name tempo \
  -p 3200:3200 \
  -p 4318:4318 \
  grafana/tempo:latest \
  -config.file=/etc/tempo-local.yaml
```

2. **Configure ThemisDB**:
```yaml
tracing:
  enabled: true
  service_name: "themis-db"
  otlp_endpoint: "http://localhost:4318"
```

3. **Configure Grafana**:
   - Add Tempo as data source
   - URL: http://localhost:3200

### OpenTelemetry Collector

1. **Create collector config** (`otel-collector-config.yaml`):
```yaml
receivers:
  otlp:
    protocols:
      http:
        endpoint: 0.0.0.0:4318

processors:
  batch:
    timeout: 10s
    send_batch_size: 1024

exporters:
  jaeger:
    endpoint: jaeger:14250
    tls:
      insecure: true
  logging:
    loglevel: debug

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [batch]
      exporters: [jaeger, logging]
```

2. **Start collector**:
```bash
docker run -d --name otel-collector \
  -p 4318:4318 \
  -v $(pwd)/otel-collector-config.yaml:/etc/otel-collector-config.yaml \
  otel/opentelemetry-collector:latest \
  --config=/etc/otel-collector-config.yaml
```

3. **Configure ThemisDB**:
```yaml
tracing:
  enabled: true
  service_name: "themis-db"
  otlp_endpoint: "http://localhost:4318"
```

## Instrumented Components

ThemisDB automatically instruments the following components:

### Query Engine
- Query execution (`QueryEngine.executeAndKeys`, `QueryEngine.executeAndEntities`)
- AQL parsing and translation
- Query optimization
- Join operations
- Aggregations

### Storage Engine
- Storage operations (`StorageEngine.put`, `StorageEngine.get`, `StorageEngine.del`)
- Key/value size tracking

### Index Manager
- Index creation (`IndexManager.createSecondaryIndex`, `IndexManager.createVectorIndex`)
- Index lookup and updates
- Index-specific attributes

### Plugin Manager
- Plugin discovery (`PluginManager.scanPluginDirectory`)
- Plugin loading (`PluginManager.loadPlugin`)
- Plugin lifecycle events

### Security Layer (Access Control)
- Authentication (`AccessControl.authenticate`)
- Authorization checks
- MFA operations
- Security event tracking

### API Layer
- HTTP request/response flows
- All API endpoints (`/query`, `/query/aql`, `/entity`, etc.)
- Request routing and processing

## Prometheus Metrics Integration

Tracing metrics are automatically exposed on the `/metrics` endpoint:

### Available Metrics

```prometheus
# Span creation metrics
themis_trace_spans_total          # Total spans created
themis_trace_active_spans         # Currently active spans

# Span duration histograms (from MetricsCollector)
trace_span_duration_ms{span="operation_name"}    # Span duration in milliseconds
trace_spans_total{span="operation_name"}         # Span count by operation
```

### Example Prometheus Query

```promql
# Average span duration by operation
rate(trace_span_duration_ms_sum[5m]) / rate(trace_span_duration_ms_count[5m])

# Active spans over time
themis_trace_active_spans

# Span creation rate
rate(themis_trace_spans_total[1m])
```

## Span Attributes

ThemisDB automatically adds contextual attributes to spans:

### Query Engine Spans
- `query.table`: Table name
- `query.type`: Query type (AND, OR, AQL)
- `query.result_count`: Number of results
- `query.entities_count`: Number of entities loaded

### Storage Engine Spans
- `storage.key_size`: Key size in bytes
- `storage.value_size`: Value size in bytes
- `storage.operation`: Operation type

### Index Manager Spans
- `index.name`: Index name
- `index.field`: Indexed field name
- `index.type`: Index type (range, fulltext, geo, etc.)
- `index.dimension`: Vector dimension (for vector indices)

### Plugin Manager Spans
- `plugin.name`: Plugin name
- `plugin.directory`: Plugin directory path
- `plugin.discovered_count`: Number of plugins discovered

### Security Spans
- `security.user_id`: User identifier
- `security.auth_type`: Authentication type (password, oauth)
- `security.result`: Operation result

### API Spans
- `http.method`: HTTP method
- `http.path`: Request path
- `http.status`: Response status code

## Performance Considerations

### Overhead When Tracing is Enabled

Tracing adds minimal overhead when enabled:
- **Span creation**: ~100-200ns per span
- **Attribute addition**: ~50-100ns per attribute
- **OTLP export**: Asynchronous, non-blocking
- **Memory**: ~200-500 bytes per active span

### Overhead When Tracing is Disabled

When compiled without `THEMIS_ENABLE_TRACING`:
- All tracing calls become no-ops at compile time
- Zero runtime overhead
- Code optimized away by compiler

### High-Throughput Scenarios

For very high-throughput operations (>100k ops/sec), consider:
1. **Compile-time control**: Build without `THEMIS_ENABLE_TRACING` for maximum performance
2. **Selective tracing**: Only enable tracing on specific environments (staging, troubleshooting)
3. **Operation sampling**: In storage-heavy workloads, tracing every operation may add 1-2% overhead

### Sampling (Future Enhancement)

For high-throughput production environments, consider implementing sampling:

```yaml
tracing:
  enabled: true
  service_name: "themis-db"
  otlp_endpoint: "http://localhost:4318"
  sampling_rate: 0.1  # Sample 10% of traces (future)
```

### Network Impact

- Traces are exported via HTTP (non-blocking)
- Failed exports are logged but don't block operations
- Collector reachability is checked before initialization

### Memory Impact

- Each span uses approximately 200-500 bytes
- Spans are flushed immediately (no batching currently)
- Active spans are tracked but don't accumulate

## Troubleshooting

### Tracing Not Working

1. **Check if tracing is enabled**:
```bash
curl http://localhost:8000/metrics | grep trace_spans_total
```

2. **Check logs**:
```bash
grep "tracing" themis.log
```

3. **Verify collector reachability**:
```bash
curl http://localhost:4318/v1/traces
```

### No Traces in Backend

1. **Check OTLP endpoint configuration**
2. **Verify collector is running and accepting traces**
3. **Check collector logs for errors**
4. **Ensure service name matches in backend filters**

### High Overhead

1. **Verify collector is reachable** (connection timeouts cause delays)
2. **Consider implementing sampling** (future feature)
3. **Check network latency to collector**

## Example: End-to-End Trace

A typical query request creates the following span hierarchy:

```
POST /query/aql
├── aql.parse
│   └── aql.translate
├── QueryEngine.executeAndKeys
│   ├── IndexManager.lookup
│   │   └── StorageEngine.get
│   └── StorageEngine.scan
└── QueryEngine.executeAndEntities
    └── StorageEngine.get (multiple)
```

Each span includes:
- Start/end timestamps
- Duration
- Attributes (table name, query type, result count, etc.)
- Status (OK/Error)
- Error details (if failed)

## Best Practices

1. **Use descriptive span names**: Follow format `Component.Operation`
2. **Add meaningful attributes**: Include IDs, sizes, types
3. **Record errors**: Always call `recordError()` on failures
4. **Set span status**: Use `setStatus()` to indicate success/failure
5. **Use child spans**: Create hierarchies for complex operations
6. **Keep collector close**: Minimize network latency
7. **Monitor metrics**: Track span counts and durations

## Security Considerations

1. **Sensitive Data**: Span attributes should NOT include:
   - Passwords or credentials
   - Personal identifiable information (PII)
   - Encryption keys
   - Session tokens

2. **Network Security**: 
   - Use HTTPS for production OTLP endpoints
   - Implement mutual TLS if required
   - Restrict collector access via firewall

3. **Access Control**:
   - Secure Jaeger/Tempo UI with authentication
   - Limit metrics endpoint access
   - Review trace data retention policies

## Version Compatibility

- **ThemisDB**: v1.3.x+
- **OpenTelemetry C++ SDK**: 1.x
- **OTLP Protocol**: HTTP/1.1
- **Jaeger**: 1.40+
- **Grafana Tempo**: 2.0+

## References

- [OpenTelemetry Documentation](https://opentelemetry.io/docs/)
- [Jaeger Documentation](https://www.jaegertracing.io/docs/)
- [Grafana Tempo Documentation](https://grafana.com/docs/tempo/)
- [OTLP Specification](https://opentelemetry.io/docs/specs/otlp/)
