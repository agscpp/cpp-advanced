#include "fanout.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

std::string Fanout(const cactus::SocketAddress& address) {
    auto conn = cactus::DialTCP(address);
    std::vector<std::unique_ptr<cactus::IConn>> child_conns;
    size_t number_tokens = 0;

    conn->ReadFull(cactus::View(number_tokens));
    for (size_t index = 1; index <= number_tokens; ++index) {
        auto child_conn = cactus::DialTCP({
            address.GetIp(),
            static_cast<uint16_t>(address.GetPort() + index),
        });
        uint64_t token;
        conn->ReadFull(cactus::View(token));
        child_conn->Write(cactus::View(token));
        child_conns.emplace_back(std::move(child_conn));
    }

    uint64_t sum = 0;
    for (const auto& child_conn : child_conns) {
        uint64_t secret;
        child_conn->ReadFull(cactus::View(secret));
        sum += secret;
    }
    conn->Write(cactus::View(sum));
    return conn->ReadAllToString();
}
