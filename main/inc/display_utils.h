#ifndef DISPLAY_UTILS_H_
#define DISPLAY_UTILS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Show a message on LVGL display
 * 
 * @param que Message text to display
 * @param cuanto Duration to show message in milliseconds
 * @param bigf Font size (0=14, 1=18, 2=22, 3=30)
 */
void showLVGL(char *que, uint32_t cuanto, uint8_t bigf);

/**
 * @brief Internal LVGL display task handler
 * 
 * @param showLVGL Pointer to show_lvgl_t structure
 * @note This is called internally by showLVGL, not for direct use
 */
void show_lvgl(void *showLVGL);

/**
 * @brief Turn display on/off based on command
 * 
 * @param cmd JSON command string with display parameters
 * @return int ESP_OK on success, ESP_FAIL on error
 */
int turn_display(char *cmd);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_UTILS_H_ */
