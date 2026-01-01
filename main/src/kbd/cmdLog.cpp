/**
 * @file cmdLog.cpp
 * @brief Console command for log file management
 * 
 * This file implements commands for viewing and managing the system log file
 * stored in SPIFFS (/spiffs/log.txt). Supports displaying recent log entries
 * and erasing the log file.
 * 
 * @note Part of the ShrimpIoT console command system
 * @note Uses global myFile handle for log file operations
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

/**
 * @brief Console command for log file operations
 * 
 * Provides subcommands for:
 * - show: Display specified number of log lines
 * - erase: Erase the entire log file
 * 
 * @param argc Argument count from argtable3 parser
 * @param argv Argument vector from argtable3 parser
 * @return 0 on success
 * 
 * @note Usage: log -s <count> | -e <confirm>
 *       -s, --show: Show N log lines
 *       -e, --erase: Erase log file (requires confirmation value)
 */
int cmdLog(int argc, char **argv)
{
    char linea[200];

    int nerrors = arg_parse(argc, argv, (void **)&logArgs);
    if (nerrors != 0) {
        arg_print_errors(stderr, logArgs.end, argv[0]);
        return 0;
    }

    if (logArgs.show->count) {
        int cuanto = logArgs.show->ival[0];
        int ccuanto = cuanto;
        ESP_LOGI(MESH_TAG, "Show %d log lines", cuanto);
        fclose(myFile);
        myFile = fopen("/spiffs/log.txt", "r");
        if (myFile == NULL) 
        {
            ESP_LOGE(TAG, "Could not open Log file");
            return 0;
        }
        while (cuanto > 0)
        {
            if (fgets(linea, sizeof(linea), myFile) != NULL) 
            {
                printf("[%3d]%s", ccuanto - cuanto + 1, linea);
                cuanto--;
            }
            else
            {
                fclose(myFile);
                myFile = fopen("/spiffs/log.txt", "a");
                if (myFile == NULL) 
                {
                    ESP_LOGE(MESH_TAG, "Failed to open file for append");
                }
                return 0;
            }
        }
        fclose(myFile);
        myFile = fopen("/spiffs/log.txt", "a");
        if (myFile == NULL) 
        {
            ESP_LOGE(MESH_TAG, "Failed to reopen file for append");
        }
    }

    if (logArgs.erase->count) 
    {
        int cuanto = logArgs.erase->ival[0];
        ESP_LOGI(MESH_TAG, "Erase Log file confirmed %d", cuanto);

        fclose(myFile);
        remove("/spiffs/log.txt");
        myFile = fopen("/spiffs/log.txt", "a");
        if (myFile == NULL) 
        {
            ESP_LOGE(MESH_TAG, "Failed to open file for append after erase");
        }
    }

    return 0;
}