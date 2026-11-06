---
Author: "ThemisDB Contributors"
Created: "2026-11-02"
Last Updated: "2026-11-02"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

# Transaction API — recovery behavior

`DistributedTransactionManager::commitDistributed()` returns an error without
sending Phase-2 COMMIT if a COMMIT decision cannot be appended and flushed to its
configured WAL. With recovery logging disabled, the transaction remains
PREPARED and appears in `getRecoverableTransactions()`; callers must not treat
that result as a completed commit. A successful response means this coordinator
observed a successful local WAL flush and Phase-2 delivery. It is not evidence
of multi-process durability testing or a system-wide readiness check.

`GlobalTransactionManager::commit()` similarly requires a durable WAL before
participant preparation and returns `ERROR` when Phase-2 delivery or the durable
completion record is incomplete. In either manager, unresolved state must not be
treated as transaction completion.

The current DTM WAL does not persist enough participant/acknowledgement state to
prove restart-time re-delivery/completion for every decision. An empty
in-memory recovery snapshot is not a system readiness signal.

See [module recovery contract](README.md) for the limits of shared recovery
reports and multi-process evidence.
