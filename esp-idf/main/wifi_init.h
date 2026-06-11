#ifndef WIFI_INIT_H
#define WIFI_INIT_H

#include <functional>

namespace BotiEyes
{
    namespace wifi
    {

        /**
         * WiFi connection status callback
         * Called when WiFi state changes
         */
        using StatusCallback = std::function<void(bool connected, const char *ip)>;

        /**
         * Initialize WiFi in station mode
         * Starts connection attempt to configured SSID
         *
         * @param status_cb Optional callback for connection status updates
         * @return true if initialization started successfully
         */
        bool initialize(StatusCallback status_cb = nullptr);

        /**
         * Check if WiFi is currently connected
         */
        bool isConnected();

        /**
         * Get current IP address (or nullptr if not connected)
         */
        const char *getIPAddress();

    } // namespace wifi
} // namespace BotiEyes

#endif // WIFI_INIT_H
