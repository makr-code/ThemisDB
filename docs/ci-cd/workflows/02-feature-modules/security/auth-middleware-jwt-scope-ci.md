# Auth Middleware JWT Scope Enforcement CI

> [!WARNING]
> Historische Workflow-Dokumentation (Legacy): Diese Seite beschreibt eine fruehere CI-Generation.
> Der verbindliche aktuelle Stand ist der 8-Workflow-Kern in `.github/WORKFLOW_REGISTRY.md`.

🔄 **CI/CD**

> **Workflow-Datei (historisch):** .github/workflows/02-feature-modules_security_auth-middleware-jwt-scope-ci.yml
> **Aktueller Stand:** .github/WORKFLOW_REGISTRY.md

## Aufgabe

CI-Workflow zur automatischen Überprüfung und Validierung von: **Auth Middleware JWT Scope Enforcement**.

## Auslöser (Triggers)

- **`push`** — Automatisch bei jedem Push auf die konfigurierten Branches (Branches: `main`, `develop`) (9 überwachte Pfade)
- **`pull_request`** — Automatisch bei Pull Requests (opened, synchronize, reopened) (9 überwachte Pfade)
- **`workflow_dispatch`** — Manuell über die GitHub Actions UI ausführbar

## Nebenläufigkeit

- **Gruppe:** `${{ github.workflow }}-${{ github.ref }}`
- **Cancel-in-progress:** Ja

## Jobs

### `ci-scope-classifier`
**Typ:** Reusable Workflow Call
**Verwendet:** `Historisch (fruehere CI-Generation). Aktueller Stand: .github/WORKFLOW_REGISTRY.md`

### `auth-middleware-scope-tests`
**Anzeigename:** Auth Middleware Scope (${{ matrix.os }} / ${{ matrix.compiler }})

**Läuft auf:** `${{ matrix.os }}`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`
**Matrix:** 2 Konfiguration(en)

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Install build dependencies** — `sudo apt-get update -qq`
- **Configure CMake (COMMUNITY edition)** — `cmake -S . -B build \`
- **Build test_auth_middleware** — `cmake --build build \`
- **Run AuthMiddlewareFocusedTests (CTest)** — `set -o pipefail`
- **Run all Auth Middleware tests via focused binary** — `set -o pipefail`
- **Upload test results** — `actions/upload-artifact@v4`

### `auth-middleware-docs-gate`
**Anzeigename:** Auth Middleware — Documentation Gate

**Läuft auf:** `ubuntu-24.04`
**Abhängigkeiten:** `ci-scope-classifier`
**Bedingung:** `needs.ci-scope-classifier.outputs.has_code_changes == 'true'`

**Schritte:**

- **Checkout repository** — `actions/checkout@v4`
- **Verify JWTClaims scopes field exists** — `grep -q "scopes" include/auth/jwt_validator.h || \`
- **Verify setRoleScopeMapping public API exists** — `grep -q "setRoleScopeMapping" include/server/auth_middleware.h || \`
- **Verify rbac_roles.yaml exists** — `test -f config/security/rbac_roles.yaml || \`
- **Verify scope/scp claim parsing in jwt_validator.cpp** — `grep -q '"scope"' src/auth/jwt_validator.cpp && \`
- **Verify scope enforcement in authorizeViaJWT** — `grep -q "JWT missing required scope" src/server/auth_middleware.cpp || \`
- **Verify scope enforcement in authorizeViaKerberos** — `grep -q "Kerberos principal missing required scope" src/server/auth_middleware.c`

## Verwandte Ressourcen

- [Workflow-Datei](../../../.github/workflows/02-feature-modules_security_auth-middleware-jwt-scope-ci.yml)
- [Alle Workflows](../README.md)


