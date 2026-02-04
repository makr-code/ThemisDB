---
name: Video Hardware Acceleration
about: Add hardware-accelerated video processing (NVENC/VAAPI/QSV)
title: '[VIDEO] Hardware Acceleration: Use NVENC/VAAPI/QSV for faster processing'
labels: enhancement, content-processing, video, performance
assignees: ''
---

## Feature Description
Add hardware acceleration support for video decoding and encoding using NVIDIA NVENC, Intel Quick Sync Video (QSV), and VA-API. This will significantly improve video processing performance for metadata extraction and thumbnail generation.

## Problem Statement
Current video processing uses CPU-only decoding, which is:
- **Slow**: 2-5x slower than hardware decoding
- **Resource-intensive**: High CPU usage during video processing
- **Not scalable**: Limited throughput for batch processing
- **Inefficient**: Underutilizes available GPU resources

Hardware acceleration can provide:
- **5-10x faster decoding** for 4K videos
- **Lower CPU usage** (offload to GPU)
- **Higher throughput** for concurrent video processing
- **Better energy efficiency** for data centers

## Proposed Solution
Integrate FFmpeg's hardware acceleration APIs:

### 1. **Hardware Decoders**
- **NVIDIA (NVDEC/CUVID)**: For NVIDIA GPUs
- **Intel QSV**: For Intel CPUs with integrated graphics
- **VA-API**: For Intel/AMD GPUs on Linux
- **Video Toolbox**: For Apple Silicon/macOS

### 2. **Implementation Approach**
```cpp
// Auto-detect hardware acceleration
AVBufferRef* hw_device_ctx = nullptr;
enum AVHWDeviceType hw_type = AV_HWDEVICE_TYPE_NONE;

// Try CUDA first (NVIDIA)
if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) >= 0) {
    hw_type = AV_HWDEVICE_TYPE_CUDA;
}
// Fallback to VAAPI (Intel/AMD on Linux)
else if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI, nullptr, nullptr, 0) >= 0) {
    hw_type = AV_HWDEVICE_TYPE_VAAPI;
}
// Fallback to QSV (Intel)
else if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_QSV, nullptr, nullptr, 0) >= 0) {
    hw_type = AV_HWDEVICE_TYPE_QSV;
}

// Set hardware context for decoder
codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
```

### 3. **Configuration**
```cpp
config.set("video.hw_accel", "auto");        // "auto" | "cuda" | "vaapi" | "qsv" | "none"
config.set("video.hw_device", "");           // Device path (empty = auto)
config.set("video.prefer_hw", true);         // Prefer HW, fallback to SW
```

## Alternative Solutions
1. **Software-only with SIMD**: Use AVX2/NEON optimizations
   - Pros: Portable, no hardware dependency
   - Cons: Still slower than GPU

2. **OpenMAX (embedded devices)**: For ARM SBCs
   - Pros: Works on Raspberry Pi
   - Cons: Limited to embedded platforms

3. **DirectX Video Acceleration (Windows)**: DXVA2/D3D11VA
   - Pros: Native Windows support
   - Cons: Windows-only

4. **Separate hardware service**: Dedicated GPU server for video processing
   - Pros: Centralized, scalable
   - Cons: Additional infrastructure complexity

## Use Case
**High-throughput Video Ingestion:**
```cpp
// Enable hardware acceleration
PluginConfig config;
config.set("video.hw_accel", "auto");          // Auto-detect best HW
config.set("video.prefer_hw", true);           // Fallback to SW if needed
config.set("thumbnail.max_width", 320);
config.set("thumbnail.max_height", 240);

VideoProcessor processor;
processor.initialize(config);

// Process multiple videos concurrently
std::vector<std::future<ContentExtractionResult>> futures;
for (const auto& video : videos) {
    futures.push_back(std::async([&]() {
        return processor.extract(video, "video/mp4");
    }));
}

// Hardware acceleration allows more concurrent processing
// CPU: ~4-8 concurrent videos
// GPU: ~20-50 concurrent videos (depending on GPU)
```

## Example Usage
```cpp
// Configuration
themis::content::PluginConfig config;
config.set("video.hw_accel", "auto");           // Try CUDA -> VAAPI -> QSV -> CPU
config.set("video.hw_device", "");              // Empty = default device
config.set("video.prefer_hw", true);            // Fallback to CPU if HW fails
config.set("video.hw_surfaces", 4);             // Number of HW surfaces (buffering)

themis::content::VideoProcessor processor;
processor.initialize(config);

// Processing is transparent - same API
auto result = processor.extract(video_blob, "video/mp4");

// Check if hardware was used
if (result.metadata.contains("hw_accel_used")) {
    std::cout << "Hardware: " << result.metadata["hw_accel_used"] << "\n";
    // Output: "cuda" | "vaapi" | "qsv" | "none"
}

// Performance improvement
std::cout << "Processing time: " << result.processing_time_ms << "ms\n";
// CPU: ~500-2000ms for 1080p 1-minute video
// GPU: ~100-400ms for same video (3-5x faster)
```

## Implementation Considerations
### Technical Details
- **Hardware Decoder Selection**:
  ```cpp
  // Check codec support for hardware
  const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
  for (int i = 0;; i++) {
      const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
      if (!config) break;
      if (config->device_type == hw_type) {
          // Hardware decoder available
      }
  }
  ```

- **Surface Management**:
  - GPU decoding outputs to GPU memory
  - Need to transfer to CPU for processing
  - Use `av_hwframe_transfer_data()` to copy GPU → CPU

- **Format Conversion**:
  - Hardware surfaces use NV12/P010 formats
  - Need conversion to RGB24 for thumbnails
  - Can do on GPU with swscale hardware context

### Dependencies
- **Existing**: FFmpeg (libavcodec, libavutil, libavformat)
- **Runtime**:
  - CUDA: NVIDIA drivers (no CUDA toolkit needed for decoding)
  - VA-API: libva, va-api drivers
  - QSV: Intel Media SDK or oneVPL
- **Build-time**: FFmpeg compiled with `--enable-cuda --enable-vaapi --enable-qsv`

### Platform Support
| Platform | Hardware API | GPU Vendors |
|----------|-------------|-------------|
| Linux | CUDA, VA-API, QSV | NVIDIA, Intel, AMD |
| Windows | CUDA, DXVA2, D3D11VA | NVIDIA, Intel, AMD |
| macOS | Video Toolbox | Apple Silicon, Intel |
| Docker | CUDA, VA-API | NVIDIA, Intel |

### Configuration Options
```cpp
config.set("video.hw_accel", string);          // "auto" | "cuda" | "vaapi" | "qsv" | "videotoolbox" | "none"
config.set("video.hw_device", string);         // Device identifier (empty = default)
config.set("video.prefer_hw", bool);           // Fallback to SW if HW fails
config.set("video.hw_surfaces", int);          // Number of HW decode surfaces (2-16)
config.set("video.hw_copy_mode", string);      // "auto" | "gpu" | "cpu" - where to do RGB conversion
```

### Performance Benchmarks
**1080p H.264 video (1 minute):**
| Method | Decode Time | CPU Usage | GPU Usage |
|--------|-------------|-----------|-----------|
| CPU (Software) | 2000ms | 100% | 0% |
| CUDA (NVIDIA) | 400ms | 15% | 30% |
| VA-API (Intel) | 500ms | 20% | 40% |
| QSV (Intel) | 450ms | 18% | 35% |

**4K H.265 video (1 minute):**
| Method | Decode Time | CPU Usage | GPU Usage |
|--------|-------------|-----------|-----------|
| CPU (Software) | 8000ms | 100% | 0% |
| CUDA (NVIDIA) | 800ms | 12% | 25% |
| VA-API (Intel) | 1200ms | 18% | 40% |

### Error Handling
```cpp
// Graceful fallback to software decoding
if (hw_accel_enabled) {
    try {
        result = decodeWithHardware(blob);
    } catch (const HardwareError& e) {
        THEMIS_WARN("Hardware decoding failed: {}, falling back to software", e.what());
        result = decodeWithSoftware(blob);
    }
}
```

## Related Features
- **Video Processor with FFmpeg** (✅ Implemented in v1.3.0+)
- **Thumbnail Generation** (✅ Implemented - will be accelerated)
- **Batch Processing** (Future work - benefits from HW acceleration)
- **Distributed Sharding** (HW acceleration per shard)

## Additional Context
### References
- [FFmpeg Hardware Acceleration](https://trac.ffmpeg.org/wiki/HWAccelIntro)
- [NVIDIA Video Codec SDK](https://developer.nvidia.com/nvidia-video-codec-sdk)
- [Intel Media SDK](https://github.com/Intel-Media-SDK/MediaSDK)
- [VA-API Documentation](https://github.com/intel/libva)

### Current Bottlenecks
CPU decoding in `generateThumbnailFFmpeg()`:
```cpp
// Current: CPU decoding
avcodec_send_packet(codec_ctx, packet);
avcodec_receive_frame(codec_ctx, frame);  // CPU decode
```

### Hardware Acceleration Flow
```
1. Initialize HW context: av_hwdevice_ctx_create()
2. Set codec context: codec_ctx->hw_device_ctx = ...
3. Decode to GPU: avcodec_receive_frame() → frame in GPU memory
4. Transfer to CPU: av_hwframe_transfer_data()
5. Convert format: swscale RGB conversion
6. Generate thumbnail
```

### Docker Considerations
**NVIDIA GPU in Docker:**
```dockerfile
# Dockerfile
FROM nvidia/cuda:11.8-runtime

# Install FFmpeg with CUDA support
RUN apt-get update && apt-get install -y ffmpeg
```

```bash
# Run with GPU access
docker run --gpus all themisdb:latest
```

**Intel VA-API in Docker:**
```bash
# Pass device to container
docker run --device=/dev/dri themisdb:latest
```

### Monitoring
```cpp
// Add metrics to statistics
json stats = processor.getStatistics();
stats["hw_accel_available"] = isHardwareAvailable();
stats["hw_accel_type"] = getHardwareType();  // "cuda" | "vaapi" | "qsv"
stats["videos_hw_decoded"] = hw_decode_count;
stats["videos_sw_decoded"] = sw_decode_count;
stats["avg_hw_decode_ms"] = avg_hw_time;
stats["avg_sw_decode_ms"] = avg_sw_time;
```

---

**Checklist:**
- [x] I have searched existing issues to ensure this is not a duplicate
- [x] I have clearly described the problem this feature solves
- [x] I have provided a detailed description of the proposed solution
- [x] I have considered the impact on existing functionality
