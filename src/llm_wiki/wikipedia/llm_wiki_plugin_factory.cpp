/**
 * @file llm_wiki_plugin_factory.cpp
 * @brief Shared-library entry point for the LLM Wiki plugin.
 */

#include "wikipedia/llm_wiki_plugin_impl.h"
#include "llm_wiki/llm_wiki_plugin_interface.h"
#include "plugins/plugin_interface.h"

#if !defined(THEMIS_TEST_BUILD) && defined(THEMIS_PLUGIN_EXPORTS)
extern "C" THEMIS_PLUGIN_EXPORT
themis::plugins::llm_wiki::ILLMWikiPlugin* themisdb_llm_wiki_create() {
    return new themis::plugins::llm_wiki::LLMWikiPluginImpl();
}

extern "C" THEMIS_PLUGIN_EXPORT
void themisdb_llm_wiki_destroy(themis::plugins::llm_wiki::ILLMWikiPlugin* plugin) {
    delete plugin;
}
#endif
