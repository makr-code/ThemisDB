/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            onnx_clip_plugin.h                                 ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:11:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     70                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9d8c5ce371  2026-03-15  Refactor service mesh API handler to use fully qualified ... ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
