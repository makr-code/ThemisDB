> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Historical validation stamp: 2026-08-17; changelog entries are not test evidence -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - GPU Module

All notable changes to the GPU module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- GPU documentation audited against public declarations, implementation, test CMake, benchmark sources, and CMake registration; corrected public header paths, allocation/fallback claims, benchmark mapping, and historical test evidence.

## [1.8.0] - 2026-03-21

### Added
- Public header documentation for GPU interfaces with structured usage and reference coverage.

## [1.7.0] - 2026-03-09

### Added
- WASM kernel sandbox, time-slicing isolation, unified memory support, and advanced backend feature surfaces.

## [1.6.0] - 2026-02-15

### Added
- Graph-capture integration, expanded stream support, profiling markers, and multi-node topology coordination.

## [1.5.0] - 2026-01-10

### Added
- ROCm backend parity, memory defragmentation, and accelerated ANN infrastructure.

## [1.4.0] - 2025-12-01

### Added
- stream manager, query accelerator, tensor buffer, training loop, and multi-GPU balancing workflows.

## [1.0.0] - 2024-01-01

### Added
- foundational GPU policy, device discovery, allocation, safety, logging, and integration facade paths.