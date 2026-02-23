# Labels Registry and Governance Mapping

## Label Categories

### Type
- **Feature**: New functionality or improvement.
- **Bug**: Issues that are broken or not functioning as intended.
- **Documentation**: Updates or changes to documentation.
- **Task**: Miscellaneous tasks that need to be completed.

### Priority
- **High**: Requires immediate attention.
- **Medium**: Important but not urgent.
- **Low**: Can be addressed at a later time.

### Status
- **Open**: Issue is open and awaiting resolution.
- **In Progress**: Work is currently being done on the issue.
- **Resolved**: Issue has been resolved.
- **Closed**: Issue is completed and closed.

### Area Labels
- **Frontend**: Relates to the client-side development.
- **Backend**: Relates to server-side development.
- **Testing**: Related to quality assurance and testing processes.

### Copilot Dispatcher Labels

These labels are managed by the [Copilot Issue Dispatcher](.github/copilot-dispatcher.md) automation.

**Issue labels**
- **`queue/copilot`** — Issue is eligible for automatic Copilot processing.
- **`in-progress/copilot`** — Issue has been claimed by the dispatcher; a Copilot PR exists for it.

**PR labels**
- **`pr/copilot`** — PR was created by the dispatcher.
- **`copilot/status-working`** — Copilot is actively working on this PR. Counts against the 5-slot WIP limit.
- **`copilot/status-ready-requested`** — Copilot signals it is done; the readiness gate will promote to `copilot/status-ready` once CI checks and the Copilot review are green.
- **`copilot/status-ready`** — Copilot work is complete and all gates are green. PR no longer counts against the WIP limit.
- **`copilot/status-blocked`** — Copilot cannot proceed. Requires human intervention.

## Migration Map from Old Labels to New Standardized Schema
| Old Label Name       | New Label Name         |
|----------------------|------------------------|
| `feature`            | `Type: Feature`        |
| `bug`                | `Type: Bug`            |
| `documentation`      | `Type: Documentation`   |
| `task`               | `Type: Task`           |
| `high-priority`      | `Priority: High`       |
| `medium-priority`    | `Priority: Medium`     |
| `low-priority`       | `Priority: Low`        |
| `open`               | `Status: Open`         |
| `in-progress`        | `Status: In Progress`  |
| `resolved`           | `Status: Resolved`     |
| `closed`             | `Status: Closed`       |
| `frontend`           | `Area: Frontend`       |
| `backend`            | `Area: Backend`        |
| `testing`            | `Area: Testing`        |
