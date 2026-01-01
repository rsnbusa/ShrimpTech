#ifndef TIME_UTILS_H_
#define TIME_UTILS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Delay execution for specified milliseconds
 * 
 * @param cuanto Delay duration in milliseconds
 */
void delay(uint32_t cuanto);

/**
 * @brief Get current time in milliseconds since system start
 * 
 * @return uint32_t Current time in milliseconds
 */
uint32_t xmillis(void);

#ifdef __cplusplus
}
#endif

#endif /* TIME_UTILS_H_ */
