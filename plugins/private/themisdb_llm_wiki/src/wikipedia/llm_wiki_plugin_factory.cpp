/**
 * @file llm_wiki_plugin_factory.cpp
 * @brief Shared-library entry point for the LLM Wiki plugin.
 *
 * Exports `themisdb_llm_wiki_create()` with C linkage so that the
 * ThemisDB `PluginManager` can load the plugin at runtime via `dlopen` /
 * `LoadLibrary` without requiring C++ name-mangling knowledge.
 *
 * @version 0.1.0
 * @note Maturity: 🟡 BETA (Phase 2)
 * @note Edition: enterprise / hyperscaler / military
 */

#include "wikipedia/llm_wiki_plugin_impl.h"
#include "llm_wiki/llm_wiki_plugin_interface.h"
#include "plugins/plugin_interface.h"

#include <memory>

/**
 * @brief Factory entry point for the LLM Wiki plugin shared library.
 *
 * Called by `PluginManager::load()` after `dlopen` / `LoadLibrary`. Returns a
 * `shared_ptr<ILLMWikiPlugin>` whose concrete type is `LLMWikiPluginImpl`.
 *
 * @return Heap-allocated plugin instance ready for `initialize()`.
 */
extern "C" THEMIS_PLUGIN_EXPORT
std::shared_ptr<themis::plugins::llm_wiki::ILLMWikiPlugin>
themisdb_llm_wiki_create() {
    return std::make_shared<themis::plugins::llm_wiki::LLMWikiPluginImpl>();
}
