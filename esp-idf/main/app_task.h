/**
 * @file app_task.h
 * @brief Application task declarations for BotiEyes ESP-IDF firmware
 *
 * This file declares the main application task that handles:
 * - BotiEyes instance lifecycle
 * - Network command processing from queue
 * - Display rendering loop
 * - Watchdog feeding
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Main application task function
     *
     * Runs on Core 1 with NORMAL priority. Responsible for:
     * - Initializing BotiEyes instance
     * - Processing network commands from g_network_cmd_queue
     * - Updating BotiEyes state (emotions, positions, idle behavior)
     * - Rendering to display via HAL
     * - Feeding task watchdog
     *
     * @param arg Task parameter (unused)
     */
    void app_task(void *arg);

#ifdef __cplusplus
}
#endif
