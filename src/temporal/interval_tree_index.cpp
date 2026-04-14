/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            interval_tree_index.cpp                            ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 11:38:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     337                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c5ff147e9f  2026-03-20  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Augmented Interval Tree Index Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/interval_tree_index.h"
#include <algorithm>

namespace themisdb {
namespace temporal {

// ============================================================================
// Construction
// ============================================================================

IntervalTreeIndex::IntervalTreeIndex(std::string name)
    : name_(std::move(name)) {}

// ============================================================================
// Static node helpers
// ============================================================================

Timestamp IntervalTreeIndex::nodeMaxEnd(const Node* n) noexcept {
    return n ? n->subtree_max_end : kMinTimestamp;
}

void IntervalTreeIndex::updateSubtreeMax(Node* n) noexcept {
    if (!n) return;
    n->subtree_max_end = n->entry.range.end;
    if (n->left  && n->left->subtree_max_end  > n->subtree_max_end)
        n->subtree_max_end = n->left->subtree_max_end;
    if (n->right && n->right->subtree_max_end > n->subtree_max_end)
        n->subtree_max_end = n->right->subtree_max_end;
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

    updateSubtreeMax(root.get());
    return root;
}

// ============================================================================
// BST remove — exact (key + range) match
// ============================================================================

IntervalTreeIndex::Node*
IntervalTreeIndex::findMin(Node* n) noexcept {
    while (n && n->left) n = n->left.get();
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
    updateSubtreeMax(root.get());
    return root;
}

std::unique_ptr<IntervalTreeIndex::Node>
IntervalTreeIndex::removeNode(std::unique_ptr<Node> root,
                               const std::string& key,
                               const TimeRange& range,
                               size_t& removed_count) {
    if (!root) return nullptr;

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
            updateSubtreeMax(successor.get());
            return successor;
        }
        // Not this node — search both sides for duplicate starts
        root->left  = removeNode(std::move(root->left),  key, range, removed_count);
        root->right = removeNode(std::move(root->right), key, range, removed_count);
    }

    updateSubtreeMax(root.get());
    return root;
}

std::unique_ptr<IntervalTreeIndex::Node>
IntervalTreeIndex::removeKeyNode(std::unique_ptr<Node> root,
                                  const std::string& key,
                                  size_t& removed_count) {
    if (!root) return nullptr;

    root->left  = removeKeyNode(std::move(root->left),  key, removed_count);
    root->right = removeKeyNode(std::move(root->right), key, removed_count);

    if (root->entry.key == key) {
        ++removed_count;
        if (!root->right) return std::move(root->left);
        if (!root->left)  return std::move(root->right);
        std::unique_ptr<Node> successor;
        root->right = detachMin(std::move(root->right), successor);
        successor->left  = std::move(root->left);
        successor->right = std::move(root->right);
        updateSubtreeMax(successor.get());
        return successor;
    }

    updateSubtreeMax(root.get());
    return root;
}

// ============================================================================
// Public mutation
// ============================================================================

void IntervalTreeIndex::insert(const IntervalEntry& entry) {
    std::lock_guard<std::mutex> lk(mutex_);
    root_ = insertNode(std::move(root_), entry);
    ++size_;

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
    std::lock_guard<std::mutex> lk(mutex_);
    size_t removed = 0;
    root_ = removeNode(std::move(root_), key, range, removed);
    size_ -= removed;
    stats_.total_entries -= removed;
    return removed;
}

size_t IntervalTreeIndex::removeKey(const std::string& key) {
    std::lock_guard<std::mutex> lk(mutex_);
    size_t removed = 0;
    root_ = removeKeyNode(std::move(root_), key, removed);
    size_ -= removed;
    stats_.total_entries -= removed;
    return removed;
}

void IntervalTreeIndex::clear() {
    std::lock_guard<std::mutex> lk(mutex_);
    root_.reset();
    size_ = 0;
    stats_ = IntervalTreeStats{};
}

// ============================================================================
// Query helpers (called under lock)
// ============================================================================

void IntervalTreeIndex::collectOverlap(const Node* n,
                                        Timestamp from, Timestamp to,
                                        std::vector<IntervalEntry>& out) const {
    if (!n) return;

    // Pruning: if subtree's max_end <= from, no interval in this subtree
    // can overlap [from, to) because all intervals end at or before from.
    if (nodeMaxEnd(n) <= from) return;

    // Check left subtree first (may contain smaller starts that still overlap)
    collectOverlap(n->left.get(), from, to, out);

    // If this node's start >= to, no node in the right subtree can overlap
    // (right subtree starts are even larger) — prune right entirely.
    if (n->entry.range.start >= to) return;

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
    if (!n) return;
    collectKey(n->left.get(),  key, range, out);
    if (n->entry.key == key) {
        if (!range.has_value() || n->entry.range.overlaps(*range)) {
            out.push_back(n->entry);
        }
    }
    collectKey(n->right.get(), key, range, out);
}

size_t IntervalTreeIndex::treeHeight(const Node* n) noexcept {
    if (!n) return 0;
    return 1 + std::max(treeHeight(n->left.get()),
                        treeHeight(n->right.get()));
}

// ============================================================================
// Public queries
// ============================================================================

std::vector<IntervalEntry>
IntervalTreeIndex::queryPoint(Timestamp t) const {
    std::lock_guard<std::mutex> lk(mutex_);
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
    std::lock_guard<std::mutex> lk(mutex_);
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
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<IntervalEntry> result;
    collectKey(root_.get(), key, range, result);
    return result;
}

// ============================================================================
// Metadata
// ============================================================================

size_t IntervalTreeIndex::size() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return size_;
}

IntervalTreeStats IntervalTreeIndex::stats() const {
    std::lock_guard<std::mutex> lk(mutex_);
    stats_.height = treeHeight(root_.get());
    return stats_;
}

} // namespace temporal
} // namespace themisdb
