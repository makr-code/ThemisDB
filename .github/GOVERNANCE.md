# Governance Model for ThemisDB

This document defines the authoritative governance standards for all issues, pull requests, milestones, and repository metadata in the ThemisDB project. All contributors and automated agents **MUST** comply with these standards.

> Referenced by [copilot-instructions.md](copilot-instructions.md) as the binding standard for label schema, milestone structure, relationships, and issue metadata.

---

## Labels

### Mandatory Label Schema

Every issue and pull request **MUST** carry exactly one label from each of the following four categories:

#### `area:*` — Module / Domain

| Label | Description |
|-------|-------------|
| `area:acceleration` | CUDA/Vulkan GPU acceleration |
| `area:analytics` | Analytics and OLAP |
| `area:api` | HTTP/gRPC API layer |
| `area:aql` | AQL query language and NL-to-AQL |
| `area:auth` | Authentication and authorization |
| `area:cache` | Caching layer |
| `area:cdc` | Change-data-capture |
| `area:chimera` | Multi-database adapter layer |
| `area:config` | Configuration subsystem |
| `area:content` | Content extraction (PDF, OCR, audio) |
| `area:core` | Core framework and DI |
| `area:docs` | Documentation system, MkDocs |
| `area:exporters` | Data exporters |
| `area:geo` | Geospatial module |
| `area:governance` | Governance and compliance |
| `area:gpu` | GPU device management |
| `area:graph` | Graph query engine |
| `area:importers` | Data importers |
| `area:index` | Index management |
| `area:ingestion` | Data ingestion connectors |
| `area:llm` | LLM inference integration |
| `area:metadata` | Metadata storage and MVCC |
| `area:network` | Network layer |
| `area:observability` | Metrics, tracing, SLO |
| `area:performance` | Performance / benchmarking |
| `area:plugins` | Plugin system |
| `area:prompt_engineering` | Prompt engineering |
| `area:query` | Query execution engine |
| `area:rag` | Retrieval-Augmented Generation |
| `area:replication` | Replication and WAL |
| `area:scheduler` | Task scheduler |
| `area:search` | Full-text and vector search |
| `area:security` | Security hardening |
| `area:server` | HTTP server core |
| `area:sharding` | Distributed sharding |
| `area:storage` | Storage backend (RocksDB/MVCC) |
| `area:temporal` | Temporal data handling |
| `area:timeseries` | Time-series module |
| `area:transaction` | Transaction coordinator |
| `area:training` | Model fine-tuning and training |
| `area:updates` | Update propagation |
| `area:utils` | Shared utilities |
| `area:voice` | Voice/audio processing |
| `area:ci` | CI/CD pipelines and automation |
| `area:infra` | Infrastructure, Docker, Helm |

#### `priority:*` — Urgency

| Label | Alias | Meaning |
|-------|-------|---------|
| `priority:critical` | P0 | Requires immediate attention — blocks release |
| `priority:high` | P1 | Important, address in current sprint |
| `priority:medium` | P2 | Normal priority, schedule accordingly |
| `priority:low` | P3 | Can be addressed in a future iteration |

#### `type:*` — Category of Work

| Label | Meaning |
|-------|---------|
| `type:feature` | New functionality or capability |
| `type:bug` | Defect in existing functionality |
| `type:test` | Test additions or improvements |
| `type:documentation` | Documentation improvements or additions |
| `type:refactor` | Code restructuring without behavior change |
| `type:chore` | Maintenance, dependency bumps, tooling |
| `type:security` | Security fix or hardening |
| `type:performance` | Performance improvement |

#### `status:*` — Workflow State

| Label | Meaning |
|-------|---------|
| `status:open` | New issue, not yet triaged |
| `status:in_progress` | Actively being worked on |
| `status:blocked` | Blocked by an external dependency |
| `status:ready` | Implementation ready, awaiting review |
| `status:review` | Under active code review |

### Copilot Automation Labels

| Label | Managed By | Meaning |
|-------|-----------|---------|
| `queue/copilot` | Dispatcher | Issue eligible for Copilot delegation |
| `copilot/delegated` | Dispatcher | Delegation comment posted; prevents re-delegation |

---

## Milestones

### Structure

Milestones track delivery against the project roadmap. Every issue **MUST** be assigned to a milestone.

#### Quarterly Milestones (primary)

| Milestone | Period | Focus |
|-----------|--------|-------|
| `Q1 2026` | Jan–Mar 2026 | Foundation hardening, core stability |
| `Q2 2026` | Apr–Jun 2026 | Acceleration, API, content, geo |
| `Q3 2026` | Jul–Sep 2026 | AI/LLM expansion, CDC, chimera adapters |
| `Q4 2026` | Oct–Dec 2026 | Distributed maturity, observability |
| `Q1 2027` | Jan–Mar 2027 | Production excellence, operations |

#### Version Milestones (secondary, for patch/minor releases)

Format: `v<major>.<minor>.<patch>` — e.g., `v1.5.0`, `v1.6.0`.

Use version milestones when a fix or feature must ship in a specific release, independent of the quarterly cycle.

### Standards

- Every milestone **MUST** have a due date.
- Milestone names use the exact formats above — no variations.
- When an issue slips, move it to the next milestone; do not leave it unassigned.
- Milestones are managed canonically via `.github/milestones.yml` and synchronized by `.github/workflows/maintenance-milestones.yml`.
- Hotfix and strategic lanes are supported via dedicated milestones (`HOTPATCH`, `LONG-TERM`) and label-driven auto-assignment rules.

---

## Relationships

### Syntax

Use the following GitHub keyword links in issue and PR descriptions:

| Keyword | Effect |
|---------|--------|
| `Fixes #<number>` | Closes the linked issue when PR merges |
| `Closes #<number>` | Closes the linked issue when PR merges |
| `Relates to #<number>` | Informational link, no auto-close |
| `Depends on #<number>` | Signals a blocking dependency |
| `Blocks #<number>` | Signals this issue blocks another |
| `Part of #<number>` | Marks this as a sub-task of an epic |

### Parent–Child (Epics)

- Large features are tracked as **epics** (parent issues).
- Sub-tasks use `Part of #<epic>` in their description.
- Epics list all sub-tasks with checkboxes in their description.

### Roadmap Traceability

- Every implementation issue **SHOULD** reference the roadmap item it implements using `Relates to` or by quoting the roadmap task in the description.
- Roadmap items use `[I]` status markers when a GitHub issue exists.

---

## Issue Metadata Standards

### Mandatory Fields

Every new issue **MUST** include:

| Field | Requirement |
|-------|-------------|
| **Title** | ≤ 60 characters, imperative mood, no trailing period |
| **Labels** | One each of `area:*`, `priority:*`, `type:*`, `status:*` |
| **Milestone** | Quarterly (`Q# YYYY`) or version (`v#.#.#`) |
| **Assignee** | Individual responsible for resolution (if known) |
| **Description** | See template below |

### Description Template

```markdown
### Context
<Why does this issue exist? What problem does it solve?>

### Goal
<Specific, measurable objective for this issue.>

### Acceptance Criteria
- [ ] <Verifiable criterion 1>
- [ ] <Verifiable criterion 2>

### Relationships
- Relates to #<roadmap epic>
- Depends on #<blocking issue> (if any)

### References
- <Link to roadmap section, design doc, or prior discussion>
```

### Pull Request Standards

- **Title**: matches the issue title or summarizes the change (≤ 72 characters).
- **Description**: references the issue with `Fixes #<number>` or `Relates to #<number>`.
- **Labels**: inherit from the linked issue; add `status:review` when opened.
- **Milestone**: same as the linked issue.
- **Reviewers**: at least one maintainer required before merge.

---

## Conclusion

Consistent application of these governance standards ensures traceability from roadmap to implementation, enables automated tooling (labeler, Copilot dispatcher, CI scoping), and maintains a clear audit trail for all changes to ThemisDB.