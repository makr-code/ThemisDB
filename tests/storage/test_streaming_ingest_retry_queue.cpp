// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "storage/detail/streaming_ingest_retry.h"

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

}  // namespace
