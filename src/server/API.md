---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-11-13"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: api-contract
Canonical references: `include/server/api_gateway.h`, `include/server/auth_middleware.h`, `src/server/api_gateway.cpp`, `src/server/auth_middleware.cpp`, `tests/server/test_api_gateway.cpp`
Related issues: #7093, #7167

# Server API contract

## Public surfaces

- `APIGateway::handleRequest()` receives an HTTP request and a local-handler callback and returns an HTTP response.
- `APIGateway::Config` controls optional routing/admission features, request limits/timeouts, metrics, and API version checks. Runtime behavior depends on these settings and the dependencies supplied at construction.
- `AuthMiddleware` exposes configuration for JWT, Kerberos, mTLS, API-key, and token paths. Authorization returns `AuthResult`, including `authorized`, user/tenant context, and a reason.
- `HttpServer` enables JWT validation when `THEMIS_JWT_JWKS_URL` is set. Both `THEMIS_JWT_EXPECTED_ISSUER` and `THEMIS_JWT_EXPECTED_AUDIENCE` are required; incomplete configuration fails server construction. `THEMIS_JWT_TENANT_CLAIM` optionally selects the signed claim mapped to `AuthResult.tenant_id` (default `tenant_id`). Without the JWKS URL, JWT validation is not enabled by this bootstrap.
- `THEMIS_TOKEN_RAG` configures a static `rag_reader` identity with only `llm` and `data:read` scopes for deployments that need a dedicated RAG caller. Its value must differ from configured admin, readonly, and analyst tokens; a collision fails server construction.
- `THEMIS_TOKEN_RAG_TENANT` optionally binds that static RAG identity to one configured tenant. It is trusted server configuration, not a request-header value; it has no effect unless `THEMIS_TOKEN_RAG` is set. Without it, the RAG token has no tenant scope.
- JWT authentication establishes a verified identity and optional tenant claim, not a collection grant. HTTP RAG requires an explicit matching policy for authenticated non-admin callers; an empty policy set denies this route. Configure policies using `THEMIS_POLICIES_PATH` or `config/policies.yaml|yml|json`; native collection grants match the validated user ID (or an explicitly configured subject wildcard), action `data.read`, and the exact collection resource (or an explicitly configured resource wildcard). The existing LLM route gate also requires an `llm` policy for `/api/v1/llm/` when policies are configured. The existing default-allow behavior remains for other policy callers. Other PolicyEngine callers retain prefix matching, and policies remain first-match-wins. OPA results are honored as configured, so external policy rules must enforce exact collection matching themselves.
- HTTP RAG's collection-policy requirement applies only when authentication is enabled. Existing auth-disabled mode treats policy checks as advisory, and authenticated `admin` bypasses policy checks by design. Tenant filtering remains independent of collection authorization.
- The HTTP RAG route carries user and tenant identity from its successful `requireAccess()` decision into retrieval; it does not revalidate the bearer token to construct retrieval context.
- `POST /api/v1/llm/aql/rag` invokes `LLMAQLHandler::executeRAG()` over a request-local `QueryEngine` composed from the server's live RocksDB and index managers. It accepts `query` (required string), `collection` (default `default`), `top_k` (default 5, range 1–1000), `max_tokens` (default 512, range 1–4096), and optional `similarity_threshold` (finite number), `vector_field` (1–256 bytes), `temperature` (finite 0–2), and `top_p` (finite 0–1). It does not accept caller-selected tenant or LoRA identity. The authenticated context and tenant come from the same successful access decision; the collection predicate and required `data:read` policy bind retrieval to that exact collection. A valid W3C `traceparent` is propagated to inference; malformed or absent trace context is ignored.
- The AQL-backed RAG endpoint returns `{text, retrieval_attempted, collection, top_k}` on success. Invalid JSON/options return 400, denied scope or collection policy returns 403, missing retrieval dependencies return 503, cooperative deadline expiry returns 504, and other retrieval/inference failures return 500. QueryEngine and inference calls may not be interrupted while blocked, so the deadline is cooperative.
- The HttpServer bootstrap does not apply `THEMIS_JWT_SCOPE_CLAIM`; JWT scope authorization uses parsed `scope`/`scp` claims or configured role-to-scope mappings.

## Caller obligations and failure cases

- Configure and inject the checks required by the deployment; do not assume gateway features are active from the existence of an API alone.
- Inspect `AuthResult.authorized` before using its identity context, and preserve the denial reason where the caller reports or audits the result.
- Route/version normalization, admission rejection, and handler execution are separate branches; preserve the intended request path when changing one.
- Avoid assigning a universal HTTP status mapping unless it is defined by the changed handler or response code.

## Validation references

Use `tests/server/test_api_gateway.cpp`, `test_server_contract_hardening_focused.cpp`, and `tests/server/test_rag_collection_policy.cpp` for the HTTP RAG auth, tenant-isolation, and AQL-backed route contract.

See `IMPLEMENTATION.md` for source mapping and `MODULE_GUIDE.md` for maintainer workflow.
