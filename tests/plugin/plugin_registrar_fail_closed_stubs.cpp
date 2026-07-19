#ifdef THEMIS_FAIL_CLOSED_TEST_STUBS

#include "rag/rag_context_assembler.h"
#include "plugins/plugin_manager.h"
#include "llm/llm_plugin_manager.h"
#include "llm/json_schema_converter.h"
#include "whisper/whisper_config.h"
#include "whisper/audio_chunk_reader.h"

// RAGContextAssembler stubs

namespace themis { namespace rag {

RAGContextAssembler::RAGContextAssembler(const RAGContextAssemblerConfig& cfg)
    : config_(cfg) {}

AssembledContext RAGContextAssembler::assemble(
    const std::vector<RetrievedChunk>& ,
    const std::string& ,
    const std::string& ) const { return AssembledContext{}; }

int RAGContextAssembler::computeMaxTokens(
    const ContextWindowBudget& , int ) { return 2048; }

const RAGContextAssemblerConfig& RAGContextAssembler::getConfig() const {
    return config_;
}

}} // namespace themis::rag

// PluginManager HotPlug stubs

namespace themis { namespace plugins {

bool PluginManager::enableHotPlug(const std::string& , const HotPlugConfig& ) { return false; }
void PluginManager::disableHotPlug() {}

}} // namespace themis::plugins

// LLMPluginManager stubs

namespace themis { namespace llm {

void LLMPluginManager::registerPlugin(const std::string& , std::unique_ptr<ILLMPlugin> ) {}
std::optional<ToolCall> JsonSchemaConverter::parseToolCall(const std::string& ) { return std::nullopt; }

}} // namespace themis::llm

// WhisperConfig + WavAudioChunkReader stubs

namespace themis { namespace whisper {

WhisperConfig WhisperConfig::fromJson(const nlohmann::json& ) { return WhisperConfig{}; }
std::vector<float> WavAudioChunkReader::readFile(const std::string& , float& ) { return {}; }
bool WavAudioChunkReader::canRead(const std::string& ) const { return false; }
std::map<std::string, std::string> WavAudioChunkReader::getMetadata(const std::string& ) const { return {}; }

}} // namespace themis::whisper

#endif // THEMIS_FAIL_CLOSED_TEST_STUBS
