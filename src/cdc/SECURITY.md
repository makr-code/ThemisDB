# Security - CDC Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the cdc module focuses on safe event delivery boundaries, reliable acknowledgement/replay controls, bounded dead-letter handling, and resilient behavior under degraded transport backends.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unauthorized or unsafe stream delivery behavior | explicit transport and admin control surfaces |
| delivery duplication/loss edge risks | delivery tracker and acknowledgement/replay controls |
| dead-letter overflow or unbounded failure replay | dedicated DLQ paths with bounded operational controls |
| backend transport outage impact | structured degradation behavior for transport integrations |
| operational blind spots in stream lag/failures | CDC admin and monitoring-oriented runtime diagnostics |

## Implemented Security Controls

- delivery and replay paths expose explicit control semantics.
- dead-letter and outbox behavior is separated into dedicated reliability surfaces.
- transport integrations fail with observable runtime signals.
- administration surfaces centralize CDC control and diagnostics behavior.

## Security Follow-ups

- continue hardening transport/backpressure failure-path consistency.
- maintain deterministic replay behavior under degraded dependencies.
- keep diagnostics actionable for stream integrity and delivery incidents.

## Sourcecode Verification (Module: cdc/security)

- Verified files:
  - src/cdc/changefeed.cpp
  - src/cdc/delivery_tracker.cpp
  - src/cdc/dead_letter_queue.cpp
  - src/cdc/consumer_group.cpp
  - src/cdc/ws_transport.cpp
  - src/cdc/kafka_cdc_producer.cpp
  - src/cdc/cdc_admin.cpp
- Verified controls:
  - bounded delivery/replay and DLQ/outbox reliability surfaces
  - controlled transport degradation behavior
  - observable operational and admin control paths