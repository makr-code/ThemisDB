#include "websocket_binary_frame_parser.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 2;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::perror("Unable to open wire protocol corpus input");
        return 1;
    }
    const auto begin = std::istreambuf_iterator<char>(input);
    const auto end = std::istreambuf_iterator<char>();
    std::vector<uint8_t> bytes(begin, end);
    (void)themis::network::detail::parseWebSocketBinaryFrame(bytes);
    return 0;
}
