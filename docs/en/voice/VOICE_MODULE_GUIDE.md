# Voice Module Configuration & Usage Guide

## Overview

The ThemisDB Voice Module provides a full production-ready speech pipeline covering audio preprocessing, speech-to-text (STT), text-to-speech (TTS), intent detection, session management, meeting support, audio storage, security & PII redaction, error handling, accessibility (closed captions), model caching, and batch quality processing.

### Module Phases

| Phase | Component | Purpose |
|-------|-----------|---------|
| 1 | `AudioPreprocessingPipeline` | Noise reduction, echo cancellation, VAD, normalization |
| 2 | `VoiceTTSCustomizer` | TTS voice profiles, SSML, emotion & rate control |
| 3 | `VoiceIntentDetector` | Slot-filling, confidence-ranked intent detection |
| 4 | `VoiceMeetingSupport` | Speaker diarization, live transcription, meeting notes |
| 5 | `VoiceAudioStorage` | Hot/warm/cold tiered audio storage with metadata |
| 6 | `VoiceSessionManager` | Conversation sessions, context tracking |
| 7 | `VoiceSecurity` | PII redaction, consent management, audit logging |
| 8 | `VoiceErrorHandler` | Circuit breaker, retry logic, structured error events |
| 9 | `VoiceAccessibility` | Closed captions (VTT, SRT, plain text, HTML, JSON) |
| 10 | `VoiceModelCache` | LRU model cache with memory limits and auto-eviction |
| 10 | `VoiceBatchProcessor` | Batch audio processing, WER, PESQ-like quality metrics |
| 11 | `VoiceBiometricAuthenticator` | Voice biometric authentication: enrollment, 1:1 verification, 1:N identification, liveness detection |
| 2b | `WakeWordDetector` | Hands-free activation via configurable wake-word phrases |

---

## Configuration

All components accept configuration structs passed at construction time. Sensible defaults are provided for every field.

### Audio Preprocessing

```cpp
#include "voice/audio_preprocessing.h"

themis::voice::PreprocessingOptions opts;
opts.enable_noise_reduction    = true;
opts.enable_echo_cancellation  = false;
opts.enable_vad                = true;
opts.enable_normalization      = true;
opts.vad_threshold             = 0.5f;
opts.noise_reduction_strength  = 0.7f;
opts.target_rms                = 0.1f;
opts.target_sample_rate        = 16000;

themis::voice::AudioPreprocessingPipeline pipeline(opts);
```

### Session Management

```cpp
#include "voice/voice_session_manager.h"

themis::voice::SessionConfig cfg;
cfg.max_sessions         = 100;
cfg.session_timeout_ms   = 300000; // 5 minutes
cfg.max_context_turns    = 20;

themis::voice::VoiceSessionManager mgr(cfg);
```

### Model Cache

```cpp
#include "voice/voice_model_cache.h"

themis::voice::ModelCacheConfig cache_cfg;
cache_cfg.max_memory_bytes     = 2ULL * 1024 * 1024 * 1024; // 2 GB
cache_cfg.max_models           = 5;
cache_cfg.enable_lru_eviction  = true;
cache_cfg.pin_frequently_used  = true;
cache_cfg.pin_threshold        = 50;

themis::voice::VoiceModelCache cache(cache_cfg);
```

### Batch Processor

```cpp
#include "voice/voice_batch_processor.h"

themis::voice::BatchProcessorConfig bp_cfg;
bp_cfg.default_batch_size       = 8;
bp_cfg.compute_quality_metrics  = true;
bp_cfg.compute_wer              = true;
bp_cfg.item_timeout_ms          = 30000;

themis::voice::VoiceBatchProcessor processor(bp_cfg);
```

---

## Quick Start

```cpp
#include "voice/audio_preprocessing.h"
#include "voice/voice_session_manager.h"
#include "voice/voice_security.h"
#include "voice/voice_accessibility.h"

// 1. Create a session
themis::voice::VoiceSessionManager sessions;
std::string session_id = sessions.createSession("user-42");

// 2. Preprocess audio
themis::voice::AudioPreprocessingPipeline pipeline;
auto result = pipeline.process(raw_audio_bytes, 16000);

// 3. Generate captions
themis::voice::VoiceAccessibility accessibility;
auto cues = accessibility.generateCaptions({{0, "Hello, world."}}, "Speaker1");

themis::voice::TranscriptExportOptions opts;
opts.format = themis::voice::CaptionFormat::VTT;
auto export_result = accessibility.exportTranscript(cues, opts);
std::cout << export_result.content;
```

---

## STT Configuration

STT is integrated into the preprocessing pipeline. Configure VAD to gate silence:

```cpp
opts.enable_vad       = true;
opts.vad_threshold    = 0.4f; // Lower = more sensitive
```

For streaming, use `processFrame()`:

```cpp
themis::voice::AudioFrame frame;
frame.samples     = my_float_samples;
frame.sample_rate = 16000;
frame.channels    = 1;
auto frame_result = pipeline.processFrame(frame);
```

---

## TTS Configuration

```cpp
#include "voice/voice_tts_customizer.h"

themis::voice::VoiceTTSCustomizer tts;
themis::voice::TTSVoiceProfile profile;
profile.voice_id      = "en-US-neural";
profile.speaking_rate = 1.0f;
profile.pitch         = 0.0f;
profile.volume        = 1.0f;
tts.setVoiceProfile(profile);

auto tts_result = tts.synthesize("Hello, welcome to ThemisDB.");
```

---

## Session Management

```cpp
// Create and update session
std::string sid = sessions.createSession("user-123");
sessions.addTurn(sid, "user", "What is my balance?");
sessions.addTurn(sid, "assistant", "Your balance is $500.");

// Retrieve context
auto ctx = sessions.getContext(sid);
sessions.endSession(sid);
```

---

## Security & Privacy

```cpp
#include "voice/voice_security.h"

themis::voice::VoiceSecurity security;

// Grant user consent
security.grantConsent("user-123", {"recording", "transcription"});

// Redact PII from transcript
std::string clean = security.redactPII("Call me at 555-1234 or user@example.com");
// Returns: "Call me at [PHONE] or [EMAIL]"

// Verify consent before processing
if (!security.hasConsent("user-123", "recording")) {
    // Deny processing
}
```

---

## Error Handling

```cpp
#include "voice/voice_error_handler.h"

themis::voice::VoiceErrorHandler handler;
handler.onError([](const themis::voice::VoiceError& err) {
    std::cerr << "Voice error: " << err.message << std::endl;
});

// Execute with circuit breaker
auto result = handler.executeWithCircuitBreaker("stt", []() {
    return run_stt();
});
```

---

## Examples

### Closed Captions Export (VTT)

```cpp
#include "voice/voice_accessibility.h"

themis::voice::VoiceAccessibility acc;

// From timed segments
std::vector<std::pair<int64_t, std::string>> segs = {
    {0,    "Hello, everyone."},
    {2000, "Welcome to the meeting."},
    {5000, "Let us begin."}
};
auto cues = acc.generateCaptions(segs, "Host");

themis::voice::TranscriptExportOptions opts;
opts.format = themis::voice::CaptionFormat::VTT;
auto res = acc.exportTranscript(cues, opts);
// res.content contains valid WebVTT text
```

### Caption from JSON Transcript

```cpp
nlohmann::json transcript = {
    {{"text","Good morning"},{"start_ms",0},{"end_ms",1500},{"speaker","Alice"}},
    {{"text","Good morning to you too"},{"start_ms",1600},{"end_ms",3200},{"speaker","Bob"}}
};
auto cues = acc.generateCaptionsFromJSON(transcript);
```

### Batch Quality Processing

```cpp
#include "voice/voice_batch_processor.h"

themis::voice::VoiceBatchProcessor bp;

std::vector<themis::voice::BatchAudioItem> items;
items.push_back({"item-1", my_audio_bytes, 16000, "expected transcript"});
items.push_back({"item-2", other_audio, 16000, ""});

auto results = bp.processBatchSync(items, [](const std::string& job, size_t done, size_t total) {
    std::cout << job << ": " << done << "/" << total << "\n";
});

for (auto& r : results) {
    std::cout << r.item_id << " PESQ=" << r.pesq_score << " WER=" << r.wer_score << "\n";
}
```

### Model Cache

```cpp
#include "voice/voice_model_cache.h"

themis::voice::VoiceModelCache cache;

// Register loader/unloader for "stt" model type
cache.registerLoader("stt",
    [](const std::string& path, const nlohmann::json&) -> void* {
        return load_stt_model(path); // Your loader
    },
    [](void* handle) {
        unload_stt_model(handle);   // Your unloader
    }
);

auto model = cache.get("stt-en-v1", "/models/stt-en.bin", "stt");
if (model) {
    use_model(model->handle);
}
```

### Wake-Word Detection

The `WakeWordDetector` enables hands-free activation: audio is continuously scanned for a
keyword phrase; when detected, the full voice pipeline is triggered.

```cpp
#include "voice/wake_word_detector.h"

themis::voice::WakeWordConfig ww_cfg;
ww_cfg.sensitivity        = 0.5f;   // 0 = permissive, 1 = strict
ww_cfg.buffer_length_ms   = 1500;   // Rolling audio window
ww_cfg.cooldown_ms        = 1000;   // Time between re-detections (ms)
ww_cfg.vad_min_energy     = 0.005f; // RMS gate – silence is skipped for free
ww_cfg.continuous_listen  = true;   // Keep listening after a hit

themis::voice::WakeWordDetector detector(ww_cfg);
detector.addWakeWord("hey-themis", "hey themis");
detector.addWakeWord("themis",     "themis");

detector.setDetectionCallback([](const themis::voice::WakeWordDetectionResult& r) {
    std::cout << "Wake word fired: " << r.wake_word_id
              << "  confidence=" << r.confidence << "\n";
});

// Microphone loop — feed raw 16-bit PCM chunks
while (mic.isOpen()) {
    auto chunk = mic.read(160);          // e.g. 10 ms @ 16 kHz
    auto res   = detector.processAudioChunk(chunk);
    if (res.detected) {
        start_voice_command_pipeline();
    }
}
```

Integration into `VoiceAssistant` via `detectWakeWord()` / `setWakeWordCallback()`:

```cpp
#include "voice/voice_assistant.h"

themis::voice::VoiceAssistant::Config cfg;
cfg.enable_wake_word            = true;
cfg.wake_word_config.sensitivity = 0.5f;
cfg.wake_words = {{"hey-themis", "hey themis"}};

themis::voice::VoiceAssistant va(cfg);
va.setWakeWordCallback([](const themis::voice::WakeWordDetectionResult& r) {
    // Trigger UI or pipeline here
});

// Per microphone chunk:
va.detectWakeWord(pcm_chunk);
```

**Detection pipeline (two stages):**

| Stage | Description | CPU cost |
|-------|-------------|----------|
| VAD gate | RMS energy vs. `vad_min_energy` | ~0% in silence |
| Keyword scoring | Phrase density × centroid proxy × crest factor | < 0.1 ms/chunk |

**Statistics:**

```cpp
auto stats = detector.getStatistics();
// {
//   "total_chunks_processed": 84000,
//   "total_detections": 12,
//   "registered_wake_words": 2,
//   "buffer_samples": 24000
// }
```

---

### Voice Biometric Authentication

`VoiceBiometricAuthenticator` (Phase 11) provides speaker enrollment, 1:1 verification, 1:N
identification, liveness detection, and a combined `authenticate()` helper.  All operations are
thread-safe.  The feature extractor is model-free; a future neural i-vector/x-vector backend can
be plugged in without changing the public API.

**Enrollment**

```cpp
#include "voice/voice_auth.h"

themis::voice::VoiceAuthConfig cfg;
cfg.verification_threshold   = 0.72f; // Min cosine similarity for a match
cfg.identification_threshold = 0.68f; // Min score for 1:N candidates
cfg.liveness_threshold       = 0.55f; // Min score to accept as live speech

themis::voice::VoiceBiometricAuthenticator auth(cfg);

// Collect ≥ 3 audio samples from the speaker (real microphone audio).
std::vector<std::vector<uint8_t>> samples = {
    record_audio_pcm(3000),   // 3-second PCM clip
    record_audio_pcm(3000),
    record_audio_pcm(3000),
};

themis::voice::VoiceProfileID profile_id;
bool ok = auth.enroll_voice("alice", samples, profile_id);
// profile_id is set on success; persists as long as auth object is alive.
```

**1:1 Verification**

```cpp
auto probe = record_audio_pcm(2000);
auto result = auth.verify_speaker(profile_id, probe);
if (result.verified) {
    // result.match_score  – cosine similarity [0, 1]
    // result.threshold    – decision threshold used
}
```

**1:N Identification**

```cpp
std::vector<themis::voice::VoiceProfileID> candidates = {profile_a, profile_b, profile_c};
auto id_result = auth.identify_speaker(candidates, probe);
if (id_result.identified) {
    // id_result.matches  – sorted by score desc, each with rank, profile_id, user_id
    // id_result.top_match_id / top_match_score
}
```

**Full authentication (liveness + verification)**

```cpp
// Looks up the profile by user_id, checks liveness first, then verifies.
auto auth_result = auth.authenticate("alice", probe);
if (auth_result.authenticated) {
    // auth_result.confidence_score, auth_result.timestamp_ms
} else {
    // auth_result.decision_reason: "liveness_failed", "profile_not_found",
    //                               "verification_failed", …
}
```

**Liveness detection (standalone)**

```cpp
auto liveness = auth.detect_liveness(probe);
// liveness.is_live   – true when the sample appears to be genuine live speech
// liveness.score     – confidence [0, 1]
// liveness.reason    – "live_speech" | "suspected_replay" | "empty_audio"
```

**Profile management**

```cpp
auth.list_profiles();                   // → std::vector<VoiceProfileID>
auth.has_profile(profile_id);           // → bool
auth.get_user_id(profile_id);           // → std::optional<std::string>
auth.delete_profile(profile_id);        // → bool
```

**Statistics**

```cpp
auto stats = auth.get_statistics();
// {
//   "enrolled_profiles":        1,
//   "total_enrollments":        1,
//   "total_verifications":      5,
//   "total_identifications":    2,
//   "successful_authentications": 3
// }
```

> **Note:** The default `EnrollmentConfig` has `require_liveness = true`, which rejects synthetic
> or replayed audio during enrollment.  Disable this only for unit-test purposes with artificial
> audio.

---

## Advanced Configuration

### Accessibility Caption Style

```cpp
themis::voice::CaptionStyle style;
style.max_chars_per_line      = 42;    // Broadcast standard
style.max_lines               = 2;
style.include_speaker_labels  = true;
style.min_duration_ms         = 1000;  // Merge cues shorter than 1s
style.max_duration_ms         = 7000;  // Split cues longer than 7s

themis::voice::VoiceAccessibility acc(style);
```

### Storage Tier Policy

```cpp
#include "voice/voice_audio_storage.h"

themis::voice::StorageTierPolicy policy;
policy.hot_to_warm_after_ms  = 86400000LL;  // 1 day
policy.warm_to_cold_after_ms = 604800000LL; // 7 days

themis::voice::VoiceAudioStorage storage(policy);
storage.applyTierPolicy(); // Demote eligible records
```

---

## Performance Tuning

| Setting | Recommendation | Impact |
|---------|---------------|--------|
| `PreprocessingOptions::enable_echo_cancellation` | Disable if not needed | –20% CPU |
| `ModelCacheConfig::max_models` | Set to max models you load simultaneously | Memory |
| `BatchProcessorConfig::default_batch_size` | 8–16 for CPU, 32+ for GPU | Throughput |
| `SessionConfig::max_context_turns` | Keep ≤ 20 for low-latency | Latency |
| `CaptionStyle::max_duration_ms` | 5000–7000ms | Readability |
| `ModelCacheConfig::pin_frequently_used` | Enable for hot models | Cache hit rate |
