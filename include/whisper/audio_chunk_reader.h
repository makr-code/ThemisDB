#pragma once

#include <string>
#include <vector>
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
    virtual std::vector<float> readFile(const std::string& path,
                                        float& out_sample_rate) = 0;

    /**
     * @brief Return true if this reader can handle the given file path/extension.
     */
    virtual bool canRead(const std::string& path) const = 0;
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

} // namespace whisper
} // namespace themis
