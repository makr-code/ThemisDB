---
Author: ThemisDB Contributors
Created: 2026-10-19
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: themisdb-doc-orchestrator
Human_Review: false
Human_Author: unknown
Source_Level: level1
SOT_Domain: api-contract
Canonical_References:
  - include/security/key_provider.h
  - include/security/encryption.h
  - include/security/rbac.h
  - include/security/access_control.h
  - include/security/access_control_manager.h
  - include/security/zero_trust_policy_enforcer.h
  - include/security/aql_injection_detector.h
  - include/security/ai_operation_guard.h
  - include/security/security_evidence_collector.h
  - include/security/security_api_contract.h
Related_Issue: "#7379"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Security Module API Reference

This page is a declaration-level index. Public names, namespaces, signatures, and
configuration fields are defined by the linked headers; runtime enforcement must
be verified in the corresponding implementation and tests.

## Key management and encryption

- `include/security/key_provider.h` declares `themis::KeyProvider`,
  `themis::KeyMetadata`, `themis::KeyStatus`, and `themis::KeyCache`.
  `KeyProvider` derives from the base `IKeyProvider` interface but remains
  abstract: provider-specific key retrieval, rotation, listing, metadata,
  deletion, existence, and key-creation operations are pure virtual.
  `KeyCache` is a separate cache utility, not a provider. The status
  enumerators are `ACTIVE`, `ROTATING`, `DEPRECATED`, and `DELETED`.
- `include/security/encryption.h` declares `themis::FieldEncryption`,
  `EncryptedBlob`, and `EncryptedField<T>`. Its `encrypt` overloads take
  `(plaintext, key_id)`. Decryption APIs include `decryptToString(blob)`,
  `decryptToBytes(blob)`, and `decrypt(blob)`; key-version helpers include
  `decryptAndReEncrypt(blob, key_id, updated_blob)` and
  `needsReEncryption(blob, key_id)`. There is no `reEncrypt` method.
- `include/security/vault_key_provider.h` declares `themis::VaultKeyProvider`
  and its nested `Config`; configuration fields include the Vault address/token,
  KV and Transit mount names, cache settings, request timeout, TLS verification,
  and retry settings.

## Access and request policy

- `include/security/rbac.h` declares `themis::security::Permission`, `Role`,
  `RBACConfig`, `RBAC`, `User`, and `UserRoleStore`. Permission checks use
  `RBAC::checkPermission(user_roles, resource, action)`. Role assignment is not
  an `RBAC` method; separate assignment methods are declared on `UserRoleStore`,
  `AccessControl`, and `AccessControlManager`.
- `include/security/access_control_manager.h` declares
  `themis::security::AccessControlManager`, `AccessControlConfig`,
  `SecurityContext`, and `AccessDecision`; its public operations include
  `authorize(context, resource, action)` and `assignRole(user_id, role)`.
- `include/security/zero_trust_policy_enforcer.h` declares request context,
  network-policy, verification-result, and trust-score fields. This reference
  exposes `ZeroTrustPolicyEnforcer::verify(const ZeroTrustContext&)`. This
  reference describes the declared API only; it does not assert a
  deployment-specific score threshold or that every caller invokes the enforcer.

## Query checks, AI-operation decisions, and evidence

- `include/security/aql_injection_detector.h` declares
  `themis::security::AQLInjectionDetector::validateParameterizedQuery(
  const std::string&, const std::vector<std::string>&)`,
  `validateAQLAST(const std::string&)`,
  `validateForReadOnlyContext(const std::string&)`, and
  `validateUnboundedForLoops(const std::string&)`.
- `include/security/ai_operation_guard.h` declares
  `themis::security::AiOperationGuard::evaluate()` and response builders. The
  header describes classification and decision data; it does not establish that
  every AI tool-call path is wired to this class.
- `include/security/security_evidence_collector.h` declares
  `themis::security::SecurityEvidenceCollector` and structured evidence types,
  including audit-log, metrics, key-rotation, access-control, network-control,
  and change-management fields.

## Contract and implementation boundary

`include/security/security_api_contract.h` contains policy constants and
normative contract text, including timeout and rotation-overlap values. Their
presence in that header alone is not evidence that every implementation enforces
them; consult implementation and test sources before making runtime claims.

The headers under `include/security/` are the canonical API references. This
summary intentionally omits usage examples, backend support claims, performance
figures, and blanket fail-closed or compliance guarantees.
