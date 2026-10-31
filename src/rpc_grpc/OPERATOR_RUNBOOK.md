---
Author: "ThemisDB Team"
Created: "2026-08-06"
Last Updated: "2026-10-31"
Status: "deprecated"
Source_Level: "L1"
SOT_Domain: "module-behavior"
Issue: "makr-code/ThemisDB#7145"
Milestone: "DOC-WEEKLY-2026-W44"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# RPC gRPC Operator Runbook (Superseded)

This historical runbook is retained only to preserve the previous document
location. Its former operational examples assumed a standalone `themis-grpc`
service, JSON configuration parsing, HTTP admin endpoints, and live TLS
listener replacement that are not implemented by the current
`src/rpc_grpc` source.

Use the source-verified [RPC gRPC Runbook](RUNBOOK.md) for diagnosis and
recovery, [Operations](OPERATIONS.md) for deployment readiness and monitoring
limits, and [Trust](TRUST.md) for listener exposure and credential controls.
The host process and deployment controller own service management.
