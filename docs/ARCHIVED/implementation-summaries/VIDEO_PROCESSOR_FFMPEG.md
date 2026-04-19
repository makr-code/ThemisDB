# Video Processor FFmpeg Integration

## Overview

The ThemisDB video processor now supports real video processing using FFmpeg libraries (libavformat, libavcodec, libswscale). This implementation provides production-ready video analysis capabilities while maintaining backward compatibility with simulation mode.

## Features

### Implemented Features

- ✅ **Real Metadata Extraction**: Extract duration, resolution, codecs, bitrate, and framerate from video files
- ✅ **Thumbnail Generation**: Generate thumbnails with color space conversion (YUV to RGB)
- ✅ **Container Format Detection**: Automatic detection of MP4, WebM, Matroska, MOV, AVI, FLV, MPEG, OGG
- ✅ **Multi-Stream Support**: Extract metadata from both video and audio streams
- ✅ **Backward Compatibility**: Falls back to simulation mode when FFmpeg is not available
- ✅ **Version Compatibility**: Works with both FFmpeg 4.x and 5.x+ (handles API changes)

### Security Features

- ✅ **Secure Temp Files**: Race condition-free temporary file creation with unique IDs
- ✅ **Resource Cleanup**: Automatic cleanup of FFmpeg resources in all code paths
- ✅ **Buffer Safety**: Row-by-row memory copy to handle padding in video frames

## Build Configuration

### Dependencies

The video processor requires the following FFmpeg libraries:
- `libavformat` - Container format handling
- `libavcodec` - Video/audio codec decoding
- `libswscale` - Frame scaling and color space conversion
- `libavutil` - Utility functions

### Build with FFmpeg Support

#### Using vcpkg (Recommended)

The FFmpeg dependency is already configured in `vcpkg.json`:

```json
{
  "name": "ffmpeg",
  "features": ["avcodec", "avformat", "swscale", "avfilter"]
}
```

Build with vcpkg:
```bash
# Install dependencies
vcpkg install

# Configure and build
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

#### Using System Libraries (Linux)

Install FFmpeg development packages:

**Ubuntu/Debian:**
```bash
sudo apt-get install libavformat-dev libavcodec-dev libswscale-dev libavutil-dev
```

**RHEL/CentOS/Fedora:**
```bash
sudo yum install ffmpeg-devel
```

**macOS:**
```bash
brew install ffmpeg
```

Build normally:
```bash
cmake -B build
cmake --build build
```

### Build without FFmpeg (Simulation Mode)

If FFmpeg is not available, the video processor will automatically fall back to simulation mode:

```bash
cmake -B build
cmake --build build
```

The build system will detect the absence of FFmpeg and print:
```
-- FFmpeg not found - video processor will use simulation mode
```

## Usage

### Initialization

```cpp
#include "content/video_processor.h"

themis::content::VideoProcessor processor;

// Configure processor
themis::content::PluginConfig config;
config.set("thumbnail.max_width", 320);
config.set("thumbnail.max_height", 240);
config.set("keyframes.max_count", 10);

processor.initialize(config);
```

### Extract Metadata

```cpp
// Read video file
std::vector<uint8_t> video_data = readVideoFile("sample.mp4");

// Extract metadata
themis::content::ExtractionOptions options;
auto result = processor.extract(video_data, "video/mp4", options);

if (result.success) {
    auto& media = result.media;
    std::cout << "Duration: " << media.duration_ms << "ms\n";
    std::cout << "Resolution: " << media.width << "x" << media.height << "\n";
    std::cout << "Video Codec: " << media.video_codec << "\n";
    std::cout << "Framerate: " << media.framerate << " fps\n";
}
```

### Generate Thumbnail

```cpp
// Extract with thumbnail generation
themis::content::ExtractionOptions options;
options.generate_thumbnail = true;

auto result = processor.extract(video_data, "video/mp4", options);

if (result.success && !result.thumbnail.empty()) {
    // Thumbnail is raw RGB24 data
    int width = result.media.width;  // Scaled to config max
    int height = result.media.height;
    
    // Convert to JPEG or save as needed
    saveRGBAsJPEG(result.thumbnail, width, height, "thumbnail.jpg");
}
```

## Implementation Details

### Metadata Extraction Process

1. **File Preparation**: Create secure temporary file with unique ID
2. **Open Video**: Use `avformat_open_input()` to open the video file
3. **Stream Discovery**: Use `avformat_find_stream_info()` to find all streams
4. **Data Extraction**:
   - Container format from `AVFormatContext`
   - Duration and bitrate from format context
   - Video stream: width, height, codec, framerate
   - Audio stream: codec, sample rate, channels
5. **Cleanup**: Close format context and remove temporary file

### Thumbnail Generation Process

1. **File Preparation**: Create secure temporary file
2. **Video Opening**: Open video with `avformat_open_input()`
3. **Stream Selection**: Find first video stream
4. **Codec Setup**: Allocate and configure codec context
5. **Seeking**: Seek to 10% of video duration
6. **Frame Decoding**: Decode frames until valid frame found
7. **Scaling**: Scale frame to thumbnail size with aspect ratio preservation
8. **Color Conversion**: Convert YUV to RGB24 using libswscale
9. **Data Extraction**: Copy RGB data row-by-row (handles padding)
10. **Cleanup**: Free all FFmpeg resources and remove temporary file

### Backward Compatibility

The implementation uses conditional compilation to maintain compatibility:

```cpp
#ifdef THEMIS_HAS_FFMPEG
    // Use real FFmpeg implementation
    return extractMetadataFFmpeg(blob);
#else
    // Fall back to simulation
    return simulateMetadata(blob);
#endif
```

When FFmpeg is not available:
- Metadata returns simulated values based on file format detection
- Thumbnails return empty vector
- All API contracts remain unchanged

## Configuration Options

### Plugin Configuration

```cpp
PluginConfig config;

// Thumbnail settings
config.set("thumbnail.max_width", 320);       // Max thumbnail width in pixels
config.set("thumbnail.max_height", 240);      // Max thumbnail height in pixels

// Feature toggles
config.set("keyframes.max_count", 10);        // Max keyframes to extract
config.set("subtitles.extract", true);        // Extract embedded subtitles
config.set("scene_detection.enabled", false); // Enable scene detection
```

### Runtime Behavior

The processor automatically:
- Maintains aspect ratio when generating thumbnails
- Seeks to 10% of video duration for thumbnail (first interesting frame)
- Handles various video formats and codecs automatically
- Manages FFmpeg resources safely with RAII patterns

## Performance Considerations

### Metadata Extraction
- **Typical Time**: 10-50ms for small videos, 100-500ms for large videos
- **Memory Usage**: ~10MB temporary buffer
- **CPU Usage**: Single-threaded codec detection

### Thumbnail Generation
- **Typical Time**: 100-500ms depending on video size and codec
- **Memory Usage**: ~20MB for frame decoding and scaling
- **CPU Usage**: Single-threaded decoding and scaling

### Optimization Tips
- Use smaller thumbnail sizes for faster generation
- Consider caching thumbnails for frequently accessed videos
- Process videos in batches for better throughput

## Troubleshooting

### FFmpeg Not Found During Build

**Symptom**: CMake warning "FFmpeg not found - video processor will use simulation mode"

**Solution**: Install FFmpeg development packages:
```bash
# Ubuntu/Debian
sudo apt-get install libavformat-dev libavcodec-dev libswscale-dev libavutil-dev

# Or use vcpkg
vcpkg install ffmpeg[avcodec,avformat,swscale,avfilter]
```

### Runtime Errors

**Symptom**: "Failed to open video file"
- **Cause**: Unsupported codec or corrupted file
- **Solution**: Verify file integrity and codec support

**Symptom**: "Decoder not found"
- **Cause**: FFmpeg built without required codec
- **Solution**: Rebuild FFmpeg with required codecs or use vcpkg

## License Considerations

FFmpeg is licensed under LGPL 2.1+ or GPL 2+, depending on build configuration:

- **LGPL Build**: Default, allows dynamic linking in proprietary software
- **GPL Build**: Required if using GPL-only components (e.g., x264 encoder)

ThemisDB uses FFmpeg as a dynamically linked optional dependency:
- ✅ Compatible with Apache 2.0 license (ThemisDB)
- ✅ No static linking - uses system or vcpkg-installed FFmpeg
- ✅ Optional feature - can be disabled at build time

## Future Enhancements

Planned features for future releases:

- [ ] **Scene Detection**: Automatic detection of scene changes
- [ ] **Subtitle Extraction**: Extract embedded subtitles (SRT, ASS, WebVTT)
- [ ] **Multiple Thumbnails**: Generate thumbnails at various timestamps
- [ ] **JPEG Encoding**: Direct JPEG output instead of raw RGB
- [ ] **Hardware Acceleration**: Use NVENC/VAAPI/QSV for faster processing
- [ ] **Batch Processing**: Process multiple videos efficiently
- [ ] **Streaming Support**: Process video streams without temporary files

## References

- [FFmpeg Documentation](https://ffmpeg.org/documentation.html)
- [libavformat API](https://ffmpeg.org/doxygen/trunk/group__libavf.html)
- [libavcodec API](https://ffmpeg.org/doxygen/trunk/group__libavc.html)
- [libswscale API](https://ffmpeg.org/doxygen/trunk/group__libsws.html)

## Related Documentation

- [Content Processing Pipeline](architecture/architecture_content_pipeline.md)
- [STUB Audit](../../Audit/STUB_AUDIT_SYSTEMATISCH.md#41-video-processor)
