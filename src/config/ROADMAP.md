# Config Module Roadmap

## Current Status
Production-ready for legacy-to-new config path resolution with LRU caching, path validation, deprecation metadata, and thread-safe metrics. Runtime hot-reload and YAML/JSON parsing are out of scope for this module.

## Completed ✅
- [x] Legacy-to-new config path mapping with 60+ path mappings
- [x] Filesystem fallback: tries new path first, then legacy path with deprecation warning
- [x] LRU cache (capacity 1000, TTL 5 min) for resolved paths
- [x] Path traversal prevention and normalization
- [x] Deprecation and removal-date metadata per mapped path
- [x] Migration guide links per deprecated path
- [x] Thread-safe metrics: hits, misses, cache hits, legacy fallbacks (std::atomic)
- [x] Optional API: `tryResolve()` returning `std::nullopt` on failure
- [x] Typed exception hierarchy for config-related errors
- [x] Coverage of AI/ML, security, compliance, performance, platform, networking, and monitoring categories

## In Progress 🚧
- [ ] Migration tooling to batch-rename legacy config files to new paths (Target: Q2 2026)
- [ ] Deprecation warning aggregation report (list all legacy paths in use) (Target: Q2 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] CLI tool to scan deployment and report all legacy config paths in use
- [ ] Automatic legacy path migration script with dry-run mode
- [ ] Configurable LRU cache size and TTL via environment variable
- [ ] Metrics export to Prometheus endpoint
- [ ] Warning threshold alerting when legacy fallback rate exceeds threshold

### Long-term (6-12 months)
- [ ] Complete removal of all deprecated legacy path mappings (post-migration)
- [ ] Integration with config validation (JSON Schema / YAML schema)
- [ ] Runtime hot-reload of resolved path cache on SIGHUP
- [ ] Config audit trail: log which paths were accessed and when
- [ ] Multi-environment config overlay (dev/staging/prod path sets)

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (path resolution, LRU cache, fallback, metadata)
- [ ] Performance benchmarks (cache hit rate, resolution latency)
- [x] Security audit (path traversal prevention)
- [x] Documentation complete
- [x] API stability guaranteed for ConfigPathResolver

## Known Issues & Limitations
- Does not parse or validate config file contents (YAML/JSON parsing is out of scope)
- Runtime hot-reload is not supported; cache is rebuilt on next access after TTL expiry
- Secrets management and credential injection are explicitly out of scope
- Migration tooling is not yet implemented; teams must manually rename config files

## Breaking Changes
- Removal of deprecated legacy path mappings is planned once migration tooling is released and a deprecation window expires
- Cache size and TTL will become configurable (currently hardcoded)
