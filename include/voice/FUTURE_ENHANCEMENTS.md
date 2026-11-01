# Voice Public Header — Planning Notes

<!-- Reviewed 2026-10-31. Ideas only; no delivery commitment or API status implied. -->

Potential follow-up work, subject to maintainer review and source evidence:

- Document stable, tested behavior for callback/back-end injection points where
  the current header contract is incomplete.
- Clarify which telephony/streaming interfaces are abstractions versus verified
  protocol integrations; do not imply SIP/WebRTC conformance from names alone.
- Add explicit security/privacy boundaries to API comments when a helper's
  in-process behavior could be mistaken for durable or cryptographic behavior.
- Establish API compatibility and migration notes from the active release
  policy before making a public breaking change.

The prior plan contained unverified claims about completed fail-closed
guarantees and “stable” API statuses. Those claims are not carried forward.
Current interface declarations are in the adjacent headers; implementation
planning belongs to [`../../src/voice/FUTURE_ENHANCEMENTS.md`](../../src/voice/FUTURE_ENHANCEMENTS.md).
