#include <chrono>
#include <iostream>
#include <ostream>

#include <cactus/cactus.h>

const std::string kHostIp = "51.250.102.65";
constexpr uint16_t kStartPort = 11'000;
constexpr uint16_t kEndPort = 11'200;
constexpr std::chrono::duration kTimeout = std::chrono::seconds(1);

int main() {
    cactus::Scheduler sched;
    sched.Run([&] {
        for (auto port : std::views::iota(kStartPort, kEndPort)) {
            try {
                cactus::TimeoutGuard guard{kTimeout};
                auto conn = cactus::DialTCP({kHostIp, port});
                std::cout << conn->ReadAllToString() << std::endl;
            } catch (...) {
            }
        }
    });
}