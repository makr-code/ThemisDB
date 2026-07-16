# Content Troubleshooting Guide

The `content` module handles multi-format content processing and management for ThemisDB, including audio, image, CAD, and archive files; asynchronous ingestion workers; content validation, security scanning, policy enforcement, and LLM-powered content enrichment.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `ContentManager: unsupported type` | File format not registered | Add MIME type to `content.supported_types` |
| `ImageProcessor: library not found` | libvips/ImageMagick not installed | Install `libvips-dev` |
| Content policy blocks valid upload | Policy too strict | Adjust `content.policy.max_file_size_mb` |
| `AsyncIngestionWorker: queue full` | Too many pending ingestion jobs | Increase `content.ingestion.queue_size` |
| `ContentSecurity: scan timeout` | ClamAV timeout | Increase `content.security.scan_timeout_ms` |
| LLM content enrichment slow | Large documents sent to LLM | Enable `content.llm.chunk_size_tokens` |
| `ContentLogger: permission denied` | Log directory not writable | Fix permissions on log directory |
| Audio processing fails | ffmpeg not installed | Install `ffmpeg` |
| `GeoProcessor: GDAL error` | GDAL not installed | Install `libgdal-dev` |
| Content validator rejects valid metadata | Metadata schema mismatch | Update `content.validator.schema` |

## Common Issues

### Issue 1: Unsupported Content Type

**Description:** File uploads fail because the MIME type is not in the supported list.

**Symptoms:**
- Error: `ContentType: MIME type 'application/x-parquet' not supported`
- Upload API returns `415 Unsupported Media Type`

**Cause:** The MIME type is not registered as a supported content type.

**Solution:**
```yaml
content:
  supported_types:
    - application/pdf
    - image/jpeg
    - image/png
    - audio/mpeg
    - application/x-parquet       # add custom types
    - application/octet-stream    # allow all binary files
  type_detection: auto            # detect by content, not extension
```

---

### Issue 2: Image Processor Library Not Found

**Description:** Image processing fails because native libraries are not installed.

**Symptoms:**
- Log: `ImageProcessor: libvips.so not found; image processing disabled`
- Image thumbnails not generated

**Cause:** libvips or ImageMagick not installed.

**Solution:**
```bash
# Install libvips
apt install libvips-dev

# Or install ImageMagick
apt install imagemagick

# Verify
ldconfig -p | grep vips
convert --version
```
```yaml
content:
  image:
    backend: vips                 # "vips" | "imagemagick" | "stb"
    thumbnail_sizes: [100, 300, 800]
    max_image_dimension: 10000
```

---

### Issue 3: Async Ingestion Worker Queue Full

**Description:** New content ingestion jobs are rejected because the queue is full.

**Symptoms:**
- Log: `AsyncIngestionWorker: queue full (max=100); rejecting job`
- Content uploads succeed but processing is delayed or lost

**Cause:** Too many pending jobs; worker threads insufficient.

**Solution:**
```yaml
content:
  ingestion:
    worker_threads: 8             # increase from default 2
    queue_size: 1000
    job_timeout_ms: 300000
    retry_failed_jobs: true
    retry_max: 3
```

---

### Issue 4: Content Security Scan Timeout

**Description:** Content security scanning times out for large files.

**Symptoms:**
- Log: `ContentSecurity: scan timeout after 5000ms for file=large_video.mp4`
- Large file uploads always fail

**Cause:** Scan timeout too short; ClamAV overloaded.

**Solution:**
```yaml
content:
  security:
    scan_enabled: true
    scan_timeout_ms: 60000        # 60 seconds for large files
    max_scan_size_mb: 500         # skip scanning files > 500MB
    async_scan: true              # scan asynchronously; allow upload to proceed
    quarantine_dir: /var/lib/themisdb/quarantine
```

---

### Issue 5: Audio Processing Fails

**Description:** Audio file processing fails because ffmpeg is not available.

**Symptoms:**
- Log: `AudioProcessor: ffmpeg not found; audio processing disabled`
- Audio transcription and metadata extraction fail

**Cause:** ffmpeg not installed.

**Solution:**
```bash
# Install ffmpeg
apt install ffmpeg

# Verify
ffmpeg -version
```
```yaml
content:
  audio:
    processor: ffmpeg
    ffmpeg_path: /usr/bin/ffmpeg
    output_format: wav
    sample_rate: 16000            # 16kHz for speech recognition
    channels: 1                   # mono for speech
```

---

### Issue 6: LLM Content Enrichment Timeout

**Description:** LLM-based content enrichment times out for long documents.

**Symptoms:**
- Log: `ContentManagerLlm: enrichment timeout after 30000ms for document`
- No AI-generated metadata for long documents

**Cause:** Full document sent to LLM; exceeds context window.

**Solution:**
```yaml
content:
  llm:
    enabled: true
    chunk_size_tokens: 512        # split documents into chunks
    max_chunks_per_doc: 10        # limit chunks processed per document
    timeout_ms: 60000
    enrichment_fields:
      - summary
      - keywords
      - sentiment
    async: true                   # enrich asynchronously after ingestion
```

---

### Issue 7: Content Policy Blocks Valid Uploads

**Description:** Content policy rejects valid business documents.

**Symptoms:**
- Upload returns `422 Content Policy Violation`
- Log: `ContentPolicy: file 'report.pdf' exceeds max_file_size_mb=10`

**Cause:** Policy limits are too restrictive.

**Solution:**
```yaml
content:
  policy:
    max_file_size_mb: 500         # increase from 10
    allowed_extensions: []        # empty = all extensions
    max_metadata_size_kb: 64
    allow_encrypted_files: true
    require_virus_scan: false
```

---

### Issue 8: CAD File Processing Fails

**Description:** CAD files cannot be processed because the CAD library is missing.

**Symptoms:**
- Log: `CadProcessor: no CAD library available; CAD processing disabled`
- `.dwg`, `.dxf` files not processed

**Cause:** OpenCASCADE or libDXF not installed.

**Solution:**
```bash
# Install libdxf
apt install libdxf-dev

# Or use the built-in basic DXF parser
```
```yaml
content:
  cad:
    enabled: true
    backend: builtin              # "builtin" | "opencascade" | "libdxf"
    supported_formats: [dxf, svg]  # builtin supports only DXF/SVG
```

## Diagnostic Commands

```bash
# Content module health
themisdb-admin content status

# Ingestion queue stats
themisdb-admin content ingestion-stats

# Content type support
themisdb-admin content supported-types

# Pending ingestion jobs
themisdb-admin content jobs list --state pending

# Live content metrics
curl -s http://localhost:9100/metrics | grep themisdb_content

# Tail content logs
journalctl -u themisdb -f | grep -E "content|ingestion|image|audio|cad|security.scan"
```

## Configuration Reference

```yaml
content:
  enabled: true
  policy:
    max_file_size_mb: 100
    require_virus_scan: true
  ingestion:
    worker_threads: 4
    queue_size: 500
  security:
    scan_enabled: true
    scan_timeout_ms: 30000
  llm:
    enabled: false
    chunk_size_tokens: 512
  image:
    backend: vips
  audio:
    processor: ffmpeg
```

## Known Limitations

- Archive extraction (ZIP, tar.gz) depth is limited to 3 levels to prevent zip-bomb attacks.
- LLM enrichment is asynchronous; enriched metadata may not be available immediately after upload.
- CAD processing supports only 2D DXF in the built-in backend; 3D formats require OpenCASCADE.
- Video processing requires ffmpeg with appropriate codecs; not all containers are supported.

## Related Documentation

- [Content Module ROADMAP](../../src/content/ROADMAP.md)
- [Content Roadmap](../de/roadmap/content_roadmap.md)
- [Ingestion Troubleshooting](./ingestion_troubleshooting.md)
- [Security Troubleshooting](./security_troubleshooting.md)
- [Video Processor ffmpeg](../ARCHIVED/implementation-summaries/VIDEO_PROCESSOR_FFMPEG.md)
