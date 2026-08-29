/**
 * @file llm_wiki_plugin_factory.cpp
 * @brief Shared-library entry point for the LLM Wiki plugin.
 *
 * Exports `themisdb_llm_wiki_create()` with C linkage so that the
 * ThemisDB runtime can load the plugin without C++ name-mangling.
 *
 * @version 0.1.0
 * @note Maturity: 🟡 BETA (Phase 2)
 * @note Edition: enterprise / hyperscaler / military
 */

#include "wikipedia/llm_wiki_plugin_impl.h"
#include "llm_wiki/llm_wiki_plugin_interface.h"
#include "plugins/plugin_interface.h"

extern "C" THEMIS_PLUGIN_EXPORT
    themis::plugins::llm_wiki::ILLMWikiPlugin* themisdb_llm_wiki_create() {
    return new themis::plugins::llm_wiki::LLMWikiPluginImpl();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themisdb_llm_wiki_destroy(themis::plugins::llm_wiki::ILLMWikiPlugin* plugin) {
    delete plugin;
}
