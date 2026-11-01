# Voice Module — Future Enhancements

<!-- Planning notes reviewed 2026-10-31. These are proposals, not delivery commitments or status evidence. -->

## Evidence-led follow-up

- Capture representative-hardware p95/p99 results for the registered voice
  benchmark cases and retain reproducible artifacts.
- Extend integration coverage for backend availability, streaming teardown and
  sustained multi-session behavior where current source/tests leave gaps.
- Validate the browser streaming placeholder path is replaced by an explicitly
  injected, supported transcription and synthesis integration before relying
  on it for user-facing speech.
- Review persistence, privacy, audit durability and access-control needs at
  the application/service boundary; in-process voice helpers do not provide
  these deployment guarantees.

## Constraints for changes

- Preserve fail-closed handling at validation, intent, authentication and
  liveness boundaries; do not broaden fallback behavior without evidence.
- Keep limits specific to each API surface rather than presenting them as
  global voice settings.
- Update the relevant public header, focused tests and module docs together
  when behavior or a public contract changes.
- Keep performance targets separate from measured results and distinguish
  fixtures/mocks from real backend measurements.

Implementation/status authority remains the source and headers plus
[`ROADMAP.md`](ROADMAP.md); aggregate maturity belongs to root
[`../../ROADMAP.md`](../../ROADMAP.md).
