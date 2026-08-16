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

// Register a factory from the full LLM implementation. Registration is
// expected to be performed by the themis_llm module during startup or via
// an explicit initialization function. Registrations may be overwritten.
void registerDocsAssistantFactory(DocsAssistantFactory f);
void registerEmbeddedLLMFactory(EmbeddedLLMFactory f);
void registerThemisHelpLoRAFactory(ThemisHelpLoRAFactory f);
void registerLlamaWrapperFactory(LlamaWrapperFactory f);
void registerLLMModelAuditLoggerFactory(LLMModelAuditLoggerFactory f);
void registerLLMPluginManagerFactory(LLMPluginManagerFactory f);
void registerLoRAOrchestratorFactory(LoRAOrchestratorFactory f);

// Create functions return a shared_ptr or `nullptr` if no factory is
// registered. Consumers must handle absence of implementations at runtime.
std::shared_ptr<IDocsAssistant> createDocsAssistant();
std::shared_ptr<IEmbeddedLLM> createEmbeddedLLM();
std::shared_ptr<IThemisHelpLoRA> createThemisHelpLoRA();
std::shared_ptr<ILlamaWrapper> createLlamaWrapper();
std::shared_ptr<ILLMModelAuditLogger> createLLMModelAuditLogger();
std::shared_ptr<themis::llm::ILLMPluginManager> createLLMPluginManager();
std::shared_ptr<themis::llm::lora::ILoRAOrchestrator> createLoRAOrchestrator();

} // namespace llm
} // namespace themis
