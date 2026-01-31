---
name: Video Subtitle Extraction
about: Extract embedded subtitles from video files
title: '[VIDEO] Subtitle Extraction: Extract embedded subtitles (SRT, ASS, WebVTT)'
labels: enhancement, content-processing, video
assignees: ''
---

## Feature Description
Extract embedded subtitle streams from video files and convert them to plain text for indexing and search. Support multiple subtitle formats including SRT, ASS, WebVTT, and DVD/Blu-ray subtitles.

## Problem Statement
Currently, the video processor extracts video/audio metadata but ignores subtitle streams. Subtitles contain valuable textual content that should be:
- Indexed for full-text search
- Used for video content summarization
- Available for accessibility features
- Chunked with timestamps for precise retrieval

The placeholder `extractSubtitles()` function currently returns an empty string.

## Proposed Solution
Implement real subtitle extraction using FFmpeg:

1. **Detect Subtitle Streams**: Identify all subtitle streams in the container
2. **Extract Text Subtitles**: Extract text-based subtitle formats (SRT, ASS, WebVTT)
3. **Convert Bitmap Subtitles**: OCR for DVD/Blu-ray bitmap subtitles (optional, low priority)
4. **Format Conversion**: Convert all subtitle formats to unified plain text with timestamps
5. **Multi-language Support**: Extract all subtitle tracks with language metadata

**Technical Implementation:**
```cpp
// Iterate through streams to find subtitle tracks
for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
    AVStream* stream = fmt_ctx->streams[i];
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
        // Extract subtitle data
        // Convert to text format
        // Add to result with language tag
    }
}
```

## Alternative Solutions
1. **External ffmpeg CLI**: Call `ffmpeg -i input.mp4 -map 0:s:0 output.srt`
   - Pros: Simple, well-tested
   - Cons: Subprocess overhead, temp files

2. **libass Library**: Use libass for ASS/SSA subtitle parsing
   - Pros: Comprehensive ASS support
   - Cons: Additional dependency

3. **Regex Parsing**: Parse SRT files manually
   - Pros: No additional dependencies
   - Cons: Fragile, doesn't handle all formats

## Use Case
**Content Search and Indexing:**
```cpp
// Enable subtitle extraction
PluginConfig config;
config.set("subtitles.extract", true);
config.set("subtitles.include_timestamps", true);

VideoProcessor processor;
processor.initialize(config);

// Process video with subtitles
auto result = processor.extract(video_data, "video/mp4");

// Access subtitle text
std::cout << "Subtitles:\n" << result.text << "\n";

// Metadata includes subtitle info
auto sub_info = result.metadata["subtitles"];
// [{language: "en", format: "srt", default: true}, ...]

// Chunk by subtitle segments for precise search
auto chunks = processor.chunk(result, max_tokens, overlap);
// Each chunk has timestamp metadata for seeking
```

## Example Usage
```cpp
// Configuration
themis::content::PluginConfig config;
config.set("subtitles.extract", true);
config.set("subtitles.include_timestamps", true);
config.set("subtitles.all_languages", true);  // Extract all subtitle tracks

// Initialize processor
themis::content::VideoProcessor processor;
processor.initialize(config);

// Extract with subtitles
auto result = processor.extract(video_blob, "video/mp4");

// Plain text with timestamps
std::cout << result.text << "\n";
// Output:
// [00:00:05.000] Welcome to the presentation
// [00:00:08.500] Today we will discuss...

// Subtitle metadata
json sub_metadata = result.metadata["subtitles"];
for (auto& sub : sub_metadata) {
    std::cout << "Language: " << sub["language"] 
              << " Format: " << sub["format"] << "\n";
}

// Chunk by subtitle segments
auto chunks = processor.chunk(result, 512, 50);
for (auto& chunk : chunks) {
    // Each chunk represents subtitle segment(s)
    // Can be used for vector indexing with timestamps
}
```

## Implementation Considerations
### Technical Details
- **Subtitle Formats**:
  - Text-based: SRT, ASS/SSA, WebVTT, SubRip → Parse directly
  - Bitmap-based: DVD (VOB), Blu-ray (PGS) → Requires OCR (future work)

- **Codec Types**:
  - `CODEC_ID_SUBRIP` (SRT)
  - `CODEC_ID_ASS` (ASS/SSA)
  - `CODEC_ID_WEBVTT` (WebVTT)
  - `CODEC_ID_DVD_SUBTITLE` (VOB - bitmap)
  - `CODEC_ID_HDMV_PGS_SUBTITLE` (PGS - bitmap)

- **Decoding**:
  ```cpp
  AVCodecContext* subtitle_ctx = avcodec_alloc_context3(subtitle_codec);
  AVSubtitle subtitle;
  avcodec_decode_subtitle2(subtitle_ctx, &subtitle, &got_subtitle, &packet);
  ```

### Dependencies
- Existing: FFmpeg (libavformat, libavcodec)
- Optional: Tesseract OCR (for bitmap subtitle OCR - future enhancement)

### Configuration Options
```cpp
config.set("subtitles.extract", bool);              // Enable/disable extraction
config.set("subtitles.include_timestamps", bool);   // Include [HH:MM:SS.mmm] timestamps
config.set("subtitles.all_languages", bool);        // Extract all tracks or just default
config.set("subtitles.prefer_language", string);    // Preferred language code (e.g., "en")
config.set("subtitles.format", string);             // Output format: "plain" | "srt" | "json"
```

### Performance
- **Processing Time**: Near-instant (subtitles are pre-encoded text)
- **Memory**: Minimal (~1-5MB for typical subtitle file)
- **No Impact**: Subtitle extraction doesn't require video decoding

## Related Features
- **Video Processor with FFmpeg** (✅ Implemented in v1.3.0+)
- **Content Chunking** (✅ Implemented - can chunk by subtitle segments)
- **Full-text Search** (Subtitles enable video content search)
- **Multi-language Support** (Subtitle language metadata)

## Additional Context
### References
- [FFmpeg Subtitle Codecs](https://ffmpeg.org/doxygen/trunk/group__lavc__codec.html)
- [SubRip Format Specification](https://wiki.videolan.org/SubRip/)
- [WebVTT Specification](https://www.w3.org/TR/webvtt1/)
- [ASS/SSA Format](https://en.wikipedia.org/wiki/SubStation_Alpha)

### Current Implementation
Placeholder in `src/content/video_processor.cpp`:
```cpp
std::string VideoProcessor::extractSubtitles(const std::vector<uint8_t>& blob) {
    // Real implementation would:
    // 1. Check for subtitle streams in container
    // 2. Extract subtitle track(s)
    // 3. Convert to plain text
    
    return "";
}
```

### Example Output Formats
**Plain Text:**
```
Welcome to the presentation
Today we will discuss advanced database features
Let's start with vector indexing
```

**With Timestamps:**
```
[00:00:05.000] Welcome to the presentation
[00:00:08.500] Today we will discuss advanced database features
[00:00:12.000] Let's start with vector indexing
```

**JSON Format:**
```json
{
  "subtitles": [
    {
      "start_ms": 5000,
      "end_ms": 8500,
      "text": "Welcome to the presentation"
    },
    {
      "start_ms": 8500,
      "end_ms": 12000,
      "text": "Today we will discuss advanced database features"
    }
  ]
}
```

---

**Checklist:**
- [x] I have searched existing issues to ensure this is not a duplicate
- [x] I have clearly described the problem this feature solves
- [x] I have provided a detailed description of the proposed solution
- [x] I have considered the impact on existing functionality
