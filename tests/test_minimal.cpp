// Minimal test - NO dependencies
#include <gtest/gtest.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif
#include <cstring>

TEST(MinimalTest, ReachesTestBody) {
    const char* msg = "Minimal binary reached main()\n";
#ifdef _WIN32
    _write(2, msg, static_cast<unsigned int>(strlen(msg)));
#else
    write(2, msg, strlen(msg));
#endif
    SUCCEED();
}
