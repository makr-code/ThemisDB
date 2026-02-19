# AQL/LLM Subsystem: Production-Readiness Assessment & Roadmap

## Current Status: Not Production-Ready

The AQL/LLM subsystem is **NOT** 100% production-ready. While the core functionality is implemented and demonstrates promising capabilities, several critical gaps remain that must be addressed before deploying to production environments.

### Key Gaps Identified

- **Missing Input/Size/Time Limits**: No enforced limits on prompt size, query complexity, or execution time
- **Absent Authorization & Rate Limiting**: No scope-based authorization (AuthZ) or per-user/tenant rate limiting
- **Lack of Observability**: Missing metrics, distributed tracing, and structured logging for LLM operations
- **Limited Error Masking**: Internal errors exposed to clients; no structured error objects with codes
- **RAG Context Issues**: 
  - Uses document primary key (pk) as placeholder content instead of fetching actual document content from storage
  - No score normalization for similarity search results
- **Minimal Logging & Resilience**: No circuit breakers, retries with exponential backoff, or timeout handling
- **Missing Tests**: Limited unit tests, no integration tests for LLM operations, no fuzz testing
- **Governance for Model/LoRA Loading**: No policy enforcement for model loading, version control, or artifact signing

## Roadmap to Production

### 1. Stabilität & Sicherheit (Stability & Security)

#### Input Validation & Limits
- Enforce maximum prompt length (e.g., 32K tokens)
- Add query complexity limits (max depth, max operations)
- Implement execution timeouts with configurable defaults
- Validate model and LoRA IDs against allow-list

#### Rate Limiting & Authorization
- Implement per-user/tenant rate limiting (requests per minute/hour)
- Add scope-based authorization for LLM operations (e.g., `llm:infer`, `llm:rag`, `llm:model:load`)
- Enforce tenant isolation for model access and caching

#### Resilience & Error Handling
- Add timeouts for all external LLM calls
- Implement retry logic with exponential backoff
- Add circuit breakers to prevent cascade failures
- Implement graceful degradation when models are unavailable

#### Structured Errors
- Define error code taxonomy (e.g., `LLM_MODEL_NOT_FOUND`, `LLM_TIMEOUT`)
- Mask internal errors; return sanitized messages to clients
- Include correlation IDs for tracing
- Provide safe defaults for all error conditions

---

### 2. Korrektheit & Tests (Correctness & Tests)

#### Unit Tests
- Add unit tests for all LLM handler functions
- Test error paths and edge cases
- Mock LLM backends for fast, deterministic tests

#### Integration Tests
- End-to-end tests for `LLM INFER`, `LLM RAG`, `LLM EMBED` operations
- Test model loading/unloading lifecycle
- Test LoRA adapter switching

#### Schema-Aware Validation
- Validate natural language → AQL translation against schema
- Ensure generated AQL queries are syntactically correct
- Verify semantic correctness (e.g., collection names, field names)

#### RAG Quality Benchmarks
- Establish baseline accuracy metrics for RAG queries
- Test retrieval relevance with known ground truth
- Measure answer quality with LLM-as-judge or human eval

#### Fuzz Testing
- Fuzz test prompt inputs for crashes or hangs
- Test malformed AQL fragments
- Validate robustness against adversarial inputs

---

### 3. Observability & Operations

#### Metrics
- Track inference latency (p50, p95, p99)
- Monitor token throughput (tokens/sec)
- Measure cache hit rates (prefix cache, response cache)
- Count errors by type and model
- Track active model memory usage

#### Distributed Tracing
- Instrument LLM operations with OpenTelemetry spans
- Trace end-to-end query execution (AQL → RAG → LLM → response)
- Include model ID, LoRA ID, and prompt metadata in spans

#### Logging
- Structured JSON logging for all LLM operations
- Include request ID, user ID, tenant ID, model ID
- Log query inputs (sanitized), outputs, and latencies
- Implement log level controls (DEBUG, INFO, WARN, ERROR)

#### Dashboards & Alerts
- Create Grafana dashboards for LLM health and performance
- Set up alerts for high error rates, slow queries, or OOM conditions
- Monitor model availability and version drift

---

### 4. API-Design & DX (Developer Experience)

#### Persisted Queries
- Allow pre-registered, named LLM queries (e.g., `EXECUTE QUERY 'summarize_docs'`)
- Support parameterized queries for safety and reusability
- Maintain query registry with versioning

#### Allow-Listed Queries
- Option to restrict LLM operations to allow-listed queries only
- Prevents arbitrary prompt injection in production

#### Strict Response Formats
- Define JSON schemas for LLM responses
- Support structured output modes (JSON, YAML, XML)
- Validate outputs against schema before returning

#### Cleanup Pipeline
- Add cleanup hooks for temporary data (embeddings, cached results)
- Implement TTL policies for cached responses
- Provide manual cache invalidation APIs

#### Consistent Error Objects
- Standardize error response format across all LLM operations
- Include error code, message, details, and timestamp
- Support localization for error messages

---

### 5. Performance

#### Batch Inference
- Support batch processing of multiple prompts in a single request
- Optimize throughput with KV-cache sharing across batch items

#### Streaming
- Implement streaming responses for long-running generations
- Use Server-Sent Events (SSE) or WebSocket for real-time updates

#### Caching Strategies
- Integrate with ThemisDB's SemanticCache for response caching
- Use HNSW-based similarity search for cache lookups
- Implement prefix cache with actual embedding similarity (not stub)

#### RAG Content Fetching
- Use actual document content instead of `_key` placeholders
- Optimize vector index queries for low latency
- Support chunking strategies for large documents

#### Score Normalization
- Normalize similarity scores across different embedding models
- Apply calibration for consistent thresholds

#### Model & LoRA Lifecycle
- Implement lazy loading and unloading of models
- Support model warm-up to avoid cold-start latency
- Add memory management policies (LRU eviction, priority queues)

#### High-Throughput Backends (Optional)
- Integrate with vLLM, TensorRT-LLM, or llama.cpp for optimized inference
- Support GPU batching and paged attention

---

### 6. Daten- & Änderungsmanagement (Data & Change Management)

#### Model & Adapter Registry
- Centralized registry for all available models and LoRA adapters
- Store model metadata (version, size, capabilities, license)
- Track adapter compatibility with base models

#### Versioning
- Semantic versioning for models and adapters
- Support multiple versions in parallel for A/B testing
- Provide rollback mechanism for model updates

#### Reproducibility
- Pin model versions in production queries
- Log model version used for each inference request
- Archive model artifacts with checksums

#### Migration Handling
- Define upgrade paths for model version changes
- Test backward compatibility for existing queries
- Provide migration scripts and documentation

---

### 7. Security Hardening

#### Input Sanitization & Redaction
- Sanitize prompts to prevent injection attacks
- Redact PII (names, emails, phone numbers) from logs
- Validate all user inputs against strict schemas

#### Signed Artifacts
- Require cryptographic signatures for model and LoRA files
- Verify integrity before loading artifacts
- Maintain chain-of-custody for model provenance

#### Least-Privilege Paths
- Run LLM processes with minimal OS permissions
- Use dedicated service accounts with restricted access
- Isolate model files in read-only directories

#### Secrets Management
- Store API keys and credentials in secure vaults (e.g., HashiCorp Vault)
- Rotate secrets regularly
- Never log or expose secrets in error messages

---

### 8. Delivery & Governance

#### CI/CD Gates
- Enforce linting (clang-format, clang-tidy) in CI pipeline
- Run all unit and integration tests before merge
- Include fuzz tests in nightly builds
- Perform SAST (Static Application Security Testing) scans

#### Canary Deployments
- Deploy new model versions to a subset of traffic first
- Monitor for regressions before full rollout
- Automate rollback on errors or performance degradation

#### Feature Flags
- Use feature flags to enable/disable LLM features per tenant
- Allow gradual rollout of new capabilities
- Support emergency kill switches

#### Runbooks
- Document operational procedures for common issues
- Provide troubleshooting guides for LLM failures
- Define escalation paths and SLAs

---

## Conclusion

The AQL/LLM subsystem has a solid foundation but requires significant hardening before production deployment. The roadmap outlined above provides a structured path to address critical gaps in stability, security, testing, observability, and operations. Prioritization should focus on:

1. **Immediate (P0)**: Stabilität & Sicherheit, Korrektheit & Tests
2. **Short-term (P1)**: Observability & Operations, Security Hardening
3. **Medium-term (P2)**: Performance, API-Design & DX
4. **Long-term (P3)**: Daten- & Änderungsmanagement, Delivery & Governance

Progress should be tracked iteratively, with each milestone validated through testing and real-world use cases.
