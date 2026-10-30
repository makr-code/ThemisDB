---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
Source_Level: "L1"
SOT_Domain: "security"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Geo trust boundaries

## Purpose and scope

This page makes the data and component trust boundaries of geo explicit for
developers and operators. The module threat summary and control inventory are
in [SECURITY.md](./SECURITY.md); this page focuses on how those boundaries
intersect with actual APIs and integration ownership.

## Trust boundaries

| Boundary | Trust assumption and control owner |
|---|---|
| Request payload → geo parser/validator | Treat GeoJSON, coordinates, geometry nesting, and query thresholds as untrusted. Parser and validator paths must reject malformed values before spatial execution; validate the concrete path, not just a helper declaration. |
| Geo query policy → server authorization | `SpatialQueryPolicy::allowed_collections` is advisory and empty means open. The geo module does not grant collection access; the server boundary must enforce the principal's authorization. |
| Geo process → GPU/CPU backend | A compiled or discovered device is a compute target, not a security principal. Both backends receive geometry-derived data. Keep device/runtime access controlled by deployment policy and preserve CPU fallback/explicit failure semantics. |
| Geo extension interface → backend implementation | `ISpatialComputeBackend` and the registry define an extension contract, not a sandbox or plugin-authentication mechanism. Treat registered backend code and the loader that supplies it as trusted application components. |
| Diagnostics → logs/tickets | Incident descriptions and remediation strings are retained and formatted as supplied. Callers must avoid secrets, raw customer geometry, and identifiers that are not approved for the destination. |
| Feature keys → index/storage owner | Geo index keys are caller-supplied feature identifiers, not cryptographic keys. Geo does not own key generation, storage-key rotation, or secret retrieval; those responsibilities stay with the owning service and its storage/security components. Protect feature identifiers as application data when they can identify a customer or record. |

## Validation and limit interpretation

The public contract in
[`geo_api_contract.h`](../../include/geo/geo_api_contract.h) declares
WGS84 bounds, structural geometry checks, a 10,000,000-coordinate contract
limit, collection depth 16, and index/join/query limits. Separately,
[`GeoValidator`](../../include/utils/geo/validator.h) declares a 10 MiB
geometry-size limit and a 100,000-coordinate helper limit. These constants
belong to different interfaces. Do not describe either as a universal
deployment guard until the request path is traced to the validator that is
actually called.

For `GeoJSONGeometry`, the implementation rejects non-finite values, applies
WGS84 ranges when the geometry is marked WGS84, checks minimum line/ring
positions and ring closure, and checks polygon ring orientation. Parsing,
validation, and downstream query use are separate steps; callers must not
assume a geometry object has been validated merely because it was constructed.
The geometry model names CRS values but does not itself transform coordinates.

`SpatialQueryPolicy` has another important limit: its collection list is
advisory and an empty list permits all collections under that policy object.
Only the server/query integration can make the principal-level decision. Do
not use the policy as a substitute for authentication or authorization, and
do not assume its resource fields are effective unless the execution path
consumes them.

## Backend and extension review

- Confirm the deployed geo CUDA/HIP build selection, device capability report,
  and runtime backend before assigning data to a GPU path.
- Keep CPU fallback available for supported operations; fallback does not
  imply that both paths have identical diagnostics or that the GPU was
  exercised.
- `CudaBuffer`/`HipBuffer` guards provide RAII for allocations that use those
  wrappers, but not every kernel path is covered: for example,
  `gpu_backend_cuda.cu` contains direct `cudaMalloc`/`cudaFree` calls. Review
  the concrete path before asserting deterministic cleanup. Neither RAII nor
  fallback establishes zeroization, encryption, or isolation guarantees.
- Review custom backend registration and dynamic loading in the owning build
  and plugin integration. The geo interface alone documents no code
  authentication or process boundary.

## Incident handling

Capture the error code, backend/device reports, relevant redacted request
shape, and diagnostics before restart or clearing in-memory history. Restrict
raw geometry and identifiers to approved incident channels. Report suspected
security vulnerabilities through project-level
[SECURITY.md](../../SECURITY.md); include the exact path, input shape,
observable impact, and a minimized reproducer that contains no customer data.

## Sourcecode verification

- Contract limits and error model:
  [`geo_api_contract.h`](../../include/geo/geo_api_contract.h).
- Standalone validation bounds:
  [`validator.h`](../../include/utils/geo/validator.h).
- Policy boundary:
  [`geo_policy.h`](../../include/geo/geo_policy.h),
  [`geo_policy.cpp`](./geo_policy.cpp), and
  [`spatial_api_handler.cpp`](../../src/server/spatial_api_handler.cpp).
- Feature identifier ownership:
  [`rtree_cursor.h`](../../include/geo/rtree_cursor.h) and
  [`geo_rtree.h`](../../include/geo/geo_rtree.h).
- Geometry checks:
  [`geo_json_geometry.cpp`](./geo_json_geometry.cpp).
- Backend and diagnostics interfaces:
  [`spatial_backend.h`](../../include/geo/spatial_backend.h) and
  [`geo_operator_diagnostics.cpp`](./geo_operator_diagnostics.cpp).
- GPU allocation paths:
  [`gpu_buffer_guard.h`](../../include/geo/gpu_buffer_guard.h) and
  [`gpu_backend_cuda.cu`](./gpu_backend_cuda.cu).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
