---
Author: "ThemisDB Team"
Created: "2026-09-16"
Last Updated: "2026-10-30"
Status: "deprecated"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Chaos Fault Injection Runbook

This legacy root-level runbook is superseded. Its former CLI commands, metric names, log tags, and trace spans were not implemented by the current chaos runtime; do not execute or configure alerts from that content.

Use the source-verified module guides instead:

- [Chaos Runbook](../../src/chaos/RUNBOOK.md)
- [Chaos Operations Guide](../../src/chaos/OPERATIONS.md)

The current module is an in-process simulator for consumers that explicitly inspect fault state. It does not manipulate real network or disk I/O, persist state across process restarts, expose a production admin route, or provide built-in metrics, logs, traces, or alerts.
