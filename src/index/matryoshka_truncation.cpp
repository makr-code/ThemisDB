/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            matryoshka_truncation.cpp                          ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 04:17:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     43                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 05e3e54aa8  2026-03-24  fix(index): address matryoshka_truncation review comments ║
    • 93f695ae42  2026-03-24  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * MatryoshkaTruncation and MatryoshkaTruncatedIndex are implemented
 * entirely as header-only components in:
 *
 *   include/index/matryoshka_truncation.h
 *
 * This .cpp file is intentionally kept empty and is not referenced by
 * the build system. It exists only as a documentation anchor to signal
 * that the matryoshka truncation index is designed to be header-only.
 *
 * If a future change requires non-inline / non-header-only logic for
 * these types, the recommended approach is:
 *   1. Add the corresponding .cpp file to the relevant CMake target
 *      (e.g., themis_core / modular index build), and
 *   2. Move the non-inline definitions from the header into that .cpp.
 *
 * Until such a change is made, this file should not contain any code
 * or #include directives so that the implementation remains purely
 * header-only and cannot silently drift out of CI coverage.
 */
