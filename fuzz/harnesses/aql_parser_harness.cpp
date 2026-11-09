#include "query/aql_parser.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

namespace {
constexpr std::size_t kMaxInputBytes = 1024 * 1024;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 2;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::perror("Unable to open AQL corpus input");
        return 1;
    }
    std::string query(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (query.size() > kMaxInputBytes) {
        std::fprintf(stderr, "AQL corpus input exceeds the 1 MiB harness limit\n");
        return 1;
    }

    themis::query::AQLParser parser;
    (void)parser.parse(query);
    return 0;
}
