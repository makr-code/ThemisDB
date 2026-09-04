/**
 * @file video_processor.cpp
 * @brief Content processor module for video processor operations.
 * @version 0.0.47
 * @note Maturity: 🟡 BETA
 * @note Score: 79/100
 * @note Gap Summary: total=9; TODO=1, Stub=0, Unimpl=1, Mock=0, Sim=0, Debt=2, C=1, H=2, M=5, L=0
 * @note Status: Production Ready; FFmpeg frame extraction working; real-time transcoding deferred
 * @note This block is auto-generated and will be overwritten.
 */
// Ensure plugin entry points export correctly when built into core
#define THEMIS_PLUGIN_EXPORTS

#include "content/video_processor.h"
#include <exception>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <sstream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <stdexcept>

#ifdef THEMIS_HAS_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#endif

namespace themis {
namespace content {

namespace {

bool isValidThumbnailBufferLayout(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    constexpr auto kRgbChannels = size_t{3};
    const auto safe_width = static_cast<size_t>(width);
    const auto safe_height = static_cast<size_t>(height);

    if (safe_width > static_cast<size_t>(std::numeric_limits<int>::max()) / kRgbChannels) {
        return false;
    }

    const auto row_size = safe_width * kRgbChannels;
    return safe_height <= std::numeric_limits<size_t>::max() / row_size;
}

} // namespace

VideoProcessor::VideoProcessor() = default;

VideoProcessor::~VideoProcessor() {
    if (initialized_) {
        shutdown();
    }
}

PluginInfo VideoProcessor::getInfo() const {
    PluginInfo info;
    info.name        = "video-processor";
    info.version     = "1.0.0";
    info.description = "Video content processor using FFmpeg";
    info.author      = "ThemisDB Team";
    info.license     = "Apache-2.0";

    info.mime_types = {"video/mp4",       "video/webm",  "video/x-matroska", "video/quicktime",
                       "video/x-msvideo", "video/x-flv", "video/mpeg",       "video/ogg"};

    info.extensions = {"mp4", "m4v", "webm", "mkv", "mov", "avi", "flv", "mpeg", "mpg", "ogv"};

    info.supports_chunking  = true;
    info.supports_embedding = false;
    info.supports_streaming = true;

    info.min_memory_mb         = 128;
    info.recommended_memory_mb = 512;

    return info;
}

bool VideoProcessor::initialize(const PluginConfig &config) {
    if (initialized_) {
        return true;
    }

    // Load configuration
    max_thumbnail_width_       = config.get<int>("thumbnail.max_width", 320);
    max_thumbnail_height_      = config.get<int>("thumbnail.max_height", 240);
    max_keyframes_             = config.get<int>("keyframes.max_count", 10);
    extract_subtitles_         = config.get<bool>("subtitles.extract", true);
    enable_scene_detection_    = config.get<bool>("scene_detection.enabled", false);
    scene_detection_threshold_ = config.get<double>("scene_detection.threshold", 0.4);

    if (!isValidThumbnailBufferLayout(max_thumbnail_width_, max_thumbnail_height_)) {
        return false;
    }
    
#ifdef THEMIS_HAS_FFMPEG
// Initialize FFmpeg library (only needed for older versions)
// Modern FFmpeg doesn't require explicit initialization
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif
    avformat_network_init();
#endif

    initialized_ = true;
    return true;
}

void VideoProcessor::shutdown() {
    if (!initialized_) {
        return;
    }

#ifdef THEMIS_HAS_FFMPEG
    // Clean up FFmpeg network resources
    avformat_network_deinit();
#endif

    initialized_ = false;
}

bool VideoProcessor::canProcess(const std::string &mime_type) const {
    static const std::vector<std::string> supported
        = {"video/mp4",       "video/webm",  "video/x-matroska", "video/quicktime",
           "video/x-msvideo", "video/x-flv", "video/mpeg",       "video/ogg"};

    return std::find(supported.begin(), supported.end(), mime_type) != supported.end();
}

ContentExtractionResult VideoProcessor::extract(const std::vector<uint8_t> &blob, const std::string &mime_type,
                                                const ExtractionOptions &options) {
    auto start = std::chrono::steady_clock::now();
    ContentExtractionResult result;
    result.input_size_bytes = blob.size();

    if (!initialized_) {
        result.success       = false;
        result.error_message = "Video processor not initialized";
        errors_++;
        return result;
    }

    if (blob.empty()) {
        result.success       = false;
        result.error_message = "Empty input blob";
        errors_++;
        return result;
    }

    // Minimum size check: any valid container needs at least 8 bytes for a box header
    if (static_cast<int>(blob.size()) < 8) {
        result.success       = false;
        result.error_message = "Input blob too small to be a valid video file";
        errors_++;
        return result;
    }

    // Validate MIME type
    if (!canProcess(mime_type)) {
        result.success       = false;
        result.error_message = "Unsupported MIME type: " + mime_type;
        errors_++;
        return result;
    }

    try {
        // Extract metadata using FFmpeg
        // This is a simulation - real implementation would use libavformat
        MediaExtractionData media = extractMetadata(blob);

        // Build metadata JSON
        json metadata;
        metadata["duration_ms"]      = media.duration_ms;
        metadata["width"]            = media.width;
        metadata["height"]           = media.height;
        metadata["video_codec"]      = media.video_codec;
        metadata["audio_codec"]      = media.audio_codec;
        metadata["bitrate_kbps"]     = media.bitrate_kbps;
        metadata["framerate"]        = media.framerate;
        metadata["container_format"] = media.container_format;

        // Calculate aspect ratio
        if (media.height > 0) {
            metadata["aspect_ratio"] = static_cast<double>(media.width) / media.height;
        }

        // Resolution classification
        if (media.height >= 2160) {
            metadata["resolution_class"] = "4K";
        } else if (media.height >= 1080) {
            metadata["resolution_class"] = "1080p";
        } else if (media.height >= 720) {
            metadata["resolution_class"] = "720p";
        } else if (media.height >= 480) {
            metadata["resolution_class"] = "480p";
        } else {
            metadata["resolution_class"] = "SD";
        }

        result.metadata = metadata;

        // Generate thumbnail if requested
        if (options.generate_thumbnail) {
            result.thumbnail           = generateThumbnail(blob);
            result.thumbnail_mime_type = "image/jpeg";
        }

        // Extract keyframe timestamps if requested
        if (options.extract_keyframes) {
            auto keyframes            = extractKeyframes(blob);
            media.keyframe_timestamps = keyframes;
            json kf_times             = json::array();
            for (const auto &time : keyframes) {
                kf_times.push_back(time);
            }
            result.metadata["keyframe_timestamps_ms"] = kf_times;
        }

        // Extract subtitles if requested (via options or plugin config)
        if (options.extract_subtitles || extract_subtitles_) {
            std::string subtitles = extractSubtitles(blob);
            if (!subtitles.empty()) {
                result.text                      = subtitles;
                result.metadata["has_subtitles"] = true;
                media.subtitles                  = subtitles;
            }
        }

        // Scene detection if requested
        if (options.extract_scenes || enable_scene_detection_) {
            auto scenes            = detectScenes(blob);
            media.scene_boundaries = scenes;
            json scene_times       = json::array();
            for (auto time : scenes) {
                scene_times.push_back(time);
            }
            result.metadata["scene_changes_ms"] = scene_times;
        }

        result.media   = media;
        result.success = true;
        videos_processed_++;
        total_duration_ms_ += media.duration_ms;

    } catch (const std::exception &e) {
        result.success       = false;
        result.error_message = std::string("Video processing failed: ") + e.what();
        errors_++;
    }

    auto end                  = std::chrono::steady_clock::now();
    result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return result;
}

std::vector<ContentChunk> VideoProcessor::chunk(const ContentExtractionResult &result, int max_tokens, int /*overlap*/
) {
    std::vector<ContentChunk> chunks;

    if (!result.success || result.text.empty()) {
        return chunks;
    }

    // For videos, we chunk by subtitle segments (if available)
    // Each subtitle block becomes a chunk with timestamp metadata

    std::istringstream stream(result.text);
    std::string line = {};
    std::string current_text = {};
    int sequence = 0;

    while (std::getline(stream, line)) {
        if (line.empty()) {
            if (!current_text.empty()) {
                ContentChunk chunk;
                chunk.text        = current_text;
                chunk.sequence    = sequence++;
                chunk.token_count = countTokens(current_text);
                chunks.push_back(chunk);
                current_text.clear();
            }
        } else {
            if (!current_text.empty()) {
                current_text += " ";
            }
            current_text += line;
        }

        // Check token limit
        if (countTokens(current_text) >= max_tokens) {
            ContentChunk chunk;
            chunk.text        = current_text;
            chunk.sequence    = sequence++;
            chunk.token_count = countTokens(current_text);
            chunks.push_back(chunk);
            current_text.clear();
        }
    }

    // Add remaining text
    if (!current_text.empty()) {
        ContentChunk chunk;
        chunk.text        = current_text;
        chunk.sequence    = sequence++;
        chunk.token_count = countTokens(current_text);
        chunks.push_back(chunk);
    }

    return chunks;
}

bool VideoProcessor::healthCheck() const {
#ifdef THEMIS_HAS_FFMPEG
    // Check if FFmpeg libraries are properly loaded
    return initialized_;
#else
    // Without FFmpeg the processor is initialised but cannot do real work;
    // report unhealthy so health-check aggregators surface the missing dependency.
    return false;
#endif
}

json VideoProcessor::getStatistics() const {
    json stats;
    stats["videos_processed"]    = videos_processed_.load();
    stats["total_duration_ms"]   = total_duration_ms_.load();
    stats["errors"]              = errors_.load();
    stats["average_duration_ms"] = videos_processed_ > 0 ? total_duration_ms_.load() / videos_processed_.load() : 0;
    return stats;
}

// Private implementation methods

MediaExtractionData VideoProcessor::extractMetadata(const std::vector<uint8_t> &blob) {
#ifdef THEMIS_HAS_FFMPEG
    return extractMetadataFFmpeg(blob);
#else
    // PERMANENT FALLBACK NOTE:
    // Purpose: Return a plausible MediaExtractionData structure when compiled
    //          without FFmpeg (THEMIS_HAS_FFMPEG not defined).  Allows the
    //          content pipeline to exercise the video-processing code path in
    //          unit-test and development environments that lack FFmpeg.
    // Activation: THEMIS_HAS_FFMPEG is NOT defined at compile time.
    // Production path: Build with -DTHEMIS_HAS_FFMPEG=ON and link
    //                  libavformat/libavcodec; the real extractMetadataFFmpeg()
    //                  path above this #else is then used.
    //                  See src/content/ROADMAP.md § "Long-term: Video frame extraction"
    //                  and src/content/FUTURE_ENHANCEMENTS.md § "Video Processing".
    MediaExtractionData data;

    // Analyze blob header to detect format
    if (static_cast<int>(blob.size()) >= 12) {
        // MP4/MOV detection (ftyp box)
        if (blob[4] == 'f' && blob[5] == 't' && blob[6] == 'y' && blob[7] == 'p') {
            data.container_format = "mp4";
            data.video_codec      = "h264";
            data.audio_codec      = "aac";
        }
        // WebM detection (EBML magic bytes)
        else if (blob[0] == 0x1A && blob[1] == 0x45 && blob[2] == 0xDF && blob[3] == 0xA3) {
            data.container_format = "webm";
            data.video_codec      = "vp9";
            data.audio_codec      = "opus";
        }
    }
    
    // Placeholder metadata (static values — not decoded from container)
    data.duration_ms = 120000;  // 2 minutes (placeholder)
    data.width = 1920;
    data.height = 1080;
    data.bitrate_kbps = 5000;
    data.framerate    = 30.0;
    data.sample_rate  = 48000;
    data.channels     = 2;

    return data;
#endif
}

std::vector<uint8_t> VideoProcessor::generateThumbnail(const std::vector<uint8_t> &blob) {
#ifdef THEMIS_HAS_FFMPEG
    return generateThumbnailFFmpeg(blob);
#else
    // Return empty thumbnail placeholder in simulation mode
    return std::vector<uint8_t>();
#endif
}

std::string VideoProcessor::extractSubtitles(const std::vector<uint8_t> & /*blob*/) {
    // Real implementation would:
    // 1. Check for subtitle streams in container
    // 2. Extract subtitle track(s)
    // 3. Convert to plain text

    return "";
}

std::vector<int64_t> VideoProcessor::detectScenes(const std::vector<uint8_t> &blob) {
#ifdef THEMIS_HAS_FFMPEG
    return detectScenesFFmpeg(blob);
#else
    // Without FFmpeg, video frames cannot be decoded for histogram analysis.
    // Scene detection requires per-frame access, so return empty in simulation mode.
    return {};
#endif
}

std::vector<int64_t> VideoProcessor::extractKeyframes(const std::vector<uint8_t> &blob) {
#ifdef THEMIS_HAS_FFMPEG
    return extractKeyframesFFmpeg(blob);
#else
    // Without FFmpeg, synthesise evenly-distributed keyframe timestamps based
    // on the simulated video duration (120 s at 30 fps, I-frame every 2 s).
    const int64_t duration_ms = 120000;
    std::vector<int64_t> keyframes = {};

    if (max_keyframes_ <= 0) {
        return keyframes;
    }
    // Divide the duration into (max_keyframes_ + 1) equal intervals so that
    // keyframes are evenly distributed with a margin on both ends.  This mirrors
    // how a real I-frame sequence typically starts a few frames in and ends
    // before the very last frame of the stream.
    const int64_t interval_ms = duration_ms / (max_keyframes_ + 1);
    for (int i = 1; i <= max_keyframes_; i++) {
        keyframes.push_back(i * interval_ms);
    }
    return keyframes;
#endif
}

#ifdef THEMIS_HAS_FFMPEG
/**
 * @brief Extract video metadata using FFmpeg libraries
 *
 * This function uses libavformat and libavcodec to extract real metadata from video files.
 * It opens the video file, retrieves stream information, and extracts:
 * - Container format and duration
 * - Video stream: width, height, codec, framerate
 * - Audio stream: codec, sample rate, channels
 *
 * @param blob Raw video file data
 * @return MediaExtractionData containing all extracted metadata
 * @throws std::runtime_error if video cannot be opened or processed
 *
 * @note This function creates a temporary file for FFmpeg processing.
 *       The temporary file is automatically cleaned up on success or error.
 */
MediaExtractionData VideoProcessor::extractMetadataFFmpeg(const std::vector<uint8_t> &blob) {
    MediaExtractionData data;

    // Create unique temporary file path to avoid race conditions
    auto temp_dir         = std::filesystem::temp_directory_path();
    std::string unique_id = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_"
                            + std::to_string(reinterpret_cast<uintptr_t>(this));
    std::string temp_path = (temp_dir / ("themis_video_" + unique_id)).string();

    try {
        std::ofstream temp_file(temp_path, std::ios::binary | std::ios::trunc);
        if (!temp_file) {
            throw std::runtime_error("Failed to create temporary file");
        }
        temp_file.write(reinterpret_cast<const char *>(blob.data()),static_cast<int>(blob.size()));
        temp_file.close();

        // Open video file
        AVFormatContext *fmt_ctx = nullptr;
        if (avformat_open_input(&fmt_ctx, temp_path.c_str(), nullptr, nullptr) < 0) {
            std::filesystem::remove(temp_path);
            throw std::runtime_error("Failed to open video file");
        }

        // Retrieve stream information
        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            throw std::runtime_error("Failed to find stream info");
        }

        // Extract container format
        if (fmt_ctx->iformat) {
            data.container_format = fmt_ctx->iformat->name;
        }

        // Extract duration (in microseconds -> milliseconds)
        if (fmt_ctx->duration != AV_NOPTS_VALUE) {
            data.duration_ms = fmt_ctx->duration / 1000;
        }

        // Extract bitrate
        if (fmt_ctx->bit_rate > 0) {
            data.bitrate_kbps = fmt_ctx->bit_rate / 1000;
        }

        // Find video and audio streams
        for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
            AVStream *stream            = fmt_ctx->streams[i];
            AVCodecParameters *codecpar = stream->codecpar;

            if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && data.width == 0) {
                // Video stream
                data.width  = codecpar->width;
                data.height = codecpar->height;

                // Get codec name
                const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
                if (codec) {
                    data.video_codec = codec->name;
                }

                // Calculate framerate
                if (stream->avg_frame_rate.den > 0) {
                    data.framerate = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
                }
            } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && data.audio_codec.empty()) {
                // Audio stream
                data.sample_rate = codecpar->sample_rate;

// Get number of channels (handle both old and new FFmpeg API)
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 24, 100)
                // FFmpeg 5.1+ uses ch_layout
                data.channels = codecpar->ch_layout.nb_channels;
#else
                // Older FFmpeg uses channels field
                data.channels = codecpar->channels;
#endif

                // Get codec name
                const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
                if (codec) {
                    data.audio_codec = codec->name;
                }
            }
        }

        // Cleanup
        avformat_close_input(&fmt_ctx);
        std::filesystem::remove(temp_path);
        
    } catch (...) {
        // Ensure temp file is cleaned up
        if (std::filesystem::exists(temp_path)) {
            std::filesystem::remove(temp_path);
        }
        throw;
    }

    return data;
}

/**
 * @brief Generate video thumbnail using FFmpeg libraries
 *
 * This function uses libavformat, libavcodec, and libswscale to generate a thumbnail:
 * 1. Opens the video file with FFmpeg
 * 2. Seeks to 10% of the video duration (or first keyframe)
 * 3. Decodes a frame using the appropriate video codec
 * 4. Scales the frame to the configured thumbnail size (maintains aspect ratio)
 * 5. Converts color space from YUV to RGB24
 * 6. Returns raw RGB data (can be encoded to JPEG/PNG later)
 *
 * @param blob Raw video file data
 * @return std::vector<uint8_t> containing raw RGB24 thumbnail data (width*height*3 bytes)
 * @throws std::runtime_error if video cannot be opened, decoded, or scaled
 *
 * @note The returned data is in RGB24 format with no padding.
 *       Each pixel is 3 bytes (R, G, B) in row-major order.
 * @note This function creates a temporary file for FFmpeg processing.
 *       The temporary file is automatically cleaned up on success or error.
 */
std::vector<uint8_t> VideoProcessor::generateThumbnailFFmpeg(const std::vector<uint8_t> &blob) {
    std::vector<uint8_t> thumbnail;

    // Create unique temporary file path to avoid race conditions
    auto temp_dir         = std::filesystem::temp_directory_path();
    std::string unique_id = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_"
                            + std::to_string(reinterpret_cast<uintptr_t>(this));
    std::string temp_path = (temp_dir / ("themis_thumb_" + unique_id)).string();

    try {
        std::ofstream temp_file(temp_path, std::ios::binary | std::ios::trunc);
        if (!temp_file) {
            throw std::runtime_error("Failed to create temporary file");
        }
        temp_file.write(reinterpret_cast<const char *>(blob.data()),static_cast<int>(blob.size()));
        temp_file.close();

        // Open video file
        AVFormatContext *fmt_ctx = nullptr;
        if (avformat_open_input(&fmt_ctx, temp_path.c_str(), nullptr, nullptr) < 0) {
            std::filesystem::remove(temp_path);
            throw std::runtime_error("Failed to open video file");
        }

        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            throw std::runtime_error("Failed to find stream info");
        }

        // Find video stream
        int video_stream_index = -1;
        for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index = i;
                break;
            }
        }

        if (video_stream_index < 0) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            throw std::runtime_error("No video stream found");
        }

        AVStream *video_stream      = fmt_ctx->streams[video_stream_index];
        AVCodecParameters *codecpar = video_stream->codecpar;

        // Find decoder
        const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
        if (!codec) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            throw std::runtime_error("Decoder not found");
        }

        // Allocate codec context
        AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            throw std::runtime_error("Failed to allocate codec context");
        }

        if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            throw std::runtime_error("Failed to copy codec parameters");
        }

        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            throw std::runtime_error("Failed to open codec");
        }

        // Seek to 10% of video duration
        int64_t seek_target = fmt_ctx->duration / 10;
        av_seek_frame(fmt_ctx, -1, seek_target, AVSEEK_FLAG_BACKWARD);

        // Read and decode frames until we get one
        AVPacket *packet = av_packet_alloc();
        AVFrame *frame   = av_frame_alloc();
        bool got_frame   = false;

        while (av_read_frame(fmt_ctx, packet) >= 0) {
            if (packet->stream_index == video_stream_index) {
                if (avcodec_send_packet(codec_ctx, packet) >= 0) {
                    if (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                        got_frame = true;
                        av_packet_unref(packet);
                        break;
                    }
                }
            }
            av_packet_unref(packet);
        }

        if (got_frame) {
            // Scale frame to thumbnail size
            int thumb_width  = max_thumbnail_width_;
            int thumb_height = max_thumbnail_height_;

            // Maintain aspect ratio
            double aspect = static_cast<double>(frame->width) / frame->height;
            if (frame->width > frame->height) {
                thumb_height = std::max(1, static_cast<int>(thumb_width / aspect));
            } else {
                thumb_width = std::max(1, static_cast<int>(thumb_height * aspect));
            }

            if (!isValidThumbnailBufferLayout(thumb_width, thumb_height)) {
                throw std::runtime_error("Thumbnail dimensions exceed RGB buffer limits");
            }

            // Create scaling context
            SwsContext *sws_ctx
                = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format), thumb_width,
                                 thumb_height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);

            if (sws_ctx) {
                // Allocate RGB frame
                AVFrame *rgb_frame = av_frame_alloc();
                rgb_frame->format  = AV_PIX_FMT_RGB24;
                rgb_frame->width   = thumb_width;
                rgb_frame->height  = thumb_height;
                av_frame_get_buffer(rgb_frame, 0);

                // Convert to RGB
                sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, rgb_frame->data,
                          rgb_frame->linesize);

                // Copy RGB data - optimize for case without padding
                constexpr auto kRgbChannels = size_t{3};
                const auto safe_width = static_cast<size_t>(thumb_width);
                const auto safe_height = static_cast<size_t>(thumb_height);
                const auto row_size = safe_width * kRgbChannels;
                const auto thumbnail_size = row_size * safe_height;

                thumbnail.resize(thumbnail_size);
                uint8_t* dst = thumbnail.data();
                const uint8_t* src = rgb_frame->data[0];

                if (rgb_frame->linesize[0] <= 0) {
                    throw std::runtime_error("Invalid RGB frame line size");
                }
                
                if (rgb_frame->linesize[0] == static_cast<int>(row_size)) {
                    // No padding - single fast copy
                    memcpy(dst, src,static_cast<int>(thumbnail.size()));
                } else {
                    // Handle padding - copy row by row
                    for (int y = 0; y < thumb_height; y++) {
                        const auto row_index = static_cast<size_t>(y);
                        memcpy(dst + row_index * row_size,
                               src + row_index * static_cast<size_t>(rgb_frame->linesize[0]),
                               row_size);
                    }
                }

                av_frame_free(&rgb_frame);
                sws_freeContext(sws_ctx);
            }
        }

        // Cleanup
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        std::filesystem::remove(temp_path);
        
    } catch (...) {
        // Ensure temp file is cleaned up
        if (std::filesystem::exists(temp_path)) {
            std::filesystem::remove(temp_path);
        }
        throw;
    }

    return thumbnail;
}

/**
 * @brief Extract keyframe timestamps using FFmpeg libraries
 *
 * Scans video packets for I-frames (keyframes) and collects their timestamps.
 * Results are limited to max_keyframes_ entries and returned in ascending
 * millisecond order.
 *
 * @param blob Raw video file data
 * @return Keyframe timestamps in milliseconds, up to max_keyframes_ entries
 */
std::vector<int64_t> VideoProcessor::extractKeyframesFFmpeg(const std::vector<uint8_t> &blob) {
    std::vector<int64_t> keyframes;

    auto temp_dir         = std::filesystem::temp_directory_path();
    std::string unique_id = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_kf_"
                            + std::to_string(reinterpret_cast<uintptr_t>(this));
    std::string temp_path = (temp_dir / ("themis_kf_" + unique_id)).string();

    try {
        std::ofstream temp_file(temp_path, std::ios::binary | std::ios::trunc);
        if (!temp_file) {
            return keyframes;
        }
        temp_file.write(reinterpret_cast<const char *>(blob.data()),static_cast<int>(blob.size()));
        temp_file.close();

        AVFormatContext *fmt_ctx = nullptr;
        if (avformat_open_input(&fmt_ctx, temp_path.c_str(), nullptr, nullptr) < 0) {
            std::filesystem::remove(temp_path);
            return keyframes;
        }

        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            return keyframes;
        }

        // Locate the primary video stream
        int video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (video_stream_index < 0) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            return keyframes;
        }

        AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;

        // Iterate packets; collect those flagged as keyframes (I-frames)
        AVPacket *packet = av_packet_alloc();
        while (av_read_frame(fmt_ctx, packet) >= 0) {
            if (packet->stream_index == video_stream_index && (packet->flags & AV_PKT_FLAG_KEY)
                && packet->pts != AV_NOPTS_VALUE) {
                int64_t pts_ms = av_rescale_q(packet->pts, time_base, {1, 1000});
                keyframes.push_back(pts_ms);
                if (max_keyframes_ > 0 && keyframes.size() >= static_cast<size_t>(max_keyframes_)) {
                    av_packet_unref(packet);
                    break;
                }
            }
            av_packet_unref(packet);
        }
        av_packet_free(&packet);
        avformat_close_input(&fmt_ctx);
        std::filesystem::remove(temp_path);
    } catch (...) {
        if (std::filesystem::exists(temp_path)) {
            std::filesystem::remove(temp_path);
        }
    }

    return keyframes;
}

/**
 * @brief Detect scene boundaries using FFmpeg frame histogram comparison
 *
 * Decodes video frames and computes per-frame luma histogram differences.
 * When the L1 distance between consecutive normalised histograms exceeds
 * scene_detection_threshold_, the current frame's timestamp is recorded as
 * a scene boundary.
 *
 * @param blob Raw video file data
 * @return Scene boundary timestamps in milliseconds
 */
std::vector<int64_t> VideoProcessor::detectScenesFFmpeg(const std::vector<uint8_t> &blob) {
    std::vector<int64_t> scenes;

    auto temp_dir         = std::filesystem::temp_directory_path();
    std::string unique_id = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_sc_"
                            + std::to_string(reinterpret_cast<uintptr_t>(this));
    std::string temp_path = (temp_dir / ("themis_sc_" + unique_id)).string();

    try {
        std::ofstream temp_file(temp_path, std::ios::binary | std::ios::trunc);
        if (!temp_file) {
            return scenes;
        }
        temp_file.write(reinterpret_cast<const char *>(blob.data()),static_cast<int>(blob.size()));
        temp_file.close();

        AVFormatContext *fmt_ctx = nullptr;
        if (avformat_open_input(&fmt_ctx, temp_path.c_str(), nullptr, nullptr) < 0) {
            std::filesystem::remove(temp_path);
            return scenes;
        }

        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            return scenes;
        }

        int video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (video_stream_index < 0) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            return scenes;
        }

        AVStream *video_stream      = fmt_ctx->streams[video_stream_index];
        AVCodecParameters *codecpar = video_stream->codecpar;

        const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
        if (!codec) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            return scenes;
        }

        AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            return scenes;
        }

        if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0 || avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&fmt_ctx);
            std::filesystem::remove(temp_path);
            return scenes;
        }

        AVPacket *packet    = av_packet_alloc();
        AVFrame *frame      = av_frame_alloc();
        AVFrame *prev_frame = av_frame_alloc();
        bool has_prev       = false;

        // Lambda: compute normalised L1 luma histogram difference in [0, 1]
        // Outer loop on y (rows) then inner on x (columns) matches the row-major
        // memory layout of AVFrame (data[0] + y * linesize[0]), ensuring sequential
        // access and good CPU cache utilisation. linesize[] may include padding bytes
        // beyond 'width', which we deliberately exclude by iterating only up to w.
        auto histDiff = [](const AVFrame *a, const AVFrame *b) -> double {
            constexpr int BINS = 256;
            std::array<int, BINS> ha{}, hb{};
            int w     = std::min(a->width, b->width);
            int h     = std::min(a->height, b->height);
            int total = w * h;
            if (total == 0)
                return 0.0;
            for (int y = 0; y < h; ++y) {
                const uint8_t *ra = a->data[0] + static_cast<ptrdiff_t>(y) * a->linesize[0];
                const uint8_t *rb = b->data[0] + static_cast<ptrdiff_t>(y) * b->linesize[0];
                for (int x = 0; x < w; ++x) {
                    ha[ra[x]]++;
                    hb[rb[x]]++;
                }
            }
            double diff = 0.0;
            for (int i = 0; i < BINS; ++i) {
                diff += std::abs(static_cast<double>(ha[i]) - static_cast<double>(hb[i]));
            }
            return diff / (2.0 * total);
        };

        auto processFrame = [&]() {
            if (has_prev) {
                double diff = histDiff(prev_frame, frame);
                if (diff > scene_detection_threshold_ && frame->pts != AV_NOPTS_VALUE) {
                    int64_t pts_ms = av_rescale_q(frame->pts, video_stream->time_base, {1, 1000});
                    scenes.push_back(pts_ms);
                }
            }
            std::swap(frame, prev_frame);
            has_prev = true;
        };

        while (av_read_frame(fmt_ctx, packet) >= 0) {
            if (packet->stream_index == video_stream_index) {
                if (avcodec_send_packet(codec_ctx, packet) >= 0) {
                    while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                        processFrame();
                    }
                }
            }
            av_packet_unref(packet);
        }

        // Flush decoder
        avcodec_send_packet(codec_ctx, nullptr);
        while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
            processFrame();
        }

        av_frame_free(&frame);
        av_frame_free(&prev_frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        std::filesystem::remove(temp_path);
    } catch (...) {
        if (std::filesystem::exists(temp_path)) {
            std::filesystem::remove(temp_path);
        }
    }

    return scenes;
}
#endif

// Plugin entry point
THEMIS_CONTENT_PLUGIN(VideoProcessor)

} // namespace content
} // namespace themis

