# Plan: Fix ODR TimestampAuthority blocker (#5939)

- Verify duplicate `TimestampAuthority` definitions and include guards.
- Apply minimal fix by keeping a single OpenSSL implementation path and removing duplicate definitions.
- Add focused build validation for `libthemis_security` target.
- Run targeted tests for timestamp authority if available.
- Run secret scan and CodeQL checker before finalization.
