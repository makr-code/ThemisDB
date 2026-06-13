/**
 * @file voice_activity_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace themis {
namespace whisper {

/**
 * @struct SpeechSegment
 * @brief Represents a detected segment of speech in the audio stream.
 *
 * This structure encapsulates the timing and duration information for an identified speech burst.
 * Samples should be interpreted against the known original sample rate of the recording.
 */
struct SpeechSegment {
    /**
     * @brief Inclusive starting sample index of the speech segment in samples.
     */
    std::size_t start_sample = 0; ///< Inclusive start sample index
    /**
     * @brief Exclusive ending sample index of the speech segment in samples.
     */
    std::size_t end_sample   = 0; ///< Exclusive end sample index
};

/**
 * @struct VadConfig
 * @brief Configuration parameters used by the Voice Activity Detector.
 *
 * This holds necessary thresholds and structural limits required to perform reliable pre-detection analysis.
 * Proper configuration is crucial for achieving low false-positive rates in noisy environments.
 */
struct VadConfig {
    /** @var double rms_threshold The Root Mean Square energy threshold (normalized amplitude) below which audio frames are considered silence. */
    double rms_threshold = 0.01;

    /** @var float energy_threshold The minimum normalized energy level that indicates potential speech. */
    float energy_threshold = 0.01f; ///< Minimum normalized energy required to consider a segment for VAD processing.
    /** @var float min_speech_ms The minimum duration in milliseconds considered as valid speech. */
    float min_speech_ms    = 50.0f; ///< Filters out segments that are too short to be meaningful speech.
    /** @var float frame_ms The length of the analysis frame window in milliseconds. */
    float frame_ms         = 20.0f; ///< The fixed frame size used for calculating features like energy.
};

/**
 * @interface IVoiceActivityDetector
 * @brief Strategy interface for Voice Activity Detection.
 *
 * Implementations receive a mono float32 PCM buffer and return a list of
 * sample ranges that contain speech.  The buffer is assumed to be normalised
 * to [-1, 1].
 *
 * Thread-safety: implementations must be safe for concurrent calls.
 */
class IVoiceActivityDetector {
public:
    virtual ~IVoiceActivityDetector() = default;

    /**
     * @brief Detect speech segments in a PCM buffer.
     *
     * @param pcm          Mono float32 samples, normalised to [-1, 1].
     * @param sample_rate  Sampling rate of @p pcm (e.g. 16000.0f).
     * @param cfg          VAD parameters.
     * @return List of speech segments ordered by start_sample.
     */
    [[nodiscard]] virtual std::vector<SpeechSegment>
    detect(const std::vector<float>& pcm,
           float                     sample_rate,
           const VadConfig&          cfg) const = 0;

    /**
     * @brief Initializes the VAD detector with configuration parameters.
     *
     * @param config The configuration object containing operational settings for the detector. Must not be empty.
     */
};

/**
 * @brief Energy-threshold-based Voice Activity Detector.
 *
 * Divides the PCM buffer into fixed-length frames and computes the
 * root-mean-square (RMS) energy of each frame.  Consecutive frames whose
 * RMS exceeds @c VadConfig::energy_threshold are merged into a single
 * SpeechSegment.  Segments shorter than @c VadConfig::min_speech_ms are
 * discarded.
 *
 * Performance: < 5 ms per 1-second chunk at 16 kHz on a modern CPU.
 */
class EnergyThresholdVad : public IVoiceActivityDetector {
public:
    [[nodiscard]] std::vector<SpeechSegment>
    detect(const std::vector<float>& pcm,
           float                     sample_rate,
           const VadConfig&          cfg) const override {
        if (pcm.empty() || sample_rate <= 0.0f) return {};

        const auto frame_samples = static_cast<std::size_t>(
            std::max(1.0f, cfg.frame_ms * 0.001f * sample_rate));
        const auto min_samples = static_cast<std::size_t>(
            std::max(0.0f, cfg.min_speech_ms * 0.001f * sample_rate));

        std::vector<SpeechSegment> segments;
        bool in_speech = false;
        std::size_t seg_start = 0;

        for (std::size_t i = 0; i < pcm.size(); i += frame_samples) {
            const std::size_t end = std::min(i + frame_samples, pcm.size());
            const float rms = frameRms(pcm, i, end);
            const bool is_speech = rms >= cfg.energy_threshold;

            if (is_speech && !in_speech) {
                seg_start = i;
                in_speech = true;
            } else if (!is_speech && in_speech) {
                if (end - seg_start >= min_samples) {
                    segments.push_back({seg_start, end});
                }
                in_speech = false;
            }
        }
        // Close any open segment at end-of-buffer
        if (in_speech && pcm.size() - seg_start >= min_samples) {
            segments.push_back({seg_start, pcm.size()});
        }
        return segments;
    }

private:
    static float frameRms(const std::vector<float>& pcm,
                           std::size_t start, std::size_t end) noexcept {
        float sum = 0.0f;
        for (std::size_t i = start; i < end; ++i) {
            sum += pcm[i] * pcm[i];
        }
        const float count = static_cast<float>(end - start);
        return (count > 0.0f) ? std::sqrt(sum / count) : 0.0f;
    }
};

} // namespace whisper
} // namespace themis
