#include <gtest/gtest.h>

// Disable legacy binary protocol buffer tests
#if 0
#include "server/buffer_binary_protocol.h"
#include "timeseries/tsstore.h"
#include "index/vector_index_manager.h"
#include "index/property_graph.h"
#include <msgpack.hpp>
#include <thread>
#include <vector>

using namespace themisdb::server;

class BufferBinaryProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock components
        tsstore_ = std::make_shared<TSStore>();
        vector_index_ = std::make_shared<VectorIndexManager>();
        property_graph_ = std::make_shared<PropertyGraph>();
        
        // Create handler
        handler_ = std::make_unique<BufferBinaryProtocolHandler>(
            tsstore_,
            vector_index_,
            property_graph_
        );
        
        // Start handler
        handler_->start();
    }
    
    void TearDown() override {
        handler_->stop();
    }
    
    std::vector<uint8_t> createTSPutBufferedMessage(
        const std::string& metric,
        const std::string& entity,
        int64_t timestamp,
        double value
    ) {
        msgpack::sbuffer sbuf;
        msgpack::packer<msgpack::sbuffer> packer(sbuf);
        
        packer.pack_map(4);
        packer.pack("metric");
        packer.pack(metric);
        packer.pack("entity");
        packer.pack(entity);
        packer.pack("timestamp");
        packer.pack(timestamp);
        packer.pack("value");
        packer.pack(value);
        
        return std::vector<uint8_t>(sbuf.data(), sbuf.data() + sbuf.size());
    }
    
    std::vector<uint8_t> createVectorRemoveMessage(const std::string& pk) {
        msgpack::sbuffer sbuf;
        msgpack::packer<msgpack::sbuffer> packer(sbuf);
        
        packer.pack_map(1);
        packer.pack("pk");
        packer.pack(pk);
        
        return std::vector<uint8_t>(sbuf.data(), sbuf.data() + sbuf.size());
    }
    
    std::vector<uint8_t> createBufferFlushMessage(const std::string& buffer) {
        msgpack::sbuffer sbuf;
        msgpack::packer<msgpack::sbuffer> packer(sbuf);
        
        packer.pack_map(1);
        packer.pack("buffer");
        packer.pack(buffer);
        
        return std::vector<uint8_t>(sbuf.data(), sbuf.data() + sbuf.size());
    }
    
    uint8_t extractStatus(const std::vector<uint8_t>& response) {
        EXPECT_GE(response.size(), 5);  // At least status + length
        return response[0];
    }
    
    std::shared_ptr<TSStore> tsstore_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<PropertyGraph> property_graph_;
    std::unique_ptr<BufferBinaryProtocolHandler> handler_;
};

TEST_F(BufferBinaryProtocolTest, TSPutBuffered_SinglePoint) {
    // Create message
    auto payload = createTSPutBufferedMessage("cpu.usage", "server01", 1700000000, 75.5);
    
    // Handle message
    auto response = handler_->handleMessage(
        BufferBinaryProtocolHandler::TS_PUT_BUFFERED,
        payload
    );
    
    // Verify response
    EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
}

TEST_F(BufferBinaryProtocolTest, TSPutBufferedBatch_MultiplePoints) {
    // Create batch message
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> packer(sbuf);
    
    packer.pack_array(100);  // 100 points
    for (int i = 0; i < 100; i++) {
        packer.pack_map(4);
        packer.pack("metric");
        packer.pack("cpu.usage");
        packer.pack("entity");
        packer.pack("server01");
        packer.pack("timestamp");
        packer.pack(1700000000 + i);
        packer.pack("value");
        packer.pack(75.5 + i * 0.1);
    }
    
    std::vector<uint8_t> payload(sbuf.data(), sbuf.data() + sbuf.size());
    
    // Handle message
    auto response = handler_->handleMessage(
        BufferBinaryProtocolHandler::TS_PUT_BUFFERED_BATCH,
        payload
    );
    
    // Verify response
    EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
    
    // Verify buffered count in payload
    uint32_t payload_len = (response[1] << 24) | (response[2] << 16) | 
                           (response[3] << 8) | response[4];
    EXPECT_GT(payload_len, 0);
}

TEST_F(BufferBinaryProtocolTest, VectorRemoveBuffered) {
    // Create message
    auto payload = createVectorRemoveMessage("doc123");
    
    // Handle message
    auto response = handler_->handleMessage(
        BufferBinaryProtocolHandler::VECTOR_REMOVE_BUFFERED,
        payload
    );
    
    // Verify response
    EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
}

TEST_F(BufferBinaryProtocolTest, BufferStats) {
    // Send some buffered operations first
    auto ts_payload = createTSPutBufferedMessage("cpu.usage", "server01", 1700000000, 75.5);
    handler_->handleMessage(BufferBinaryProtocolHandler::TS_PUT_BUFFERED, ts_payload);
    
    // Get stats
    std::vector<uint8_t> empty_payload;
    auto response = handler_->handleMessage(
        BufferBinaryProtocolHandler::BUFFER_STATS,
        empty_payload
    );
    
    // Verify response
    EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
    
    // Verify stats payload exists
    uint32_t payload_len = (response[1] << 24) | (response[2] << 16) | 
                           (response[3] << 8) | response[4];
    EXPECT_GT(payload_len, 0);
}

TEST_F(BufferBinaryProtocolTest, BufferFlush_All) {
    // Send some buffered operations
    auto ts_payload = createTSPutBufferedMessage("cpu.usage", "server01", 1700000000, 75.5);
    handler_->handleMessage(BufferBinaryProtocolHandler::TS_PUT_BUFFERED, ts_payload);
    
    // Flush all buffers
    auto flush_payload = createBufferFlushMessage("all");
    auto response = handler_->handleMessage(
        BufferBinaryProtocolHandler::BUFFER_FLUSH,
        flush_payload
    );
    
    // Verify response
    EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
}

TEST_F(BufferBinaryProtocolTest, BufferFlush_Specific) {
    // Flush only TS buffer
    auto flush_payload = createBufferFlushMessage("ts");
    auto response = handler_->handleMessage(
        BufferBinaryProtocolHandler::BUFFER_FLUSH,
        flush_payload
    );
    
    // Verify response
    EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
}

TEST_F(BufferBinaryProtocolTest, InvalidOpcode) {
    std::vector<uint8_t> payload;
    auto response = handler_->handleMessage(0xFF, payload);
    
    // Verify error response
    EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_INVALID_OPCODE);
}

TEST_F(BufferBinaryProtocolTest, MalformedPayload) {
    // Send invalid MessagePack data
    std::vector<uint8_t> invalid_payload = {0xFF, 0xFF, 0xFF};
    auto response = handler_->handleMessage(
        BufferBinaryProtocolHandler::TS_PUT_BUFFERED,
        invalid_payload
    );
    
    // Verify error response
    EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_MALFORMED_PAYLOAD);
}

TEST_F(BufferBinaryProtocolTest, ConcurrentOperations) {
    const int num_threads = 4;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; i++) {
                auto payload = createTSPutBufferedMessage(
                    "cpu.usage",
                    "server" + std::to_string(t),
                    1700000000 + i,
                    75.5 + i * 0.1
                );
                
                auto response = handler_->handleMessage(
                    BufferBinaryProtocolHandler::TS_PUT_BUFFERED,
                    payload
                );
                
                EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify stats show all operations
    std::vector<uint8_t> empty_payload;
    auto stats_response = handler_->handleMessage(
        BufferBinaryProtocolHandler::BUFFER_STATS,
        empty_payload
    );
    
    EXPECT_EQ(extractStatus(stats_response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
}

TEST_F(BufferBinaryProtocolTest, GraphNodeBuffered) {
    // Create graph node message
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> packer(sbuf);
    
    packer.pack_map(2);
    packer.pack("pk");
    packer.pack("user123");
    packer.pack("graph_id");
    packer.pack("social");
    
    std::vector<uint8_t> payload(sbuf.data(), sbuf.data() + sbuf.size());
    
    // Handle message
    auto response = handler_->handleMessage(
        BufferBinaryProtocolHandler::GRAPH_NODE_BUFFERED,
        payload
    );
    
    // Verify response
    EXPECT_EQ(extractStatus(response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
}

TEST_F(BufferBinaryProtocolTest, MultipleMessageTypes) {
    // Send TS buffered
    auto ts_payload = createTSPutBufferedMessage("cpu.usage", "server01", 1700000000, 75.5);
    auto ts_response = handler_->handleMessage(
        BufferBinaryProtocolHandler::TS_PUT_BUFFERED,
        ts_payload
    );
    EXPECT_EQ(extractStatus(ts_response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
    
    // Send vector remove
    auto vector_payload = createVectorRemoveMessage("doc123");
    auto vector_response = handler_->handleMessage(
        BufferBinaryProtocolHandler::VECTOR_REMOVE_BUFFERED,
        vector_payload
    );
    EXPECT_EQ(extractStatus(vector_response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
    
    // Get stats
    std::vector<uint8_t> empty_payload;
    auto stats_response = handler_->handleMessage(
        BufferBinaryProtocolHandler::BUFFER_STATS,
        empty_payload
    );
    EXPECT_EQ(extractStatus(stats_response), BufferBinaryProtocolHandler::STATUS_SUCCESS);
}

#endif // legacy binary protocol buffer tests

TEST(BufferBinaryProtocolTest, DISABLED_BinaryProtocolLegacy) {
    GTEST_SKIP() << "Binary protocol buffer tests disabled in this configuration";
}


