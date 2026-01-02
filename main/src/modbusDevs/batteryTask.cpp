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

extern descriptor_array_t * initialize_sensor_descriptors(const general_4modbus_specs_t *sensorinfo,
            char *whichDev,int MAXSENSORS,int *sensor_count);


// ============================================================================
// Constants
// ============================================================================

static constexpr int BAT_SENSORS = 4;          ///< Maximum number of battery sensors
static constexpr int MAX_ERRORS = BAT_SENSORS; ///< Error array size
static constexpr uint32_t BYTE_MASK = 0xFF;    ///< Byte mask for data parsing

// ============================================================================
// Private Functions
// ============================================================================

// /**
//  * @brief Print battery sensor data for debugging
//  * 
//  * Logs battery monitoring data including State of Charge (SOC), State of Health (SOH),
//  * battery cycle count, and BMS temperature.
//  * 
//  * @param batteryData Reference to battery data structure
//  * @param errors Pointer to error code array
//  * 
//  * @note Only prints if MODBUS debug flag is enabled and no errors occurred
//  */
// static void print_sensor_data(const battery_t &batteryData, const int *errors)
// {
//     if (errors[0] != 0)
//         return;  // Skip printing if errors occurred
        
//     if (!((theConf.debug_flags >> dMODBUS) & 1U))
//         return;  // Skip if debug not enabled

//     ESP_LOGI(TAG, "%sBattery - SOC:%d%% SOH:%d%% CycleCount:%d BmsTemp:%.02f°C",GRAY,
//              batteryData.batSOC,
//              batteryData.batSOH,
//              batteryData.batteryCycleCount,
//              batteryData.batBmsTemp);
// }

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Main battery monitoring task
 * 
 * Periodically reads battery parameters from the solar inverter via RS485/Modbus.
 * Monitors State of Charge (SOC), State of Health (SOH), cycle count, and BMS
 * temperature. Initializes sensor descriptors from configuration, sends read
 * requests to the RS485 handler task, and logs the received data.
 * 
 * @param pArg Task parameter (unused)
 * 
 * @note This is a FreeRTOS task that runs indefinitely
 * @note Task deletes itself if initialization fails
 * @note Reading interval is configured via theConf.modbus_battery.regfresh
 */
void battery_task(void *pArg)
{
    rs485queue_t mensaje;
    int errors[MAX_ERRORS] = {0};  // Initialize error array
    void *devicesarr;
    int sensor_count=0;

    ESP_LOGI(TAG, "%sBattery monitoring task started",GRAY);

    // Get battery configuration from global settings... its a 4 column spec
    const general_4modbus_specs_t *sensorinfo = (const general_4modbus_specs_t *)&theConf.modbus_battery;
    const int refreshrate = sensorinfo->regfresh;

    devicesarr = initialize_sensor_descriptors( sensorinfo,(char*)"Battery",BAT_SENSORS,&sensor_count);
    if (devicesarr == NULL )
    {
        ESP_LOGW(TAG, "Battery task: No valid sensors configured, task exiting");
        vTaskDelete(NULL);
    }
    
    ESP_LOGI(TAG, "%sBattery task: Initialized %d sensors, refresh rate: %d min", GRAY, sensor_count, refreshrate);
    
    // Prepare message structure for RS485 communication
    mensaje.numCids = sensor_count;
    mensaje.descriptors = (mb_parameter_descriptor_t*)devicesarr;
    mensaje.dataReceiver = (void*)&batteryData;
    mensaje.requester = xTaskGetCurrentTaskHandle();
    mensaje.errCode = &errors[0];

    // Main monitoring loop
    while (true)
    {
        // Send read request to RS485 handler task
        if (xQueueSend(rs485Q, &mensaje, pdMS_TO_TICKS(100)) != pdPASS)
        {
            ESP_LOGE(TAG, "Battery task: Failed to send message to RS485 queue");
        }
        else
        {
            // Wait for RS485 task to complete the read operation
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            
            // Log the received data if debug enabled
            print_sensor_data(batteryData, errors);
        }

        // Wait before next reading cycle
        vTaskDelay(pdMS_TO_TICKS(refreshrate * 1000) * theConf.minutes);
    }
}