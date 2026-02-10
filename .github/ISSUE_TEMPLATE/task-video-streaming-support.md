---
name: Video Streaming Support
about: Process video streams without temporary files
title: '[VIDEO] Streaming Support: Process video streams without temporary files'
labels: enhancement, content-processing, video, performance
assignees: ''
---

## Feature Description
Add support for processing video streams directly in memory without creating temporary files. This enables real-time video analysis, reduces I/O overhead, and improves security by avoiding disk writes.

## Problem Statement
Currently, the video processor:
- Writes video data to temporary files on disk
- Requires file I/O for every video operation
- Cannot process video streams (HTTP, RTSP, WebRTC)
- Leaves disk footprint (security/privacy concern)
- Slower due to disk I/O bottleneck

**Limitations:**
```cpp
// Current implementation requires temp file
std::string temp_path = temp_directory / unique_filename;
std::ofstream temp_file(temp_path, std::ios::binary);
temp_file.write(blob.data(), blob.size());  // Write to disk
avformat_open_input(&fmt_ctx, temp_path.c_str(), nullptr, nullptr);
```

**Problems:**
- I/O overhead: ~50-200ms for 50MB video
- Disk wear on SSDs
- Security: Temp files may contain sensitive data
- Cannot handle live streams

## Proposed Solution
Implement FFmpeg custom I/O using `AVIOContext`:

### 1. **In-Memory Processing**
```cpp
// Custom read function
static int read_packet(void* opaque, uint8_t* buf, int buf_size) {
    MemoryBuffer* buffer = static_cast<MemoryBuffer*>(opaque);
    int bytes_to_read = std::min(buf_size, buffer->remaining());
    memcpy(buf, buffer->current(), bytes_to_read);
    buffer->advance(bytes_to_read);
    return bytes_to_read;
}

// Create custom I/O context
AVIOContext* avio_ctx = avio_alloc_context(
    buffer, buffer_size, 0, &memory_buffer,
    read_packet, nullptr, nullptr
);

// Open format with custom I/O
AVFormatContext* fmt_ctx = avformat_alloc_context();
fmt_ctx->pb = avio_ctx;
avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr);
```

### 2. **Stream Processing**
- HTTP streams: `http://example.com/video.mp4`
- RTSP streams: `rtsp://camera.local/stream`
- RTP/UDP streams
- WebRTC data channels
- Pipe inputs: `pipe:0`

### 3. **Configuration**
```cpp
config.set("video.use_temp_files", false);     // Disable temp files
config.set("video.stream_buffer_size", 65536); // Buffer size for streaming
config.set("video.stream_timeout_ms", 10000);  // Timeout for stream reads
```

## Alternative Solutions
1. **Named Pipes (FIFO)**: Use Unix pipes instead of files
   - Pros: No disk I/O
   - Cons: Platform-specific, still requires special files

2. **Memory-mapped Files**: Use `mmap()` for temp files
   - Pros: Reduces I/O overhead
   - Cons: Still requires file system

3. **RAMDisk**: Create temp files in `/dev/shm`
   - Pros: Fast, no disk I/O
   - Cons: Linux-specific, limited RAM

4. **External Streaming Service**: Dedicated video streaming server
   - Pros: Specialized infrastructure
   - Cons: Added complexity, network overhead

## Use Case
**Real-time Camera Feed Processing:**
```cpp
// Process RTSP stream from security camera
PluginConfig config;
config.set("video.use_temp_files", false);
config.set("video.stream_timeout_ms", 5000);

VideoProcessor processor;
processor.initialize(config);

// Process stream URL directly
std::string stream_url = "rtsp://camera.local:554/stream";
auto result = processor.extractFromURL(stream_url);

// Or process in-memory buffer (e.g., from HTTP upload)
std::vector<uint8_t> video_data = receiveHTTPUpload();
auto result = processor.extract(video_data, "video/mp4");
// No temp file created!
```

## Example Usage
```cpp
// Configuration for in-memory processing
themis::content::PluginConfig config;
config.set("video.use_temp_files", false);       // Process in memory
config.set("video.stream_buffer_size", 65536);   // 64KB buffer
config.set("thumbnail.max_width", 320);
config.set("thumbnail.max_height", 240);

themis::content::VideoProcessor processor;
processor.initialize(config);

// Option 1: Process from memory buffer (no temp file)
std::vector<uint8_t> video_blob = loadVideoToMemory("video.mp4");
auto result = processor.extract(video_blob, "video/mp4");
// No disk I/O - processes entirely in memory!

// Option 2: Process from URL/stream
std::string rtsp_url = "rtsp://192.168.1.100:554/stream1";
auto result = processor.extractFromURL(rtsp_url);
// Opens stream directly, no download needed

// Option 3: Process from stdin pipe
auto result = processor.extractFromStream(std::cin);
// Useful for: ffmpeg | themisdb process-video

// Performance benefit
std::cout << "Processing time: " << result.processing_time_ms << "ms\n";
// With temp file: ~500ms (50MB video)
// Without temp file: ~300ms (40% faster)
```

## Implementation Considerations
### Technical Details
- **Custom I/O Context**:
  ```cpp
  struct MemoryBuffer {
      const uint8_t* data;
      size_t size;
      size_t position = 0;
      
      int remaining() const { return size - position; }
      const uint8_t* current() const { return data + position; }
      void advance(int bytes) { position += bytes; }
  };
  
  // Callbacks for AVIOContext
  static int read_packet(void* opaque, uint8_t* buf, int buf_size);
  static int64_t seek(void* opaque, int64_t offset, int whence);
  ```

- **Buffer Management**:
  - Allocate internal buffer for FFmpeg (default: 64KB)
  - For large videos, use incremental reading
  - Support seeking (required for thumbnail generation)

- **Stream Protocols**:
  - HTTP/HTTPS: FFmpeg built-in support
  - RTSP/RTMP: Requires libavformat network support
  - File: Standard file I/O
  - Pipe: stdin/stdout
  - Memory: Custom AVIOContext

### Dependencies
- Existing: FFmpeg (libavformat with custom I/O support)
- No new dependencies required

### Configuration Options
```cpp
config.set("video.use_temp_files", bool);        // true = use temp files (default), false = in-memory
config.set("video.stream_buffer_size", int);     // Buffer size in bytes (default: 65536)
config.set("video.stream_timeout_ms", int);      // Timeout for stream operations (default: 30000)
config.set("video.allow_network_streams", bool); // Allow HTTP/RTSP URLs (default: false, security)
config.set("video.max_memory_buffer_mb", int);   // Max in-memory buffer (default: 500MB)
```

### Backward Compatibility
- **Default**: Keep temp file approach for safety/compatibility
- **Opt-in**: Enable in-memory via configuration
- **Automatic**: Use in-memory for small videos (<100MB), temp file for large

### Security Considerations
- **Network streams**: Validate URLs, prevent SSRF attacks
- **Memory limit**: Prevent DoS by limiting buffer size
- **Timeout**: Prevent hanging on slow streams
- **Protocol whitelist**: Only allow safe protocols

```cpp
// Security checks
if (config.get<bool>("video.allow_network_streams")) {
    // Validate URL
    if (isNetworkURL(input)) {
        if (!isWhitelisted(input)) {
            throw SecurityError("URL not whitelisted");
        }
    }
}

// Memory limit
if (blob.size() > config.get<int>("video.max_memory_buffer_mb") * 1024 * 1024) {
    throw MemoryError("Video exceeds maximum buffer size");
}
```

### Performance Impact
**50MB video processing:**
| Method | I/O Time | Processing Time | Total |
|--------|----------|-----------------|-------|
| Temp File | 150ms | 300ms | 450ms |
| In-Memory | 0ms | 300ms | 300ms |
| RTSP Stream | N/A (streaming) | 300ms | 300ms |

**Savings:**
- Small videos (<10MB): ~50ms saved
- Medium videos (50MB): ~150ms saved
- Large videos (500MB): ~1000ms saved

## Related Features
- **Video Processor with FFmpeg** (✅ Implemented in v1.3.0+)
- **Security Scanner** (Temp files avoided = better security)
- **Real-time Processing** (Enables live stream analysis)
- **Batch Processing** (Faster with in-memory processing)

## Additional Context
### References
- [FFmpeg Custom I/O](https://ffmpeg.org/doxygen/trunk/avio_8h.html)
- [AVIOContext Documentation](https://ffmpeg.org/doxygen/trunk/structAVIOContext.html)
- [FFmpeg I/O Examples](https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/avio_reading.c)

### Current Implementation
Creates temp file in `extractMetadataFFmpeg()`:
```cpp
// Current: Write to temp file
std::string temp_path = temp_directory / unique_filename;
std::ofstream temp_file(temp_path, std::ios::binary);
temp_file.write(blob.data(), blob.size());
avformat_open_input(&fmt_ctx, temp_path.c_str(), nullptr, nullptr);
// ... processing ...
std::filesystem::remove(temp_path);
```

### Example: Custom I/O Implementation
```cpp
#ifdef THEMIS_HAS_FFMPEG
MediaExtractionData VideoProcessor::extractMetadataFFmpeg(
    const std::vector<uint8_t>& blob) {
    
    MediaExtractionData data;
    
    if (use_temp_files_) {
        // Existing temp file approach
        return extractMetadataFromFile(blob);
    }
    
    // In-memory approach
    MemoryBuffer mem_buffer{blob.data(), blob.size()};
    
    // Allocate I/O buffer
    constexpr size_t BUFFER_SIZE = 65536;
    uint8_t* io_buffer = static_cast<uint8_t*>(av_malloc(BUFFER_SIZE));
    
    // Create custom I/O context
    AVIOContext* avio_ctx = avio_alloc_context(
        io_buffer, BUFFER_SIZE, 0, &mem_buffer,
        [](void* opaque, uint8_t* buf, int size) -> int {
            auto* buffer = static_cast<MemoryBuffer*>(opaque);
            int bytes = std::min(size, buffer->remaining());
            memcpy(buf, buffer->current(), bytes);
            buffer->advance(bytes);
            return bytes > 0 ? bytes : AVERROR_EOF;
        },
        nullptr,  // write callback (not needed)
        [](void* opaque, int64_t offset, int whence) -> int64_t {
            auto* buffer = static_cast<MemoryBuffer*>(opaque);
            if (whence == SEEK_SET) {
                buffer->position = offset;
            } else if (whence == SEEK_CUR) {
                buffer->position += offset;
            } else if (whence == SEEK_END) {
                buffer->position = buffer->size + offset;
            } else if (whence == AVSEEK_SIZE) {
                return buffer->size;
            }
            return buffer->position;
        }
    );
    
    // Open format with custom I/O
    AVFormatContext* fmt_ctx = avformat_alloc_context();
    fmt_ctx->pb = avio_ctx;
    
    if (avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr) < 0) {
        av_freep(&avio_ctx->buffer);
        avio_context_free(&avio_ctx);
        throw std::runtime_error("Failed to open input");
    }
    
    // ... rest of processing ...
    
    // Cleanup
    avformat_close_input(&fmt_ctx);
    av_freep(&avio_ctx->buffer);
    avio_context_free(&avio_ctx);
    
    return data;
}
#endif
```

### Use Cases
1. **Web Upload**: Process video immediately after HTTP upload
2. **Live Streams**: Analyze RTSP camera feeds in real-time
3. **Microservices**: Process video from message queue without disk
4. **Privacy**: Avoid writing sensitive videos to disk
5. **Performance**: Eliminate I/O bottleneck for small videos

---

**Checklist:**
- [x] I have searched existing issues to ensure this is not a duplicate
- [x] I have clearly described the problem this feature solves
- [x] I have provided a detailed description of the proposed solution
- [x] I have considered the impact on existing functionality
