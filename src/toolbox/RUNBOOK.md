---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Toolbox incident runbook

**Purpose and audience:** Triage checklist for service operators and on-call engineers investigating extraction, registry, content-bridge, or sink-write symptoms. The toolbox library does not provide an operator CLI or a metrics HTTP endpoint; the host application must expose logs and metric text.

## First response

1. Identify affected request IDs/content IDs, time window, deployment version, workflow profile, and whether the issue is extraction, content storage, or a sink write.
2. Check host-application logs for toolbox messages such as `ContentManager failed`, `graph_writer failed`, `vector_writer failed`, `registry not initialised`, or `ToolboxBuilder: failed to load profile`.
3. Collect the host's exported `toolbox_*` metrics. `IngestionToolbox::getMetricsText()` and `ContentToolboxBridge::getMetricsText()` return Prometheus text for the owning objects; the application is responsible for making it scrapeable.
4. Determine whether the result was empty, failed, or partially persisted. Empty extraction can be a valid no-match. Bridge sink failures are separate from the bridge-level `ok` result.

## Symptom: empty extraction or rising extraction failures

- Compare `toolbox_extract_calls_total`, `toolbox_extract_empty_results_total`, and `toolbox_extraction_failures_total` across the incident window.
- Check the workflow profile path and startup warning logs. The builder continues when a profile fails to load, so a constructed toolbox does not prove the intended profile is active.
- Verify the expected ingestion steps and text backend were registered/injected. A default toolbox uses a null text-generation backend.
- Reproduce with the same text, MIME hint, filename hint, and workflow configuration in the focused toolbox ingestion tests.

Do not treat every empty result as a fault. Empty input returns immediately, and a successfully run workflow may legitimately extract no entities. The failure counter also uses a minimum input-size rule and does not encode the underlying workflow error.

## Symptom: registry access throws

- `ToolboxRegistry::instance()` throws `std::logic_error` until initialization has happened. Verify server bootstrap ordering and that all consumer paths use the intended registry instance.
- A second `initialize()` replaces the previous instance and logs a warning. Check for duplicate initialization or an unexpected reconfiguration.
- Restore the intended instance through the host's normal bootstrap/reconfiguration path. Do not call `ToolboxRegistry::reset()` during active production requests; reset is intended for test isolation or controlled teardown.

## Symptom: bridge reports failure or missing content

- For `ingest()`, inspect the returned `BridgeResult::error` and the `ContentManager` failure message first. Confirm content manager initialization and raw input is non-empty.
- For a result with a content ID but no entities/vectors, verify whether assembled text was available; the bridge intentionally skips enrichment when no text is returned.
- For `enrichExisting()`, confirm the ID exists and assembled text is available. An empty ID and unknown ID fail explicitly.
- Review `toolbox_bridge_failures_total` and compare `failuresTotal()` with graph/vector writer counters. The bridge histogram counts completed paths; early-return failures may not appear in the latency exposition.

## Symptom: graph or vector results are missing

- Check `graphWriteFailuresTotal()` and `vectorWriteFailuresTotal()` separately. Sink write failures are logged and soft-failed; `BridgeResult::ok` can remain true.
- Confirm the relevant writer was provided to the builder/bridge and that the returned `BridgeResult::sinks_written` matches observed writes.
- Verify the downstream sink's own health, schema, and durability state with that subsystem's runbook.
- Do not blindly retry `ingest()`: content may already have been stored before enrichment or sink failure. `enrichExisting()` skips raw-content scanning and deduplication and may repeat external writes, so use it only under the caller's idempotency policy.

## Recovery and escalation

- Prefer correcting the host bootstrap, workflow profile, backend, or sink configuration, then replacing/reinitializing the toolbox through the host's supported lifecycle.
- Preserve request IDs, content IDs, relevant metric samples, exact errors, active profile, and whether any sink accepted data.
- Escalate workflow execution/profile failures to ingestion owners, raw-content/storage failures to content owners, and graph/vector persistence failures to the respective sink owners.
- Follow the deployment-level rollback procedure if configuration correction does not restore service. The toolbox has no transactional rollback for content or sink writes.

## Validation references

- [`OPERATIONS.md`](./OPERATIONS.md) for metric interpretation.
- [`SECURITY.md`](./SECURITY.md) and [`TRUST.md`](./TRUST.md) for handling untrusted inputs and re-enrichment.
- [`tests/toolbox/README.md`](../../tests/toolbox/README.md) for focused regression commands.

> Exact model backend not verifiable from final document text alone.
