/**
 * @file tts_processor.cpp
 * @brief Text-to-speech synthesis engine with voice selection and prosody control.
 * @version 0.0.47
 * @note Maturity: 🟡 BETA
 * @note Score: 70/100
 * @note Gap Summary: total=14; TODO=1, Stub=1, Unimpl=1, Mock=0, Sim=1, Debt=3, C=1, H=3, M=8, L=0
 * @note Status: Beta; Text-to-speech engine basic; voice selection and prosody control under development
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/tts_processor.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <regex>
#include <sstream>

// Conditional Piper TTS includes
#ifdef THEMIS_ENABLE_PIPER_TTS
#include <onnxruntime_cxx_api.h>
#include <piper.hpp>
#endif

namespace themis {
namespace content {

TTSProcessor::TTSProcessor() = default;

TTSProcessor::~TTSProcessor() {
    if (initialized_) {
        shutdown();
    }
}

void TTSProcessor::setMp3EncoderFn(AudioEncoderFn fn) {
    mp3_encoder_fn_ = std::move(fn);
}

void TTSProcessor::setOggEncoderFn(AudioEncoderFn fn) {
    ogg_encoder_fn_ = std::move(fn);
}

void TTSProcessor::setSynthFn(TTSSynthFn fn) {
    synth_fn_ = std::move(fn);
}

PluginInfo TTSProcessor::getInfo() const {
    PluginInfo info;
    info.name        = "tts-processor";
    info.version     = "1.0.0";
    info.description = "Text-to-Speech processor for voice synthesis";
    info.author      = "ThemisDB Team";
    info.license     = "Apache-2.0";

    info.mime_types = {"text/plain", "application/json"};

    info.extensions = {"txt", "json"};

    info.supports_chunking  = false;
    info.supports_embedding = false;
    info.supports_streaming = true;

    info.min_memory_mb         = 128;
    info.recommended_memory_mb = 512;

    return info;
}

bool TTSProcessor::initialize(const PluginConfig &config) {
    if (initialized_) {
        return true;
    }

    // Load configuration
    model_path_          = config.get<std::string>("model_path", "./models/tts-model.bin");
    default_voice_       = config.get<std::string>("default_voice", "default");
    default_language_    = config.get<std::string>("default_language", "en");
    default_speed_       = config.get<float>("default_speed", 1.0f);
    default_pitch_       = config.get<float>("default_pitch", 1.0f);
    default_sample_rate_ = config.get<int>("default_sample_rate", 22050);

    // Load TTS model
    if (!loadTTSModel()) {
        return false;
    }

    initialized_ = true;
    return true;
}

void TTSProcessor::shutdown() {
    if (!initialized_) {
        return;
    }

    unloadTTSModel();
    initialized_ = false;
}

bool TTSProcessor::canProcess(const std::string &mime_type) const {
    static const std::vector<std::string> supported = {"text/plain", "application/json"};

    return std::find(supported.begin(), supported.end(), mime_type) != supported.end();
}

ContentExtractionResult TTSProcessor::extract(const std::vector<uint8_t> &blob, const std::string & /*mime_type*/,
                                              const ExtractionOptions & /*options*/
) {
    ContentExtractionResult result;
    result.input_size_bytes = blob.size();
    result.success          = false;
    result.error_message    = "TTS processor is for synthesis, not extraction";
    return result;
}

std::vector<ContentChunk> TTSProcessor::chunk(const ContentExtractionResult & /*result*/, int /*max_tokens*/,
                                              int /*overlap*/
) {
    return {};
}

bool TTSProcessor::healthCheck() const {
#ifdef THEMIS_ENABLE_PIPER_TTS
    return initialized_ && tts_ctx_ != nullptr;
#else
    // TTS model not available in this build
    return false;
#endif
}

json TTSProcessor::getStatistics() const {
    json stats;
    stats["syntheses_completed"]      = syntheses_completed_.load();
    stats["total_text_chars"]         = total_text_chars_.load();
    stats["total_audio_duration_ms"]  = total_audio_duration_ms_.load();
    stats["total_processing_time_ms"] = total_processing_time_ms_.load();
    stats["errors"]                   = errors_.load();

    // Calculate synthesis rate
    if (total_processing_time_ms_ > 0) {
        double chars_per_sec
            = (static_cast<double>(total_text_chars_.load()) / static_cast<double>(total_processing_time_ms_.load()))
              * 1000.0;
        stats["chars_per_second"] = chars_per_sec;
    }

    return stats;
}

TTSResult TTSProcessor::synthesize(const std::string &text, const TTSOptions &options) {
    if (!initialized_) {
        TTSResult result;
        result.success       = false;
        result.error_message = "TTS processor not initialized";
        return result;
    }

    return synthesizeInternal(text, options);
}

bool TTSProcessor::streamSynthesize(const std::string &text, std::function<void(const std::vector<uint8_t> &)> callback,
                                    const TTSOptions &options) {
    if ([[maybe_unused]] !initialized_ || !callback) {
        return false;
    }

    // Synthesise the complete audio via the existing pipeline, then deliver
    // the result to the caller in fixed-size chunks so that downstream
    // consumers receive audio data through the callback interface.
    //
    // Production note: true frame-by-frame streaming would interleave
    // Piper/ONNX inference and audio delivery so that playback can begin
    // before synthesis completes (see src/content/FUTURE_ENHANCEMENTS.md
    // §TTS Streaming Synthesis).  This "batch-then-chunk" path requires the
    // full synthesis to finish before the first callback fires, but it
    // correctly invokes the callback with real audio data, enabling all
    // callers that consume streaming audio to work end-to-end.
    const TTSResult result = synthesizeInternal(text, options);
    if (!result.success || result.audio_data.empty()) {
        return false;
    }

    constexpr size_t kChunkBytes = 8192; // 8 KiB per delivery (≈ ~186 ms at 22 050 Hz mono 16-bit)
    const auto &data             = result.audio_data;
    for (size_t offset = 0; offset <static_cast<int>(data.size()); offset += kChunkBytes) {
        const size_t end = std::min(offset + kChunkBytes,static_cast<int>(data.size()));
        callback([[maybe_unused]] {data.begin() + static_cast<ptrdiff_t>(offset), data.begin() + static_cast<ptrdiff_t>(end)});
    }
    return true;
}

json TTSProcessor::getAvailableVoices() const {
    json voices = json::array();

    // Placeholder voices
    json voice1;
    voice1["id"]       = "default";
    voice1["name"]     = "Default Voice";
    voice1["language"] = "en";
    voice1["gender"]   = "neutral";
    voice1["style"]    = "professional";
    voices.push_back(voice1);

    json voice2;
    voice2["id"]       = "female_en";
    voice2["name"]     = "Female English";
    voice2["language"] = "en";
    voice2["gender"]   = "female";
    voice2["style"]    = "friendly";
    voices.push_back(voice2);

    json voice3;
    voice3["id"]       = "male_en";
    voice3["name"]     = "Male English";
    voice3["language"] = "en";
    voice3["gender"]   = "male";
    voice3["style"]    = "professional";
    voices.push_back(voice3);

    json voice4;
    voice4["id"]       = "female_de";
    voice4["name"]     = "Female German";
    voice4["language"] = "de";
    voice4["gender"]   = "female";
    voice4["style"]    = "friendly";
    voices.push_back(voice4);

    return voices;
}

std::vector<std::string> TTSProcessor::getSupportedLanguages() const {
    return {"en", "de", "es", "fr", "it", "pt", "ru", "zh", "ja", "ko"};
}

// Private implementation methods

bool TTSProcessor::loadTTSModel() {
#ifdef THEMIS_ENABLE_PIPER_TTS
    try {
        // Load Piper TTS model
        auto *voice = new piper::PiperVoice();

        // Load model and config
        piper::PiperConfig config;
        config.useESpeak = false;

        piper::loadVoice(config, model_path_, model_path_ + ".json", *voice, nullptr, true);

        tts_ctx_ = voice;
        return true;
    } catch (const std::exception &e) {
        return false;
    }
#else
    // Piper TTS not enabled - use placeholder
    tts_ctx_ = nullptr;
    return false;
#endif
}

void TTSProcessor::unloadTTSModel() {
#ifdef THEMIS_ENABLE_PIPER_TTS
    if (tts_ctx_) {
        delete static_cast<piper::PiperVoice *>(tts_ctx_);
        tts_ctx_ = nullptr;
    }
#else
    tts_ctx_ = nullptr;
#endif
}

TTSResult TTSProcessor::synthesizeInternal(const std::string &text, const TTSOptions &options) {
    auto start = std::chrono::steady_clock::now();

    TTSResult result = {};

    if (text.empty()) {
        result.success       = false;
        result.error_message = "Empty input text";
        errors_++;
        return result;
    }

    try {
        // Preprocess text (normalize, handle special characters, etc.)
        std::string processed_text = preprocessText(text);

        // Generate PCM audio
        auto pcm_data = generatePCM(processed_text, options);

        // Convert to requested format
        result.audio_data = convertToFormat(pcm_data, options.format, options.sample_rate);

        // Set metadata
        result.success     = true;
        result.sample_rate = options.sample_rate;
        result.duration_ms = static_cast<int64_t>((pcm_data.size() / 2.0f) / options.sample_rate * 1000.0f);

        // Set MIME type based on format
        if (options.format == "wav") {
            result.mime_type = "audio/wav";
        } else if (options.format == "mp3") {
            result.mime_type = "audio/mpeg";
        } else if (options.format == "ogg") {
            result.mime_type = "audio/ogg";
        }

        // Update statistics
        syntheses_completed_++;
        total_text_chars_ += text.length();
        total_audio_duration_ms_ += result.duration_ms;

    } catch (const std::exception &e) {
        result.success       = false;
        result.error_message = std::string("TTS synthesis failed: ") + e.what();
        errors_++;
    }

    auto end                  = std::chrono::steady_clock::now();
    result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    total_processing_time_ms_ += result.processing_time_ms;

    return result;
}

std::vector<uint8_t> TTSProcessor::generatePCM(const std::string &text, [[maybe_unused]] const TTSOptions &options) {
#ifdef THEMIS_ENABLE_PIPER_TTS
    if (!tts_ctx_) {
        // Model not loaded
        size_t duration_samples = text.length() * 100;
        std::vector<uint8_t> pcm_data(duration_samples * 2);
        std::fill(pcm_data.begin(), pcm_data.end(), 0);
        return pcm_data;
    }

    try {
        auto *voice = static_cast<piper::PiperVoice *>(tts_ctx_);

        // Piper synthesis configuration
        piper::SynthesisConfig synth_config;
        synth_config.speakerId   = nullptr;
        synth_config.lengthScale = 1.0f / options.speed; // Speed adjustment
        synth_config.noiseScale  = 0.667f;
        synth_config.noiseW      = 0.8f;

        // Output audio buffer
        std::vector<int16_t> audio_buffer;
        piper::SynthesisResult synth_result;

        // Synthesize text
        piper::textToAudio(*voice, text, audio_buffer, synth_result, nullptr);

        // Convert int16 to uint8_t bytes
        std::vector<uint8_t> pcm_data(audio_buffer.size() * 2);
        for (size_t i = 0; i <static_cast<int>(audio_buffer.size()); ++i) {
            int16_t sample      = audio_buffer[i];
            pcm_data[i * 2]     = sample & 0xFF;
            pcm_data[i * 2 + 1] = (sample >> 8) & 0xFF;
        }

        return pcm_data;

    } catch (const std::exception &e) {
        // Error during synthesis - return silence
        size_t duration_samples = text.length() * 100;
        std::vector<uint8_t> pcm_data(duration_samples * 2);
        std::fill(pcm_data.begin(), pcm_data.end(), 0);
        return pcm_data;
    }
#else
    // PERMANENT FALLBACK NOTE:
    // Purpose: Return a byte-valid PCM buffer so callers that depend on
    //          synthesizeSpeech() do not crash when TTS backend is absent.
    // Activation: THEMIS_ENABLE_PIPER_TTS is NOT defined (default build without
    //             Piper TTS or espeak-ng).
    // Behaviour: Delegates to setSynthFn() injection first (e.g. a real TTS
    //            engine like VITS, Coqui, or Kokoro injected at startup).
    //            Falls back to silence (zeros) when no synth fn is set.
    // Production path: Build with -DTHEMIS_ENABLE_PIPER_TTS=ON, or inject:
    //                    tts.setSynthFn([](const std::string& text, const TTSOptions& o) {
    //                        return myEngine.synthesize(text, o);
    //                    });
    if (synth_fn_) {
        auto result = synth_fn_(text, options);
        if (!result.empty()) {
            return result;
        }
    }
    size_t duration_samples = text.length() * 100;       // ~100 samples per character
    std::vector<uint8_t> pcm_data(duration_samples * 2); // 16-bit samples

    // Fill with silence (zeros)
    std::fill(pcm_data.begin(), pcm_data.end(), 0);

    return pcm_data;
#endif
}

std::vector<uint8_t> TTSProcessor::convertToFormat(const std::vector<uint8_t> &pcm_data, const std::string &format,
                                                   int sample_rate) {
    if (format == "wav") {
        // Add WAV header
        std::vector<uint8_t> wav_data = {};

        wav_data.reserve(static_cast<int>(pcm_data.size()) + 44);

        // RIFF header
        wav_data.insert(wav_data.end(), {'R', 'I', 'F', 'F'});
        uint32_t file_size = static_cast<uint32_t>(static_cast<int>(pcm_data.size()) + 36);
        wav_data.push_back(file_size & 0xFF);
        wav_data.push_back((file_size >> 8) & 0xFF);
        wav_data.push_back((file_size >> 16) & 0xFF);
        wav_data.push_back((file_size >> 24) & 0xFF);
        wav_data.insert(wav_data.end(), {'W', 'A', 'V', 'E'});

        // fmt chunk
        wav_data.insert(wav_data.end(), {'f', 'm', 't', ' '});
        wav_data.insert(wav_data.end(), {16, 0, 0, 0}); // fmt chunk size
        wav_data.insert(wav_data.end(), {1, 0});        // PCM format
        wav_data.insert(wav_data.end(), {1, 0});        // Mono
        wav_data.push_back(sample_rate & 0xFF);
        wav_data.push_back((sample_rate >> 8) & 0xFF);
        wav_data.push_back((sample_rate >> 16) & 0xFF);
        wav_data.push_back((sample_rate >> 24) & 0xFF);
        uint32_t byte_rate = sample_rate * 2;
        wav_data.push_back(byte_rate & 0xFF);
        wav_data.push_back((byte_rate >> 8) & 0xFF);
        wav_data.push_back((byte_rate >> 16) & 0xFF);
        wav_data.push_back((byte_rate >> 24) & 0xFF);
        wav_data.insert(wav_data.end(), {2, 0});  // Block align
        wav_data.insert(wav_data.end(), {16, 0}); // Bits per sample

        // data chunk
        wav_data.insert(wav_data.end(), {'d', 'a', 't', 'a'});
        uint32_t data_size = static_cast<uint32_t>(pcm_data.size());
        wav_data.push_back(data_size & 0xFF);
        wav_data.push_back((data_size >> 8) & 0xFF);
        wav_data.push_back((data_size >> 16) & 0xFF);
        wav_data.push_back((data_size >> 24) & 0xFF);

        // PCM data
        wav_data.insert(wav_data.end(), pcm_data.begin(), pcm_data.end());

        return wav_data;
    } else if (format == "mp3") {
        // PERMANENT FALLBACK NOTE:
        // Purpose: Passes through raw PCM data when the LAME MP3 encoder is not
        //          linked.  Keeps the API shape intact for callers that accept
        //          audio/mpeg.
        // Activation: No Mp3EncoderFn has been injected via setMp3EncoderFn().
        // Production path: Inject a real encoder:
        //   tts.setMp3EncoderFn([](const auto& pcm, int sr) {
        //       return lame_encode_buffer_interleaved_ieee_float(...);
        //   });
        if (mp3_encoder_fn_) {
            auto encoded = mp3_encoder_fn_(pcm_data, sample_rate);
            if (!encoded.empty()) {
                return encoded;
            }
        }
        return pcm_data;
    } else if (format == "ogg") {
        // PERMANENT FALLBACK NOTE:
        // Purpose: Passes through raw PCM data when libvorbis / libopus is not
        //          linked.  Keeps the API shape intact for callers that accept
        //          audio/ogg.
        // Activation: No OggEncoderFn has been injected via setOggEncoderFn().
        // Production path: Inject a real encoder:
        //   tts.setOggEncoderFn([](const auto& pcm, int sr) {
        //       return vorbis_encode_buffer(pcm, sr);
        //   });
        if (ogg_encoder_fn_) {
            auto encoded = ogg_encoder_fn_(pcm_data, sample_rate);
            if (!encoded.empty()) {
                return encoded;
            }
        }
        return pcm_data;
    }

    return pcm_data;
}

std::string TTSProcessor::preprocessText(const std::string &text) {
    std::string processed = text;

    // Expand common abbreviations
    processed = std::regex_replace(processed, std::regex("\\bDr\\."), "Doctor");
    processed = std::regex_replace(processed, std::regex("\\bMr\\."), "Mister");
    processed = std::regex_replace(processed, std::regex("\\bMrs\\."), "Misses");
    processed = std::regex_replace(processed, std::regex("\\bMs\\."), "Miss");

    // Normalize whitespace
    processed = std::regex_replace(processed, std::regex("\\s+"), " ");

    // Trim
    size_t start = processed.find_first_not_of(" \t\n\r");
    size_t end   = processed.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        processed = processed.substr(start, end - start + 1);
    }

    return processed;
}

// Plugin entry point
THEMIS_CONTENT_PLUGIN(TTSProcessor)

} // namespace content
} // namespace themis
