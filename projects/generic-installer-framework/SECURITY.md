# SECURITY

## Supported Versions

Pre-1.0 versions are considered actively evolving. Security fixes target the latest minor release line.

## Security Baseline

- Manifest signatures are mandatory
- Artifact SHA-256 verification is mandatory
- TLS is mandatory for remote sources
- Downgrade is blocked by default policy

## Threat Model (MVP)

- Tampered release manifests
- Tampered artifact payloads
- Replay/downgrade attempts
- Partial or interrupted update application

## Disclosure

Report vulnerabilities privately to project maintainers before public disclosure.

Include:

- Affected component and version
- Reproduction steps
- Impact assessment
- Optional mitigation ideas

## Hardening Requirements

- No unsigned execution-relevant configuration in production mode
- Security-relevant decisions must be audit logged
- Secrets and keys must never be written to plain logs
