---
name: Video Scene Detection
about: Implement automatic scene change detection in video processor
title: '[VIDEO] Scene Detection: Automatic detection of scene changes'
labels: enhancement, content-processing, video
assignees: ''
---

## Feature Description
Implement automatic scene change detection for video files using FFmpeg. This feature will analyze video frames to identify scene transitions and return timestamps of scene changes.

## Problem Statement
Currently, the video processor can extract metadata and generate thumbnails, but cannot identify scene changes within videos. Scene detection is useful for:
- Automatic video segmentation
- Smart thumbnail selection at scene boundaries
- Content-based video indexing
- Video summarization

## Proposed Solution
Extend the existing FFmpeg-based video processor to:
1. Decode video frames sequentially
2. Calculate frame differences using histograms or pixel differences
3. Detect scene changes when difference exceeds threshold
4. Return array of timestamps (in milliseconds) where scenes change

**Technical Implementation:**
- Use `avcodec_send_packet()` and `avcodec_receive_frame()` for frame decoding
- Calculate frame similarity using histogram comparison or SAD (Sum of Absolute Differences)
- Configurable threshold via `PluginConfig`: `scene_detection.threshold` (default: 0.3)
- Configurable interval via `PluginConfig`: `scene_detection.min_interval_ms` (default: 1000ms)

## Alternative Solutions
1. **External Tool Integration**: Use `ffmpeg` command-line with `select` filter
   - Pros: Mature implementation
   - Cons: Requires subprocess execution, less control

2. **OpenCV-based Detection**: Use OpenCV's scene detection algorithms
   - Pros: More detection algorithms available
   - Cons: Additional dependency

3. **Machine Learning Approach**: Train a model to detect scene changes
   - Pros: Potentially more accurate
   - Cons: High complexity, requires training data

## Use Case
**Enterprise Analytics - Video Upload Validation:**
```cpp
// Enable scene detection
PluginConfig config;
config.set("scene_detection.enabled", true);
config.set("scene_detection.threshold", 0.3);
config.set("scene_detection.min_interval_ms", 1000);

VideoProcessor processor;
processor.initialize(config);

// Process video
auto result = processor.extract(video_data, "video/mp4");

// Access scene timestamps
auto scenes = result.metadata["scene_changes_ms"];
for (auto timestamp : scenes) {
    std::cout << "Scene change at: " << timestamp << "ms\n";
    // Generate thumbnail at scene boundary
}
```

## Example Usage
```cpp
// Configuration
themis::content::PluginConfig config;
config.set("scene_detection.enabled", true);
config.set("scene_detection.threshold", 0.3);  // 30% change threshold
config.set("scene_detection.min_interval_ms", 1000);  // Min 1 second between scenes

// Initialize processor
themis::content::VideoProcessor processor;
processor.initialize(config);

// Extract with scene detection
auto result = processor.extract(video_blob, "video/mp4");

// Scene timestamps available in metadata
json scenes = result.metadata["scene_changes_ms"];
// Example output: [5000, 12500, 24300, 45600]
```

## Implementation Considerations
### Technical Details
- **Performance**: Scene detection requires decoding all frames, which is CPU-intensive
  - Optimize by analyzing keyframes only (configurable)
  - Consider downscaling frames before comparison
  - Use SIMD for histogram calculations

- **Memory**: Frame buffers consume ~10-20MB per frame
  - Process frames in streaming fashion (decode-analyze-discard)
  - Avoid storing all frames in memory

- **Accuracy**: Balance between false positives and false negatives
  - Provide configurable threshold
  - Support multiple detection algorithms (histogram, SAD, edge detection)

### Dependencies
- Existing: FFmpeg (libavformat, libavcodec, libavutil)
- New: None (use existing FFmpeg infrastructure)

### Configuration Options
```cpp
config.set("scene_detection.enabled", bool);           // Enable/disable feature
config.set("scene_detection.threshold", double);       // 0.0-1.0, change threshold
config.set("scene_detection.min_interval_ms", int);    // Minimum time between scenes
config.set("scene_detection.algorithm", string);       // "histogram" | "sad" | "edge"
config.set("scene_detection.keyframes_only", bool);    // Only analyze keyframes
```

## Related Features
- **Video Processor with FFmpeg** (✅ Implemented in v1.3.0+)
- **Thumbnail Generation** (✅ Implemented)
- **Multiple Thumbnails** (Future work - can use scene timestamps)
- **Content-based Indexing** (Related to scene metadata)

## Additional Context
### References
- [FFmpeg Scene Detection](https://ffmpeg.org/ffmpeg-filters.html#select_002c-aselect)
- [PySceneDetect Algorithm](https://scenedetect.com/docs/latest/api/detectors/)
- [Shot Boundary Detection Research](https://ieeexplore.ieee.org/document/8485095)

### Current Implementation
Scene detection placeholder exists in `src/content/video_processor.cpp`:
```cpp
std::vector<int64_t> VideoProcessor::detectScenes(const std::vector<uint8_t>& blob) {
    // Real implementation would:
    // 1. Decode video frames
    // 2. Calculate frame differences/histograms
    // 3. Detect scene changes based on threshold
    
    return std::vector<int64_t>();
}
```

### Performance Estimates
- **Processing Time**: ~2-5x realtime (e.g., 10-minute video takes 20-50 minutes)
- **Optimization**: Keyframe-only mode: ~10-20x faster (1-5 minutes for 10-minute video)
- **Memory**: ~20MB peak (single frame buffer)

---

**Checklist:**
- [x] I have searched existing issues to ensure this is not a duplicate
- [x] I have clearly described the problem this feature solves
- [x] I have provided a detailed description of the proposed solution
- [x] I have considered the impact on existing functionality
