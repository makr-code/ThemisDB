/**
 * @file http2_session.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief HTTP/2 Session Handler
 * 
 * Manages a single HTTP/2 connection using nghttp2 library.
 * Handles multiplexed streams, HPACK compression, and flow control.
 */
class Http2Session : public std::enable_shared_from_this<Http2Session> {
public:
    /**
     * @brief Construct an HTTP/2 session bound to one accepted TLS socket.
     *
     * @param socket Accepted TCP socket already selected for HTTP/2 handling.
     * @param ssl_ctx TLS context used for the HTTP/2 TLS handshake.
     * @param server Owning HTTP server instance; must remain valid for the
     *        session lifetime.
     * @param max_concurrent_streams Maximum concurrent HTTP/2 streams allowed.
     * @param initial_window_size Initial per-stream flow-control window size.
     * @param connection_slot_reserved When true, accept-path admission already
     *        reserved one `active_connections_` slot and the constructor must
     *        not increment it again. The destructor still releases the slot.
     */
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
     * @brief Start the HTTP/2 session
     * 
     * Performs TLS handshake with ALPN negotiation for "h2"
     */
    void start();

private:
    // TLS handshake with ALPN
    void doHandshake();
    void onHandshake(boost::system::error_code ec);
    
    // HTTP/2 session management
    void doRead();
    void onRead(boost::system::error_code ec, std::size_t bytes_transferred);
    void doWrite();
    void onWrite(boost::system::error_code ec, std::size_t bytes_transferred);
    void armReadTimer();
    void cancelReadTimer();
    void armWriteTimer();
    void cancelWriteTimer();
    
    // nghttp2 callbacks
    static ssize_t sendCallback(nghttp2_session* session, const uint8_t* data,
                                size_t length, int flags, void* user_data);
    static int onFrameRecvCallback(nghttp2_session* session,
                                   const nghttp2_frame* frame, void* user_data);
    static int onDataChunkRecvCallback(nghttp2_session* session, uint8_t flags,
                                       int32_t stream_id, const uint8_t* data,
                                       size_t len, void* user_data);
    static int onStreamCloseCallback(nghttp2_session* session, int32_t stream_id,
                                     uint32_t error_code, void* user_data);
    static int onHeaderCallback(nghttp2_session* session,
                                const nghttp2_frame* frame,
                                const uint8_t* name, size_t namelen,
                                const uint8_t* value, size_t valuelen,
                                uint8_t flags, void* user_data);
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
    
    void processStream(int32_t stream_id);
    void sendResponse(int32_t stream_id, int status, 
                      const std::string& body,
                      const std::unordered_map<std::string, std::string>& headers = {});
    
    // HTTP/2 Server Push for CDC
    void sendServerPush(int32_t stream_id, const std::string& push_path, 
                        const std::string& body,
                        const std::unordered_map<std::string, std::string>& headers = {});
    void subscribeToCDC(int32_t stream_id);
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

/**
 * @brief HTTP/2 Protocol Handler
 * 
 * Factory for creating HTTP/2 sessions with proper ALPN negotiation
 */
class Http2Handler {
public:
    /**
     * @brief Configure ALPN for HTTP/2 negotiation
     * 
     * Sets up "h2" protocol negotiation in TLS context
     */
    static void configureAlpn(boost::asio::ssl::context& ssl_ctx);
    
    /**
     * @brief Check if ALPN negotiated HTTP/2
     */
    static bool isHttp2Negotiated(SSL* ssl);
    
    /**
     * @brief Create HTTP/2 session
     */
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
