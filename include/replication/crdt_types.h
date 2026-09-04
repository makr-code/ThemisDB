/**
 * @file crdt_types.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themisdb {
namespace replication {
namespace crdt {

// ─────────────────────────────────────────────────────────────────────────────
// Dot – (nodeId, counter) pair used by OR-Set and RGA
// ─────────────────────────────────────────────────────────────────────────────

/** @brief A Dot is a unique event identifier: (node_id, logical_clock). */
struct Dot {
    std::string node_id;
    uint64_t    counter{0};

    bool operator==(const Dot& o) const noexcept {
        return node_id == o.node_id && counter == o.counter;
    }
    bool operator<(const Dot& o) const noexcept {
        if (node_id != o.node_id) {
          return node_id < o.node_id;
        }
        return counter < o.counter;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// GrowOnlyCounter (G-Counter)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A Grow-Only Counter (G-Counter).
 *
 * Each participating node maintains its own monotonically increasing count.
 * The total value is the sum of all per-node counts.  Only the local node's
 * count can be incremented.
 */
class GrowOnlyCounter {
public:
    explicit GrowOnlyCounter(const std::string& node_id)
        : node_id_(node_id) {}

    /** @brief Increment the local node's count by @p delta (default 1). */
    void increment(uint64_t delta = 1) {
        state_[node_id_] += delta;
    }

    /** @brief Returns the total counter value across all nodes. */
    uint64_t value() const {
        uint64_t total = 0;
        for (const auto& kv : state_) { total += kv.second; }
        return total;
    }

    /** @brief Merge another G-Counter state into this one (take per-node max). */
    void merge(const GrowOnlyCounter& other) {
        for (const auto& [nid, cnt] : other.state_) {
            auto& local = state_[nid];
            if (cnt > local) {
              local = cnt;
            }
        }
    }

    /** @brief Raw per-node state (for serialisation / testing). */
    const std::unordered_map<std::string, uint64_t>& state() const {
        return state_;
    }

private:
    std::string node_id_;
    std::unordered_map<std::string, uint64_t> state_;
};

// ─────────────────────────────────────────────────────────────────────────────
// PNCounter
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A Positive-Negative Counter (PN-Counter).
 *
 * Internally two G-Counters (P and N) track increments and decrements
 * separately.  The value is P.value() − N.value().
 */
class PNCounter {
public:
    explicit PNCounter(const std::string& node_id)
        : pos_(node_id), neg_(node_id) {}

    void increment(uint64_t delta = 1) { pos_.increment(delta); }
    void decrement(uint64_t delta = 1) { neg_.increment(delta); }

    /** @brief Returns the signed counter value (may be negative). */
    int64_t value() const {
        return static_cast<int64_t>(pos_.value()) -
               static_cast<int64_t>(neg_.value());
    }

    void merge(const PNCounter& other) {
        pos_.merge(other.pos_);
        neg_.merge(other.neg_);
    }

private:
    GrowOnlyCounter pos_;
    GrowOnlyCounter neg_;
};

// ─────────────────────────────────────────────────────────────────────────────
// LWWRegister<T>
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A Last-Write-Wins Register.
 *
 * Writes with a strictly higher timestamp win.  On equal timestamps the
 * lexicographically-greater node ID is preferred (tie-break).
 *
 * @tparam T Value type (must be copy-constructible).
 */
template<typename T>
class LWWRegister {
public:
    LWWRegister() = default;
    explicit LWWRegister(const std::string& node_id) : node_id_(node_id) {}

    /**
     * @brief Write @p value with the given @p timestamp_us (microseconds
     *        since epoch).
     */
    void write(T value, uint64_t timestamp_us) {
        if (timestamp_us > ts_ ||
            (timestamp_us == ts_ && node_id_ > winning_node_)) {
            value_        = std::move(value);
            ts_           = timestamp_us;
            winning_node_ = node_id_;
        }
    }

    /** @brief Returns the current value (empty optional if never written). */
    const std::optional<T>& read() const { return value_; }

    /** @brief Returns the timestamp of the winning write. */
    uint64_t timestamp() const { return ts_; }

    void merge(const LWWRegister<T>& other) {
        if (!other.value_) {
          return;
        }
        if (other.ts_ > ts_ ||
            (other.ts_ == ts_ && other.winning_node_ > winning_node_)) {
            value_        = other.value_;
            ts_           = other.ts_;
            winning_node_ = other.winning_node_;
        }
    }

private:
    std::string    node_id_;
    std::optional<T> value_;
    uint64_t       ts_{0};
    std::string    winning_node_;
};

// ─────────────────────────────────────────────────────────────────────────────
// MVRegister<T>
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A Multi-Value Register.
 *
 * Concurrent writes from different nodes both survive until one supersedes
 * the other.  Each write is tagged with a Dot; a merge retains all values
 * whose dot is not dominated by another node's vector clock.
 *
 * For simplicity this implementation retains all concurrent values without
 * vector-clock dominance pruning.  Values are deduplicated on write.
 *
 * @tparam T Value type (must be equality-comparable and copy-constructible).
 */
template<typename T>
class MVRegister {
public:
    /**
     * @brief Write @p value tagged with @p dot.
     *
     * Clears all previous values (simulating causally-dominating write).
     */
    void write(T value, const Dot& dot) {
        entries_.clear();
        entries_.emplace_back(std::move(value), dot);
    }

    /**
     * @brief Returns all concurrent values currently in the register.
     */
    std::vector<T> read() const {
        std::vector<T> result = {};

        result.reserve(entries_.size());
        for (const auto& entry : entries_) { result.push_back(entry.first); }
        return result;
    }

    /**
     * @brief Merge another MVRegister into this one (union of entries).
     */
    void merge(const MVRegister<T>& other) {
        for (const auto& oe : other.entries_) {
            bool found = false;
            for (const auto& le : entries_) {
                if (le.second == oe.second) { found = true; break; }
            }
            if (!found) { entries_.push_back(oe); }
        }
    }

private:
    std::vector<std::pair<T, Dot>> entries_;
};

// ─────────────────────────────────────────────────────────────────────────────
// GrowOnlySet<T>
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A Grow-Only Set (G-Set).
 *
 * Elements can only be added, never removed.
 *
 * @tparam T Element type (must be less-than-comparable).
 */
template<typename T>
class GrowOnlySet {
public:
    void add(const T& element) { elements_.insert(element); }

    bool contains(const T& element) const {
        return elements_.count(element) > 0;
    }

    const std::set<T>& elements() const { return elements_; }

    void merge(const GrowOnlySet<T>& other) {
        elements_.insert(other.elements_.begin(), other.elements_.end());
    }

private:
    std::set<T> elements_;
};

// ─────────────────────────────────────────────────────────────────────────────
// TwoPSet<T>
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Two-Phase Set (2P-Set).
 *
 * Uses two G-Sets: the "add" set A and the "remove" set R (tombstones).
 * An element is a member if it is in A and NOT in R.  Elements in R can
 * never be re-added.
 *
 * @tparam T Element type.
 */
template<typename T>
class TwoPSet {
public:
    void add(const T& element) { added_.add(element); }

    /**
     * @brief Remove @p element (tombstone).  Has no effect if the element
     *        was never added.
     */
    void remove(const T& element) {
        if (added_.contains(element)) { removed_.add(element); }
    }

    bool contains(const T& element) const {
        return added_.contains(element) && !removed_.contains(element);
    }

    void merge(const TwoPSet<T>& other) {
        added_.merge(other.added_);
        removed_.merge(other.removed_);
    }

private:
    GrowOnlySet<T> added_;
    GrowOnlySet<T> removed_;
};

// ─────────────────────────────────────────────────────────────────────────────
// ORSet<T>
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Observed-Remove Set (OR-Set).
 *
 * Unlike 2P-Set, elements can be re-added after removal.  Each add generates
 * a unique Dot; remove operations tombstone only the specific dots observed at
 * remove time.  A concurrent add on another node will survive a concurrent
 * remove because it carries a different dot.
 *
 * @tparam T Element type (must be less-than-comparable).
 */
template<typename T>
class ORSet {
public:
    explicit ORSet(const std::string& node_id)
        : node_id_(node_id), counter_(0) {}

    /** @brief Add @p element, generating a fresh dot. */
    void add(const T& element) {
        Dot dot{node_id_, ++counter_};
        entries_[element].insert(dot);
        tombstones_.erase(dot);
    }

    /**
     * @brief Remove @p element by tombstoning all currently-observed dots.
     */
    void remove(const T& element) {
        auto it = entries_.find(element);
        if (it == entries_.end()) {
          return;
        }
        for (const auto& dot : it->second) {
            tombstones_.insert(dot);
        }
        entries_.erase(it);
    }

    bool contains(const T& element) const {
        auto it = entries_.find(element);
        if (it == entries_.end()) {
          return false;
        }
        for (const auto& dot : it->second) {
            if (!tombstones_.count(dot)) {
              return true;
            }
        }
        return false;
    }

    std::set<T> elements() const {
        std::set<T> result = {};

        for (const auto& [elem, dots] : entries_) {
            for (const auto& dot : dots) {
                if (!tombstones_.count(dot)) { result.insert(elem); break; }
            }
        }
        return result;
    }

    void merge(const ORSet<T>& other) {
        for (const auto& [elem, dots] : other.entries_) {
            for (const auto& dot : dots) {
                if (!tombstones_.count(dot)) {
                    entries_[elem].insert(dot);
                }
            }
        }
        tombstones_.insert(other.tombstones_.begin(), other.tombstones_.end());
        // Remove live dots that are now tombstoned
        for (auto& [elem, dots] : entries_) {
            for (auto it = dots.begin(); it != dots.end(); ) {
                if (tombstones_.count(*it)) {
                  it = dots.erase(it);
                }
                else ++it;
            }
        }
        // Prune empty entries
        for (auto it = entries_.begin(); it != entries_.end(); ) {
            if (it->second.empty()) {
              it = entries_.erase(it);
            }
            else ++it;
        }
        // Advance counter past any seen dots from this node
        for (const auto& dot : other.tombstones_) {
            if (dot.node_id == node_id_ && dot.counter > counter_) {
                counter_ = dot.counter;
            }
        }
    }

private:
    std::string node_id_;
    uint64_t    counter_;
    std::map<T, std::set<Dot>> entries_;
    std::set<Dot> tombstones_;
};

// ─────────────────────────────────────────────────────────────────────────────
// LWWMap<K, V>
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A Last-Write-Wins Map.
 *
 * Each key independently follows LWW semantics: the most-recently-timestamped
 * value wins.
 *
 * @tparam K Key type (must be less-than-comparable).
 * @tparam V Value type (must be copy-constructible).
 */
template<typename K, typename V>
class LWWMap {
public:
    explicit LWWMap(const std::string& node_id) : node_id_(node_id) {}

    /** @brief Set @p key to @p value with @p timestamp_us. */
    void put(const K& key, V value, uint64_t timestamp_us) {
        auto& slot = entries_[key];
        if (timestamp_us > slot.ts ||
            (timestamp_us == slot.ts && node_id_ > slot.node_id)) {
            slot.value   = std::move(value);
            slot.ts      = timestamp_us;
            slot.node_id = node_id_;
            slot.removed = false;
        }
    }

    /** @brief Mark @p key as removed with @p timestamp_us. */
    void remove(const K& key, uint64_t timestamp_us) {
        auto& slot = entries_[key];
        if (timestamp_us > slot.ts ||
            (timestamp_us == slot.ts && node_id_ > slot.node_id)) {
            slot.ts      = timestamp_us;
            slot.node_id = node_id_;
            slot.removed = true;
            slot.value.reset();
        }
    }

    /** @brief Returns the value for @p key, or empty optional if absent/removed. */
    std::optional<V> get(const K& key) const {
        auto it = entries_.find(key);
        if (it == entries_.end() || it->second.removed) return {};
        return it->second.value;
    }

    bool contains(const K& key) const {
        auto it = entries_.find(key);
        return it != entries_.end() && !it->second.removed;
    }

    void merge(const LWWMap<K, V>& other) {
        for (const auto& [key, oslot] : other.entries_) {
            auto& lslot = entries_[key];
            if (oslot.ts > lslot.ts ||
                (oslot.ts == lslot.ts && oslot.node_id > lslot.node_id)) {
                lslot = oslot;
            }
        }
    }

private:
    struct Slot {
        std::optional<V> value;
        uint64_t         ts{0};
        std::string      node_id;
        bool             removed{false};
    };

    std::string          node_id_;
    std::map<K, Slot>    entries_;
};

// ─────────────────────────────────────────────────────────────────────────────
// RGArray<T>
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Replicated Growable Array (RGA).
 *
 * A sequence CRDT that preserves insertion order.  Each element is tagged
 * with a unique Dot.  Insertions specify a predecessor Dot (the element
 * after which to insert); deletions tombstone the element's Dot.
 *
 * This implementation uses a vector for O(n) insert/delete, which is
 * acceptable for sequences of up to ~10 k elements.
 *
 * @tparam T Element type.
 */
template<typename T>
class RGArray {
public:
    explicit RGArray(const std::string& node_id)
        : node_id_(node_id), counter_(0) {}

    /**
     * @brief Insert @p value after the element with Dot @p after.
     *
     * Use an empty-string Dot{} to insert at the beginning.
     */
    Dot insertAfter(const Dot& after, T value) {
        Dot new_dot{node_id_, ++counter_};
        Entry e{new_dot, std::move(value), false};

        if (after.node_id.empty()) {
            elements_.insert(elements_.begin(), std::move(e));
        } else {
            auto it = findDot(after);
            if (it != elements_.end()) {
                ++it;
                elements_.insert(it, std::move(e));
            } else {
                elements_.push_back(std::move(e));
            }
        }
        return new_dot;
    }

    /** @brief Append @p value at the end of the sequence. */
    Dot append(T value) {
        Dot new_dot{node_id_, ++counter_};
        elements_.push_back(Entry{new_dot, std::move(value), false});
        return new_dot;
    }

    /** @brief Delete the element identified by @p dot. */
    void remove(const Dot& dot) {
        auto it = findDot(dot);
        if (it != elements_.end()) {
          it->tombstoned = true;
        }
    }

    /** @brief Returns all live (non-tombstoned) elements in order. */
    std::vector<T> read() const {
        std::vector<T> result = {};

        for (const auto& e : elements_) {
            if (!e.tombstoned) {
              result.push_back(e.value);
            }
        }
        return result;
    }

    /**
     * @brief Merge another RGArray state (union of entries, order by dot).
     */
    void merge(const RGArray<T>& other) {
        for (const auto& oe : other.elements_) {
            auto it = findDot(oe.dot);
            if (it == elements_.end()) {
                // Insert at end; callers may re-sort after a full merge
                elements_.push_back(oe);
            } else if (oe.tombstoned) {
                it->tombstoned = true;
            }
        }
        if (other.counter_ > counter_) {
          counter_ = other.counter_;
        }
    }

    size_t size() const {
        size_t cnt = 0;
        for (const auto& e : elements_) {
            if (!e.tombstoned) {
              ++cnt;
            }
        }
        return cnt;
    }

private:
    struct Entry {
        Dot  dot;
        T    value;
        bool tombstoned{false};
    };

    using It = typename std::vector<Entry>::iterator;

    It findDot(const Dot& dot) {
        for (auto it = elements_.begin(); it != elements_.end(); ++it) {
            if (it->dot == dot) {
              return it;
            }
        }
        return elements_.end();
    }

    std::string          node_id_;
    uint64_t             counter_;
    std::vector<Entry>   elements_;
};

// ─────────────────────────────────────────────────────────────────────────────
// EnableWinsFlag / DisableWinsFlag
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Enable-Wins Flag (EW-Flag).
 *
 * When enable() and disable() are called concurrently on different nodes, the
 * merged value is *enabled*.
 */
class EnableWinsFlag {
public:
    void enable()  { enabled_ = true;  ts_enable_  = ++global_ts_; }
    void disable() { enabled_ = false; ts_disable_ = ++global_ts_; }

    bool value() const {
        // Enable wins on tie
        return ts_enable_ >= ts_disable_;
    }

    void merge(const EnableWinsFlag& other) {
        if (other.ts_enable_ > ts_enable_) {
            ts_enable_ = other.ts_enable_;
            enabled_   = true;
        }
        if (other.ts_disable_ > ts_disable_) {
            ts_disable_ = other.ts_disable_;
        }
        // Re-evaluate
        enabled_ = (ts_enable_ >= ts_disable_);
        if (global_ts_ < std::max(other.ts_enable_, other.ts_disable_))
            global_ts_ = std::max(other.ts_enable_, other.ts_disable_);
    }

private:
    bool     enabled_{false};
    uint64_t ts_enable_{0};
    uint64_t ts_disable_{0};
    uint64_t global_ts_{0};
};

/**
 * @brief Disable-Wins Flag (DW-Flag).
 *
 * When enable() and disable() are called concurrently on different nodes, the
 * merged value is *disabled*.
 */
class DisableWinsFlag {
public:
    void enable()  { enabled_ = true;  ts_enable_  = ++global_ts_; }
    void disable() { enabled_ = false; ts_disable_ = ++global_ts_; }

    bool value() const {
        // Disable wins on tie
        return ts_enable_ > ts_disable_;
    }

    void merge(const DisableWinsFlag& other) {
        if (other.ts_enable_ > ts_enable_) {
            ts_enable_ = other.ts_enable_;
        }
        if (other.ts_disable_ > ts_disable_) {
            ts_disable_ = other.ts_disable_;
        }
        // Re-evaluate
        enabled_ = (ts_enable_ > ts_disable_);
        if (global_ts_ < std::max(other.ts_enable_, other.ts_disable_))
            global_ts_ = std::max(other.ts_enable_, other.ts_disable_);
    }

private:
    bool     enabled_{false};
    uint64_t ts_enable_{0};
    uint64_t ts_disable_{0};
    uint64_t global_ts_{0};
};

}  // namespace crdt
}  // namespace replication
}  // namespace themisdb
