---
name: Video JPEG Thumbnail Encoding
about: Add direct JPEG encoding for video thumbnails
title: '[VIDEO] JPEG Encoding: Direct JPEG output instead of raw RGB'
labels: enhancement, content-processing, video
assignees: ''
---

## Feature Description
Add native JPEG encoding to the video processor thumbnail generation, replacing the current raw RGB24 output. This reduces memory usage, network bandwidth, and simplifies thumbnail storage.

## Problem Statement
Currently, `generateThumbnail()` returns raw RGB24 data (3 bytes per pixel), which:
- Consumes significant memory (320x240 = ~230KB uncompressed)
- Requires additional processing for storage/transmission
- Lacks compression (JPEG typically achieves 10:1 ratio)
- Forces downstream consumers to handle encoding

For a 320x240 thumbnail:
- **Current**: 320 × 240 × 3 = 230,400 bytes (225 KB)
- **With JPEG**: ~15-25 KB (typical 10:1 compression)

## Proposed Solution
Add JPEG encoding using FFmpeg's MJPEG encoder:

1. **Encode to JPEG**: Use libavcodec's MJPEG encoder
2. **Configurable Quality**: JPEG quality setting (1-100)
3. **Format Selection**: Choose between RGB24 and JPEG output
4. **Metadata**: Include MIME type in result

**Technical Implementation:**
```cpp
// After RGB conversion
AVCodecContext* jpeg_ctx = avcodec_alloc_context3(avcodec_find_encoder(AV_CODEC_ID_MJPEG));
jpeg_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;  // JPEG uses YUV420P
jpeg_ctx->width = thumb_width;
jpeg_ctx->height = thumb_height;
jpeg_ctx->time_base = {1, 25};
jpeg_ctx->qmin = quality;  // JPEG quality
jpeg_ctx->qmax = quality;

avcodec_open2(jpeg_ctx, nullptr, nullptr);

// Convert RGB -> YUV420P -> Encode JPEG
AVFrame* yuv_frame = convertRGBtoYUV420P(rgb_frame);
AVPacket* jpeg_packet = av_packet_alloc();
avcodec_send_frame(jpeg_ctx, yuv_frame);
avcodec_receive_packet(jpeg_ctx, jpeg_packet);

// Return JPEG data
std::vector<uint8_t> jpeg_data(jpeg_packet->data, jpeg_packet->data + jpeg_packet->size);
```

## Alternative Solutions
1. **External Library (libjpeg-turbo)**:
   - Pros: Fast, optimized, simpler API
   - Cons: Additional dependency

2. **stb_image_write.h (single-header)**:
   - Pros: No dependency, simple integration
   - Cons: Slower than libjpeg-turbo

3. **Keep RGB, encode elsewhere**:
   - Pros: Separation of concerns
   - Cons: Wastes memory/bandwidth for most use cases

4. **WebP format**:
   - Pros: Better compression than JPEG
   - Cons: Less universal support

## Use Case
**API Response Size Optimization:**
```cpp
// Configure JPEG encoding
PluginConfig config;
config.set("thumbnail.format", "jpeg");     // "rgb24" or "jpeg"
config.set("thumbnail.quality", 85);        // JPEG quality 1-100
config.set("thumbnail.max_width", 320);
config.set("thumbnail.max_height", 240);

VideoProcessor processor;
processor.initialize(config);

// Generate JPEG thumbnail
auto result = processor.extract(video_data, "video/mp4", options);

// Thumbnail is now JPEG-encoded
// result.thumbnail = JPEG data (~20KB instead of ~230KB)
// result.thumbnail_mime_type = "image/jpeg"

// Can be directly:
// - Sent over HTTP without re-encoding
// - Stored in database as BLOB
// - Embedded in JSON response (base64)
```

## Example Usage
```cpp
// Configuration
themis::content::PluginConfig config;
config.set("thumbnail.format", "jpeg");       // Enable JPEG encoding
config.set("thumbnail.quality", 85);          // JPEG quality (1-100)
config.set("thumbnail.max_width", 320);
config.set("thumbnail.max_height", 240);

themis::content::VideoProcessor processor;
processor.initialize(config);

// Extract with JPEG thumbnail
themis::content::ExtractionOptions options;
options.generate_thumbnail = true;

auto result = processor.extract(video_blob, "video/mp4", options);

if (result.success && !result.thumbnail.empty()) {
    // Direct JPEG output
    std::cout << "Thumbnail size: " << result.thumbnail.size() << " bytes\n";
    std::cout << "MIME type: " << result.thumbnail_mime_type << "\n";
    
    // Save directly
    std::ofstream out("thumbnail.jpg", std::ios::binary);
    out.write((char*)result.thumbnail.data(), result.thumbnail.size());
    
    // Or serve via HTTP
    response.set_content(result.thumbnail, "image/jpeg");
    
    // Or base64 encode for JSON
    std::string base64 = encodeBase64(result.thumbnail);
    json["thumbnail"] = "data:image/jpeg;base64," + base64;
}
```

## Implementation Considerations
### Technical Details
- **Encoding Process**:
  1. Current: Video frame → YUV → RGB24 (via swscale)
  2. New: RGB24 → YUV420P (via swscale) → JPEG (via MJPEG codec)
  3. Alternative: Video frame → YUV → YUV420P → JPEG (skip RGB step)

- **Color Space**:
  - JPEG uses YUV420P (chroma subsampling)
  - Need to convert from RGB24 or original YUV format
  - YUVJ420P (full-range) for better quality

- **Quality vs Size Tradeoff**:
  ```
  Quality 50: ~10 KB, noticeable artifacts
  Quality 75: ~15 KB, good balance
  Quality 85: ~20 KB, high quality (recommended)
  Quality 95: ~30 KB, minimal artifacts
  Quality 100: ~50 KB, lossless (not recommended)
  ```

### Dependencies
- Existing: FFmpeg (libavcodec with MJPEG encoder)
- No new dependencies required (MJPEG is part of standard FFmpeg)

### Configuration Options
```cpp
config.set("thumbnail.format", string);        // "rgb24" | "jpeg" | "webp"
config.set("thumbnail.quality", int);          // JPEG quality 1-100 (default: 85)
config.set("thumbnail.progressive", bool);     // Progressive JPEG (default: false)
config.set("thumbnail.optimize", bool);        // Huffman optimization (default: true)
```

### Backward Compatibility
- **Default**: Keep RGB24 as default for backward compatibility
- **Opt-in**: JPEG encoding via configuration
- **Detection**: `result.thumbnail_mime_type` indicates format
  - `"image/raw-rgb24"` for RGB
  - `"image/jpeg"` for JPEG

### Performance Impact
- **Encoding Time**: +10-30ms per thumbnail (negligible)
- **Memory Savings**: ~10x reduction (230KB → 20KB)
- **Network Savings**: Same as memory savings
- **CPU**: Slightly higher (JPEG encoding), but still fast

## Related Features
- **Video Processor with FFmpeg** (✅ Implemented in v1.3.0+)
- **Thumbnail Generation** (✅ Implemented - returns RGB24)
- **Multiple Thumbnails** (Future work - will benefit from JPEG compression)
- **Content API** (Can serve JPEG thumbnails directly)

## Additional Context
### References
- [FFmpeg MJPEG Encoder](https://ffmpeg.org/ffmpeg-codecs.html#mjpeg)
- [JPEG Quality Guide](https://www.impulseadventure.com/photo/jpeg-quality.html)
- [libjpeg-turbo](https://libjpeg-turbo.org/) (alternative library)

### Current Implementation
Returns raw RGB24 in `src/content/video_processor.cpp`:
```cpp
// Copy RGB data - optimize for case without padding
thumbnail.resize(thumb_width * thumb_height * 3);
// ... copy RGB data ...
return thumbnail;  // Returns raw RGB24, no MIME type
```

### MIME Type Support
Current: `result.thumbnail_mime_type = "image/jpeg"` (set, but data is RGB)
After: Actually return JPEG data matching the MIME type

### Example JPEG Encoding Code
```cpp
#ifdef THEMIS_HAS_FFMPEG
// Find MJPEG encoder
const AVCodec* mjpeg_codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
AVCodecContext* jpeg_ctx = avcodec_alloc_context3(mjpeg_codec);

// Configure encoder
jpeg_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
jpeg_ctx->width = thumb_width;
jpeg_ctx->height = thumb_height;
jpeg_ctx->time_base = {1, 1};
jpeg_ctx->qmin = jpeg_quality;
jpeg_ctx->qmax = jpeg_quality;

avcodec_open2(jpeg_ctx, mjpeg_codec, nullptr);

// Convert RGB24 -> YUVJ420P
SwsContext* sws_ctx_yuv = sws_getContext(
    thumb_width, thumb_height, AV_PIX_FMT_RGB24,
    thumb_width, thumb_height, AV_PIX_FMT_YUVJ420P,
    SWS_BILINEAR, nullptr, nullptr, nullptr
);

AVFrame* yuv_frame = av_frame_alloc();
yuv_frame->format = AV_PIX_FMT_YUVJ420P;
yuv_frame->width = thumb_width;
yuv_frame->height = thumb_height;
av_frame_get_buffer(yuv_frame, 0);

sws_scale(sws_ctx_yuv, rgb_frame->data, rgb_frame->linesize,
          0, thumb_height, yuv_frame->data, yuv_frame->linesize);

// Encode to JPEG
AVPacket* packet = av_packet_alloc();
avcodec_send_frame(jpeg_ctx, yuv_frame);
avcodec_receive_packet(jpeg_ctx, packet);

// Copy JPEG data
std::vector<uint8_t> jpeg_data(packet->data, packet->data + packet->size);

// Cleanup
av_packet_free(&packet);
av_frame_free(&yuv_frame);
sws_freeContext(sws_ctx_yuv);
avcodec_free_context(&jpeg_ctx);

return jpeg_data;
#endif
```

### Size Comparison
**320x240 thumbnail:**
- RGB24: 320 × 240 × 3 = 230,400 bytes (225 KB)
- JPEG Q=85: ~20 KB (91% reduction)
- JPEG Q=75: ~15 KB (93% reduction)
- JPEG Q=50: ~10 KB (96% reduction, visible artifacts)

**For 10 thumbnails:**
- RGB24: 2.25 MB
- JPEG Q=85: 200 KB (89% reduction)

---

**Checklist:**
- [x] I have searched existing issues to ensure this is not a duplicate
- [x] I have clearly described the problem this feature solves
- [x] I have provided a detailed description of the proposed solution
- [x] I have considered the impact on existing functionality
