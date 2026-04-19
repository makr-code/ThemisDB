<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Toolbox Module (Public Headers)

All notable changes to the Toolbox module public headers are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/toolbox/CHANGELOG.md`.

## [Unreleased]
- DSL builder enhancements for composable toolbox pipelines (Target: Q3 2026)

## [1.0.0] — 2024-01-01

### Added
- `content_toolbox_bridge.h`: `IContentToolboxBridge` — bridge interface between content pipeline
  and toolbox execution
- `ingestion_toolbox.h`: `IIngestionToolbox` — ingestion pipeline toolbox (extract/transform/load)
- `toolbox_builder.h`: `IToolboxBuilder` — DSL builder for assembling toolbox pipelines
