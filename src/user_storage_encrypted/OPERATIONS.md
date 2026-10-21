---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-21
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Operations Guide for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This document describes how to operate the module during steady state and in degraded service. The implementation exposes explicit health and metrics surfaces, so operators should treat those as the primary runtime evidence rather than relying only on generic container health.

## Runtime responsibilities

The module is responsible for:

- validating backend availability and host mount prerequisites
- managing encrypted container lifecycle for each configured security level
- keeping key derivation and rotation state explicit and inspectable
- surfacing health and Prometheus-style metrics to the wider system

The central operations surfaces are `checkHealth()`, `checkLevelHealth(level)`, and `getMetricsText()` in `MultiLevelEncryptedStorage`.

## Metrics to monitor

The implementation defines four operational metric families:

- `user_storage_mounts_active` — active mounted containers
- `user_storage_mount_operations_total` — mount/unmount count, labeled by operation
- `user_storage_key_rotations_total` — rotation callbacks fired, labeled by security level
- `user_storage_container_size_bytes` — sum of encrypted container sizes

Operators should treat deviations here as a leading signal. A static or missing counter may indicate the module is not mounted or is no longer processing scheduled rotation events.

## Health checks

`checkHealth()` and `checkLevelHealth(level)` provide the main health API. The module's design expects health checks to answer:

- is a mount path valid?
- is the backend reachable and working?
- is the key provider configured for the current level?
- is the scheduler or level state degraded?

If these checks fail, the module should be treated as unhealthy even if the process remains running.

## Normal operating posture

A healthy instance should show:

- the backend is installed and available
- security levels validate without missing provider or config data
- active mount counts remain consistent with policy
- key rotation events are observed on schedule
- stale mounts are reconciled rather than accumulating across restarts

## Degraded operating posture

The module is designed to remain explicit under degraded conditions. Typical degraded states include:

- gocryptfs binary absent from PATH
- mount point permission issue
- invalid key material or missing salt state
- stale mount cleaning failing on the host
- rotation store state mismatch or corruption

These should be treated as service-affecting signs, not as safe fallback states.

## Recovery expectations

Recovery must be narrow and specific:

1. restore backend availability or fix the host prerequisite
2. validate the mount path and configuration
3. confirm key provider and salt state for the level
4. re-run health and metrics checks
5. confirm rotation scheduler state has not been silently corrupted

Because the contract is fail-closed, a downgraded or partial state should be kept visible to operators rather than being normalized into an implicit success.

## Monitoring and escalation

Escalate to the owning subsystem when:

- mount counts fall unexpectedly or growth diverges from configuration
- metric families are missing or non-updating
- rotation callbacks are absent after the expected interval
- stale mounts repeatedly appear after startup or crash recovery
- host support for FUSE or gocryptfs is unstable across runtime environments

## Ownership of operations

This module is not a general storage runtime; it is a security-sensitive storage boundary. Operational ownership therefore belongs to the same team that manages encrypted filesystem support, key management, and host-level system constraints. The module is intentionally explicit about failure modes so that the responsible operator can act quickly without misclassifying the issue as a generic storage outage.
