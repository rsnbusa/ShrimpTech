
/**
 * @file printSensors.cpp
 * @brief Debug printing functions for Modbus sensor data
 * 
 * Provides formatted console output for various sensor types including
 * environmental sensors, energy monitoring, battery status, and solar panels.
 * Output is controlled by MODBUS debug flag and error status.
 * 
 * @note Part of the ShrimpIoT Modbus device monitoring system
 */

#define GLOBAL
#include "includes.h"
#include "defines.h"
#include "typedef.h"
#include "globals.h"

// ============================================================================
// Environmental Sensors
// ============================================================================

/**
 * @brief Print environmental sensor data for debugging
 * 
 * Logs environmental monitoring data including water temperature, dissolved
 * oxygen concentration and percentage.
 * 
 * @param sensors Pointer to sensor_t data structure
 * @param errors Pointer to error code array
 * 
 * @note Checks errors[1] instead of errors[0] for multi-sensor setups
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
void print_sensor_data(void *sensors, int *errors,char *color)
{
    if (errors[1] != 0)
        return;
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

    sensor_t *data = (sensor_t*)sensors;

    ESP_LOGI(TAG, "%sSensors - WaterTemp:%.02f°C DO%%.%.02f%% DO:%.02fppm", color,
             data->WTemp,
             data->percentDO * 100.0,
             data->DO);
}

// ============================================================================
// Energy Monitoring
// ============================================================================

/**
 * @brief Print energy sensor data for debugging
 * 
 * Logs energy monitoring data including battery charge/discharge statistics,
 * energy generation, and consumption. Output varies based on charging state.
 * 
 * @param energy Pointer to energy_t data structure
 * @param errors Pointer to error code array
 * 
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
void print_energy_data(void *energy, int *errors,char * color)
{
    if (errors[0] != 0)
        return;
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

    energy_t *data = (energy_t*)energy;

    // Print charging or discharging data based on current state
    if (pvPanelData.chargeCurr)
    {
        ESP_LOGI(TAG, "%sEnergy [CHARGING] - BatChgAH(Today:%u Total:%u) GenEnergy:%.02fkWh BatChg:%.02fkWh", color,
                 data->batChgAHToday,
                 data->batChgAHTotal,
                 data->generateEnergyToday,
                 data->batChgkWhToday);   
    }
    else
    {
        ESP_LOGI(TAG, "%sEnergy [DISCHARGING] - BatDischgAH(Today:%u Total:%u) UsedEnergy:%.02fkWh LoadConsumTotal:%.02fkWh BatDischg:%.02fkWh GenLoadConsum:%.02fkWh", CYAN,
                 data->batDischgAHToday,
                 data->batDischgAHTotal,
                 data->usedEnergyToday,
                 data->gLoadConsumLineTotal,
                 data->batDischgkWhToday,
                 data->genLoadConsumToday);   
    }
}

// ============================================================================
// Battery Status
// ============================================================================

/**
 * @brief Print battery sensor data for debugging
 * 
 * Logs battery monitoring data including State of Charge (SOC), State of Health (SOH),
 * battery cycle count, and BMS temperature.
 * 
 * @param batteryData Pointer to battery_t data structure
 * @param errors Pointer to error code array
 * 
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
void print_battery_data(void *batteryData, int *errors,char *color)
{
    if (errors[0] != 0)
        return;
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

    battery_t *data = (battery_t*)batteryData;

    ESP_LOGI(TAG, "%sBattery - SOC:%d%% SOH:%d%% CycleCount:%d BmsTemp:%.02f°C", color,
             data->batSOC,
             data->batSOH,
             data->batteryCycleCount,
             data->batBmsTemp);
}

// ============================================================================
// Solar Panels
// ============================================================================

/**
 * @brief Print panel sensor data for debugging
 * 
 * Logs solar panel monitoring data including charge state and both PV strings'
 * voltage and current readings.
 * 
 * @param pvPanel Pointer to pvPanel_t data structure
 * @param errors Pointer to error code array
 * 
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
void print_panel_data(void *pvPanel, int *errors,char * color)
{
    if (errors[0] != 0)
        return;
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;

    pvPanel_t *data = (pvPanel_t*)pvPanel;

    ESP_LOGI(TAG, "%sPanels [%s] - String1:[%.02fV / %.02fA] String2:[%.02fV / %.02fA]", color,
             data->chargeCurr ? "CHARGING" : "DISCHARGING",
             data->pv1Volts,                    
             data->pv1Amp,
             data->pv2Volts,
             data->pv2Amp);
}
