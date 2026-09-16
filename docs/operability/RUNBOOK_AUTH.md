# Runbook: Auth Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB `auth` module. Covers the five
most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Scenario 1 — Provider Degraded

**Log pattern:** `[AUTH:ProviderDegraded]`

**Symptoms**
- An authentication provider (LDAP, SAML IDP, OIDC, WebAuthn) becomes unavailable
  or responds too slowly.
- Log lines contain `[AUTH:ProviderDegraded]` with provider ID and degradation type.
- Login attempts against the affected provider fail or time out.

**Triage**
1. Identify the degraded provider from the log line.
2. Check provider health: `themis_admin auth provider status <id>`.
3. Verify network connectivity and TLS certificates for the provider endpoint.
4. Review the provider circuit breaker state: `themis_admin auth circuit-breaker status`.

**Remediation**
1. Trigger a provider health re-check: `themis_admin auth provider probe <id>`.
2. If the provider is unresponsive, switch to a backup provider:
   `themis_admin auth provider failover <id>`.
3. Enable degraded-mode local token caching to serve cached sessions:
   set `auth.cache.allow_stale_on_provider_degraded: true`.
4. Notify users that authentication via the affected provider may be temporarily unavailable.

**Escalation**
If the provider does not recover within 10 minutes, escalate to the identity
infrastructure team with provider health logs, TLS status, and the circuit
breaker state.

---

## Scenario 2 — Token Revoked

**Log pattern:** `[AUTH:TokenRevoked]`

**Symptoms**
- Requests are rejected because the bearer token has been explicitly revoked.
- Log lines contain `[AUTH:TokenRevoked]` with token ID and revocation reason.
- Clients receive `401 Unauthorized` after the token was previously valid.

**Triage**
1. Identify the revoked token ID and the revocation reason from the log.
2. Determine who initiated the revocation: `themis_admin auth token audit <id>`.
3. Check whether the revocation was intentional (user logout, admin action) or erroneous.
4. Verify the distributed blacklist propagation: `themis_admin auth blacklist status`.

**Remediation**
1. If the revocation was erroneous, restore the token from the audit log and
   issue a replacement: `themis_admin auth token issue --for <user_id>`.
2. If the blacklist propagation is lagging, force a sync:
   `themis_admin auth blacklist sync --force`.
3. Advise the user to re-authenticate to obtain a new valid token.
4. Review the revocation trigger to prevent erroneous future revocations.

**Escalation**
Widespread erroneous revocations affecting multiple users require escalation to
the auth module owner with the revocation event log and affected user count.

---

## Scenario 3 — Federation Failed

**Log pattern:** `[AUTH:FederationFailed]`

**Symptoms**
- A federated authentication request to an external IDP fails.
- Log lines contain `[AUTH:FederationFailed]` with federation provider ID and error code.
- Users from federated identity domains cannot authenticate.

**Triage**
1. Identify the failing federation provider from the log.
2. Check federation endpoint connectivity: `themis_admin auth federation probe <provider_id>`.
3. Verify SAML/OIDC metadata is current: `themis_admin auth federation metadata status`.
4. Review recent changes to the federation configuration or IDP metadata.

**Remediation**
1. Refresh the IDP metadata: `themis_admin auth federation metadata refresh <provider_id>`.
2. Verify that SAML assertions or OIDC tokens from the IDP meet the configured validation rules.
3. If the IDP has rotated signing keys, update the trusted certificate store:
   `themis_admin auth federation certs update <provider_id>`.
4. Temporarily disable the failing federation and route users to an alternate auth path.

**Escalation**
Federation failures affecting a large tenant require escalation to the IDP
admin team and the auth module owner — provide the federation provider ID,
error code, and the IDP metadata version.

---

## Scenario 4 — Policy Deny

**Log pattern:** `[AUTH:PolicyDeny]`

**Symptoms**
- An authentication or authorisation request is denied by an access policy.
- Log lines contain `[AUTH:PolicyDeny]` with policy ID, user ID, and denied resource.
- Users receive `403 Forbidden` despite having valid tokens.

**Triage**
1. Identify the policy ID, user ID, and denied resource from the log.
2. Review the active policy definition: `themis_admin auth policy show <id>`.
3. Verify that the user has the expected roles and attributes:
   `themis_admin auth user inspect <user_id>`.
4. Check for recent policy updates that may have inadvertently restricted access.

**Remediation**
1. If the deny is a misconfiguration, update the policy:
   `themis_admin auth policy update <id>`.
2. If the user is missing a required role, grant it through the RBAC system:
   `themis_admin auth role assign <user_id> <role>`.
3. Roll back the policy if a recent update caused widespread denials:
   `themis_admin auth policy rollback <id>`.
4. Log the exception and notify the affected user with remediation instructions.

**Escalation**
Policy denials affecting a large group of users require escalation to the
governance and identity teams — provide the policy ID, affected user sample,
and the resource scope.

---

## Scenario 5 — Distributed Blacklist Lag

**Log pattern:** `[AUTH:TokenRevoked]` (delayed propagation)

**Symptoms**
- Revoked tokens are still accepted on some nodes due to blacklist propagation lag.
- Log lines show `[AUTH:TokenRevoked]` appearing later than the revocation timestamp.
- Security audits detect post-revocation access events.

**Triage**
1. Measure blacklist propagation lag: `themis_admin auth blacklist lag`.
2. Identify the nodes with stale blacklists.
3. Review the blacklist replication configuration and network health between nodes.
4. Check whether the blacklist store is healthy and accepting writes.

**Remediation**
1. Force an immediate blacklist sync across all nodes:
   `themis_admin auth blacklist sync --all-nodes --force`.
2. Increase the blacklist propagation frequency:
   set `auth.blacklist.propagation_interval_ms`.
3. Add the affected tokens to a per-node revocation emergency list:
   `themis_admin auth token revoke-emergency <token_id>`.
4. Audit all access events between the revocation time and the propagation completion.

**Escalation**
Post-revocation access is a security incident — escalate immediately to the
security team with the affected token IDs, the lag measurement, and the audit
event log.

---

## Reference

| Log Pattern               | Severity | SLO Impact | Owner |
|---------------------------|----------|------------|-------|
| `[AUTH:ProviderDegraded]` | High     | Yes        | auth  |
| `[AUTH:TokenRevoked]`     | High     | Yes        | auth  |
| `[AUTH:FederationFailed]` | High     | Yes        | auth  |
| `[AUTH:PolicyDeny]`       | Medium   | Partial    | auth  |

---

*Wave D operability deliverable — see `src/auth/ROADMAP.md`.*
