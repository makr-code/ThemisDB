<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Content Module Public Headers

**Module Path:** `include/content/`
**Implementation Security:** `../../src/content/SECURITY.md`

---

## Scope

Security considerations for the content processing module's public header API surface.
Covers malicious content ingestion, PII exposure, archive bomb attacks, plugin safety,
and abuse detection.

---

## Threat Model

| Threat | Vector | Mitigation Header |
|--------|--------|------------------|
| Malware upload | Executable or exploit payload in ingested content | `content_security.h` — `IContentSecurity::scan()` before persistence |
| Archive bomb (zip bomb) | Recursive archive with explosive expansion | `archive_processor.h` — `max_extracted_size` and `max_depth` limits |
| Malicious PDF exploit | Embedded JavaScript / RCE in PDF | `pdf_processor.h` — extraction-only mode; no JavaScript execution |
| MIME type confusion | Content disguised as safe type | `mime_detector.h` — magic-byte detection, not filename extension |
| PII in content stream | Unredacted PII persisted in content store | `content_policy.h` — PII detection hook before storage (redaction planned in `IPIIRedactor` Q3 2026) |
| Abuse content upload | CSAM or hate speech in image/video | `abuse_detector.h` — `IAbuseDetector::classify()` on all media |
| Plugin code execution | Malicious ingestion plugin loaded | `ingestion_plugin.h` — `PluginManifest` requires signed manifest |
| Deduplication oracle attack | Timing-based content inference via dedup | `deduplication_checker.h` — constant-time hash comparison |
| Language detection exfiltration | Content structure reveals user language | `language_detector.h` — language result is metadata-only; no content retained |
| STT audio PII | Personal data in transcribed audio | `stt_processor.h` — transcription result includes PII field markers |

---

## Security Controls

### Content Security Scanning
`IContentSecurity::scan(content)` runs before any content is written to the storage layer.
Results include `ThreatResult::CLEAN`, `SUSPICIOUS`, or `MALICIOUS`; malicious content is
rejected and logged.

### Archive Bomb Protection
`IArchiveProcessor` enforces `max_extracted_size_bytes` and `max_nesting_depth` from
`archive_processor.h` config; extraction stops and returns `ContentErrorCode::ARCHIVE_BOMB`
if limits are exceeded.

### MIME Type Verification
`IMIMEDetector::detect()` uses magic-byte analysis, not filename extensions. MIME type
mismatches between claimed and detected types are flagged as `SUSPICIOUS`.

### Plugin Manifest Signing
`IIngestionPlugin` requires a `PluginManifest` with a cryptographic signature; unsigned
plugins are rejected at registration time.

### Abuse Detection
`IAbuseDetector::classify()` is a required step in the processing chain for all image,
audio, and video content; results are logged to the content audit trail.

---

## Known Limitations

- PII redaction at the content layer (`IPIIRedactor`) is planned for Q3 2026; until then,
  callers are responsible for PII handling before invoking the content pipeline.
- `mock_clip_processor.h` is a test mock in the public include path and should not be
  installed in production deployments.
- OCR-extracted text may contain sensitive data; `IOCRProcessor` results should be treated
  as potentially PII-bearing.
