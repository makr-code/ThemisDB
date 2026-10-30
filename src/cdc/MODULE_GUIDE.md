---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source level: "level1"
SOT domain: "module-behavior"
Canonical source references: "src/cdc/README.md; src/cdc/ARCHITECTURE.md; src/cdc/ROADMAP.md; include/cdc/README.md; tests/cdc/CMakeLists.txt"
Related milestone: "DOC-WEEKLY-2026-44"
---

# CDC module developer guide

## Purpose and boundaries

CDC provides event recording, listing and replay, consumer acknowledgement,
transport integration, and related admin/reliability surfaces. Storage
internals and external pipeline orchestration belong to their owning modules.
For a concise module overview, see [`README.md`](./README.md); the declarations
are indexed in [`include/cdc/README.md`](../../include/cdc/README.md).

## Code map

| Area | Source |
|---|---|
| Event capture, listing, subscription and retention | [`changefeed.cpp`](./changefeed.cpp), [`changefeed_buffer.cpp`](./changefeed_buffer.cpp) |
| Consumer progress and delivery | [`consumer_group.cpp`](./consumer_group.cpp), [`delivery_tracker.cpp`](./delivery_tracker.cpp) |
| Replay and transport | [`ws_transport.cpp`](./ws_transport.cpp), [`kafka_cdc_producer.cpp`](./kafka_cdc_producer.cpp), [`cdc_ws_handler.cpp`](./cdc_ws_handler.cpp) |
| Failure recovery and transactional event relay | [`dead_letter_queue.cpp`](./dead_letter_queue.cpp), [`outbox.cpp`](./outbox.cpp) |
| Administration and integrations | [`cdc_admin.cpp`](./cdc_admin.cpp), [`cdc_materialized_view.cpp`](./cdc_materialized_view.cpp), [`cross_collection_stream.cpp`](./cross_collection_stream.cpp) |

The source tree has focused tests under
[`tests/cdc/`](../../tests/cdc/); CMake discovers `test_*.cpp` files and
registers focused targets in
[`tests/cdc/CMakeLists.txt`](../../tests/cdc/CMakeLists.txt).

## Documentation map

- [`API.md`](./API.md): public C++ interfaces and contract summary.
- [`MODELING.md`](./MODELING.md): event, offset, outbox and DLQ state model.
- [`IMPLEMENTATION.md`](./IMPLEMENTATION.md): runtime paths and change invariants.
- [`EMBEDDING.md`](./EMBEDDING.md): integrating CDC into a host application.
- [`OPERATIONS.md`](./OPERATIONS.md) and [`RUNBOOK.md`](./RUNBOOK.md):
  diagnostics, routine operation and incident triage.
- [`TRUST.md`](./TRUST.md): trust boundaries and security limits.
- [`ROADMAP.md`](./ROADMAP.md): active/planned work; [`CHANGELOG.md`](./CHANGELOG.md):
  completed history.

## Change and review workflow

1. Identify the owning public header and the applicable rule in
   [`cdc_delivery_contract.h`](../../include/cdc/cdc_delivery_contract.h).
2. Trace the implementation and focused test before describing behavior.
   Treat roadmap items as plans unless code and tests show otherwise.
3. Update the relevant topic page, not every page with a repeated copy of the
   same contract. Keep API details in headers/API.md, future work in
   ROADMAP.md/FUTURE_ENHANCEMENTS.md, and completed history in CHANGELOG.md.
4. Build/run the relevant CDC focused test targets. Record environment
   limitations instead of implying a test passed.
5. Check references and ensure the documentation still matches the public
   interface and module-local canonical sources.

## Sourcecode verification

- Scope files: `include/cdc/cdc_delivery_contract.h`, `include/cdc/changefeed.h`,
  `include/cdc/consumer_group.h`, `include/cdc/outbox.h`,
  `src/cdc/changefeed.cpp`, `src/cdc/consumer_group.cpp`,
  `src/cdc/outbox.cpp`, `tests/cdc/CMakeLists.txt`.
- Verified behavior: sequence assignment/list bounds → `changefeed.cpp`;
  durable group offsets and in-memory in-flight records →
  `consumer_group.h`/`consumer_group.cpp`; transaction-taking outbox writer and
  relay state transitions → `outbox.h`/`outbox.cpp`.
- Verified test registration: one focused target is generated per
  `test_*.cpp` in `tests/cdc/CMakeLists.txt`.
- Result: documented behavior is tied to these declarations/implementations;
  open roadmap items remain explicitly marked as plans.

## Source references

- [Module overview](./README.md)
- [Architecture](./ARCHITECTURE.md)
- [Public CDC headers](../../include/cdc/README.md)
- [Delivery contract](../../include/cdc/cdc_delivery_contract.h)
- [CDC focused test registration](../../tests/cdc/CMakeLists.txt)

**Exact model backend not verifiable from final document text alone.**
