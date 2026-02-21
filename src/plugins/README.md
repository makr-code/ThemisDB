# Plugins Module

Plugin system infrastructure for ThemisDB.

## Module Purpose

Implements the plugin system infrastructure for ThemisDB, providing dynamic plugin loading, secure plugin execution with manifest validation and signing, and plugin lifecycle management.

## Subsystem Scope

**In scope:** Dynamic shared library loading, plugin manifest validation, Ed25519 signing/verification, plugin lifecycle (register/initialize/execute/shutdown), capability-based permissions.

**Out of scope:** Plugin business logic (in individual plugin packages), WASM sandboxing (planned), plugin dependency registry (planned).

## Relevant Interfaces

- `plugin_loader.cpp` — dynamic library loading and registration
- `plugin_api.cpp` — plugin API implementation
- `manifest_validator.cpp` — manifest schema validation
- `plugin_signer.cpp` — signing/verification

## Current Delivery Status

**Maturity:** 🔴 Alpha — Core plugin loading and manifest validation operational; WASM sandbox and Ed25519 signing in progress.

## Components

- Plugin loader
- Plugin API implementation
- Plugin lifecycle management
- Plugin security and signing

## Features

- Dynamic plugin loading
- Secure plugin execution
- Plugin manifest validation
- Plugin signing and verification

## Documentation

For plugin documentation, see:
- [Plugin Security](../../docs/plugins/PLUGIN_SECURITY.md)
- [Plugin Migration](../../docs/plugins/PLUGIN_MIGRATION.md)
- [Manifest Signatures](../../docs/plugins/MANIFEST_SIGNATURES.md)
- [Plugin Signer Tool](../../tools/plugin_signer/)
