// Factory registration API for lightweight linkage: concrete LLM
// implementations register factories so consumers can obtain instances
// without linking against the full LLM library at compile/link time.
#pragma once

#include <functional>
#include <memory>

#include "themis/llm/llm_interfaces.h"
#include "themis/llm/llm_plugin_manager.h"
#include "themis/llm/lora_orchestrator_interface.h"

namespace themis {
namespace llm {

using DocsAssistantFactory = std::function<std::shared_ptr<IDocsAssistant>()>;
using EmbeddedLLMFactory = std::function<std::shared_ptr<IEmbeddedLLM>()>;
using ThemisHelpLoRAFactory = std::function<std::shared_ptr<IThemisHelpLoRA>()>;
using LlamaWrapperFactory = std::function<std::shared_ptr<ILlamaWrapper>()>;
using LLMModelAuditLoggerFactory = std::function<std::shared_ptr<ILLMModelAuditLogger>()>;
using LLMPluginManagerFactory = std::function<std::shared_ptr<themis::llm::ILLMPluginManager>()>;
using LoRAOrchestratorFactory = std::function<std::shared_ptr<themis::llm::lora::ILoRAOrchestrator>()>;

/**
 * @brief Register a factory from the full LLM implementation.
 * @param[in] f Input parameter.
 * @details Registration is expected to be performed by the themis_llm module during startup or via an explicit initialization function. Registrations may be overwritten.
 */
void registerDocsAssistantFactory(DocsAssistantFactory f);
/**
 * @brief TBD: Describe registerEmbeddedLLMFactory.
 * @param[in] f Input parameter.
 */
void registerEmbeddedLLMFactory(EmbeddedLLMFactory f);
/**
 * @brief TBD: Describe registerThemisHelpLoRAFactory.
 * @param[in] f Input parameter.
 */
void registerThemisHelpLoRAFactory(ThemisHelpLoRAFactory f);
/**
 * @brief TBD: Describe registerLlamaWrapperFactory.
 * @param[in] f Input parameter.
 */
void registerLlamaWrapperFactory(LlamaWrapperFactory f);
/**
 * @brief TBD: Describe registerLLMModelAuditLoggerFactory.
 * @param[in] f Input parameter.
 */
void registerLLMModelAuditLoggerFactory(LLMModelAuditLoggerFactory f);
/**
 * @brief TBD: Describe registerLLMPluginManagerFactory.
 * @param[in] f Input parameter.
 */
void registerLLMPluginManagerFactory(LLMPluginManagerFactory f);
/**
 * @brief TBD: Describe registerLoRAOrchestratorFactory.
 * @param[in] f Input parameter.
 */
void registerLoRAOrchestratorFactory(LoRAOrchestratorFactory f);

/**
 * @brief Create functions return a shared_ptr or `nullptr` if no factory is registered.
 * @return Return value.
 * @details Consumers must handle absence of implementations at runtime.
 */
std::shared_ptr<IDocsAssistant> createDocsAssistant();
/**
 * @brief TBD: Describe createEmbeddedLLM.
 * @return Return value.
 */
std::shared_ptr<IEmbeddedLLM> createEmbeddedLLM();
/**
 * @brief TBD: Describe createThemisHelpLoRA.
 * @return Return value.
 */
std::shared_ptr<IThemisHelpLoRA> createThemisHelpLoRA();
/**
 * @brief TBD: Describe createLlamaWrapper.
 * @return Return value.
 */
std::shared_ptr<ILlamaWrapper> createLlamaWrapper();
/**
 * @brief TBD: Describe createLLMModelAuditLogger.
 * @return Return value.
 */
std::shared_ptr<ILLMModelAuditLogger> createLLMModelAuditLogger();
/**
 * @brief TBD: Describe createLLMPluginManager.
 * @return Return value.
 */
std::shared_ptr<themis::llm::ILLMPluginManager> createLLMPluginManager();
/**
 * @brief TBD: Describe createLoRAOrchestrator.
 * @return Return value.
 */
std::shared_ptr<themis::llm::lora::ILoRAOrchestrator> createLoRAOrchestrator();

} // namespace llm
} // namespace themis
