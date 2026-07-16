### Context

This issue implements the roadmap item 'Plugin Security: PE Certificate Table Extraction' for the acceleration domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Plugin Security: PE Certificate Table Extraction

### Goal

Deliver the scoped changes for Plugin Security: PE Certificate Table Extraction in src/acceleration/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Plugin Security: PE Certificate Table Extraction
**Priority:** Medium
**Target Version:** v1.8.0

`EnhancedPluginSecurityVerifier::extractSigningCertificate()` in `plugin_security.cpp:1075–1092` detects the PE magic bytes (`0x00004550`) but then hits a comment block reading *"For now: indicate PE format detected but extraction not fully implemented"* and falls through without returning certificate data. The Linux ELF path below it is similarly incomplete. Without this, `verifyAuthenticodeSignature()` (line 1202) receives an empty certificate string and cannot perform the Authenticode check.

**Implementation Notes:**
- `[ ]` Parse PE optional-header data directories: seek to `e_lfanew + 0x18 + offsetof(OptionalHeader, DataDirectory[4])`, read the `VirtualAddress` and `Size` fields for the Security directory (entry 4, `IMAGE_DIRECTORY_ENTRY_SECURITY`).
- `[ ]` Map the certificate table: for each `WIN_CERTIFICATE` record in the table, check `wCertificateType == WIN_CERT_TYPE_PKCS_SIGNED_DATA` and extract `bCertificate[dwLength - offsetof(WIN_CERTIFICATE, bCertificate)]` as a DER blob.
- `[ ]` For ELF plugins on Linux: look for a `.note.gnu.signature` section or a sidecar `plugin.so.sig` file; fall back to returning an empty string (unsigned) rather than leaving the code path unreachable.
- `[ ]` Return the first valid PKCS#7 DER blob; log a warning if multiple certificates are present.
- `[ ]` Add a fixture-based unit test with a pre-signed PE test binary to validate extraction end-to-end.

---

### Acceptance Criteria

- [ ] Parse PE optional-header data directories: seek to `e_lfanew + 0x18 + offsetof(OptionalHeader, DataDirectory[4])`, read the `VirtualAddress` and `Size` fields for the Security directory (entry 4, `IMAGE_DIRECTORY_ENTRY_SECURITY`).
- [ ] Map the certificate table: for each `WIN_CERTIFICATE` record in the table, check `wCertificateType == WIN_CERT_TYPE_PKCS_SIGNED_DATA` and extract `bCertificate[dwLength - offsetof(WIN_CERTIFICATE, bCertificate)]` as a DER blob.
- [ ] For ELF plugins on Linux: look for a `.note.gnu.signature` section or a sidecar `plugin.so.sig` file; fall back to returning an empty string (unsigned) rather than leaving the code path unreachable.
- [ ] Return the first valid PKCS#7 DER blob; log a warning if multiple certificates are present.
- [ ] Add a fixture-based unit test with a pre-signed PE test binary to validate extraction end-to-end.

### Relationships

- Roadmap row: #129 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#plugin-security-pe-certificate-table-extraction
- Source key: roadmap:129:acceleration:v1.8.0:plugin-security-pe-certificate-table-extraction

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:129:acceleration:v1.8.0:plugin-security-pe-certificate-table-extraction -->
<!-- roadmap-ref: row=129;module=acceleration;target=v1.8.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#plugin-security-pe-certificate-table-extraction -->
