
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"
extern void show_profiles();

char lims[][10]={"AHUM","ATEMP","WTEMP","LIMITPH","LIMITDO","GENLCT","BMDDKW","BMCHKW","LCONLI","USEDEN","GENEER",
"BMDDKT","BMCHKT","BMDDAH","BMCHAH","BMTEMP","BMCC","BMSOH","BMSOC","PV1A","PV1V","PV2A","PV2V"};

char modb_names[][30]={
 "ChargeState","PV1Volts","PV1AMps","PV2Volts","PV2AMps",
"SOC","SOH","CycleCount","BatTemp",
"DO","PH","WTemp","A Temp","Humidity",
"BatChToday","BatDscToday","BatChgTotal","BatDscTotal","GenToday","UsedToday","LoadUsedTotal","BatChdToday","BatDscToday","LoadUsedToday","BatTemp"
};

/**
 * @brief Display complete Modbus configuration for all devices
 * 
 * Shows detailed Modbus settings for:
 * - PV Panels (solar panels - charge state, voltages, currents)
 * - Battery (SOC, SOH, cycle count, temperature)
 * - Sensors (DO, PH, water temp, air temp, humidity)
 * - Inverter (energy metrics, charging/discharging stats)
 * 
 * Each section displays: device address, refresh rate, register offsets,
 * start addresses, point counts, and multiplexer values.
 */
void show_mimodbus()
{
    printf("%s\n",LRED);
    printf("┌═════════════════════════════════════════════════════════────────┐\n");
    printf("│                  MODBUS CONFIGURATION                           │\n");
    printf("└─────────────────────────────────────────────────────────────────┘\n\n");

    // ===== PV PANELS =====
    printf("  ┌─ PV Panels (Addr: %02d | Refresh: %dms) ────────────────┐\n", 
           theConf.modbus_panels.PVAddress, theConf.modbus_panels.refresh_rate);
    printf("  │ %-16s │ Offset │ Start  │ Points │  Mux   │\n", "Name");
    printf("  ├─────────────────┼────────┼────────┼────────┼────────┤\n");
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[0], 
           theConf.modbus_panels.Charge_StateOffset, theConf.modbus_panels.ChargeStart, 
           theConf.modbus_panels.ChargePoints, theConf.modbus_panels.ChargeMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[1], 
           theConf.modbus_panels.PV1VoltsOffset, theConf.modbus_panels.PV1VStart, 
           theConf.modbus_panels.PV1VPoints, theConf.modbus_panels.PV1VMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[2], 
           theConf.modbus_panels.PV2VoltsOffset, theConf.modbus_panels.PV2VStart, 
           theConf.modbus_panels.PV2VPoints, theConf.modbus_panels.PV2VMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[3], 
           theConf.modbus_panels.PV1_AmpsOffset, theConf.modbus_panels.PV1AmpsStart, 
           theConf.modbus_panels.PV1AmpsPoints, theConf.modbus_panels.PV1AmpsMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[4], 
           theConf.modbus_panels.PV2AmpsOffset, theConf.modbus_panels.PV2AmpsStart, 
           theConf.modbus_panels.PV2AmpsPoints, theConf.modbus_panels.PV2AmpsMux);
    printf("  └─────────────────┴────────┴────────┴────────┴────────┘\n\n");

    // ===== BATTERY =====
    printf("  ┌─ Battery (Addr: %02d | Refresh: %dms) ────────────────┐\n", 
           theConf.modbus_battery.batAddress, theConf.modbus_battery.refresh_rate);
    printf("  │ %-16s │ Offset │ Start  │ Points │  Mux   │\n", "Name");
    printf("  ├─────────────────┼────────┼────────┼────────┼────────┤\n");
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[5], 
           theConf.modbus_battery.SOCOffset, theConf.modbus_battery.SOCStart, 
           theConf.modbus_battery.SOCPoints, theConf.modbus_battery.SOCMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[6], 
           theConf.modbus_battery.SOHOffset, theConf.modbus_battery.SOHStart, 
           theConf.modbus_battery.SOHPoints, theConf.modbus_battery.SOHMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[7], 
           theConf.modbus_battery.cycleOffset, theConf.modbus_battery.cycleStart, 
           theConf.modbus_battery.cyclePoints, theConf.modbus_battery.cycleMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[8], 
           theConf.modbus_battery.tempOffset, theConf.modbus_battery.tempStart, 
           theConf.modbus_battery.tempPoints, theConf.modbus_battery.tempMux);
    printf("  └─────────────────┴────────┴────────┴────────┴────────┘\n\n");

    // ===== SENSORS =====
    printf("  ┌─ Sensors (Refresh: %dms) ─────────────────────────────┐\n", 
           theConf.modbus_sensors.refresh_rate);
    printf("  │ %-16s │ Addr │ Offset │ Start  │ Points │  Mux   │\n", "Name");
    printf("  ├─────────────────┼──────┼────────┼────────┼────────┼────────┤\n");
    printf("  │ %-16s │ %4d │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[9], 
           theConf.modbus_sensors.DOAddress, theConf.modbus_sensors.DOOffset, 
           theConf.modbus_sensors.DOStart, theConf.modbus_sensors.DOPoints, theConf.modbus_sensors.DOMux);
    printf("  │ %-16s │ %4d │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[10], 
           theConf.modbus_sensors.PHAddress, theConf.modbus_sensors.PHOffset, 
           theConf.modbus_sensors.PHStart, theConf.modbus_sensors.PHPoints, theConf.modbus_sensors.PHMux);
    printf("  │ %-16s │ %4d │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[11], 
           theConf.modbus_sensors.WAddress, theConf.modbus_sensors.WOffset, 
           theConf.modbus_sensors.WStart, theConf.modbus_sensors.WPoints, theConf.modbus_sensors.WMux);
    printf("  │ %-16s │ %4d │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[12], 
           theConf.modbus_sensors.AAddress, theConf.modbus_sensors.AOffset, 
           theConf.modbus_sensors.AStart, theConf.modbus_sensors.APoints, theConf.modbus_sensors.AMux);
    printf("  │ %-16s │ %4d │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[13], 
           theConf.modbus_sensors.HAddress, theConf.modbus_sensors.HumOffset, 
           theConf.modbus_sensors.humStart, theConf.modbus_sensors.humPoints, theConf.modbus_sensors.humMux);
    printf("  └─────────────────┴──────┴────────┴────────┴────────┴────────┘\n\n");

    // ===== INVERTER =====
    printf("  ┌─ Inverter (Addr: %02d | Refresh: %dms) ────────────────┐\n", 
           theConf.modbus_inverter.InverterAddress, theConf.modbus_inverter.refresh_rate);
    printf("  │ %-16s │ Offset │ Start  │ Points │  Mux   │\n", "Name");
    printf("  ├─────────────────┼────────┼────────┼────────┼────────┤\n");
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[14], 
           theConf.modbus_inverter.I1_BatChHoyOff, theConf.modbus_inverter.I1_BatChHoyStart, 
           theConf.modbus_inverter.I1_BatChHoyPoints, theConf.modbus_inverter.I1_BatChHoyMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[15], 
           theConf.modbus_inverter.I2_BatDscHoyOff, theConf.modbus_inverter.I2_BatDscHoyStart, 
           theConf.modbus_inverter.I2_BatDscHoyPoints, theConf.modbus_inverter.I2_BatDscHoyMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[16], 
           theConf.modbus_inverter.I3_BatChgTotalOff, theConf.modbus_inverter.I3_BatChgTotalStart, 
           theConf.modbus_inverter.I3_BatChgTotalPoints, theConf.modbus_inverter.I3_BatChgTotalMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[17], 
           theConf.modbus_inverter.I4_BatDscTotalOff, theConf.modbus_inverter.I4_BatDscTotalStart, 
           theConf.modbus_inverter.I4_BatDscTotalPoints, theConf.modbus_inverter.I4_BatDscTotalMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[18], 
           theConf.modbus_inverter.I5_GenkWhHoyOff, theConf.modbus_inverter.I5_GenkWhHoyStart, 
           theConf.modbus_inverter.I5_GenkWhHoyPoints, theConf.modbus_inverter.I5_GenkWhHoyMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[19], 
           theConf.modbus_inverter.I6_UsedkwhHoyOff, theConf.modbus_inverter.I6_UsedkwhHoyStart, 
           theConf.modbus_inverter.I6_UsedkwhHoyPoints, theConf.modbus_inverter.I6_UsedkwhHoyMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[20], 
           theConf.modbus_inverter.I7_LoadUsedTotalOff, theConf.modbus_inverter.I7_LoadUsedTotalStart, 
           theConf.modbus_inverter.I7_LoadUsedTotalPoints, theConf.modbus_inverter.I7_LoadUsedTotalMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[21], 
           theConf.modbus_inverter.I8_BatChdHoyOff, theConf.modbus_inverter.I8_BatChdHoyStart, 
           theConf.modbus_inverter.I8_BatChdHoyPoints, theConf.modbus_inverter.I8_BatChdHoyMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[22], 
           theConf.modbus_inverter.I9_BatDscHoyOff, theConf.modbus_inverter.I9_BatDscHoyStart, 
           theConf.modbus_inverter.I9_BatDscHoyPoints, theConf.modbus_inverter.I9_BatDscHoyMux);
    printf("  │ %-16s │ %6d │ %3d │ %6d │ %.2f  │\n", modb_names[23], 
           theConf.modbus_inverter.I10_LoadUsedHoyOff, theConf.modbus_inverter.I10_LoadUsedHoyStart, 
           theConf.modbus_inverter.I10_LoadUsedHoyPoints, theConf.modbus_inverter.I10_LoadUsedHoyMux);
    printf("  └─────────────────┴────────┴────────┴────────┴────────┘\n\n");
}

/**
 * @brief Display configured min/max limits for all monitored parameters
 * 
 * Shows operational limits for environmental sensors, battery metrics,
 * energy consumption, and solar generation. Used for alarming and
 * validation of sensor readings.
 * 
 * Parameters include: humidity, temperatures, pH, DO, battery SOC/SOH,
 * energy generation/consumption, PV voltages/currents, etc.
 */
void show_limits()
{
    printf("%s\n",BLUE);
    printf("┌────────────────────────────────────────────────────┐\n");
    printf("│                 OPERATIONAL LIMITS                │\n");
    printf("├──────────────────────┬──────────┬──────────────────┤\n");
    printf("│ %-20s │   Min    │       Max        │\n", "Parameter");
    printf("├──────────────────────┼──────────┼──────────────────┤\n");
    printf("│ %-20s │ %8d │ %16d │\n", lims[0], theConf.milim.hummin, theConf.milim.hummax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[1], theConf.milim.atempmin, theConf.milim.atempmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[2], theConf.milim.wtempmin, theConf.milim.wtempmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[3], theConf.milim.phmin, theConf.milim.phmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[4], theConf.milim.domin, theConf.milim.domax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[5], theConf.milim.kwchoymin, theConf.milim.kwchoymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[6], theConf.milim.kwbatdhoymin, theConf.milim.kwbatdhoymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[7], theConf.milim.kwbatchoymin, theConf.milim.kwbatchoymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[8], theConf.milim.kwloadhoymin, theConf.milim.kwloadhoymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[9], theConf.milim.kwctodaymin, theConf.milim.kwctodaymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[10], theConf.milim.kwgtodaymin, theConf.milim.kwgtodaymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[11], theConf.milim.bdATotmin, theConf.milim.bdATotmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[12], theConf.milim.bcATotmin, theConf.milim.bcATotmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[13], theConf.milim.bdAhoymin, theConf.milim.bcAhoymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[14], theConf.milim.btempmin, theConf.milim.btempmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[15], theConf.milim.bcyclemin, theConf.milim.bcyclemax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[16], theConf.milim.bSOHmin, theConf.milim.bSOHmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[17], theConf.milim.bSOCmin, theConf.milim.bSOCmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[18], theConf.milim.amin, theConf.milim.amax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[19], theConf.milim.vmin, theConf.milim.vmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[20], theConf.milim.amin, theConf.milim.amax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[21], theConf.milim.vmin, theConf.milim.vmax);
    printf("└──────────────────────┴──────────┴──────────────────┘\n\n");
}

/**
 * @brief Display comprehensive system configuration (runs as FreeRTOS task)
 * 
 * This function displays complete system status and configuration including:
 * - Device information (boot count, reset reasons, version info)
 * - Production configuration (blower mode, debug flags, active profile)
 * - MQTT topics and server settings
 * - Network/Mesh configuration (SSIDs, mesh topology, routing table)
 * - Timing information (next scheduled actions)
 * - Statistics (bytes in/out, message counts, connection stats)
 * - Location settings
 * - Current blower/solar system data
 * - Operational limits (via show_limits)
 * - Modbus configuration (via show_mimodbus)
 * 
 * @param pArg Unused task parameter (required by FreeRTOS task signature)
 * @note Deletes itself after completion using vTaskDelete(NULL)
 */
void showconf(void *pArg)
{

    char            buf[50],buf2[50],fecha[60],myssid[20];
    time_t          lastwrite,now;
    struct tm       timeinfo;
    // portMUX_TYPE    xTimerLock = portMUX_INITIALIZER_UNLOCKED;
    TickType_t      xRemainingTime;
    int             routet;
    mesh_type_t     typ;
    char            my_mac[8]={0};
    uint8_t         min,secs;
    mesh_addr_t     bssid;
    time_t         guardDate,bootdate;

    time(&now);
    localtime_r(&now, &timeinfo);

    bzero(myssid,sizeof(myssid));


    char *tipo[]={"Idle","ROOT","NODE","LEAF","STA"};
    wifi_config_t conf;

    unsigned char mac_base[6] = {0};
    esp_efuse_mac_get_default(mac_base);
    esp_read_mac(mac_base, ESP_MAC_WIFI_STA);

    if(mesh_started)
    {
        memcpy(my_mac,mesh_netif_get_station_mac(),6);
        if(esp_wifi_get_config(WIFI_IF_STA, &conf)==ESP_OK)
        {
            strcpy(myssid,(char*)conf.sta.ssid);
        }
        else
        {
            printf("Error reading wifi config\n");
        }

        typ=esp_mesh_get_type();
    }

    min=10;
    secs=10*2-(min*60);

    timeinfo.tm_min=min;
    timeinfo.tm_sec=secs; 
    time_t nexthour = mktime(&timeinfo);
    int faltan=nexthour-now;

    esp_mesh_get_parent_bssid(&bssid);
    const esp_app_desc_t *mip=esp_app_get_description();
    if(mip)
        printf("\t\t Mesh Configuration Date %s ",ctime(&now));

    if(!theConf.ptch)
        printf("Virgin Chip\n");

    uint32_t nada;    // this is compiler error, it goes crazy if done directly like fram.read_fdate(uint8_t*)&guarddate)

    nada=theBlower.getLastUpdate();
    // fram.read_fdate((uint8_t*)&nada);				// read last saved datetime.
    guardDate=(time_t)nada;

    bootdate=(time_t)theConf.lastRebootTime;    //same compiler error

    // ===== DEVICE INFORMATION =====
    printf("%s",LYELLOW);
    printf("\n┌─────────────────────────────────────────────────────────────┐\n");
    printf("│                   DEVICE INFORMATION                        │\n");
    printf("├─────────────────────────────────────────────────────────────┤\n");
    printf("│ Boot Count:                                                 │\n");
    printf("│   %-57d │\n", theConf.bootcount);
    printf("│ Last Reset & Reason:                                        │\n");
    printf("│   Reset: %-6d  Reason: %-34d │\n", theConf.lastResetCode, theConf.lastResetCode);
    printf("│ Log Level & Down Time:                                      │\n");
    printf("│   Level: %-6d  Down Time: %-28lus │\n", theConf.loglevel, theConf.downtime);
    printf("│ Last Reboot:                                                │\n");
    char reboot_str[60];
    strftime(reboot_str, sizeof(reboot_str), "  %Y-%m-%d %H:%M:%S", localtime((time_t*)&bootdate));
    printf("│ %-59s │\n", reboot_str);
    printf("│ Configuration Flags:                                        │\n");
    printf("│   Meter: %-6d  MQTT: %-6d  Send Meter: %-16d │\n", theConf.meterconf, mqttf, sendMeterf);
    printf("│ Guard Date:                                                 │\n");
    strftime(reboot_str, sizeof(reboot_str), "  %Y-%m-%d %H:%M:%S", localtime(&guardDate));
    printf("│ %-59s │\n", reboot_str);
    printf("│ Display & System Status:                                    │\n");
    printf("│   Display Active: %-42s │\n", gdispf?"Yes":"No");
    printf("│ Version Information:                                        │\n");
    printf("│   App: %-11s  IDF: %-33s │\n", mip->version, mip->idf_ver);
    printf("│   Project: %-48s │\n", mip->project_name);
    printf("│   Compiled: %s @ %-36s │\n", mip->date, mip->time);
    printf("│   Latest Sent: %-44s │\n", theConf.lastVersion);
    printf("│ Network Settings:                                           │\n");
    printf("│   Mesh Delay: %-10s  Login Wait: %-23d │\n", theConf.delay_mesh?"Yes":"No", theConf.loginwait);
    printf("└─────────────────────────────────────────────────────────────┘\n\n");

    // ===== PRODUCTION CONFIGURATION =====
    printf("%s",CYAN);
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│              PRODUCTION CONFIGURATION                       │\n");
    printf("├─────────────────────────────────────────────────────────────┤\n");
    printf("│ Blower Mode: %d                                            │\n", theConf.blower_mode);
    printf("│ Active Profile: %d | Start Day: %d%%                         │\n", theConf.activeProfile, theConf.dayCycle);
    printf("│ Is Master Node: %s                                          │\n", theConf.masternode?"Yes":"No");
    printf("│ Debug Flags (0x%X): ", theConf.debug_flags);
    if((theConf.debug_flags >> dSCH) & 1U)   printf("Schedule ");
    if((theConf.debug_flags >> dMESH) & 1U)  printf("Mesh ");
    if((theConf.debug_flags >> dBLE) & 1U)   printf("Ble ");
    if((theConf.debug_flags >> dMQTT) & 1U)  printf("Mqtt ");
    if((theConf.debug_flags >> dXCMDS) & 1U) printf("Xcmds ");
    if((theConf.debug_flags >> dBLOW) & 1U)  printf("Blower ");
    if((theConf.debug_flags >> dLOGIC) & 1U) printf("Logic ");
    if((theConf.debug_flags >> dMODBUS) & 1U) printf("Modbus ");
    printf("              │\n");
    if(theConf.blower_mode)
        printf("│ Cycle: %d | Day: %d | Timer Div: %d                              │\n", ck, ck_d, theConf.test_timer_div);
    else
        printf("│ Status: Waiting for Production Cycle start                   │\n");
    printf("└─────────────────────────────────────────────────────────────┘\n\n");

    // ===== MQTT CONFIGURATION =====
    printf("%s",MAGENTA);
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│                  MQTT CONFIGURATION                        │\n");
    printf("├─────────────────────────────────────────────────────────────┤\n");
    printf("│ Command Topic: %-47s │\n", cmdQueue);
    printf("│ Info Topic:    %-47s │\n", infoQueue);
    printf("│ Alarm Topic:   %-47s │\n", alarmQueue);
    printf("│ Server: [%s] | User: [%s] | Pass: [%s] │\n", theConf.mqttServer, theConf.mqttUser, theConf.mqttPass);
    printf("└─────────────────────────────────────────────────────────────┘\n\n");


    // ===== NETWORK & MESH INFO =====
    if(meshf)
    {
       printf("%s",YELLOW);
        printf("┌─────────────────────────────────────────────────────────────┐\n");
        printf("│              NETWORK & MESH CONFIGURATION                   │\n");
        printf("├─────────────────────────────────────────────────────────────┤\n");
        printf("│ Mesh ID: " MACSTR " (SubNode: %d | Pool: %d | Unit: %d)    │\n", 
               MAC2STR(MESH_ID), theConf.subnode, theConf.poolid, theConf.unitid);
        printf("│ STA Mode: [SSID: %s] [Pass: %s]             │\n", conf.sta.ssid, conf.sta.password);
        printf("│ Flash Config: [SSID: %s] [Pass: %s]         │\n", theConf.thessid, theConf.thepass);
        printf("│ Parent BSSID: " MACSTR "                                │\n", MAC2STR(bssid.addr));
        printf("│ AP Mode: [SSID: %s] [Pass: %s]              │\n", conf.ap.ssid, conf.ap.password);
        
        mesh_addr_t mmeshid;
        esp_mesh_get_id(&mmeshid);
        printf("│ Node Type: %-40s │\n", tipo[typ]);
        printf("│ MAC Address: " MACSTR "                                  │\n", MAC2STR(mac_base));
        printf("│ Mesh ID: " MACSTR "                                    │\n", MAC2STR(mmeshid.addr));
        printf("│ Device State: %s                                         │\n", esp_mesh_is_device_active?"UP":"DOWN");
        printf("└─────────────────────────────────────────────────────────────┘\n\n");

        // ===== TIMING INFORMATION =====
        printf("%s",WHITEC);
        printf("┌─────────────────────────────────────────────────────────────┐\n");
        printf("│                 TIMING INFORMATION                          │\n");
        printf("├─────────────────────────────────────────────────────────────┤\n");
        if(collectTimer)
          if(xTimerIsTimerActive(collectTimer))
          {
            xRemainingTime = xTimerGetExpiryTime( collectTimer ) - xTaskGetTickCount();
          }
        printf("│ Next Send Data: %dms | Base Time: %d | Repeat Timer: %d    │\n", 
               xRemainingTime, theConf.baset, theConf.repeat);
        printf("└─────────────────────────────────────────────────────────────┘\n\n");

        // ===== MESH ROUTING TABLE =====
        esp_err_t err=esp_mesh_get_routing_table((mesh_addr_t *) &s_route_table,CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &routet);

        if(err==ESP_OK)
        {
              printf("%S",LGREEN);
            printf("┌─────────────────────────────────────────────────────────────┐\n");
            printf("│                   MESH NETWORK TOPOLOGY                     │\n");
            printf("├─────────────────────────────────────────────────────────────┤\n");
            if(routet<11)
            {
                printf("│ Total Nodes: %-51d │\n", routet);
                printf("├─────────────────────────────────────────────────────────────┤\n");
                for (int a=0;a<routet;a++)
                {
                    printf("│ [%2d] MAC: " MACSTR " %2s | Meter: %-15s | Send: %s | Power: %s │\n",
                           a, MAC2STR(s_route_table[a].addr),
                           MAC_ADDR_EQUAL(s_route_table[a].addr, my_mac)?"*ME":"",
                           masterNode.theTable.meterName[a],
                           masterNode.theTable.sendit[a]?"Y":"N",
                           masterNode.theTable.onoff[a]?"ON":"OFF");
                }
            }
            else
            {
                printf("│ Total Nodes: %-51d │\n", routet);
            }
            printf("└─────────────────────────────────────────────────────────────┘\n\n");
        }
    }
    // ===== STATISTICS =====
    printf("%s",BLUE);
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│                    STATISTICS                              │\n");
    printf("├─────────────────────────────────────────────────────────────┤\n");
    printf("│ Bytes Out: %-14d │ Bytes In: %-16d │\n", theBlower.getStatsBytesOut(), theBlower.getStatsBytesIn());
    printf("│ Messages In: %-13d │ Messages Out: %-15d │\n", theBlower.getStatsMsgIn(), theBlower.getStatsMsgOut());
    printf("│ STA Connections: %-11d │ STA Disconnections: %-14d │\n", theBlower.getStatsStaConns(), theBlower.getStatsStaDiscos());
    printf("│ Last Activity: %s", ctime(&now));
    printf("└─────────────────────────────────────────────────────────────┘\n\n");

    // ===== SYSTEM CONFIGURATION =====
    printf("%s",GRAY);
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│                SYSTEM CONFIGURATION                        │\n");
    printf("├─────────────────────────────────────────────────────────────┤\n");
    printf("│ Expected Nodes: %-43lu │\n", theConf.totalnodes);
    printf("│ Expected Connections: %-39lu │\n", theConf.conns);
    printf("└─────────────────────────────────────────────────────────────┘\n\n");

    printf("%s",RESETC);
    print_blower("Blower",theBlower.getSolarSystem(),false);

    show_limits();
    show_mimodbus();
    vTaskDelete(NULL);
}

/**
 * @brief Console command to display complete system configuration
 * 
 * Creates a FreeRTOS task to display comprehensive system status.
 * Task runs independently to avoid blocking the console while generating
 * the large configuration output.
 * 
 * @param argc Argument count (unused, no arguments expected)
 * @param argv Argument vector (unused, no arguments expected)
 * @return 0 on success
 * 
 * @note Usage: config
 * @note Task stack size: 4096 bytes, priority: 10
 */
int cmdConfig(int argc, char **argv)
{

    xTaskCreate(&showconf,"show",4096,NULL, 10, NULL); 	        // long display allow for others to do stuff
    return 0;
}
