---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-30"
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

## Caller obligations and failure cases

- Configure and inject the checks required by the deployment; do not assume gateway features are active from the existence of an API alone.
- Inspect `AuthResult.authorized` before using its identity context, and preserve the denial reason where the caller reports or audits the result.
- Route/version normalization, admission rejection, and handler execution are separate branches; preserve the intended request path when changing one.
- Avoid assigning a universal HTTP status mapping unless it is defined by the changed handler or response code.

## Validation references

Use `tests/server/test_api_gateway.cpp`, `test_server_contract_hardening_focused.cpp`, and the nearest auth/handler test registered in `tests/server/CMakeLists.txt`.

See `IMPLEMENTATION.md` for source mapping and `MODULE_GUIDE.md` for maintainer workflow.
