# GIUF Architecture

## Overview

GIUF is a product-agnostic install/update/uninstall framework for desktop software.

Core goals:

- Cross-platform runtime for Windows, Linux, macOS
- External signed configuration model
- Deterministic workflow engine with rollback
- Security-first artifact delivery

## Layers

1. API/CLI/UI Layer
- CLI reference tool and optional host UI bindings
- User-facing operation dispatch and progress reporting

2. Workflow Engine Layer
- Operation state machine: check, install, update, uninstall
- Retry, cancellation, rollback orchestration

3. Domain Services Layer
- Manifest loading and validation
- Signature verification
- Artifact download and hash verification
- Staging, atomic switch, uninstall orchestration

4. Infrastructure Layer
- Filesystem, process, network, cryptography adapters
- Platform-specific hooks

## Main Components

- AppController
  - operation entrypoint and control flow
- ConfigLoader
  - parse/validate external configuration and recipes
- ReleaseProvider
  - fetch release metadata from configured sources
- ManifestVerifier
  - signature and schema validation
- ArtifactManager
  - download, resume, integrity check, extraction
- InstallEngine
  - install/update transaction with rollback
- UninstallEngine
  - uninstall transaction with preserve policy
- PlatformAdapter
  - OS-specific actions and safety checks

## Runtime Flow

### Install

1. Load signed product configuration
2. Resolve release source and manifest
3. Verify manifest signature and schema
4. Select artifact by os/arch/channel
5. Download artifact (resume/retry)
6. Verify artifact SHA-256
7. Extract into staging
8. Atomic switch to install path
9. Persist installation state
10. Run post-install hooks

### Update

1. Load current installation state
2. Check policy and channel
3. Verify manifest + artifact
4. Stage update payload
5. Preflight: disk, lock, permissions
6. Atomic swap and health check
7. Rollback on failure

### Uninstall

1. Read installation state
2. Stop managed processes
3. Execute uninstall recipe
4. Preserve user data if configured
5. Remove state and registration artifacts

## Configuration Model

Configuration is external and versioned outside the binary payload.

- product config
- channel policy config
- source config
- trust config
- install/uninstall recipes

Security rule: execution-relevant configuration must be signed and validated.

## Data Model

Installation state file contains:

- product_id
- installed_version
- install_path
- installed_files
- previous_version_snapshot
- rollback_metadata
- install_timestamp

## Security Architecture

- TLS-only transport
- Mandatory manifest signature verification
- Mandatory artifact SHA-256 verification
- Downgrade protection by policy
- Audit log for security decisions

## Extensibility

- Additional source providers (S3, internal registry, mirror)
- Additional signature backends
- Product-specific optional plugin hooks
- Optional GUI integration through host adapters

## Non-Goals (MVP)

- Full package-manager replacement
- Kernel-level installer functionality
- Cloud service dependency in core runtime
