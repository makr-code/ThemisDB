# Geo test-suite future work

## Scope

This file tracks candidate additions to
`tests/geo/`. The source's focused test
registration is defined in
`tests/geo/CMakeLists.txt`.

## Evidence-based candidate

- Add a dedicated test for `GeoFaissKnn`. The public API exists in
  `include/geo/geo_faiss_knn.h`, and
  `src/geo/EMBEDDING.md` documents that no
  module test or benchmark currently covers it. Cover point-only input
  filtering, mapping results back to original input indices, query/radius
  behavior, and backend reporting. This is a proposed test, not existing
  coverage or a measured performance target.

**TODO:** Assign an implementation issue/owner and agree on the build variants
before setting a delivery date. Other test gaps should be added only with
specific source/API evidence and an acceptance test.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
