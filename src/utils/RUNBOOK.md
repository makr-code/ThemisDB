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

Exact model backend not verifiable from final document text alone.

# Utils Incident Runbook

## Scope and safety

Use this runbook to triage shared-helper failures that affect multiple
ThemisDB modules. First identify the helper and error context; utility error
categories are defined in
[`include/utils/error_contracts.h`](../../include/utils/error_contracts.h).
Do not bypass authentication, signature verification, input limits, TLS
pinning, or key checks to restore service. If the event may expose data or
compromise integrity, follow the project
[`SECURITY.md`](../../SECURITY.md) incident process.

## Triage matrix

| Symptom | Diagnose | Safe immediate action |
|---|---|---|
| PII findings unexpectedly absent or incomplete | Check enabled engines and last configuration error; distinguish detector log entries from scanner fail-closed sentinels. Validate config signature when configured and test the same input through the focused regex/NER test. | Treat the affected data as unscanned; stop or quarantine the consuming operation if its policy requires a complete scan. Restore the last known-valid configuration; do not equate empty results with a clean scan. |
| Regex timeout, invalid UTF-8, or oversized input | Check `PRIVACY_*` error context and the regex header's configured input/pattern bounds and timeout. | Bound or reject the input; correct the pattern/configuration and rerun the focused test. Do not increase limits without a review of CPU and memory impact. |
| Key retrieval/rotation or signature failure | Check `CRYPTO_*` context, LEK date/revocation state, RocksDB availability, PKI configuration, certificate/trust store, and pin material. | Keep encryption/signature-dependent writes blocked. Restore valid storage/trust material or rotate using the approved key procedure; never substitute plaintext or unverified signatures. |
| Compression decode error or abnormal expansion | Check codec result, input length, original size metadata for LZ4, and configured size/ratio bounds. | Reject the affected payload and preserve it for controlled investigation. Do not retry with bounds disabled. |
| Thread-pool queue rejection or rate-limit timeout | Inspect `ThreadPool::getStatistics()` (queued/active/executed/failed) and rate-limiter state; check whether shutdown or configured capacity caused rejection. | Apply backpressure or reject work at the caller. Reduce incoming load or restore healthy workers; do not retry in an unbounded loop. |
| Audit write/flush failure | Check audit error category, sink/storage health, queue capacity, and flush result. | Preserve the failure evidence and follow the consuming security workflow; do not treat an unrecorded security event as audited. Escalate if integrity evidence may be incomplete. |
| Trace initialization/export degradation | Check initialization error context and whether tracing was compiled/configured; compare sampling configuration and effective rate. | Keep application correctness independent of tracing. Restore exporter/configuration and confirm spans resume; do not infer request success from absent spans. |

## Focused verification and recovery

From the repository root, use the existing utility test guide for the
configured build directory and focused CTest selection:
[`tests/utils/PHASE_4_TEST_EXECUTION_GUIDE.md`](../../tests/utils/PHASE_4_TEST_EXECUTION_GUIDE.md).
Relevant suites include `RegexDetectionEngineFocusedTests`,
`NERDetectionEngineFocusedTests`, privacy/audit hardening, compression
hardening, and thread-pool/rate-limiter tests. When diagnosing documentation
or metadata changes, run:

```bash
python -m unittest discover -s tests -p "test_doc_metadata_gate.py"
```

After recovery, verify the failing helper's specific success and failure
cases, inspect structured logs/error context, and confirm the downstream
consumer has resumed only after required bounds and trust controls are active.
Do not roll back to an unsigned configuration, an unbounded queue, or a codec
path that hides errors.

## Escalation

Escalate shared-key, audit-integrity, certificate/pinning, or privacy exposure
incidents to the security maintainers using the project security process.
Escalate repeated saturation or broad consumer impact to the owning module
maintainers and attach the relevant error context, configuration (with
secrets removed), focused test result, and benchmark evidence if performance
is involved. See [`OPERATIONS.md`](OPERATIONS.md) for post-recovery checks.
