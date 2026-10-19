---
Author: ThemisDB Contributors
Created: 2026-10-19
Last Updated: 2026-10-19
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Security Module Runbook

AI provenance note: Exact model backend not verifiable from final document text alone.


This runbook captures the most common diagnosis, recovery, rollback, and escalation paths for the security module based on the source contracts and runtime behavior in `src/security`.

## 1. Incident triage checklist

When a problem appears in security behavior, confirm the following first:

- Is the failure at the transport layer, key-provider layer, policy layer, or audit layer?
- Did the system deny a request because of missing or invalid trust data?
- Did a key provider return an error or a degraded fallback state?
- Were security events logged or exported successfully?

The code path is usually visible in the component that raised the exception or denial.

## 2. Common failure modes and response

### Rule: policy denied or access denied unexpectedly

Likely sources:

- `zero_trust_policy_enforcer.cpp`
- `access_control_manager.cpp`
- `rbac.cpp`
- `row_level_security.cpp`

Response:

- verify the principal identity and token
- check whether the policy is missing or malformed
- confirm that the IP or user trust boundary is in the allowed list
- review whether the risk score exceeds the configured threshold

Do not override a deny result with a silent allow path. The contract requires fail-closed behavior.

### Rule: key lookup, rotation, or decrypt failure

Likely sources:

- `include/security/key_provider.h`
- `src/security/field_encryption.cpp`
- `src/security/vault_key_provider.cpp`
- `src/security/hsm_provider_pkcs11.cpp`

Response:

- confirm the key exists and is not revoked
- check whether the version is correct and active
- inspect rotation overlap state and recent key-creation time
- verify provider availability and production environment settings

### Rule: HSM or PKCS#11 unavailable or degraded

Likely source:

- `src/security/hsm_provider_pkcs11.cpp`

Response:

- review whether the environment is on a supported runtime configuration
- check whether the fallback path is explicitly allowed by the environment guard
- confirm production-mode guard requirements before permitting the software fallback to continue
- escalate if a fallback is required in a production environment without explicit approval

### Rule: suspicious or malicious query input

Likely source:

- `src/security/aql_injection_detector.cpp`

Response:

- verify whether the query includes mutation-statement patterns, suspicious literals, or unsafe AST markers
- review upstream input normalization and parameterization rules
- reject the query before it reaches execution

### Rule: audit export or evidence failure

Likely source:

- `src/security/security_evidence_collector.cpp`
- `include/security/security_evidence_collector.h`

Response:

- inspect export metrics and retention-window state
- confirm the audit chain remains intact
- determine whether the issue is a transient write failure or a deeper evidence integrity problem
- if audit writes fail, treat the operation as denied or blocked rather than silently continuing

## 3. Recovery procedures

- Rebuild trust state after policy or role changes with a controlled rollout.
- Rotate keys deliberately and verify the overlap window before retiring the prior key version.
- Remediate blocked query patterns at the caller boundary rather than disabling detection.
- Restore evidence export after verifying the backing store and retention configuration.

## 4. Rollback strategy

The safest rollback is operationally narrow:

- revert only the offending policy or key-management change
- ensure the previous active key version is still valid for decryption during the rollback window
- verify that the deny-by-default policy still applies during the rollback period
- confirm that audit evidence still covers the change window

## 5. Escalation path

Escalate to the module owner or security maintainer when:

- a key provider fails in production and no safe recovery path exists
- a policy or trust decision changes from fail-closed to permissive behavior
- evidence export or audit integrity is compromised
- AI-destructive operations are being evaluated outside the approved production policy

The operational requirement is that a security issue is not considered solved until the cause, response, and evidence trail have been reviewed.
