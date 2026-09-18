#include "themis/llm/llm_factory.h"
#include "themis/llm/llm_plugin_manager.h"
#include "themis/llm/lora_orchestrator_interface.h"

namespace themis {
namespace llm {

static DocsAssistantFactory g_docs_factory = nullptr;
static EmbeddedLLMFactory g_embedded_factory = nullptr;
static ThemisHelpLoRAFactory g_help_lora_factory = nullptr;
static LlamaWrapperFactory g_llama_factory = nullptr;
static LLMModelAuditLoggerFactory g_audit_factory = nullptr;
static LLMPluginManagerFactory g_plugin_manager_factory = nullptr;
static LoRAOrchestratorFactory g_lora_orchestrator_factory = nullptr;

/**
 * @brief Register Docs Assistant Factory.
 * @param[in] f Input parameter.
 * @details Calls: std::move().
 */
void registerDocsAssistantFactory(DocsAssistantFactory f) { g_docs_factory = std::move(f); }
/**
 * @brief Register Embedded LLMFactory.
 * @param[in] f Input parameter.
 * @details Calls: std::move().
 */
void registerEmbeddedLLMFactory(EmbeddedLLMFactory f) { g_embedded_factory = std::move(f); }
/**
 * @brief Register Themis Help Lo RAFactory.
 * @param[in] f Input parameter.
 * @details Calls: std::move().
 */
void registerThemisHelpLoRAFactory(ThemisHelpLoRAFactory f) { g_help_lora_factory = std::move(f); }
/**
 * @brief Register Llama Wrapper Factory.
 * @param[in] f Input parameter.
 * @details Calls: std::move().
 */
void registerLlamaWrapperFactory(LlamaWrapperFactory f) { g_llama_factory = std::move(f); }
/**
 * @brief Register LLMModel Audit Logger Factory.
 * @param[in] f Input parameter.
 * @details Calls: std::move().
 */
void registerLLMModelAuditLoggerFactory(LLMModelAuditLoggerFactory f) { g_audit_factory = std::move(f); }
/**
 * @brief Register LLMPlugin Manager Factory.
 * @param[in] f Input parameter.
 * @details Calls: std::move().
 */
void registerLLMPluginManagerFactory(LLMPluginManagerFactory f) { g_plugin_manager_factory = std::move(f); }
/**
 * @brief Register Lo RAOrchestrator Factory.
 * @param[in] f Input parameter.
 * @details Calls: std::move().
 */
void registerLoRAOrchestratorFactory(LoRAOrchestratorFactory f) { g_lora_orchestrator_factory = std::move(f); }

/**
 * @brief Create Docs Assistant.
 * @return Return value.
 * @details Calls: g_docs_factory().
 */
std::shared_ptr<IDocsAssistant> createDocsAssistant() {
    if(g_docs_factory) {
      return g_docs_factory();
    }
    return nullptr;
}

/**
 * @brief Create Embedded LLM.
 * @return Return value.
 * @details Calls: g_embedded_factory().
 */
std::shared_ptr<IEmbeddedLLM> createEmbeddedLLM() {
    if(g_embedded_factory) {
      return g_embedded_factory();
    }
    return nullptr;
}

/**
 * @brief Create Themis Help Lo RA.
 * @return Return value.
 * @details Calls: g_help_lora_factory().
 */
std::shared_ptr<IThemisHelpLoRA> createThemisHelpLoRA() {
    if(g_help_lora_factory) {
      return g_help_lora_factory();
    }
    return nullptr;
}

/**
 * @brief Create Llama Wrapper.
 * @return Return value.
 * @details Calls: g_llama_factory().
 */
std::shared_ptr<ILlamaWrapper> createLlamaWrapper() {
    if(g_llama_factory) {
      return g_llama_factory();
    }
    return nullptr;
}

/**
 * @brief Create LLMModel Audit Logger.
 * @return Return value.
 * @details Calls: g_audit_factory().
 */
std::shared_ptr<ILLMModelAuditLogger> createLLMModelAuditLogger() {
    if(g_audit_factory) {
      return g_audit_factory();
    }
    return nullptr;
}

/**
 * @brief Create LLMPlugin Manager.
 * @return Return value.
 * @details Calls: g_plugin_manager_factory().
 */
std::shared_ptr<themis::llm::ILLMPluginManager> createLLMPluginManager() {
    if (g_plugin_manager_factory) {
      return g_plugin_manager_factory();
    }
    return nullptr;
}

/**
 * @brief Create Lo RAOrchestrator.
 * @return Return value.
 * @details Calls: g_lora_orchestrator_factory().
 */
std::shared_ptr<themis::llm::lora::ILoRAOrchestrator> createLoRAOrchestrator() {
    if (g_lora_orchestrator_factory) {
      return g_lora_orchestrator_factory();
    }
    return nullptr;
}

} // namespace llm
} // namespace themis
