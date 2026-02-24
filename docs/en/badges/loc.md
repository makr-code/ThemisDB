# Lines of Code Badge

[![Lines of Code](https://tokei.rs/b1/github/makr-code/ThemisDB)](https://github.com/makr-code/ThemisDB)

## What it shows

The total number of lines of source code in the repository as counted by [Tokei](https://github.com/XAMPPRocky/tokei), a fast code statistics tool. The count covers all recognised source files across all languages used in ThemisDB (C++, CMake, Python, YAML, etc.).

## What it does NOT guarantee

- The count includes comments and blank lines unless Tokei's default filtering applies.
- Auto-generated files (e.g., protobuf outputs, vendored dependencies) may be included in the total depending on which files are tracked in the repository.
- The badge is recomputed on-demand by `tokei.rs` and reflects the state of the `main`/default branch.

## Source of truth

| Source | URL |
|--------|-----|
| Tokei badge service | <https://tokei.rs> |
| Repository root | <https://github.com/makr-code/ThemisDB> |

## How contributors can verify

If Tokei is installed locally:

```bash
tokei .
```

This prints a per-language breakdown of code, comment, and blank lines for the repository checkout.
