/**
 * @file training_data_iterator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Stub Implementation**: Iterator for training data batches.
 *       Provides efficient sequential or shuffled access to training samples.
 */

#include "llm/training_data_iterator.h"
#include <spdlog/spdlog.h>

namespace themis::llm {

TrainingDataIterator::TrainingDataIterator(
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<exporters::JSONLLLMExporter> exporter)
    : db_(std::move(db)), exporter_(std::move(exporter)) {
    spdlog::debug("TrainingDataIterator initialized (default config)");
}

TrainingDataIterator::TrainingDataIterator(
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<exporters::JSONLLLMExporter> exporter,
    Config config)
    : db_(std::move(db)), exporter_(std::move(exporter)), config_(std::move(config)) {
    spdlog::debug("TrainingDataIterator initialized");
}

}  // namespace themis::llm
