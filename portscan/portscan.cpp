#include "portscan.h"

#include <cactus/cactus.h>

#include <cstddef>
#include <memory>
#include <utility>

Ports ScanPorts(const cactus::SocketAddress& remote, uint16_t start, uint16_t end,
                cactus::Duration timeout) {
    Ports result;
    for (auto port : std::views::iota(start, end)) {
        try {
            cactus::TimeoutGuard guard{timeout};
            cactus::DialTCP({remote.GetIp(), port});
            result.emplace_back(port, PortState::OPEN);
        } catch (const cactus::TimeoutException& ex) {
            result.emplace_back(port, PortState::TIMEOUT);
        } catch (...) {
            result.emplace_back(port, PortState::CLOSED);
        }
    }
    return result;
}
