---
name: "Security Hardening Review"
description: "Run a focused security and reliability review for ThemisDB changes and return findings-first output with concrete mitigations."
argument-hint: "Paste changed files, PR context, or module scope"
agent: "themisdb-reviewer"
---

Review the provided change scope with emphasis on security hardening and operational reliability.

## Inputs

- Review scope: ${input}
- Optional threat context: auth, transport, secrets, plugin loading, serialization, filesystem, network

## Required Checks

1. Authentication and authorization bypass risks
2. Input validation and deserialization safety
3. Resource lifetime, ownership, and cleanup under failures
4. Concurrency hazards (races, deadlocks, lock order, atomic misuse)
5. Error-handling quality (silent failures, swallowed errors, dangerous defaults)
6. Sensitive-data exposure in logs, metrics, or errors
7. Test gaps for negative and adversarial paths

## Output Requirements

Return:

1. Findings first (Critical -> High -> Medium -> Low)
2. For each finding: evidence, impact, minimal remediation
3. Open assumptions and what to verify next
4. Suggested focused tests or checks
