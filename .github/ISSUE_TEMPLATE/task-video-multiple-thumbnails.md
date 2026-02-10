---
name: Video Multiple Thumbnails
about: Generate thumbnails at multiple timestamps in a video
title: '[VIDEO] Multiple Thumbnails: Generate thumbnails at various timestamps'
labels: enhancement, content-processing, video
assignees: ''
---

## Feature Description
Extend the video processor to generate multiple thumbnails at different timestamps, instead of just a single thumbnail at 10% of video duration. This allows for better video preview, storyboard generation, and keyframe extraction.

## Problem Statement
Currently, the video processor generates only one thumbnail at 10% of the video duration. This limitation prevents:
- Creating video storyboards (grid of thumbnails)
- Extracting thumbnails at specific user-defined timestamps
- Generating keyframe-based thumbnail sequences
- Providing multiple preview images for video players

## Proposed Solution
Extend `VideoProcessor` to support multiple thumbnail generation:

1. **Timestamp-based Extraction**: Generate thumbnails at specific timestamps
2. **Interval-based Extraction**: Generate thumbnails at regular intervals (e.g., every N seconds)
3. **Keyframe Extraction**: Extract thumbnails from video keyframes (I-frames)
4. **Scene-based Extraction**: Generate thumbnails at scene boundaries (requires scene detection)

**API Extensions:**
```cpp
struct ThumbnailRequest {
    int64_t timestamp_ms;      // Specific timestamp
    int max_width;             // Per-thumbnail size override
    int max_height;
};

struct ThumbnailResult {
    int64_t timestamp_ms;      // Actual timestamp
    std::vector<uint8_t> data; // RGB24 data
    int width;
    int height;
    bool is_keyframe;          // True if extracted from keyframe
};

// New method in VideoProcessor
std::vector<ThumbnailResult> generateThumbnails(
    const std::vector<uint8_t>& blob,
    const std::vector<ThumbnailRequest>& requests
);

// Or configuration-based
config.set("thumbnails.count", 10);              // Generate N thumbnails
config.set("thumbnails.mode", "interval");       // "interval" | "keyframes" | "scenes"
config.set("thumbnails.interval_ms", 10000);     // Every 10 seconds
```

## Alternative Solutions
1. **External Tool**: Use ffmpeg CLI with multiple `-ss` seeks
   - Pros: Battle-tested
   - Cons: Subprocess overhead, multiple passes through video

2. **Single-pass Decoding**: Decode video once, extract multiple frames
   - Pros: More efficient for many thumbnails
   - Cons: Memory intensive for long videos

3. **Keyframe-only Extraction**: Only extract from I-frames
   - Pros: Very fast (no decoding needed)
   - Cons: Limited timestamp precision

## Use Case
**Video Storyboard Generation:**
```cpp
// Configure for storyboard
PluginConfig config;
config.set("thumbnails.count", 12);           // 3x4 grid
config.set("thumbnails.mode", "interval");    // Even spacing
config.set("thumbnail.max_width", 160);       // Smaller thumbnails
config.set("thumbnail.max_height", 120);

VideoProcessor processor;
processor.initialize(config);

// Generate storyboard
auto result = processor.extract(video_data, "video/mp4");
auto thumbnails = result.metadata["thumbnails"];  // Array of thumbnail data

// Create storyboard image
createStoryboardGrid(thumbnails, 3, 4);  // 3 rows x 4 columns
```

**Timestamp-specific Thumbnails:**
```cpp
// Request thumbnails at specific timestamps
std::vector<ThumbnailRequest> requests = {
    {5000, 320, 240},    // 5 seconds
    {15000, 320, 240},   // 15 seconds
    {30000, 320, 240},   // 30 seconds
    {60000, 320, 240}    // 1 minute
};

auto thumbnails = processor.generateThumbnails(video_blob, requests);

for (const auto& thumb : thumbnails) {
    saveJPEG(thumb.data, thumb.width, thumb.height, 
             "thumb_" + std::to_string(thumb.timestamp_ms) + ".jpg");
}
```

## Example Usage
```cpp
// Configuration-based approach
themis::content::PluginConfig config;
config.set("thumbnails.count", 10);
config.set("thumbnails.mode", "interval");      // Even spacing throughout video
config.set("thumbnails.interval_ms", 0);        // 0 = auto-calculate
config.set("thumbnail.max_width", 320);
config.set("thumbnail.max_height", 240);

themis::content::VideoProcessor processor;
processor.initialize(config);

auto result = processor.extract(video_blob, "video/mp4");

// Access multiple thumbnails from metadata
json thumbnails = result.metadata["thumbnails"];
// [
//   {timestamp_ms: 6000, data: [base64], width: 320, height: 180},
//   {timestamp_ms: 12000, data: [base64], width: 320, height: 180},
//   ...
// ]

// Or programmatic approach
std::vector<themis::content::ThumbnailRequest> requests;
for (int i = 0; i < 10; i++) {
    int64_t timestamp = (result.media.duration_ms / 10) * i;
    requests.push_back({timestamp, 320, 240});
}

auto thumbnails = processor.generateThumbnails(video_blob, requests);
```

## Implementation Considerations
### Technical Details
- **Seeking Strategy**:
  - Multiple seeks: Simple but slow (N seeks for N thumbnails)
  - Single-pass: Fast but memory-intensive
  - Keyframe-only: Fastest, but imprecise timestamps

- **Optimization**:
  - Sort requests by timestamp for sequential seeking
  - Batch nearby requests to avoid multiple seeks
  - Cache decoded frames for nearby requests

- **Performance Tradeoffs**:
  ```
  10 thumbnails, 1-hour video:
  - Multiple seeks: ~5-10 seconds
  - Single-pass: ~30-60 seconds (but processes all frames)
  - Keyframe-only: ~0.5-1 second (no decoding)
  ```

### Dependencies
- Existing: FFmpeg (libavformat, libavcodec, libswscale)
- No new dependencies required

### Configuration Options
```cpp
// Mode-based configuration
config.set("thumbnails.count", int);           // Number of thumbnails
config.set("thumbnails.mode", string);         // "interval" | "keyframes" | "scenes" | "manual"
config.set("thumbnails.interval_ms", int);     // Interval for "interval" mode

// Per-thumbnail overrides
config.set("thumbnail.max_width", int);        // Default width
config.set("thumbnail.max_height", int);       // Default height

// Advanced options
config.set("thumbnails.keyframes_only", bool); // Only extract from keyframes (fast)
config.set("thumbnails.quality", int);         // JPEG quality (1-100) if encoding
```

### API Design
**Option 1: Configuration-based (simpler)**
```cpp
// Set count/mode in config, thumbnails returned in result.metadata
config.set("thumbnails.count", 10);
auto result = processor.extract(blob, mime);
auto thumbs = result.metadata["thumbnails"];
```

**Option 2: Explicit method (more flexible)**
```cpp
// New method for thumbnail generation
std::vector<ThumbnailResult> generateThumbnails(
    const std::vector<uint8_t>& blob,
    const std::vector<ThumbnailRequest>& requests
);
```

**Recommendation**: Support both approaches for flexibility.

## Related Features
- **Video Processor with FFmpeg** (✅ Implemented in v1.3.0+)
- **Thumbnail Generation** (✅ Implemented - single thumbnail)
- **Scene Detection** (Future work - can provide timestamps)
- **Keyframe Detection** (Can be used for thumbnail extraction)

## Additional Context
### References
- [FFmpeg Seeking Documentation](https://trac.ffmpeg.org/wiki/Seeking)
- [Video Thumbnails Best Practices](https://en.wikipedia.org/wiki/Thumbnail#Video)
- [YouTube Thumbnail Generation](https://support.google.com/youtube/answer/72431)

### Current Implementation
Single thumbnail in `src/content/video_processor.cpp`:
```cpp
std::vector<uint8_t> VideoProcessor::generateThumbnail(const std::vector<uint8_t>& blob) {
    // Seeks to 10% of duration
    // Decodes one frame
    // Returns RGB24 data
}
```

### Use Cases
1. **Video Player Scrubbing**: Show preview on timeline hover
2. **Storyboard Generation**: Create grid of thumbnails for overview
3. **Content Moderation**: Review multiple frames for policy compliance
4. **Video Search**: Index multiple frames for visual search
5. **Chapter Markers**: Generate thumbnails for chapter navigation

### Performance Estimates
**10 thumbnails from 10-minute video:**
- Current approach (10 seeks): ~5-10 seconds
- Single-pass optimization: ~2-3 minutes (decode all frames)
- Keyframe-only: ~0.5-1 second (no decoding)

**Recommendation**: Start with multiple-seek approach, optimize later if needed.

---

**Checklist:**
- [x] I have searched existing issues to ensure this is not a duplicate
- [x] I have clearly described the problem this feature solves
- [x] I have provided a detailed description of the proposed solution
- [x] I have considered the impact on existing functionality
