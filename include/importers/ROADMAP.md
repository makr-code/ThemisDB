<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/importers/ROADMAP.md -->

# Roadmap — Importers Module (Public Headers)

> Implementation roadmap: `../../src/importers/ROADMAP.md`

## Current Status

v2.2.0 — Production-ready. 35 public headers. 9 source connectors, MDM engine, blockchain integrity, federated learning, GUI wizard, OZG service registry, XÖV importer.

## Completed ✅

- [x] `IImporter` base interface and plugin API
- [x] PostgreSQL, MySQL, Oracle, SQLite, Kafka, MongoDB, S3, flat file, GraphQL importers
- [x] PostgreSQL CDC (logical replication)
- [x] MDM engine with entity resolution, deduplication, golden record
- [x] CRDT conflict resolution, canonical resolver, entity linker/matcher
- [x] Schema inference and validation
- [x] Blockchain-anchored integrity (stub Ethereum anchor)
- [x] Federated learning data partitioning (FedAvg; FedProx stub)
- [x] GUI import wizard interface
- [x] `ozg_service_registry.h` — OZG 2.0 service catalog registry (`IOZGServiceRegistry`, `InMemoryOZGServiceRegistry`)
- [x] `xoev_importer.h` — XÖV data model importer/exporter (`IXOEVImporter`, `InMemoryXOEVImporter`)

## Planned

- [ ] Microsoft SQL Server importer (Issue #1845) (Target: v2.2.0)
- [ ] FedProx aggregation production implementation (Target: v2.2.0)
- [ ] Ethereum smart contract production anchor (Target: v2.2.0)
- [ ] Quantum-safe audit trail signatures (NIST PQC CRYSTALS-Kyber) (Target: v3.0.0)
- [ ] Kafka consumer group rebalance-aware import (Target: v2.2.0)

## Production Readiness Checklist

- [x] 9 source connectors stable
- [x] MDM golden record pipeline complete
- [x] Audit trail for all import operations
- [ ] Microsoft SQL Server connector
- [ ] FedProx production algorithm
- [ ] Quantum-safe audit signatures
