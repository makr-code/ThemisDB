---
Author: "ThemisDB Team"
Created: "2026-11-09"
Last Updated: "2026-11-09"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Marker locations — 2026-11-09 source snapshot

## Reproducibility

- Source level/domain: tracked C/C++ source under `src/`; audit evidence, not release-status authority.
- Branch: `develop`
- Commit: `7cb488830b93a9b08b2a3da366087d80c7c496f1`
- Scan timestamp: `2026-11-09T09:35:02Z`
- Tool: Git `grep` (case-sensitive, tracked files at the recorded commit)
- Command:

```sh
git grep -n -I -E 'TODO|STUB|MOCK|FIXME' 7cb488830b93a9b08b2a3da366087d80c7c496f1 -- \
  'src/*.cpp' 'src/**/*.cpp' 'src/*.cc' 'src/**/*.cc' 'src/*.c' 'src/**/*.c' \
  'src/*.h' 'src/**/*.h' 'src/*.hpp' 'src/**/*.hpp'
```

The command output is the complete, reproducible location inventory for this scope. It includes marker text in comments, string literals, and test files nested under `src/`; it does not determine whether a runtime path is production-reachable.

## Scope and exclusions

- Included: tracked `src/` files matching `.cpp`, `.cc`, `.c`, `.h`, or `.hpp`.
- Excluded: untracked/ignored files; `tests/` outside `src/`; `include/`; generated and vendored sources; other suffixes such as `.cxx`, `.hxx`, `.ipp`, `.tpp`, `.cu`, and `.cuh`; and markers with different case.
- `STUB`/`MOCK` hits are raw text matches, not verified gap or severity counts. The historical October 1 inventory used a working-tree recursive `grep`; this snapshot uses a commit-pinned tracked-file scan for reproducibility.

## Raw results

- Marker-bearing lines: **131** across **51** source files
- `STUB`: **128**
- `MOCK`: **3**
- `TODO`: **0**
- `FIXME`: **0**

| Module | Raw marker lines |
|---|---:|
| `security` | 46 |
| `llama_cpp` | 19 |
| `llm` | 11 |
| `acceleration` | 9 |
| `analytics` | 7 |
| `ingestion` | 6 |
| `storage` | 5 |
| `tensor` | 4 |
| `training` | 4 |
| `voice` | 4 |
| `geo` | 3 |
| `performance` | 3 |
| `index` | 2 |
| `transaction` | 2 |
| `(root)` | 1 |
| `api` | 1 |
| `ethics_ai` | 1 |
| `plugins` | 1 |
| `process` | 1 |
| `server` | 1 |

This is a fresh raw inventory only. See [`MARKER_GAP_CLASSIFICATION_2026-11-09.md`](MARKER_GAP_CLASSIFICATION_2026-11-09.md) for the source-backed triage performed so far and the remaining unclassified scope.
