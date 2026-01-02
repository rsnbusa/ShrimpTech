
#define GLOBAL
#include "includes.h"
#include "defines.h"
#include "typedef.h"
#include "globals.h"

/**
 * @brief Print energy sensor data for debugging
 * 
 * Logs energy monitoring data including battery charge/discharge statistics,
 * energy generation, and consumption. Output varies based on charging state.
 * 
 * @param energy Reference to energy data structure
 * @param errors Pointer to error code array
 * 
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
 void print_energy_data( void *energy,  int *errors)
//  void print_energy_data( energy_t &energy, const int *errors)
{
    if (errors[0] != 0)
        return;  // Skip printing if errors occurred
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;  // Skip if debug not enabled

    // Print charging or discharging data based on current state
    if (pvPanelData.chargeCurr)
    {
        ESP_LOGI(TAG, "%sEnergy [CHARGING] - BatChgAH(Today:%u Total:%u) GenEnergy:%.02fkWh BatChg:%.02fkWh",CYAN,
                 ((energy_t*)energy)->batChgAHToday,
                 ((energy_t*)energy)->batChgAHTotal,
                 ((energy_t*)energy)->generateEnergyToday,
                 ((energy_t*)energy)->batChgkWhToday);   
    }
    else
    {
        ESP_LOGI(TAG, "%sEnergy [DISCHARGING] - BatDischgAH(Today:%u Total:%u) UsedEnergy:%.02fkWh LoadConsumTotal:%.02fkWh BatDischg:%.02fkWh GenLoadConsum:%.02fkWh",CYAN,
                 ((energy_t*)energy)->batDischgAHToday,
                 ((energy_t*)energy)->batDischgAHTotal,
                 ((energy_t*)energy)->usedEnergyToday,
                 ((energy_t*)energy)->gLoadConsumLineTotal,
                 ((energy_t*)energy)->batDischgkWhToday,
                 ((energy_t*)energy)->genLoadConsumToday);   
    }
}


/**
 * @brief Print battery sensor data for debugging
 * 
 * Logs battery monitoring data including State of Charge (SOC), State of Health (SOH),
 * battery cycle count, and BMS temperature.
 * 
 * @param batteryData Reference to battery data structure
 * @param errors Pointer to error code array
 * 
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
void print_battery_data(void * batteryData, int *errors)
{
    if (errors[0] != 0)
        return;  // Skip printing if errors occurred
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;  // Skip if debug not enabled

    ESP_LOGI(TAG, "%sBattery - SOC:%d%% SOH:%d%% CycleCount:%d BmsTemp:%.02f°C",GRAY,
             ((battery_t*)batteryData)->batSOC,
             ((battery_t*)batteryData)->batSOH,
             ((battery_t*)batteryData)->batteryCycleCount,
             ((battery_t*)batteryData)->batBmsTemp);
}


/**
 * @brief Print panel sensor data for debugging
 * 
 * Logs solar panel monitoring data including charge state and both PV strings'
 * voltage and current readings.
 * 
 * @param pvPanel Reference to PV panel data structure
 * @param errors Pointer to error code array
 * 
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
void print_panel_data(void * pvPanel, int *errors)
{
    if (errors[0] != 0)
        return;  // Skip printing if errors occurred
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;  // Skip if debug not enabled

    ESP_LOGI(TAG, "%sPanels [%s] - String1:[%.02fV / %.02fA] String2:[%.02fV / %.02fA]",LYELLOW,
             ((pvPanel_t*)pvPanel)->chargeCurr ? "CHARGING" : "DISCHARGING",
             ((pvPanel_t*)pvPanel)->pv1Volts,                    
             ((pvPanel_t*)pvPanel)->pv1Amp,
             ((pvPanel_t*)pvPanel)->pv2Volts,
             ((pvPanel_t*)pvPanel)->pv2Amp);
}
