### Context

This issue implements the roadmap item 'Plugin Security: CRL and OCSP Certificate Revocation Checking' for the acceleration domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Plugin Security: CRL and OCSP Certificate Revocation Checking

### Goal

Deliver the scoped changes for Plugin Security: CRL and OCSP Certificate Revocation Checking in src/acceleration/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Plugin Security: CRL and OCSP Certificate Revocation Checking
**Priority:** High
**Target Version:** v1.8.0

`plugin_security.cpp` contains two complete stub methods for certificate revocation:
- `PluginSecurityVerifier::checkCRL()` (line 598): extracts CRL distribution points but never fetches or validates the CRL — the body is a comment block listing the 4 required steps; when revocation checking is configured it fail-safes to `false` (line 636) and warns `"actual CRL checking not implemented"`.
- `PluginSecurityVerifier::checkOCSP()` (line 654): identical structure — OCSP responder URLs are extracted but no OCSP request is built or sent; also fail-safes to `false` (line 691).
- `EnhancedPluginSecurityVerifier::verifyCertificateChain()` (line 1036–1037) emits `THEMIS_WARN("Revocation checking configured but not yet implemented")`.

This means any GPU plugin with a revoked code-signing certificate will pass security validation when `requireRevocationCheck = false` (the default), and will fail to load with an opaque error when `requireRevocationCheck = true`, with no actionable diagnostic.

**Implementation Notes:**
- `[ ]` Implement `checkCRL()` in `plugin_security.cpp`: (1) for each CRL distribution point URL, perform an HTTP GET using `libcurl` or `WinHttp`; (2) parse the DER-encoded CRL with OpenSSL `d2i_X509_CRL`; (3) verify the CRL signature against the issuer certificate; (4) call `X509_CRL_get0_by_cert()` to check the target certificate's serial number; (5) validate CRL `thisUpdate` / `nextUpdate` timestamps. Honour a configurable timeout (default 5 s) to prevent hangs on unreachable endpoints.
- `[ ]` Implement `checkOCSP()` in `plugin_security.cpp`: (1) build an OCSP request with `OCSP_REQUEST_new()` + `OCSP_request_add0_id()`; (2) POST to each OCSP responder URL via HTTP; (3) parse the response with `OCSP_response_status()` and `OCSP_resp_find_status()`; (4) verify the responder's signature; (5) check `thisUpdate` / `nextUpdate` bounds.
- `[ ]` Implement PE certificate table extraction in `EnhancedPluginSecurityVerifier::extractSigningCertificate()` (line 1092): parse the PE optional header's DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY], extract the WIN_CERTIFICATE structure, and return the embedded PKCS#7 blob as a DER byte string. Currently the method detects the PE magic bytes but returns without extracting the cert (comment: *"extraction not fully implemented"*).
- `[ ]` Cache CRL/OCSP results per certificate serial number with a configurable TTL (default: CRL `nextUpdate`, OCSP 1 h) to avoid per-load network round trips.
- `[ ]` Add unit tests with a mock HTTP server (or pre-fetched fixtures) for both CRL and OCSP paths; cover revoked-cert, unknown-cert, network-timeout, and signature-invalid cases.

**Security Note:** Until this is implemented, plugin revocation is not enforced even when `requireRevocationCheck = true`. The fail-safe behaviour (returning `false`) prevents revoked plugins from loading only when the calling code actually checks the `checkCRL()`/`checkOCSP()` return value; `verifyCertificateChain()` currently bypasses both checks with a warning.

---

### Acceptance Criteria

- [ ] `PluginSecurityVerifier::checkCRL()` (line 598): extracts CRL distribution points but never fetches or validates the CRL — the body is a comment block listing the 4 required steps; when revocation checking is configured it fail-safes to `false` (line 636) and warns `"actual CRL checking not implemented"`.
- [ ] `PluginSecurityVerifier::checkOCSP()` (line 654): identical structure — OCSP responder URLs are extracted but no OCSP request is built or sent; also fail-safes to `false` (line 691).
- [ ] `EnhancedPluginSecurityVerifier::verifyCertificateChain()` (line 1036–1037) emits `THEMIS_WARN("Revocation checking configured but not yet implemented")`.
- [ ] Implement `checkCRL()` in `plugin_security.cpp`: (1) for each CRL distribution point URL, perform an HTTP GET using `libcurl` or `WinHttp`; (2) parse the DER-encoded CRL with OpenSSL `d2i_X509_CRL`; (3) verify the CRL signature against the issuer certificate; (4) call `X509_CRL_get0_by_cert()` to check the target certificate's serial number; (5) validate CRL `thisUpdate` / `nextUpdate` timestamps. Honour a configurable timeout (default 5 s) to prevent hangs on unreachable endpoints.
- [ ] Implement `checkOCSP()` in `plugin_security.cpp`: (1) build an OCSP request with `OCSP_REQUEST_new()` + `OCSP_request_add0_id()`; (2) POST to each OCSP responder URL via HTTP; (3) parse the response with `OCSP_response_status()` and `OCSP_resp_find_status()`; (4) verify the responder's signature; (5) check `thisUpdate` / `nextUpdate` bounds.
- [ ] Implement PE certificate table extraction in `EnhancedPluginSecurityVerifier::extractSigningCertificate()` (line 1092): parse the PE optional header's DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY], extract the WIN_CERTIFICATE structure, and return the embedded PKCS#7 blob as a DER byte string. Currently the method detects the PE magic bytes but returns without extracting the cert (comment: *"extraction not fully implemented"*).
- [ ] Cache CRL/OCSP results per certificate serial number with a configurable TTL (default: CRL `nextUpdate`, OCSP 1 h) to avoid per-load network round trips.
- [ ] Add unit tests with a mock HTTP server (or pre-fetched fixtures) for both CRL and OCSP paths; cover revoked-cert, unknown-cert, network-timeout, and signature-invalid cases.

### Relationships

- Roadmap row: #38 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#plugin-security-crl-and-ocsp-certificate-revocation-checking
- Source key: roadmap:38:acceleration:v1.8.0:plugin-security-crl-and-ocsp-certificate-revocation-checking

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:38:acceleration:v1.8.0:plugin-security-crl-and-ocsp-certificate-revocation-checking -->
<!-- roadmap-ref: row=38;module=acceleration;target=v1.8.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#plugin-security-crl-and-ocsp-certificate-revocation-checking -->
