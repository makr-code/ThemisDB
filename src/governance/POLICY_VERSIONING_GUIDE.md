---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Policy Versioning and Change Management

This guide is aligned to the current declarations in
`policy_version_history.h`, `policy_change_manager.h`, and
`policy_approval_workflow.h`. It replaces prior claims of complete automatic
versioning, guaranteed cross-policy atomicity, fixed performance/scalability
limits, and regulatory audit compliance. Those guarantees are not established
by the presence of these interfaces.

## Version history

`PolicyVersionHistory` exposes `recordVersion()`, version-list/look-up methods,
`getLatestVersion()`, `getPreviousVersion()`, version comparison, audit record
query, and JSON import/export. `PolicyRuleVersion` contains a version string,
rule identifier, author, timestamp, change description, and JSON snapshot.
Whether a mutation is recorded depends on the specific manager/path. Inspect
the current implementation before assuming that every policy creation, update,
or lifecycle operation is automatically represented in the history.

The version string is documented as semantic `major.minor.patch`, but this page
does not assert that every update automatically selects or validates a version
increment. The history API exposes import/export; durable storage behavior
depends on the caller's use of those methods.

## Rollback and approval APIs

`PolicyChangeManager` declares safety-check/preview methods, single and
coordinated rollback methods, rollback history, reversal, and change-data
file import/export. Check each returned `RollbackSafetyReport` and
`RollbackOperation`; declarations do not establish atomicity across external
systems or storage.

`PolicyApprovalWorkflow` declares transitions among `DRAFT`, `REVIEW`,
`APPROVED`, `ACTIVE`, and `DEPRECATED`, including submission, approval,
rejection, activation, rollback approval, and emergency override. The API
records workflow status/history in its object; caller authentication,
authorization, durable audit storage, and emergency approval policy are not
provided by this summary.

## Suggested caller sequence

1. Construct and configure the relevant manager/history/workflow objects.
2. Record rule-version changes explicitly and inspect returned identifiers.
3. Before rollback, call `checkRollbackSafety()` and `previewRollback()`;
   review warnings/conflicts and apply the deployment's approval process.
4. Call the supported rollback method, inspect `success` and error details, and
   verify resulting policy state using the owning manager.
5. Persist/export history only through an explicitly configured storage path;
   preserve records according to the deployment's own retention policy.

Do not treat example code from earlier revisions as a complete runbook. No
versioning/approval test currently participates in the governance module-focused
CMake loop: `test_policy_versioning_and_approval.cpp` is explicitly skipped
there. The test source remains useful evidence but is not a registered test
result.

## References

- [`policy_version_history.h`](../../include/governance/policy_version_history.h)
- [`policy_change_manager.h`](../../include/governance/policy_change_manager.h)
- [`policy_approval_workflow.h`](../../include/governance/policy_approval_workflow.h)
- [Governance test CMake](../../tests/governance/CMakeLists.txt)
- [Module implementation guide](IMPLEMENTATION.md)
