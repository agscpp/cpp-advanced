#include "portknock.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>

#include <cactus/cactus.h>

constexpr std::chrono::duration kDelay = std::chrono::seconds(0);
constexpr std::chrono::duration kTimeout = std::chrono::seconds(1);
constexpr uint16_t kDataPort = 10080;

const cactus::SocketAddress kRemoteHost{"51.250.102.65", 0};
const Ports kPingPorts = {
    {10001, KnockProtocol::TCP},
    {10022, KnockProtocol::TCP},
    {10003, KnockProtocol::TCP},
};
const std::string kFileName = "../portknock/flag.txt";

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
        KnockPorts(kRemoteHost, kPingPorts, kDelay);
        try {
            cactus::TimeoutGuard guard{kTimeout};
            auto conn = cactus::DialTCP({kRemoteHost.GetIp(), kDataPort});
            WriteTo(kFileName, conn->ReadAllToString());
        } catch (...) {
        }
    });
}