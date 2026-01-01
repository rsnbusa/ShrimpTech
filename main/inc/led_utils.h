#ifndef LED_UTILS_H_
#define LED_UTILS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Blink LED task for root node
 * 
 * @param pArg Blink interval in milliseconds (passed as void*)
 */
void blinkRoot(void *pArg);

/**
 * @brief Blink LED task for configuration mode
 * 
 * Alternates between WiFi LED and heartbeat LED
 * 
 * @param pArg Blink interval in milliseconds (passed as void*)
 */
void blinkConf(void *pArg);

#ifdef __cplusplus
}
#endif

#endif /* LED_UTILS_H_ */
