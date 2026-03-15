#pragma once

#include "plugins/image_analysis_interface.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace image {

class ONNXClipPlugin : public IImageAnalysisBackend {
public:
	ONNXClipPlugin();
	~ONNXClipPlugin() override;

	PluginInfo getInfo() const override;
	bool initialize(const PluginConfig& config, BackendType backend = BackendType::AUTO) override;
	void shutdown() override;
	bool isReady() const override;
	BackendType getBackend() const override;

	EmbeddingResult generateEmbedding(
		const std::vector<uint8_t>& image_data,
		const ImageMetadata* metadata = nullptr
	) override;

	std::vector<EmbeddingResult> generateEmbeddingBatch(
		const std::vector<std::vector<uint8_t>>& images
	) override;

	bool healthCheck() const override;
	nlohmann::json getStatistics() const override;
	void warmup() override;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace image
} // namespace plugins
} // namespace themis
