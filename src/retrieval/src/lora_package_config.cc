/**
 * @file lora_package_config.cc
 * @brief ThemisDB configuration-path integration for the retrieval artifact store.
 */

#include "lora_package.h"

#include "config/config_path_resolver.h"

namespace themis::retrieval {

LoRAManifestStore::RuntimeConfig
LoRAManifestStore::RuntimeConfig::loadFromThemisConfig() {
    const auto resolved_path =
        themis::config::ConfigPathResolver::resolve("config/ai_ml/retrieval.yaml");
    return loadFromYaml(resolved_path);
}

std::unique_ptr<LoRAManifestStore> LoRAManifestStore::openFromThemisConfig() {
    return std::make_unique<LoRAManifestStore>(
        RuntimeConfig::loadFromThemisConfig());
}

}  // namespace themis::retrieval
