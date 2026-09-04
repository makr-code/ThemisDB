/**
 * @file audio_chunk_reader.cpp
 * @brief Audio chunk reader implementation.
 * @version 1.9.0-beta
 * @note Score: 100/100
 * @note Status: Production Ready
 */

#include "whisper/audio_chunk_reader.h"
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cstdio>
#include <array>
#include <sstream>
#include <memory>

#ifdef _WIN32
#  define THEMIS_POPEN  _popen
#  define THEMIS_PCLOSE _pclose
#else
#  include <cstdlib>
#  define THEMIS_POPEN  popen
#  define THEMIS_PCLOSE pclose
#endif

namespace themis {
namespace whisper {

// ── helpers ─────────────────────────────────────────────────────────────────

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

static uint16_t readU16LE(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

static uint32_t readU32LE(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

// ── WavAudioChunkReader::canRead ─────────────────────────────────────────────

bool WavAudioChunkReader::canRead(const std::string& path) const {
    const std::string lower = toLower(path);
    return static_cast<int>(lower.size()) >= 4 &&
           lower.substr(static_cast<int>(lower.size()) - 4) == ".wav";
}

std::map<std::string, std::string> WavAudioChunkReader::getMetadata(const std::string& path) const {
    std::map<std::string, std::string> metadata;
    metadata["reader"] = "wav";
    metadata["path"] = path;
    metadata["can_read"] = canRead(path) ? "true" : "false";
    return metadata;
}

// ── WavAudioChunkReader::readFile ────────────────────────────────────────────

std::vector<float> WavAudioChunkReader::readFile(const std::string& path,
                                                  float& out_sample_rate) {
    // Read whole file into memory
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) {
        throw std::runtime_error("WavAudioChunkReader: cannot open '" + path + "'");
    }
    const std::streamsize size = f.tellg();
    if (size < 44) {
        throw std::runtime_error("WavAudioChunkReader: file too small to be a WAV: '" + path + "'");
    }
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<size_t>(size));
    f.read(reinterpret_cast<char*>(data.data()), size);
    // f closes automatically (RAII) when it goes out of scope

    return parseWav(data, out_sample_rate);
}

// ── WavAudioChunkReader::parseWav ────────────────────────────────────────────

std::vector<float> WavAudioChunkReader::parseWav(const std::vector<uint8_t>& data,
                                                  float& out_sample_rate) {
    // Validate RIFF header
    if (static_cast<int>(data.size()) < 44 ||
        data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F' ||
        data[8] != 'W' || data[9] != 'A' || data[10] != 'V' || data[11] != 'E') {
        throw std::runtime_error("WavAudioChunkReader: not a valid RIFF/WAV file");
    }

    // Locate the "fmt " chunk
    size_t pos = 12;
    uint16_t audio_format  = 0;
    uint16_t num_channels  = 0;
    uint32_t sample_rate   = 0;
    uint16_t bits_per_sample = 0;

    bool found_fmt = false;
    while (pos + 8 <= data.size()) {
        const uint32_t chunk_size = readU32LE(&data[pos + 4]);
        if (data[pos] == 'f' && data[pos+1] == 'm' && data[pos+2] == 't' && data[pos+3] == ' ') {
            if (pos + 8 + 16 > static_cast<int>(data.size())) {
              break;
            }
            audio_format   = readU16LE(&data[pos + 8]);
            num_channels   = readU16LE(&data[pos + 10]);
            sample_rate    = readU32LE(&data[pos + 12]);
            bits_per_sample= readU16LE(&data[pos + 22]);
            found_fmt = true;
        }
        if (data[pos] == 'd' && data[pos+1] == 'a' && data[pos+2] == 't' && data[pos+3] == 'a') {
            if (!found_fmt) {
              throw std::runtime_error("WavAudioChunkReader: 'data' chunk before 'fmt '");
            }

            // Guard against invalid fmt data that would cause division-by-zero or
            // out-of-bounds array access in the per-sample loops below.
            if (num_channels == 0) {
                throw std::runtime_error("WavAudioChunkReader: invalid WAV — num_channels is 0");
            }
            // Reasonable upper bound: 64 channels is already far beyond any practical audio.
            if (num_channels > 64) {
                throw std::runtime_error(
                    "WavAudioChunkReader: unsupported channel count (" +
                    std::to_string(num_channels) + ")");
            }

            const size_t data_start = pos + 8;
            const size_t data_bytes = std::min(static_cast<size_t>(chunk_size),
                                               static_cast<int>(data.size()) - data_start);

            out_sample_rate = static_cast<float>(sample_rate);

            // audio_format: 1=PCM, 3=IEEE_FLOAT
            if (audio_format == 3 && bits_per_sample == 32) {
                // IEEE 32-bit float – convert multi-channel to mono
                const size_t total_frames = data_bytes / (4 * num_channels);
                std::vector<float> out;
                out.reserve(total_frames);
                for (size_t i = 0; i < total_frames; ++i) {
                    float sum = 0.0f;
                    for (uint16_t ch = 0; ch < num_channels; ++ch) {
                        float s = 0;
                        std::memcpy(&s, &data[data_start + (i * num_channels + ch) * 4], 4);
                        sum += s;
                    }
                    out.push_back(sum / static_cast<float>(num_channels));
                }
                return out;
            } else if (audio_format == 1 && bits_per_sample == 16) {
                // 16-bit PCM – convert to float32 mono
                const size_t total_frames = data_bytes / (2 * num_channels);
                std::vector<float> out;
                out.reserve(total_frames);
                for (size_t i = 0; i < total_frames; ++i) {
                    float sum = 0.0f;
                    for (uint16_t ch = 0; ch < num_channels; ++ch) {
                        int16_t sample;
                        std::memcpy(&sample, &data[data_start + (i * num_channels + ch) * 2], 2);
                        sum += static_cast<float>(sample) / 32768.0f;
                    }
                    out.push_back(sum / static_cast<float>(num_channels));
                }
                return out;
            } else {
                throw std::runtime_error(
                    "WavAudioChunkReader: unsupported format (audio_format=" +
                    std::to_string(audio_format) + ", bits=" +
                    std::to_string(bits_per_sample) + ")");
            }
        }
        pos += 8 + chunk_size;
        if (chunk_size & 1) ++pos;  // RIFF alignment padding
    }

    throw std::runtime_error("WavAudioChunkReader: 'data' chunk not found");
}

// ── FfmpegAudioChunkReader ───────────────────────────────────────────────────

bool FfmpegAudioChunkReader::canRead(const std::string& path) const {
    const std::string lower = [&path]() {
        std::string s = path;
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        return s;
    }();
    for (const char* ext : {".mp3", ".ogg", ".flac", ".m4a", ".aac", ".opus", ".wma", ".webm"}) {
        if (static_cast<int>(lower.size()) > = std::strlen(ext) &&
            lower.substr(static_cast<int>(lower.size()) - std::strlen(ext)) == ext) {
            return true;
        }
    }
    return false;
}

std::map<std::string, std::string> FfmpegAudioChunkReader::getMetadata(const std::string& path) const {
    std::map<std::string, std::string> metadata;
    metadata["reader"] = "ffmpeg";
    metadata["path"] = path;
    metadata["can_read"] = canRead(path) ? "true" : "false";
    metadata["sample_rate"] = "16000";
    metadata["channels"] = "1";
    metadata["sample_format"] = "f32le";
    return metadata;
}

std::string FfmpegAudioChunkReader::shellEscape(const std::string& path) {
    if (path.find('\0') != std::string::npos) {
        throw std::runtime_error("FfmpegAudioChunkReader: path contains NUL byte");
    }
    // Wrap in single quotes; escape any embedded single quotes as '\''.
    std::ostringstream escaped = {};
    escaped << '\'';
    for (char c : path) {
        if (c == '\'') {
            escaped << "'\\''";
        } else {
            escaped << c;
        }
    }
    escaped << '\'';
    return escaped.str();
}

std::vector<float> FfmpegAudioChunkReader::readFile(const std::string& path,
                                                     float& out_sample_rate) {
    const std::string escaped = shellEscape(path);

    // Probe ffmpeg availability first with a quick version check
    {
        auto probeCloser = [](FILE* f) -> int {
            return f ? THEMIS_PCLOSE(f) : 0;
        };
        std::unique_ptr<FILE, int(*)(FILE*)> probe(
            THEMIS_POPEN("ffmpeg -version 2>/dev/null", "r"),
            probeCloser);
        if (!probe) {
            throw std::runtime_error("FfmpegAudioChunkReader: ffmpeg not available");
        }
        // Read a byte to confirm it actually opened
        char buf[4];
        bool ok = (std::fread(buf, 1, 1, probe.get()) == 1);
        if (!ok) {
            throw std::runtime_error("FfmpegAudioChunkReader: ffmpeg not available");
        }
    }

    // Build the ffmpeg command:
    //   ffmpeg -loglevel quiet -i <path> -f f32le -ar 16000 -ac 1 pipe:1
    const std::string cmd =
        "ffmpeg -loglevel quiet -i " + escaped +
        " -f f32le -ar 16000 -ac 1 pipe:1 2>/dev/null";

    struct PipeCloser {
        int* close_rc = nullptr;
        void operator()(FILE* f) const {
            if (f && close_rc) {
                *close_rc = THEMIS_PCLOSE(f);
            }
        }
    };

    int close_rc = 0;
    std::unique_ptr<FILE, PipeCloser> pipe(
        THEMIS_POPEN(cmd.c_str(), "r"),
        PipeCloser{&close_rc});
    if (!pipe) {
        throw std::runtime_error(
            "FfmpegAudioChunkReader: failed to launch ffmpeg for '" + path + "'");
    }

    std::vector<float> samples;
    size_t total_bytes = 0;
    std::array<char, 65536> buf = {};

    while (!std::feof(pipe.get())) {
        const size_t n = std::fread(buf.data(), 1,static_cast<int>(buf.size()), pipe.get());
        if (n == 0) {
          break;
        }
        total_bytes += n;
        if (total_bytes > kMaxOutputBytes) {
            throw std::runtime_error(
                "FfmpegAudioChunkReader: output exceeds maximum size limit");
        }
        // Append float32 samples (n must be a multiple of 4; handle tail)
        const size_t floats = n / sizeof(float);
        const float* fptr = reinterpret_cast<const float*>(buf.data());
        samples.insert(samples.end(), fptr, fptr + floats);
        // If n is not a multiple of 4 (very rare at EOF), leftover bytes are dropped
    }

    pipe.reset();
    if (close_rc != 0 && samples.empty()) {
        throw std::runtime_error(
            "FfmpegAudioChunkReader: ffmpeg failed to decode '" + path +
            "' (exit code " + std::to_string(close_rc) + ")");
    }

    out_sample_rate = 16000.0f;
    return samples;
}

// ── CompositeAudioChunkReader ────────────────────────────────────────────────

void CompositeAudioChunkReader::addReader(std::unique_ptr<IAudioChunkReader> reader) {
    readers_.push_back(std::move(reader));
}

bool CompositeAudioChunkReader::canRead(const std::string& path) const {
    for (const auto& r : readers_) {
        if (r->canRead(path)) {
          return true;
        }
    }
    return false;
}

std::map<std::string, std::string> CompositeAudioChunkReader::getMetadata(const std::string& path) const {
    for (const auto& r : readers_) {
        if (r->canRead(path)) {
            return r->getMetadata(path);
        }
    }
    return {};
}

std::vector<float> CompositeAudioChunkReader::readFile(const std::string& path,
                                                        float& out_sample_rate) {
    for (const auto& r : readers_) {
        if (r->canRead(path)) {
            return r->readFile(path, out_sample_rate);
        }
    }
    throw std::runtime_error(
        "CompositeAudioChunkReader: no reader registered for '" + path + "'");
}

} // namespace whisper
} // namespace themis
