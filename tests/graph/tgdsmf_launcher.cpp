#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    // Launcher that invokes tgdist.exe with the focused gtest filter
    // It lives in the same output directory as tgdist.exe when installed by CMake.
    const std::string exe = "tgdist.exe";
    const std::string filter = "--gtest_filter=DistributedGraphSharedMutexStressTest.*";

    std::string cmd = exe + " " + filter;
    if (argc > 1) {
        // forward any additional args
        for (int i = 1; i < argc; ++i) {
            cmd += " ";
            cmd += argv[i];
        }
    }

    std::cout << "Running: " << cmd << std::endl;
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "tgdist returned: " << rc << std::endl;
    }
    return rc;
}
