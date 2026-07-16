# GPU CI Badge

[![GPU CI](https://github.com/makr-code/ThemisDB/actions/workflows/06-infrastructure_gpu_gpu-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/06-infrastructure_gpu_gpu-ci.yml)

## What it shows

The result of the most recent run of the **GPU Module CI Gate** workflow. This workflow validates GPU-accelerated code paths (HNSW vector search GPU backend, geospatial GPU extensions, and related benchmarks) whenever the corresponding sources change.

## What it does NOT guarantee

- A passing badge covers only the GPU module gate, not the entire GPU test suite (which requires dedicated GPU hardware).
- The workflow may be skipped on pushes that do not touch GPU-related paths, in which case the badge reflects the last triggered run.

## Source of truth

| Source | URL |
|--------|-----|
| Workflow file | [`.github/workflows/06-infrastructure_gpu_gpu-ci.yml`](../../../.github/workflows/06-infrastructure_gpu_gpu-ci.yml) |
| All workflow runs | <https://github.com/makr-code/ThemisDB/actions/workflows/06-infrastructure_gpu_gpu-ci.yml> |

## How contributors can verify

1. Go to the [Actions tab](https://github.com/makr-code/ThemisDB/actions) on GitHub.
2. Select the **GPU Module CI Gate** workflow.
3. Review the most recent run for detailed step-by-step logs.
