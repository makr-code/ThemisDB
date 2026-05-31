> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Core Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production Ready** — Dependency injection, cross-cutting concerns management (logging, tracing, metrics, caching, secrets, feature flags, audit log), and pluggable adapter infrastructure are functional. OpenTelemetry, Jaeger, Zipkin, and Prometheus adapters are implemented.

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋
- [I] Plugin-based adapter loading (no recompile needed) (Issue: #1706)
- [ ] Adapter plugin hardening and signing workflow (Target: Q4 2026)

## Production Readiness Checklist
- Status: Completed
- Nachweise: test_concerns_context.cpp (146 Tests), test_fuzz_core.cpp, benchmarks/bench_di_logging.cpp
- Hinweis: Erledigte Tasks sind im `CHANGELOG.md` dokumentiert.

## Known Issues & Limitations
- Context propagation across async/thread boundaries is supported via `startSpanFromHeaders` / `injectContext`; caller is responsible for passing headers across async boundaries
- Feature flags are now a first-class concern in the DI system via `IFeatureFlags` / `InMemoryFeatureFlags`
- Plugin-based adapter loading remains open (Issue: #1706)

## Breaking Changes
- New concern interfaces (health check, feature flags, secrets) will be additive
- Existing ILogger, ITracer, IMetrics, ICache interfaces are stable
