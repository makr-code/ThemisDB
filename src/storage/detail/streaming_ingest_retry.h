// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "storage/streaming_ingest_manager.h"

#include <cstddef>
#include <vector>

namespace themis::storage::detail {

inline void requeueUnwrittenIngestEvents(
    std::vector<StreamingIngestManager::Event>& pending,
    std::vector<StreamingIngestManager::Event>& failed_batch,
    size_t first_unwritten) {
    std::vector<StreamingIngestManager::Event> retry;
    retry.reserve(failed_batch.size() - first_unwritten + pending.size());
    for (size_t index = first_unwritten; index < failed_batch.size(); ++index) {
        retry.push_back(std::move(failed_batch[index]));
    }
    for (auto& event : pending) {
        retry.push_back(std::move(event));
    }
    pending = std::move(retry);
}

}  // namespace themis::storage::detail
