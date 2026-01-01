/**
 * @file sensorTask.cpp
 * @brief Environmental sensor monitoring task via Modbus
 * 
 * This task handles periodic reading of environmental sensor data including
 * water temperature, dissolved oxygen (DO), DO percentage, pH, air temperature,
 * and air humidity. Communicates with sensors over RS485/Modbus protocol.
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

static constexpr int MAX_SENSORS = 5;  ///< Maximum number of environmental sensors

// ============================================================================
// Private Functions
// ============================================================================

/**
 * @brief Print environmental sensor data for debugging
 * 
 * Logs environmental monitoring data including water temperature, dissolved
 * oxygen concentration and percentage.
 * 
 * @param data_ptr Pointer to sensor data structure (cast from void*)
 * @param errors Pointer to error code array
 * 
 * @note Checks errors[1] instead of errors[0] for multi-sensor setups
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
static void print_sensor_data(void *data_ptr, const int *errors)
{
    if (errors[1] != 0)
        return;  // Skip printing if errors occurred
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;  // Skip if debug not enabled

    const sensor_t *sensorData = (const sensor_t*)data_ptr;

    ESP_LOGI(TAG, "%sSensors - WaterTemp:%.02f°C DO%%.%.02f%% DO:%.02fppm",BLUE,
             sensorData->WTemp,
             sensorData->percentDO * 100.0,
             sensorData->DO);
}

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Main environmental sensor monitoring task
 * 
 * Periodically reads environmental sensor data via RS485/Modbus.
 * Uses the generic modbus_task_runner to handle the monitoring loop.
 * 
 * @param pArg Task parameter (unused)
 * 
 * @note This is a FreeRTOS task that runs indefinitely
 * @note Task deletes itself if initialization fails
 * @note Reading interval is configured via theConf.modbus_sensors.regfresh
 * @note Supports multiple sensors with individual Modbus slave addresses
 */
void sensor_task(void *pArg)
{
    // Prepare configuration for generic task runner
    // Note: five_t = {double mux(8 bytes); int devices[5](20 bytes)} = 32 bytes total (with padding)
    modbus_sensor_config_t config = {
        .addr = 0,  // Not used for sensors (each sensor has its own address)
        .regfresh = theConf.modbus_sensors.refresh_rate,
        .specs = (void*)&theConf.modbus_sensors.humMux,
        .max_sensors = MAX_SENSORS,
        .spec_size_bytes = 32,  // sizeof(double) + sizeof(int[5]) = 8 + 20 + padding
        .use_per_device_addr = true  // Sensors use individual addresses from specs array
    };
    
    // Run generic task loop
    modbus_task_runner(&config, &sensorData, print_sensor_data, MAX_SENSORS, "Sensor", BLUE);
}