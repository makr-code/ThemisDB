<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Chimera Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Chimera module provides a vendor-neutral adapter framework for connecting to external databases (MongoDB, PostgreSQL, Neo4j, Elasticsearch, Pinecone, Qdrant, Weaviate, and others). Security concerns focus on: secure credential handling for external database connections, preventing adapter injection, safe simulation mode use, and protecting benchmark operations from data leakage.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Credential exposure in adapter configuration | Database credentials injected via configuration; not logged or included in metrics output |
| Unauthorized adapter registration | Adapter factory registry uses thread-safe singleton; registration from untrusted code paths is blocked by RBAC at the module loading layer |
| Simulation mode in production | Simulation mode adapters are clearly flagged in metrics and system information; production deployments should disable simulation adapters |
| Cross-tenant data access via shared adapter | Each adapter connection is tenant-scoped; benchmark framework uses isolated connections per test run |
| Injection via adapter operation parameters | All operation wrappers validate parameter types before forwarding to underlying driver |
| SQL injection via PostgreSQL adapter | PostgreSQL adapter uses parameterized queries via `libpqxx` (production driver pending) |
| Unsecured external database connections | TLS configuration is required for production adapter connections; adapters expose TLS configuration in `ConnectionConfig` |

## Security Controls

### Credential Handling
- Database credentials (username, password, API keys, connection strings) are read from configuration and injected at adapter construction time.
- Credentials are never included in log output, metrics labels, or benchmark results.
- Connection strings containing embedded credentials are masked in debug output.

### Adapter Factory Security
- Adapter factory uses a thread-safe singleton with mutex-protected registration.
- Dynamic registration is allowed only during initialization; post-startup registration is blocked.
- Each adapter reports its system information including whether it is in simulation mode.

### Simulation Mode
- Adapters operating in simulation mode (e.g., PostgreSQL pending `libpqxx`, HTTP-based adapters pending driver integration) are explicitly identified via `isSimulationMode()` flag.
- Production deployments should audit adapter registrations to ensure only fully wired adapters are active.

### Connection Security
- All planned production adapters will use TLS for wire encryption.
- Elasticsearch, Pinecone, Qdrant, Weaviate (HTTP-based): TLS enforced via `cpp-httplib`/`cpr` with certificate validation.
- Neo4j (Bolt protocol): TLS-encrypted Bolt connections planned.
- MongoDB: TLS via `libmongocxx` driver.

## Data Handling

- Chimera adapters are used for benchmarking and cross-database query federation; they forward queries to external systems.
- Query results returned through adapters are held in memory for the duration of the operation; not persisted by this module.
- Benchmark results (latency, throughput) do not include document content; only aggregate statistics.
- No PII or sensitive data should be included in benchmark test data sets.

## Known Limitations

- PostgreSQL adapter (`postgresql_adapter.cpp`) is in simulation mode; production `libpqxx` wiring is pending (Issue #1632).
- HTTP-based adapters (Elasticsearch, Pinecone, Qdrant, Weaviate) are in simulation mode pending driver integration.
- Cross-system query federation (planned) will require careful tenant isolation to prevent cross-database data leakage.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| libpqxx (planned) | PostgreSQL production driver | TLS via OpenSSL |
| libmongocxx (planned) | MongoDB production driver | TLS connection option |
| cpp-httplib / cpr (planned) | HTTP-based adapters | TLS with certificate validation |
| Neo4j Bolt client (planned) | Neo4j graph database | TLS-encrypted Bolt protocol |
