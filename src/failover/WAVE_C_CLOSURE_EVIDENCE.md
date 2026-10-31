---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Wave C Failover Evidence — Source-Checked Status

> Historical test inventory, rechecked against current source on 2026-10-31.
> This document does not certify production security or claim tests passed in
> this update.

`tests/failover/test_failover_wave_c_fencing_security.cpp` contains nine tests
for plan-result caching, DR fencing behavior, and the concurrent-execution
guard. The fencing cases use a test double that returns configured epoch values;
the DR steps other than fencing are replaced with success hooks in those cases.
The concurrency cases exercise cached same-ID results and one rejected
different-ID execution.

These tests do **not** establish fencing-token replay resistance, distributed
leader uniqueness, stale-replica exclusion under network partitions, or
production security integration. A successful mocked DR result does not verify
external restore or traffic effects. The test target is included by
`tests/failover/CMakeLists.txt` and receives the `failover;release_critical`
labels; registration is not a record of execution or pass status.

See [`TRUST.md`](TRUST.md), [`SECURITY.md`](SECURITY.md), and
[`AUDIT.md`](AUDIT.md) for current controls and remaining limits.
