/**
 * @file buffer_binary_protocol.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
    
    BufferBinaryProtocolHandler(
        std::shared_ptr<themis::TSStore> tsstore,
        std::shared_ptr<themis::VectorIndexManager> vector_index,
        std::shared_ptr<themis::PropertyGraphManager> property_graph
    );
    
    ~BufferBinaryProtocolHandler();
    
    /**
     * @brief Start.
     */
    void start();
    
    /**
     * @brief Stop.
     */
    void stop();
    
    /**
     * @brief Handle Message.
     * @param[in] opcode Input parameter.
     * @param[in] payload Input parameter.
     * @return Return value.
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
    /**
     * @brief Handle TSPut Buffered.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> handleTSPutBuffered(const std::vector<uint8_t>& payload);
    /**
     * @brief Handle TSPut Buffered Batch.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> handleTSPutBufferedBatch(const std::vector<uint8_t>& payload);
    /**
     * @brief Handle Vector Add Buffered.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> handleVectorAddBuffered(const std::vector<uint8_t>& payload);
    /**
     * @brief Handle Vector Update Buffered.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> handleVectorUpdateBuffered(const std::vector<uint8_t>& payload);
    /**
     * @brief Handle Vector Remove Buffered.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> handleVectorRemoveBuffered(const std::vector<uint8_t>& payload);
    /**
     * @brief Handle Graph Node Buffered.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> handleGraphNodeBuffered(const std::vector<uint8_t>& payload);
    /**
     * @brief Handle Graph Edge Buffered.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> handleGraphEdgeBuffered(const std::vector<uint8_t>& payload);
    /**
     * @brief Handle Buffer Stats.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> handleBufferStats(const std::vector<uint8_t>& payload);
    /**
     * @brief Handle Buffer Flush.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> handleBufferFlush(const std::vector<uint8_t>& payload);
    
    // Helper methods
    std::vector<uint8_t> createResponse(uint8_t status, const std::vector<uint8_t>& payload = {});
    /**
     * @brief Create Error Response.
     * @param[in] status Input parameter.
     * @param[in] error_message Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> createErrorResponse(uint8_t status, const std::string& error_message);
};

} // namespace server
} // namespace themisdb
