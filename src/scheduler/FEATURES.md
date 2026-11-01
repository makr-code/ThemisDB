---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: include/scheduler/*.h; src/scheduler/*.cpp; src/scheduler/README.md
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler feature map

The following implementation surfaces are present in the scheduler module:

- task registration, validation, execution, scheduling, statistics, and optional
  persistence in `TaskScheduler`;
- separate distributed-leadership and external-scheduler adapter classes;
- configurable hybrid time-series retention stages;
- changefeed event filtering, condition matching, debounce, and callbacks;
- audit/security event recording, result storage, and anomaly metrics.

These are component capabilities, not a guarantee that every path is enabled,
integrated, or production-ready in a given build. For source-specific
limitations see [README.md](README.md) and [ARCHITECTURE.md](ARCHITECTURE.md).
