# ARCHITECTURE — grafana

## Kontext
- Modul/Ordner: `grafana`
- Bereitstellung des vollständigen Observability-Stacks für ThemisDB.

## Komponenten und Datenfluss

```
ThemisDB Process
│  ├── HTTP API          :8080   → client requests
│  ├── Health endpoint   :8081   → Kubernetes liveness / readiness probes
│  └── Prometheus metrics:9091   ← scraped by Prometheus
│       (OTLP spans also pushed to OTel Collector)
│
│  OTLP gRPC/HTTP push
│  └──────────────────► OTel Collector  :4319 (gRPC) / :4320 (HTTP)
│                            │
│                 ┌──────────┴──────────┐
│                 ▼                     ▼
│            Jaeger :4317          Prometheus exporter
│            (trace storage)        :8889 (span RED metrics)
│                 │
│                 └──────────────► Grafana Explore (trace view)
│
│  Prometheus :9090
│  ├── scrapes ThemisDB      :9091
│  ├── scrapes OTel self     :8888
│  ├── scrapes OTel spans    :8889
│  ├── scrapes Grafana       :3000
│  ├── evaluates alert rules  (alerts/*.yml)
│  ├── evaluates recording rules (alerts/recording_rules.yml)
│  └── sends firing alerts ──► Alertmanager :9093
│
│  Alertmanager :9093
│  ├── routes critical  ──► PagerDuty
│  ├── routes warning   ──► Slack #ops-warnings
│  ├── routes security  ──► Slack #security-alerts + Email
│  └── routes all       ──► Slack #themisdb-alerts (default)
│
│  Grafana :3000
│  ├── datasource: Prometheus  http://prometheus:9090
│  ├── datasource: Jaeger      http://jaeger:16686
│  ├── provisioning: dashboards/llm/
│  ├── provisioning: dashboards/performance/
│  ├── provisioning: dashboards/security/
│  ├── provisioning: dashboards/operations/
│  ├── provisioning: dashboards/compliance/
│  ├── provisioning: dashboards/plugins/
│  └── provisioning: dashboards/bt4/
```

## Verzeichnisstruktur

```
grafana/
├── dashboards/                        # Dashboard JSONs (thematisch strukturiert)
│   ├── llm/                           # LLM / GPU / Cache / Scheduler
│   ├── performance/                   # Query, Timeseries, Cycle metrics
│   ├── security/                      # SIEM, Authentication
│   ├── operations/                    # System, Replication, Sharding, Storage, Gossip
│   ├── compliance/                    # SLA, Advanced features
│   ├── plugins/                       # Plugin dashboards
│   └── bt4/                           # BT-4/FLARE dashboards
├── alerts/                            # Prometheus alert + recording rules (canonical)
│   ├── llm_alerts.yml                 # LLM inference alerts
│   ├── recording_rules.yml            # Pre-computed recording rules
│   ├── siem_security_alerts.yaml      # SIEM / authentication alerts
│   ├── graph_security.yaml            # Graph traversal security alerts
│   ├── performance_regression_alerts.yaml  # Performance regression alerts
│   └── bt4_flare_alerts.yml           # BT4/FLARE bridge alerts
├── provisioning/
│   ├── datasources/
│   │   ├── prometheus.yml             # Prometheus datasource
│   │   └── jaeger.yml                 # Jaeger datasource
│   ├── dashboards/
│   │   └── dashboards.yml             # One provider per dashboard category
│   └── alerts.yml                     # Grafana contact-point loader (not Prometheus rules)
├── alertmanager.yml                   # Alertmanager routing config template
├── prometheus.yml                     # Prometheus scrape + rule_files config
├── otel-collector-config.yml          # OpenTelemetry Collector pipeline config
├── docker-compose.yml                 # Local development stack
└── compliance_exporter.py             # Compliance report generator
```

## Helm Integration

The Helm chart (`helm/themisdb`) ships `templates/grafana-dashboards.yaml` which renders
dashboard JSONs as Kubernetes ConfigMaps when `grafana.dashboards.enabled=true`.
The Grafana sidecar container (`grafana-sc-dashboard`) watches for ConfigMaps with label
`grafana_dashboard: "1"` and imports them without restarting Grafana.

## Schnittstellen
- **Nach außen**: Grafana UI `:3000`, Prometheus API `:9090`, Jaeger UI `:16686`, Alertmanager UI `:9093`
- **Nach innen**: Prometheus scrapes ThemisDB `:9091`; OTel Collector receives OTLP spans from ThemisDB

## Nicht-Ziele
- Keine Duplizierung von Alert-Regeln zwischen `provisioning/alerts.yml` und `alerts/` — `provisioning/alerts.yml` ist ausschließlich Grafana-Kontaktpunkt-Loader.
- Keine hardcodierten Credentials in Konfigurationsdateien.

