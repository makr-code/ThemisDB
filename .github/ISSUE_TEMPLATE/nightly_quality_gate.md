---
name: Nightly Quality Gate Alert
about: Track failing nightly quality gate runs (GoogleTest/Google Benchmark JSON evidence)
title: "[Nightly Quality Gate] FAIL on "
labels: ["nightly-quality-gate", "ci", "quality-gate", "area:ai"]
assignees: []
---

## Summary

Nightly quality gate reported a failure and requires triage.

## Gate Result

- Status: FAIL
- Failure criteria:
  - GoogleTest failures > 0
  - OR no GoogleTest JSON evidence
  - OR no Google Benchmark JSON evidence

## Run Metadata

- Branch:
- Commit:
- Workflow run URL:
- Trigger event:

## GoogleTest Metrics

- JSON files:
- Total tests:
- Total failures:

## Google Benchmark Metrics

- JSON files:
- Total benchmark entries:

## Triage Checklist

- [ ] Reproduce the failure locally or in rerun
- [ ] Identify failing test/benchmark scope
- [ ] Decide: code fix vs. flaky-infra mitigation
- [ ] Add/update regression tests if needed
- [ ] Verify nightly gate returns to PASS

## Recovery Condition

Issue can be closed automatically or manually once a later nightly run passes.

## Additional Notes

Add logs, links, and root-cause details here.
