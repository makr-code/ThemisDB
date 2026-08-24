/**
 * @file llm_wiki_plugin_factory.cpp
 * @brief Shared-library entry point for the LLM Wiki plugin.
 */

#include "wikipedia/llm_wiki_plugin_impl.h"
#include "llm_wiki/llm_wiki_plugin_interface.h"
#include "plugins/plugin_interface.h"

#include <memory>

extern "C" THEMIS_PLUGIN_EXPORT
std::shared_ptr<themis::plugins::llm_wiki::ILLMWikiPlugin>
themisdb_llm_wiki_create() {
    return std::make_shared<themis::plugins::llm_wiki::LLMWikiPluginImpl>();
}
