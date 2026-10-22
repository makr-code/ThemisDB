---
name: Release Review / SOC Sign-Off
about: Standardized human approval record for release publication and distribution gates with SOC evidence capture.
title: "[release-review] "
labels: ["release-review", "status/needs-approval", "security"]
assignees: []
Author: "ThemisDB Team"
Created: "2026-10-22"
Last Updated: "2026-10-22"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

## Summary

Describe the release or publication lane that needs human approval before public distribution.

## Release Metadata

- **Release Version / Tag:** <!-- e.g. v2.5.0 -->
- **Edition / Channel:** <!-- community / minimal / enterprise / military / nightly / prerelease -->
- **GitHub Release URL:** <!-- link to draft or published release -->
- **Target Publication Lane:** <!-- GitHub Release / Docker / WinGet / Linux distro / Windows distro -->
- **Requested By:** <!-- maintainer or release owner -->
- **Approver / Reviewer:** <!-- required maintainer GitHub handle -->

## Release Evidence Checklist

- [ ] Release notes and changelog are complete and reviewed
- [ ] Artifact checksums were generated and verified
- [ ] GPG/signing evidence is attached or linked
- [ ] SBOM / provenance evidence exists for the packaging lane
- [ ] Downstream dependency matrix was checked for blockers
- [ ] Security review / risk notes are documented for this release
- [ ] Known issues or rollback notes are captured below

## SOC / Compliance Evidence

- [ ] A release evidence record was created or linked via `.github/workflows/release-evidence-chain.yml`
- [ ] Artifact digest(s) recorded for the exact version being approved
- [ ] Approver identity and role are recorded in the evidence trail
- [ ] Any hold / reject / rollback decisions include rationale and follow-up owner
- [ ] The release owner confirms the chosen publication lane matches the intended risk profile

## Publication Preconditions

- [ ] GitHub Release exists and is the correct source-of-truth artifact
- [ ] Registry target is explicit: `ghcr.io`, `winget`, distro repository, or GitHub release
- [ ] Manifest / installation metadata matches the signed release payload
- [ ] No blocking CVE, dependency, or critical platform regression remains open
- [ ] Manual approval is limited to `admin` or `maintain` users only

## Decision Log

- **Approval Command:** <!-- /publish-release, /approve-docker, /approve-linux, etc. -->
- **Status:** <!-- approved / hold / reject / rollback -->
- **Decision Time (UTC):** <!-- ISO 8601 -->
- **Decision Rationale:** <!-- short but explicit human rationale -->
- **Rollback Owner / Follow-up:** <!-- who owns remediation or rollback -->

## Validation Summary

- [ ] Required artifacts downloaded and verified locally or in CI
- [ ] Release is safe to publish for the selected environment
- [ ] Approval is recorded with evidence and can be audited later

## Additional Notes

Add links to the relevant workflow runs, changelog entries, security findings, or operational evidence here.
