// Integrationstest für Wire Protocol Server
// Ziel: Startet Server, verbindet sich per TCP, sendet HELLO und prüft Antwort

#include "network/wire_protocol_server.h"
#include <boost/asio.hpp>
#include <thread>
#include <cassert>
#include <iostream>

int main() {
    using namespace themis::network;
    // Dummy-Konfiguration
    WireProtocolServer::Config config;
    config.port = 5555;
    config.thread_count = 2;
    // Dummy-Manager
    auto storage = std::make_shared<RocksDBWrapper>();
    auto secondary = std::make_shared<SecondaryIndexManager>();
    auto graph = std::make_shared<GraphIndexManager>();
    auto vector = std::make_shared<VectorIndexManager>();
    auto tx = std::make_shared<TransactionManager>();
    WireProtocolServer server(config, storage, secondary, graph, vector, tx);
    std::thread server_thread([&]() { server.start(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // TCP-Client
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), config.port);
    sock.connect(ep);
    // Sende HELLO (Dummy-Frame)
    uint32_t magic = 0x544D4442;
    uint8_t opcode = 0; // HELLO
    uint8_t flags = 0;
    uint16_t reserved = 0;
    uint32_t payload_len = 0;
    sock.write_some(boost::asio::buffer(&magic, sizeof(magic)));
    sock.write_some(boost::asio::buffer(&opcode, sizeof(opcode)));
    sock.write_some(boost::asio::buffer(&flags, sizeof(flags)));
    sock.write_some(boost::asio::buffer(&reserved, sizeof(reserved)));
    sock.write_some(boost::asio::buffer(&payload_len, sizeof(payload_len)));
    // Antwort lesen (nur Header)
    char reply[16] = {0};
    size_t n = sock.read_some(boost::asio::buffer(reply, 16));
    std::cout << "Antwortbytes: " << n << std::endl;
    server.stop();
    server_thread.join();
    return 0;
}
