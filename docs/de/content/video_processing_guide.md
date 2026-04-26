# Video Processing Guide

**Version:** v1.3.0 Phase 2  
**Status:** Production-Ready  
**Last Updated:** April 2026

---

## Overview

ThemisDB's Video Processor provides comprehensive video content analysis using FFmpeg/LibAV. It extracts metadata, detects keyframes and scenes, generates thumbnails, and processes subtitles from various video formats.

## Supported Formats

- MP4 (.mp4, .m4v)
- WebM (.webm)
- Matroska (.mkv)
- QuickTime (.mov)
- AVI (.avi)
- FLV (.flv)
- MPEG (.mpeg, .mpg)
- Ogg Video (.ogv)

## Features

### Metadata Extraction
- Video codec, resolution, bitrate, frame rate
- Audio codec, sample rate, channels
- Duration and file format information

### Keyframe Detection
- Automatic I-frame detection
- Configurable maximum keyframe count
- Timestamp-based access

### Scene Detection
- Content-based scene change detection
- Configurable detection threshold
- Boundary timestamp generation

### Thumbnail Generation
- Configurable size and aspect ratio
- First frame or keyframe selection
- Format: JPEG or PNG

### Subtitle Extraction
- Embedded subtitle track extraction
- Multiple format support (SRT, VTT, etc.)
- Timing information preservation

---

See full documentation at https://github.com/makr-code/ThemisDB

---

**Last Updated:** April 2026  
**Version:** v1.3.0 Phase 2  
**Status:** Production-Ready
