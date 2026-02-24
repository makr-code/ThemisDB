# pre-commit Badge

[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen)](https://pre-commit.com/)

## What it shows

A static badge indicating that ThemisDB uses [pre-commit](https://pre-commit.com/) to enforce code style and basic correctness checks automatically before every commit. Enabled hooks include clang-format, trailing whitespace, YAML/JSON linting, and more.

## What it does NOT guarantee

- pre-commit hooks run locally on a contributor's machine only when installed (`pre-commit install`). The badge signals the project's intent, not enforcement for every commit in the repository.

## Source of truth

| Source | URL |
|--------|-----|
| Hook configuration | [`.pre-commit-config.yaml`](../../../.pre-commit-config.yaml) in the repository root |
| pre-commit documentation | <https://pre-commit.com/> |

## How contributors can verify

```bash
pip install pre-commit
pre-commit install
pre-commit run --all-files
```
