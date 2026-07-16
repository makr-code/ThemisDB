# Security - Voice Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via SECURITY.md.

## Threat Model

| Threat | Mitigation surface |
|---|---|
| unauthorized voice session initiation | session and authentication guard paths |
| replay or spoof attempts on biometric flows | authenticator and voice security controls |
| oversized or malformed streaming input | bounded streaming/session handling |
| sensitive transcript leakage | storage and processing safeguards with controlled handling |
| telephony/browser injection or misuse | telephony and streaming validation paths |

## Security Controls

- voice authentication and security checks are integrated in dedicated module paths
- session and streaming behavior enforce bounded lifecycle controls
- storage and processing paths expose explicit error/failure signaling
- operational diagnostics support audit and incident triage

## Known Limitations

- anti-spoofing effectiveness depends on deployment configuration and model profile
- telephony/browser threat handling requires continuous regression coverage
- environment-dependent integrations can alter security control envelopes

## Sourcecode Verification (Module: voice/security)

- Verified files:
  - src/voice/voice_authenticator.cpp
  - src/voice/voice_security.cpp
  - src/voice/voice_session_manager.cpp
  - src/voice/voice_browser_streaming.cpp
  - src/voice/voice_telephony.cpp
  - src/voice/voice_error_handler.cpp
- Verified controls:
  - auth and session guard behavior
  - streaming/telephony validation surfaces
  - failure handling and security-related diagnostics
