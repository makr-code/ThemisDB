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

void registerDocsAssistantFactory(DocsAssistantFactory f) { g_docs_factory = std::move(f); }
void registerEmbeddedLLMFactory(EmbeddedLLMFactory f) { g_embedded_factory = std::move(f); }
void registerThemisHelpLoRAFactory(ThemisHelpLoRAFactory f) { g_help_lora_factory = std::move(f); }
void registerLlamaWrapperFactory(LlamaWrapperFactory f) { g_llama_factory = std::move(f); }
void registerLLMModelAuditLoggerFactory(LLMModelAuditLoggerFactory f) { g_audit_factory = std::move(f); }
void registerLLMPluginManagerFactory(LLMPluginManagerFactory f) { g_plugin_manager_factory = std::move(f); }
void registerLoRAOrchestratorFactory(LoRAOrchestratorFactory f) { g_lora_orchestrator_factory = std::move(f); }

std::shared_ptr<IDocsAssistant> createDocsAssistant() {
    if(g_docs_factory) {
      return g_docs_factory();
    }
    return nullptr;
}

std::shared_ptr<IEmbeddedLLM> createEmbeddedLLM() {
    if(g_embedded_factory) {
      return g_embedded_factory();
    }
    return nullptr;
}

std::shared_ptr<IThemisHelpLoRA> createThemisHelpLoRA() {
    if(g_help_lora_factory) {
      return g_help_lora_factory();
    }
    return nullptr;
}

std::shared_ptr<ILlamaWrapper> createLlamaWrapper() {
    if(g_llama_factory) {
      return g_llama_factory();
    }
    return nullptr;
}

std::shared_ptr<ILLMModelAuditLogger> createLLMModelAuditLogger() {
    if(g_audit_factory) {
      return g_audit_factory();
    }
    return nullptr;
}

std::shared_ptr<themis::llm::ILLMPluginManager> createLLMPluginManager() {
    if (g_plugin_manager_factory) {
      return g_plugin_manager_factory();
    }
    return nullptr;
}

std::shared_ptr<themis::llm::lora::ILoRAOrchestrator> createLoRAOrchestrator() {
    if (g_lora_orchestrator_factory) {
      return g_lora_orchestrator_factory();
    }
    return nullptr;
}

} // namespace llm
} // namespace themis
