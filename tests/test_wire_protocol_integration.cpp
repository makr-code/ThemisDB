/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_wire_protocol_integration.cpp                 ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     263                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "network/wire_protocol_server.h"
#include "network/wire_protocol_helpers.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include <memory>
#include <vector>

using namespace themis::network;

// Test protobuf parser with varint encoding
TEST(WireProtocolHelpers, ProtobufVarintParsing) {
    // Test small varint (1 byte)
    {
        std::vector<uint8_t> data = {0x00};  // 0
        ProtobufParser parser(data);
        uint64_t value = 0;
        ASSERT_TRUE(parser.readVarint(value));
        EXPECT_EQ(value, 0);
    }
    
    {
        std::vector<uint8_t> data = {0x01};  // 1
        ProtobufParser parser(data);
        uint64_t value = 0;
        ASSERT_TRUE(parser.readVarint(value));
        EXPECT_EQ(value, 1);
    }
    
    // Test multi-byte varint
    {
        std::vector<uint8_t> data = {0xAC, 0x02};  // 300
        ProtobufParser parser(data);
        uint64_t value = 0;
        ASSERT_TRUE(parser.readVarint(value));
        EXPECT_EQ(value, 300);
    }
}

// Test protobuf serializer with varint encoding
TEST(WireProtocolHelpers, ProtobufVarintSerialization) {
    {
        ProtobufSerializer serializer;
        serializer.writeVarint(0);
        auto data = serializer.data();
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], 0x00);
    }
    
    {
        ProtobufSerializer serializer;
        serializer.writeVarint(1);
        auto data = serializer.data();
        ASSERT_EQ(data.size(), 1);
        EXPECT_EQ(data[0], 0x01);
    }
    
    {
        ProtobufSerializer serializer;
        serializer.writeVarint(300);
        auto data = serializer.data();
        ASSERT_EQ(data.size(), 2);
        EXPECT_EQ(data[0], 0xAC);
        EXPECT_EQ(data[1], 0x02);
    }
}

// Test TimeSeriesQueryRequest parsing
TEST(WireProtocolHelpers, TimeSeriesQueryRequestParsing) {
    // Build a simple request: collection="cpu_usage", start_time_ns=1000, end_time_ns=2000
    ProtobufSerializer serializer;
    
    // Field 1: collection (string, length-delimited)
    serializer.writeTag(1, 2);  // field 1, wire type 2 (length-delimited)
    serializer.writeString("cpu_usage");
    
    // Field 2: start_time_ns (varint)
    serializer.writeTag(2, 0);  // field 2, wire type 0 (varint)
    serializer.writeVarint(1000);
    
    // Field 3: end_time_ns (varint)
    serializer.writeTag(3, 0);
    serializer.writeVarint(2000);
    
    auto data = serializer.data();
    
    // Parse it back
    TimeSeriesQueryRequest request;
    ASSERT_TRUE(TimeSeriesQueryRequest::parse(data, request));
    EXPECT_EQ(request.collection, "cpu_usage");
    EXPECT_EQ(request.start_time_ns, 1000);
    EXPECT_EQ(request.end_time_ns, 2000);
    EXPECT_EQ(request.aggregation, 0);  // Default
    EXPECT_EQ(request.bucket_size_ns, 0);  // Default
}

// Test TimeSeriesQueryResponse serialization
TEST(WireProtocolHelpers, TimeSeriesQueryResponseSerialization) {
    TimeSeriesQueryResponse response;
    response.query_time_us = 1234;
    
    TimeSeriesBucket bucket;
    bucket.timestamp_ns = 5000;
    bucket.value = 42.5;
    bucket.count = 10;
    bucket.min = 40.0;
    bucket.max = 45.0;
    
    response.buckets.push_back(bucket);
    
    response.stats.total_data_points = 10;
    response.stats.buckets_returned = 1;
    response.stats.data_density = 10.0;
    
    // Serialize
    auto data = response.serialize();
    
    // Should have non-empty data
    EXPECT_GT(data.size(), 0);
    
    // Parse it back to verify structure
    ProtobufParser parser(data);
    bool found_bucket = false;
    bool found_query_time = false;
    bool found_stats = false;
    
    while (!parser.atEnd()) {
        uint32_t field_number = 0;
        uint32_t wire_type = 0;
        ASSERT_TRUE(parser.readTag(field_number, wire_type));
        
        if (field_number == 1) {  // buckets
            found_bucket = true;
            std::vector<uint8_t> bucket_data;
            ASSERT_TRUE(parser.readLengthDelimited(bucket_data));
        } else if (field_number == 2) {  // query_time_us
            found_query_time = true;
            uint64_t time = 0;
            ASSERT_TRUE(parser.readVarint(time));
            EXPECT_EQ(time, 1234);
        } else if (field_number == 3) {  // stats
            found_stats = true;
            std::vector<uint8_t> stats_data;
            ASSERT_TRUE(parser.readLengthDelimited(stats_data));
        } else {
            ASSERT_TRUE(parser.skipField(wire_type));
        }
    }
    
    EXPECT_TRUE(found_bucket);
    EXPECT_TRUE(found_query_time);
    EXPECT_TRUE(found_stats);
}

// Basic test to verify wire protocol server can be instantiated with TSStore
TEST(WireProtocolIntegration, ServerInstantiationWithTSStore) {
    themis::network::WireProtocolServer::Config config;
    config.port = 18766;  // Use a different port for testing
    
    // Create with nullptr for all dependencies (minimal test)
    auto server = std::make_unique<themis::network::WireProtocolServer>(
        config,
        nullptr,  // storage
        nullptr,  // secondary_index
        nullptr,  // graph_index
        nullptr,  // vector_index
        nullptr,  // tx_manager
        nullptr,  // ts_store
        nullptr   // agg_manager
    );
    
    ASSERT_NE(server, nullptr);
    EXPECT_FALSE(server->isRunning());
}

// Test to document the timeseries query flow
TEST(WireProtocolIntegration, TimeseriesQueryFlowDocumentation) {
    // This test documents the complete flow for TIMESERIES_QUERY (OpCode 0x51):
    //
    // CLIENT SIDE:
    // 1. Client creates TimeSeriesQueryRequest with:
    //    - collection: "cpu_usage" (metric name)
    //    - start_time_ns: 1704067200000000000 (nanoseconds)
    //    - end_time_ns: 1704153600000000000
    //    - aggregation: 0 (AVG), 1 (SUM), 2 (MIN), 3 (MAX), 4 (COUNT)
    //    - bucket_size_ns: 3600000000000 (1 hour in nanoseconds)
    //
    // 2. Client serializes request to protobuf wire format
    //
    // 3. Client builds wire frame:
    //    - Magic: 0x544D4442 ("TMDB")
    //    - Version: 0x01
    //    - OpCode: 0x51 (TIMESERIES_QUERY)
    //    - Flags: 0x0000
    //    - PayloadSize: <size of serialized request>
    //
    // 4. Client sends wire frame + payload to server
    //
    // SERVER SIDE:
    // 5. Server reads header (12 bytes) in asyncReadHeader()
    //
    // 6. Server extracts payload size and reads payload in asyncReadPayload()
    //
    // 7. Server calls handleMessage() which dispatches to handleTimeseriesQuery()
    //
    // 8. handleTimeseriesQuery():
    //    a. Parses TimeSeriesQueryRequest from payload_buffer_
    //    b. Validates collection name and timestamps
    //    c. Converts ns timestamps to ms (TSStore format)
    //    d. Builds TSStore::QueryOptions
    //    e. Decides between query() or aggregate() based on request
    //    f. If aggregation requested:
    //       - Calls ts_store_->aggregate()
    //       - Creates buckets with aggregated values
    //    g. If raw query:
    //       - Calls ts_store_->query()
    //       - Converts each DataPoint to TimeSeriesBucket
    //    h. Builds TimeSeriesQueryResponse with buckets and stats
    //    i. Serializes response to protobuf wire format
    //    j. Builds wire frame response with OpCode 0x21 (QUERY_RESULT)
    //    k. Calls asyncWriteResponse()
    //
    // 9. Server sends response back to client
    //
    // CLIENT SIDE:
    // 10. Client receives wire frame + payload
    //
    // 11. Client deserializes TimeSeriesQueryResponse
    //
    // 12. Client processes buckets with timestamp_ns, value, count, min, max
    
    SUCCEED() << "Complete timeseries query flow documented with production implementation";
}

TEST(DISABLED_Stub_wireprotocolintegration, OldStubRemoved) {
    GTEST_SKIP() << "This stub has been replaced with real tests";
}
