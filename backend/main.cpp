#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <thread>

#include "httplib.h"

namespace {

void openBrowser() {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
#ifdef _WIN32
    std::system("start http://localhost:8080");
#elif defined(__APPLE__)
    std::system("open http://localhost:8080");
#else
    std::system("xdg-open http://localhost:8080");
#endif
}

}  // namespace

int main(int argc, char** argv) {
    std::filesystem::path exeDir = std::filesystem::absolute(argv[0]).parent_path();
    std::filesystem::path webDir = exeDir / "web";

    httplib::Server server;
    server.set_mount_point("/", webDir.string());

    std::thread(openBrowser).detach();
    server.listen("localhost", 8080);
    return 0;
}
