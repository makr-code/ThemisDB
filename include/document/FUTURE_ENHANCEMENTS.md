> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/document/FUTURE_ENHANCEMENTS.md -->

# Document Module — Public Header Future Enhancements

**Module Path:** `include/document/`  
**Canonical implementation enhancements:** [`../../src/document/FUTURE_ENHANCEMENTS.md`](../../src/document/FUTURE_ENHANCEMENTS.md)

---

## Scope

Candidate enhancements to the **public header contract** in `include/document/`. Runtime and benchmark work remains tracked in:

→ [`../../src/document/FUTURE_ENHANCEMENTS.md`](../../src/document/FUTURE_ENHANCEMENTS.md)

---

## Confirmed boundaries

- The headers expose interfaces and some in-memory reference implementations; they do not declare a stable ABI or compatibility matrix.
- The `document_manager_deprecated.h` header is explicitly a deprecated compatibility shim.
- Do not assume a module-layer classification or compatibility promise not stated by the owning canonical policy.

---

## Open decisions (not commitments)

- TODO: decide whether the public API requires a compatibility matrix and standalone compile checks; see [ROADMAP.md](ROADMAP.md).
- TODO: confirm migration guidance and deprecation scope beyond the existing compatibility shim.
- TODO: identify an approved issue and milestone before adding dates or proposed API additions.

No separate public-header enhancement milestone was confirmed during the 2026-10-31 source review. The module-level runtime plan is in [`../../src/document/FUTURE_ENHANCEMENTS.md`](../../src/document/FUTURE_ENHANCEMENTS.md).
