// Minimal test - NO dependencies
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif
#include <cstring>

int main() {
    const char* msg = "Minimal binary reached main()\n";
#ifdef _WIN32
    _write(2, msg, static_cast<unsigned int>(strlen(msg)));
#else
    write(2, msg, strlen(msg));
#endif
    return 0;
}
