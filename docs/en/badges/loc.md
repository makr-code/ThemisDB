# Lines of Code Badge

[![Lines of Code](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/makr-code/ThemisDB/develop/.github/badges/lines-of-code.json&query=%24.message&label=Lines%20of%20Code&color=blue)](https://github.com/makr-code/ThemisDB)

## What it shows

The total number of lines of source code in the repository as counted by [Tokei](https://github.com/XAMPPRocky/tokei), a fast code statistics tool. The count is stored in the `.github/badges/lines-of-code.json` file on the `develop` branch and rendered as a [shields.io dynamic JSON badge](https://shields.io/badges/dynamic-json-badge).

## What it does NOT guarantee

- The count includes comments and blank lines unless Tokei's default filtering applies.
- Auto-generated files (e.g., protobuf outputs, vendored dependencies) may be included in the total depending on which files are tracked in the repository.
- The badge reflects the value stored in `.github/badges/lines-of-code.json` and must be updated manually or via a scheduled workflow whenever the codebase changes significantly.

## Source of truth

| Source | URL |
|--------|-----|
| Badge data file | [`.github/badges/lines-of-code.json`](../../../.github/badges/lines-of-code.json) on `develop` |
| Repository root | <https://github.com/makr-code/ThemisDB> |

## How contributors can verify

If Tokei is installed locally:

```bash
tokei .
```

This prints a per-language breakdown of code, comment, and blank lines for the repository checkout.
