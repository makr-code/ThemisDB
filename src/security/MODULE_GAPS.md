# security — MODULE_GAPS.md (Targeted Refresh)

## Status (2026-08-17)

- Legacy Phase-5 scanner baseline remains archived in Batch-4 planning artifacts.
- This module file now tracks **current residual-risk state** for active fixes instead of repeating stale aggregate counters.

## Closed in this refresh

- ✅ `timestamp_authority_openssl.cpp` TSA transport hardening aligned:
  - HTTPS-only transport enforcement.
  - libcurl protocol restriction to HTTPS (including redirects).
  - Explicit connect timeout and TLS minimum version hardening.
  - Shared hardening path reused by timestamp request and availability probing.

## Remaining work

- Not all gaps are closed yet: final module-level production-ready sign-off remains pending until a fresh full security gap scan is completed and the remaining non-TSA CRITICAL/HIGH items are closed.
- Run a fresh full security gap scan to replace legacy Batch-4 aggregate counts with current numbers.
- Continue closure of non-TSA remaining CRITICAL/HIGH findings tracked in Security Batch-4 artifacts.

## Source of truth

- `ai_working/SECURITY_MODULE_GAPS_BATCH4_MASTER_PLAN.md` (legacy planning baseline, now marked outdated)
- `ai_working/SECURITY_BATCH4_IMPLEMENTATION_PLAN_2026-08-17.md` (current targeted execution notes)
