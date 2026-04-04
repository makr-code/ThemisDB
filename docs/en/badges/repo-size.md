# Repo Size Badge

[![Repo Size](https://img.shields.io/github/repo-size/makr-code/ThemisDB)](https://github.com/makr-code/ThemisDB)

## What it shows

The total size of the ThemisDB repository as reported by the GitHub API. This includes all tracked source files, documentation, configuration, and submodule metadata (but **not** untracked build artifacts or dependencies installed locally).

## What it does NOT guarantee

- Large binary blobs committed historically inflate this number even if removed in later commits.
- The badge is served by shields.io and may be cached for a short period.

## Source of truth

| Source | URL |
|--------|-----|
| GitHub API | `GET /repos/makr-code/ThemisDB` → `size` field |
| Repository root | <https://github.com/makr-code/ThemisDB> |

## How contributors can verify

```bash
git count-objects -v -H
```

Or check the repository homepage on GitHub, which shows the repository size in the sidebar.
