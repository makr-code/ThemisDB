<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Analytics Module Public Headers

**Module Path:** `include/analytics/`
**Implementation Security:** `../../src/analytics/SECURITY.md`

---

## Scope

Security considerations for the public API surface of the analytics module headers.
Covers data exposure risks in analytics results, ML model integrity, and cross-tenant
data isolation in distributed analytics.

---

## Threat Model

| Threat | Vector | Mitigation |
|--------|--------|-----------|
| Cross-tenant data leakage in OLAP results | Shared columnar buffer aliasing | `IOLAPEngine` contract requires tenant-scoped query contexts |
| Model poisoning via `IAutoMLEngine` | Malicious training data injection | Input validation defined in `automl.h`; pipeline hooks for data provenance |
| ML inference side-channel | Timing-based model extraction | `IMLServingEngine` rate-limiting hooks |
| PII exposure in CEP event stream | Unredacted events in `ICEPEngine` output | `CEPEvent` schema includes `pii_fields` redaction list |
| Arrow Flight data exfiltration | Unathenticated Arrow Flight endpoint | `IArrowFlightServer` requires auth token in `FlightDescriptor` |
| Anomaly detection bypass | Adversarial input crafted to evade detection | Documented limitation; model retraining pipeline recommended |

---

## Security Controls

### Tenant Isolation
All analytics interfaces (`IOLAPEngine`, `IDistributedAnalytics`, `ICEPEngine`) accept a
`TenantContext` parameter; cross-tenant result mixing is a contract violation.

### PII Redaction Hooks
`CEPEvent` and `TextAnalysisResult` include a `pii_field_mask` to allow redaction before
results are surfaced to callers.

### Arrow Flight Authentication
`IArrowFlightServer::connect()` requires a `FlightCallOptions` bearer token; unauthenticated
connections are rejected.

### Model Input Validation
`ModelInferRequest` defines `max_input_tokens` and `input_schema` constraints enforced before
inference dispatch.

---

## Known Limitations

- Federated analytics (`IFederatedAnalytics`, planned Q4 2026) will require differential
  privacy guarantees not yet present in the header contract.
- ML model integrity verification (supply-chain signing) is operator-managed; the module
  does not enforce model provenance at the header API level.
- NLP text analysis may inadvertently retain PII in intermediate buffers; callers are
  responsible for applying `pii_field_mask` on `TextAnalysisResult`.
