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

# Scraper Runbook

**Purpose and audience:** Triage checklist for an operator or on-call engineer investigating a scraper run. This module exposes a synchronous C++ API; no scraper-specific admin endpoint or built-in pause/cancel command is defined here.

## First response

1. Identify the invoking service/job, the exact config revision, selected seeds/source IDs, render mode, optional dependency flags, and run start/end times.
2. Capture the returned `ScraperRunStats`: compare `urls_visited`, `urls_skipped`, `docs_scraped`, `docs_accepted`, `docs_discarded`, `docs_written`, `write_errors`, and `elapsed_ms`.
3. Preserve a redacted diagnostic sample and the source identifier. Do not place API credentials, full sensitive page contents, or unredacted authorization headers in tickets or logs.
4. Since the plugin has no cancellation method, stop new invocations at the owning scheduler/orchestrator. For subsequent runs, remove or disable the affected source in the caller's config; do not assume an in-flight call can be paused.

## Triage by symptom

| Symptom | Checks | Safe response |
|---|---|---|
| No content / `urls_visited` remains low | Verify plugin initialization, configured seeds, URL scheme and whitelist/blacklist, and whether API/HTML mode matches the source. Check whether curl is enabled; without it the built-in fetch path returns an empty body. | Test one approved source with an injected or controlled fetch. Correct the config/build integration before restoring the full source set. |
| Seed fetched but no documents | In search mode, verify form discovery and result parsing; the default HTML parser requires pugixml. Check `search_options.enabled`, effective query list, result limit, and the 50-character text floor. For API mode, inspect the expected JSON results field and pagination fields. | Use a representative fixture and inspect parsed results before changing thresholds. No parsed items is not proof that the upstream source is empty. |
| Unexpected discard rate | Compare configured threshold, quality and relevance scores, discard reasons, and whether evaluation used the LLM or heuristic fallback. Inspect the first 3,000 characters supplied to the LLM path where safely available. | Re-check the gap context and source content; do not lower the threshold globally merely to mask a broken extraction path. |
| `write_errors` increases | Inspect the supplied `IScraperMetadataWriter` and the per-layer fields in `WriteResult`; verify storage health and mandatory provenance construction. | Stop adding load at the orchestrator, repair the writer/storage dependency, then run a small controlled batch. A failed writer call does not abort the scraper loop. |
| Run reports success but final writes are uncertain | `flush()` is called when the run finishes, but the current plugin ignores its boolean return value. | Verify durability using the writer/storage's own health and record checks. Do not equate method return with durable flush confirmation. |
| Seeds are skipped | Check whether a caller injected a `BurstCrawlController`; exhausted `tryAcquire()` skips the seed and increments `urls_skipped`. | Reduce the next-run seed batch or adjust the injected controller in its owning integration. There is no module admin interface to reset it while a run is in progress. |

## Recovery and rollback

- Roll back a bad source change by reverting the caller's config/catalog overlay to the last known-good revision, then launch a small run before restoring normal scheduling.
- If an API key may have been exposed, revoke/rotate it at the upstream provider and update the environment variable named by the source catalog. Do not copy the secret into YAML or diagnostics.
- If records were written from untrusted or incorrect content, use the downstream storage owner's documented correction/deletion workflow; this module exposes no record-retraction API.
- Escalate repeated extraction mismatch, redirect/policy concerns, or provenance defects to the scraper maintainer and security owner. Preserve config revision, redacted samples, error/result counts, and writer layer outcomes.

For standing configuration and capability limits see [OPERATIONS.md](./OPERATIONS.md), [TRUST.md](./TRUST.md), and [the project ingestion runbook](../../docs/operability/RUNBOOK_SCRAPER_INGESTION.md). The latter contains additional commands and telemetry assumptions that must be verified against the actual deployment before use.

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
