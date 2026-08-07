#ifndef BT_JAMMER_H
#define BT_JAMMER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the Bluetooth Classic jammer.
 * @note Disables Wi-Fi and Bluetooth controller.
 * @return true if started successfully, false otherwise.
 */
bool startBtJammer(void);

/**
 * @brief Stop the Bluetooth Classic jammer.
 * @return true if stopped successfully, false if not running.
 */
bool stopBtJammer(void);

/**
 * @brief Check if jammer is currently running.
 * @return true if active, false otherwise.
 */
bool btJammerIsActive(void);

/**
 * @brief Toggle jammer on/off.
 * @return new state (true = running, false = stopped).
 */
bool btJammerToggle(void);

#ifdef __cplusplus
}
#endif

#endif // BT_JAMMER_H
