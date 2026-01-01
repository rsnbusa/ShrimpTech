/**
 * @file cmdFindUnit.cpp
 * @brief Console command to visually identify a unit by blinking its LED
 * 
 * This file implements a command that helps locate a specific physical unit
 * in a mesh network by blinking its WiFi LED. Root nodes blink continuously
 * via a task, while leaf/node units turn their LED on solid.
 * 
 * @note Part of the ShrimpIoT console command system
 * @note Uses WIFILED GPIO pin for visual identification
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
#include "defines.h"
#include "time_utils.h"
#include "led_utils.h"

/**
 * @brief Console command to identify unit by blinking WiFi LED
 * 
 * Blinks the WiFi LED 10 times rapidly, then:
 * - For root nodes: Creates a continuous blinking task
 * - For other nodes: Turns LED on solid
 * 
 * If a blink task is already running, it will be deleted first.
 * 
 * @param argc Argument count (unused)
 * @param argv Argument vector (unused)
 * @return 0 on success
 * 
 * @note Usage: findunit
 * @note Blink pattern: 10 fast blinks (100ms on/off), then continuous or solid
 */
int cmdFindUnit(int argc, char **argv)
{
    if (blinkHandle)
        vTaskDelete(blinkHandle);

    for (int a = 0; a < 10; a++)
    {
        gpio_set_level((gpio_num_t)WIFILED, 1);
        delay(100);
        gpio_set_level((gpio_num_t)WIFILED, 0);
        delay(100);
    }
    
    if (esp_mesh_is_root())
        xTaskCreate(&blinkRoot, "root", 1024, NULL, 5, &blinkHandle);
    else
        gpio_set_level((gpio_num_t)WIFILED, 1);

    return 0;
}