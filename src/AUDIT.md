> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

# AUDIT — Cross-Module Source Code Analysis

**Last Updated:** 2026-04-21 | **Analysis:** 5-session deep source code review across all production-critical modules

## Scope
- Modul/Ordner: `src`
- Full source code analysis (not checklist): all major `.cpp` files in 10 modules read line-by-line
- Focus: safety violations, deadlocks, use-after-free, auth bypass, injection, resource exhaustion

## Module Status Snapshot

The authoritative per-module matrix lives in [`MODULE_INDEX.md`](MODULE_INDEX.md). This audit keeps the high-level grouping visible:

| Status | Module groups | Audit posture |
|---|---|---|
| Stable / production-ready | `server`, `storage`, `network`, `auth`, `security`, `cache`, `analytics`, `failover`, `maintenance`, `updates`, `process`, `execution` | Current audit stack treats these as the lower-risk baseline with residual hardening or documentation tasks. |
| Real gaps still present | `themis`, `transaction`, `query`, `index`, `sharding`, `replication`, `graph`, `cdc`, `llm`, `rag`, `gpu`, `acceleration`, `geo`, `voice`, `access_model`, `ethics_ai` | These modules still require source-level follow-up, either because gaps remain or because evidence refresh is pending. |
| Planned / partially externalized | `chimera`, `user_storage`, plugin externalization tracks | These are not yet fully owned by a closed implementation path and remain tracked as planned work. |

---

## ⚠️ Executive Summary

> The previous audit framework (checklists, build registration, test coverage counts)
> missed critical safety and security violations in the actual source code.
> Across 5 sessions of direct code analysis, **23 S0 (Critical / Safety-violation) findings**
> were identified, distributed across 8 modules.
>
> The most severe pattern is that **both the HTTP server and wire protocol server lack
> centralized authentication enforcement** — individual handler omissions are a recurring
> vulnerability class. The **authentication module itself deadlocks on every login attempt**.
> The **LLM model loading pipeline accepts arbitrary filesystem paths**, enabling arbitrary
> file read and write from the server process.

---

## Cumulative Findings by Severity

| Session | Modules | S0 | S1 | S2 | S3 |
|---------|---------|----|----|----|----|
| 1 | sharding, transaction | 8 | — | — | — |
| 2 | storage, security, cache | 4 | 9 | 11 | 4 |
| 3 | network, server | 3 | 10 | 7 | 1 |
| 4 | query, aql, graph | 5 | 9 | 1 | — |
| 5 | llm, rag | 3 | 12 | 6 | 1 |
| **Total** | | **23** | **40** | **25** | **6** |

---

## S0 — Critical Findings (All Sessions)

### Session 1: Sharding / Transaction

| ID | Module | File | Description |
|----|--------|------|-------------|
| SH-1 | sharding | `paxos_coordinator.cpp` | Self-deadlock: `mutex_` re-acquired in `onPromiseResponse()` |
| SH-2 | sharding | `gossip_protocol.cpp` | Deadlock: `members_mutex_` acquired inside `send()` while already held |
| SH-3 | sharding | `cross_shard_transaction.cpp` | Use-after-free: `ShardConnection` stored by raw ref, used after vector realloc |
| SH-4 | sharding | `cross_shard_transaction.cpp` | Dangling ref: lambda captures `&coordinator_node_id`, copied std::string destroyed |
| SH-5 | sharding | `cross_shard_transaction.cpp` | Dangling ref: callback captures `&result`, stack object outlived by async callback |
| SH-6 | sharding | `raft_wal_integration.cpp` | Self-deadlock: WAL flush mutex re-acquired from WAL callback |
| SH-7 | sharding | `paxos_coordinator.cpp` | Quorum bypass: hardcoded `accepted_count >= 1` instead of majority |
| SH-8 | transaction | `distributed_transaction_manager.cpp` | Integer overflow: `setTimeout(ms)` wraps `uint32_t` → negative timeout |

### Session 2: Storage / Security / Cache

| ID | Module | File | Description |
|----|--------|------|-------------|
| R-1 | storage | `rocksdb_wrapper.cpp` | TOCTOU in `close()`: `db_lifecycle_mutex_` released before busy-wait; new ops start after release; `db_.reset()` races → use-after-free |
| A-1 | security | `access_control.cpp` | Guaranteed deadlock in `authenticate()`: `mutex_` held, `getUserRoles()` + `createSession()` both re-acquire |
| A-2 | security | `access_control.cpp` | Guaranteed deadlock in `changePassword()`: `mutex_` held, `invalidateUserSessions()` re-acquires |
| D-1 | cache | `distributed_cache_coordinator.cpp` | Non-POSIX `verifyHmac()` stub returns `true` unconditionally — all unsigned messages accepted |

### Session 3: Network / Server

| ID | Module | File | Description |
|----|--------|------|-------------|
| WPS-1 | network | `wire_protocol_server.cpp` | Missing auth check in `handleTimeseriesQuery()` (opcode 0x51) — unauthenticated TS reads |
| HS-1 | server | `http_server.cpp` | Admin shard management endpoints inline in router with no auth check |
| HS-2 | server | `http_server.cpp` | WAL apply endpoint (`WalApplyPost`) has no auth at routing layer |

### Session 4: Query / AQL / Graph

| ID | Module | File | Description |
|----|--------|------|-------------|
| QE-1 | query | `query_engine.cpp` | Data race on shared `errors` vector written by concurrent TBB tasks |
| QE-2 | query | `query_engine.cpp` | No ACL check on collection name in any `execute*` method — any caller reads any collection |
| PA-1 | query | `aql_parser.cpp` | No recursion depth limit in recursive-descent parser → stack overflow on crafted input |
| LLM-1 | aql | `llm_aql_handler.cpp` | `schema_context` injected verbatim into LLM system prompt → prompt injection |
| LLM-2 | aql | `llm_aql_handler.cpp` | Generated AQL never privilege-checked; runs at system privilege level |

### Session 5: LLM / RAG

| ID | Module | File | Description |
|----|--------|------|-------------|
| F1-1 | llm | `multi_lora_manager.cpp` | Arbitrary path injection in `loadLoRAInternal()` — no trusted-directory check |
| F1-2 | llm | `multi_lora_manager.cpp` | Remote-deserialized LoRA `path` passed to `llama_lora_adapter_init()` without sanitization |
| F2-1 | llm | `llama_wrapper.cpp` | Path traversal via `model_id` → arbitrary file write of attacker-controlled binary data |

---

## Module Status Summary

| Module | AUDIT.md | S0 | S1 | Status |
|--------|----------|----|----|--------|
| sharding | `src/sharding/AUDIT.md` | 0 (PAX-1/2/3, GOS-1, CST-1/2/3, RWALI-1/2 fixed) | — | ✅ S0+S2+S3 resolved; S3: 2PC-3, RLOG-2, TWAL-2 fixed 2026-05-04; CC-1..CC-5 addressed 2026-05-04 (1 S2 open: CST-6 design limitation) |
| transaction | `src/transaction/ROADMAP.md` | 0 (SH-8 fixed) | — | ✅ S0 resolved |
| storage | `src/storage/AUDIT.md` | 0 (R-1 fixed 2026-05-04) | 0 (R-2 fixed 2026-05-04) | ✅ S0+S1+S2+S3 resolved (R-3/R-4/R-5/W-1/W-2 fixed 2026-05-04; W-3/R-6 fixed 2026-05-04) |
| security | `src/security/AUDIT.md` | 0 (A-1/A-2 fixed) | 0 (A-3, E-1, E-2, E-4, RB-1 fixed 2026-05-04; SEC-AUTH-01/SEC-NET-01 fixed 2026-05-05) | ✅ S0+S1+S2+S3 resolved (A-4/A-5/E-3/RB-2 fixed 2026-05-04; A-6/RB-3 fixed 2026-05-04) |
| cache | `src/cache/AUDIT.md` | 0 (D-1 fixed 2026-05-04) | 0 (C-1, C-2, C-4 fixed 2026-05-04) | 0 (C-3, D-2, D-3 fixed 2026-05-04) — ✅ S0+S1+S2 resolved |
| network | `src/network/AUDIT.md` | 0 (WPS-1..5 fixed 2026-05-04) | 0 | 0 (WPS-6..10 fixed 2026-05-04) — ✅ S0+S1+S2+S3 resolved (WPS-11 fixed 2026-05-04) |
| server | `src/server/AUDIT.md` | 0 (HS-1/HS-2 fixed) | 0 (HS-3..HS-9 fixed 2026-05-04) | 0 (HS-10, HS-11, HS-12 fixed 2026-05-04) — ✅ S0+S1+S2 resolved |
| query | `src/query/AUDIT.md` | 0 (QE-1, PA-1 fixed 2026-05-04) | 0 (QE-3, QE-4, QE-5, PA-2, TR-1, TR-2 fixed 2026-05-04) | ✅ S0+S1 resolved |
| aql | `src/aql/AUDIT.md` | 0 (LLM-1/LLM-2 addressed) | 0 (LLM-3 fixed 2026-05-04) | 0 (LLM-4 fixed 2026-05-04) — ✅ S0+S1+S2 resolved |
| graph | `src/graph/AUDIT.md` | — | 0 (GQ-1, GQ-2 fixed 2026-05-04) | ✅ S1 resolved |
| llm | `src/llm/AUDIT.md` | 0 (F1-1/F1-2/F2-1 fixed) | 0 (F1-3..F3-2 fixed 2026-05-04) | ✅ S0+S1+S2 resolved (F2-5, F2-6 fixed 2026-05-04) |
| rag | `src/rag/AUDIT.md` | — | 0 (F4-1, F4-2, F5-1, F5-2 fixed 2026-05-04) | ✅ S1+S2+S3 resolved (F4-3/F4-4/F5-3/F5-4 fixed 2026-05-04; F4-5 fixed 2026-05-04) |

---

## Prüffelder
- Eingabevalidierung und Fehlerpfade
- Logging/Auditing und Nachvollziehbarkeit
- Abhängigkeiten und externe Integrationen
- Testabdeckung für kritische Pfade

## Aktueller Stand
- [x] Initiale Modul-Audit-Checkliste vollständig abgearbeitet (5 sessions, 12 modules)
- [ ] Findings priorisiert und Issues/PRs verknüpft
- [ ] Re-Audit nach Änderungen durchgeführt

## Nachweis
- Audit-Ergebnisse in modul-spezifischen AUDIT.md Dateien (`src/*/AUDIT.md`)
- Alle Befunde mit Quelldatei, Zeilennummer und Code-Ausschnitt belegt
