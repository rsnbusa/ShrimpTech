/**
 * @file cmdMeter.cpp
 * @brief Console command to display meter/blower timestamp information
 * 
 * This file implements a command that displays formatted timestamps for
 * the blower system's last update and lifetime initialization dates.
 * Currently simplified to show basic timestamp information.
 * 
 * @note Part of the ShrimpIoT console command system
 * @note Most meter-specific functionality is currently commented out
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

/**
 * @brief Console command to display meter/blower information
 * 
 * Displays formatted timestamps for:
 * - Last update time
 * - Lifetime initialization date
 * 
 * @param argc Argument count (unused)
 * @param argv Argument vector (unused)
 * @return 0 on success
 * 
 * @note Usage: meter
 * @note Iterates MAXDEVSS times but only uses theBlower object
 */
int cmdMeter(int argc, char **argv)
{
    struct tm *timeinfo;
    char *uupdate = (char*)malloc(100);
    char *iinitd = (char*)malloc(100);
    
    if (!uupdate || !iinitd)
    {
        printf("Error: Memory allocation failed\n");
        if (uupdate) free(uupdate);
        if (iinitd) free(iinitd);
        return 0;
    }
    
    time_t mio;

    for (int a = 0; a < MAXDEVSS; a++)
    {
        mio = theBlower.getLastUpdate();
        timeinfo = localtime((time_t*)&mio);
        strftime(uupdate, 100, "%c", timeinfo);
        
        mio = theBlower.getLifeDate();
        timeinfo = localtime((time_t*)&mio);
        strftime(iinitd, 100, "%c", timeinfo);
        
        printf("Device[%d] Last Update: %s, Init Date: %s\n", a, uupdate, iinitd);
    }
    
    free(uupdate);
    free(iinitd);
    return 0;
}