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

# Voice Feature Representation

> Exact model backend not verifiable from final document text alone.

**Audience:** Developers maintaining voice biometric storage and comparison. In this module, “embedding” refers only to the internal speaker feature vector; there is no general-purpose text/audio semantic embedding or vector-database contract in the voice API.

## Representation in source

[`voice_auth.h`](../../include/voice/voice_auth.h) and [`voice_authenticator.cpp`](voice_authenticator.cpp) define a 32-value handcrafted vector:

- Eight sub-band RMS energy values and eight zero-crossing-rate values.
- Four global spectral-moment values (centroid, spread, skewness, kurtosis).
- Four temporal/signal values (global RMS, crest factor, peak energy, spectral flatness).
- Eight consecutive sub-band RMS-difference values.

Enrollment computes features from each sample that passes its quality and, by default, liveness checks. The accepted vectors are averaged and L2-normalized into one profile per `user_id`. Verification normalizes a probe and compares it with a profile using cosine similarity. Identification only searches the candidate profile IDs supplied by its caller and sorts matches by score.

## Input and lifecycle constraints

- The API accepts raw PCM byte vectors; callers must provide the expected PCM representation and suitable audio quality. The biometric API itself does not carry an explicit sample-rate field on each sample.
- `EnrollmentConfig` defaults to three samples, a 3,000 ms sample-duration expectation, quality threshold 0.60, and liveness required. The implementation rejects an empty ID, too few supplied or surviving samples, and duplicate enrollment for the same user.
- Verification against an absent profile returns a negative result; an empty probe does not authenticate. Identification with no candidates or empty audio returns no matches.
- Profile IDs and feature vectors are held in the authenticator's in-process maps. This interface does not define persistence, cross-process synchronization, or remote embedding service behavior.

## Semantics and limitations

This is a fixed-dimensional acoustic descriptor used for similarity matching, not a learned representation with semantic invariance. Similarity scores and configured thresholds must not be described as calibrated probabilities. Microphone, channel, codec, sample rate, duration, and environmental differences can move scores. The matching feature vector is biometric data and must be treated as sensitive even though it is not raw audio.

The liveness decision is a separate heuristic gate; it does not make the feature representation spoof-proof. The API's 1:N search is bounded by the caller-provided candidate list, but this documentation does not promise a latency or accuracy target.

## Validation before changing representation

Preserve separate checks for enrollment eligibility, liveness, comparison threshold, and profile ownership. Compare any proposed feature revision against the current representation using disjoint speakers and sessions; report false-accept/false-reject rates, ROC/DET curves, quality rejection rates, and runtime/memory cost. Add regression cases for empty/odd-byte/short input, insufficient accepted enrollment samples, duplicate user enrollment, missing profiles, and score values around the threshold. Existing coverage is in [`test_voice_adversarial_anti_spoof.cpp`](../../tests/voice/test_voice_adversarial_anti_spoof.cpp) and [`test_voice_auth_security_focused.cpp`](../../tests/voice/test_voice_auth_security_focused.cpp); the evaluation method is detailed in [`RESEARCH.md`](RESEARCH.md).

## Related contracts

- [`API.md`](API.md) — public class behavior and result semantics.
- [`TRUST.md`](TRUST.md) — sensitivity and trust-boundary requirements for biometric data.
- [`SECURITY.md`](SECURITY.md) — access, deletion, and incident controls.
