
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
void show_modbus()
{
//     printf("%s\n",LRED);
    printf("┌─────────────────────────────────────────────────────────────────┐\n");
    printf("│%s%s                  MODBUS CONFIGURATION                           %s│\n",RESETC,BK_RED,RESETC);
    printf("└─────────────────────────────────────────────────────────────────┘\n\n");

    // ===== PV PANELS =====
    printf("  ┌─ %sPV Panels (Addr: %3d | Refresh: %3dms) %s────────────┐\n",BK_BLUE, 
           theConf.modbus_panels.PVAddress, theConf.modbus_panels.refresh_rate,RESETC);
    printf("  │ %-15s │ Offset │ Start  │ Points │  Mux   │\n", "Name");
    printf("  ├─────────────────┼────────┼────────┼────────┼────────┤\n");
    printf("  │ %-15s │ %6d │ %5d  │ %6d  │ %4.2f  │\n", modb_names[0], 
           theConf.modbus_panels.Charge_StateOffset, theConf.modbus_panels.ChargeStart, 
           theConf.modbus_panels.ChargePoints, theConf.modbus_panels.ChargeMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d  │ %4.2f  │\n", modb_names[1], 
           theConf.modbus_panels.PV1VoltsOffset, theConf.modbus_panels.PV1VStart, 
           theConf.modbus_panels.PV1VPoints, theConf.modbus_panels.PV1VMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d  │ %4.2f  │\n", modb_names[2], 
           theConf.modbus_panels.PV2VoltsOffset, theConf.modbus_panels.PV2VStart, 
           theConf.modbus_panels.PV2VPoints, theConf.modbus_panels.PV2VMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d  │ %4.2f  │\n", modb_names[3], 
           theConf.modbus_panels.PV1_AmpsOffset, theConf.modbus_panels.PV1AmpsStart, 
           theConf.modbus_panels.PV1AmpsPoints, theConf.modbus_panels.PV1AmpsMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d  │ %4.2f  │\n", modb_names[4], 
           theConf.modbus_panels.PV2AmpsOffset, theConf.modbus_panels.PV2AmpsStart, 
           theConf.modbus_panels.PV2AmpsPoints, theConf.modbus_panels.PV2AmpsMux);
    printf("  └─────────────────┴────────┴────────┴────────┴────────┘\n\n");

    // ===== BATTERY =====
    printf("  ┌─ %sBattery (Addr: %3d | Refresh: %3dms) %s ─────────────┐\n", BK_GREEN,
           theConf.modbus_battery.batAddress, theConf.modbus_battery.refresh_rate, RESETC);
    printf("  │ %-15s │ Offset │ Start  │ Points │  Mux   │\n", "Name");
    printf("  ├─────────────────┼────────┼────────┼────────┼────────┤\n");
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[5], 
           theConf.modbus_battery.SOCOffset, theConf.modbus_battery.SOCStart, 
           theConf.modbus_battery.SOCPoints, theConf.modbus_battery.SOCMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[6], 
           theConf.modbus_battery.SOHOffset, theConf.modbus_battery.SOHStart, 
           theConf.modbus_battery.SOHPoints, theConf.modbus_battery.SOHMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[7], 
           theConf.modbus_battery.cycleOffset, theConf.modbus_battery.cycleStart, 
           theConf.modbus_battery.cyclePoints, theConf.modbus_battery.cycleMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[8], 
           theConf.modbus_battery.tempOffset, theConf.modbus_battery.tempStart, 
           theConf.modbus_battery.tempPoints, theConf.modbus_battery.tempMux);
    printf("  └─────────────────┴────────┴────────┴────────┴────────┘\n\n");

    // ===== SENSORS =====
    printf("  ┌─ %sSensors (Refresh: %2dms) %s─────────────────────────────────┐\n", BK_YELLOW,
           theConf.modbus_sensors.refresh_rate, RESETC);
    printf("  │ %-15s │ Addr │ Offset │ Start  │ Points │  Mux  │\n", "Name");
    printf("  ├─────────────────┼──────┼────────┼────────┼────────┼───────┤\n");
    printf("  │ %-15s │ %4d │ %6d │ %5d  │ %6d │ %4.2f  │\n", modb_names[9], 
           theConf.modbus_sensors.DOAddress, theConf.modbus_sensors.DOOffset, 
           theConf.modbus_sensors.DOStart, theConf.modbus_sensors.DOPoints, theConf.modbus_sensors.DOMux);
    printf("  │ %-15s │ %4d │ %6d │ %5d  │ %6d │ %4.2f  │\n", modb_names[10], 
           theConf.modbus_sensors.PHAddress, theConf.modbus_sensors.PHOffset, 
           theConf.modbus_sensors.PHStart, theConf.modbus_sensors.PHPoints, theConf.modbus_sensors.PHMux);
    printf("  │ %-15s │ %4d │ %6d │ %5d  │ %6d │ %4.2f  │\n", modb_names[11], 
           theConf.modbus_sensors.WAddress, theConf.modbus_sensors.WOffset, 
           theConf.modbus_sensors.WStart, theConf.modbus_sensors.WPoints, theConf.modbus_sensors.WMux);
    printf("  │ %-15s │ %4d │ %6d │ %5d  │ %6d │ %4.2f  │\n", modb_names[12], 
           theConf.modbus_sensors.AAddress, theConf.modbus_sensors.AOffset, 
           theConf.modbus_sensors.AStart, theConf.modbus_sensors.APoints, theConf.modbus_sensors.AMux);
    printf("  │ %-15s │ %4d │ %6d │ %5d  │ %6d │ %4.2f  │\n", modb_names[13], 
           theConf.modbus_sensors.HAddress, theConf.modbus_sensors.HumOffset, 
           theConf.modbus_sensors.humStart, theConf.modbus_sensors.humPoints, theConf.modbus_sensors.humMux);
    printf("  └─────────────────┴──────┴────────┴────────┴────────┴───────┘\n\n");

    // ===== INVERTER =====
    printf("  ┌─ %sInverter (Addr: %3d | Refresh: %3dms) %s ────────────┐\n", BK_MAGENTA,
           theConf.modbus_inverter.InverterAddress, theConf.modbus_inverter.refresh_rate, RESETC);
    printf("  │ %-15s │ Offset │ Start  │ Points │  Mux   │\n", "Name");
    printf("  ├─────────────────┼────────┼────────┼────────┼────────┤\n");
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[14], 
           theConf.modbus_inverter.I1_BatChHoyOff, theConf.modbus_inverter.I1_BatChHoyStart, 
           theConf.modbus_inverter.I1_BatChHoyPoints, theConf.modbus_inverter.I1_BatChHoyMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[15], 
           theConf.modbus_inverter.I2_BatDscHoyOff, theConf.modbus_inverter.I2_BatDscHoyStart, 
           theConf.modbus_inverter.I2_BatDscHoyPoints, theConf.modbus_inverter.I2_BatDscHoyMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[16], 
           theConf.modbus_inverter.I3_BatChgTotalOff, theConf.modbus_inverter.I3_BatChgTotalStart, 
           theConf.modbus_inverter.I3_BatChgTotalPoints, theConf.modbus_inverter.I3_BatChgTotalMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[17], 
           theConf.modbus_inverter.I4_BatDscTotalOff, theConf.modbus_inverter.I4_BatDscTotalStart, 
           theConf.modbus_inverter.I4_BatDscTotalPoints, theConf.modbus_inverter.I4_BatDscTotalMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[18], 
           theConf.modbus_inverter.I5_GenkWhHoyOff, theConf.modbus_inverter.I5_GenkWhHoyStart, 
           theConf.modbus_inverter.I5_GenkWhHoyPoints, theConf.modbus_inverter.I5_GenkWhHoyMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[19], 
           theConf.modbus_inverter.I6_UsedkwhHoyOff, theConf.modbus_inverter.I6_UsedkwhHoyStart, 
           theConf.modbus_inverter.I6_UsedkwhHoyPoints, theConf.modbus_inverter.I6_UsedkwhHoyMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[20], 
           theConf.modbus_inverter.I7_LoadUsedTotalOff, theConf.modbus_inverter.I7_LoadUsedTotalStart, 
           theConf.modbus_inverter.I7_LoadUsedTotalPoints, theConf.modbus_inverter.I7_LoadUsedTotalMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[21], 
           theConf.modbus_inverter.I8_BatChdHoyOff, theConf.modbus_inverter.I8_BatChdHoyStart, 
           theConf.modbus_inverter.I8_BatChdHoyPoints, theConf.modbus_inverter.I8_BatChdHoyMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[22], 
           theConf.modbus_inverter.I9_BatDscHoyOff, theConf.modbus_inverter.I9_BatDscHoyStart, 
           theConf.modbus_inverter.I9_BatDscHoyPoints, theConf.modbus_inverter.I9_BatDscHoyMux);
    printf("  │ %-15s │ %6d │ %5d  │ %6d │ %4.2f   │\n", modb_names[23], 
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

//     printf("%s\n",BLUE);
    printf("┌────────────────────────────────────────────────────┐\n");
    printf("│%s%s                 OPERATIONAL LIMITS                 %s│\n",RESETC,BK_GREEN,RESETC);
    printf("├──────────────────────┬──────────┬──────────────────┤\n");
// printf("%s", RESETC);
    printf("│ %-20s │   Min    │       Max        │\n", "Parameter");
    printf("├──────────────────────┼──────────┼──────────────────┤\n");
    printf("│%s %-20s%s │ %8d │ %16d │\n",BK_GRAY, lims[0], RESETC, theConf.milim.hummin, theConf.milim.hummax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[1], theConf.milim.atempmin, theConf.milim.atempmax);
    printf("│%s %-20s %s│ %8d │ %16d │\n", BK_GRAY, lims[2], RESETC, theConf.milim.wtempmin, theConf.milim.wtempmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[3], theConf.milim.phmin, theConf.milim.phmax);
    printf("│%s %-20s %s│ %8d │ %16d │\n", BK_GRAY, lims[4], RESETC, theConf.milim.domin, theConf.milim.domax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[5], theConf.milim.kwchoymin, theConf.milim.kwchoymax);
    printf("│ %s%-20s%s │ %8d │ %16d │\n", BK_GRAY, lims[6], RESETC, theConf.milim.kwbatdhoymin, theConf.milim.kwbatdhoymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[7], theConf.milim.kwbatchoymin, theConf.milim.kwbatchoymax);
    printf("│ %s%-20s%s │ %8d │ %16d │\n", BK_GRAY, lims[8], RESETC, theConf.milim.kwloadhoymin, theConf.milim.kwloadhoymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[9], theConf.milim.kwctodaymin, theConf.milim.kwctodaymax);
    printf("│ %s%-20s %s│ %8d │ %16d │\n", BK_GRAY, lims[10], RESETC, theConf.milim.kwgtodaymin, theConf.milim.kwgtodaymax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[11], theConf.milim.bdATotmin, theConf.milim.bdATotmax);
    printf("│ %s%-20s %s│ %8d │ %16d │\n", BK_GRAY, lims[12], RESETC, theConf.milim.bcATotmin, theConf.milim.bcATotmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[13], theConf.milim.bdAhoymin, theConf.milim.bcAhoymax);
    printf("│ %s%-20s%s │ %8d │ %16d │\n", BK_GRAY, lims[14], RESETC, theConf.milim.btempmin, theConf.milim.btempmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[15], theConf.milim.bcyclemin, theConf.milim.bcyclemax);
    printf("│ %s%-20s%s │ %8d │ %16d │\n", BK_GRAY, lims[16], RESETC, theConf.milim.bSOHmin, theConf.milim.bSOHmax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[17], theConf.milim.bSOCmin, theConf.milim.bSOCmax);
    printf("│ %s%-20s%s │ %8d │ %16d │\n", BK_GRAY, lims[18], RESETC, theConf.milim.amin, theConf.milim.amax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[19], theConf.milim.vmin, theConf.milim.vmax);
    printf("│ %s%-20s%s │ %8d │ %16d │\n", BK_GRAY, lims[20], RESETC, theConf.milim.amin, theConf.milim.amax);
    printf("│ %-20s │ %8d │ %16d │\n", lims[21], theConf.milim.vmin, theConf.milim.vmax);
    printf("└──────────────────────┴──────────┴──────────────────┘\n\n");
}

/**
 * @brief Display schedule information including active profile and day cycle
 * 
 * Shows current scheduling status:
 * - Active profile index
 * - Current day cycle
 * - Profile activation date
 * - Day cycle start date
 */
void show_schedule_info()
{
       char date_buf[30];
       struct tm timeinfo;
       wschedule_t wsched;

       theBlower.setScheduleStruct(wsched);

       // printf("%s\n", CYAN);
       printf("┌────────────────────────────────────────────────────┐\n");
       printf("│%s%s              SCHEDULE INFORMATION                  %s│\n",RESETC,BK_BLUE,RESETC);
       printf("├────────────────────────────────────────────────────┤\n");
       //     printf("%s", RESETC);
       printf("│ Current Cycle :      %-28d  │\n", theConf.activeProfile);
       printf("│ Current Cycle:       %-28d  │\n", wsched.currentCycle);
       printf("│ Current Day:         %-28d  │\n", wsched.currentDay);
       printf("│ Current Horario:     %-28d  │\n", wsched.currentHorario);
       printf("│ Current Start Hour:  %-28d  │\n", wsched.currentStartHour);
       printf("│ Current End Hour:    %-28d  │\n", wsched.currentEndHour);
       printf("│ Current PWM Duty:    %-28d  │\n", wsched.currentPwmDuty);
       printf("│ Schedule Status:     %-28d  │\n", wsched.status);

       printf("└────────────────────────────────────────────────────┘\n\n");
}

/**
 * @brief Display device information section
 * 
 * Shows boot count, reset reasons, version info, guard date,
 * display status, and network settings.
 */
void show_device_info(time_t bootdate, time_t guardDate)
{
    const esp_app_desc_t *mip = esp_app_get_description();
    
//     printf("%s", LYELLOW);
    printf("\n┌─────────────────────────────────────────────────────────────┐\n");
    printf("│%s%s                   DEVICE INFORMATION                        %s│\n",RESETC,BK_YELLOW,RESETC);
    printf("├─────────────────────────────────────────────────────────────┤\n");
       //  printf("%s", RESETC);
    printf("│ %sBoot Count: %s                                                │\n",BK_GRAY,RESETC);
    printf("│   %-57d │\n", theConf.bootcount);
    printf("│ %sLast Reset & Reason:%s                                        │\n",BK_GRAY,RESETC);
    printf("│   Reset: %-6d  Reason: %-34d │\n", theConf.lastResetCode, theConf.lastResetCode);
    printf("│ %sLog Level & Down Time:%s                                      │\n",BK_GRAY,RESETC);
    printf("│   Level: %-6d  Down Time: %-28lu    │\n", theConf.loglevel, theConf.downtime);
    printf("│ %sLast Reboot:%s                                                │\n",BK_GRAY,RESETC);
    char reboot_str[60];
    strftime(reboot_str, sizeof(reboot_str), "  %Y-%m-%d %H:%M:%S", localtime((time_t*)&bootdate));
    printf("│ %-59s │\n", reboot_str);
    printf("│ %sConfiguration Flags:%s                                        │\n",BK_GRAY,RESETC);
    printf("│   Meter: %-6d  MQTT: %-6d  Send Meter: %-16d │\n", theConf.meterconf, mqttf, sendMeterf);
    printf("│ %sGuard Date:%s                                                 │\n",BK_GRAY,RESETC);
    strftime(reboot_str, sizeof(reboot_str), "  %Y-%m-%d %H:%M:%S", localtime(&guardDate));
    printf("│ %-59s │\n", reboot_str);
    printf("│ %sDisplay & System Status:%s                                    │\n",BK_GRAY,RESETC);
    printf("│   Display Active: %-42s│\n", gdispf?"Yes":"No");
    printf("│ %sVersion Information:%s                                        │\n",BK_GRAY,RESETC);
    printf("│   App: %-11s  IDF: %-33s│\n", mip->version, mip->idf_ver);
    printf("│   Project: %-48s │\n", mip->project_name);
    printf("│   Compiled: %s @ %-34s│\n", mip->date, mip->time);
    printf("│   Latest Sent: %-44s │\n", theConf.lastVersion);
    printf("│ %sNetwork Settings:%s                                           │\n",BK_GRAY,RESETC);
    printf("│   Mesh Delay: %-10s  Login Wait: %-22d│\n", theConf.delay_mesh?"Yes":"No", theConf.loginwait);
    printf("└─────────────────────────────────────────────────────────────┘\n\n");
}

/**
 * @brief Display production configuration section
 * 
 * Shows blower mode, active profile, debug flags, and cycle status.
 */
void show_production_config()
{
//     printf("%s", CYAN);     // 63
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│%s%s              WORKING   CONFIGURATION                        %s│\n",RESETC,BK_CYAN,RESETC);
    printf("├─────────────────────────────────────────────────────────────┤\n");
       //  printf("%s", RESETC);
//     printf("│ Blower Mode: %-47d│\n", theConf.blower_mode);
//     printf("│ Active Profile: %1d | Start Day: %-33d│\n", theConf.activeProfile, theConf.dayCycle);
    printf("│ Is Master Node: %-44s│\n", theConf.masternode?"Yes":"No ");
    printf("│ Debug Flags (0x%X): ", theConf.debug_flags);
    if((theConf.debug_flags >> dSCH) & 1U)   printf("Schedule ");
    if((theConf.debug_flags >> dMESH) & 1U)  printf("Mesh ");
    if((theConf.debug_flags >> dBLE) & 1U)   printf("Ble ");
    if((theConf.debug_flags >> dMQTT) & 1U)  printf("Mqtt ");
    if((theConf.debug_flags >> dXCMDS) & 1U) printf("Xcmds ");
    if((theConf.debug_flags >> dBLOW) & 1U)  printf("Blower ");
    if((theConf.debug_flags >> dLOGIC) & 1U) printf("Logic ");
    if((theConf.debug_flags >> dMODBUS) & 1U) printf("Modbus ");
    printf("│\n");
//     if(theBlower.getScheduleStatus()==BLOWERON)
//         printf("│ Cycle: %1d | Day: %3d | Timer Div: %3d Status %2d│\n", ck, ck_d, theConf.test_timer_div, theBlower.getScheduleStatus());
//     else
//         printf("│ Status: Waiting for Production Cycle start    %d│\n", theBlower.getScheduleStatus());
    printf("└─────────────────────────────────────────────────────────────┘\n\n");
}

/**
 * @brief Display MQTT configuration section
 * 
 * Shows MQTT topics and server credentials.
 */
void show_mqtt_config()
{
//     printf("%s", MAGENTA);
    printf("┌─────────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│%s%s                               MQTT CONFIGURATION                                %s│\n",RESETC,BK_MAGENTA,RESETC);
    printf("├─────────────────────────────────────────────────────────────────────────────────┤\n");
       //  printf("%s", RESETC);
    printf("│ Command Topic: %-65s│\n", cmdQueue);
    printf("│ Info Topic:    %-65s│\n", infoQueue);
    printf("│ Alarm Topic:   %-65s│\n", alarmQueue);
    printf("│ Server: [%-30s] | User: [%-10s] | Pass: [%-8s]│\n", theConf.mqttServer, theConf.mqttUser, theConf.mqttPass);
    printf("└─────────────────────────────────────────────────────────────────────────────────┘\n\n");
}

/**
 * @brief Display network and mesh configuration section
 * 
 * Shows mesh topology, routing table, and timing information.
 * Only displays if mesh is enabled.
 */
void show_network_mesh(wifi_config_t conf, mesh_addr_t bssid, unsigned char *mac_base, 
                       mesh_type_t typ, char *my_mac, TickType_t xRemainingTime)
{
    if(!meshf)
        return;
        
    char *tipo[]={"Idle", "ROOT", "NODE", "LEAF", "STA"};
    int routet;
        
    mesh_addr_t mmeshid;
    esp_mesh_get_id(&mmeshid);

//     printf("%s", YELLOW);
    printf("┌──────────────────────────────────────────────────────────────────┐\n");
    printf("│%s%s                 NETWORK & MESH CONFIGURATION                     %s│\n",RESETC,BK_YELLOW,RESETC);
    printf("├──────────────────────────────────────────────────────────────────┤\n");
       //  printf("%s", RESETC);
    printf("│ %sMesh ID:%s " MACSTR "  SubNode: %2d | Pool: %3d | Unit: %2d   │\n", BK_GRAY,RESETC,
           MAC2STR(MESH_ID), theConf.subnode, theConf.poolid, theConf.unitid);
    printf("│ STA Mode:  SSID: %-27s   Pass: %-10s  │\n", conf.sta.ssid, conf.sta.password);
    printf("│ %sFlash Config:%s  SSID: %-23s   Pass: %-10s  │\n", BK_GRAY,RESETC, theConf.thessid, theConf.thepass);
    printf("│ Parent BSSID: " MACSTR " %-32s │\n", MAC2STR(bssid.addr)," ");
    printf("│ %sAP Mode:%s  SSID: %-28s   Pass: %-10s  │\n",BK_GRAY,RESETC, conf.ap.ssid, conf.ap.password);
    printf("│ Node Type: %-53s │\n", tipo[typ]);
    printf("│ %sMAC Address:%s " MACSTR " %-34s│\n",BK_GRAY,RESETC, MAC2STR(mac_base)," ");
    printf("│ Mesh ID: " MACSTR " %-37s │\n", MAC2STR(mmeshid.addr)," ");
    printf("│ %sDevice State:%s %-50s │\n", BK_GRAY,RESETC,esp_mesh_is_device_active?"UP":"DOWN");
    printf("└──────────────────────────────────────────────────────────────────┘\n\n");

    // ===== TIMING INFORMATION =====
//     printf("%s", WHITEC);
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│%s%s               TIMING INFORMATION                            %s│\n",RESETC,BK_GREEN,RESETC);
    printf("├─────────────────────────────────────────────────────────────┤\n");
       //  printf("%s", RESETC);
    printf("│ Next Send Data: %dms | Base Time: %d | Repeat Timer: %d    │\n", 
           xRemainingTime, theConf.baset, theConf.repeat);
    printf("└─────────────────────────────────────────────────────────────┘\n\n");

    // ===== MESH ROUTING TABLE =====
    esp_err_t err = esp_mesh_get_routing_table((mesh_addr_t *) &s_route_table, CONFIG_MESH_ROUTE_TABLE_SIZE * 6, &routet);

    if(err == ESP_OK)
    {
       //  printf("%S", LGREEN);
        printf("┌─────────────────────────────────────────────────────────────────────┐\n");
        printf("│%s%s                       MESH NETWORK TOPOLOGY                         %s│\n",RESETC,BK_RED,RESETC);
        printf("├─────────────────────────────────────────────────────────────────────┤\n");
       //      printf("%s", RESETC);
        if(routet < 11)
        {
            printf("│ Total Nodes: %-54d │\n", routet);
            printf("├─────────────────────────────────────────────────────────────────────┤\n");
            for (int a = 0; a < routet; a++)
            {
                printf("│ [%2d] MAC: " MACSTR " %3s |  Unit: %-3s | Send: %-1s | Power: %-3s │\n",
                       a, MAC2STR(s_route_table[a].addr),
                       MAC_ADDR_EQUAL(s_route_table[a].addr, my_mac)?"*ME":"   ",
                       " ",
                       "Y",
                       "ON");
            }
        }
        else
        {
            printf("│ Total Nodes: %-55d │\n", routet);
        }
        printf("└─────────────────────────────────────────────────────────────────────┘\n\n");
    }
}

/**
 * @brief Display statistics section
 * 
 * Shows bytes/messages in/out, connection stats, and last activity.
 */
void show_statistics(time_t now)
{
//     printf("%s", BLUE);
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│%s%s                    STATISTICS                               %s│\n",RESETC,BK_BLUE,RESETC);
    printf("├─────────────────────────────────────────────────────────────┤\n");
       //  printf("%s", RESETC);
    printf("│ Bytes Out: %-16d │  Bytes In: %-18d │\n", theBlower.getStatsBytesOut(), theBlower.getStatsBytesIn());
    printf("│ Messages In: %-14d │ Messages Out: %-15d │\n", theBlower.getStatsMsgIn(), theBlower.getStatsMsgOut());
    printf("│ STA Connections: %-10d │ STA Disconnections: %-9d │\n", theBlower.getStatsStaConns(), theBlower.getStatsStaDiscos());
    printf("│ Last Activity: %s", ctime(&now));
    printf("└─────────────────────────────────────────────────────────────┘\n\n");
}

/**
 * @brief Display system configuration section
 * 
 * Shows expected nodes and connection settings.
 */
void show_system_config()
{
//     printf("%s", GRAY);
    printf("┌─────────────────────────────────────────────────────────────┐\n");
    printf("│%s%s                 SYSTEM CONFIGURATION                        %s│\n",RESETC,BK_CYAN,RESETC);
    printf("├─────────────────────────────────────────────────────────────┤\n");
       //  printf("%s", RESETC);
    printf("│ Expected Nodes: %-43lu │\n", theConf.totalnodes);
    printf("│ Expected Connections: %-37lu │\n", theConf.conns);
    printf("└─────────────────────────────────────────────────────────────┘\n\n");
}

/**
 * @brief Display detailed information for the first profile
 * 
 * Shows complete profile structure including:
 * - Profile name and version
 * - Issue and expiration dates
 * - Number of cycles
 * - For each cycle: day, duration, number of schedules (horarios)
 * - For each horario: start hour, duration, PWM duty cycle
 */
void show_first_profile()
{
    profile_t *profile = &theConf.profiles[0];
    
//     printf("%s\n", MAGENTA);
    printf("┌──────────────────────────────────────────────────────────────────┐\n");
    printf("│%s%s                      FIRST PROFILE DETAILS                       %s│\n",RESETC,BK_MAGENTA,RESETC);
    printf("├──────────────────────────────────────────────────────────────────┤\n");
       //  printf("%s", RESETC);
    printf("│ Name:            %-47s │\n", profile->name);
    printf("│ Version:         %-47s │\n", profile->version);
    
    char date_buf[30];
    struct tm timeinfo;
    
    if (profile->issued > 0) {
        localtime_r(&profile->issued, &timeinfo);
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        printf("│ Issued:          %-47s │\n", date_buf);
    } else {
        printf("│ Issued:          %-47s │\n", "Not Set");
    }
    
    if (profile->expires > 0) {
        localtime_r(&profile->expires, &timeinfo);
        strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        printf("│ Expires:         %-47s │\n", date_buf);
    } else {
        printf("│ Expires:         %-47s │\n", "Not Set");
    }
    
    printf("│ Number of Cycles: %-46d │\n", profile->numCycles);
    printf("└──────────────────────────────────────────────────────────────────┘\n\n");
    
    // Display each cycle
    for (int i = 0; i < profile->numCycles && i < MAXCICLOS; i++) {
        ciclo_t *cycle = &profile->cycle[i];
        
        printf("  ┌─ %sCycle %1d%s ───────────────────────────────────────────────────┐\n", BK_GRAY, i, RESETC);
        printf("  │ Day:             %-42d │\n", cycle->day);
        printf("  │ Duration:        %-42d │\n", cycle->duration);
        printf("  │ Num Schedules:   %-42d │\n", cycle->numHorarios);
        printf("  └─────────────────────────────────────────────────────────────┘\n");
        
        if (cycle->numHorarios > 0) {
            printf("    ┌───────────────────────────────────────────────────────────┐\n");
            printf("    │%s %-10s │ %-15s │ %-15s │ %-8s %s│\n",BK_BLUE, "Schedule", "Start Hour", "Duration", "PWM Duty",RESETC);
            printf("    ├────────────┼─────────────────┼─────────────────┼──────────┤\n");
            
            for (int j = 0; j < cycle->numHorarios && j < MAXHORARIOS; j++) {
                horario_t *horario = &cycle->horarios[j];
                printf("    │ %-10d │ %-15d │ %-15u │ %-8d │\n", 
                       j, horario->hourStart, horario->horarioLen, horario->pwmDuty);
            }
            
            printf("    └────────────┴─────────────────┴─────────────────┴──────────┘\n");
        }
        printf("\n");
    }
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
    char            myssid[20];
    time_t          now;
    struct tm       timeinfo;
    TickType_t      xRemainingTime = 0;
    mesh_type_t     typ = MESH_IDLE;
    char            my_mac[8] = {0};
    uint8_t         min, secs;
    mesh_addr_t     bssid;
    time_t          guardDate, bootdate;
    wifi_config_t   conf;
    unsigned char   mac_base[6] = {0};

        // Parse command line arguments
    int nerrors = arg_parse(argc, argv, (void **)&configArgs);
    if (nerrors != 0) {
        arg_print_errors(stderr, configArgs.end, argv[0]);
        return 0;
    }

    time(&now);
    localtime_r(&now, &timeinfo);
    bzero(myssid, sizeof(myssid));

    esp_efuse_mac_get_default(mac_base);
    esp_read_mac(mac_base, ESP_MAC_WIFI_STA);

    if(mesh_started)
    {
        memcpy(my_mac, mesh_netif_get_station_mac(), 6);
        if(esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK)
        {
            strcpy(myssid, (char*)conf.sta.ssid);
        }
        else
        {
            printf("Error reading wifi config\n");
        }
        typ = esp_mesh_get_type();
    }

    min = 10;
    secs = 10 * 2 - (min * 60);
    timeinfo.tm_min = min;
    timeinfo.tm_sec = secs; 
    time_t nexthour = mktime(&timeinfo);
    int faltan = nexthour - now;

    esp_mesh_get_parent_bssid(&bssid);
    const esp_app_desc_t *mip = esp_app_get_description();
    if(mip)
        printf("\t\t Mesh Configuration Date %s ", ctime(&now));

    if(!theConf.ptch)
        printf("Virgin Chip\n");
    printf("%s", RESETC);

    uint32_t nada = theBlower.getLastUpdate();
    guardDate = (time_t)nada;
    bootdate = (time_t)theConf.lastRebootTime;

    // Calculate timing information
    if(collectTimer && xTimerIsTimerActive(collectTimer))
    {
        xRemainingTime = xTimerGetExpiryTime(collectTimer) - xTaskGetTickCount();
    }

    // Display all configuration sections
//     if (configArgs.all->count || configArgs.device->count)  
       show_device_info(bootdate, guardDate);
    if (configArgs.all->count || configArgs.prod->count)         
       show_production_config();
    if (configArgs.all->count || configArgs.mqtt->count)         
       show_mqtt_config();
    if (configArgs.all->count || configArgs.meshnet->count)         
       show_network_mesh(conf, bssid, mac_base, typ, my_mac, xRemainingTime);
    if (configArgs.all->count || configArgs.stats->count)         
       show_statistics(now);
    if (configArgs.all->count || configArgs.system->count)         
       show_system_config();
    if (configArgs.all->count || configArgs.blow->count)         
       print_blower("Blower", theBlower.getSolarSystem(), false);
    if (configArgs.all->count || configArgs.sch->count)         
       show_schedule_info();
    if (configArgs.all->count || configArgs.profile->count)         
           show_first_profile();
    if (configArgs.all->count || configArgs.limits->count)         
           show_limits();
    if (configArgs.all->count || configArgs.modbus->count)         
           show_modbus();
    
    printf("%s", RESETC);
    return 0;
}
