/**
 * @file llm_wiki_plugin_factory.cpp
 * @brief Shared-library entry point for the LLM Wiki plugin.
 */

#include "wikipedia/llm_wiki_plugin_impl.h"
#include "llm_wiki/llm_wiki_plugin_interface.h"
#include "plugins/plugin_interface.h"

#if !defined(THEMIS_TEST_BUILD) && defined(THEMIS_PLUGIN_EXPORTS)
extern "C" THEMIS_PLUGIN_EXPORT
/**
 * @brief Themisdb llm wiki create.
 * @return Pointer to the result.
 * @details Calls: themis::plugins::llm_wiki::LLMWikiPluginImpl().
 */
themis::plugins::llm_wiki::ILLMWikiPlugin* themisdb_llm_wiki_create() {
    return new themis::plugins::llm_wiki::LLMWikiPluginImpl();
}

extern "C" THEMIS_PLUGIN_EXPORT
/**
 * @brief Themisdb llm wiki destroy.
 * @param[in,out] plugin Input/output parameter.
 * @details Implements themisdb_llm_wiki_destroy without additional internal calls.
 */
void themisdb_llm_wiki_destroy(themis::plugins::llm_wiki::ILLMWikiPlugin* plugin) {
    delete plugin;
}
#endif
