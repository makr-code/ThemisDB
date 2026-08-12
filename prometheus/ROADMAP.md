# prometheus/ ROADMAP

Tracks planned enhancements and production-readiness work for the Prometheus alerting rules in ThemisDB.

## Current Status

- [x] `rules/llm_alerts.yml` — LLM inference alert rules implemented
- [x] `rules/sla-rules.yml` — SLA / availability alert rules implemented and moved to standard location
- [x] `grafana/prometheus.yml` includes `llm_alerts.yml` via rule_files
- [ ] `grafana/prometheus.yml` does not yet include `sla-rules.yml` (see In Progress)
- [ ] Alert expressions not yet validated against live ThemisDB metric names

## In Progress

- [~] Wire `sla-rules.yml` into `grafana/prometheus.yml` rule_files (Target: Q3 2026)
- [~] Validate all alert expressions against ThemisDB metric names (Target: Q3 2026)

## Planned Features

- [ ] Add sharding/replication lag alert rules (Target: Q4 2026)
- [ ] Add query performance alert rules (Target: Q4 2026)
- [ ] Add storage engine health alert rules (Target: Q4 2026)
- [ ] Add runbook URLs to all alert annotations (Target: Q3 2026)
- [ ] Add severity labels (`critical`, `warning`, `info`) consistently across all rules (Target: Q3 2026)

## Implementation Phases

### Phase 1: Design / API-Vertrag
- [x] Standard rules directory layout (`prometheus/rules/`)
- [x] SLA rule file migrated from non-standard `alerts/` to `rules/`
- [ ] Agree on metric naming convention for all ThemisDB components (Target: Q3 2026)

### Phase 2: Core-Implementierung
- [x] LLM inference alert rules (`llm_alerts.yml`)
- [x] SLA / availability alert rules (`sla-rules.yml`)
- [ ] Sharding / replication lag alert rules (Target: Q4 2026)

### Phase 3: Fehlerbehandlung & Edge Cases
- [ ] Test alert rules with `promtool check rules rules/*.yml` in CI (Target: Q3 2026)
- [ ] Add absence alerts for critical scraped metrics (Target: Q3 2026)

### Phase 4: Tests
- [ ] Unit-test alert expressions with `promtool test rules` (Target: Q3 2026)
- [ ] CI gate: all rule files pass `promtool check rules` (Target: Q3 2026)

### Phase 5: Performance/Hardening
- [ ] Review and tune `for:` durations to reduce alert flap (Target: Q4 2026)
- [ ] Add recording rules for expensive PromQL expressions (Target: Q4 2026)

### Phase 6: Dokumentation & Abnahme
- [ ] Runbooks for all alerts in `docs/operations/` (Target: Q4 2026)
- [ ] Operations acceptance sign-off (Target: Q4 2026)

## Production Readiness Checklist

- [x] Alert rule files present and correctly placed in `rules/`
- [x] Directory structure follows Prometheus convention
- [ ] `sla-rules.yml` wired into `grafana/prometheus.yml`
- [ ] All alert expressions validated against live metric names
- [ ] `promtool check rules` passes in CI
- [ ] Runbooks linked via `runbook_url` annotation on every alert
- [ ] Severity labels applied consistently

## Known Issues

- [!] `grafana/prometheus.yml` rule_files references legacy `/etc/prometheus/alerts/sla-rules.yml` path — must be updated to `/etc/prometheus/rules/sla-rules.yml` (tracked in In Progress above)
- [!] Alert metric names in `sla-rules.yml` and `llm_alerts.yml` are not yet cross-referenced against the ThemisDB metrics registry

## Breaking Changes

- `prometheus/alerts/sla-rules.yml` has been moved to `prometheus/rules/sla-rules.yml`. Any Prometheus configuration referencing the old path must be updated.
