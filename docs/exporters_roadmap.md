# Exporters Module: Production-Readiness Assessment & Roadmap

## Current Status: Not Production-Ready

The exporters module (`src/exporters`, specifically `jsonl_llm_exporter.cpp`) is **NOT** 100% production-ready. While the core functionality provides basic JSONL export for LLM training data, significant gaps remain that must be addressed before deploying to production environments.

### Key Gaps Identified

- **Input/Schema Validation**: Missing comprehensive input validation and schema validation coverage beyond simple checks; simplified schema validation implementation without robust json-schema validator library integration
- **Authorization & Tenant Controls**: No per-tenant authorization controls or multi-tenant isolation enforcement
- **PII & Content Safety**: No PII detection/redaction enforcement; no content safety checks or redaction policies before export
- **Resource Limits**: Lack of rate limiting, backpressure mechanisms, and size limits on output lines/files
- **Error Handling**: Error handling exists but without structured error surface; limited retry/backoff strategies
- **Observability Gaps**: No metrics (throughput, latency, error rates, quality metrics persistence); no tracing/dashboards for export operations
- **Duplicate Detection**: In-memory only (no streaming dedupe or sliding window support)
- **Testing Coverage**: Missing integration tests for JSON serialization, schema validation, metadata parsing; no fuzz tests; no golden-file tests
- **Output Integrity**: No checksum/signature generation on output files
- **Warmup/Preflight**: No warmup or preflight checks for output paths
- **Operations**: No runbooks, limited operational tooling

Current implementation logs many issues but does not enforce safety/performance limits.

---

## Roadmap to Production

### 1. Stabilität & Sicherheit (Stability & Security)

#### Resource Limits & Backpressure
- Enforce maximum output file size limits (configurable per export)
- Implement throughput limits (records per second, MB/s)
- Add backpressure mechanisms for downstream consumers
- Configure safe defaults for all resource limits
- Add memory limits for in-memory buffers

#### Authorization & Tenant Isolation
- Implement per-tenant authorization controls for export operations
- Add scope-based permissions (e.g., `export:read`, `export:write`, `export:admin`)
- Enforce tenant isolation for exported data (prevent cross-tenant data leakage)
- Add audit logging for all export requests with tenant context

#### PII & Redaction
- Integrate PII detection capabilities (pattern-based and ML-based)
- Implement configurable redaction policies (hash, mask, remove)
- Add policy enforcement for PII-sensitive fields
- Support region-specific compliance requirements (GDPR, CCPA, etc.)

#### Content Safety
- Add pre-export content safety checks
- Implement toxicity/harm detection gates
- Support configurable safety thresholds
- Log safety violations with context for audit

#### Safe Defaults
- Set conservative defaults for all limits (size, rate, memory)
- Enable safety checks by default
- Provide "strict mode" for maximum safety

---

### 2. Korrektheit & Tests (Correctness & Tests)

#### Unit Tests
- Add comprehensive unit tests for all formatting styles (instruction tuning, chat completion, text completion)
- Test schema validation logic with valid/invalid samples
- Test metadata parsing and generation
- Test duplicate detection logic (hash collisions, edge cases)
- Test weight calculation algorithms
- Test quality filtering logic

#### Integration Tests
- Test end-to-end export pipeline with real data
- Test different formatting styles with various data shapes
- Test schema validation with complex nested structures
- Test error handling and recovery paths
- Test multi-tenant scenarios
- Test large-scale exports (GB+ data)

#### JSON/Schema Validation
- Integrate robust json-schema validator library (e.g., nlohmann/json-schema-validator)
- Test validation with various JSON Schema drafts (Draft-07, 2019-09, 2020-12)
- Test with complex schemas (nested objects, arrays, references, conditionals)
- Test validation error reporting and localization

#### Golden-File Tests
- Create golden-file test suite for each formatting style
- Test backward compatibility of output formats
- Test metadata generation stability

#### Fuzz Testing
- Implement fuzz tests for JSON serialization edge cases
- Fuzz metadata parsing logic
- Fuzz schema validation with malformed schemas
- Fuzz input entities with extreme/malicious values

#### Failure Injection
- Test IO failures (disk full, permission denied, network errors)
- Test OOM scenarios
- Test timeout handling
- Test concurrent export conflicts

---

### 3. Observability & Operations (Observability & Operations)

#### Metrics
- **Export Rate**: Records per second, MB/s written
- **Latency**: P50, P95, P99 export latency per record and per batch
- **Error Rates**: Failed entities, validation failures, IO errors
- **Duplicate Counts**: Duplicate detected and skipped
- **Quality Metrics**: Quality filter pass/fail rates, weight distributions
- **Resource Usage**: Memory usage, file descriptor count, disk space used
- Persist quality metrics to time-series database for trend analysis

#### Tracing
- Add OpenTelemetry tracing spans for export operations
- Trace individual entity processing
- Trace schema validation steps
- Trace duplicate detection lookups
- Include correlation IDs for request tracking

#### Dashboards & Alerts
- Create Grafana dashboards for export health
- Alert on high error rates
- Alert on export latency degradation
- Alert on resource limit violations
- Alert on duplicate rate anomalies

#### Progress Callbacks
- Implement configurable progress callbacks for long-running exports
- Support webhook notifications for export completion
- Add streaming progress updates via SSE or WebSocket

#### Structured Logging
- Enhance logging with structured fields (tenant, export_id, entity_count, etc.)
- Add log levels appropriate to each event (INFO, WARN, ERROR)
- Include context in error logs (entity ID, line number, etc.)

---

### 4. Performance (Performance)

#### Streaming IO
- Implement streaming writes to avoid large memory buffers
- Use memory-mapped IO for large exports
- Support chunked/segmented output files

#### Batching
- Implement configurable batch sizes for export operations
- Optimize batch sizes based on downstream consumer requirements
- Support parallel batch processing

#### Memory-Bounded Duplicate Detection
- Replace unbounded in-memory set with sliding window approach
- Implement disk-backed Bloom filter for large-scale dedupe
- Add configurable memory limits for dedupe structures
- Support streaming duplicate detection

#### Compression
- Add optional compression for output files (gzip, zstd)
- Support streaming compression
- Make compression level configurable

#### Warmup/Preflight Checks
- Validate output paths exist and are writable before export
- Pre-allocate file space when possible
- Check available disk space against estimated export size

#### Configurable Flush Intervals
- Add configurable buffer flush intervals
- Support immediate flush mode for real-time consumers
- Optimize flush intervals for throughput vs. latency trade-offs

---

### 5. Security/Privacy (Security/Privacy)

#### PII Detection & Redaction
- Integrate PII detection libraries (pattern-based and ML)
- Support multiple PII categories (email, phone, SSN, credit card, etc.)
- Implement pluggable redaction strategies (hash, mask, encrypt, remove)
- Add PII detection metrics and alerting

#### Encryption & Signing
- Support optional encryption of output files (AES-256-GCM)
- Implement digital signatures for output integrity (Ed25519, RSA)
- Add key management integration (KMS, Vault)
- Support per-tenant encryption keys

#### Supply Chain Security
- Validate schemas against known-good checksums
- Scan dependencies for vulnerabilities (CVE database integration)
- Implement schema provenance tracking
- Add dependency pinning and verification

---

### 6. API/Config & DX (API/Config & Developer Experience)

#### Structured Error Types
- Define error taxonomy for export operations
  - `EXPORT_SCHEMA_VALIDATION_FAILED`
  - `EXPORT_IO_ERROR`
  - `EXPORT_SIZE_LIMIT_EXCEEDED`
  - `EXPORT_TENANT_UNAUTHORIZED`
  - `EXPORT_PII_VIOLATION`
- Include error codes, messages, and remediation hints
- Support error localization (i18n)

#### Config Validation
- Implement schema validation for export configurations
- Provide sensible defaults for all config parameters
- Add config linting tool
- Support config file hot-reloading

#### Feature Flags
- Add feature flags for experimental features
- Support "strict mode" for maximum validation/safety
- Add "preview mode" for new formatting styles
- Enable gradual rollout of new features

#### Admin Operations
- **Dry-Run Mode**: Validate export without writing output
- **Validate-Only Mode**: Check schema/config validity only
- **Resumable Exports**: Support checkpoint/resume for large exports
- **Checksums**: Generate and verify checksums for exported files
- Add admin API for export status monitoring

---

### 7. Daten- & Änderungsmanagement (Data & Change Management)

#### Versioned Export Schemas
- Version all export format schemas (semantic versioning)
- Document schema changes in changelog
- Support multiple schema versions simultaneously
- Add schema version to output metadata

#### Compatibility
- Maintain backward compatibility for at least 2 major versions
- Document breaking changes clearly
- Provide compatibility testing tools
- Add deprecation warnings for old formats

#### Migration Guidance
- Provide migration guides for format changes
- Include example conversion scripts
- Document common migration pitfalls
- Offer migration validation tools

---

### 8. Delivery & Governance (Delivery & Governance)

#### CI Gates
- **Linting**: Enforce code style (clang-format, clang-tidy)
- **Unit Tests**: Require 80%+ coverage for exporters module
- **Integration Tests**: Gate on passing integration test suite
- **Fuzz Tests**: Run fuzz tests in CI (continuous fuzzing)
- **SAST**: Static analysis security testing (CodeQL, Coverity)
- Block merges on CI failures

#### Canary Deployments
- Support canary exports with subset of traffic
- Add feature flags for gradual rollout
- Implement rollback mechanisms
- Monitor canary metrics before full rollout

#### Runbooks
- Create runbooks for common operational scenarios:
  - Export failures and recovery
  - Performance degradation troubleshooting
  - Security incident response
  - Capacity planning
- Document escalation paths
- Include example queries and commands

#### Change Management
- Require security review for PII/redaction changes
- Require performance testing for high-impact changes
- Document all config changes in release notes
- Maintain ADR (Architecture Decision Records) for major changes

---

## Priority Recommendations

1. **P0 (Critical)**: Structured errors, basic metrics, integration tests
2. **P1 (High)**: PII detection/redaction, tenant isolation, schema validator library
3. **P2 (Medium)**: Streaming IO, compression, observability dashboards
4. **P3 (Low)**: Encryption/signing, resumable exports, migration tools

---

## Conclusion

The exporters module provides foundational functionality but requires significant hardening before production deployment. The roadmap above outlines a path to production-readiness with emphasis on safety, correctness, and operational excellence. Teams should prioritize P0/P1 items and implement incrementally with rigorous testing at each stage.
