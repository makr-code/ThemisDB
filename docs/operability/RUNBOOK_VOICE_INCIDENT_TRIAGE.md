# RUNBOOK: Voice Incident Triage & Session Lifecycle Management

**Audience:** Database Operators, SREs, Voice/Audio Team Lead  
**Purpose:** Diagnose and resolve voice session failures, stream validation issues, and lifecycle anomalies  
**Severity:** High (affects all active voice-based workloads)  
**Estimated Duration:** 15 min - 1 hour (diagnosis + recovery)  

---

## Overview

This runbook guides operators through diagnosing voice session failures, understanding session lifecycle states, validating streams, detecting liveness/anti-spoof violations, and managing multi-session safety. It covers common failure modes (malformed streams, oversized payloads, timeouts) and recovery procedures.

**Key Principles:**
- Sessions have strict state machine: setup → active → teardown
- All streams must be validated before processing (format, size, sequence)
- Liveness/anti-spoof verification must complete before session transitions to active
- Multi-session cleanup must be atomic to prevent resource leaks

---

## Prerequisites Checklist

- [ ] Operator familiar with voice session state machine (diagram below)
- [ ] Access to session logs and session lifecycle metrics
- [ ] Stream validation tools available (format checker, size validator)
- [ ] Liveness/anti-spoof testing tools available
- [ ] Session recovery procedures tested in staging environment
- [ ] Multi-session cleanup procedures documented and tested

---

## Voice Session State Machine

```
┌─────────────────────────────────────────────────┐
│ Client Initiates Connection                      │
└──────────────────┬──────────────────────────────┘
                   │
                   ▼
        ┌──────────────────────┐
        │   SETUP Phase        │
        │  (validate stream)   │
        └──────────┬───────────┘
                   │
        Success?   │
        ─────────┬─┘
                 │
            ┌────▼─────────────────────────────┐
            │ Fail? Return to client (error)   │
            └────┬──────────────────────────────┘
                 │
        ┌────────▼──────────┐
        │ VALIDATION Phase  │
        │ (liveness check)  │
        └────────┬──────────┘
                 │
        Success? │
        ─────────┤
                 │
            ┌────▼─────────────────────────────┐
            │ Fail? Return to client (denied)  │
            └────┬──────────────────────────────┘
                 │
        ┌────────▼──────────────────────────┐
        │   ACTIVE Phase                    │
        │  (process audio stream)           │
        │  (monitors for timeout/stall)     │
        └────────┬───────────────────────────┘
                 │
        Client sends │
        END_STREAM?  │
        ─────────────┤
        Timeout?     │
        ─────────────┤
        Error?       │
                 │
        ┌────────▼──────────────────────────┐
        │   TEARDOWN Phase                  │
        │  (cleanup resources)              │
        │  (finalize metrics/logs)          │
        └────────┬───────────────────────────┘
                 │
        ┌────────▼──────────────────────────┐
        │   CLOSED                          │
        │  (session complete)               │
        └──────────────────────────────────┘
```

---

## Step-by-Step Incident Triage

### Step 1: Session State Diagnosis (5-10 min)

**Objective:** Identify which phase of the session lifecycle is experiencing failure.

1. **Query session information:**
   ```bash
   query-session \
     --session-id <session-id> \
     --include-timeline true \
     --include-logs true

   # Output:
   # Session ID: sess_abc123
   # State: ACTIVE
   # Lifetime: 2 min 45 sec
   # Client IP: 203.0.113.42
   # Stream Format: Opus (48kHz, 20ms frames)
   # Stream Size: 15.2 MB received, 5.8 MB processed
   # Last Activity: 3 seconds ago
   # Error Count: 0
   ```

2. **Check recent session errors (if applicable):**
   ```bash
   query-session-errors --window 5m --limit 10
   
   # Example output:
   # Session: sess_xyz789, Error: MALFORMED_STREAM, Count: 3
   # Session: sess_abc123, Error: STREAM_TIMEOUT, Count: 1
   ```

3. **Decision Tree:**

   ```
   Is session in CLOSED state?
   │
   ├─ YES
   │  └─ When did it close?
   │     ├─ < 10 min ago: Analyze closed session logs (Step 2)
   │     └─ > 10 min ago: Session lifecycle complete (normal)
   │
   ├─ NO, session in SETUP phase
   │  └─ Fail during setup? → Step 2A (Stream Validation)
   │
   ├─ NO, session in VALIDATION phase
   │  └─ Fail during validation? → Step 2B (Liveness/Anti-Spoof)
   │
   ├─ NO, session in ACTIVE phase
   │  └─ Problem during active? → Step 2C (Active Session Issues)
   │
   └─ NO, session in TEARDOWN phase
      └─ Problem during cleanup? → Step 2D (Teardown Issues)
   ```

### Step 2A: Stream Validation Diagnosis (5-10 min)

**Objective:** Diagnose setup-phase failures related to stream format or size.

1. **Check stream properties:**
   ```bash
   query-stream --session-id <session-id> \
     --include-validation-report true

   # Output:
   # Stream Properties:
   #   Format: Opus (expected: Opus/WebM/WAV)
   #   Bitrate: 128 kbps (expected: 64-256 kbps)
   #   Sample Rate: 48 kHz (expected: 16/48 kHz)
   #   Frame Duration: 20 ms (expected: 20 ms)
   #   Total Size: 15.2 MB
   #   Max Frame Size: 4096 bytes (expected: < 4096)
   # Validation Result: PASS
   ```

2. **If validation failed, investigate:**
   ```bash
   # Check for malformed stream frames
   validate-stream-frames --session-id <session-id>
   
   # Output example:
   # Frame 1-100: VALID
   # Frame 101-150: INVALID (header mismatch)
   # Frame 151-200: VALID
   # → Malformed frames at 101-150
   ```

3. **Check for oversized payload:**
   ```bash
   query-stream-size --session-id <session-id>
   
   # Output:
   # Total Stream Size: 156 MB (limit: 50 MB)
   # → EXCEEDS LIMIT!
   ```

4. **Resolution Actions:**

   | Issue | Action |
   |-------|--------|
   | Malformed frames 101-150 | Reject client version X.Y, coordinate upgrade |
   | Stream size 156 MB > 50 MB limit | Reject client, suggest streaming mode instead |
   | Unsupported codec (e.g., G.711) | Reject, provide list of supported codecs |
   | Sample rate mismatch (44.1 kHz not 48 kHz) | Reject, coordinate client resampling |

5. **Client Communication:**
   - Send rejection reason: `STREAM_VALIDATION_FAILED`
   - Include remediation hint (codec, bitrate, size limits)
   - Provide link to voice documentation

### Step 2B: Liveness & Anti-Spoof Diagnosis (5-10 min)

**Objective:** Diagnose validation-phase failures related to liveness/anti-spoof checks.

1. **Check liveness verification result:**
   ```bash
   query-session --session-id <session-id> \
     --include-liveness-report true

   # Output:
   # Liveness Verification:
   #   Challenge: 12-frame challenge sequence issued
   #   Response Received: Yes
   #   Challenge Match: FAIL (confidence: 87%)
   #   Spoof Probability: HIGH (87% likely replay/spoofed)
   #   Action: DENY_SESSION
   ```

2. **If liveness failed, investigate cause:**
   ```bash
   # Check if client is replaying previous session
   find-replay-candidates --session-id <session-id>
   
   # Check if client sent inverted/flipped audio (spoof technique)
   detect-audio-manipulation --session-id <session-id>
   
   # Check network issues (latency, packet loss)
   analyze-network-quality --session-id <session-id>
   ```

3. **Resolution Actions:**

   | Cause | Action |
   |-------|--------|
   | Replay detected (same session audio as previous) | Block client IP; coordinate with security team |
   | Audio manipulation (inversion, echo) | Increase liveness challenge difficulty |
   | High network latency (> 500ms) | Allow client to retry; increase timeout |
   | Normal variation in client liveness| Issue debug credential for client troubleshooting |

### Step 2C: Active Session Issues (10-20 min)

**Objective:** Diagnose failures during active audio streaming.

1. **Check active session metrics:**
   ```bash
   query-session-metrics --session-id <session-id> \
     --metric stream_processing_latency_p99,audio_buffer_underrun_count,timeout_events

   # Output:
   # Stream Processing Latency p99: 185 ms (threshold: 200 ms)
   # Audio Buffer Underruns: 2 (normal: 0)
   # Timeout Events: 1 (normal: 0)
   # Last Frame Processed: 30 seconds ago
   # Stream Status: ACTIVE
   ```

2. **If timeout detected, investigate:**
   ```bash
   # Check network connectivity
   test-network-connectivity --session-id <session-id>
   
   # Check if client sent KEEP_ALIVE
   query-session-heartbeats --session-id <session-id> --window 5m
   
   # Check server processing (CPU, memory)
   query-resource-metrics --session-id <session-id>
   ```

3. **If buffer underrun detected, investigate:**
   ```bash
   # Check audio frame arrival rate
   query-frame-arrival-rate --session-id <session-id>
   
   # Check if frames are out-of-order
   detect-out-of-order-frames --session-id <session-id>
   
   # Check if frames are being dropped
   query-dropped-frames --session-id <session-id>
   ```

4. **Resolution Actions:**

   | Issue | Action |
   |-------|--------|
   | Timeout (client stopped sending KEEP_ALIVE) | End session gracefully; investigate client |
   | Network connectivity loss | Trigger automatic session cleanup |
   | Buffer underruns (2-5 events) | Increase buffer size; non-critical |
   | Buffer underruns (>10 events) | End session; investigate network quality |
   | CPU/memory contention on server | Migrate other workloads; scale server if persistent |

### Step 2D: Teardown Phase Issues (5-10 min)

**Objective:** Diagnose failures during session cleanup.

1. **Check teardown logs:**
   ```bash
   query-session-teardown --session-id <session-id>

   # Output:
   # Teardown Initiated: Yes
   # Teardown Start Time: 2026-08-15 14:30:00 UTC
   # Teardown Completion: PENDING (in progress)
   # Resources Released:
   #   - Audio processing: DONE
   #   - Metrics finalization: DONE
   #   - Liveness verification cleanup: PENDING (hanging for 5 min!)
   #   - Notification to client: PENDING
   ```

2. **If teardown stalled, investigate:**
   ```bash
   # Check if cleanup process is hung
   query-cleanup-process --session-id <session-id>
   
   # Check system logs for errors
   collect-logs --component session_cleanup --session-id <session-id>
   
   # Check if resources are locked
   query-resource-locks --session-id <session-id>
   ```

3. **Force cleanup if necessary:**
   ```bash
   # Attempt graceful cleanup with timeout
   force-session-cleanup --session-id <session-id> --wait-timeout 30s
   
   # If still not cleaned up:
   force-session-termination --session-id <session-id> --immediate true
   ```

---

## Multi-Session Safety & Concurrent Handling

### Issue: Multiple Sessions from Same Client (or Same Cluster)

**Objective:** Ensure cleanup is atomic and resources are released properly.

1. **List all sessions for client:**
   ```bash
   list-sessions --client-ip <ip-address>
   
   # Output:
   # Session 1 (sess_abc123): ACTIVE, 5 min
   # Session 2 (sess_xyz789): ACTIVE, 2 min
   # Session 3 (sess_def456): ACTIVE, 30 sec
   # Session 4 (sess_ghi123): CLOSED, 1 min ago
   ```

2. **If multiple ACTIVE sessions (potential resource leak):**
   ```bash
   # Check if this is normal (parallel transcription streams)
   query-concurrent-limit --client-ip <ip-address>
   
   # If limit exceeded:
   terminate-oldest-session --client-ip <ip-address> \
     --keep-count 2 \  # Keep only 2 active
     --gracefully true
   ```

3. **Verify cleanup across all closed sessions:**
   ```bash
   verify-cleanup-completeness --client-ip <ip-address>
   
   # Output:
   # Session 4 (closed 1 min ago):
   #   - Audio buffer: RELEASED
   #   - Liveness state: RELEASED
   #   - Metrics: FINALIZED
   #   - Client notification: SENT
   # Status: CLEAN (no orphaned resources)
   ```

---

## Troubleshooting Table

| Symptom | Likely Cause | Investigation | Resolution |
|---------|--------------|---|----------|
| 50% of session setup failures | Unsupported codec from new client version | Check client logs for codec format | Coordinate client update or support older codec |
| All sessions timeout after 10 min | Server keep-alive timeout too short | Check server timeout config (should be 15+ min) | Increase timeout; restart server |
| Liveness check fails 100% of time | Anti-spoof logic too strict or broken | Compare liveness scores to baseline | Relax threshold or debug anti-spoof algorithm |
| Multiple sessions from same client | Client sending multiple concurrent streams intentionally | Check if concurrent streams are expected | If not, terminate oldest; contact client |
| Session cleanup hangs on liveness cleanup | Race condition in liveness verification cleanup | Check cleanup logs for deadlock | Force termination; escalate to voice team |

---

## Incident Report Template

```markdown
# Voice Session Incident Report

## Incident Details
- **Session ID:** [sess_xxx]
- **Client IP:** [203.0.113.x]
- **Phase of Failure:** [SETUP/VALIDATION/ACTIVE/TEARDOWN]
- **Start Time:** YYYY-MM-DD HH:MM:SS UTC
- **Failure Time:** YYYY-MM-DD HH:MM:SS UTC
- **Duration Before Failure:** [seconds]

## Failure Description
[What happened; which state machine transition failed]

## Root Cause
[From Step 2A/2B/2C/2D diagnosis]

## Impact
- **Sessions Affected:** [number]
- **Data Loss:** [yes/no]
- **Client Communication:** [error code sent]

## Remediation
[Action taken to resolve]

## Prevention
[What will prevent similar incidents]

## Sign-Off
- **Operator:** [name]
- **Voice Team Lead:** [name]
- **Date:** YYYY-MM-DD
```

---

## Quick Reference

```bash
# Check session state
query-session --session-id <session-id> --include-timeline true

# Validate stream
validate-stream-frames --session-id <session-id>

# Check liveness result
query-session --session-id <session-id> --include-liveness-report true

# List active sessions for client
list-sessions --client-ip <ip-address>

# Force cleanup
force-session-cleanup --session-id <session-id> --wait-timeout 30s
```

---

**Runbook Version:** 1.0  
**Last Updated:** 2026-08-15  
**Owner:** Voice/Audio Team  
**Next Review:** 2026-12-15
