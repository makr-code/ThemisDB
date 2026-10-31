---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Module Triage Notes

## Purpose and safety

These are diagnostic boundaries for `src/config`, not a replacement for the
owning service's recovery/runbook or project security process. Do not include
plaintext secrets or serialized encrypted-store snapshots in general logs or
tickets. A resolver miss or validation failure is not authorization to continue.

## Path lookup returns no result

1. Capture the requested path and, if using `resolve()`, the attempted paths
   exposed by `ConfigNotFoundException`. Redact sensitive path components as
   required by the service's logging policy.
2. Check active `ConfigPathResolver::getEnvironment()` and filesystem access.
   `THEMIS_CONFIG_ENV` is read at process initialization; absent or unrecognized
   values select production.
3. Check whether a mapping exists with `isLegacyPath()` and inspect the
   resolver's metrics/deprecation report. A legacy mapping is a candidate path,
   not proof that the corresponding file exists.
4. Correct the source path or deployment input, then confirm the consuming
   service successfully loads and validates the expected file.

## Validation returns errors

1. Inspect `ValidationResult.config_path`, `schema_path`, `errors`, and
   `warnings`; parse/load failures and schema mismatches have different
   diagnostic contexts.
2. Review local `$ref` targets and the schema keywords implemented in
   `config_schema_validator.cpp`. External references are not fetched, and
   unsupported keywords must not be assumed to enforce a constraint.
3. Use the owning service's approved rollback/reload path. Validate the
   candidate before applying it.

## Watch notification or reload issue

1. Confirm `startHotReload()` returned true and the watched path exists.
2. Confirm the file event uses `.yaml`, `.yml`, or `.json`, and account for the
   configured debounce interval.
3. The resolver callback clears its path cache only. Verify separately that the
   owning service reloaded, validated and applied the final file contents.
4. `registerSighupHandler()` is an optional POSIX registration; the application
   must call it. SIGHUP causes a cache clear on a later resolver call, not a
   configuration reload. If watcher startup fails, the resolver logs that
   SIGHUP-only fallback is available; availability still depends on application
   registration and handling.

## Encrypted-store failure

1. Preserve the exception type/message and key version without copying secret
   values into logs.
2. `get()` throws for a missing key and propagates crypto failures.
   `tryGet()` returns `std::nullopt` only for absence and propagates crypto
   failures as `ConfigEncryptionException`.
3. Treat authentication failure as an integrity signal. Do not hand-edit
   ciphertext, IV, tag, or key-version fields.
4. Any serialized snapshot contains key bytes. Restore only through the
   owning service's reviewed secret-handling procedure.

## Escalation

Escalate repeated fallback use, unexpected misses, watcher startup failure,
schema uncertainty, crypto/authentication errors, or snapshot exposure to the
module maintainer and the owning service. Follow project-level
[`../../SECURITY.md`](../../SECURITY.md) for suspected exposure or tampering.
See [`OPERATIONS.md`](OPERATIONS.md) for routine component boundaries.
