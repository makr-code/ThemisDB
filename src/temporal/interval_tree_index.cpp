/**
 * @file interval_tree_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Augmented Interval Tree Index Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/interval_tree_index.h"
#include <algorithm>
#include <cassert>
#include <mutex>

namespace themisdb {
namespace temporal {

// ============================================================================
// Construction
// ============================================================================

IntervalTreeIndex::IntervalTreeIndex(std::string name)
    : name_(std::move(name)) {}

// ============================================================================
// Static node helpers — subtree max-end and AVL height
// ============================================================================

Timestamp IntervalTreeIndex::nodeMaxEnd(const Node* n) noexcept {
    return n ? n->subtree_max_end : kMinTimestamp;
}

void IntervalTreeIndex::updateSubtreeMax(Node* n) noexcept {
    if (!n) {
      return;
    }
    n->subtree_max_end = n->entry.range.end;
    if (n->left  && n->left->subtree_max_end  > n->subtree_max_end)
        n->subtree_max_end = n->left->subtree_max_end;
    if (n->right && n->right->subtree_max_end > n->subtree_max_end)
        n->subtree_max_end = n->right->subtree_max_end;
}

int IntervalTreeIndex::nodeHeight(const Node* n) noexcept {
    return n ? n->height : 0;
}

int IntervalTreeIndex::balanceFactor(const Node* n) noexcept {
    if (!n) {
      return 0;
    }
    return nodeHeight(n->left.get()) - nodeHeight(n->right.get());
}

void IntervalTreeIndex::updateHeight(Node* n) noexcept {
    if (!n) {
      return;
    }
    n->height = 1 + std::max(nodeHeight(n->left.get()),
                              nodeHeight(n->right.get()));
}

// ============================================================================
// AVL rotations — both maintain subtree_max_end and height invariants
// ============================================================================

std::unique_ptr<IntervalTreeIndex::Node>
IntervalTreeIndex::rotateRight(std::unique_ptr<Node> y) {
    auto x     = std::move(y->left);
    y->left    = std::move(x->right);
    updateSubtreeMax(y.get());
    updateHeight(y.get());
    x->right   = std::move(y);
    updateSubtreeMax(x.get());
    updateHeight(x.get());
    return x;
}

std::unique_ptr<IntervalTreeIndex::Node>
IntervalTreeIndex::rotateLeft(std::unique_ptr<Node> x) {
    auto y     = std::move(x->right);
    x->right   = std::move(y->left);
    updateSubtreeMax(x.get());
    updateHeight(x.get());
    y->left    = std::move(x);
    updateSubtreeMax(y.get());
    updateHeight(y.get());
    return y;
}

// Rebalance a node after insert/remove, updating both height and max-end.
std::unique_ptr<IntervalTreeIndex::Node>
IntervalTreeIndex::balance(std::unique_ptr<Node> n) {
    if (!n) {
      return nullptr;
    }
    updateSubtreeMax(n.get());
    updateHeight(n.get());

    const int bf = balanceFactor(n.get());

    if (bf > 1) {
        // Left-heavy
        if (balanceFactor(n->left.get()) < 0) {
            // Left-Right case: double rotation
            n->left = rotateLeft(std::move(n->left));
        }
        return rotateRight(std::move(n));
    }

    if (bf < -1) {
        // Right-heavy
        if (balanceFactor(n->right.get()) > 0) {
            // Right-Left case: double rotation
            n->right = rotateRight(std::move(n->right));
        }
        return rotateLeft(std::move(n));
    }

    return n;
}

// ============================================================================
// BST insert — ordered by range.start, ties broken by range.end then key
// ============================================================================

std::unique_ptr<IntervalTreeIndex::Node>
IntervalTreeIndex::insertNode(std::unique_ptr<Node> root,
                               const IntervalEntry& entry) {
    if (!root) {
        return std::make_unique<Node>(entry);
    }

    const Timestamp pivot = root->entry.range.start;

    if (entry.range.start < pivot) {
        root->left = insertNode(std::move(root->left), entry);
    } else if (entry.range.start > pivot) {
        root->right = insertNode(std::move(root->right), entry);
    } else {
        // Same start: order by end, then key, to keep the BST deterministic
        if (entry.range.end < root->entry.range.end ||
            (entry.range.end == root->entry.range.end && entry.key < root->entry.key)) {
            root->left = insertNode(std::move(root->left), entry);
        } else {
            root->right = insertNode(std::move(root->right), entry);
        }
    }

    return balance(std::move(root));
}

// ============================================================================
// BST remove — exact (key + range) match
// ============================================================================

IntervalTreeIndex::Node*
IntervalTreeIndex::findMin(Node* n) noexcept {
    while (n && n->left) {
      n = n->left.get();
    }
    return n;
}

std::unique_ptr<IntervalTreeIndex::Node>
IntervalTreeIndex::detachMin(std::unique_ptr<Node> root,
                              std::unique_ptr<Node>& out_node) {
    if (!root->left) {
        out_node = std::move(root);
        auto right = std::move(out_node->right);
        out_node->right = nullptr;
        return right;
    }
    root->left = detachMin(std::move(root->left), out_node);
    return balance(std::move(root));
}

std::unique_ptr<IntervalTreeIndex::Node>
IntervalTreeIndex::removeNode(std::unique_ptr<Node> root,
                               const std::string& key,
                               const TimeRange& range,
                               size_t& removed_count) {
    if (!root) {
      return nullptr;
    }

    const Timestamp pivot = root->entry.range.start;

    if (range.start < pivot) {
        root->left = removeNode(std::move(root->left), key, range, removed_count);
    } else if (range.start > pivot) {
        root->right = removeNode(std::move(root->right), key, range, removed_count);
    } else {
        // range.start == pivot: check this node and continue in both children
        // (duplicate starts may live in either subtree)
        if (root->entry.key == key &&
            root->entry.range.start == range.start &&
            root->entry.range.end   == range.end) {
            ++removed_count;
            // Standard BST delete with in-order successor
            if (!root->right) {
                return std::move(root->left);
            }
            if (!root->left) {
                return std::move(root->right);
            }
            std::unique_ptr<Node> successor;
            root->right = detachMin(std::move(root->right), successor);
            successor->left  = std::move(root->left);
            successor->right = std::move(root->right);
            return balance(std::move(successor));
        }
        // Not this node — search both sides for duplicate starts
        root->left  = removeNode(std::move(root->left),  key, range, removed_count);
        root->right = removeNode(std::move(root->right), key, range, removed_count);
    }

    return balance(std::move(root));
}

std::unique_ptr<IntervalTreeIndex::Node>
IntervalTreeIndex::removeKeyNode(std::unique_ptr<Node> root,
                                  const std::string& key,
                                  size_t& removed_count) {
    if (!root) {
      return nullptr;
    }

    root->left  = removeKeyNode(std::move(root->left),  key, removed_count);
    root->right = removeKeyNode(std::move(root->right), key, removed_count);

    if (root->entry.key == key) {
        ++removed_count;
        if (!root->right) {
          return std::move(root->left);
        }
        if (!root->left) {
          return std::move(root->right);
        }
        std::unique_ptr<Node> successor;
        root->right = detachMin(std::move(root->right), successor);
        successor->left  = std::move(root->left);
        successor->right = std::move(root->right);
        return balance(std::move(successor));
    }

    return balance(std::move(root));
}

// ============================================================================
// Public mutation
// ============================================================================

void IntervalTreeIndex::insert(const IntervalEntry& entry) {
    std::unique_lock<std::shared_mutex> lk(mutex_);
    root_ = insertNode(std::move(root_), entry);
    ++size_;

    // Update secondary key index
    key_index_[entry.key].push_back(entry);

    // Maintain stats
    ++stats_.total_entries;
    if (entry.range.start < stats_.min_start)
        stats_.min_start = entry.range.start;
    const Timestamp effective_end =
        (entry.range.end == kMaxTimestamp) ? entry.range.start : entry.range.end;
    if (effective_end > stats_.max_end)
        stats_.max_end = effective_end;
}

size_t IntervalTreeIndex::remove(const std::string& key, const TimeRange& range) {
    std::unique_lock<std::shared_mutex> lk(mutex_);
    size_t removed = 0;
    root_ = removeNode(std::move(root_), key, range, removed);
    size_ -= removed;
    stats_.total_entries -= removed;

    if (removed > 0) {
        auto it = key_index_.find(key);
        if (it != key_index_.end()) {
            auto& bucket = it->second;
            bucket.erase(
                std::remove_if(bucket.begin(), bucket.end(),
                    [&]([[maybe_unused]] const IntervalEntry& e) {
                        return e.range.start == range.start &&
                               e.range.end   == range.end;
                    }),
                bucket.end());
            if (bucket.empty()) {
              key_index_.erase(it);
            }
        }
    }
    return removed;
}

size_t IntervalTreeIndex::removeKey(const std::string& key) {
    std::unique_lock<std::shared_mutex> lk(mutex_);
    size_t removed = 0;
    root_ = removeKeyNode(std::move(root_), key, removed);
    size_ -= removed;
    stats_.total_entries -= removed;
    key_index_.erase(key);
    return removed;
}

size_t IntervalTreeIndex::erase(const std::string& key) {
    // Delegate to removeKey — same implementation, STL-compatible name.
    return removeKey(key);
}

void IntervalTreeIndex::clear() {
    std::unique_lock<std::shared_mutex> lk(mutex_);
    root_.reset();
    size_ = 0;
    key_index_.clear();
    stats_ = IntervalTreeStats{};
}

// ============================================================================
// Query helpers (called under shared lock)
// ============================================================================

void IntervalTreeIndex::collectOverlap(const Node* n,
                                        Timestamp from, Timestamp to,
                                        std::vector<IntervalEntry>& out) const {
    if (!n) {
      return;
    }

    // Pruning: if subtree's max_end <= from, no interval in this subtree
    // can overlap [from, to) because all intervals end at or before from.
    if (nodeMaxEnd(n) <= from) {
      return;
    }

    // Check left subtree first (may contain smaller starts that still overlap)
    collectOverlap(n->left.get(), from, to, out);

    // If this node's start >= to, no node in the right subtree can overlap
    // (right subtree starts are even larger) — prune right entirely.
    if (n->entry.range.start >= to) {
      return;
    }

    // Check this node
    if (n->entry.range.overlaps({from, to})) {
        out.push_back(n->entry);
    }

    // Check right subtree
    collectOverlap(n->right.get(), from, to, out);
}

void IntervalTreeIndex::collectKey(const Node* n,
                                    const std::string& key,
                                    std::optional<TimeRange> range,
                                    std::vector<IntervalEntry>& out) const {
    if (!n) {
      return;
    }
    collectKey(n->left.get(),  key, range, out);
    if (n->entry.key == key) {
        if (!range.has_value() || n->entry.range.overlaps(*range)) {
            out.push_back(n->entry);
        }
    }
    collectKey(n->right.get(), key, range, out);
}

size_t IntervalTreeIndex::treeHeight(const Node* n) noexcept {
    return static_cast<size_t>(nodeHeight(n));
}

// ============================================================================
// Public queries
// ============================================================================

std::vector<IntervalEntry>
IntervalTreeIndex::queryPoint(Timestamp t) const {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    ++stats_.point_queries;
    std::vector<IntervalEntry> result;
    // A point query is a degenerate overlap query: [t, t+1)
    // We use the same routine but check contains() for correctness.
    collectOverlap(root_.get(), t, t + 1, result);
    // Filter to strictly containing entries (collectOverlap may admit t==end)
    result.erase(
        std::remove_if(result.begin(), result.end(),
                       [t](const IntervalEntry& e) {
                           return !e.range.contains(t);
                       }),
        result.end());
    return result;
}

std::vector<IntervalEntry>
IntervalTreeIndex::queryOverlap(Timestamp from, Timestamp to) const {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    ++stats_.overlap_queries;
    std::vector<IntervalEntry> result;
    collectOverlap(root_.get(), from, to, result);
    return result;
}

std::vector<IntervalEntry>
IntervalTreeIndex::queryOverlap(const TimeRange& range) const {
    return queryOverlap(range.start, range.end);
}

std::vector<IntervalEntry>
IntervalTreeIndex::queryKey(const std::string& key,
                             std::optional<TimeRange> range) const {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    const auto it = key_index_.find(key);
    if (it == key_index_.end()) return {};

    std::vector<IntervalEntry> result = {};

    result.reserve(it->second.size());
    for (const auto& entry : it->second) {
        if (!range.has_value() || entry.range.overlaps(*range)) {
            result.push_back(entry);
        }
    }
    return result;
}

// ============================================================================
// Metadata
// ============================================================================

size_t IntervalTreeIndex::size() const noexcept {
    return size_.load(std::memory_order_relaxed);
}

IntervalTreeStats IntervalTreeIndex::stats() const {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    stats_.height = treeHeight(root_.get());
    return stats_;
}

} // namespace temporal
} // namespace themisdb
