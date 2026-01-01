/**
 * @file energyTask.cpp
 * @brief Energy monitoring task for solar inverter via Modbus
 * 
 * This task handles periodic reading of energy-related parameters from the solar
 * inverter including battery charge/discharge data, energy generation, and consumption
 * metrics. It communicates with the inverter over RS485/Modbus protocol.
 * 
 * @note Part of the ShrimpIoT Modbus device monitoring system
 */

#define GLOBAL
#include "includes.h"
#include "defines.h"
#include "typedef.h"
#include "globals.h"
#include "modbus_task_utils.h"

// ============================================================================
// Constants
// ============================================================================

static constexpr int MAX_SENSORS = 10;  ///< Maximum number of energy sensors

// ============================================================================
// Private Functions
// ============================================================================

/**
 * @brief Print energy sensor data for debugging
 * 
 * Logs energy monitoring data including battery charge/discharge statistics,
 * energy generation, and consumption. Output varies based on charging state.
 * 
 * @param data_ptr Pointer to energy data structure (cast from void*)
 * @param errors Pointer to error code array
 * 
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
static void print_energy_data(void *data_ptr, const int *errors)
{
    if (errors[0] != 0)
        return;  // Skip printing if errors occurred
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;  // Skip if debug not enabled

    const energy_t *energy = (const energy_t*)data_ptr;

    // Print charging or discharging data based on current state
    if (pvPanelData.chargeCurr)
    {
        ESP_LOGI(TAG, "%sEnergy [CHARGING] - BatChgAH(Today:%u Total:%u) GenEnergy:%.02fkWh BatChg:%.02fkWh",CYAN,
                 energy->batChgAHToday,
                 energy->batChgAHTotal,
                 energy->generateEnergyToday,
                 energy->batChgkWhToday);   
    }
    else
    {
        ESP_LOGI(TAG, "%sEnergy [DISCHARGING] - BatDischgAH(Today:%u Total:%u) UsedEnergy:%.02fkWh LoadConsumTotal:%.02fkWh BatDischg:%.02fkWh GenLoadConsum:%.02fkWh",CYAN,
                 energy->batDischgAHToday,
                 energy->batDischgAHTotal,
                 energy->usedEnergyToday,
                 energy->gLoadConsumLineTotal,
                 energy->batDischgkWhToday,
                 energy->genLoadConsumToday);   
    }
}

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Main energy monitoring task
 * 
 * Periodically reads energy parameters from the solar inverter via RS485/Modbus.
 * Uses the generic modbus_task_runner to handle the monitoring loop.
 * 
 * @param pArg Task parameter (unused)
 * 
 * @note This is a FreeRTOS task that runs indefinitely
 * @note Task deletes itself if initialization fails
 * @note Reading interval is configured via theConf.modbus_inverter.regfresh
 */
void energy_task(void *pArg)
{
    // Prepare configuration for generic task runner
    // Note: four_t = {double mux(8 bytes); int devices[4](16 bytes)} = 24 bytes total
    modbus_sensor_config_t config = {
        .addr = theConf.modbus_inverter.InverterAddress,
        .regfresh = theConf.modbus_inverter.refresh_rate,
        .specs = (void*)&theConf.modbus_inverter.I10_LoadUsedHoyMux,
        .max_sensors = MAX_SENSORS,
        .spec_size_bytes = 24,  // sizeof(double) + sizeof(int[4]) = 8 + 16
        .use_per_device_addr = false
    };
    
    // Run generic task loop
    modbus_task_runner(&config, &energyData, print_energy_data, MAX_SENSORS, "Energy", CYAN);
}