# Voice Public Header Roadmap

<!-- Source review: 2026-10-31. This page tracks documentation/planning only. -->

## Current inventory

There are 23 public headers under `include/voice/`. This corrects the
18-header count in the prior snapshot. The current header-to-type map is
[`ARCHITECTURE.md`](ARCHITECTURE.md); exact declarations and API comments in
the headers are authoritative.

## Documentation follow-up

- [x] Replace the outdated header/type map with a map checked against the
  current header declarations.
- [x] Mark the old session-contract snapshot non-normative where it conflicts
  with current source.
- [ ] Keep header comments and focused tests synchronized with any future
  public contract changes.
- [ ] Add release/version migration guidance only when supported by the active
  root release/versioning policy and an approved API change.

## Boundary

This file is not a runtime implementation roadmap or a release commitment.
Implementation and evidence status are in
[`../../src/voice/ROADMAP.md`](../../src/voice/ROADMAP.md); aggregate maturity
and release blockers are in root [`../../ROADMAP.md`](../../ROADMAP.md).
