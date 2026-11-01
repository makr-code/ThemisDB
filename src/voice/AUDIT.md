---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Voice Documentation Source Review

## Scope and method

On 2026-10-31, existing Markdown files directly under `src/voice/`,
`include/voice/`, `tests/voice/`, and `benchmarks/voice/` were inventoried and
compared with current in-tree voice headers/implementations, CMake files, and
the repository roadmap. No file outside those voice Markdown paths was
modified. No build, test, benchmark, or linter was run.

The wiki status says its full sync was generated 2026-10-31 11:12 UTC; it is
operational context only and does not override source, headers, or root
governance. Its delta report lists only `UPDATES.md` as changed. Voice-specific
behavior in this review was checked against current code rather than inferred
from the wiki.

## Static source findings

- There are 23 public headers under `include/voice/`.
- `THEMIS_ENABLE_VOICE_ASSISTANT` is an opt-in CMake feature (default `OFF`);
  `THEMIS_ENABLE_RNNOISE` is also optional.
- `tests/CMakeLists.txt` auto-discovers the module directory. The voice
  `CMakeLists.txt` globs `test_*.cpp`; only its explicit label patterns receive
  `wave_a`/`release_critical`.
- `benchmarks/CMakeLists.txt` conditionally adds the voice module; its
  `CMakeLists.txt` globs `bench_*.cpp` when benchmarks are enabled.
- Browser streaming without injected processing callbacks has placeholder
  transcript/TTS behavior. Session/auth/storage/security helpers do not by
  themselves establish durable storage, cryptographic protection, or
  application authorization.
- Registration establishes neither compilation nor successful execution.
  Representative-hardware voice baselines remain an open item in the root
  roadmap.

## Remaining uncertainty

This static documentation pass is not runtime verification. Current CI run
results, deployment configuration, backend availability, measured performance,
security effectiveness, and compliance posture were not independently tested.
See [`ROADMAP.md`](ROADMAP.md) and root [`../../ROADMAP.md`](../../ROADMAP.md)
for the status authority.
