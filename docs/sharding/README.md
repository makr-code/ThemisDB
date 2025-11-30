# Sharding & Horizontal Scaling Documentation

Documentation for ThemisDB's sharding and horizontal scaling capabilities.

## Contents

- **implementation_summary.md** - Sharding implementation overview
- **phase1_report.md** - Phase 1 implementation report
- **phases_1-3_summary.md** - Phases 1-3 summary
- **horizontal_scaling_strategy.md** - Horizontal scaling strategy
- **[Auto-Rebalancing Report](../reports/SHARDING_AUTO_REBALANCING.md)** - Phase 2-3: Automatic Rebalancing (Q4 2025)

## Features

✅ **Phase 1: Foundation** (Completed)
- Horizontal data partitioning
- Consistent hashing for shard assignment
- PKI-based mutual TLS authentication
- Cross-shard query execution

✅ **Phase 2-3: Auto-Rebalancing** (Completed)
- Multi-criteria load detection (Storage, Request, Latency, Resource)
- Automatic rebalancing coordination
- Safety mechanisms (Cooldown, Concurrency limits, Daily limits)
- Full observability (Prometheus + OpenTelemetry)

📋 **Phase 4: Advanced Features** (Planned Q1 2026)
- Multi-DC replication
- Geo-aware sharding
- Read replicas
- Automatic failover

## See Also

- [Architecture](../architecture/)
- [Performance](../performance/)
- [Source Code: sharding module](../src/sharding/)
