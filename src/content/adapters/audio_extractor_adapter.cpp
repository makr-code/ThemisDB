/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audio_extractor_adapter.cpp                        ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-16                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "content/adapters/format_extractor_adapters.h"

#ifdef THEMIS_ENABLE_VOICE_ASSISTANT
#include "content/stt_processor.h"
#include "content/content_plugin_interface.h"

namespace themis {
namespace content {
namespace adapters {

namespace {

class AudioExtractorAdapter : public ingestion::IFormatExtractor {
public:
    AudioExtractorAdapter() {
        PluginConfig cfg;
        processor_.initialize(cfg);
    }

    ingestion::FormatExtractResult extract(
        std::span<const std::byte> data,
        const std::string& mime_type,
        const std::string& /*filename_hint*/) override
    {
        ingestion::FormatExtractResult out;
        try {
            std::vector<uint8_t> blob(
                reinterpret_cast<const uint8_t*>(data.data()),
                reinterpret_cast<const uint8_t*>(data.data()) + data.size());

            ExtractionOptions opts;
            auto result = processor_.extract(blob, mime_type, opts);
            if (!result.success) {
                out.error = result.error_message;
                return out;
            }

            out.raw_text      = std::move(result.text);
            out.metadata      = std::move(result.metadata);
            out.detected_lang = result.metadata.value("language", "");
            out.ok = true;
        } catch (const std::exception& ex) {
            out.error = std::string("AudioExtractorAdapter: exception: ") + ex.what();
        }
        return out;
    }

    std::vector<std::string> supportedMimeTypes() const override {
        return {
            "audio/mpeg",
            "audio/mp3",
            "audio/wav",
            "audio/x-wav",
            "audio/ogg",
            "audio/flac",
            "audio/mp4",
            "audio/aac",
            "audio/opus",
            "audio/webm",
        };
    }

    const char* name() const noexcept override {
        return "AudioExtractorAdapter";
    }

private:
    STTProcessor processor_;
};

} // anonymous namespace

std::shared_ptr<ingestion::IFormatExtractor> createAudioExtractorAdapter() {
    return std::make_shared<AudioExtractorAdapter>();
}

} // namespace adapters
} // namespace content
} // namespace themis

#else // !THEMIS_ENABLE_VOICE_ASSISTANT

namespace themis { namespace content { namespace adapters {
std::shared_ptr<ingestion::IFormatExtractor> createAudioExtractorAdapter() {
    return nullptr;
}
} } }

#endif // THEMIS_ENABLE_VOICE_ASSISTANT
