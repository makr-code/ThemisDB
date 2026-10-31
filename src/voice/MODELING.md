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

# Voice Models and Decision Logic

> Exact model backend not verifiable from final document text alone.

**Audience:** Developers changing detection, classification, or biometric behavior in `src/voice`. This document describes implemented decision logic and its constraints; it does not imply a trained model or measured accuracy.

## Implemented components

### Audio preprocessing and wake-word decisions

`AudioPreprocessingPipeline` works on `AudioFrame` samples and applies configured conversion, filtering, normalization, noise suppression, and voice-activity calculations. Its non-RNNoise suppression path is a simple energy/noise-floor gate. `WakeWordDetector` consumes 16-bit little-endian PCM at its configured sample rate, retains a bounded rolling window, and checks energy, cooldown, phrase score, confidence, and utterance-duration gates. The implementation has a hard confidence gate; the `sensitivity` configuration must not be interpreted as a complete calibration control.

### Intent classification

`VoiceIntentDetector` receives transcript text, classifies it into `QUERY`, `COMMAND`, `QUESTION`, `CONVERSATION`, or `UNKNOWN`, and may attach extracted entities and context hints. The current implementation is keyword/heuristic driven. Its default minimum intent confidence is 0.5 and entity threshold is 0.4; low-confidence and timeout paths have safe defaults. Those thresholds are contracts/configuration values, not benchmark-derived quality guarantees.

### Speaker verification and liveness

`VoiceBiometricAuthenticator` uses 32-dimensional handcrafted signal features. Profiles are normalized averages from enrollment samples; 1:1 verification compares a probe by cosine similarity, while 1:N identification ranks only supplied candidate profile IDs. Liveness is implemented with signal heuristics and minimum input checks. It is not a speaker neural embedding model, cryptographic challenge-response, or proof of authorization.

## Decision inputs and failure handling

| Component | Input expectation | Low-quality or unavailable behavior |
|---|---|---|
| Preprocessing | `AudioFrame` with sample metadata consistent with its samples | Empty input produces empty/unchanged results in individual transforms; callers must preserve format validation |
| Wake word | non-empty 16-bit little-endian PCM | No detection on empty/quiet/low-score/out-of-duration input; timeout result is `detected=false` |
| Intent | non-empty transcript text; optional `ConversationContext` | Low confidence may request clarification; timeout returns `UNKNOWN` with zero confidence |
| Biometric enrollment | user ID and at least configured `min_samples` acceptable PCM samples | Returns false if user ID is empty, too few acceptable samples remain, or a profile already exists |
| Liveness | even-byte PCM with minimum sample length | Empty/short/low-energy/clipped or heuristic-rejected input is not live |

See the corresponding contracts in [`audio_preprocessing.h`](../../include/voice/audio_preprocessing.h), [`wake_word_detector.h`](../../include/voice/wake_word_detector.h), [`voice_intent_detector.h`](../../include/voice/voice_intent_detector.h), and [`voice_auth.h`](../../include/voice/voice_auth.h).

## Evaluation and change control

When modifying a gate, retain its fail-closed behavior and add tests for the boundary and adjacent values. Evaluate false accepts and false rejects separately on held-out speakers and conditions; report class imbalance and threshold calibration. Do not select thresholds based only on the tests' synthetic or adversarial fixtures. Current test and measurement surfaces are listed in [`RESEARCH.md`](RESEARCH.md); the module roadmap records that representative-hardware benchmark baselines are still outstanding.

## Known limitations

- There is no source-backed corpus, trained intent model, or published quality score in this module.
- Performance and recognition quality depend on input encoding, environment, and integrated STT/TTS backend.
- Heuristic liveness and wake-word gates are not substitutes for deployment authorization controls or security review.
- Browser streaming's built-in transcript path is placeholder behavior unless a real backend is injected; see [`IMPLEMENTATION.md`](IMPLEMENTATION.md).
