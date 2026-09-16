# Runbook: Content Pipeline — Wave D Operability

<!-- Runbook: content | Wave D | validated: 2026-09-16 -->
<!-- Links: src/content/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `content`
module. Use it to diagnose and remediate processor failures, queue overflows,
unsupported format incidents, and large-payload rejection scenarios.

---

## Scenario 1 — Processor Failed

**Log pattern:** `[CONTENT:ProcessorFailed]`

### Symptoms
- Content processor fails to process an item.
- `[CONTENT:ProcessorFailed]` emitted with `content_id`, `format`, `processor_name`, and `error_code` fields.
- Items may be quarantined; pipeline throughput drops.

### Diagnosis
1. Confirm processor failures:
   ```
   grep '\[CONTENT:ProcessorFailed\]' /var/log/themisdb/content.log
   ```
2. Identify `processor_name` (pdf, docx, audio, image, video) and `error_code`.
3. Check processor availability:
   ```
   themisdb-admin content list-processors
   ```
4. Review `content_processor_failure_total` metric.

### Remediation
1. For dependency failures (e.g., missing LibreOffice for docx): install missing dependency
   and restart processor.
2. For extraction errors: inspect quarantined item for corruption:
   `themisdb-admin content inspect-quarantine --content-id <id>`.
3. Restart processor: `themisdb-admin content restart-processor --name <processor>`.
4. Confirm processor failure rate returns to zero.

### Escalation
Escalate to the content engineering team if a processor fails to restart or if failure
rate exceeds 1%.

---

## Scenario 2 — Queue Overflow

**Log pattern:** `[CONTENT:QueueOverflow]`

### Symptoms
- Async content processing queue exceeds capacity.
- `[CONTENT:QueueOverflow]` emitted with `queue_name`, `queue_depth`, `drop_count`, and `producer_rate` fields.
- Items may be dropped; consumers may fall behind.

### Diagnosis
1. Confirm queue overflow:
   ```
   grep '\[CONTENT:QueueOverflow\]' /var/log/themisdb/content.log
   ```
2. Identify `queue_name` and `producer_rate` vs. consumer throughput.
3. Check async worker count:
   ```
   themisdb-admin content show-workers
   ```
4. Review `content_queue_depth` and `content_drop_rate` metrics.

### Remediation
1. Scale up async workers: `themisdb-admin content scale-workers --count +2`.
2. Increase queue capacity: `content.queue.max_depth`.
3. Apply back-pressure to upstream producers if immediate scaling is not possible.
4. Confirm queue depth returns below high-water mark.

### Escalation
Escalate to the platform team if drop rate exceeds 0.1% for more than 2 minutes.

---

## Scenario 3 — Format Unsupported

**Log pattern:** `[CONTENT:FormatUnsupported]`

### Symptoms
- Content item has an unsupported or unrecognized format.
- `[CONTENT:FormatUnsupported]` emitted with `content_id`, `detected_format`, `mime_type` fields.
- Item is quarantined; no processor can handle it.

### Diagnosis
1. Confirm unsupported format incidents:
   ```
   grep '\[CONTENT:FormatUnsupported\]' /var/log/themisdb/content.log
   ```
2. Identify `detected_format` and `mime_type`.
3. Check if format processor is installed:
   ```
   themisdb-admin content list-processors | grep <format>
   ```
4. Review `content_unsupported_format_total` metric.

### Remediation
1. If a processor exists for the format: verify it is enabled in config.
2. If no processor exists: route to manual review queue.
3. For newly common formats: plan processor development in backlog.
4. Confirm quarantine queue is monitored and drained periodically.

### Escalation
Escalate to the content engineering team if a high-volume format is unsupported and
causing significant quarantine accumulation.

---

## Scenario 4 — Payload Too Large

**Log pattern:** `[CONTENT:PayloadTooLarge]`

### Symptoms
- Content item payload exceeds the configured size limit.
- `[CONTENT:PayloadTooLarge]` emitted with `content_id`, `payload_size_bytes`, `limit_bytes` fields.
- Item is rejected; producer may retry with no success.

### Diagnosis
1. Confirm payload rejections:
   ```
   grep '\[CONTENT:PayloadTooLarge\]' /var/log/themisdb/content.log
   ```
2. Identify `payload_size_bytes` vs. `limit_bytes`.
3. Check if rejection is justified by resource constraints or a misconfigured limit.
4. Review `content_payload_too_large_total` metric.

### Remediation
1. If limit is too restrictive: increase `content.processor.max_payload_bytes` with
   infrastructure team approval (memory and disk impact assessment required).
2. If item is legitimately oversized: route to streaming processor if available.
3. Notify upstream producer of the size constraint.
4. Confirm rejection rate returns to expected baseline.

### Escalation
Escalate to the platform team if payload limits need to be increased and the infrastructure
impact is not assessed.

---

## Scenario 5 — Async Queue Pressure During Large Mixed-Media Burst

**Log pattern:** `[CONTENT:QueueOverflow]` with `media_type_mix=true`

### Symptoms
- Mixed-media burst (documents, audio, video) simultaneously fills the async queue.
- Media types with long processing times (video, audio) block fast-processing types (text, html).
- `content_queue_depth_by_format` metric shows format-level imbalance.

### Diagnosis
1. Check format-level queue depth:
   ```
   grep 'media_type_mix=true' /var/log/themisdb/content.log
   ```
2. Identify which format is dominating queue depth.
3. Review per-format processor concurrency settings.

### Remediation
1. Enable format-partitioned queues: `content.queue.partition_by_format=true`.
2. Prioritize lightweight formats (text, html) with higher concurrency.
3. Rate-limit heavyweight formats (video, audio) at ingestion.
4. Confirm queue depth per format returns to balanced state.

### Escalation
Escalate to the content engineering team if format-partitioned queues are not configured
and queue imbalance persists.
