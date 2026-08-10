# Blob Storage Plugins

## Status: ✅ Production-Ready (Azure/S3) · 🔧 GCS hardening in progress

External blob storage backend implementations for ThemisDB.

## Available Backends

### Azure Blob Storage ✅
**Path:** `azure/`

**Status:** Production-ready

Integration with Microsoft Azure Blob Storage for external blob storage.

### Amazon S3 ✅
**Path:** `s3/`

**Status:** Production-ready

Integration with Amazon S3 (and S3-compatible services) for external blob storage.

### Google Cloud Storage (GCS) ✅
**Path:** `gcs/`

**Status:** Implemented; emulator/nightly validation pending

Integration with Google Cloud Storage for external blob storage.

## Validation Status

- Server-side encryption support is implemented per cloud backend
- Presigned URL generation is implemented for Azure, S3, and GCS
- Emulator-backed integration coverage is env-gated today (Azurite / MinIO / fake-gcs-server)
- Multi-region routing/replication remains a dedicated follow-up hardening area

## Features

- Store large binary objects externally
- Reduce database storage costs
- Support for multiple cloud providers
- Transparent integration with ThemisDB

## Documentation

For blob storage documentation, see:
- [External Blob Storage Analysis](../../EXTERNAL_BLOB_STORAGE_ANALYSIS.md) (to be moved to docs/)
- [Cloud Blob Backends](../../docs/storage/CLOUD_BLOB_BACKENDS.md)
