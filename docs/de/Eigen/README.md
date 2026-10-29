# Eigen Module

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: ../../../include/Eigen/README.md -->

## Overview

The `Eigen` module is the vendored linear algebra integration used by ThemisDB for matrix and vector operations, especially in analytics, embedding, and numerical processing paths. It is treated as a dependency-facing integration module rather than a custom application subsystem.

## Primary sources

- `include/Eigen/` — vendored Eigen headers and library integration layer
- upstream Eigen source tree integrated through the project include path

## Key responsibilities

- dense and sparse numerical linear algebra support
- matrix/vector operations for analytics and model-related workflows
- numerical primitives used by downstream modules when operations require high-efficiency linear algebra

## Related documentation

- [include/Eigen/README.md](../../../include/Eigen/README.md)

## Validation

This landing page closes the remaining `src/include ↔ docs/de` alignment gap for the vendored Eigen dependency module.
