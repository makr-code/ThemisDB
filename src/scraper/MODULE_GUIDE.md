---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Scraper Developer Guide

**Purpose and audience:** A first-change guide for developers integrating or modifying the scraper module.

## Locate the contract

- Public C++ API lives in [`include/scraper/`](../../include/scraper/); start with [`scraper_plugin.h`](../../include/scraper/scraper_plugin.h) and [`scraper_config.h`](../../include/scraper/scraper_config.h).
- Implementations live in this directory. The flow and failure boundaries are summarized in [ARCHITECTURE.md](./ARCHITECTURE.md) and [IMPLEMENTATION.md](./IMPLEMENTATION.md).
- Focused unit tests live in [`tests/scraper/`](../../tests/scraper/). The local CMake file creates one focused target/test per `test_scraper_*.cpp`.

## Configure a controlled run

Create a `ScraperConfig` with explicit `seed_urls`, a narrow whitelist, a blacklist for known out-of-scope paths, and an explicit gap context. For HTML direct-page ingestion set `search_options.enabled` to `false`; otherwise the plugin searches the first discovered form and may return no documents when no supported form is found. Choose `STATIC`, `JS_RENDERED`, or API mode deliberately and verify optional dependencies in the actual build.

Do not infer that every declared crawl option is enforced. In particular, `respect_robots`, `same_domain_only`, `max_depth`, and crawl `max_pages` are parsed but are not consumed by the orchestration loop. The standalone robots and sitemap helpers are not automatically wired into `ScraperPlugin`.

## Implement a change

1. Identify the owning public interface and the implementation path; keep additions behind existing injected interfaces when practical.
2. Preserve URL-policy checks, bounded result/API pagination, explicit evaluator decisions, and the mandatory provenance fields in every writer path.
3. Use injected HTTP/API/search/renderer/evaluator/writer dependencies for deterministic tests; avoid live network dependencies in unit tests.
4. Cover empty and malformed inputs, unavailable optional backends, pagination stop conditions, and write failures relevant to the changed path.
5. Check documentation consistency across [API.md](./API.md), [TRUST.md](./TRUST.md), [RUNBOOK.md](./RUNBOOK.md), and [OPERATIONS.md](./OPERATIONS.md).

## Verify

The requested documentation-gate command is:

```bash
python -m unittest discover -s tests -p "test_doc_metadata_gate.py"
```

For code changes, build the existing focused scraper target in the configured CMake build and run the matching CTest test (for example, select tests with `ctest -R scraper`). The test registration and target naming are generated from `tests/scraper/CMakeLists.txt`; use the active build preset rather than inventing a new build configuration.

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
