> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Scraper Module

> Report vulnerabilities via the project-level [SECURITY.md](../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| SSRF — requests to internal services via injected URLs | `UrlPolicy` silently rejects any URL that does not have an `http` or `https` scheme; loopback / private IP filtering planned for v1.1.0 |
| Prompt injection via scraped page content | Scraped content passed to `IScraperLLMEvaluator` only for quality scoring; it is not used as a system prompt; LLM output is a numeric score only |
| Denial of Service via oversized pages | `crawl_options.max_pages` hard cap (default 500); individual page size limits are a Phase-2 item |
| Malicious JavaScript execution | JS rendering is performed in an external subprocess (`SubprocessJSRenderer`); no in-process JS engine |
| Provenance spoofing — downstream consumers ignoring mandatory fields | `ScraperRecordBuilder` unconditionally sets `is_scraper_ingested`, `ingestion_source_type`, and `ingestion_plugin_version` before every write; documented as an immutable contract |
| Log injection via scraped content | URL and source names logged only; raw page HTML is not written to logs |
| Config injection via YAML | `ScraperConfig::loadFromYaml()` throws `std::runtime_error` on parse error; no partial state applied |
| Unvalidated subprocess path (JS renderer) | `SubprocessJSRenderer::isAvailable()` must be called before use; subprocess binary path is validated at construction |

## Security Controls

- **SSRF guard:** `UrlPolicy` rejects non-http/https schemes before any network
  request is made. The rejected URL is counted as visited but no connection attempt
  is started.
- **Polite crawling defaults:** `crawl_options.respect_robots = true` and
  `crawl_options.request_delay_ms = 250` are the out-of-the-box settings,
  reducing the risk of unintentional server overload.
- **Dependency injection:** all external dependencies (HTTP, HTML parser, LLM,
  DB writer, JS renderer) are injected via interfaces. This limits the attack
  surface in production and enables controlled security testing.
- **Subprocess isolation:** headless-browser rendering runs in a separate process.
  A crash or exploit in the browser subprocess does not affect the ThemisDB process.
- **Error isolation:** per-URL failures (network errors, JS renderer failures,
  write errors) are caught, logged, and counted. They do not abort the run or
  propagate exceptions to the caller of `ScraperPlugin::scrape()`.
- **Compile-time feature isolation:** `THEMIS_ENABLE_CURL`, `THEMIS_ENABLE_PUGIXML`,
  and `THEMIS_ENABLE_LLM` allow each external dependency to be excluded at build
  time, reducing the binary attack surface in constrained deployments.

## Known Limitations

- `robots.txt` parsing is best-effort (URL prefix match only). Full RFC 9309
  compliance (wildcard patterns, `Crawl-delay`, `Allow:` overrides) is not yet
  implemented. See `AUDIT.md` finding SCR-SEC-01.
- `UrlPolicy` does not yet block crawls to loopback addresses (`127.0.0.1`,
  `::1`) or RFC 1918 private IP ranges. This is a Phase-2 hardening item
  (Target: v1.1.0).
- `ScraperLLMEvaluator` heuristic fallback uses keyword frequency and may produce
  inaccurate scores; inaccurate scores alone do not constitute a security risk but
  may affect data quality.

## Dependency Security

- **libcurl** (`THEMIS_ENABLE_CURL`) — HTTP client; TLS certificate validation
  is enabled by default. Pin `CURLOPT_SSL_VERIFYPEER` and `CURLOPT_SSL_VERIFYHOST`
  in production configurations.
- **pugixml** (`THEMIS_ENABLE_PUGIXML`) — HTML/XML parser; operates on memory
  buffers. No file system access from the parser path.
- **LLM service** (`THEMIS_ENABLE_LLM`) — external HTTP endpoint; TLS
  verification required; endpoint must be an internal or trusted service.
- Internal: `spdlog` for logging; no known CVEs affecting current usage.
