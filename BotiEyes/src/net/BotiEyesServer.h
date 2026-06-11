#ifndef BOTIEYES_NET_SERVER_H
#define BOTIEYES_NET_SERVER_H

// BotiEyesServer — ESP32 UDP transport adapter that binds the platform-agnostic
// CommandCodec + SessionManager to a live BotiEyes instance over Wi-Fi.
//
// This file is intentionally gated on ESP32 so that non-ESP32 builds and the
// existing examples are completely unaffected (FR-020). The pure codec/session
// logic in CommandCodec.* / SessionManager.* remains host-testable on its own.

#if defined(ESP32) || defined(ESP_PLATFORM)

#include <stdint.h>
#include <stddef.h>

#if defined(ESP_PLATFORM)
#include <sys/socket.h>
#include <netinet/in.h>
#elif defined(ESP32)
#include <WiFiUdp.h>
#endif

#include "../BotiEyes.h"
#include "NetProtocol.h"
#include "CommandCodec.h"
#include "SessionManager.h"

namespace BotiEyes {
namespace net {

/**
 * Non-blocking UDP control server for a BotiEyes instance.
 *
 * Usage (in an embedded app):
 *   BotiEyes::net::BotiEyesServer server(eyes);
 *   ...
 *   server.begin();          // after WiFi is connected
 *   loop() {
 *     server.poll();         // drain inbound datagrams (never blocks)
 *     server.applyPending(); // apply newest-wins targets + one-shot actions
 *     eyes.update();         // render a frame
 *   }
 */
class BotiEyesServer {
public:
    explicit BotiEyesServer(BotiEyes& eyes);

    // Bind the UDP socket. Call after Wi-Fi is connected.
    void begin(uint16_t port = DEFAULT_UDP_PORT);

    // Drain all currently-available datagrams, decode and route them.
    // Non-blocking: returns immediately when no datagrams are pending.
    void poll();

    // Apply the coalesced newest-wins targets and queued one-shot actions to
    // the BotiEyes instance, then clear them. Also drives idle fallback when
    // the controller lock times out. Call once per render frame.
    void applyPending();

    // True while no controller holds a live lock (idle/autonomous mode).
    bool isIdle() const { return idleFallbackActive_; }

private:
    void handleDatagram(const uint8_t* buf, size_t len,
                        uint32_t ip, uint16_t port, uint32_t nowMs);
    void sendAck(uint32_t ip, uint16_t port, uint32_t seq,
                 bool accepted, uint8_t reason);
    void sendStatus(uint32_t ip, uint16_t port);

    BotiEyes&      eyes_;
#if defined(ESP_PLATFORM)
    int            udpSock_;
#elif defined(ESP32)
    WiFiUDP        udp_;
#endif
    SessionManager session_;
    uint16_t       port_;
    bool           started_;
    bool           idleFallbackActive_;
    uint32_t       startMs_;
};

} // namespace net
} // namespace BotiEyes

#endif // ESP32 || ESP_PLATFORM
#endif // BOTIEYES_NET_SERVER_H
