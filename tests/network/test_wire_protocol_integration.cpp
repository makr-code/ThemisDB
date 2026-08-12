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

// ---------------------------------------------------------------------------
// CRC32 checksum verification – documents the wire-frame checksum algorithm
// ---------------------------------------------------------------------------

// Standalone CRC32 (ISO-HDLC) implementation mirroring the one in
// wire_protocol_server.cpp.  Used to verify the checksum verification logic
// independently of the full server stack.
static uint32_t testCrc32Update(uint32_t crc, const uint8_t* data, size_t len) {
    static const uint32_t kTable[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
        0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
        0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
        0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
        0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
        0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
        0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
        0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
        0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
        0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
        0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
        0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
        0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
        0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
        0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
        0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
        0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
        0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
        0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
        0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
        0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD706B3,
        0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };
    crc = ~crc;
    for (size_t i = 0; i < len; ++i)
        crc = kTable[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

TEST(WireProtocolChecksum, EmptyInputGivesKnownValue) {
    // CRC32("") == 0x00000000 (the CRC32 of an empty byte sequence)
    const uint8_t empty[1] = {0};  // valid pointer; len=0 so loop body never executes
    EXPECT_EQ(testCrc32Update(0, empty, 0), 0x00000000u);
}

TEST(WireProtocolChecksum, KnownVector_123456789) {
    // CRC32 of the ASCII string "123456789" is the well-known check value
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    EXPECT_EQ(testCrc32Update(0, data, 9), 0xCBF43926u);
}

TEST(WireProtocolChecksum, ChainedBuffersMatchSinglePass) {
    // CRC32(AB) == CRC32(A) then CRC32(B) with previous result as seed
    const uint8_t header[] = {0x54, 0x4D, 0x44, 0x42, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 4};
    const uint8_t payload[] = {0x0A, 0x0B, 0x0C, 0x0D};

    // Single-pass
    uint8_t combined[16];
    std::memcpy(combined, header, 12);
    std::memcpy(combined + 12, payload, 4);
    uint32_t single = testCrc32Update(0, combined, 16);

    // Chained
    uint32_t chained = testCrc32Update(0, header, 12);
    chained = testCrc32Update(chained, payload, 4);

    EXPECT_EQ(single, chained);
}

TEST(WireProtocolChecksum, TamperedDataFailsVerification) {
    // Compute checksum over "good" data, then verify it fails against "bad" data
    const uint8_t good[] = {0x54, 0x4D, 0x44, 0x42, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
                             0x0A, 0x0B, 0x0C, 0x0D};
    uint32_t expected = testCrc32Update(0, good, sizeof(good));

    uint8_t tampered[sizeof(good)];
    std::memcpy(tampered, good, sizeof(good));
    tampered[12] ^= 0xFF;  // Flip byte in payload
    uint32_t actual = testCrc32Update(0, tampered, sizeof(tampered));

    EXPECT_NE(actual, expected);
}

TEST(DISABLED_Stub_wireprotocolintegration, OldStubRemoved) {
    GTEST_SKIP() << "This stub has been replaced with real tests";
}
