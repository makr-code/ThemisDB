// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "storage/detail/streaming_ingest_retry.h"

#include <cstddef>
#include <string>
#include <vector>

namespace {

using themis::StreamingIngestManager;
using themis::storage::detail::requeueUnwrittenIngestEvents;

TEST(StreamingIngestRetryQueue, LaterChunkFailureRetainsSuffixBeforeNewEvents) {
    std::vector<StreamingIngestManager::Event> failed_batch{
        {"committed-1", "v1"},
        {"committed-2", "v2"},
        {"retry-3", "v3"},
        {"retry-4", "v4"},
    };
    std::vector<StreamingIngestManager::Event> pending{
        {"newer-1", "v5"},
        {"newer-2", "v6"},
    };

    // The failed offset is the beginning of chunk 3 after two chunks committed.
    requeueUnwrittenIngestEvents(pending, failed_batch, 2);

    ASSERT_EQ(pending.size(), 4u);
    EXPECT_EQ(pending[0].key, "retry-3");
    EXPECT_EQ(pending[1].key, "retry-4");
    EXPECT_EQ(pending[2].key, "newer-1");
    EXPECT_EQ(pending[3].key, "newer-2");
    EXPECT_EQ(pending[0].value, "v3");
    EXPECT_EQ(pending[3].value, "v6");
}

TEST(StreamingIngestRetryQueue, LastChunkFailureRetainsTheLastEvent) {
    std::vector<StreamingIngestManager::Event> failed_batch{
        {"committed-1", "v1"},
        {"committed-2", "v2"},
        {"retry-3", "v3"},
    };
    std::vector<StreamingIngestManager::Event> pending;

    requeueUnwrittenIngestEvents(pending, failed_batch, 2);

    ASSERT_EQ(pending.size(), 1u);
    EXPECT_EQ(pending.front().key, "retry-3");
    EXPECT_EQ(pending.front().value, "v3");
}

TEST(StreamingIngestRetryQueue, EveryFailureOffsetPreservesSuffixOrder) {
    for (std::size_t batch_size = 1; batch_size <= 128; ++batch_size) {
        for (std::size_t first_unwritten = 0;
             first_unwritten < batch_size; ++first_unwritten) {
            std::vector<StreamingIngestManager::Event> failed_batch;
            failed_batch.reserve(batch_size);
            for (std::size_t index = 0; index < batch_size; ++index) {
                failed_batch.push_back(
                    {"batch-" + std::to_string(index),
                     "value-" + std::to_string(index)});
            }

            std::vector<StreamingIngestManager::Event> pending{
                {"newer-1", "new-value-1"},
                {"newer-2", "new-value-2"},
            };
            requeueUnwrittenIngestEvents(
                pending, failed_batch, first_unwritten);

            ASSERT_EQ(pending.size(),
                      batch_size - first_unwritten + 2);
            for (std::size_t index = first_unwritten;
                 index < batch_size; ++index) {
                const auto retry_index = index - first_unwritten;
                EXPECT_EQ(pending[retry_index].key,
                          "batch-" + std::to_string(index));
                EXPECT_EQ(pending[retry_index].value,
                          "value-" + std::to_string(index));
            }
            EXPECT_EQ(pending[batch_size - first_unwritten].key, "newer-1");
            EXPECT_EQ(pending[batch_size - first_unwritten + 1].key,
                      "newer-2");
        }
    }
}

}  // namespace
