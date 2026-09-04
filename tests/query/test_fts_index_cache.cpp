#include <gtest/gtest.h>

#include "query/index_cache.h"

namespace themis::query::fts {
namespace {

PostingList makePostingList(uint64_t doc_id, uint32_t tf) {
  PostingList list;
  PostingListEntry entry;
  entry.doc_id = doc_id;
  entry.term_freq = tf;
  entry.positions = {1U, 2U};
  list.push_back(std::move(entry));
  return list;
}

TEST(FtsIndexCacheTest, LookupMissThenHitUpdatesStats) {
  IndexCache cache;
  EXPECT_FALSE(cache.lookup("missing").has_value());

  EXPECT_TRUE(cache.insert("alpha", makePostingList(1, 2)));
  auto found = cache.lookup("alpha");
  ASSERT_TRUE(found.has_value());
  ASSERT_EQ(found->size(), 1U);
  EXPECT_EQ((*found)[0].doc_id, 1U);

  const auto stats = cache.getStats();
  EXPECT_EQ(stats.hits, 1U);
  EXPECT_EQ(stats.misses, 1U);
}

TEST(FtsIndexCacheTest, EvictsWhenCapacityExceeded) {
  IndexCache::Config config;
  config.max_size_mb = 0;
  config.bloom_filter_size_bits = 256;
  IndexCache cache(config);

  PostingList big_list;
  for (uint64_t i = 0; i < 3000; ++i) {
    PostingListEntry entry;
    entry.doc_id = i + 1;
    entry.term_freq = 1;
    entry.positions = {1U, 2U, 3U, 4U, 5U};
    big_list.push_back(std::move(entry));
  }

  EXPECT_FALSE(cache.insert("too_big", std::move(big_list)));
}

}  // namespace
}  // namespace themis::query::fts
