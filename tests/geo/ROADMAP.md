# Geo test roadmap

## Current registration

`tests/geo/CMakeLists.txt` generates
focused test targets and CTest entries from `test_*.cpp`; prefix-matched
`test_geo_*.cpp` files are also considered by a second registration loop.
Source presence and registration do not establish that a test passed.

The current source inventory includes geometry/parsing, CPU/GPU backend,
index/cursor, join/filter, clustering, raster, temporal-spatial, tile, and
server integration suites. Consult each test for concrete coverage and
environment requirements.

## Follow-up coverage

- [ ] Add a focused `GeoFaissKnn` test covering point filtering, original input
  index mapping, search ordering, and CPU/GPU selection. No corresponding
  `test_geo_faiss_knn.cpp` exists in the current test directory. **TODO:** assign
  an implementation issue and owner before scheduling.
- [ ] Record fresh results only after building/running the relevant focused
  target in the intended backend configuration. Do not copy historical results
  into current validation status.

Runtime feature status and implementation plans are maintained in
`src/geo/ROADMAP.md`.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
