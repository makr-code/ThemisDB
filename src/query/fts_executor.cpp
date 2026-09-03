#include "query/fts_executor.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace themis::query::fts {
namespace {

using Clock = std::chrono::steady_clock;

std::vector<std::string> split(const std::string& value, char delim) {
  std::vector<std::string> out;
  std::stringstream ss(value);
  std::string token;
  while (std::getline(ss, token, delim)) {
    out.push_back(token);
  }
  return out;
}

std::string normalizeTerm(const std::string& input) {
  std::string out;
  out.reserve(input.size());
  for (unsigned char c : input) {
    if (std::isalnum(c)) {
      out.push_back(static_cast<char>(std::tolower(c)));
    }
  }
  return out;
}

std::vector<std::pair<std::string, uint32_t>> tokenize(const std::string& text) {
  std::vector<std::pair<std::string, uint32_t>> tokens;
  std::string current;
  uint32_t position = 0;
  for (unsigned char c : text) {
    if (std::isalnum(c)) {
      current.push_back(static_cast<char>(std::tolower(c)));
      continue;
    }
    if (!current.empty()) {
      tokens.emplace_back(current, position++);
      current.clear();
    }
  }
  if (!current.empty()) {
    tokens.emplace_back(current, position);
  }
  return tokens;
}

class DirectoryFtsIndex final : public FtsIndex {
 public:
  explicit DirectoryFtsIndex(std::filesystem::path index_path)
      : index_path_(std::move(index_path)) {}

  bool initialize() {
    std::error_code ec;
    std::filesystem::create_directories(index_path_, ec);
    if (ec) {
      healthy_ = false;
      return false;
    }
    return loadFromDisk();
  }

  PostingList lookupTerm(const std::string& term) const override {
    std::lock_guard<std::mutex> lock(mu_);
    const auto normalized = normalizeTerm(term);
    auto it = postings_.find(normalized);
    if (it == postings_.end()) {
      return {};
    }
    return it->second;
  }

  DocumentMetadata getDocumentStats(uint64_t doc_id) const override {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = documents_.find(doc_id);
    if (it == documents_.end()) {
      return DocumentMetadata{doc_id, 0U, 0.0f, "en"};
    }
    return it->second;
  }

  IndexStatistics getStatistics() const override {
    std::lock_guard<std::mutex> lock(mu_);
    IndexStatistics stats;
    stats.document_count = static_cast<uint32_t>(documents_.size());
    stats.term_count = static_cast<uint32_t>(postings_.size());
    stats.average_doc_length = averageDocLengthUnsafe();
    stats.language = "en";
    std::error_code ec;
    const auto postings_file = index_path_ / "postings.tsv";
    if (std::filesystem::exists(postings_file, ec)) {
      stats.index_size_bytes += std::filesystem::file_size(postings_file, ec);
    }
    const auto docs_file = index_path_ / "docs.tsv";
    if (std::filesystem::exists(docs_file, ec)) {
      stats.index_size_bytes += std::filesystem::file_size(docs_file, ec);
    }
    return stats;
  }

  bool isHealthy() const override {
    std::lock_guard<std::mutex> lock(mu_);
    return healthy_;
  }

  void addDocuments(
      const std::vector<std::pair<uint64_t, std::string>>& documents) override {
    std::lock_guard<std::mutex> lock(mu_);
    for (const auto& [doc_id, text] : documents) {
      eraseDocumentUnsafe(doc_id);
      const auto tokens = tokenize(text);
      std::unordered_map<std::string, std::vector<uint32_t>> positions_by_term;
      for (const auto& [term, pos] : tokens) {
        if (!term.empty()) {
          positions_by_term[term].push_back(pos);
        }
      }

      float avg_tf = 0.0f;
      for (auto& [term, positions] : positions_by_term) {
        PostingListEntry entry;
        entry.doc_id = doc_id;
        entry.term_freq = static_cast<uint32_t>(positions.size());
        entry.positions = std::move(positions);
        auto& posting_list = postings_[term];
        posting_list.push_back(std::move(entry));
        std::sort(posting_list.begin(), posting_list.end(),
                  [](const PostingListEntry& a, const PostingListEntry& b) {
                    return a.doc_id < b.doc_id;
                  });
        avg_tf += static_cast<float>(posting_list.back().term_freq);
      }
      if (!positions_by_term.empty()) {
        avg_tf /= static_cast<float>(positions_by_term.size());
      }
      documents_[doc_id] = DocumentMetadata{
          doc_id, static_cast<uint32_t>(tokens.size()), avg_tf, "en"};
    }
    persistToDiskUnsafe();
  }

  void removeDocuments(const std::vector<uint64_t>& doc_ids) override {
    std::lock_guard<std::mutex> lock(mu_);
    for (uint64_t doc_id : doc_ids) {
      eraseDocumentUnsafe(doc_id);
    }
    persistToDiskUnsafe();
  }

 private:
  bool loadFromDisk() {
    std::lock_guard<std::mutex> lock(mu_);
    postings_.clear();
    documents_.clear();
    healthy_ = true;

    const auto docs_file = index_path_ / "docs.tsv";
    if (std::filesystem::exists(docs_file)) {
      std::ifstream in(docs_file);
      if (!in.good()) {
        healthy_ = false;
        return false;
      }
      std::string line;
      while (std::getline(in, line)) {
        auto fields = split(line, '\t');
        if (fields.size() < 4) {
          healthy_ = false;
          return false;
        }
        DocumentMetadata md;
        md.doc_id = std::stoull(fields[0]);
        md.length_tokens = static_cast<uint32_t>(std::stoul(fields[1]));
        md.avg_term_freq = std::stof(fields[2]);
        md.language = fields[3];
        documents_[md.doc_id] = std::move(md);
      }
    }

    const auto postings_file = index_path_ / "postings.tsv";
    if (std::filesystem::exists(postings_file)) {
      std::ifstream in(postings_file);
      if (!in.good()) {
        healthy_ = false;
        return false;
      }
      std::string line;
      while (std::getline(in, line)) {
        auto fields = split(line, '\t');
        if (fields.size() < 4) {
          healthy_ = false;
          return false;
        }
        PostingListEntry entry;
        const std::string term = fields[0];
        entry.doc_id = std::stoull(fields[1]);
        entry.term_freq = static_cast<uint32_t>(std::stoul(fields[2]));
        if (!fields[3].empty()) {
          for (const auto& position : split(fields[3], ',')) {
            if (!position.empty()) {
              entry.positions.push_back(static_cast<uint32_t>(std::stoul(position)));
            }
          }
        }
        postings_[term].push_back(std::move(entry));
      }
      for (auto& [_, posting_list] : postings_) {
        std::sort(posting_list.begin(), posting_list.end(),
                  [](const PostingListEntry& a, const PostingListEntry& b) {
                    return a.doc_id < b.doc_id;
                  });
      }
    }
    return healthy_;
  }

  void persistToDiskUnsafe() {
    const auto docs_file = index_path_ / "docs.tsv";
    std::ofstream docs_out(docs_file, std::ios::trunc);
    if (!docs_out.good()) {
      healthy_ = false;
      throw std::runtime_error("failed to persist docs.tsv");
    }
    std::vector<uint64_t> doc_ids;
    doc_ids.reserve(documents_.size());
    for (const auto& [doc_id, _] : documents_) {
      doc_ids.push_back(doc_id);
    }
    std::sort(doc_ids.begin(), doc_ids.end());
    for (uint64_t doc_id : doc_ids) {
      const auto& md = documents_.at(doc_id);
      docs_out << md.doc_id << '\t' << md.length_tokens << '\t' << md.avg_term_freq
               << '\t' << md.language << '\n';
    }

    const auto postings_file = index_path_ / "postings.tsv";
    std::ofstream postings_out(postings_file, std::ios::trunc);
    if (!postings_out.good()) {
      healthy_ = false;
      throw std::runtime_error("failed to persist postings.tsv");
    }
    std::vector<std::string> terms;
    terms.reserve(postings_.size());
    for (const auto& [term, _] : postings_) {
      terms.push_back(term);
    }
    std::sort(terms.begin(), terms.end());
    for (const auto& term : terms) {
      const auto& posting_list = postings_.at(term);
      for (const auto& entry : posting_list) {
        postings_out << term << '\t' << entry.doc_id << '\t' << entry.term_freq << '\t';
        for (size_t i = 0; i < entry.positions.size(); ++i) {
          if (i > 0) {
            postings_out << ',';
          }
          postings_out << entry.positions[i];
        }
        postings_out << '\n';
      }
    }
  }

  float averageDocLengthUnsafe() const {
    if (documents_.empty()) {
      return 0.0f;
    }
    uint64_t total = 0;
    for (const auto& [_, md] : documents_) {
      total += md.length_tokens;
    }
    return static_cast<float>(total) / static_cast<float>(documents_.size());
  }

  void eraseDocumentUnsafe(uint64_t doc_id) {
    documents_.erase(doc_id);
    for (auto it = postings_.begin(); it != postings_.end();) {
      auto& posting_list = it->second;
      posting_list.erase(
          std::remove_if(posting_list.begin(), posting_list.end(),
                         [doc_id](const PostingListEntry& entry) {
                           return entry.doc_id == doc_id;
                         }),
          posting_list.end());
      if (posting_list.empty()) {
        it = postings_.erase(it);
      } else {
        ++it;
      }
    }
  }

  std::filesystem::path index_path_;
  mutable std::mutex mu_;
  bool healthy_{true};
  std::unordered_map<std::string, PostingList> postings_;
  std::unordered_map<uint64_t, DocumentMetadata> documents_;
};

using DocAccumulator = std::unordered_map<std::string, PostingListEntry>;

struct IntermediateDocResult {
  uint64_t doc_id = 0;
  std::unordered_map<std::string, PostingListEntry> per_term;
  std::vector<uint32_t> merged_positions;
};

std::vector<std::string> collectTerms(const SearchNode& node) {
  std::vector<std::string> terms;
  if ((node.type == SearchNodeType::TERM || node.type == SearchNodeType::PHRASE) &&
      !node.term.empty()) {
    terms.push_back(node.term);
  }
  for (const auto& child : node.children) {
    auto child_terms = collectTerms(child);
    terms.insert(terms.end(), child_terms.begin(), child_terms.end());
  }
  return terms;
}

SearchNode annotateNodeForDoc(const SearchNode& node,
                              const IntermediateDocResult& doc,
                              const std::unordered_map<std::string, uint32_t>& dfs,
                              uint32_t doc_length) {
  SearchNode annotated = node;
  annotated.children.clear();
  annotated.children.reserve(node.children.size());
  if (node.type == SearchNodeType::TERM || node.type == SearchNodeType::PHRASE) {
    const std::string normalized = normalizeTerm(node.term);
    auto hit = doc.per_term.find(normalized);
    if (hit != doc.per_term.end()) {
      annotated.document_term_frequency = hit->second.term_freq;
      annotated.document_length_tokens = doc_length;
      auto df_it = dfs.find(normalized);
      if (df_it != dfs.end()) {
        annotated.document_frequency = df_it->second;
      }
    } else {
      annotated.document_term_frequency = 0;
      annotated.document_length_tokens = doc_length;
      annotated.document_frequency = 0;
    }
  }
  for (const auto& child : node.children) {
    annotated.children.push_back(annotateNodeForDoc(child, doc, dfs, doc_length));
  }
  return annotated;
}

bool docMatchesQuery(const SearchNode& node, const IntermediateDocResult& doc) {
  switch (node.type) {
    case SearchNodeType::TERM:
    case SearchNodeType::PHRASE:
      return doc.per_term.find(normalizeTerm(node.term)) != doc.per_term.end();
    case SearchNodeType::AND:
      return std::all_of(node.children.begin(), node.children.end(),
                         [&](const SearchNode& child) { return docMatchesQuery(child, doc); });
    case SearchNodeType::OR:
      return std::any_of(node.children.begin(), node.children.end(),
                         [&](const SearchNode& child) { return docMatchesQuery(child, doc); });
    case SearchNodeType::NOT:
      if (node.children.empty()) {
        return false;
      }
      return !docMatchesQuery(node.children.front(), doc);
  }
  return false;
}

}  // namespace

std::unique_ptr<FtsIndex> FtsIndex::open(const std::string& index_path) {
  if (index_path.empty()) {
    return nullptr;
  }
  auto index = std::make_unique<DirectoryFtsIndex>(std::filesystem::path(index_path));
  if (!index->initialize()) {
    return nullptr;
  }
  return index;
}

FtsExecutor::FtsExecutor(const std::string& index_path)
    : cache_(std::make_unique<IndexCache>()),
      index_(FtsIndex::open(index_path)),
      scorer_(std::make_unique<BM25Scorer>()) {}

FtsExecutor::~FtsExecutor() = default;

Result<std::vector<SearchResult>> FtsExecutor::execute(const SearchNode& query,
                                                       const ExecutionOptions& options) {
  std::shared_lock<std::shared_mutex> lock(index_lock_);
  return traverseAndScore(query, options);
}

Result<std::vector<std::vector<SearchResult>>> FtsExecutor::executeBatch(
    const std::vector<SearchNode>& queries, const ExecutionOptions& options) {
  std::shared_lock<std::shared_mutex> lock(index_lock_);
  if (!index_) {
    return tl::unexpected(FtsError::INDEX_NOT_FOUND);
  }
  if (!index_->isHealthy()) {
    return tl::unexpected(FtsError::INVALID_INDEX_FORMAT);
  }

  std::vector<std::vector<SearchResult>> out;
  out.reserve(queries.size());
  for (const auto& query : queries) {
    auto single = traverseAndScore(query, options);
    if (!single) {
      return tl::unexpected(single.error());
    }
    out.push_back(std::move(single.value()));
  }
  return out;
}

Result<void> FtsExecutor::updateIndex(const IndexUpdateBatch& updates) {
  auto deadline = Clock::now() + std::chrono::milliseconds(200);
  std::unique_lock<std::shared_mutex> lock(index_lock_, std::defer_lock);
  while (!lock.try_lock()) {
    if (Clock::now() >= deadline) {
      return tl::unexpected(FtsError::INDEX_LOCKED);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  if (!index_) {
    return tl::unexpected(FtsError::INDEX_NOT_FOUND);
  }

  try {
    if (!updates.deletions.empty()) {
      index_->removeDocuments(updates.deletions);
    }
    if (!updates.additions.empty()) {
      index_->addDocuments(updates.additions);
    }
    cache_->clear();
  } catch (...) {
    return tl::unexpected(FtsError::INTERNAL_ERROR);
  }
  return {};
}

IndexStatistics FtsExecutor::getStatistics() const {
  std::shared_lock<std::shared_mutex> lock(index_lock_);
  if (!index_) {
    return {};
  }
  auto stats = index_->getStatistics();
  return stats;
}

bool FtsExecutor::isIndexHealthy() const {
  std::shared_lock<std::shared_mutex> lock(index_lock_);
  return index_ && index_->isHealthy();
}

FtsExecutor::CacheStats FtsExecutor::getCacheStats() const {
  CacheStats stats;
  if (!cache_) {
    return stats;
  }
  const auto cache_stats = cache_->getStats();
  stats.hits = cache_stats.hits;
  stats.misses = cache_stats.misses;
  stats.evictions = cache_stats.evictions;
  return stats;
}

Result<std::vector<SearchResult>> FtsExecutor::traverseAndScore(
    const SearchNode& query, const ExecutionOptions& options) {
  metrics_.total_queries.fetch_add(1, std::memory_order_relaxed);

  if (!index_) {
    return tl::unexpected(FtsError::INDEX_NOT_FOUND);
  }
  if (!index_->isHealthy()) {
    return tl::unexpected(FtsError::INVALID_INDEX_FORMAT);
  }

  const auto deadline = Clock::now() + std::max(options.timeout, std::chrono::milliseconds(1));
  auto timed_out = [&deadline]() { return Clock::now() >= deadline; };

  std::unordered_set<std::string> unique_terms;
  for (const auto& term : collectTerms(query)) {
    const auto normalized = normalizeTerm(term);
    if (!normalized.empty()) {
      unique_terms.insert(normalized);
    }
  }
  if (unique_terms.empty()) {
    return std::vector<SearchResult>{};
  }

  std::unordered_map<std::string, PostingList> postings;
  std::unordered_map<std::string, uint32_t> dfs;
  postings.reserve(unique_terms.size());
  dfs.reserve(unique_terms.size());
  for (const auto& term : unique_terms) {
    if (timed_out()) {
      metrics_.total_timeout_queries.fetch_add(1, std::memory_order_relaxed);
      return tl::unexpected(FtsError::EXECUTION_TIMEOUT);
    }
    if (auto cached = cache_->lookup(term); cached.has_value()) {
      dfs[term] = static_cast<uint32_t>(cached->size());
      postings[term] = std::move(*cached);
      continue;
    }
    auto posting_list = index_->lookupTerm(term);
    dfs[term] = static_cast<uint32_t>(posting_list.size());
    cache_->insert(term, PostingList(posting_list));
    postings[term] = std::move(posting_list);
  }

  std::unordered_map<uint64_t, IntermediateDocResult> docs;
  for (const auto& [term, posting_list] : postings) {
    for (const auto& entry : posting_list) {
      auto& doc = docs[entry.doc_id];
      doc.doc_id = entry.doc_id;
      doc.per_term[term] = entry;
      doc.merged_positions.insert(doc.merged_positions.end(), entry.positions.begin(),
                                  entry.positions.end());
    }
  }

  std::vector<SearchResult> out;
  out.reserve(docs.size());
  const auto stats = index_->getStatistics();
  for (const auto& [doc_id, doc] : docs) {
    if (timed_out()) {
      metrics_.total_timeout_queries.fetch_add(1, std::memory_order_relaxed);
      return tl::unexpected(FtsError::EXECUTION_TIMEOUT);
    }
    if (!docMatchesQuery(query, doc)) {
      continue;
    }
    const auto md = index_->getDocumentStats(doc_id);
    SearchNode annotated = annotateNodeForDoc(query, doc, dfs, md.length_tokens);
    const float score = scorer_->compute(doc_id, annotated, stats);
    if (score <= 0.0f) {
      continue;
    }
    SearchResult result;
    result.doc_id = doc_id;
    result.score = score;
    result.term_positions = doc.merged_positions;
    if (options.include_snippets) {
      result.snippet = "doc:" + std::to_string(doc_id);
    }
    out.push_back(std::move(result));
  }

  std::sort(out.begin(), out.end(), [](const SearchResult& a, const SearchResult& b) {
    if (a.score == b.score) {
      return a.doc_id < b.doc_id;
    }
    return a.score > b.score;
  });

  if (out.size() > options.limit) {
    out.resize(options.limit);
  }
  metrics_.total_result_count.fetch_add(out.size(), std::memory_order_relaxed);
  return out;
}

}  // namespace themis::query::fts
