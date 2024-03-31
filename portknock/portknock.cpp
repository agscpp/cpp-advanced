#include "portknock.h"

void KnockPorts(const cactus::SocketAddress& remote, const Ports& ports, cactus::Duration delay) {
    for (auto [port, protocol] : ports) {
        try {
            switch (protocol) {
                case KnockProtocol::TCP: {
                    cactus::DialTCP({remote.GetIp(), port});
                    break;
                }
                case KnockProtocol::UDP: {
                    auto lsn = cactus::ListenUDP(remote);
                    lsn->SendTo(cactus::ConstView(), {remote.GetIp(), port});
                    break;
                }
            }
            cactus::SleepFor(delay);
        } catch (...) {
        }
    }
}
