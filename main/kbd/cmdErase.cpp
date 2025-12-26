/**
 * @file cmdErase.cpp
 * @brief Console command to erase controller configuration
 * 
 * This file implements a destructive command that erases all controller
 * configuration settings. This is typically used for factory reset or
 * troubleshooting purposes.
 * 
 * @warning This is a destructive operation that will erase all stored configuration
 * @note Part of the ShrimpIoT console command system
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern void erase_config();

/**
 * @brief Console command to erase controller configuration
 * 
 * Erases all controller configuration settings by calling erase_config().
 * This is a destructive operation that cannot be undone.
 * 
 * @param argc Argument count (unused)
 * @param argv Argument vector (unused)
 * @return 0 on success
 * 
 * @warning All configuration will be permanently deleted
 * @note Usage: erase
 */
int cmdErase(int argc, char **argv)
{
    printf("Erase Controller...");
    erase_config();
    printf("done\n");
    return 0;
}