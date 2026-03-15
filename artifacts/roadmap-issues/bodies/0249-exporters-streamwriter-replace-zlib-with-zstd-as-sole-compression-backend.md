### Context

This issue implements the roadmap item '`StreamWriter`: Replace zlib with ZSTD as Sole Compression Backend' for the exporters domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v1.8.0.

Primary detail section: `StreamWriter`: Replace zlib with ZSTD as Sole Compression Backend

### Goal

Deliver the scoped changes for `StreamWriter`: Replace zlib with ZSTD as Sole Compression Backend in src/exporters/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `StreamWriter`: Replace zlib with ZSTD as Sole Compression Backend
**Priority:** Low
**Target Version:** v1.8.0

`stream_writer.cpp` includes both `zlib.h` (line 27) and `zstd.h` (line 29), maintaining two separate compression code paths (gzip via `deflateInit2`, and zstd). The zlib path adds a dependency and binary size overhead. The module already uses `utils/zstd_codec.h` in other paths.

**Implementation Notes:**
- `[x]` Remove zlib/gzip compression path from `StreamWriter`; replace gzip-format output with zstd-compressed output using `ZSTD_createCStream` (already present at line 172).
- `[x]` Offer a `ZSTD_MAGICNUMBER`-prefixed output mode that most data pipeline tools can ingest directly; for tools requiring gzip, document the `pigz` / `zstd -d | gzip` conversion path.
- `[x]` Remove `<zlib.h>` include and the associated `z_stream` compression state path; reduces binary size and maintenance surface.

---

### Acceptance Criteria

- [x] Remove zlib/gzip compression path from `StreamWriter`; replace gzip-format output with zstd-compressed output using `ZSTD_createCStream` (already present at line 172).
- [x] Offer a `ZSTD_MAGICNUMBER`-prefixed output mode that most data pipeline tools can ingest directly; for tools requiring gzip, document the `pigz` / `zstd -d | gzip` conversion path.
- [x] Remove `<zlib.h>` include and the associated `z_stream` compression state path; reduces binary size and maintenance surface.

### Relationships

- Roadmap row: #249 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/exporters/FUTURE_ENHANCEMENTS.md#streamwriter-replace-zlib-with-zstd-as-sole-compression-backend
- Source key: roadmap:249:exporters:v1.8.0:streamwriter-replace-zlib-with-zstd-as-sole-compression-backend

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:249:exporters:v1.8.0:streamwriter-replace-zlib-with-zstd-as-sole-compression-backend -->
<!-- roadmap-ref: row=249;module=exporters;target=v1.8.0 -->
<!-- roadmap-detail: src/exporters/FUTURE_ENHANCEMENTS.md#streamwriter-replace-zlib-with-zstd-as-sole-compression-backend -->
