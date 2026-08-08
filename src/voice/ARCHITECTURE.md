# Voice Module - Architecture Guide

<!-- Status: current | validated: 2026-08-08 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PRODUCTION_REQUIREMENTS.md -->

**Version:** 1.0-production  
**Last Updated:** 2026-08-08  
**Module Path:** src/voice/  
**Status:** 🟢 Production Ready  

---

## 1. Overview

The Voice module implements voice input processing, session control, assistant orchestration, and streaming/telephony voice interfaces. The module follows a **frozen API contract** (Phase 1) with all state machines, error codes, and thread-safety guarantees immutable.

**Key Characteristics:**
- 🎯 Thread-safe all public APIs
- 📡 Real-time streaming (browser WebSocket, SIP/WebRTC)
- 🔐 Built-in security (biometric auth, PII redaction, audit logging)
- 🛡️ Anti-spoofing and liveness detection
- 📊 Session lifecycle management with timeouts
- ⚡ High-concurrency (100+ concurrent streams)

---

## 2. Module Architecture Surfaces

| Surface | Source Files | Responsibility |
|---|---|---|
| **Session Management** | voice_session_manager.h/cpp | Session lifecycle, timeouts, context persistence |
| **Assistant Orchestration** | voice_assistant.h/cpp, voice_assistant_llm.cpp | Command routing, LLM integration, response generation |
| **Audio Preprocessing** | audio_preprocessing.h/cpp | Normalization, validation, frame buffering |
| **Wake Word Detection** | wake_word_detector.h/cpp | Wake-word spotting, chunk-level detection |
| **Intent Detection** | voice_intent_detector.h/cpp | NLU, command classification, parameter extraction |
| **Authentication** | voice_auth.h/cpp | Biometric speaker verification, enrollment, liveness |
| **Streaming** | voice_browser_streaming.h/cpp | WebSocket streaming, chunk delivery, STT integration |
| **Telephony** | voice_telephony.h/cpp | SIP/WebRTC, call routing, recording, transcription |
| **Security & Privacy** | voice_security.h/cpp | PII redaction, consent tracking, audit logging, GDPR/CCPA |
| **Error Handling** | voice_error_handler.h/cpp | Unified error codes (42 defined), circuit breakers, resilience |
| **Storage** | voice_audio_storage.h/cpp | Recording persistence, transcript storage, encryption |
| **Batch Processing** | voice_batch_processor.h/cpp | High-volume async processing |
| **Emotion Analysis** | emotion_analyzer.h/cpp | Voice emotion detection and sentiment analysis |

---

## 3. Runtime Control Flow (ASCII Diagrams)

### 3.1 Voice Command Processing Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                    USER VOICE INPUT (Audio bytes)                    │
└────────────────────────┬────────────────────────────────────────────┘
                         │
                         ▼
        ┌─────────────────────────────────────────┐
        │  1. Session Lookup & Validation         │
        │  - Find session by ID                   │
        │  - Check not expired                    │
        │  - Check user authenticated             │
        └────────────┬────────────────────────────┘
                     │
          ┌──────────▼──────────┐
          │ Session found?      │ NO
          └──────────┬──────────┘
                     │ YES
                     ▼
        ┌─────────────────────────────────────────┐
        │  2. Audio Preprocessing                 │
        │  - Validate frame size (≤ 64KB)         │
        │  - Normalize sample rate (16kHz)        │
        │  - Apply noise filtering                │
        │  - Check for silence                    │
        └────────────┬────────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────────┐
        │  3. Wake-Word Detection                 │
        │  - Invoke wake_word_detector            │
        │  - Check if "wake phrase" detected      │
        │  - Skip if already active               │
        └────────────┬────────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────────┐
        │  4. Speech-to-Text (STT)                │
        │  - Invoke content/stt_processor         │
        │  - Stream audio to STT engine           │
        │  - Collect partial + final transcripts  │
        └────────────┬────────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────────┐
        │  5. Intent Detection                    │
        │  - Parse transcript to intent           │
        │  - Extract parameters                   │
        │  - Check command validity               │
        └────────────┬────────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────────┐
        │  6. LLM Processing (if applicable)      │
        │  - Invoke llm/llama_wrapper             │
        │  - Generate contextual response         │
        │  - Apply constraints/guardrails         │
        └────────────┬────────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────────┐
        │  7. Security Checks                     │
        │  - Redact PII from transcripts          │
        │  - Validate consent (recording, etc.)   │
        │  - Audit log the operation              │
        └────────────┬────────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────────┐
        │  8. Text-to-Speech (TTS)                │
        │  - Invoke content/tts_processor         │
        │  - Generate audio response              │
        │  - Apply voice customization if set     │
        └────────────┬────────────────────────────┘
                     │
                     ▼
        ┌─────────────────────────────────────────┐
        │  9. Session State Update                │
        │  - Update last_activity_ms              │
        │  - Add to conversation_history          │
        │  - Increment turn counter               │
        │  - Persist to backend                   │
        └────────────┬────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────────┐
│              RESPONSE (Text + Audio + Intent metadata)               │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.2 Session Creation & Lifecycle State Machine

```
┌────────────────────────────────────────────────────────────────────┐
│                   createSession(user_id, device_id)                │
└──────────────────────────┬─────────────────────────────────────────┘
                           │
                ┌──────────▼──────────┐
                │ Validate user_id    │ FAIL
                │ (non-empty, auth)   │────────────┐
                └──────────┬──────────┘            │
                           │ PASS                  │
                           ▼                       │
                   ╔════════════════╗              │
                   ║   [ACTIVE]     ║  ◄──────────┴─ Error: 6605 (User ID invalid)
                   ╚════════════════╝
                       │       ▲
          touchSession()│       │ touchSession() resets timer
                        │       │
                   (idle_timeout_ms)
                        │       │
                        ▼       │
                   ╔════════════════╗
                   ║     [IDLE]     ║  ◄───┐
                   ╚════════════════╝      │
                       │                   │
                (max_session_duration_ms)  │ touchSession()
                       │                   │
                       ▼                   │
                   ╔════════════════╗      │
                   ║   [EXPIRED]    ║      │
                   ╚════════════════╝      │
                       │                   │
                (cleanup cycle)            │
                       │                   │
                       ▼                   │
                   ╔════════════════╗      │
                   ║ [TERMINATED]   ║ ◄────┘
                   ╚════════════════╝

OR (explicit termination):

   [ACTIVE/IDLE/EXPIRED] ──terminate()──> [TERMINATED]
```

### 3.3 Streaming Input Processing (Browser WebSocket)

```
┌───────────────────────────────────────────────────────────┐
│          Browser → WebSocket → VoiceStreamingManager       │
└───────────────────┬─────────────────────────────────────────┘
                    │
        ┌───────────▼───────────┐
        │ createStream()        │
        │ - Allocate StreamID   │
        │ - Bind callbacks      │
        │ - Initialize buffers  │
        └───────────┬───────────┘
                    │
                    ▼
         ╔═════════════════════╗
         ║  [CONNECTED]        ║  Ready for audio frames
         ╚════════╤════════════╝
                  │
    ┌─────────────┴─────────────┬─────────────┐
    │                           │             │
    ▼                           ▼             ▼
sendAudioFrame()         (no frames)    receivePartialTranscript()
(audio bytes)            (timeout)        [callback fires]
    │                           │             │
    ▼                           ▼             ▼
 QUEUE FRAME             ╔═════════════╗  EMIT PARTIAL
 (validate size)         ║   [ERROR]   ║  (live feedback)
    │                    ╚═════════════╝  │
    ▼                           ▲          ▼
 BATCH FRAMES              closeStream()  [STREAMING]
 (coalesce 4+)                 │          state
    │                          │
    ▼                          │
 INVOKE STT                    │
    │                          │
    ├─> onPartialTranscript()  │
    │   (intermediate result)   │
    │                          │
    ▼                          │
 FINAL TRANSCRIPT READY        │
    │                          │
    ├─> onFinalTranscript()    │
    │   (complete text)         │
    │                          │
    └─────────────────────────┘
                │
                ▼
         ╔═════════════════════╗
         ║   [CLOSED]          ║  Stream done
         ╚═════════════════════╝
```

---

## 4. Component Architecture Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                         Voice Module Components                           │
└──────────────────────────────────────────────────────────────────────────┘

                         ┌─────────────────────┐
                         │  VoiceAssistant     │ (main orchestrator)
                         │  - routeCommand()   │
                         │  - getResponse()    │
                         └──────────┬──────────┘
                                    │
                  ┌─────────────────┼─────────────────┐
                  │                 │                 │
                  ▼                 ▼                 ▼
        ┌──────────────────┐ ┌──────────────┐ ┌──────────────────┐
        │ SessionManager   │ │ IntentDetect │ │ LLMIntegration   │
        │ - createSession()│ │ - parse()    │ │ - generateResp() │
        │ - updateSession()│ │ - classify() │ │                  │
        │ - getAnalytics() │ └──────────────┘ └──────────────────┘
        └────────┬─────────┘
                 │
            ┌────┴────┐
            ▼         ▼
   ┌─────────────┐  ┌────────────────────┐
   │ Persistence │  │ TimeoutManager     │
   │ - save()    │  │ - expireOldSes()   │
   │ - load()    │  │ - cleanup()        │
   └─────────────┘  └────────────────────┘

        ┌─────────────────┬─────────────────┐
        │                 │                 │
        ▼                 ▼                 ▼
   ┌──────────────┐ ┌───────────────┐ ┌──────────────────┐
   │ StreamMgr    │ │ TelephonyMgr  │ │ AuthManager      │
   │ (WebSocket)  │ │ (SIP/WebRTC)  │ │ (Biometric auth) │
   │- createStr() │ │ - initCall()  │ │ - enroll()       │
   │- sendFrame() │ │ - endCall()   │ │ - verify()       │
   │- closeStr()  │ │ - record()    │ │ - identify()     │
   └──────────────┘ └───────────────┘ └──────────────────┘

   ┌────────────────────────────────────────────────────────────┐
   │              Supporting Components                          │
   ├────────────────────────────────────────────────────────────┤
   │ - AudioPreprocessor: Normalize, validate audio frames      │
   │ - WakeWordDetector: Detect wake phrases                    │
   │ - VoiceSecurityMgr: Redact PII, log audit, consent track  │
   │ - ErrorHandler: Circuit breaker, retry policy             │
   │ - VoiceModelCache: Preload/cache ML models                │
   │ - BathProcessor: Async high-volume voice jobs             │
   │ - AudioStorage: Record + store audio securely             │
   │ - TTSCustomizer: Voice personalization                    │
   │ - EmotionAnalyzer: Detect emotion from voice             │
   └────────────────────────────────────────────────────────────┘
```

---

## 5. Concurrency Model & Thread Safety

### 5.1 Thread-Safety Guarantees

**All public Voice APIs are thread-safe.**

Synchronization Strategy:
- Each manager class owns a `std::mutex`
- Public methods acquire mutex for duration of operation
- Session data is copied (not referenced) when returned
- Backend persistence calls are serialized
- No deadlock risk (only single mutex per object)

```cpp
// Example: Thread-safe session access
class VoiceSessionManager {
    mutable std::mutex mutex_;  // Protects all mutable state
    
    VoiceSessionData createSession(...) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Safe: only one thread can modify state at a time
        ...
    }
    
    std::optional<VoiceSessionData> getSession(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        // Safe: return copy (by value), not reference
        return sessions_.at(id);  // Copy constructor invoked
    }
};
```

### 5.2 Concurrent Session Limits

| Parameter | Limit | Notes |
|---|---|---|
| Max concurrent sessions | Unbounded | Enforced externally (e.g., via reverse proxy) |
| Max concurrent streams | 100+ | Per VoiceStreamingManager instance |
| Max concurrent calls | 50+ | Per VoiceTelephonyManager instance |
| Max threads per component | N_CPU | Worker threads for async tasks |

### 5.3 Resource Cleanup & Deadlock Prevention

**No deadlocks possible because:**
1. Only single mutex per component (no nested locks)
2. No callbacks hold locks (callbacks are async)
3. Backend operations don't re-enter Voice module
4. Timeout-based cleanup prevents resource exhaustion

**Resource Limits (enforced):**
- Max session store size: Configurable (default: 10,000 sessions)
- Max stream buffer: 64 KB per stream
- Max conversation history: 1,000 turns per session
- Audio frame timeout: 1 second (config)

---

## 6. State Machine Contracts (Frozen Phase 1)

### 6.1 Session State Transitions

```
enum class SessionState {
    ACTIVE,      // Session created, user is interacting
    IDLE,        // No activity for idle_timeout_ms
    EXPIRED,     // Exceeded max_session_duration_ms
    TERMINATED   // Cleaned up
};

Valid Transitions (Immutable):
  CREATE     → ACTIVE
  ACTIVE     → IDLE        (after idle_timeout_ms)
  ACTIVE     → EXPIRED     (after max_session_duration_ms)
  ACTIVE     → TERMINATED  (on terminate())
  IDLE       → ACTIVE      (on touchSession())
  IDLE       → EXPIRED     (after max_session_duration_ms)
  IDLE       → TERMINATED  (on cleanup)
  EXPIRED    → TERMINATED  (on cleanup)

Invalid Transitions (Rejected):
  IDLE → CREATE, IDLE → IDLE
  EXPIRED → ACTIVE, EXPIRED → IDLE
  TERMINATED → * (no transitions from TERMINATED)
```

### 6.2 Streaming State Machine

```
enum class StreamState {
    CONNECTED,   // Session created, ready to receive audio
    STREAMING,   // Audio flowing, STT active
    CLOSED,      // Session terminated
    ERROR        // Unrecoverable error
};

Valid Transitions:
  CREATE  → CONNECTED
  CONNECTED → STREAMING  (first audio frame sent)
  STREAMING → STREAMING  (frames flowing)
  STREAMING → CLOSED     (on closeStream())
  ANY → ERROR            (on fatal error)
  ERROR → CLOSED         (on cleanup)
```

---

## 7. Integration Boundaries

### 7.1 Consumed Dependencies

| Module | Interface | Usage |
|---|---|---|
| `content/stt_processor.h` | Speech-to-text | Convert audio → text |
| `content/tts_processor.h` | Text-to-speech | Convert text → audio |
| `llm/llama_wrapper.h` | LLM inference | Generate responses |
| `security/auth_manager.h` | User authentication | Verify user identity |
| `storage/kv_store.h` | Key-value storage | Persist sessions |

### 7.2 Produced Interfaces

| Interface | Consumers | Responsibility |
|---|---|---|
| `voice_session_manager.h` | Voice API, CLI, tests | Session lifecycle |
| `voice_assistant.h` | API handlers, assistants | Command routing |
| `voice_browser_streaming.h` | Web frontend | Real-time streaming |
| `voice_telephony.h` | Telephony handlers | Call management |
| `voice_auth.h` | Security, voice API | Speaker verification |

---

## 8. Resource Limits & SLA Expectations

### 8.1 Resource Limits (Configuration)

```cpp
// Session limits
max_concurrent_sessions = unbounded (external limit recommended: 10,000)
idle_timeout_ms = 5 * 60 * 1000          // 5 minutes
max_session_duration_ms = 60 * 60 * 1000  // 1 hour
cleanup_interval_ms = 30 * 1000            // 30 seconds

// Streaming limits
max_chunk_size_bytes = 64 * 1024           // 64 KB
max_concurrent_streams = 100               // Per streaming manager
chunk_timeout_ms = 1000                    // 1 second
stt_batch_frames = 4                       // Coalesce 4 frames

// Audio limits
min_sample_rate = 8000                     // 8 kHz
max_sample_rate = 48000                    // 48 kHz
bits_per_sample = 16                       // 16-bit PCM required
```

### 8.2 SLA Expectations (Typical Hardware)

| Metric | Target | Notes |
|---|---|---|
| **Latency** | | |
| Audio→Transcript (STT) | 200-500ms | Depends on audio duration |
| Transcript→Response (LLM) | 100-500ms | Model size & hardware dependent |
| End-to-end latency | 500-1500ms | For short commands (<5s audio) |
| **Throughput** | | |
| Concurrent streams | 100+ | Per streaming manager |
| Concurrent sessions | 1,000+ | Per session manager |
| Commands/second | 10-50 | Depends on LLM and backend |
| **Reliability** | | |
| Session success rate | >95% | Excludes user auth failures |
| Stream stability | >99% | For 5+ minute streams |
| Command accuracy | 85-95% | Depends on audio quality |

---

## 9. Failure Modes & Degradation Paths

### 9.1 Graceful Degradation

| Failure | Behavior | Fallback |
|---|---|---|
| LLM timeout | Return "I didn't understand" | Direct command routing |
| TTS unavailable | Return text response only | No audio output |
| STT error | Return partial transcript if available | Manual input fallback |
| Session expired | Return error 6602 | Retry with new session |
| Auth failure | Reject command (error 7000) | Re-authenticate & retry |
| Stream buffer full | Signal error 6902 | Slow down sender, retry |

### 9.2 Circuit Breaker Pattern

```cpp
// Prevents cascading failures
CircuitBreakerConfig {
    failure_threshold = 5,           // Open after 5 failures
    success_threshold = 2,           // Close after 2 successes
    open_duration_ms = 30000,        // Stay open for 30s
    half_open_probe_interval = 5000  // Probe every 5s
};

States:
  CLOSED     → (failures >= threshold) → OPEN
  OPEN       → (time >= open_duration) → HALF_OPEN
  HALF_OPEN  → (probe succeeds) → CLOSED
  HALF_OPEN  → (probe fails) → OPEN
```

---

## 10. Operational Considerations

### 10.1 Logging & Monitoring

**Audit Events Logged (immutable list):**
- `session_created` - New session created
- `session_terminated` - Session ended
- `command_received` - Command processed
- `authentication_success` - User verified
- `authentication_failure` - Auth check failed
- `pii_detected` - PII found in transcript
- `transcript_generated` - STT complete
- `response_generated` - LLM complete
- `streaming_started` - Stream created
- `streaming_error` - Stream error

**Metrics to Monitor:**
- Active sessions (gauge)
- Commands/min (counter)
- Avg latency (histogram)
- Error rate by code (counter)
- Circuit breaker state (gauge)

### 10.2 Deployment Topology

```
┌────────────────────────────────────────────────────────────┐
│                  Production Deployment                      │
├────────────────────────────────────────────────────────────┤
│                                                              │
│  Load Balancer                                              │
│         │                                                    │
│    ┌────┴────┬────────┬────────┐                           │
│    ▼         ▼        ▼        ▼                           │
│  Pod1     Pod2      Pod3     Pod4  (Kubernetes replicas)  │
│  ├─VoiceMgr │─VoiceMgr │─VoiceMgr │─VoiceMgr            │
│  ├─Session │─Session │─Session │─Session               │
│  └─Stream   │─Stream   │─Stream   │─Stream    Mgr        │
│    │        │        │        │                           │
│    └────────┴────────┴────────┘                           │
│             │                                              │
│             ▼                                              │
│  Shared Redis (Session Cache)                            │
│  PostgreSQL (Session Persistence)                        │
│  S3 / Object Storage (Recordings)                        │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

## 11. Sourcecode Verification (Module: voice/architecture)

**Verified Components:**
- ✅ voice_session_manager.h/cpp - Session lifecycle, persistence
- ✅ voice_assistant.h/cpp - Command orchestration
- ✅ voice_browser_streaming.h/cpp - WebSocket streaming
- ✅ voice_telephony.h/cpp - SIP/WebRTC integration
- ✅ voice_auth.h/cpp - Biometric authentication
- ✅ voice_security.h/cpp - Security & privacy
- ✅ audio_preprocessing.h/cpp - Audio validation & filtering
- ✅ voice_error_handler.h/cpp - Error handling, circuit breaker
- ✅ voice_assistant_llm.cpp - LLM integration

**Interface Verification:**
- ✅ All public methods documented with Doxygen
- ✅ Thread-safety guarantees documented
- ✅ Error codes (42 total) documented
- ✅ Resource limits enforced and tested
- ✅ State machines immutable (Phase 1 frozen)

---

**Document Version:** v1.0-production (frozen 2026-08-08)  
**Level:** 1 (Module-Level Developer Documentation)  
**Source:** `src/voice/ARCHITECTURE.md`  
**Related:** Level 2: `docs/voice/README.md` (public API docs)
