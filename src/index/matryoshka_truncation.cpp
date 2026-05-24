/*
 * ThemisDB | File: matryoshka_truncation.cpp | Version: 0.0.12
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
