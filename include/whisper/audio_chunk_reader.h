/**
 * @file audio_chunk_reader.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: audio_chunk_reader.h | Version: 0.0.10
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace themis {
namespace whisper {

/**
 * @brief Interface for reading audio files and returning PCM float32 samples.
 */
class IAudioChunkReader {
public:
    virtual ~IAudioChunkReader() = default;

    /**
     * @brief Read an audio file and return mono float32 PCM samples at the
     *        native sample rate (set via out_sample_rate).
     * @throws std::runtime_error if the file cannot be read or format is unsupported.
     */
    [[nodiscard]] virtual std::vector<float> readFile(const std::string& path,
                                        float& out_sample_rate) = 0;

    /**
     * @brief Return true if this reader can handle the given file path/extension.
     */
    [[nodiscard]] virtual bool canRead(const std::string& path) const = 0;
};

/**
 * @brief Minimal RIFF/WAV reader.  Supports 16-bit PCM and 32-bit float WAV.
 *
 * No external library dependency.  If the file is not a valid WAV,
 * readFile() throws std::runtime_error.
 */
class WavAudioChunkReader : public IAudioChunkReader {
public:
    std::vector<float> readFile(const std::string& path,
                                float& out_sample_rate) override;

    bool canRead(const std::string& path) const override;

private:
    std::vector<float> parseWav(const std::vector<uint8_t>& data,
                                float& out_sample_rate);
};

/**
 * @brief Audio reader that decodes MP3, OGG, FLAC and other formats by shelling
 *        out to the `ffmpeg` binary.
 *
 * The binary must be on PATH.  Output is always resampled to 16 kHz mono
 * float32 PCM.
 *
 * Security: the file path is shell-escaped before being passed to the subprocess.
 * Files whose path contains characters that cannot be safely escaped are rejected
 * with std::runtime_error.
 *
 * If `ffmpeg` is not found on PATH, readFile() throws
 * std::runtime_error("ffmpeg not available").
 */
class FfmpegAudioChunkReader : public IAudioChunkReader {
public:
    /// Maximum raw PCM output accepted from ffmpeg (≈ 3.5 h at 16 kHz mono f32).
    static constexpr size_t kMaxOutputBytes = 500UL * 1024UL * 1024UL;

    std::vector<float> readFile(const std::string& path,
                                float& out_sample_rate) override;

    bool canRead(const std::string& path) const override;

private:
    /// Shell-escape a path for use inside single quotes.
    /// Throws std::runtime_error if the path contains a NUL byte.
    static std::string shellEscape(const std::string& path);
};

/**
 * @brief Composite reader that delegates to the first registered
 *        IAudioChunkReader whose canRead() returns true.
 *
 * Readers are tried in registration order.  If none accepts the file,
 * readFile() throws std::runtime_error.
 */
class CompositeAudioChunkReader : public IAudioChunkReader {
public:
    /// Register a reader.  Readers are tried in the order they are added.
    void addReader(std::unique_ptr<IAudioChunkReader> reader);

    std::vector<float> readFile(const std::string& path,
                                float& out_sample_rate) override;

    bool canRead(const std::string& path) const override;

private:
    std::vector<std::unique_ptr<IAudioChunkReader>> readers_;
};

} // namespace whisper
} // namespace themis
