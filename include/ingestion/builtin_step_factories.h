/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            builtin_step_factories.h                           ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-15                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "ingestion/ingestion_step.h"
#include "ingestion/inference_backend.h"
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

} // namespace builtin
} // namespace ingestion
} // namespace themis
