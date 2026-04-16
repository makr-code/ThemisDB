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

} // namespace builtin
} // namespace ingestion
} // namespace themis
