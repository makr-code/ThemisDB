# Security - Scraper Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

**Author:** ThemisDB Contributors<br>
**Created:** 2026-10-31<br>
**Last Updated:** 2026-10-31<br>
**Status:** review

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot; themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security review focuses on inputs and external fetches, URL-policy limits, renderer/LLM trust boundaries, and provenance in constructed records. The controls below describe implemented behavior, not a claim that the module is a complete network or authorization boundary.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unsafe/unapproved source traversal | HTTP(S)-only string matching in `UrlPolicy`; no redirect destination recheck or general egress control |
| malformed content / parser mismatch | parser may return empty results; no unified diagnostic event is emitted by `ScraperPlugin` |
| untrusted page content sent to an LLM | page text is included in evaluation prompt; the result is a heuristic/model score, not authorization or factual verification |
| renderer process risk | configured external command receives the URL; timeout enforcement is delegated to the renderer command |
| provenance loss in output | record builders stamp provenance fields on relational, graph, and vector records |

## Implemented Security Controls

- `UrlPolicy` permits only `http://` and `https://`, applies blacklist checks before whitelist checks, and supports simple prefix/limited `*` matching. Empty whitelist means any remaining HTTP(S) URL. The plugin checks explicit seed and HTML result URLs, but selected catalog seed URLs are added without that check and the exact API endpoint is exempted during API result filtering. These string patterns are not robust hostname validation.
- The libcurl fetch paths enable TLS peer and hostname verification and follow redirects. Redirect destinations are not rechecked by the plugin's `UrlPolicy`; apply network egress controls outside the process.
- `respect_robots`, `same_domain_only`, crawl `max_depth`, and crawl `max_pages` are parsed but not enforced in the main loop. `scraper_robots.*` and `scraper_sitemap.*` are independent helpers.
- API keys named by a catalog source's `api_key_env` are read from the environment and placed in an Authorization bearer header. Protect environment access and redact credentials.
- `ScraperPlugin` does not emit `ScraperDiagnosticEvent` events. Diagnostic types and `faultClassOf()` exist as standalone helpers; parser/fetch/evaluation outcomes are not universally reported through them.
- `ScraperRecordBuilder` stamps provenance. The vector builder leaves its embedding empty; this is not evidence of semantic indexing.

## Security Follow-ups

- Wire and verify a diagnostic sink through the plugin if end-to-end structured fault reporting is required.
- Enforce and test redirect-aware egress policy, renderer process deadlines, robots/domain rules, and crawl-wide limits before describing them as runtime controls.

## Sourcecode Verification (Module: scraper/security)

- Verified files:
  - src/scraper/scraper_config.cpp
  - src/scraper/scraper_api_client.cpp
  - src/scraper/scraper_search_engine.cpp
  - src/scraper/scraper_llm_evaluator.cpp
  - src/scraper/scraper_metadata_writer.cpp
- Verified controls:
  - HTTP(S)-only URL matching; it is not redirect-aware egress protection
  - LLM-to-heuristic fallback in the evaluator; malformed parser results may be empty
  - provenance stamps in record builders; no embedding generation