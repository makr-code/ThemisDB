> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/document/ARCHITECTURE.md -->

# Document Module — Public Header Architecture

**Module Path:** `include/document/`  
**Implementation:** `../../src/document/`  
**Canonical architecture doc:** [`../../src/document/ARCHITECTURE.md`](../../src/document/ARCHITECTURE.md)

---

## 1. Overview

`include/document/` defines the **public document lifecycle management, versioning, schema evolution, encrypted entities, round-trip editing, and XDOMEA connector API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/document/ARCHITECTURE.md`](../../src/document/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Document Lifecycle

| Header | Public Type | Purpose |
|--------|------------|---------|
| `document_lifecycle.h` | `DocumentLifecycle` | Create/update/archive/delete lifecycle |
| `document_manager.h` | `DocumentManager` | Primary document CRUD entry point |
| `document_manager_deprecated.h` | `DocumentManagerDeprecated` | Deprecated document API shim |
| `document_store.h` | `DocumentStore` | Pluggable persistent document store |
| `round_trip_editor.h` | `RoundTripEditor` | Lossless edit-serialise-deserialise round-trip |
### 2.2 Versioning and Evolution

| Header | Public Type | Purpose |
|--------|------------|---------|
| `document_schema_evolution.h` | `DocumentSchemaEvolution` | Backward-compatible schema migration |
| `document_diff_merge.h` | `DocumentDiffMerge` | Three-way diff and merge for documents |
### 2.3 Security and Compliance

| Header | Public Type | Purpose |
|--------|------------|---------|
| `encrypted_entities.h` | `EncryptedEntities` | Field-level encryption for document entities |
### 2.4 Connectors

| Header | Public Type | Purpose |
|--------|------------|---------|
| `xdomea_connector.h` | `XdomeaConnector` | XDOMEA e-government file format connector |

---

## 3. Namespace Layout

All public types reside in the `themis::document` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/document/` expose the **stable public API**; internal types live in `src/document/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
