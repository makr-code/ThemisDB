# ROADMAP — grafana

## Current Status
- [x] Dashboard JSON files exist and cover LLM, Security, SIEM, Performance, Operations, Compliance, BT4/FLARE
- [x] Docker Compose stack (Prometheus, Grafana, Jaeger, OTel Collector, Alertmanager)
- [x] Alert rules: LLM, SIEM, Graph Security, BT4/FLARE, Performance Regression
- [x] Datasource provisioning: Prometheus + Jaeger
- [x] Dashboard directory restructured into thematic subfolders
- [x] Alert consolidation: central `alerts/` tree, recording rules, Alertmanager config template
- [x] Helm integration: Grafana dashboard ConfigMaps via Operator pattern
- [x] Q3 2026 operations dashboards: failover-state, process-lifecycle, updates-rollback, cdc-pipeline, chaos-resilience, scheduler-detail
- [x] Q3 2026 alert rules: failover_alerts.yml, updates_alerts.yml, cdc_alerts.yml, process_alerts.yml, rag_alerts.yml, watchdog_alerts.yml
- [x] Q4 2026 dashboards (JSON added): vector-search, rag-hallucination, tensor-distributed, access-model, ethics-governance, lora-training, onnx-inference, whisper-voice
- [x] New categories: analytics/ (analytics-overview.json), geo/ (geo-queries.json)
- [x] Provisioning providers for analytics/ and geo/ added to dashboards.yml
- [x] Helm grafana-dashboards.yaml and values.yaml updated with all new dashboards and toggle flags

## In Progress
- [ ] Alertmanager contact point credentials filled for production environments (Target: 2026-Q3)
- [ ] Dashboard variable templating (multi-instance, namespace selectors) (Target: 2026-Q3)
- [~] Q3 dashboard coverage: operations dashboards for failover, process, updates, CDC, chaos (Target: 2026-Q3)

## Planned Features
- [ ] Multi-cluster Prometheus federation support (Target: 2026-Q4)
- [ ] Loki integration for log-to-metric correlation in dashboards (Target: 2026-Q4)
- [ ] OpenTelemetry exemplar linking (trace ↔ metric drill-through) (Target: 2026-Q4)
- [ ] Automated compliance report scheduling via compliance_exporter.py + CronJob (Target: 2026-Q4)
- [ ] Grafana Unified Alerting migration (alerting rules in Grafana instead of Prometheus) (Target: 2027-Q1)
- [ ] Q4 dashboards: vector-search, rag-hallucination, tensor-distributed, access-model, ethics-governance, lora-training, onnx-inference, whisper-voice, analytics-overview (Target: 2026-Q4)
- [ ] geo/geo-queries.json (dependent on CUDA geo kernels, Target: 2027-Q1)

## Implementation Phases

### Phase 1: Design / API-Vertrag
- [x] Dashboard category taxonomy defined (llm / performance / security / operations / compliance / plugins / bt4)
- [x] Provisioning provider structure per category documented
- [x] Alert rule ownership: all rules under `grafana/alerts/`; recording rules in `recording_rules.yml`
- [x] Alertmanager routing tree designed (severity + SIEM category split)

### Phase 2: Core-Implementierung
- [x] Dashboard JSONs moved to thematic subdirectories under `dashboards/`
- [x] `provisioning/dashboards/dashboards.yml` rewritten: one provider per category, correct paths
- [x] `provisioning/alerts.yml` converted to Grafana contact-point loader (rules moved to `alerts/`)
- [x] `alertmanager.yml` template created with Slack / PagerDuty / Email receivers
- [x] `alerts/recording_rules.yml` added with LLM, auth baseline, and performance recording rules
- [x] `alerts/llm_alerts.yml` canonical LLM alert rules (de-duped from provisioning/alerts.yml)
- [x] Helm `templates/grafana-dashboards.yaml`: ConfigMaps for all dashboard categories
- [x] `values.yaml` Grafana section with per-category toggles

### Phase 3: Fehlerbehandlung & Edge Cases
- [x] Alertmanager dead-man's switch (Watchdog alert) added to `alerts/watchdog_alerts.yml` (2026-Q3)
- [x] Alert inhibition rules tuned for split-brain and maintenance windows in `alertmanager.yml` (2026-Q3)
- [ ] Jaeger datasource URL health check in docker-compose (Target: 2026-Q3)

### Phase 4: Tests
- [ ] `promtool check rules alerts/*.yml` in CI pipeline (Target: 2026-Q3)
- [ ] Helm `helm lint` and `helm template` validation in CI (Target: 2026-Q3)
- [ ] Integration test: docker-compose up + Grafana provisioning smoke test (Target: 2026-Q3)

### Phase 5: Performance/Hardening
- [ ] Prometheus recording rules cover all high-cardinality dashboard queries (Target: 2026-Q4)
- [ ] Retention and compaction settings documented per deployment tier (Target: 2026-Q4)
- [ ] Grafana image pinned to specific version (no `latest`) in docker-compose (Target: 2026-Q3)

### Phase 6: Dokumentation & Abnahme
- [x] `ARCHITECTURE.md` updated with actual component diagram
- [x] `ROADMAP.md` updated with concrete, measurable items
- [x] `FUTURE_ENHANCEMENTS.md` updated with Helm/Alertmanager/multi-cluster scope
- [ ] Operator runbook: how to add a new dashboard category end-to-end (Target: 2026-Q3)
- [ ] Compliance exporter documented in operations wiki (Target: 2026-Q3)

## Production Readiness Checklist
- [x] Dashboard provisioning covers all dashboard JSON files
- [x] Alert rules deduplicated and centrally located
- [x] Recording rules prevent expensive runtime aggregation
- [x] Alertmanager routing tree defined
- [ ] Alertmanager credentials provisioned via secrets (not committed)
- [ ] `promtool check rules` passes for all alert files in CI
- [ ] Grafana admin password not hardcoded (uses `GF_ADMIN_PASSWORD` env var)
- [ ] Prometheus retention and storage sized for target environment
- [ ] ServiceMonitor enabled and Prometheus Operator CRDs present in cluster

## Known Issues & Limitations
- Grafana dashboard ConfigMaps via Helm require the Grafana sidecar to be enabled.
- Alertmanager credentials (`SLACK_WEBHOOK_URL`, `PAGERDUTY_KEY`, etc.) must be supplied externally — never commit real values.
- Multi-cluster federation not yet implemented; all Prometheus scrape targets are single-cluster.

## Breaking Changes
- `service.metricsPort` default changed from `4318` to `9091` in Helm chart v0.2.0.
- `provisioning/alerts.yml` is now a Grafana contact-point loader, not a Prometheus rule file.
  Prometheus rules are in `alerts/llm_alerts.yml` and sibling files.

