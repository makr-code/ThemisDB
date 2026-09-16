# Runbook: Geo Engine Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB `geo` engine module. Covers the
five most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Scenario 1 — Backend Fallback

**Log pattern:** `[GEO:BackendFallback]`

**Symptoms**
- The primary geo backend (GDAL, GEOS, GPU) becomes unavailable and the engine
  falls back to the secondary or CPU backend.
- Log lines contain `[GEO:BackendFallback]` with backend name and fallback target.
- Spatial query latency increases; GPU utilisation drops.

**Triage**
1. Identify the failed primary backend from the log line.
2. Check backend health: `themis_admin geo backend status`.
3. Verify that GPU drivers and GDAL/GEOS libraries are loaded and responsive.
4. Review recent system or driver updates that may have destabilised the primary backend.

**Remediation**
1. Restart the failing primary backend: `themis_admin geo backend restart <name>`.
2. If the GPU backend is unavailable, enable CPU-only mode temporarily:
   set `geo.force_cpu: true` in the runtime config.
3. Validate that GDAL/GEOS library versions are compatible with the running binary.
4. File a hardware ticket if the GPU backend does not recover after restart.

**Escalation**
If the primary backend does not recover within 10 minutes of restart, escalate
to the infrastructure on-call with backend health logs and driver version info.

---

## Scenario 2 — Geometry Validation Failed

**Log pattern:** `[GEO:GeometryValidationFailed]`

**Symptoms**
- Incoming geometry objects fail validation checks (invalid WKB/WKT, self-intersecting
  polygons, out-of-range coordinates).
- Log lines contain `[GEO:GeometryValidationFailed]` with geometry ID and violation type.
- Spatial insert/update operations are rejected.

**Triage**
1. Identify the failing geometry ID and violation type from the log.
2. Inspect the raw geometry payload for obvious malformation.
3. Check the configured validation strictness level: `geo.validation.strict_mode`.
4. Determine the upstream data source that produced the invalid geometry.

**Remediation**
1. Correct the geometry at the source and resubmit.
2. If the validation failure is a false positive due to overly strict tolerance, adjust:
   set `geo.validation.coordinate_precision` and `geo.validation.self_intersect_tolerance`.
3. Enable auto-repair for common geometry issues: set `geo.validation.auto_repair: true`.
4. Quarantine the failing geometry for offline repair: `themis_admin geo geometry quarantine <id>`.

**Escalation**
Widespread validation failures suggest a data pipeline issue — escalate to the
data ingestion team with a sample of failing geometries and the upstream source
identifier.

---

## Scenario 3 — Raster Error

**Log pattern:** `[GEO:RasterError]`

**Symptoms**
- Raster processing operations fail during read, write, or transformation.
- Log lines contain `[GEO:RasterError]` with raster ID, operation, and error code.
- Raster query API returns errors; map tile generation stalls.

**Triage**
1. Identify the failing raster ID and operation from the log.
2. Verify that the raster source file is accessible and not corrupted.
3. Check disk space on the raster cache volume.
4. Confirm that GDAL raster drivers are loaded for the raster format in use.

**Remediation**
1. Reload the raster source from the origin: `themis_admin geo raster reload <id>`.
2. If the raster file is corrupted, restore from backup.
3. Clear the raster cache if it contains stale or corrupted data:
   `themis_admin geo raster cache clear`.
4. Increase the raster processing timeout for large tiles:
   set `geo.raster.processing_timeout_ms`.

**Escalation**
Persistent raster errors after cache clear and file restore indicate a GDAL
driver compatibility issue — escalate to the platform team with the raster
format, GDAL version, and the full error stack.

---

## Scenario 4 — Precision Drift

**Log pattern:** `[GEO:PrecisionDrift]`

**Symptoms**
- Spatial calculations produce results that deviate from expected precision bounds.
- Log lines contain `[GEO:PrecisionDrift]` with operation ID and measured drift value.
- Coordinate comparison or spatial join operations produce unexpected mismatches.

**Triage**
1. Identify the operation and coordinate system involved from the log.
2. Check the configured precision mode: `themis_admin geo precision status`.
3. Verify the coordinate reference system (CRS) configuration is consistent.
4. Look for recent changes to precision mode or CRS configuration.

**Remediation**
1. Reset precision mode to the validated configuration:
   `themis_admin geo precision reset --mode HIGH`.
2. Re-validate affected spatial calculations against known reference points.
3. If drift is caused by CRS mismatch, force a CRS re-projection:
   `themis_admin geo crs reproject <collection_id>`.
4. Rebuild affected spatial indexes: `themis_admin geo index rebuild <collection_id>`.

**Escalation**
Precision drift that affects production query results requires immediate
escalation to the geo module owner with operation IDs, drift measurements,
and CRS configuration details.

---

## Scenario 5 — Spatial Index Corruption

**Log pattern:** `[GEO:BackendFallback]` + `[GEO:GeometryValidationFailed]` (combined)

**Symptoms**
- Spatial queries return inconsistent or missing results.
- Both backend fallback and geometry validation errors appear together.
- R-tree or spatial index queries produce anomalous bounding box results.

**Triage**
1. Check spatial index health: `themis_admin geo index status`.
2. Run an index integrity verification: `themis_admin geo index verify <collection_id>`.
3. Correlate the anomaly window with recent bulk insert or delete operations.
4. Check for disk I/O errors that may have interrupted an index write.

**Remediation**
1. Rebuild the spatial index: `themis_admin geo index rebuild <collection_id>`.
2. Verify geometry integrity after rebuild: `themis_admin geo geometry verify <collection_id>`.
3. If disk I/O errors are found, repair the underlying storage before rebuild.
4. Switch to the fallback backend for queries while the primary index is rebuilt.

**Escalation**
Index corruption requiring more than one rebuild attempt indicates a storage
reliability problem — escalate to the storage infrastructure team with I/O
error logs, disk health status, and the index rebuild output.

---

## Reference

| Log Pattern                        | Severity | SLO Impact | Owner |
|------------------------------------|----------|------------|-------|
| `[GEO:BackendFallback]`            | High     | Partial    | geo   |
| `[GEO:GeometryValidationFailed]`   | Medium   | No         | geo   |
| `[GEO:RasterError]`                | Medium   | Partial    | geo   |
| `[GEO:PrecisionDrift]`             | High     | Yes        | geo   |

---

*Wave D operability deliverable — see `src/geo/ROADMAP.md`.*
