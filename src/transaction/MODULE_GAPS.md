# transaction Module — Implementation Gap Analysis

**Status:** In Progress  
**Last Updated:** 2026-05-28  

---

## 📊 Gap Summary

Refresh this module's gap analysis with:

```bash
python tools/gap_audit_pipeline_v2.py
```

---

## ✅ Recent Remediation (2026-05-28)

### Stub #279 — Phase-1 PREPARE RPC Bridge (`src/transaction/distributed_transaction_manager.cpp`)

**Linked issue:** #5363 (P0)

**Problem:** `runPhase1Unlocked()` had no mechanism to send PREPARE to remote
participants (those with `callback == nullptr`).  When a Phase-2 bridge was
configured, the coordinator assumed remote participants voted YES without asking
them, which violates strict 2PC correctness.  When no Phase-2 bridge was
configured, the coordinator voted ABORT for remote participants without contacting
them.

**Fix applied:**

- Added `remote_phase1_dispatch` to `DistributedTxnManagerConfig` — a function
  injected at construction time that delivers a PREPARE request to a remote
  participant and returns its COMMIT/ABORT vote.
  Signature: `bool(txn_id, node_id, endpoint, affected_keys)`.

- Added `Phase1RpcFn` / `phase1_rpc_fn` to `DistributedTxnManagerConfig` — a
  simpler bridge variant with signature `bool(endpoint, txn_id, affected_keys)`.

- Added `setRpcPhase1Fn` / `clearRpcPhase1Fn` static methods — process-wide
  legacy bridge mirroring the existing `setRpcPhase2Fn` / `clearRpcPhase2Fn` API.

- Updated `runPhase1Unlocked()` to check bridges in priority order:
  1. `phase1_rpc_fn` — per-instance config
  2. `remote_phase1_dispatch` — per-instance config
  3. Static `getRpcPhase1Fn()` — process-wide legacy
  4. Phase-2-bridge-only compat path (warns, assumes YES — backwards compat)
  5. No bridge → fail-closed ABORT vote

- Empty `endpoint` for a remote participant is now a hard ABORT vote with an
  error log, instead of silently falling through.

**Tests added** (`tests/test_transaction_distributed_2pc.cpp`):

| Test | Scenario |
|------|----------|
| `Stub279_Phase1RpcFnYesVoteAllowsCommit` | Remote votes YES → COMMIT succeeds |
| `Stub279_Phase1RpcFnNoVoteAbortsTransaction` | Remote votes NO → ABORT |
| `Stub279_Phase1RpcFnExceptionIsAbortVote` | RPC exception → fail-closed ABORT |
| `Stub279_RemotePhase1DispatchCommit` | `remote_phase1_dispatch` bridge works |
| `Stub279_PureRemoteTransactionSucceedsWithPhase1Rpc` | 3-shard all-remote 2PC commit |
| `Stub279_StaticPhase1FnCommit` | Static `setRpcPhase1Fn` bridge works |

**Gap delta:** Stub #279 Phase-1 closed.  Remote participants now participate in
Phase-1 voting when a bridge is configured, making pure-remote 2PC transactions
correct.  The backwards-compat Phase-2-only path is retained with a WARN to guide
operators toward full Phase-1 wiring.

---

## 🚀 How to Use This Documentation

Once generated, this file will contain:

- **Gap Statistics:** Count of unimplemented paths, TODOs, STUBs, etc.
- **Critical Issues:** What needs to be fixed first
- **Implementation Roadmap:** Phases and priorities
- **Affected Files:** Which source files have gaps
- **GitHub Issues:** Links to related GitHub issues
- **Next Steps:** Action items for developers

---

## 📍 Location

This documentation is in the module directory for easy access:
```
src/transaction/MODULE_GAPS.md  ← You are here
```

Developers working on this module can reference this file directly.

---

## 🔄 How It's Updated

The documentation is automatically generated and updated by the gap audit pipeline:

```bash
# Full pipeline (scan + update headers + generate docs)
python tools/gap_audit_pipeline_v2.py

# Just generate module docs
python tools/module_doc_generator.py . ai_working ai_working/module_gaps
```

After each run, this file is updated with fresh analysis.

---

**Format:** THEMIS_MODULE_GAPS_v1  
**Generator:** ThemisDB Gap Audit Pipeline v2  
**Auto-Generated:** Yes
