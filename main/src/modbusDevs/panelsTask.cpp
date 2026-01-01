/**
 * @file panelsTask.cpp
 * @brief Solar panel monitoring task for solar inverter via Modbus
 * 
 * This task handles periodic reading of solar panel parameters from the inverter
 * including PV string voltages, currents, and charge state. Monitors both PV string 1
 * and PV string 2 via RS485/Modbus protocol.
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

static constexpr int MAX_SENSORS = 5;  ///< Maximum number of panel sensors

// ============================================================================
// Private Functions
// ============================================================================

/**
 * @brief Print panel sensor data for debugging
 * 
 * Logs solar panel monitoring data including charge state and both PV strings'
 * voltage and current readings.
 * 
 * @param data_ptr Pointer to PV panel data structure (cast from void*)
 * @param errors Pointer to error code array
 * 
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
static void print_panel_data(void *data_ptr, const int *errors)
{
    if (errors[0] != 0)
        return;  // Skip printing if errors occurred
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;  // Skip if debug not enabled

    const pvPanel_t *pvPanel = (const pvPanel_t*)data_ptr;

    ESP_LOGI(TAG, "%sPanels [%s] - String1:[%.02fV / %.02fA] String2:[%.02fV / %.02fA]",LYELLOW,
             pvPanel->chargeCurr ? "CHARGING" : "DISCHARGING",
             pvPanel->pv1Volts,                    
             pvPanel->pv1Amp,
             pvPanel->pv2Volts,
             pvPanel->pv2Amp);
}

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Main solar panel monitoring task
 * 
 * Periodically reads solar panel parameters from the inverter via RS485/Modbus.
 * Uses the generic modbus_task_runner to handle the monitoring loop.
 * 
 * @param pArg Task parameter (unused)
 * 
 * @note This is a FreeRTOS task that runs indefinitely
 * @note Task deletes itself if initialization fails
 * @note Reading interval is configured via theConf.modbus_panels.regfresh
 */
void panels_task(void *pArg)
{
    // Prepare configuration for generic task runner
    // Note: four_t = {double mux(8 bytes); int devices[4](16 bytes)} = 24 bytes total
    modbus_sensor_config_t config = {
        .addr = theConf.modbus_panels.PVAddress,
        .regfresh = theConf.modbus_panels.refresh_rate,
        .specs = (void*)&theConf.modbus_panels.PV2AmpsMux,
        .max_sensors = MAX_SENSORS,
        .spec_size_bytes = 24,  // sizeof(double) + sizeof(int[4]) = 8 + 16
        .use_per_device_addr = false
    };
    
    // Run generic task loop
    modbus_task_runner(&config, &pvPanelData, print_panel_data, MAX_SENSORS, "Panel", LYELLOW);
}