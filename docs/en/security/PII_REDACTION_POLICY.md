# PII Redaction Policy for Logs, Traces, and Metrics

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Security Engineers, Backend Engineers, Compliance Officers

---

## Table of Contents

1. [What is PII](#what-is-pii)
2. [Redaction Policy](#redaction-policy)
3. [Architecture](#architecture)
4. [Configuration](#configuration)
5. [Extending](#extending)
6. [CI Enforcement](#ci-enforcement)
7. [Developer Checklist](#developer-checklist)

---

## What is PII

Personally Identifiable Information (PII) is any data that can be used, alone
or in combination, to identify, contact, or locate an individual.

### PII Categories (classified by ThemisDB)

| Category        | Examples                                | Redaction Mode |
|-----------------|-----------------------------------------|----------------|
| **Email**       | `alice@example.com`                     | Partial        |
| **Phone**       | `+49-123-456789`, `(555) 123-4567`      | Partial        |
| **SSN**         | `123-45-6789`                           | Strict         |
| **Credit Card** | `4242-4242-4242-4242`                   | Strict         |
| **IBAN**        | `DE89370400440532013000`                | Strict         |
| **IP Address**  | `192.168.1.42` (when user-linked)       | Partial        |
| **URL**         | URLs containing user credentials/tokens | Strict         |

### What is **not** PII in ThemisDB telemetry

- Database operation types (`tsstore_write`, `query_range`, …)
- Shard identifiers and region codes
- Latency values and performance counters
- Error codes and status labels
- Build versions and feature flags

---

## Redaction Policy

### Channels

Redaction is **enforced automatically at the infrastructure level** – no
call-site changes are needed for new code that writes to any of the three
channels.

| Channel    | Enforcement mechanism                                         | Where applied                                |
|------------|---------------------------------------------------------------|----------------------------------------------|
| **Logs**   | `PIIRedactingSink` wraps every spdlog sink installed via `Logger::init()`. Every formatted log message passes through `PIIRedactionPolicy::redactForLog()` before reaching the console or file sink. | `include/utils/pii_redacting_sink.h`, `src/utils/logger.cpp` |
| **Traces** | `Tracer::Span::setAttribute(key, string)` and `Tracer::Span::recordError(string)` route string values through `PIIRedactionPolicy` before forwarding to OpenTelemetry. | `src/utils/tracing.cpp` |
| **Metrics**| `MetricsCollector::makeKey()` runs every label map through `PIIRedactionPolicy::redactLabels()` before storing or exporting via Prometheus. | `src/observability/metrics_collector.cpp` |

> **Manual calls are still available** via `PIIRedactionPolicy::get()` for
> cases where structured data must be redacted before being embedded in a
> format string (so that the already-redacted value is passed to the log
> call), or for code that bypasses the `Logger` class (e.g., bare
> `spdlog::info()` calls).

### Redaction Modes

| Mode       | Behaviour                                              | Example Input            | Example Output           |
|------------|--------------------------------------------------------|--------------------------|--------------------------|
| `partial`  | Mask most of the value, preserve a recognisable suffix | `alice@example.com`      | `a***@example.com`       |
| `strict`   | Replace the entire value with `****`                   | `123-45-6789`            | `***-**-6789` ¹          |

¹ The exact mask characters are controlled per-pattern in `pii_patterns.yaml`.

**Strict mode override:** Set the environment variable `THEMIS_PII_STRICT=1`
before starting the server to force strict redaction for *all* PII types,
regardless of per-pattern configuration.

---

## Architecture

```
┌──────────────────────────────────────────────┐
│           Application / Handler code          │
│  (any subsystem writing to logs/traces/metrics)│
└───────┬─────────────┬──────────────┬─────────┘
        │ THEMIS_INFO │ setAttribute │ recordQuery
        ▼             ▼              ▼
┌───────────┐  ┌─────────────┐  ┌──────────────┐
│  Logger   │  │  Tracer::   │  │  Metrics     │
│  (spdlog) │  │  Span       │  │  Collector   │
└─────┬─────┘  └──────┬──────┘  └──────┬───────┘
      │                │                │
      ▼ auto           ▼ auto           ▼ auto
┌──────────────────────────────────────────────┐
│         PIIRedactionPolicy  (singleton)        │
│  ┌────────────────────────────────────────┐  │
│  │  redactForLog(msg) → safe_msg          │  │
│  │  redactAttributes(attrs) → safe_attrs  │  │
│  │  redactLabels(labels) → safe_labels    │  │
│  └────────────────────────────────────────┘  │
│               │  uses                         │
│  ┌────────────▼────────────────────────────┐ │
│  │  PIIDetector  (plugin engine orchestrator)│ │
│  │  ├── RegexDetectionEngine  (default)    │ │
│  │  ├── NERDetectionEngine    (optional)   │ │
│  │  └── EmbeddingEngine       (optional)   │ │
│  └─────────────────────────────────────────┘ │
└──────────────────┬───────────────────────────┘
                   │  safe output
                   ▼
   ┌──────────────────┬────────────┬────────────┐
   │  PIIRedacting    │  OTel SDK  │  Prometheus │
   │  Sink → spdlog   │  (Tracer)  │  endpoint  │
   └──────────────────┴────────────┴────────────┘
```

Redaction is applied **inside the infrastructure layer**, not by every
call-site.  This makes it impossible for new code to accidentally bypass
the policy by simply calling `THEMIS_INFO`, `span.setAttribute`, or
`MetricsCollector` — all three paths automatically redact before writing.

The `PIIRedactingSink` wraps every real spdlog sink (console, file) so that
every formatted log message is scanned and masked before being written.
A thread-local re-entrancy guard ensures the sink itself does not trigger
infinite recursion if `PIIRedactionPolicy` emits diagnostic messages during
lazy initialisation.

---

## Configuration

### Default config file: `config/pii_patterns.yaml`

The YAML file controls:

- Which detection engines are active (`regex`, `ner`, `embedding`)
- Per-pattern redaction mode (`strict` / `partial`)
- Confidence thresholds
- Field-name hints (used for span attribute key classification)

```yaml
version: "1.0"
detection_engines:
  - type: "regex"
    enabled: true
    settings:
      default_redaction_mode: "strict"
    patterns:
      - name: EMAIL
        regex: '[a-zA-Z0-9._%+\-]+@[a-zA-Z0-9.\-]+\.[a-zA-Z]{2,}'
        confidence: 0.95
        redaction_mode: "partial"
        field_hints: ["email", "e_mail", "user_email"]
```

### Environment variables

| Variable            | Default | Effect                                    |
|---------------------|---------|-------------------------------------------|
| `THEMIS_PII_STRICT` | `0`     | Set to `1` to force strict mode globally  |

### Runtime reload

```cpp
// Reload after rotating the YAML file without restarting:
bool ok = PIIRedactionPolicy::get().reload(); // uses constructor path
bool ok = PIIRedactionPolicy::get().reload("/etc/themisdb/pii_patterns.yaml");
```

---

## Extending

### Adding a new PII pattern (regex engine)

1. Add a new entry to the `patterns` list in `config/pii_patterns.yaml`:

```yaml
- name: PASSPORT
  description: "Passport number (DE format)"
  regex: '[A-Z]{1}[0-9]{7}'
  confidence: 0.85
  redaction_mode: "strict"
  field_hints: ["passport", "passport_number"]
  enabled: true
```

2. Add the corresponding enum value to `PIIType` in
   `include/utils/pii_detection_engine.h`.

3. Add a `toString` / `fromString` mapping in `PIITypeUtils`.

4. Add a `maskValue` branch in `src/utils/pii_detection_engine.cpp`.

5. Update this documentation and add a test case in
   `tests/test_pii_redaction_policy.cpp`.

### Adding a new detection engine

Implement the `IPIIDetectionEngine` interface
(`include/utils/pii_detection_engine.h`) and register the factory method in
`PIIDetectionEngineFactory::createUnsigned`.

---

## CI Enforcement

The CI pipeline runs three automated checks on every push and pull request
that touches `src/**`, `include/**`, `tests/**`, or `config/pii_patterns.yaml`:

### 1. PII Leakage Static Lint (`pii-leakage-lint`)

Script: `.github/scripts/check_pii_leakage.py`

Scans C++ source files for telemetry writes (`THEMIS_INFO`, `spdlog::info`,
`span.setAttribute`, `MetricsCollector`, …) where the argument name contains a
PII field-name keyword (`email`, `phone`, `ssn`, …) **without** routing through
`PIIRedactionPolicy`.

**To suppress a false positive**, add the annotation comment `// NOPII` on the
same line with a brief justification:

```cpp
// OK: user_email is already a pseudonym UUID at this point. // NOPII
THEMIS_INFO("Audit: user_email={}", user_email);
```

### 2. PII Pattern Config Validation (`pii-pattern-config-validation`)

Parses `config/pii_patterns.yaml` with PyYAML and validates the required
schema fields are present.  Fails if the YAML is malformed.

### 3. Documentation Presence Check (`pii-docs-check`)

Fails the build if `docs/en/security/PII_REDACTION_POLICY.md` is missing or
does not contain the required sections.

---

## Developer Checklist

Before merging code that handles user data in logs, traces, or metrics:

- [ ] **New code using `THEMIS_INFO` / `THEMIS_ERROR` / spdlog**: No action
      needed if using the `Logger` class — `PIIRedactingSink` redacts
      automatically.  If using bare `spdlog::info()` (bypassing `Logger`),
      pre-redact with `PIIRedactionPolicy::get().redactForLog(value)`.
- [ ] **New trace span attributes**: No action needed for `Tracer::Span` or
      `TracedSpan` / `ScopedSpan` — `setAttribute(string)` and `recordError`
      redact automatically.
- [ ] **New metric label values**: No action needed for `MetricsCollector` —
      `makeKey()` runs `redactLabels()` automatically.
- [ ] New PII field names / patterns are added to `pii_patterns.yaml`
      `field_hints` so that key-based redaction in attributes and labels
      also covers the new type.
- [ ] A test case covers the new pattern in `tests/test_pii_redaction_policy.cpp`.
- [ ] The CI `pii-redaction-check` workflow passes on the PR branch.
- [ ] Any `// NOPII` suppressions include a clear justification comment.
