---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Config Module Runbook

## Purpose and safety

Use this runbook to triage path-resolution, validation, watcher, metrics, audit,
and encrypted-store incidents. It covers the config module only; coordinate
changes to live application settings with the owning service. Do not disable
authorization, switch secrets to plaintext, or treat a failed resolution as
permission to continue.

## Triage by symptom

### A required config file cannot be found

1. Record the caller's requested path and the attempted paths exposed by
   `ConfigNotFoundException`; avoid including secret values in tickets or logs.
2. Confirm the deployment's `THEMIS_CONFIG_ENV` is intended (`dev`, `staging`,
   or `prod`). Unknown values default to production; production does not use an
   environment overlay.
3. Check filesystem existence, ownership, permissions, and the expected
   legacy-to-new mapping. Use resolver counters and the deprecation report to
   distinguish unmapped requests from legacy fallback.
4. Correct the path or deployment setting, then re-run resolution and validate
   the loaded config. Do not add an unreviewed compatibility mapping as an
   incident workaround.

### Validation rejects a file

1. Collect `ValidationResult.config_path`, `schema_path`, errors, and warnings.
2. Separate parse/load errors from schema mismatch; inspect local `$ref`
   targets and supported schema keywords.
3. Restore the last known-good config only through the service's approved
   rollout procedure, and validate it before applying it.
4. If the schema references an external URI or depends on an unsupported
   keyword, do not assume it was fetched or enforced; escalate to the config
   maintainer.

### Hot reload does not occur

1. Check that the watched path exists and `startHotReload()` returned true.
   Startup failure is logged and the resolver advertises SIGHUP-only fallback.
2. Confirm the modified file has a watched extension (`.yaml`, `.yml`, `.json`)
   and allow for configured debounce time.
3. Confirm the event invalidated the resolver cache, then explicitly verify
   that the owning service reloaded and validated the file. Cache invalidation
   alone is not evidence that new settings were applied.
4. If needed, use the registered SIGHUP handler on supported POSIX systems to
   clear resolver cache; it does not apply the config. Restart only if the
   owning service's recovery procedure requires it.

### Encrypted-store read or rotation fails

1. Preserve the failing error class and key version. Authentication, malformed
   metadata, and missing-key errors need different recovery actions.
2. Stop writes if integrity is uncertain. Keep a protected copy of the exact
   snapshot before attempting restore or rotation; serialized snapshots
   contain key bytes.
3. Restore only from a trusted snapshot through an approved secret-handling
   process. Do not manually edit IV, tag, ciphertext, or key-version fields.
4. Escalate decryption/authentication failures as potential integrity incidents.

## Escalation

Escalate repeated fallback growth, invalid-path/security signals, watcher
startup failures across hosts, persistent schema rejection, or any
authentication-tag/decryption failure to the config maintainer and the owning
service's incident lead. Follow the project-level `SECURITY.md` process for
suspected secret exposure or tampering.

See [`OPERATIONS.md`](OPERATIONS.md) for routine health checks and
[`TRUST.md`](TRUST.md) for secret and trust boundaries.
