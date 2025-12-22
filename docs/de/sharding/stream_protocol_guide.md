# Stream Protocol Guide

**Version:** v1.3.0 Phase 2  
**Status:** Production-Ready  
**Last Updated:** December 22, 2025

---

## Overview

ThemisDB's Inter-Shard Streaming Protocol provides efficient data transfer between shards during rebalancing, repair, and bootstrap operations. Inspired by Apache Cassandra's streaming architecture, it features chunk-based transfer with checksums, compression, encryption, and automatic retry capabilities.

## Key Features

### Data Integrity
- CRC32 checksums for each chunk
- File-level integrity verification
- Automatic corrupt chunk detection

### Performance
- LZ4/Zstd compression support
- Bandwidth throttling and rate limiting
- Multi-stream parallelization
- Resume on interruption

### Security
- AES-256-GCM encryption
- Secure session management
- Authentication and authorization

### Reliability
- Automatic retry on failure
- Out-of-order chunk handling
- Progress tracking
- Graceful abort handling

## Protocol Operations

### Session Lifecycle
1. **PREPARE** - Exchange metadata and establish session
2. **STREAMING** - Transfer data chunks
3. **COMPLETE** - Verify integrity and finalize
4. **ABORT** - Handle errors and cleanup

### Message Types
- Session Management: PREPARE_REQUEST, PREPARE_ACK, PREPARE_NACK
- Data Transfer: FILE_HEADER, DATA_CHUNK, DATA_CHUNK_ACK, FILE_COMPLETE
- Control: RETRY_REQUEST, ABORT, SESSION_COMPLETE
- Heartbeat: HEARTBEAT, HEARTBEAT_ACK
- Error: ERROR

---

See full documentation at https://github.com/makr-code/ThemisDB

---

**Last Updated:** December 22, 2025  
**Version:** v1.3.0 Phase 2  
**Status:** Production-Ready
