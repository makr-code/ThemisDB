/**
 * Unit tests for Arrow Flight RPC support (analytics/arrow_flight.h)
 *
 * Covers:
 *  - FlightDescriptor construction (path and command types)
 *  - Server lifecycle: create, start, stop, isRunning
 *  - registerDataset / unregisterDataset / listRegisteredDatasets
 *  - registerPutHandler
 *  - Client connect (in-process)
 *  - Client connect fails when server is not running
 *  - listFlights reflects registered datasets
 *  - doGet returns the batch produced by the registered producer
 *  - doPut invokes the registered handler with the pushed batch
 *  - doGet on unknown descriptor throws
 *  - doPut to unknown descriptor returns failure
 *  - Multiple datasets on the same server
 *  - Server stop removes datasets from the registry
 *  - Re-start re-registers the server
 *  - Empty batch round-trip (doGet / doPut)
 *  - Large batch round-trip (1 000 rows)
 *  - Multi-column batch (INT64, DOUBLE, STRING, BOOLEAN)
 *  - Null values preserved in round-trip
 *  - endpointUrl contains host and port
 *  - Two concurrent servers on different ports
 *  - FlightInfo total_records hint is propagated
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "analytics/arrow_flight.h"
#include "analytics/arrow_export.h"

#include <stdexcept>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Aliases
// ---------------------------------------------------------------------------
using namespace themisdb::analytics;
using RecordBatch = themis::analytics::ArrowRecordBatch;
using DataType    = RecordBatch::DataType;
using ColSchema   = RecordBatch::ColumnSchema;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Build a minimal INT64 + STRING + DOUBLE batch with @p num_rows rows. */
static RecordBatch makeBatch(size_t num_rows) {
    RecordBatch batch;
    batch.addColumn({"id",    DataType::INT64,  false});
    batch.addColumn({"name",  DataType::STRING, true});
    batch.addColumn({"value", DataType::DOUBLE, true});
    for (size_t i = 0; i < num_rows; ++i) {
        batch.appendRow({
            static_cast<int64_t>(i),
            std::string("row_") + std::to_string(i),
            static_cast<double>(i) * 1.5
        });
    }
    return batch;
}

/** Build a FlightServerOptions on the given port with inprocess=true. */
static FlightServerOptions makeServerOpts(int port) {
    FlightServerOptions opts;
    opts.host               = "127.0.0.1";
    opts.port               = port;
    opts.register_inprocess = true;
    return opts;
}

/** Build a FlightClientOptions pointing to the given port (inprocess first). */
static FlightClientOptions makeClientOpts(int port) {
    FlightClientOptions opts;
    opts.host             = "127.0.0.1";
    opts.port             = port;
    opts.prefer_inprocess = true;
    return opts;
}

// ===========================================================================
// FlightDescriptor tests
// ===========================================================================

TEST(FlightDescriptorTest, PathToString) {
    auto desc = FlightDescriptor::fromPath({"analytics", "sales", "2024"});
    EXPECT_EQ(desc.toString(), "path://analytics/sales/2024");
    EXPECT_EQ(desc.type, FlightDescriptor::Type::PATH);
}

TEST(FlightDescriptorTest, CommandToString) {
    auto desc = FlightDescriptor::fromCommand("SELECT * FROM sales");
    EXPECT_EQ(desc.toString(), "cmd:[SELECT * FROM sales]");
    EXPECT_EQ(desc.type, FlightDescriptor::Type::COMMAND);
}

TEST(FlightDescriptorTest, EmptyPath) {
    auto desc = FlightDescriptor::fromPath({});
    EXPECT_EQ(desc.toString(), "path://");
}

TEST(FlightDescriptorTest, SingleSegmentPath) {
    auto desc = FlightDescriptor::fromPath({"sales"});
    EXPECT_EQ(desc.toString(), "path://sales");
}

// ===========================================================================
// Server lifecycle
// ===========================================================================

TEST(ArrowFlightServerTest, CreateDoesNotStart) {
    auto server = ArrowFlightServer::create(makeServerOpts(18815));
    EXPECT_FALSE(server->isRunning());
}

TEST(ArrowFlightServerTest, StartAndStop) {
    auto server = ArrowFlightServer::create(makeServerOpts(18816));
    server->start();
    EXPECT_TRUE(server->isRunning());
    server->stop();
    EXPECT_FALSE(server->isRunning());
}

TEST(ArrowFlightServerTest, StartIsIdempotent) {
    auto server = ArrowFlightServer::create(makeServerOpts(18817));
    server->start();
    server->start();  // second call must not crash
    EXPECT_TRUE(server->isRunning());
    server->stop();
}

TEST(ArrowFlightServerTest, StopIsIdempotent) {
    auto server = ArrowFlightServer::create(makeServerOpts(18818));
    server->start();
    server->stop();
    server->stop();  // second call must not crash
    EXPECT_FALSE(server->isRunning());
}

TEST(ArrowFlightServerTest, EndpointUrlContainsPort) {
    auto server = ArrowFlightServer::create(makeServerOpts(18819));
    const std::string url = server->endpointUrl();
    EXPECT_NE(url.find("18819"), std::string::npos);
}

TEST(ArrowFlightServerTest, DestructorStopsRunningServer) {
    // After the server is destroyed the in-process registry entry should be
    // removed so a new server can be created on the same port.
    {
        auto server = ArrowFlightServer::create(makeServerOpts(18820));
        server->start();
        EXPECT_TRUE(server->isRunning());
    }
    // No crash; server was stopped by destructor.
    // We can start a new one on the same port without error.
    auto server2 = ArrowFlightServer::create(makeServerOpts(18820));
    server2->start();
    EXPECT_TRUE(server2->isRunning());
    server2->stop();
}

TEST(ArrowFlightServerTest, RestartAfterStop) {
    auto server = ArrowFlightServer::create(makeServerOpts(18821));
    server->start();
    server->stop();
    server->start();
    EXPECT_TRUE(server->isRunning());
    server->stop();
}

// ===========================================================================
// Dataset registration
// ===========================================================================

TEST(ArrowFlightServerTest, RegisterDatasetAppearsInList) {
    auto server = ArrowFlightServer::create(makeServerOpts(18830));
    server->start();
    server->registerDataset({"sales"}, []() { return makeBatch(3); });
    const auto infos = server->listRegisteredDatasets();
    ASSERT_EQ(infos.size(), 1u);
    EXPECT_EQ(infos[0].descriptor.path, std::vector<std::string>{"sales"});
    server->stop();
}

TEST(ArrowFlightServerTest, RegisterMultipleDatasetsAllListed) {
    auto server = ArrowFlightServer::create(makeServerOpts(18831));
    server->start();
    server->registerDataset({"ds1"}, []() { return makeBatch(1); });
    server->registerDataset({"ds2"}, []() { return makeBatch(2); });
    server->registerDataset({"ds3"}, []() { return makeBatch(3); });
    EXPECT_EQ(server->listRegisteredDatasets().size(), 3u);
    server->stop();
}

TEST(ArrowFlightServerTest, UnregisterDatasetRemovedFromList) {
    auto server = ArrowFlightServer::create(makeServerOpts(18832));
    server->start();
    server->registerDataset({"ds1"}, []() { return makeBatch(5); });
    server->registerDataset({"ds2"}, []() { return makeBatch(5); });
    server->unregisterDataset({"ds1"});
    const auto infos = server->listRegisteredDatasets();
    ASSERT_EQ(infos.size(), 1u);
    EXPECT_EQ(infos[0].descriptor.path, std::vector<std::string>{"ds2"});
    server->stop();
}

TEST(ArrowFlightServerTest, TotalRecordsHintPropagated) {
    auto server = ArrowFlightServer::create(makeServerOpts(18833));
    server->start();
    server->registerDataset({"bigdata"}, []() { return makeBatch(0); },
                             /*total_rows=*/1000000LL);
    const auto infos = server->listRegisteredDatasets();
    ASSERT_EQ(infos.size(), 1u);
    EXPECT_EQ(infos[0].total_records, 1000000LL);
    server->stop();
}

TEST(ArrowFlightServerTest, PutHandlerNotListedAsDataset) {
    auto server = ArrowFlightServer::create(makeServerOpts(18834));
    server->start();
    server->registerPutHandler({"sink"}, [](RecordBatch) {});
    // put-only handlers must not appear in listRegisteredDatasets
    EXPECT_TRUE(server->listRegisteredDatasets().empty());
    server->stop();
}

TEST(ArrowFlightServerTest, StopClearsDatasets) {
    auto server = ArrowFlightServer::create(makeServerOpts(18835));
    server->start();
    server->registerDataset({"x"}, []() { return makeBatch(1); });
    server->stop();
    // After stop, no datasets should be reachable via the in-process registry
    // (a client cannot connect to a stopped server)
    EXPECT_THROW(
        (void)ArrowFlightClient::connect(makeClientOpts(18835)),
        std::runtime_error);
}

// ===========================================================================
// Client connect
// ===========================================================================

TEST(ArrowFlightClientTest, ConnectToRunningServer) {
    auto server = ArrowFlightServer::create(makeServerOpts(18840));
    server->start();
    auto client = ArrowFlightClient::connect(makeClientOpts(18840));
    EXPECT_TRUE(client->isConnected());
    client->close();
    server->stop();
}

TEST(ArrowFlightClientTest, ConnectToStoppedServerThrows) {
    EXPECT_THROW(
        (void)ArrowFlightClient::connect(makeClientOpts(18841)),
        std::runtime_error);
}

TEST(ArrowFlightClientTest, CloseDisconnects) {
    auto server = ArrowFlightServer::create(makeServerOpts(18842));
    server->start();
    auto client = ArrowFlightClient::connect(makeClientOpts(18842));
    EXPECT_TRUE(client->isConnected());
    (void)client->close();
    EXPECT_FALSE(client->isConnected());
    (void)server->stop();
}

TEST(ArrowFlightClientTest, OperationsAfterCloseThrow) {
    auto server = ArrowFlightServer::create(makeServerOpts(18843));
    server->start();
    auto client = ArrowFlightClient::connect(makeClientOpts(18843));
    (void)client->close();

    EXPECT_THROW({
        auto flights = client->listFlights();
        static_cast<void>(flights);
    }, std::runtime_error);
    EXPECT_THROW(
        {
            auto batch = client->doGet(FlightDescriptor::fromPath({"x"}));
            static_cast<void>(batch);
        },
        std::runtime_error);
    auto empty = makeBatch(0);
    EXPECT_THROW(
        {
            auto put_result = client->doPut(empty, FlightDescriptor::fromPath({"x"}));
            static_cast<void>(put_result);
        },
        std::runtime_error);
    (void)server->stop();
}

// ===========================================================================
// listFlights
// ===========================================================================

TEST(ArrowFlightClientTest, ListFlightsEmpty) {
    auto server = ArrowFlightServer::create(makeServerOpts(18850));
    server->start();
    auto client = ArrowFlightClient::connect(makeClientOpts(18850));
    EXPECT_TRUE(client->listFlights().empty());
    client->close();
    server->stop();
}

TEST(ArrowFlightClientTest, ListFlightsReturnsRegisteredDatasets) {
    auto server = ArrowFlightServer::create(makeServerOpts(18851));
    server->start();
    server->registerDataset({"orders"}, []() { return makeBatch(2); });
    server->registerDataset({"items"},  []() { return makeBatch(3); });
    auto client = ArrowFlightClient::connect(makeClientOpts(18851));
    const auto infos = client->listFlights();
    EXPECT_EQ(infos.size(), 2u);
    client->close();
    server->stop();
}

// ===========================================================================
// doGet
// ===========================================================================

TEST(ArrowFlightClientTest, DoGetReturnsRegisteredBatch) {
    auto server = ArrowFlightServer::create(makeServerOpts(18860));
    server->start();
    server->registerDataset(
        {"sales"},
        []() { return makeBatch(10); });

    auto client = ArrowFlightClient::connect(makeClientOpts(18860));
    auto batch  = client->doGet(FlightDescriptor::fromPath({"sales"}));
    EXPECT_EQ(batch.rowCount(),    10u);
    EXPECT_EQ(batch.columnCount(),  3u);
    EXPECT_EQ(batch.getColumn(0).schema.name, "id");
    EXPECT_EQ(batch.getColumn(1).schema.name, "name");
    EXPECT_EQ(batch.getColumn(2).schema.name, "value");
    client->close();
    server->stop();
}

TEST(ArrowFlightClientTest, DoGetUnknownPathThrows) {
    auto server = ArrowFlightServer::create(makeServerOpts(18861));
    server->start();
    auto client = ArrowFlightClient::connect(makeClientOpts(18861));
    EXPECT_THROW(
        {
            auto batch = client->doGet(FlightDescriptor::fromPath({"does_not_exist"}));
            static_cast<void>(batch);
        },
        std::runtime_error);
    (void)client->close();
    (void)server->stop();
}

TEST(ArrowFlightClientTest, DoGetEmptyBatch) {
    auto server = ArrowFlightServer::create(makeServerOpts(18862));
    server->start();
    server->registerDataset({"empty"}, []() {
        RecordBatch b;
        b.addColumn({"x", DataType::INT64, false});
        return b;
    });
    auto client = ArrowFlightClient::connect(makeClientOpts(18862));
    auto batch  = client->doGet(FlightDescriptor::fromPath({"empty"}));
    EXPECT_EQ(batch.rowCount(),   0u);
    EXPECT_EQ(batch.columnCount(), 1u);
    client->close();
    server->stop();
}

TEST(ArrowFlightClientTest, DoGetLargeBatch) {
    constexpr size_t N = 1000;
    auto server = ArrowFlightServer::create(makeServerOpts(18863));
    server->start();
    server->registerDataset({"big"}, []() { return makeBatch(N); });
    auto client = ArrowFlightClient::connect(makeClientOpts(18863));
    auto batch  = client->doGet(FlightDescriptor::fromPath({"big"}));
    EXPECT_EQ(batch.rowCount(), N);
    client->close();
    server->stop();
}

TEST(ArrowFlightClientTest, DoGetDataValuesCorrect) {
    auto server = ArrowFlightServer::create(makeServerOpts(18864));
    server->start();
    server->registerDataset({"data"}, []() { return makeBatch(5); });
    auto client = ArrowFlightClient::connect(makeClientOpts(18864));
    auto batch  = client->doGet(FlightDescriptor::fromPath({"data"}));
    // Verify a few INT64 values via zero-copy accessor
    const int64_t* ids = batch.getInt64Data(0);
    ASSERT_NE(ids, nullptr);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(ids[i], static_cast<int64_t>(i));
    }
    // Verify DOUBLE values
    const double* vals = batch.getDoubleData(2);
    ASSERT_NE(vals, nullptr);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_DOUBLE_EQ(vals[i], static_cast<double>(i) * 1.5);
    }
    client->close();
    server->stop();
}

// ===========================================================================
// doPut
// ===========================================================================

TEST(ArrowFlightClientTest, DoPutInvokesHandler) {
    RecordBatch received;

    auto server = ArrowFlightServer::create(makeServerOpts(18870));
    server->start();
    server->registerPutHandler(
        {"sink"},
        [&received](RecordBatch b) { received = std::move(b); });

    auto client = ArrowFlightClient::connect(makeClientOpts(18870));
    auto batch  = makeBatch(7);
    auto result = client->doPut(batch, FlightDescriptor::fromPath({"sink"}));

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.rows_accepted, 7);
    EXPECT_EQ(received.rowCount(), 7u);
    client->close();
    server->stop();
}

TEST(ArrowFlightClientTest, DoPutToUnknownPathReturnsFailed) {
    auto server = ArrowFlightServer::create(makeServerOpts(18871));
    server->start();
    auto client = ArrowFlightClient::connect(makeClientOpts(18871));
    auto batch  = makeBatch(3);
    auto result = client->doPut(batch,
                                FlightDescriptor::fromPath({"no_such_handler"}));
    EXPECT_FALSE(result.success);
    client->close();
    server->stop();
}

TEST(ArrowFlightClientTest, DoPutEmptyBatch) {
    int call_count = 0;
    auto server = ArrowFlightServer::create(makeServerOpts(18872));
    server->start();
    server->registerPutHandler(
        {"sink"},
        [&call_count](RecordBatch) { ++call_count; });
    auto client = ArrowFlightClient::connect(makeClientOpts(18872));
    RecordBatch empty;
    empty.addColumn({"id", DataType::INT64, false});
    auto result = client->doPut(empty, FlightDescriptor::fromPath({"sink"}));
    EXPECT_TRUE(result.success);
    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(result.rows_accepted, 0);
    client->close();
    server->stop();
}

// ===========================================================================
// Multi-column / mixed-type round-trip
// ===========================================================================

TEST(ArrowFlightClientTest, MultiColumnBatchRoundTrip) {
    RecordBatch original;
    original.addColumn({"id",     DataType::INT64,   false});
    original.addColumn({"score",  DataType::DOUBLE,  true});
    original.addColumn({"label",  DataType::STRING,  true});
    original.addColumn({"active", DataType::BOOLEAN, false});
    original.appendRow({int64_t(1), 9.5, std::string("A"), true});
    original.appendRow({int64_t(2), 7.0, std::string("B"), false});
    original.appendRow({int64_t(3), 8.3, std::string("C"), true});

    auto server = ArrowFlightServer::create(makeServerOpts(18880));
    server->start();
    server->registerDataset({"mixed"}, [&original]() { return original; });
    auto client = ArrowFlightClient::connect(makeClientOpts(18880));
    auto got    = client->doGet(FlightDescriptor::fromPath({"mixed"}));

    EXPECT_EQ(got.rowCount(),    3u);
    EXPECT_EQ(got.columnCount(), 4u);
    EXPECT_EQ(got.getColumn(0).schema.name, "id");
    EXPECT_EQ(got.getColumn(1).schema.name, "score");
    EXPECT_EQ(got.getColumn(2).schema.name, "label");
    EXPECT_EQ(got.getColumn(3).schema.name, "active");
    client->close();
    server->stop();
}

TEST(ArrowFlightClientTest, NullValuesPreservedViaDoPut) {
    RecordBatch received;
    auto server = ArrowFlightServer::create(makeServerOpts(18881));
    server->start();
    server->registerPutHandler(
        {"sink"},
        [&received](RecordBatch b) { received = std::move(b); });

    RecordBatch pushed;
    pushed.addColumn({"id",  DataType::INT64,  true});
    pushed.addColumn({"val", DataType::DOUBLE, true});
    pushed.appendRow({int64_t(1), 3.14});
    pushed.appendRow({nullptr,    nullptr});   // null row
    pushed.appendRow({int64_t(3), 2.72});

    auto client = ArrowFlightClient::connect(makeClientOpts(18881));
    auto result = client->doPut(pushed, FlightDescriptor::fromPath({"sink"}));
    EXPECT_TRUE(result.success);
    EXPECT_EQ(received.rowCount(), 3u);
    // null_bitmap must mark the middle row as null in both columns
    EXPECT_FALSE(received.getColumn(0).null_bitmap[0]);
    EXPECT_TRUE (received.getColumn(0).null_bitmap[1]);
    EXPECT_FALSE(received.getColumn(0).null_bitmap[2]);
    client->close();
    server->stop();
}

// ===========================================================================
// Two servers on different ports
// ===========================================================================

TEST(ArrowFlightServerTest, TwoServersOnDifferentPorts) {
    auto s1 = ArrowFlightServer::create(makeServerOpts(18890));
    auto s2 = ArrowFlightServer::create(makeServerOpts(18891));
    s1->start();
    s2->start();

    s1->registerDataset({"ds"}, []() { return makeBatch(1); });
    s2->registerDataset({"ds"}, []() { return makeBatch(2); });

    auto c1 = ArrowFlightClient::connect(makeClientOpts(18890));
    auto c2 = ArrowFlightClient::connect(makeClientOpts(18891));

    auto b1 = c1->doGet(FlightDescriptor::fromPath({"ds"}));
    auto b2 = c2->doGet(FlightDescriptor::fromPath({"ds"}));

    EXPECT_EQ(b1.rowCount(), 1u);
    EXPECT_EQ(b2.rowCount(), 2u);

    c1->close();
    c2->close();
    s1->stop();
    s2->stop();
}

TEST(ArrowFlightServerTest, CrossServerClientIsolation) {
    // A client connected to server 1 must not see datasets on server 2.
    auto s1 = ArrowFlightServer::create(makeServerOpts(18892));
    auto s2 = ArrowFlightServer::create(makeServerOpts(18893));
    s1->start();
    s2->start();

    s2->registerDataset({"s2_only"}, []() { return makeBatch(5); });

    auto c1 = ArrowFlightClient::connect(makeClientOpts(18892));
    // s2_only is on server 2; client 1 must not find it
    EXPECT_THROW(
        {
            auto batch = c1->doGet(FlightDescriptor::fromPath({"s2_only"}));
            static_cast<void>(batch);
        },
        std::runtime_error);

    (void)c1->close();
    (void)s1->stop();
    s2->stop();
}

// ===========================================================================
// Re-entrant callback safety (regression test for deadlock fix)
// ===========================================================================

TEST(ArrowFlightClientTest, DoGetProducerCanRegisterNewDataset) {
    // Regression test: a producer callback must be able to call
    // registerDataset on the server without deadlocking.  Before the fix the
    // registry mutex was held while the producer was invoked, causing a
    // deadlock whenever the producer tried to register another dataset.
    auto server = ArrowFlightServer::create(makeServerOpts(18900));
    server->start();

    // Producer that registers a second dataset during its invocation
    server->registerDataset({"trigger"}, [&server]() {
        // This call must NOT deadlock
        server->registerDataset({"dynamic"}, []() { return makeBatch(2); });
        return makeBatch(1);
    });

    auto client = ArrowFlightClient::connect(makeClientOpts(18900));
    // doGet("trigger") invokes the producer which registers "dynamic"
    auto batch = client->doGet(FlightDescriptor::fromPath({"trigger"}));
    EXPECT_EQ(batch.rowCount(), 1u);

    // The dynamically registered dataset should now be reachable
    auto batch2 = client->doGet(FlightDescriptor::fromPath({"dynamic"}));
    EXPECT_EQ(batch2.rowCount(), 2u);

    client->close();
    server->stop();
}

TEST(ArrowFlightClientTest, DoPutHandlerCanRegisterNewDataset) {
    // Regression test: a put handler must be able to call registerDataset
    // on the server without deadlocking.
    auto server = ArrowFlightServer::create(makeServerOpts(18901));
    server->start();

    server->registerPutHandler({"loader"}, [&server](RecordBatch b) {
        // Register the received data as a readable dataset – must NOT deadlock
        server->registerDataset({"loaded"}, [b]() { return b; });
    });

    auto client = ArrowFlightClient::connect(makeClientOpts(18901));
    auto pushed = makeBatch(4);
    auto result = client->doPut(pushed, FlightDescriptor::fromPath({"loader"}));
    EXPECT_TRUE(result.success);

    // The handler should have registered "loaded" with the pushed data
    auto got = client->doGet(FlightDescriptor::fromPath({"loaded"}));
    EXPECT_EQ(got.rowCount(), 4u);

    client->close();
    server->stop();
}
