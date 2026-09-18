/**
 * @file http2_session.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef THEMIS_ENABLE_HTTP2

#include <nghttp2/nghttp2.h>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <set>
#include <mutex>

namespace themis {
namespace server {

namespace net = boost::asio;
using tcp = net::ip::tcp;

// Forward declarations
class HttpServer;

class Http2Session : public std::enable_shared_from_this<Http2Session> {
public:
    Http2Session(
        tcp::socket socket,
        boost::asio::ssl::context& ssl_ctx,
        HttpServer* server,
        uint32_t max_concurrent_streams,
        uint32_t initial_window_size,
        bool connection_slot_reserved = false
    );
    
    ~Http2Session();

    /**
     * @brief Start.
     */
    void start();

private:
    /**
     * @brief Do Handshake.
     */
    void doHandshake();
    /**
     * @brief On Handshake.
     * @param[in] ec Input parameter.
     */
    void onHandshake(boost::system::error_code ec);
    
    /**
     * @brief Do Read.
     */
    void doRead();
    /**
     * @brief On Read.
     * @param[in] ec Input parameter.
     * @param[in] bytes_transferred Input parameter.
     */
    void onRead(boost::system::error_code ec, std::size_t bytes_transferred);
    /**
     * @brief Do Write.
     */
    void doWrite();
    /**
     * @brief On Write.
     * @param[in] ec Input parameter.
     * @param[in] bytes_transferred Input parameter.
     */
    void onWrite(boost::system::error_code ec, std::size_t bytes_transferred);
    /**
     * @brief Arm Read Timer.
     */
    void armReadTimer();
    /**
     * @brief Cancel Read Timer.
     */
    void cancelReadTimer();
    /**
     * @brief Arm Write Timer.
     */
    void armWriteTimer();
    /**
     * @brief Cancel Write Timer.
     */
    void cancelWriteTimer();
    
    /**
     * @brief Send Callback.
     * @param[in,out] session Input/output parameter.
     * @param[in] data Input parameter.
     * @param[in] length Input parameter.
     * @param[in] flags Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static ssize_t sendCallback(nghttp2_session* session, const uint8_t* data,
                                size_t length, int flags, void* user_data);
    /**
     * @brief On Frame Recv Callback.
     * @param[in,out] session Input/output parameter.
     * @param[in] frame Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static int onFrameRecvCallback(nghttp2_session* session,
                                   const nghttp2_frame* frame, void* user_data);
    /**
     * @brief On Data Chunk Recv Callback.
     * @param[in,out] session Input/output parameter.
     * @param[in] flags Input parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static int onDataChunkRecvCallback(nghttp2_session* session, uint8_t flags,
                                       int32_t stream_id, const uint8_t* data,
                                       size_t len, void* user_data);
    /**
     * @brief On Stream Close Callback.
     * @param[in,out] session Input/output parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in] error_code Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static int onStreamCloseCallback(nghttp2_session* session, int32_t stream_id,
                                     uint32_t error_code, void* user_data);
    /**
     * @brief On Header Callback.
     * @param[in,out] session Input/output parameter.
     * @param[in] frame Input parameter.
     * @param[in] name Input parameter.
     * @param[in] namelen Input parameter.
     * @param[in] value Input parameter.
     * @param[in] valuelen Input parameter.
     * @param[in] flags Input parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static int onHeaderCallback(nghttp2_session* session,
                                const nghttp2_frame* frame,
                                const uint8_t* name, size_t namelen,
                                const uint8_t* value, size_t valuelen,
                                uint8_t flags, void* user_data);
    /**
     * @brief Response Data Read Callback.
     * @param[in,out] session Input/output parameter.
     * @param[in] stream_id Identifier of the stream.
     * @param[in,out] buf Input/output parameter.
     * @param[in] length Input parameter.
     * @param[in,out] data_flags Input/output parameter.
     * @param[in,out] source Input/output parameter.
     * @param[in,out] user_data Input/output parameter.
     * @return Return value.
     */
    static ssize_t responseDataReadCallback(nghttp2_session* session, int32_t stream_id,
                                            uint8_t* buf, size_t length, uint32_t* data_flags,
                                            nghttp2_data_source* source, void* user_data);
    
    // Stream data management
    struct StreamData {
        int32_t stream_id;
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        bool headers_complete = false;
        bool cdc_subscribed = false;
        uint64_t cdc_last_sequence = 0;
    };

    struct ResponseBuffer {
        std::string data;
        size_t offset = 0;
    };
    
    /**
     * @brief Process Stream.
     * @param[in] stream_id Identifier of the stream.
     */
    void processStream(int32_t stream_id);
    void sendResponse(int32_t stream_id, int status, 
                      const std::string& body,
                      const std::unordered_map<std::string, std::string>& headers = {});
    
    // HTTP/2 Server Push for CDC
    void sendServerPush(int32_t stream_id, const std::string& push_path, 
                        const std::string& body,
                        const std::unordered_map<std::string, std::string>& headers = {});
    /**
     * @brief Subscribe To CDC.
     * @param[in] stream_id Identifier of the stream.
     */
    void subscribeToCDC(int32_t stream_id);
    /**
     * @brief Broadcast CDCEvent.
     * @param[in] event_data Input parameter.
     */
    void broadcastCDCEvent(const std::string& event_data);
    
    // Members
    boost::beast::ssl_stream<tcp::socket> stream_;
    HttpServer* server_;
    nghttp2_session* ng2_session_;
    
    std::array<uint8_t, 16384> read_buffer_;
    std::vector<uint8_t> write_buffer_;
    std::unordered_map<int32_t, StreamData> streams_;
    net::steady_timer read_timer_;  // handshake + read timeout guard
    net::steady_timer write_timer_; // write timeout guard
    
    uint32_t max_concurrent_streams_;
    uint32_t initial_window_size_;

    // Server Push state
    int32_t next_push_stream_id_;
    std::set<int32_t> cdc_subscribed_streams_;
    mutable std::mutex push_mutex_;
    mutable std::mutex response_mutex_;
    // Per-stream response buffers for RAII lifetime management (stub #298).
    std::unordered_map<int32_t, std::shared_ptr<ResponseBuffer>> response_buffers_;
};

class Http2Handler {
public:
    /**
     * @brief Configure Alpn.
     * @param[in,out] ssl_ctx Input/output parameter.
     */
    static void configureAlpn(boost::asio::ssl::context& ssl_ctx);
    
    /**
     * @brief Is Http2 Negotiated.
     * @param[in,out] ssl Input/output parameter.
     * @return True when the operation succeeds.
     */
    static bool isHttp2Negotiated(SSL* ssl);
    
    static std::shared_ptr<Http2Session> createSession(
        tcp::socket socket,
        boost::asio::ssl::context& ssl_ctx,
        HttpServer* server,
        uint32_t max_concurrent_streams = 100,
        uint32_t initial_window_size = 65535,
        bool connection_slot_reserved = false
    );
};

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_HTTP2
