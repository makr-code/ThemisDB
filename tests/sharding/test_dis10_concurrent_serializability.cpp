#include <gtest/gtest.h>

#include "sharding/two_phase_commit_coordinator.h"
#include "sharding/two_phase_commit_participant.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

using themis::sharding::ApplyOperationsCallback;
using themis::sharding::CoordinatorTxnOutcome;
using themis::sharding::EpochFencingConfig;
using themis::sharding::EpochFencingManager;
using themis::sharding::EpochToken;
using themis::sharding::ReleaseLockCallback;
using themis::sharding::TwoPhaseCommitCoordinator;
using themis::sharding::TwoPhaseCommitParticipant;
using themis::sharding::ValidateAndLockCallback;

struct HistoryEvent {
    std::uint64_t sequence = 0;
    std::string transaction_id;
    std::string shard_id;
    std::string operation;
    std::string key;
    std::string value;
};

struct HistoryCheck {
    bool serializable = false;
    std::vector<std::pair<std::string, std::string>> edges;
    std::vector<std::string> serial_order;
};

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        static std::atomic<std::uint64_t> next_id{0};
        const auto id = next_id.fetch_add(1, std::memory_order_relaxed);
        path_ = std::filesystem::temp_directory_path() /
            ("themis-dis10-serializability-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
             "-" + std::to_string(id));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

class HistoryRecorder {
public:
    void append(std::string transaction_id,
                std::string shard_id,
                std::string operation,
                std::string key,
                std::string value) {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back({
            ++sequence_,
            std::move(transaction_id),
            std::move(shard_id),
            std::move(operation),
            std::move(key),
            std::move(value)
        });
    }

    [[nodiscard]] std::vector<HistoryEvent> snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_;
    }

private:
    mutable std::mutex mutex_;
    std::uint64_t sequence_ = 0;
    std::vector<HistoryEvent> events_;
};

class PrepareBarrier {
public:
    explicit PrepareBarrier(std::size_t expected_transactions)
        : expected_transactions_(expected_transactions) {}

    [[nodiscard]] bool arriveAndWait(const std::string& transaction_id) {
        std::unique_lock<std::mutex> lock(mutex_);
        arrived_.insert(transaction_id);
        condition_.notify_all();
        return condition_.wait_for(lock, std::chrono::seconds(10), [this] {
            return arrived_.size() == expected_transactions_;
        });
    }

private:
    const std::size_t expected_transactions_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::set<std::string> arrived_;
};

struct ParticipantModel {
    [[nodiscard]] bool acquire(const std::string& transaction_id,
                               const nlohmann::json& operations) {
        std::set<std::string> keys;
        for (const auto& operation : operations) {
            if (!operation.is_object() || !operation.contains("key") ||
                !operation["key"].is_string()) {
                return false;
            }
            keys.insert(operation["key"].get<std::string>());
        }

        std::unique_lock<std::mutex> lock(lock_mutex);
        const bool available = lock_condition.wait_for(
            lock, std::chrono::seconds(15), [this, &keys, &transaction_id] {
                return std::all_of(keys.begin(), keys.end(), [this, &transaction_id](const auto& key) {
                    const auto found = key_owners.find(key);
                    return found == key_owners.end() || found->second == transaction_id;
                });
            });
        if (!available) {
            return false;
        }

        for (const auto& key : keys) {
            key_owners[key] = transaction_id;
        }
        return true;
    }

    void release(const std::string& transaction_id) {
        {
            std::lock_guard<std::mutex> lock(lock_mutex);
            for (auto it = key_owners.begin(); it != key_owners.end();) {
                if (it->second == transaction_id) {
                    it = key_owners.erase(it);
                } else {
                    ++it;
                }
            }
        }
        lock_condition.notify_all();
    }

    std::mutex lock_mutex;
    std::condition_variable lock_condition;
    std::map<std::string, std::string> key_owners;
    std::mutex values_mutex;
    std::map<std::string, std::string> values{{"shared", "seed"}};
};

[[nodiscard]] bool replaysObservedReads(
    const std::vector<std::string>& serial_order,
    const std::vector<HistoryEvent>& events,
    const std::vector<std::string>& shard_ids) {
    for (const auto& shard_id : shard_ids) {
        std::map<std::string, std::string> values{{"shared", "seed"}};
        for (const auto& transaction_id : serial_order) {
            std::vector<const HistoryEvent*> transaction_events;
            for (const auto& event : events) {
                if (event.transaction_id == transaction_id && event.shard_id == shard_id) {
                    transaction_events.push_back(&event);
                }
            }
            std::sort(transaction_events.begin(), transaction_events.end(),
                      [](const auto* lhs, const auto* rhs) {
                          return lhs->sequence < rhs->sequence;
                      });
            for (const auto* event : transaction_events) {
                if (event->operation == "read") {
                    const auto found = values.find(event->key);
                    const auto current = found == values.end() ? std::string{} : found->second;
                    if (current != event->value) {
                        return false;
                    }
                } else if (event->operation == "write") {
                    values[event->key] = event->value;
                } else {
                    return false;
                }
            }
        }
    }
    return true;
}

[[nodiscard]] HistoryCheck checkConflictSerializability(
    const std::vector<HistoryEvent>& events,
    const std::set<std::string>& committed_transactions,
    const std::vector<std::string>& shard_ids) {
    HistoryCheck result;
    if (committed_transactions.empty() || committed_transactions.size() > 8) {
        return result;
    }

    std::map<std::string, std::set<std::string>> graph;
    std::map<std::string, std::size_t> indegree;
    for (const auto& transaction_id : committed_transactions) {
        graph[transaction_id];
        indegree[transaction_id] = 0;
    }

    for (std::size_t i = 0; i < events.size(); ++i) {
        const auto& first = events[i];
        if (committed_transactions.count(first.transaction_id) == 0) {
            continue;
        }
        for (std::size_t j = i + 1; j < events.size(); ++j) {
            const auto& second = events[j];
            if (first.transaction_id == second.transaction_id ||
                committed_transactions.count(second.transaction_id) == 0 ||
                first.shard_id != second.shard_id || first.key != second.key ||
                (first.operation != "write" && second.operation != "write")) {
                continue;
            }

            const auto& before = first.sequence < second.sequence ? first : second;
            const auto& after = first.sequence < second.sequence ? second : first;
            if (graph[before.transaction_id].insert(after.transaction_id).second) {
                ++indegree[after.transaction_id];
                result.edges.emplace_back(before.transaction_id, after.transaction_id);
            }
        }
    }

    std::vector<std::string> order;
    std::set<std::string> placed;
    std::function<bool()> search = [&]() {
        if (order.size() == committed_transactions.size()) {
            if (replaysObservedReads(order, events, shard_ids)) {
                result.serial_order = order;
                return true;
            }
            return false;
        }

        for (const auto& transaction_id : committed_transactions) {
            if (placed.count(transaction_id) != 0 || indegree[transaction_id] != 0) {
                continue;
            }
            placed.insert(transaction_id);
            order.push_back(transaction_id);
            for (const auto& next : graph[transaction_id]) {
                --indegree[next];
            }
            if (search()) {
                return true;
            }
            for (const auto& next : graph[transaction_id]) {
                ++indegree[next];
            }
            order.pop_back();
            placed.erase(transaction_id);
        }
        return false;
    };

    result.serializable = search();
    return result;
}

[[nodiscard]] EpochToken makeToken(const std::string& shard_id,
                                   const std::string& coordinator_id) {
    EpochToken token;
    token.epoch = 1;
    token.issuer = coordinator_id;
    token.shard_id = shard_id;
    token.issued_at = std::chrono::system_clock::now();
    return token;
}

[[nodiscard]] bool writeEvidence(
    const std::filesystem::path& path,
    const std::vector<std::string>& transaction_ids,
    const std::vector<std::string>& shard_ids,
    const std::vector<HistoryEvent>& events,
    const HistoryCheck& check,
    const std::vector<CoordinatorTxnOutcome>& outcomes,
    const std::vector<std::chrono::steady_clock::time_point>& starts,
    const std::vector<std::chrono::steady_clock::time_point>& ends,
    const std::chrono::steady_clock::time_point& time_origin) {
    const auto environment_value = [](const char* name) {
        const char* value = std::getenv(name);
        return value == nullptr ? std::string{} : std::string(value);
    };
    nlohmann::json evidence;
    evidence["schema"] = "themisdb.dis10.concurrent-serializability.v1";
    evidence["result"] = check.serializable ? "passed" : "failed";
    evidence["serializable"] = check.serializable;
    const auto source_commit_sha = environment_value("THEMIS_SOURCE_COMMIT_SHA");
    evidence["commit_sha"] = source_commit_sha.empty()
        ? environment_value("GITHUB_SHA")
        : source_commit_sha;
    evidence["workflow_run_id"] = environment_value("GITHUB_RUN_ID");
    evidence["workflow_run_url"] =
        environment_value("GITHUB_SERVER_URL") + "/" +
        environment_value("GITHUB_REPOSITORY") + "/actions/runs/" +
        environment_value("GITHUB_RUN_ID");
    evidence["runner_os"] = environment_value("RUNNER_OS");
    evidence["runner_arch"] = environment_value("RUNNER_ARCH");
    evidence["transactions"] = transaction_ids;
    evidence["participants"] = shard_ids;
    evidence["serial_order"] = check.serial_order;
    evidence["precedence_edges"] = nlohmann::json::array();
    for (const auto& [before, after] : check.edges) {
        evidence["precedence_edges"].push_back({before, after});
    }
    evidence["events"] = nlohmann::json::array();
    for (const auto& event : events) {
        evidence["events"].push_back({
            {"sequence", event.sequence},
            {"transaction_id", event.transaction_id},
            {"shard_id", event.shard_id},
            {"operation", event.operation},
            {"key", event.key},
            {"value", event.value}
        });
    }
    evidence["client_calls"] = nlohmann::json::array();
    for (std::size_t i = 0; i < transaction_ids.size(); ++i) {
        evidence["client_calls"].push_back({
            {"transaction_id", transaction_ids[i]},
            {"outcome", outcomes[i].committed() ? "COMMITTED" : "NOT_COMMITTED"},
            {"start_ns", std::chrono::duration_cast<std::chrono::nanoseconds>(
                starts[i] - time_origin).count()},
            {"end_ns", std::chrono::duration_cast<std::chrono::nanoseconds>(
                ends[i] - time_origin).count()}
        });
    }

    std::error_code error;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            return false;
        }
    }
    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output) {
        return false;
    }
    output << evidence.dump(2) << '\n';
    return output.good();
}

TEST(DIS10HistoryChecker, RejectsOpposingConflictOrders) {
    const std::vector<HistoryEvent> events{
        {1, "txn-a", "shard-a", "write", "key-a", "a"},
        {2, "txn-b", "shard-a", "read", "key-a", "a"},
        {3, "txn-b", "shard-b", "write", "key-b", "b"},
        {4, "txn-a", "shard-b", "read", "key-b", "b"}
    };
    const auto result = checkConflictSerializability(
        events, {"txn-a", "txn-b"}, {"shard-a", "shard-b"});
    EXPECT_FALSE(result.serializable);
    EXPECT_EQ(result.edges.size(), 2u);
}

TEST(DIS10ConcurrentSerializability, ConcurrentConflictingTransactionsHaveSerialHistory) {
    constexpr std::size_t kTransactionCount = 4;
    const std::vector<std::string> shard_ids{"shard-a", "shard-b"};
    std::vector<std::string> transaction_ids;
    for (std::size_t i = 0; i < kTransactionCount; ++i) {
        transaction_ids.push_back("txn-serial-" + std::to_string(i));
    }

    TemporaryDirectory temporary_directory;
    HistoryRecorder history;
    PrepareBarrier prepare_barrier(kTransactionCount);
    std::map<std::string, std::shared_ptr<ParticipantModel>> models;
    std::map<std::string, std::unique_ptr<TwoPhaseCommitParticipant>> participants;

    for (const auto& shard_id : shard_ids) {
        auto model = std::make_shared<ParticipantModel>();
        models.emplace(shard_id, model);

        TwoPhaseCommitParticipant::Config participant_config;
        participant_config.wal_directory =
            (temporary_directory.path() / ("participant-" + shard_id)).string();
        participant_config.sync_wal_writes = false;
        EpochFencingConfig fencing_config;
        fencing_config.shard_id = shard_id;
        fencing_config.node_id = "participant-" + shard_id;
        fencing_config.auto_stonith = false;
        participant_config.fencing_manager =
            std::make_shared<EpochFencingManager>(std::move(fencing_config));

        ValidateAndLockCallback validate =
            [&, model, shard_id](const std::string& transaction_id,
                                 const nlohmann::json& operations) {
                if (shard_id == "shard-a" &&
                    !prepare_barrier.arriveAndWait(transaction_id)) {
                    return false;
                }
                if (!model->acquire(transaction_id, operations)) {
                    return false;
                }
                std::lock_guard<std::mutex> lock(model->values_mutex);
                for (const auto& operation : operations) {
                    if (operation.value("type", std::string{}) == "read") {
                        const auto key = operation.at("key").get<std::string>();
                        const auto found = model->values.find(key);
                        history.append(
                            transaction_id, shard_id, "read", key,
                            found == model->values.end() ? std::string{} : found->second);
                    }
                }
                return true;
            };
        ApplyOperationsCallback apply =
            [&, model, shard_id](const std::string& transaction_id,
                                 const nlohmann::json& operations,
                                 int64_t) {
                std::lock_guard<std::mutex> lock(model->values_mutex);
                for (const auto& operation : operations) {
                    if (operation.value("type", std::string{}) == "write") {
                        const auto key = operation.at("key").get<std::string>();
                        const auto value = operation.at("value").get<std::string>();
                        model->values[key] = value;
                        history.append(transaction_id, shard_id, "write", key, value);
                    }
                }
                return true;
            };
        ReleaseLockCallback release =
            [model](const std::string& transaction_id) {
                model->release(transaction_id);
            };
        participants.emplace(
            shard_id,
            std::make_unique<TwoPhaseCommitParticipant>(
                shard_id, participant_config, std::move(validate),
                std::move(apply), std::move(release)));
    }

    TwoPhaseCommitCoordinator::Config coordinator_config;
    coordinator_config.wal_directory =
        (temporary_directory.path() / "coordinator").string();
    coordinator_config.sync_wal_writes = false;
    coordinator_config.fencing_token_provider =
        [](const std::string& shard_id) -> std::optional<EpochToken> {
            return makeToken(shard_id, "coord-serializability");
        };
    TwoPhaseCommitCoordinator coordinator("coord-serializability", coordinator_config);
    for (const auto& [shard_id, participant] : participants) {
        coordinator.registerParticipant(shard_id, participant.get());
    }

    std::mutex start_mutex;
    std::condition_variable start_condition;
    std::size_t ready_count = 0;
    bool start_transactions = false;
    const auto time_origin = std::chrono::steady_clock::now();
    std::vector<CoordinatorTxnOutcome> outcomes(kTransactionCount);
    std::vector<std::chrono::steady_clock::time_point> starts(kTransactionCount);
    std::vector<std::chrono::steady_clock::time_point> ends(kTransactionCount);
    std::vector<std::exception_ptr> errors(kTransactionCount);
    std::vector<std::thread> workers;
    workers.reserve(kTransactionCount);

    for (std::size_t i = 0; i < kTransactionCount; ++i) {
        workers.emplace_back([&, i] {
            {
                std::unique_lock<std::mutex> lock(start_mutex);
                ++ready_count;
                start_condition.notify_all();
                start_condition.wait(lock, [&] { return start_transactions; });
            }

            starts[i] = std::chrono::steady_clock::now();
            try {
                const auto transaction_id = transaction_ids[i];
                const nlohmann::json read_then_write = nlohmann::json::array({
                    {{"type", "read"}, {"key", "shared"}},
                    {{"type", "write"}, {"key", "shared"}, {"value", transaction_id}}
                });
                outcomes[i] = coordinator.commit(transaction_id, {
                    {"shard-a", read_then_write},
                    {"shard-b", read_then_write}
                });
            } catch (...) {
                errors[i] = std::current_exception();
            }
            ends[i] = std::chrono::steady_clock::now();
        });
    }

    {
        std::unique_lock<std::mutex> lock(start_mutex);
        start_condition.wait(lock, [&] { return ready_count == kTransactionCount; });
        start_transactions = true;
    }
    start_condition.notify_all();
    for (auto& worker : workers) {
        worker.join();
    }

    for (std::size_t i = 0; i < kTransactionCount; ++i) {
        ASSERT_EQ(errors[i], nullptr);
        ASSERT_TRUE(outcomes[i].committed()) << outcomes[i].reason;
        ASSERT_LT(starts[i], ends[i]);
    }
    const auto latest_start = *std::max_element(starts.begin(), starts.end());
    const auto earliest_end = *std::min_element(ends.begin(), ends.end());
    ASSERT_LT(latest_start, earliest_end);

    const auto events = history.snapshot();
    ASSERT_EQ(events.size(), 4u * kTransactionCount);
    const std::set<std::string> committed_transactions(
        transaction_ids.begin(), transaction_ids.end());
    const auto check = checkConflictSerializability(
        events, committed_transactions, shard_ids);
    ASSERT_TRUE(check.serializable);
    ASSERT_EQ(check.serial_order.size(), kTransactionCount);

    for (const auto& shard_id : shard_ids) {
        for (const auto& transaction_id : transaction_ids) {
            const auto state = participants.at(shard_id)->getTransactionState(transaction_id);
            ASSERT_TRUE(state.has_value());
            EXPECT_EQ(*state, themis::sharding::ParticipantTxnState::COMMITTED);
        }
    }

    const char* evidence_path = std::getenv("THEMIS_DIS10_SERIALIZABILITY_EVIDENCE_FILE");
    if (evidence_path != nullptr && *evidence_path != '\0') {
        ASSERT_TRUE(writeEvidence(
            evidence_path, transaction_ids, shard_ids, events, check,
            outcomes, starts, ends, time_origin));
    }
}

} // namespace
