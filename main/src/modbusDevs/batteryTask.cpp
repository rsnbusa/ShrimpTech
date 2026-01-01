/**
 * @file batteryTask.cpp
 * @brief Battery monitoring task for solar inverter via Modbus
 * 
 * This task handles periodic reading of battery-related parameters from the solar
 * inverter including State of Charge (SOC), State of Health (SOH), cycle count,
 * and BMS temperature. It communicates with the inverter over RS485/Modbus protocol.
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

static constexpr int MAX_SENSORS = 4;  ///< Maximum number of battery sensors

// ============================================================================
// Private Functions
// ============================================================================

/**
 * @brief Print battery sensor data for debugging
 * 
 * Logs battery monitoring data including State of Charge (SOC), State of Health (SOH),
 * battery cycle count, and BMS temperature.
 * 
 * @param data_ptr Pointer to battery data structure (cast from void*)
 * @param errors Pointer to error code array
 * 
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
static void print_battery_data(void *data_ptr, const int *errors)
{
    if (errors[0] != 0)
        return;  // Skip printing if errors occurred
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;  // Skip if debug not enabled

    const battery_t *batteryData = (const battery_t*)data_ptr;

    ESP_LOGI(TAG, "%sBattery - SOC:%d%% SOH:%d%% CycleCount:%d BmsTemp:%.02f°C",GRAY,
             batteryData->batSOC,
             batteryData->batSOH,
             batteryData->batteryCycleCount,
             batteryData->batBmsTemp);
}

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Main battery monitoring task
 * 
 * Periodically reads battery parameters from the solar inverter via RS485/Modbus.
 * Uses the generic modbus_task_runner to handle the monitoring loop.
 * 
 * @param pArg Task parameter (unused)
 * 
 * @note This is a FreeRTOS task that runs indefinitely
 * @note Task deletes itself if initialization fails
 * @note Reading interval is configured via theConf.modbus_battery.regfresh
 */
void battery_task(void *pArg)
{
    // Prepare configuration for generic task runner
    // Note: four_t = {double mux(8 bytes); int devices[4](16 bytes)} = 24 bytes total
    modbus_sensor_config_t config = {
        .addr = theConf.modbus_battery.batAddress,
        .regfresh = theConf.modbus_battery.refresh_rate,
        .specs = (void*)&theConf.modbus_battery.tempMux,
        .max_sensors = MAX_SENSORS,
        .spec_size_bytes = 24,  // sizeof(double) + sizeof(int[4]) = 8 + 16
        .use_per_device_addr = false
    };
    
    // Run generic task loop
    modbus_task_runner(&config, &batteryData, print_battery_data, MAX_SENSORS, "Battery", GRAY);
}