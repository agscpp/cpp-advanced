#include "socks4.h"

#include <netinet/in.h>
#include <cstddef>
#include <stdexcept>

namespace {

struct Socks4Data {
    uint8_t ver;
    uint8_t cmd;
    uint16_t dstport;
    uint32_t dstip;
};

}  // namespace

void Sock4Connect(cactus::IConn* conn, const cactus::SocketAddress& endpoint,
                  const std::string& user) {
    {
        Socks4Data request(0x04, 0x01, htons(endpoint.GetPort()), endpoint.GetIp());
        conn->Write(cactus::View(request));
        conn->Write(cactus::View(user.c_str(), user.size() + 1));
    }
    {
        Socks4Data response;
        conn->ReadFull(cactus::View(response));
        if (response.ver != 0 || response.cmd != 90) {
            throw std::runtime_error("Connection establishment error");
        }
    }
}

std::unique_ptr<cactus::IConn> DialProxyChain(const std::vector<Proxy>& proxies,
                                              const cactus::SocketAddress& endpoint) {
    if (proxies.empty()) {
        return cactus::DialTCP(endpoint);
    }
    auto [point, username] = proxies.front();
    std::unique_ptr<cactus::IConn> conn = cactus::DialTCP(point);
    for (size_t i = 1; i < proxies.size(); ++i) {
        Sock4Connect(conn.get(), proxies[i].proxy_address, username);
        username = proxies[i].username;
    }
    Sock4Connect(conn.get(), endpoint, username);
    return conn;
}
