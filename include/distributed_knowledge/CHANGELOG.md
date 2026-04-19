<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Distributed Knowledge Module (Public Headers)

All notable changes to the Distributed Knowledge module public headers are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/distributed_knowledge/CHANGELOG.md`.

## [Unreleased]
- Operational resilience hardening (non-blocking publish, ZeroTrust enforcement) — Target: Q2 2026

## [1.1.0] — 2026-04-17

### Added
- `lora_federation_coordinator.h`: `ILoRAFederationCoordinator` — LoRA adapter federation with
  async `triggerAggregation(timeout_ms)` and `erase()`
- `federated_rag_merger.h`: `IFederatedRAGMerger` with `shard_timeout_ms` timed-out shard support
- `cross_shard_feedback_sync.h`: `ICrossShardFeedbackSync` with non-blocking `publishFeedback()`
  and `skipped_publish_count` telemetry
- `adapter_capability_announcement.h`: `IAdapterCapabilityAnnouncement` with `eraseCount()`

### Changed
- All four DK components now expose `erase()` / `eraseCount()` for operational cleanup

## [1.0.0] — 2024-01-01

### Added
- Initial public header interfaces for federated LoRA coordination and RAG merging
