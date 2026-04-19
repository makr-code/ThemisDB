<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Toolbox Module (Implementation)

All notable implementation-level changes to the Toolbox module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For public API changes see `../../include/toolbox/CHANGELOG.md`.

## [Unreleased]
- Extended pipeline step library for content and ingestion toolboxes (Target: Q3 2026)

## [1.0.0] — 2024-01-01

### Added
- `content_toolbox_bridge.cpp`: Bridge between content processing pipeline and toolbox execution
- `ingestion_toolbox.cpp`: Ingestion pipeline toolbox — extraction, transformation, and loading steps
- `toolbox_builder.cpp`: DSL builder for assembling toolbox processing pipelines
