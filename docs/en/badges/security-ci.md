# Security CI Badge

[![Security CI](https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_security_security-hardening-ci.yml/badge.svg?branch=develop)](https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_security_security-hardening-ci.yml?query=branch%3Adevelop)

## What it shows

The result of the most recent run of the **Security Hardening CI** workflow. This workflow triggers on changes to authentication, security, and JWT-related sources (`src/auth/`, `src/security/`, related tests) and validates that security-critical code paths build and pass their dedicated test suite.

## What it does NOT guarantee

- A passing badge covers only the security-hardening workflow, not the full CI matrix.
- Third-party dependency vulnerabilities are tracked separately via the `security.yml` scanning workflow.

## Source of truth

| Source | URL |
|--------|-----|
| Workflow file | [`.github/workflows/05-quality_security_security-hardening-ci.yml`](../../../.github/workflows/05-quality_security_security-hardening-ci.yml) |
| All workflow runs | <https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_security_security-hardening-ci.yml> |

## How contributors can verify

1. Go to the [Actions tab](https://github.com/makr-code/ThemisDB/actions) on GitHub.
2. Select the **Security Hardening CI** workflow.
3. Review the most recent run for detailed step-by-step logs.
