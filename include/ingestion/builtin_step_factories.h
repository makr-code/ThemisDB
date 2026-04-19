/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            builtin_step_factories.h                           ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-15 18:45:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     70                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • db7df90e31  2026-04-15  feat(ingestion): Google Benchmarks QJ01–QJ11 + SoC/OOP do... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "ingestion/ingestion_step.h"
#include "ingestion/inference_backend.h"
#include "ingestion/format_extractor.h"
#include <memory>
namespace themis {
namespace ingestion {
namespace builtin {

/**
 * @brief Create a `builtin.ner_de` step instance.
 *
 * Factory function that allows test code and runtime bootstrap to obtain a
 * `NerDeStep` without coupling to its class definition (which lives entirely
 * in the .cpp file to avoid polluting the public API surface).
 *
 * @param backend  Optional `ITextGenerationBackend` for LLM-backed NER.
 *                 Pass `nullptr` to use the default `NullTextGenerationBackend`.
 */
std::shared_ptr<IIngestionStep> createNerDeStep(
    std::shared_ptr<ITextGenerationBackend> backend = nullptr);

/**
 * @brief Create a `builtin.llm_extract` step instance.
 *
 * @param backend  Optional `ITextGenerationBackend`.
 *                 Pass `nullptr` to use the default `NullTextGenerationBackend`.
 */
std::shared_ptr<IIngestionStep> createLlmExtractStep(
    std::shared_ptr<ITextGenerationBackend> backend = nullptr);

/**
 * @brief Set the `ITextGenerationBackend` on a step created by the factories above.
 *
 * Both `NerDeStep` and `LlmExtractStep` expose a `setBackend()` member.  This
 * helper casts the base pointer and calls it, enabling tests and the
 * `IngestionManager` to inject a backend after the step has been registered.
 *
 * Returns `false` when the step does not support backend injection (e.g. steps
 * from third-party DLL plugins).
 */
bool setStepBackend(IIngestionStep* step,
                    std::shared_ptr<ITextGenerationBackend> backend);

// ─────────────────────────────────────────────────────────────────────────────
// Format-specific parse steps (use IFormatExtractor via DI)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Create a `builtin.parse_pdf` step.
 *
 * @param extractor  An `IFormatExtractor` that handles "application/pdf".
 *                   Pass `nullptr` to create a disabled step (canHandle()
 *                   will return false; step is silently skipped).
 */
std::shared_ptr<IIngestionStep> createParsePdfStep(
    std::shared_ptr<IFormatExtractor> extractor = nullptr);

/**
 * @brief Create a `builtin.parse_office` step.
 *
 * Handles DOCX, XLSX, PPTX, ODF, and legacy Office formats.
 *
 * @param extractor  An `IFormatExtractor` that handles Office MIME types.
 */
std::shared_ptr<IIngestionStep> createParseOfficeStep(
    std::shared_ptr<IFormatExtractor> extractor = nullptr);

/**
 * @brief Create a `builtin.parse_image` step.
 *
 * Extracts EXIF metadata and (optionally) OCR-derived text.
 *
 * @param extractor  An `IFormatExtractor` that handles image/* MIME types.
 */
std::shared_ptr<IIngestionStep> createParseImageStep(
    std::shared_ptr<IFormatExtractor> extractor = nullptr);

/**
 * @brief Create a `builtin.parse_archive` step.
 *
 * Unpacks ZIP/TAR/… archives to a temporary directory and populates
 * `ExtractionContext::extracted_file_paths` for recursive ingestion.
 *
 * @param extractor  An `IFormatExtractor` that handles archive MIME types.
 */
std::shared_ptr<IIngestionStep> createParseArchiveStep(
    std::shared_ptr<IFormatExtractor> extractor = nullptr);

/**
 * @brief Create a `builtin.parse_audio` step.
 *
 * Transcribes audio to text via the STT (Whisper/FFmpeg) backend.
 * Only active when `THEMIS_ENABLE_VOICE_ASSISTANT` is ON.
 *
 * @param extractor  An `IFormatExtractor` that handles audio/* MIME types.
 */
std::shared_ptr<IIngestionStep> createParseAudioStep(
    std::shared_ptr<IFormatExtractor> extractor = nullptr);

// ─────────────────────────────────────────────────────────────────────────────
// New built-in steps (v1.4.0)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Create a `builtin.decompress` step.
 *
 * Unpacks ZIP / tar / gzip archives without a FormatExtractor dependency.
 * Uses fork/execvp to invoke `unzip` or `tar` from the system PATH.
 * Populates `ExtractionContext::extracted_file_paths` with the paths of
 * the files written to a temporary directory.
 *
 * Config keys (all optional):
 *  - `output_dir`   string   Base directory for extraction (default: /tmp/themis_decompress_XXXXXX)
 *  - `max_depth`    number   Maximum recursive unpack depth (default: 1, i.e. no recursion)
 */
std::shared_ptr<IIngestionStep> createDecompressStep();

/**
 * @brief Create a `builtin.legal_reference_extractor` step.
 *
 * Wraps `AgenticReferenceValidator` as a workflow step.
 * Operates on `ctx.raw_text`; results are stored in:
 *  - `ctx.extra["legal_refs.extracted_count"]`   — total references found
 *  - `ctx.extra["legal_refs.dangling_count"]`    — unresolved references
 *  - `ctx.extra["legal_refs.warnings_json"]`     — JSON array of warning strings
 * Dangling references also produce `ctx.warnings` entries for operator visibility.
 *
 * Config keys (all optional):
 *  - `known_laws`  array of strings   Pre-populated law IDs (e.g. ["BImSchG","StGB"])
 */
std::shared_ptr<IIngestionStep> createLegalReferenceExtractorStep();

/**
 * @brief Create a `builtin.chunk_embed` step.
 *
 * For every `TextChunk` in `ctx.chunks`, computes a dense embedding via @p
 * backend and appends a `VectorRecord` to `ctx.embeddings`.
 *
 * If @p backend is `nullptr`, a `NullEmbeddingBackend` (768-d, zero vectors)
 * is used automatically; `isAvailable()` will be false so no real inference
 * occurs but the step succeeds without error — useful for tests.
 *
 * Config keys (all optional):
 *  - `skip_when_unavailable` bool  default true — if true and backend is
 *    unavailable, the step is a no-op instead of an error.
 *  - `dims`  number  Hint for NullEmbeddingBackend dimensionality (default 768).
 *
 * @param backend  Injectable IEmbeddingBackend; nullptr → NullEmbeddingBackend.
 */
std::shared_ptr<IIngestionStep> createChunkEmbedStep(
    std::shared_ptr<IEmbeddingBackend> backend = nullptr);

} // namespace builtin
} // namespace ingestion
} // namespace themis
