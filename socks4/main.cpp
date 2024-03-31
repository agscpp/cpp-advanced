#include "socks4.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

#include <cactus/cactus.h>

constexpr std::chrono::duration kTimeout = std::chrono::seconds(1);

const cactus::SocketAddress kEndpoint{"8.8.8.8", 443};
const std::vector<Proxy> kProxies = {
    {cactus::SocketAddress("51.250.102.65", 12000), "prime"},
    {cactus::SocketAddress("4.4.4.4", 80), "prime"},
};
const std::string kFileName = "../socks4/flag.txt";

void WriteTo(const std::string& path, const std::string& data) {
    std::fstream stream(path, std::fstream::in | std::fstream::out | std::fstream::trunc);
    if (!stream.is_open()) {
        throw std::runtime_error("Problems opening file");
    }
    stream.write(data.c_str(), data.size());
    stream.close();
}

int main() {
    cactus::Scheduler sched;
    sched.Run([&] {
        try {
            cactus::TimeoutGuard guard{kTimeout};
            auto conn = DialProxyChain(kProxies, kEndpoint);
            WriteTo(kFileName, conn->ReadAllToString());
        } catch (...) {
        }
    });
}