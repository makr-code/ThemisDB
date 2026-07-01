---
name: Epic Branch Flow PR
about: Use for PRs that either merge feature work into an EPIC branch or integrate an EPIC branch into develop.
title: "[EPIC #<id>] <short summary>"
labels: ""
assignees: ""
---

# EPIC Branch Flow Pull Request

## EPIC Context

- EPIC issue: <!-- e.g. #5518 -->
- Sub-issue(s): <!-- e.g. #5467, #5468 -->
- Flow type:
  - [ ] Feature branch -> EPIC branch
  - [ ] EPIC branch -> develop

## Branch Routing (Required)

- Source branch: <!-- e.g. feature/5518-planner-freshness -->
- Target branch: <!-- e.g. epic/hybrid-boundaries-5518 or develop -->
- [ ] Target matches EPIC governance in the parent EPIC issue
- [ ] No direct feature -> develop merge

## Change Summary

<!-- Describe what changed and why in the EPIC context -->

## Linked Issues

<!-- Required: include EPIC and sub-issues -->
- Closes/Fixes/Refs: 

## Risk and Rollback

- Risk level:
  - [ ] Low
  - [ ] Medium
  - [ ] High
- Rollback plan:

## Validation Evidence

- [ ] Build passed
- [ ] Relevant focused tests passed
- [ ] Integration checks passed (if EPIC -> develop)
- [ ] Scanner/quality gates reviewed for changed scope

### Commands / Artifacts

<!-- Paste exact commands and key output paths -->

## EPIC Acceptance Impact

- [ ] Moves one or more EPIC acceptance criteria forward
- [ ] EPIC issue checklist/status updated if needed
- [ ] Follow-up items captured (if any)

## Final Checklist

- [ ] Branch naming follows EPIC convention
- [ ] PR description references EPIC and sub-issues
- [ ] No unresolved Critical/High findings without explicit maintainer acceptance
- [ ] Documentation updated where behavior/contracts changed
