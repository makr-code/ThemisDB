<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Projects Module (Implementation)

All notable implementation-level changes to the Projects module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For public API changes see `../../include/projects/CHANGELOG.md`.

## [Unreleased]
- Real-time collaboration session management improvements (Target: Q3 2026)

## [1.0.0] — 2024-01-01

### Added
- `project_lifecycle.cpp`: Project lifecycle state machine (draft / active / archived / deleted)
- `project_versioning.cpp`: Version control and snapshot management for project documents
- `project_template.cpp`: Project template instantiation and schema initialization
- `project_diff.cpp`: Structural diff and delta computation for project versions
- `collaboration_manager.cpp`: Real-time collaboration session management
