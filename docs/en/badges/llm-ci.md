# LLM CI Badge

[![LLM CI](https://github.com/makr-code/ThemisDB/actions/workflows/02-feature-modules_llm_llm-cpu-fallback-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/02-feature-modules_llm_llm-cpu-fallback-ci.yml)

## What it shows

The result of the most recent run of the **LLM CPU Fallback CI** workflow. This workflow validates the llama.cpp-based LLM integration layer in CPU-only mode (i.e., without GPU hardware), ensuring that the AI/LLM features of ThemisDB work correctly on standard server hardware.

## What it does NOT guarantee

- A passing badge confirms CPU-mode LLM inference only. CUDA/GPU-accelerated inference is covered by the separate GPU CI workflow.
- The workflow triggers only when LLM-related source files change.

## Source of truth

| Source | URL |
|--------|-----|
| Workflow file | [`.github/workflows/02-feature-modules_llm_llm-cpu-fallback-ci.yml`](../../../.github/workflows/02-feature-modules_llm_llm-cpu-fallback-ci.yml) |
| All workflow runs | <https://github.com/makr-code/ThemisDB/actions/workflows/02-feature-modules_llm_llm-cpu-fallback-ci.yml> |

## How contributors can verify

1. Go to the [Actions tab](https://github.com/makr-code/ThemisDB/actions) on GitHub.
2. Select the **LLM CPU Fallback CI** workflow.
3. Review the most recent run for detailed step-by-step logs.
