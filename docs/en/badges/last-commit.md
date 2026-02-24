# Last Commit Badge

[![Last Commit](https://img.shields.io/github/last-commit/makr-code/ThemisDB/develop)](https://github.com/makr-code/ThemisDB/commits/develop)

## What it shows

The date of the most recent commit on the `develop` branch. This gives contributors and evaluators a quick signal of how actively the project is maintained.

## What it does NOT guarantee

- The badge reflects activity on `develop` only, not on feature branches or `main`.
- A recent commit does not imply a production-ready state; `develop` is the integration branch.

## Source of truth

| Source | URL |
|--------|-----|
| GitHub commit history (develop) | <https://github.com/makr-code/ThemisDB/commits/develop> |

## How contributors can verify

```bash
git log -1 --format="%cd" --date=short origin/develop
```
