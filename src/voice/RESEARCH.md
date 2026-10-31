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

# Voice Research Notes

> Exact model backend not verifiable from final document text alone.

**Audience:** Voice maintainers evaluating recognition quality, robustness, or a replacement backend. This page records methods present in the implementation and a reproducible way to evaluate them; it does not claim production accuracy.

## Research questions and assumptions

The current code needs to turn untrusted audio into stable session input, detect wake phrases, classify transcribed text, and optionally compare a claimed speaker with an enrolled profile. Audio format, duration, noise, and the configured backend affect results. The implementation does not include a labeled corpus or published accuracy study, so thresholds in code are operating parameters, not evidence of model quality.

## Methods in the current implementation

- [`audio_preprocessing.cpp`](audio_preprocessing.cpp) converts PCM to normalized samples, estimates noise from the leading tenth of the buffer, applies simple filters/noise attenuation, and computes windowed energy for voice activity. RNNoise processing is conditional on `THEMIS_ENABLE_RNNOISE`; without it, the implementation uses its spectral-gate fallback.
- [`wake_word_detector.cpp`](wake_word_detector.cpp) keeps a rolling PCM buffer, gates on energy and cooldown, scores registered phrases, then applies fixed confidence and duration checks. It is not a neural acoustic model.
- [`voice_intent_detector.cpp`](voice_intent_detector.cpp) classifies transcript text with keyword-oriented heuristics and extracts selected entities; optional low-confidence handling and a timeout default return `UNKNOWN` rather than asserting an unsupported result.
- [`voice_authenticator.cpp`](voice_authenticator.cpp) forms a 32-value hand-engineered speaker feature vector from energy, zero-crossing, spectral-moment, and temporal features. Enrollment averages and L2-normalizes accepted samples; verification uses cosine similarity. Liveness uses deterministic signal heuristics (including clipping, energy, crest factor, spectral flatness, and zero-crossing variability), not a learned anti-spoof model.
- Browser streaming has a separate backend boundary. Without an injected transcription function, the current implementation emits synthetic placeholder transcript text; see [`voice_browser_streaming.cpp`](voice_browser_streaming.cpp).

## Evaluation protocol

Use fixed, consented audio fixtures and retain the dataset version, sample rate, channel count, codec, label provenance, build options, threshold configuration, and test revision with every result. Keep speakers and recording sessions separated between tuning and holdout sets to avoid leakage.

| Path | Measures to report | Existing validation surfaces |
|---|---|---|
| Preprocessing / VAD | active-window precision/recall, SNR change, clipping rate, added latency | [`test_voice_audio_preprocessing_focused.cpp`](../../tests/voice/test_voice_audio_preprocessing_focused.cpp), `bench_voice_audio_preprocessing.cpp` |
| Wake word | false accepts per hour, miss rate, detection delay, results by noise/profile | [`test_voice_wake_word_focused.cpp`](../../tests/voice/test_voice_wake_word_focused.cpp), `bench_voice_wake_word_batch.cpp` |
| Intent | per-class precision/recall, UNKNOWN rate, calibration, timeout rate | `tests/voice/test_voice_wave_a_noisy_wakeword.cpp`, detector statistics |
| Speaker verification / liveness | false-accept and false-reject rates, ROC/DET, replay/noisy-input slices | [`test_voice_adversarial_anti_spoof.cpp`](../../tests/voice/test_voice_adversarial_anti_spoof.cpp), [`test_voice_auth_security_focused.cpp`](../../tests/voice/test_voice_auth_security_focused.cpp) |
| Streaming / sessions | rejection correctness, teardown completion, p50/p95/p99 latency, memory under concurrency | [`test_voice_stream_validation.cpp`](../../tests/voice/test_voice_stream_validation.cpp), [`test_voice_browser_streaming.cpp`](../../tests/voice/test_voice_browser_streaming.cpp), voice benchmarks |

These are evaluation measures, not results. The module roadmap says representative-hardware p95/p99 baselines and full release-critical execution evidence remain pending; do not infer a benchmark pass from test registration alone.

## Trade-offs and limits

Simple thresholds and hand-built features are inspectable and avoid a mandatory model artifact, but can be sensitive to microphones, codecs, language, noise, and speaker variation. Threshold changes require test evidence on independent data; a lower false-reject rate may increase false accepts. The current liveness heuristics are not proof against modern replay or synthesis attacks.

For any proposed learned backend, compare it against the current path on the same held-out data, report error rates and latency by input slice, and preserve the fail-closed behavior on timeout, malformed audio, and unavailable backends. See [`MODELING.md`](MODELING.md), [`EMBEDDING.md`](EMBEDDING.md), and [`ROADMAP.md`](ROADMAP.md).
