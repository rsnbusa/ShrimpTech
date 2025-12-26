/**
 * @file cmdLogLevel.cpp
 * @brief Console command for setting system log level
 * 
 * This file implements a command to change the ESP-IDF logging level at runtime.
 * Supports standard ESP-IDF log levels from None to Verbose, and persists the
 * setting to flash memory.
 * 
 * @note Part of the ShrimpIoT console command system
 * @note Changes are persisted and applied globally to all ESP-IDF logging
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

char levels[6][10] = {"None", "Error", "Warn", "Info", "Debug", "Verbose"};

/**
 * @brief Find log level index from string name
 * 
 * Searches the levels array for a matching log level name (case-sensitive).
 * 
 * @param cual String to search for (must match exactly)
 * @return Index (0-5) if found, -1 if not found
 */
int findLevel(char *cual)
{
    for (int a = 0; a < 6; a++)
        if (strcmp(cual, levels[a]) == 0)
            return a;
    
    return -1;
}

/**
 * @brief Console command to set system log level
 * 
 * Sets the ESP-IDF logging level globally and persists to flash.
 * Input is case-insensitive.
 * 
 * @param argc Argument count from argtable3 parser
 * @param argv Argument vector from argtable3 parser
 * @return 0 on success
 * 
 * @note Usage: loglevel -l <level>
 *       -l, --level: Log level (None, Error, Warn, Info, Debug, Verbose)
 *       Case-insensitive input
 * 
 * @note Changes are saved to flash and applied immediately
 */
int cmdLogLevel(int argc, char **argv)
{
    char aca[20];
    int lev = -1;

    int nerrors = arg_parse(argc, argv, (void **)&loglevel);
    if (nerrors != 0) {
        arg_print_errors(stderr, loglevel.end, argv[0]);
        return 0;
    }
    
    if (loglevel.level->count) 
    {
        strcpy(aca, loglevel.level->sval[0]);
        for (int x = 0; x < strlen(aca); x++)
            aca[x] = toupper(aca[x]);
        lev = findLevel(aca);
        if (lev >= 0)
        {
            printf("Level set to %s\n", aca);
            theConf.loglevel = lev;
            write_to_flash();
            esp_log_level_set("*", (esp_log_level_t)theConf.loglevel);
        }
        else
        {
            printf("Invalid Level. Valid levels: None, Error, Warn, Info, Debug, Verbose\n");
        }
    }
    else
    {
        printf("No level specified. Use -l <level>\n");
    }

    return 0;
}