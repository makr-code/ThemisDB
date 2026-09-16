# Runbook: Voice Pipeline — Wave D Operability

<!-- Runbook: voice | Wave D | validated: 2026-09-16 -->
<!-- Links: src/voice/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `voice`
module. Use it to diagnose and remediate STT model unavailability, TTS backend
crashes, audio buffer overflow, streaming disconnect, and VAD false positive
storms.

---

## Scenario 1 — STT Model Unavailable

**Log pattern:** `[VOICE:STTUnavailable]`

### Symptoms
- Transcription requests return empty transcripts or `STT_BACKEND_FAILURE` markers.
- `[VOICE:STTUnavailable]` emitted with `session_id` and `backend_error` fields.
- Affected voice sessions may fall back to silent/error responses.

### Diagnosis
1. Confirm STT unavailability:
   ```
   grep '\[VOICE:STTUnavailable\]' /var/log/themisdb/voice.log
   ```
2. Check `backend_error` field — distinguish network failures from model load failures.
3. Verify STT model process or service is running:
   ```
   systemctl status themisdb-stt  # or the relevant service name
   ```
4. Check model file availability and disk space on the model volume.

### Remediation
1. Restart the STT backend service if it has crashed.
2. If the model failed to load, verify model file integrity and re-load.
3. For transient network errors to a remote STT backend, wait for reconnect
   or failover to the secondary STT endpoint.
4. Confirm `[VOICE:STTReady]` appears in logs before accepting new sessions.

### Escalation
Escalate to the ML-Ops team if the STT model repeatedly fails to load or if
transcription quality degrades after restart.

---

## Scenario 2 — TTS Backend Crash

**Log pattern:** `[VOICE:TTSCrash]`

### Symptoms
- TTS synthesis calls return empty audio or silent byte arrays.
- `[VOICE:TTSCrash]` emitted with `session_id` and `crash_reason` fields.
- Voice responses may be silently dropped or replaced with error tones.

### Diagnosis
1. Confirm TTS crash:
   ```
   grep '\[VOICE:TTSCrash\]' /var/log/themisdb/voice.log
   ```
2. Check `crash_reason` and timestamp.
3. Review TTS process exit code and core dump if available:
   ```
   journalctl -u themisdb-tts --since "1 hour ago" | tail -100
   ```
4. Check host memory and CPU under TTS workload.

### Remediation
1. Restart the TTS backend service.
2. If crash is due to OOM, reduce TTS concurrency via configuration.
3. Ensure TTS model files are intact and not corrupted.
4. After restart, confirm `[VOICE:TTSReady]` appears before accepting new requests.

### Escalation
Escalate to the ML-Ops and infrastructure teams if crashes are recurring or
associated with memory/hardware faults.

---

## Scenario 3 — Audio Buffer Overflow

**Log pattern:** `[VOICE:BufferOverflow]`

### Symptoms
- Audio chunks are being dropped due to buffer exhaustion.
- `[VOICE:BufferOverflow]` emitted with `session_id`, `dropped_chunks`, and `buffer_capacity`.
- Callers may experience garbled or incomplete audio transcription.

### Diagnosis
1. Confirm buffer overflow:
   ```
   grep '\[VOICE:BufferOverflow\]' /var/log/themisdb/voice.log
   ```
2. Review `dropped_chunks` count and rate trend.
3. Check upstream audio chunk producer rate vs. STT processing rate.
4. Inspect active session count and concurrency level.

### Remediation
1. Increase audio buffer capacity in configuration (`voice.audio_buffer_capacity`).
2. Throttle upstream audio producers if they are sending faster than STT can process.
3. Scale out STT processing workers to increase drain rate.
4. Apply session-level backpressure signals to producers (if the protocol supports it).

### Escalation
Escalate to the capacity planning team if buffer overflow persists after
scaling adjustments.

---

## Scenario 4 — Streaming Disconnect

**Log pattern:** `[VOICE:StreamDisconnect]`

### Symptoms
- Streaming voice sessions terminate unexpectedly mid-stream.
- `[VOICE:StreamDisconnect]` emitted with `session_id`, `disconnect_reason`, and `bytes_received`.
- Callers may receive incomplete transcription or TTS output.

### Diagnosis
1. Confirm streaming disconnect:
   ```
   grep '\[VOICE:StreamDisconnect\]' /var/log/themisdb/voice.log
   ```
2. Check `disconnect_reason`: distinguish client-side disconnects from server-side failures.
3. Review network stability metrics (packet loss, TCP retransmits).
4. Check server-side idle timeout configuration (`voice.stream_idle_timeout_ms`).

### Remediation
1. If client-side: confirm clients implement reconnection with exponential backoff.
2. If server-side timeout: adjust `voice.stream_idle_timeout_ms` to tolerate legitimate
   pauses in audio input.
3. For network instability: work with infrastructure team to stabilise the path.
4. Add streaming reconnection evidence to session audit logs.

### Escalation
Escalate to the networking team if disconnect rate exceeds 1 % of sessions
over any 5-minute window.

---

## Scenario 5 — VAD False Positive Storm

**Log pattern:** `[VOICE:VADFalsePositiveStorm]`

### Symptoms
- VAD (Voice Activity Detection) is triggering excessively on noise or silence.
- `[VOICE:VADFalsePositiveStorm]` emitted with `session_id`, `false_positive_rate`, and `window_ms`.
- STT pipeline is flooded with empty/noise segments, increasing latency and cost.

### Diagnosis
1. Confirm VAD false positive storm:
   ```
   grep '\[VOICE:VADFalsePositiveStorm\]' /var/log/themisdb/voice.log
   ```
2. Review `false_positive_rate` and `window_ms` to understand the storm duration and intensity.
3. Check audio input signal quality (SNR, background noise level).
4. Review VAD sensitivity settings (`voice.vad_threshold`, `voice.vad_energy_floor`).

### Remediation
1. Increase VAD energy threshold to suppress noise-triggered activations
   (`voice.vad_energy_floor` and `voice.vad_threshold`).
2. Enable or tune a noise gate pre-processor upstream of VAD.
3. For sessions with known noisy environments, apply session-level VAD override.
4. Monitor STT queue depth and confirm it normalises after threshold adjustment.

### Escalation
Escalate to the ML-Ops team if VAD false positives persist after threshold
tuning or if they are correlated with a specific audio source.

---

## References

- `src/voice/ROADMAP.md` — Wave D operability contribution
- `tests/integration/test_voice_pipeline_soak.cpp` — Wave D soak tests
- `tests/voice/test_voice_highcardinality_stress.cpp` — stress tests
- `benchmarks/voice/bench_voice_dedicated_gates.cpp` — benchmark gates
- `docs/operability/RUNBOOK_CORE.md` — Cross-module operability baseline
