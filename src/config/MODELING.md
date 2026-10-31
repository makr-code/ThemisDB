---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Config Module Model

## Purpose and reader

This document describes the config module's state and data model for developers
changing config resolution, validation, watching, or encrypted storage. It
describes software models, not trained or statistical models.

## Configuration resolution model

A request begins with a caller-provided path. `ConfigPathResolver` normalizes
and validates it, maps known legacy names, and probes the environment overlay
when the active environment is `dev` or `staging`. Production has no overlay.
The resolver may return a cached result; the cache key is the environment
string and normalized path. `setEnvironment()` clears the shared cache when the
selected environment changes. `tryResolve()` represents absence as
`std::nullopt`; `resolve()` reports missing paths with
`ConfigNotFoundException` and attempted-path details.

The resolver's observable counters separate successful resolutions, misses,
legacy fallback, new-path use, unmapped requests, and cache hits or misses. A
path mapping also has metadata used for deprecation reporting. Cache key
composition and candidate precedence are implementation details; focused tests
cover selected examples, not every mapping permutation.

## Validation model

The input is parsed as JSON or YAML into a JSON value, then checked against the
provided schema. `ValidationResult` carries a validity flag, collected errors
and warnings, and config/schema path context. Local `$ref` pointers are walked
within the root schema; remote references are not fetched. The v1.x contract
labels the intended schema support a Draft 7 subset and declares resource
bounds and a maximum collected-error count in
[`../../include/config/config_contract.h`](../../include/config/config_contract.h).
Current implementation enforcement and full keyword coverage have not been
verified; do not rely on the contract alone as proof of enforcement.
Unsupported keywords and unknown formats are not a basis for a security
decision without a test proving enforcement.

## Watcher and reload model

`ConfigFileWatcher` filters filesystem events to YAML, YML, and JSON files
using the platform backend (Linux inotify, Apple kqueue, or Windows directory
notifications). File
events enter a debounced callback path. The path resolver uses that callback to
clear its cache; the watcher does not parse a replacement config, validate it,
or atomically apply settings. Concurrent filesystem changes may collapse into
one notification, so consumers should load and validate the final file state.

## Encrypted-store model

The store maps config keys to blobs containing a key version, IV, ciphertext,
and GCM authentication tag. Reads authenticate and decrypt the full value;
`rotateKey()` re-encrypts all current entries before swapping key and store, so
an encryption/decryption failure before the swap leaves the prior entries in
place. The
serialization model includes the current key material as well as entries;
serialized data is therefore a sensitive credential-bearing artifact.

## Boundaries and limits

The config module owns path/validation support, change signals, metrics/audit
surfaces, and an in-memory encrypted store. It does not own application-level
configuration rollout, consumer authorization, secret-manager integration, or
the behavior of systems that read resolved paths. See
[`ARCHITECTURE.md`](ARCHITECTURE.md), [`TRUST.md`](TRUST.md), and
[`OPERATIONS.md`](OPERATIONS.md) for the corresponding boundaries.
