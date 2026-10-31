---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot; themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Scraper Trust Boundaries

**Purpose and audience:** Security and data-governance notes for maintainers and operators connecting the scraper to external sources, LLMs, and ThemisDB storage.

## Trust boundaries

1. **Configuration and seed/catalog input:** seed URLs, URL patterns, catalog overlays, queries, and renderer command are configuration inputs. Treat them as privileged; limit who can change them.
2. **External network and page content:** fetched HTML, JSON, redirects, links, metadata, and API responses are untrusted. Parsing or LLM evaluation does not establish factual accuracy or permission to reuse content.
3. **Renderer process:** `SubprocessJSRenderer` launches a configured external command and passes a URL to it. On Unix-like systems it builds an argv vector; the Windows path constructs a command string for `_popen`. Treat renderer command and URL handling as privileged configuration, restrict the execution environment, and never derive the renderer command from scraped content. The adapter passes the configured timeout argument but does not independently terminate a hung child.
4. **LLM boundary:** page text is included in an evaluation prompt. Treat instructions or claims in the page as untrusted data, constrain model access, and do not use model scores as authorization or legal review.
5. **Persistence and downstream consumers:** written data carries `is_scraper_ingested`, `ingestion_source_type`, and plugin-version provenance. Consumers must preserve and interpret these fields rather than treating scraped data as trusted first-party records.

## Controls present and limits

- `UrlPolicy` rejects schemes other than HTTP/HTTPS, applies blacklist before whitelist, and supports simple string-prefix or limited `*` matching. An empty whitelist permits any HTTP(S) URL. The plugin applies it to explicit seeds and HTML search-result URLs, but does not check selected catalog seed URLs; the exact API endpoint is exempted during API-result filtering. Use narrow allowlists and do not treat string prefixes as robust hostname validation.
- The libcurl paths enable TLS peer and hostname verification and follow redirects without rechecking the destination against `UrlPolicy`. A source allowlist is not a complete SSRF/effective-destination boundary; enforce egress restrictions outside the process and avoid broad or attacker-controlled seed inputs.
- Although `respect_robots` and `same_domain_only` are parsed, the main plugin orchestration does not enforce them. Standalone `RobotsTxtCache` and `SitemapCrawler` types do not change this unless an integration explicitly invokes them.
- Catalog API keys are read from the environment variable named by `api_key_env` and placed in an Authorization bearer header. Restrict environment access, redact headers, and rotate exposed keys. Do not put secret values in catalog files.
- Provenance is built into relational, graph, and vector records. The vector builder does not generate an embedding; empty vectors must not be interpreted as verified semantic evidence.

## Incident handling

For suspected unauthorized egress, stop new invocations at the caller, preserve the effective config and redacted URL/redirect evidence, and involve the security owner. For suspected credential exposure, revoke the credential at its issuer, rotate the environment value, and review access logs. For polluted/incorrect records, retain the affected source and document IDs and use the owning storage system's remediation process; the scraper API has no retraction function.

Do not include full scraped bodies, API keys, or authorization headers in incident tickets unless the approved evidence process requires them and storage is access-controlled. Continue operational triage using [RUNBOOK.md](./RUNBOOK.md).

Evidence: [`scraper_config.cpp`](./scraper_config.cpp), [`scraper_plugin.cpp`](./scraper_plugin.cpp), [`scraper_api_client.cpp`](./scraper_api_client.cpp), [`scraper_js_renderer.h`](../../include/scraper/scraper_js_renderer.h), and [`scraper_metadata_writer.h`](../../include/scraper/scraper_metadata_writer.h).

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
