#include "fanout.h"

#include <cactus/cactus.h>
#include <fstream>

constexpr std::chrono::duration kTimeout = std::chrono::seconds(1);

const cactus::SocketAddress kEndpoint{"51.250.102.65", 13000};
const std::string kFileName = "../fanout/flag.txt";

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
            WriteTo(kFileName, Fanout(kEndpoint));
        } catch (...) {
        }
    });
}