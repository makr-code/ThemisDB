/**
 * @file buffer_binary_protocol.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <string>

// Forward declarations in themis namespace
namespace themis {
class TSStore;
class VectorIndexManager;
class PropertyGraphManager;
class TSAutoBuffer;
class VectorAutoBuffer;
class GraphAutoBuffer;
}


namespace themisdb {
namespace server {

/**
 * Binary Protocol Handler for AutoBuffer Operations
 * 
 * Wire Protocol Message Types:
 * - 0x70: TS_PUT_BUFFERED - Buffered time series data point
 * - 0x71: TS_PUT_BUFFERED_BATCH - Buffered batch of time series points
 * - 0x72: VECTOR_ADD_BUFFERED - Buffered vector add operation
 * - 0x73: VECTOR_UPDATE_BUFFERED - Buffered vector update operation
 * - 0x74: VECTOR_REMOVE_BUFFERED - Buffered vector remove operation
 * - 0x75: GRAPH_NODE_BUFFERED - Buffered graph node add
 * - 0x76: GRAPH_EDGE_BUFFERED - Buffered graph edge add
 * - 0x77: BUFFER_STATS - Get buffer statistics
 * - 0x78: BUFFER_FLUSH - Manual buffer flush
 * 
 * Message Format:
 * [1 byte: opcode] [4 bytes: payload length] [N bytes: payload (MessagePack)]
 * 
 * Response Format:
 * [1 byte: status] [4 bytes: payload length] [N bytes: payload (MessagePack)]
 * 
 * Status Codes:
 * - 0x00: Success
 * - 0x01: Invalid opcode
 * - 0x02: Malformed payload
 * - 0x03: Processing error
 * - 0x04: Buffer overflow
 */
class BufferBinaryProtocolHandler {
public:
    // Opcodes
    static constexpr uint8_t TS_PUT_BUFFERED = 0x70;
    static constexpr uint8_t TS_PUT_BUFFERED_BATCH = 0x71;
    static constexpr uint8_t VECTOR_ADD_BUFFERED = 0x72;
    static constexpr uint8_t VECTOR_UPDATE_BUFFERED = 0x73;
    static constexpr uint8_t VECTOR_REMOVE_BUFFERED = 0x74;
    static constexpr uint8_t GRAPH_NODE_BUFFERED = 0x75;
    static constexpr uint8_t GRAPH_EDGE_BUFFERED = 0x76;
    static constexpr uint8_t BUFFER_STATS = 0x77;
    static constexpr uint8_t BUFFER_FLUSH = 0x78;
    
    // Status codes
    static constexpr uint8_t STATUS_SUCCESS = 0x00;
    static constexpr uint8_t STATUS_INVALID_OPCODE = 0x01;
    static constexpr uint8_t STATUS_MALFORMED_PAYLOAD = 0x02;
    static constexpr uint8_t STATUS_PROCESSING_ERROR = 0x03;
    static constexpr uint8_t STATUS_BUFFER_OVERFLOW = 0x04;
    
    /**
     * Constructor
     * 
     * @param tsstore TSStore instance for time series operations
     * @param vector_index VectorIndexManager instance for vector operations
     * @param property_graph PropertyGraph instance for graph operations
     */
    BufferBinaryProtocolHandler(
        std::shared_ptr<themis::TSStore> tsstore,
        std::shared_ptr<themis::VectorIndexManager> vector_index,
        std::shared_ptr<themis::PropertyGraphManager> property_graph
    );
    
    ~BufferBinaryProtocolHandler();
    
    /**
     * Start the buffer binary protocol handler
     * Initializes AutoBuffer instances and starts background flush threads
     */
    void start();
    
    /**
     * Stop the buffer binary protocol handler
     * Flushes all remaining data and stops background threads
     */
    void stop();
    
    /**
     * Handle a binary protocol message
     * 
     * @param opcode Message opcode (0x70-0x78)
     * @param payload Message payload (MessagePack encoded)
     * @return Response message (status + optional payload)
     */
    std::vector<uint8_t> handleMessage(uint8_t opcode, const std::vector<uint8_t>& payload);
    
private:
    // Component references
    std::shared_ptr<themis::TSStore> tsstore_;
    std::shared_ptr<themis::VectorIndexManager> vector_index_;
    std::shared_ptr<themis::PropertyGraphManager> property_graph_;
    
    // AutoBuffer instances
    std::unique_ptr<themis::TSAutoBuffer> ts_buffer_;
    std::unique_ptr<themis::VectorAutoBuffer> vector_buffer_;
    std::unique_ptr<themis::GraphAutoBuffer> graph_buffer_;
    
    // Running state
    bool running_;
    
    // Opcode handlers
    std::vector<uint8_t> handleTSPutBuffered(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> handleTSPutBufferedBatch(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> handleVectorAddBuffered(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> handleVectorUpdateBuffered(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> handleVectorRemoveBuffered(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> handleGraphNodeBuffered(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> handleGraphEdgeBuffered(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> handleBufferStats(const std::vector<uint8_t>& payload);
    std::vector<uint8_t> handleBufferFlush(const std::vector<uint8_t>& payload);
    
    // Helper methods
    std::vector<uint8_t> createResponse(uint8_t status, const std::vector<uint8_t>& payload = {});
    std::vector<uint8_t> createErrorResponse(uint8_t status, const std::string& error_message);
};

} // namespace server
} // namespace themisdb
