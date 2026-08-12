# FUTURE_ENHANCEMENTS — grafana

## grafana

### Scope
- Automated, production-ready observability stack for ThemisDB across development, community, and enterprise deployments.
- Full integration between Prometheus, Grafana, Jaeger, OpenTelemetry Collector, and Alertmanager.
- Kubernetes-native dashboard delivery via Helm + Grafana sidecar Operator pattern.
- Compliance-grade alert coverage (SOC2, GDPR, HIPAA) with automated report generation.

### Design Constraints
- All alert rules must pass `promtool check rules` with no warnings.
- Dashboard JSONs must remain valid Grafana JSON model (schema version ≥ 36).
- Recording rules must replace all alert expressions that contain nested `avg_over_time` or multi-level aggregations.
- Credentials (Slack webhooks, PagerDuty keys, SMTP passwords) must be injected as environment variables or Kubernetes Secrets — never committed to source.
- Grafana admin password must be supplied via `GF_ADMIN_PASSWORD` env var; no default `admin` password.

### Required Interfaces
- **Prometheus scrape**: `GET /metrics` on port `9091` (Prometheus-compatible text format).
- **Health endpoint**: `GET /health` on port `8081` — must return HTTP 200 when healthy.
- **OTLP trace ingest**: `POST` to OTel Collector on gRPC `4319` / HTTP `4320`.
- **Alertmanager webhook**: Alertmanager routes to external systems via HTTP webhook or PagerDuty API.
- **Grafana sidecar label**: ConfigMaps must carry `grafana_dashboard: "1"` to trigger auto-import.

### Implementation Notes

#### Multi-cluster support
- Introduce a Prometheus federation job (`federate`) scraping a remote Prometheus instance.
- Add cluster label (`cluster`) to all recording rules so dashboards can filter by cluster.
- Grafana datasource per cluster or use a single Mimir/Thanos endpoint.

#### Loki integration
- Add Loki datasource in `provisioning/datasources/loki.yml`.
- Configure derived fields in Loki to link log lines to Jaeger trace IDs.
- Add `Logs` panel to system-overview and LLM dashboards showing error logs alongside metrics.

#### Unified Alerting migration
- Grafana Unified Alerting replaces Prometheus-managed rules with Grafana-managed rules.
- Migration path: export Prometheus rules → import into Grafana via `POST /api/v1/provisioning/alert-rules`.
- Contact points and notification policies then managed exclusively in Grafana.

#### Compliance report scheduling
- Add a Kubernetes CronJob running `compliance_exporter.py` on a schedule.
- Outputs uploaded to S3-compatible object storage.
- Helm values control schedule, framework selection, and output bucket.

#### OpenTelemetry exemplar linking
- Enable exemplar storage in Prometheus (`--enable-feature=exemplar-storage`).
- Instrument ThemisDB to attach trace IDs as exemplars on histogram metrics.
- Configure Grafana Explore to follow exemplar links to Jaeger traces.

### Test Strategy
- `promtool check rules grafana/alerts/*.yml` must pass in CI (zero errors, zero warnings).
- `helm lint helm/themisdb` and `helm template helm/themisdb` must produce valid YAML.
- Docker Compose smoke test: `docker-compose up -d` → poll Grafana `/api/health` until ready → verify dashboards loaded via `GET /api/search`.
- Alert rule unit tests via `promtool test rules` with synthetic time-series fixtures.

### Performance Targets
- Recording rules must reduce Grafana dashboard load time by ≥ 30% vs. raw expression queries on a 100k-series dataset.
- Alertmanager must deliver critical notifications within 60 seconds of alert firing.
- Prometheus scrape cycle must complete within `scrapeTimeout` (10s) for all ThemisDB targets.

### Security / Reliability
- Alertmanager dead-man's switch (`Watchdog` alert) must fire if Prometheus stops evaluating rules.
- All dashboard JSONs validated via CI to prevent broken panel references.
- Grafana anonymous access disabled by default (`GF_AUTH_ANONYMOUS_ENABLED=false`).
- NetworkPolicy restricts Prometheus scrape access to the `monitoring` namespace.
- Secrets rotation: Alertmanager credentials must be rotatable without chart re-install (use `envFrom` + Secret).

