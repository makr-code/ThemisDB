# RFC 3161 TSA Library & Provider Evaluation

**Issue:** [TSA] Evaluate RFC 3161 compliant libraries  
**Parent Issue:** #3251  
**Date:** 2026-03-01  
**Status:** ✅ COMPLETE  
**Scope:** Open-source libraries and commercial/free TSA providers for RFC 3161 integration

---

## Executive Summary

This document evaluates open-source C/C++ libraries and commercial/free Timestamp Authority (TSA)
service providers for RFC 3161 compliance and integration suitability in ThemisDB.

**Current implementation:** ThemisDB uses OpenSSL's built-in TSP API (`openssl/ts.h`) with
libcurl for HTTP transport — see `src/security/timestamp_authority_openssl.cpp`.

**Recommendation:** Retain OpenSSL as the primary cryptographic backend; add Botan as a
compile-time alternative for environments where OpenSSL is restricted. For TSA services use
DigiCert (free) or Entrust (commercial/eIDAS) in production, with FreeTSA for development.

---

## 1. Open-Source C/C++ Libraries

### 1.1 OpenSSL (≥ 1.1.1) — **CURRENT / RECOMMENDED**

| Attribute | Detail |
|-----------|--------|
| License | Apache 2.0 |
| Language | C |
| RFC 3161 API | `openssl/ts.h` — `TS_REQ`, `TS_RESP`, `TS_TST_INFO`, `TS_ACCURACY` |
| Hash algorithms | SHA-1 (deprecated), SHA-224, SHA-256, SHA-384, SHA-512 |
| Transport | External (libcurl or application-supplied) |
| Platform | Linux, macOS, Windows, FIPS builds available |
| Maturity | Production — widely deployed, extensive CVE history and patch cadence |
| eIDAS suitability | ✅ Full — PKCS#7/CMS token support, certificate chain validation |

**Strengths:**
- Already integrated; zero additional dependency cost.
- `TS_REQ_new` / `i2d_TS_REQ` / `d2i_TS_RESP` / `PKCS7_to_TS_TST_INFO` cover the full TSP
  request/response cycle.
- FIPS 140-2/140-3 validated builds available (Red Hat, Ubuntu, AWS-LC).
- Strong community and long-term support guarantees.

**Weaknesses:**
- Low-level C API is verbose and error-prone (manual memory management).
- `openssl/ts.h` is not part of the "stable" public API promise; minor breakage possible across
  major versions (1.x → 3.x migration required NULL-safe accessor updates).

**Integration effort:** Already complete in `timestamp_authority_openssl.cpp`.

---

### 1.2 Botan (≥ 3.0) — **RECOMMENDED ALTERNATIVE**

| Attribute | Detail |
|-----------|--------|
| License | BSD 2-Clause |
| Language | C++20 |
| RFC 3161 API | `botan/tsp.h` (Botan ≥ 3.4) — `Botan::TSP::Request`, `Botan::TSP::Response` |
| Hash algorithms | SHA-256, SHA-384, SHA-512, SHA-3 |
| Transport | External |
| Platform | Linux, macOS, Windows; vcpkg/Conan packages available |
| Maturity | Production — actively maintained, used in GnuPG, KDE, Talos |
| eIDAS suitability | ✅ Full — CMS/PKCS#7, X.509 chain validation |

**Strengths:**
- Modern C++ API; RAII resource management eliminates the manual `free()` calls required by
  OpenSSL.
- Single library covers hashing, asymmetric crypto, X.509, CMS, and TSP.
- Easier to audit and fuzz because the codebase is smaller and more cohesive.
- Native `Botan::TSP::Response::verify()` validates the signature against an X.509 trust store
  in one call.

**Weaknesses:**
- TSP support landed in 3.4 (released 2024-Q4); older distro packages may not include it.
- Adds a new vcpkg dependency; currently ThemisDB uses OpenSSL everywhere.
- Smaller ecosystem than OpenSSL; fewer third-party integrations and audit reports.

**Integration effort:** Medium — `TSAConfig` and `TimestampAuthority` interface remain
unchanged; only `timestamp_authority_openssl.cpp` gains a sibling
`timestamp_authority_botan.cpp` compiled under `THEMIS_USE_BOTAN_TSA`.

**Sample snippet:**
```cpp
#include <botan/tsp.h>
#include <botan/x509cert.h>

Botan::TSP::Request req(hash, Botan::HashFunction::create_or_throw("SHA-256"));
req.set_nonce(nonce);
req.set_cert_req(true);

auto resp = Botan::TSP::Response::parse(raw_response_bytes);
resp.verify(trust_anchors);  // throws on failure
auto& tst = resp.tst_info();
```

---

### 1.3 GnuTLS (≥ 3.8) — **NOT RECOMMENDED**

| Attribute | Detail |
|-----------|--------|
| License | LGPLv2.1+ |
| RFC 3161 support | Partial — `gnutls_pkcs7_*` handles CMS tokens but no TSP request builder |
| Integration effort | High — request encoding must be hand-rolled with libtasn1 |
| eIDAS suitability | ⚠️ Incomplete — no built-in TST_INFO parser |

GnuTLS provides CMS/PKCS#7 decoding but lacks a first-class TSP request/response API.
Integration would require writing the ASN.1 encoding layer manually, negating any advantage
over the existing OpenSSL implementation.

---

### 1.4 libgcrypt + libksba — **NOT RECOMMENDED**

| Attribute | Detail |
|-----------|--------|
| License | LGPLv2.1 |
| RFC 3161 support | libksba covers X.509/CMS; no TSP-specific API |
| Integration effort | Very high — full TSP stack must be implemented from scratch |
| eIDAS suitability | ⚠️ Incomplete |

Used internally by GnuPG but not suitable for direct application use without significant
additional implementation effort.

---

### 1.5 Cryptlib (≥ 3.4.7) — **NICHE USE ONLY**

| Attribute | Detail |
|-----------|--------|
| License | Sleepycat-style (free for open source, commercial license required otherwise) |
| RFC 3161 support | Full — `CRYPT_SESSION_RTCS` / `cryptCreateSession` with TSP type |
| Integration effort | Medium |
| eIDAS suitability | ✅ Full |

Cryptlib has comprehensive TSP support and is certified to Common Criteria EAL3+. The
non-standard license makes it unsuitable for ThemisDB's distribution model unless a
commercial license is procured. Consider only if a CC-certified library is a hard requirement.

---

### 1.6 Summary Matrix — Libraries

| Library | RFC 3161 | eIDAS | License | C++ API | Recommended |
|---------|----------|-------|---------|---------|-------------|
| **OpenSSL 3.x** | ✅ Full | ✅ | Apache 2.0 | C (verbose) | ✅ Primary |
| **Botan 3.4+** | ✅ Full | ✅ | BSD-2 | C++20 (modern) | ✅ Alternative |
| GnuTLS 3.8 | ⚠️ Partial | ⚠️ | LGPL | C | ❌ |
| libgcrypt+libksba | ❌ None | ❌ | LGPL | C | ❌ |
| Cryptlib 3.4.7 | ✅ Full | ✅ | Proprietary | C | ⚠️ Niche |

---

## 2. TSA Provider Services

### 2.1 Free / Public Providers

#### DigiCert Timestamp Server — **RECOMMENDED (production-grade free)**

| Attribute | Detail |
|-----------|--------|
| URL | `https://timestamp.digicert.com` |
| Cost | Free (no registration) |
| Certificate | DigiCert Assured ID Root CA (publicly trusted) |
| Hash algorithms | SHA-256, SHA-384, SHA-512 |
| Rate limit | ~5 req/s (soft) |
| SLA | No formal SLA; best-effort, highly available |
| RFC 3161 | ✅ Compliant |
| eIDAS qualification | ❌ Not qualified (US-based) |
| Latency | 200–800 ms |

**Assessment:** Most reliable free option. Certificates chain to a globally trusted root — no
`verify_tsa_cert: false` workaround needed. Suitable for non-eIDAS production workloads.

---

#### FreeTSA (freetsa.org) — **RECOMMENDED (development / testing)**

| Attribute | Detail |
|-----------|--------|
| URL | `https://freetsa.org/tsr` |
| Cost | Free (no registration) |
| Certificate | Self-signed root (not publicly trusted) |
| Hash algorithms | SHA-256, SHA-384, SHA-512 |
| Rate limit | ~10 req/s (soft) |
| SLA | None — community-operated |
| RFC 3161 | ✅ Compliant |
| eIDAS qualification | ❌ Not qualified |
| Latency | 500–2000 ms |

**Assessment:** Best choice for CI and developer testing. Self-signed certificate requires
`verify_tsa_cert: false` in config. Not suitable for production or regulated workloads.

---

#### Sectigo Timestamp Server — **ACCEPTABLE (fallback)**

| Attribute | Detail |
|-----------|--------|
| URL | `http://timestamp.sectigo.com` |
| Cost | Free |
| Certificate | Sectigo RSA Time Stamping CA (publicly trusted) |
| Hash algorithms | SHA-256 |
| Rate limit | Undocumented |
| SLA | None |
| RFC 3161 | ✅ Compliant |
| eIDAS qualification | ❌ Not qualified |
| Latency | 300–1000 ms |

**Assessment:** Suitable as a secondary fallover after DigiCert. HTTP-only endpoint is a
concern for environments requiring TLS everywhere; acceptable only when wrapped in an
HTTPS proxy or for non-sensitive timestamps.

---

#### GlobalSign Free Timestamp — **ACCEPTABLE**

| Attribute | Detail |
|-----------|--------|
| URL | `http://timestamp.globalsign.com/tsa/r6advanced1` |
| Cost | Free |
| Certificate | GlobalSign Root CA R6 (publicly trusted) |
| Hash algorithms | SHA-256 |
| Rate limit | Undocumented |
| SLA | None |
| RFC 3161 | ✅ Compliant |
| eIDAS qualification | ❌ Not qualified |
| Latency | 200–600 ms |

**Assessment:** Useful third fallover option in a multi-provider failover chain. HTTP only.

---

### 2.2 European Qualified TSA Providers (eIDAS-Compliant)

#### DFN-PKI — **RECOMMENDED (EU/eIDAS, research/education)**

| Attribute | Detail |
|-----------|--------|
| Operator | DFN-Verein (German Research Network) |
| URL | `http://zeitstempel.dfn.de` |
| Cost | Free for DFN members; commercial license for others |
| Certificate | DFN-PKI Global G2 (publicly trusted) |
| eIDAS qualification | ✅ Qualified TSA (Germany, EU Trusted List) |
| Hash algorithms | SHA-256, SHA-512 |
| Rate limit | Undocumented; fair-use expected |
| RFC 3161 | ✅ Compliant |
| SLA | 99.5% (member agreement) |
| Latency | 200–800 ms |

**Assessment:** Best eIDAS-qualified option for academic and research deployments at no cost.
TSA certificate is included in major OS trust stores. Requires DFN membership for production
SLA; individual developers can use it under the public policy OID.

---

#### D-TRUST (Bundesdruckerei) — **RECOMMENDED (EU/eIDAS, commercial)**

| Attribute | Detail |
|-----------|--------|
| Operator | Bundesdruckerei GmbH |
| URL | Provided post-contract |
| Cost | Commercial (€500–2000/month depending on volume) |
| Certificate | D-TRUST Root Class 3 CA 2 2009 |
| eIDAS qualification | ✅ Qualified TSA (Germany, EU Trusted List) |
| Hash algorithms | SHA-256, SHA-384, SHA-512 |
| Rate limit | Volume-based per contract |
| RFC 3161 | ✅ Compliant |
| SLA | 99.9% |
| Latency | 100–500 ms |

**Assessment:** Premium eIDAS-qualified option for regulated industries (finance, healthcare,
government). Part of the German Trusted List and EU central Trust List. Recommended for
customers requiring a qualified electronic timestamp (QET) under eIDAS Article 42.

---

#### Entrust Timestamp Authority — **RECOMMENDED (global/enterprise)**

| Attribute | Detail |
|-----------|--------|
| Operator | Entrust, Inc. |
| URL | `https://timestamp.entrust.net/TSS/RFC3161sha2TS` |
| Cost | Included with code-signing certificate; standalone commercial plan available |
| Certificate | Entrust Root Certification Authority – G2 (publicly trusted) |
| eIDAS qualification | ✅ Qualified in multiple EU member states |
| Hash algorithms | SHA-256, SHA-384, SHA-512 |
| Rate limit | Plan-dependent |
| RFC 3161 | ✅ Compliant |
| SLA | 99.9% |
| Latency | 100–400 ms |

**Assessment:** Best choice for global enterprises requiring both high availability and eIDAS
qualification. Entrust certificates are universally trusted (Windows, macOS, Linux NSS).
No `ca_cert_path` override needed.

---

#### SwissSign TSA — **ACCEPTABLE (EU/eIDAS)**

| Attribute | Detail |
|-----------|--------|
| Operator | SwissSign AG |
| URL | Provided post-registration |
| Cost | Commercial |
| Certificate | SwissSign TSA Root (publicly trusted in major stores) |
| eIDAS qualification | ✅ Qualified (Switzerland/EU) |
| Hash algorithms | SHA-256, SHA-512 |
| Rate limit | Plan-dependent |
| RFC 3161 | ✅ Compliant |
| SLA | 99.9% |
| Latency | 200–800 ms (Swiss routing may add latency for non-European requests) |
| Auth | HTTP Basic Auth required |

**Assessment:** Viable for Swiss-regulated deployments or customers who already use
SwissSign PKI services. The HTTP Basic Auth requirement complicates secret management
compared to mTLS-based alternatives.

---

#### Deutsche Telekom Security — **ACCEPTABLE (EU/eIDAS)**

| Attribute | Detail |
|-----------|--------|
| Operator | Deutsche Telekom AG |
| URL | Provided post-contract |
| Cost | Commercial |
| eIDAS qualification | ✅ Qualified (Germany, EU Trusted List) |
| Hash algorithms | SHA-256, SHA-512 |
| RFC 3161 | ✅ Compliant |
| SLA | 99.9% |

**Assessment:** Viable for Deutsche Telekom ecosystem customers. Similar positioning to
D-TRUST (Bundesdruckerei); D-TRUST has more established documentation for third-party
integrations and is preferred unless existing Telekom contracts apply.

---

### 2.3 Summary Matrix — Providers

| Provider | Free | eIDAS | Trust | SLA | Recommended Role |
|----------|------|-------|-------|-----|-----------------|
| **DigiCert** | ✅ | ❌ | Global | None | Production (non-regulated) |
| **FreeTSA** | ✅ | ❌ | Self-signed | None | Development / CI |
| **DFN-PKI** | ✅* | ✅ | DE/EU | 99.5% | eIDAS (academic/research) |
| **Entrust** | ❌ | ✅ | Global | 99.9% | eIDAS (enterprise) |
| **D-TRUST** | ❌ | ✅ | DE/EU | 99.9% | eIDAS (regulated EU) |
| Sectigo | ✅ | ❌ | Global | None | Fallover |
| GlobalSign (free) | ✅ | ❌ | Global | None | Fallover |
| SwissSign | ❌ | ✅ | CH/EU | 99.9% | Swiss-regulated |
| Deutsche Telekom | ❌ | ✅ | DE/EU | 99.9% | Telekom customers |

\* Free for DFN members; commercial license required otherwise.

---

## 3. Integration Suitability for ThemisDB

### 3.1 Recommended Configuration by Deployment Tier

#### Development / CI

```yaml
timestamp_authority:
  url: "https://freetsa.org/tsr"
  hash_algorithm: "SHA256"
  verify_tsa_cert: false   # self-signed root
  timeout_seconds: 15
```

#### Production (non-regulated)

```yaml
timestamp_authority:
  url: "https://timestamp.digicert.com"
  hash_algorithm: "SHA256"
  verify_tsa_cert: true
  timeout_seconds: 30
  fallback_urls:
    - "http://timestamp.sectigo.com"
    - "http://timestamp.globalsign.com/tsa/r6advanced1"
```

#### Production (eIDAS-regulated)

```yaml
timestamp_authority:
  url: "https://<entrust-or-dtrust-endpoint>"
  hash_algorithm: "SHA256"
  cert_req: true
  verify_tsa_cert: true
  ca_cert_path: "/etc/ssl/certs/ca-bundle.crt"
  timeout_seconds: 30
  policy_oid: "<TSA-specific-policy-OID>"

eidas:
  enabled: true
  max_age_days: 10950   # 30 years
  qualified_tsps:
    - "Entrust"
    - "D-TRUST"
    - "DFN-PKI"
```

### 3.2 Failover Strategy

Implement a priority-ordered provider list with automatic fallover:

```cpp
std::vector<std::string> priority_urls = {
    "https://timestamp.digicert.com",         // primary
    "http://timestamp.sectigo.com",           // secondary
    "http://timestamp.globalsign.com/tsa/r6advanced1",  // tertiary
    "https://freetsa.org/tsr"                 // last-resort
};

// Populate TSAConfig
config.url = priority_urls[0];
config.fallback_urls.assign(priority_urls.begin() + 1, priority_urls.end());
```

The `TimestampAuthority` implementation tries each URL in order, passing it directly to the
internal HTTP layer without mutating the shared `config_` object, ensuring thread-safety.

For eIDAS deployments, restrict the fallback list to qualified providers only to maintain
compliance guarantees.

### 3.3 Multi-Library Build Strategy

Introduce a compile-time selection between OpenSSL and Botan:

```cmake
option(THEMIS_TSA_BACKEND "TSA cryptographic backend: openssl|botan" "openssl")

if(THEMIS_TSA_BACKEND STREQUAL "botan")
    target_compile_definitions(themis_core PUBLIC THEMIS_USE_BOTAN_TSA)
    find_package(Botan 3.4 REQUIRED)
    target_link_libraries(themis_core PRIVATE Botan::Botan)
    target_sources(themis_core PRIVATE src/security/timestamp_authority_botan.cpp)
else()
    target_compile_definitions(themis_core PUBLIC THEMIS_USE_OPENSSL_TSA)
    target_sources(themis_core PRIVATE src/security/timestamp_authority_openssl.cpp)
endif()
```

The `TimestampAuthority` public interface (`include/security/timestamp_authority.h`) is
unchanged by the backend switch.

---

## 4. Security Considerations

### 4.1 Certificate Pinning

For high-security deployments, pin the TSA root certificate to detect MITM attacks:

```yaml
timestamp_authority:
  ca_cert_path: "/path/to/pinned-digicert-root.pem"
  verify_tsa_cert: true
```

Ship pinned certificates as part of the ThemisDB release artefact and rotate on CA
certificate renewal events.

### 4.2 Nonce Replay Protection

The existing implementation already generates a cryptographically random 8-byte nonce via
`RAND_bytes()`. Verify nonce in the response matches the request to prevent timestamp
replay attacks.

### 4.3 Time Source Trust

TSA-provided time is only as trustworthy as the TSA operator. For legally binding
timestamps, use a qualified TSA from the EU Trusted List
(`https://eidas.ec.europa.eu/efda/tl-browser/`). Never rely solely on system clock time.

### 4.4 Hash Algorithm Lifecycle

| Algorithm | Status | Recommendation |
|-----------|--------|----------------|
| SHA-1 | ⚠️ Deprecated | Do not use; rejected by most modern TSAs |
| SHA-256 | ✅ Current | Default; widely supported |
| SHA-384 | ✅ Current | Use for 192-bit security margin |
| SHA-512 | ✅ Current | Use when maximum hash strength needed |

---

## 5. Compliance References

| Standard | Relevance |
|----------|-----------|
| **RFC 3161** | Internet X.509 PKI Time-Stamp Protocol (TSP) |
| **RFC 5816** | Updates RFC 3161 to require SHA-2 |
| **eIDAS (EU) 910/2014 Art. 42** | Qualified electronic time stamps |
| **ETSI EN 319 422** | TSP and timestamp token profiles |
| **ETSI TS 102 023** | Policy requirements for TSAs |
| **ISO 18014** | Time-stamping services |
| **NIST SP 800-102** | Recommendation for Digital Signature Timeliness |

---

## 6. Action Items

| # | Action | Priority | Target |
|---|--------|----------|--------|
| 1 | Add Botan TSA backend (`timestamp_authority_botan.cpp`) | Medium | Q3 2026 |
| 2 | Implement multi-provider failover in `TimestampAuthority` | High | Q2 2026 |
| 3 | Add DigiCert as default provider in `config/timestamp_authority.yaml` | High | Q2 2026 |
| 4 | Evaluate Entrust commercial contract for eIDAS production use | Medium | Q3 2026 |
| 5 | Add `THEMIS_TSA_BACKEND` CMake option for OpenSSL/Botan selection | Low | Q3 2026 |
| 6 | Update EU Trusted List cache for `isQualifiedTSA()` validation | Medium | Q3 2026 |

---

## References

- [OpenSSL TS API](https://www.openssl.org/docs/man3.0/man3/TS_REQ_new.html)
- [Botan TSP module](https://botan.randombit.net/handbook/api_ref/tsp.html)
- [EU Trusted List Browser](https://eidas.ec.europa.eu/efda/tl-browser/)
- [IETF RFC 3161](https://www.rfc-editor.org/rfc/rfc3161)
- [IETF RFC 5816](https://www.rfc-editor.org/rfc/rfc5816)
- [FreeTSA](https://freetsa.org/)
- [DigiCert Timestamp Service](https://knowledge.digicert.com/solution/SO912.html)
- [DFN-PKI Zeitstempel](https://www.pki.dfn.de/faqpki/faqzeitstempel/)
- [Entrust Timestamp Authority](https://www.entrust.com/pki/pki-solutions/code-signing/)
- [D-TRUST TSA](https://www.d-trust.net/en/products/timestamp-service.html)
- `docs/en/security/TSA_SETUP.md` — ThemisDB TSA configuration guide
- `docs/RFC3161_IMPLEMENTATION_SUMMARY.md` — Implementation summary
- `src/security/timestamp_authority_openssl.cpp` — Current implementation

---

**Document Version:** 1.0  
**Last Updated:** 2026-03-01  
**Status:** ✅ FINAL
