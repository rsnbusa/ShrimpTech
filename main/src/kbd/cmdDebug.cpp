/**
 * @file cmdDebug.cpp
 * @brief Console command for managing debug flags
 * 
 * This file implements the debug command which allows runtime control of
 * debug output for various subsystems. Debug flags are stored in theConf.debug_flags
 * as a bitmask and persisted to flash memory.
 * 
 * Supported debug flags:
 * - schedule: Schedule/timer debugging
 * - mesh: Mesh networking debugging  
 * - ble: Bluetooth Low Energy debugging
 * - mqtt: MQTT protocol debugging
 * - xcmds: External commands debugging
 * - blow: Blower/sensor simulation debugging
 * - logic: Business logic debugging
 * - all: Enable/disable all debug flags at once
 * 
 * @note Debug output is controlled via theConf.debug_flags bitmask
 * @note Changes are immediately saved to flash memory
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

/**
 * @brief Helper function to set or clear a debug flag bit
 * 
 * Converts the input string to uppercase and sets or clears the specified
 * bit in theConf.debug_flags based on "ON" or "OFF" value.
 * 
 * @param value String value to parse ("on"/"off", case-insensitive)
 * @param bit_position Bit position in debug_flags to modify
 */
static void set_debug_flag(const char* value, int bit_position)
{
    char aca[10];
    strcpy(aca, value);
    for (int x = 0; x < strlen(aca); x++)
        aca[x] = toupper(aca[x]);
    
    if (strcmp(aca, "ON") == 0)
        theConf.debug_flags |= (1U << bit_position);
    else if (strcmp(aca, "OFF") == 0)
        theConf.debug_flags &= ~(1U << bit_position);
}

/**
 * @brief Console command to enable/disable debug flags for various subsystems
 * 
 * Allows runtime control of debug output by setting/clearing individual bits
 * in the theConf.debug_flags bitmask. Accepts ON/OFF (case-insensitive) for
 * each subsystem or 'all' to control all flags at once.
 * 
 * @param argc Argument count from argtable3 parser
 * @param argv Argument vector from argtable3 parser
 * @return 0 on success
 * 
 * @note Usage: debug [-s on|off] [-m on|off] [-b on|off] [-q on|off] 
 *                    [-x on|off] [-w on|off] [-l on|off] [-a on|off]
 *       -s, --schedule: Schedule debugging
 *       -m, --mesh: Mesh networking debugging
 *       -b, --ble: Bluetooth debugging
 *       -q, --mqtt: MQTT debugging
 *       -x, --xcmds: External commands debugging
 *       -w, --blow: Blower simulation debugging
 *       -l, --logic: Logic debugging
 *       -a, --all: All debug flags
 * 
 * @note Changes are persisted to flash immediately via write_to_flash()
 */
int cmdDebug(int argc, char **argv)
{
    char debug_names[][10]={"schedule","mesh","ble","mqtt","xcmds","blow","logic","modbus","limits","rs485","DO","temp"};
    int nerrors = arg_parse(argc, argv, (void **)&dbgArg);
    if (nerrors != 0) {
        arg_print_errors(stderr, dbgArg.end, argv[0]);
        return 0;
    }

    if (dbgArg.schedule->count)
        set_debug_flag(dbgArg.schedule->sval[0], dSCH);

    if (dbgArg.mesh->count)
        set_debug_flag(dbgArg.mesh->sval[0], dMESH);

    if (dbgArg.ble->count)
        set_debug_flag(dbgArg.ble->sval[0], dBLE);

    if (dbgArg.mqtt->count)
        set_debug_flag(dbgArg.mqtt->sval[0], dMQTT);

    if (dbgArg.xcmds->count)
        set_debug_flag(dbgArg.xcmds->sval[0], dXCMDS);

    if (dbgArg.blow->count)
        set_debug_flag(dbgArg.blow->sval[0], dBLOW);

    if (dbgArg.logic->count)
        set_debug_flag(dbgArg.logic->sval[0], dLOGIC);

    if (dbgArg.modbus->count)
        set_debug_flag(dbgArg.modbus->sval[0], dMODBUS);

    if (dbgArg.rs485->count)
        set_debug_flag(dbgArg.rs485->sval[0], dRS485);

    if (dbgArg.DO->count)
        set_debug_flag(dbgArg.DO->sval[0], dDO);

    if (dbgArg.temp->count)
        set_debug_flag(dbgArg.temp->sval[0], dTEMP);
        
    if (dbgArg.gps->count)
        set_debug_flag(dbgArg.gps->sval[0], dGPS);

    if (dbgArg.all->count) 
    {
        char aca[10];
        strcpy(aca, dbgArg.all->sval[0]);
        for (int x = 0; x < strlen(aca); x++)
            aca[x] = toupper(aca[x]);
        if (strcmp(aca, "ON") == 0)
            theConf.debug_flags = 0xFFFFFFFF;
        else if (strcmp(aca, "OFF") == 0)
            theConf.debug_flags = 0;
    }
    
    for (int i=0; i<sizeof(debug_names)/sizeof(debug_names[0]); i++)
    {
        if((theConf.debug_flags >> i) & 1U)
            printf("%s ", debug_names[i]);
    }
    printf("\n");
    MESP_LOGI(TAG,"Debug %x ", theConf.debug_flags);
    write_to_flash();

    return 0;
}