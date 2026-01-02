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

extern descriptor_array_t * initialize_sensor_descriptors(const general_4modbus_specs_t *sensorinfo,
            char *whichDev,int MAXSENSORS,int *sensor_count);

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Main energy monitoring task
 * 
 * Periodically reads energy parameters from the solar inverter via RS485/Modbus.
 * Initializes sensor descriptors from configuration, sends read requests to the
 * RS485 handler task, and logs the received data.
 * 
 * @param pArg Task parameter (unused)
 * 
 * @note This is a FreeRTOS task that runs indefinitely
 * @note Task deletes itself if initialization fails
 * @note Reading interval is configured via theConf.modbus_inverter.regfresh
 */
void generic_modbus_task(void *pArg)
{
    rs485queue_t mensaje;
    int errors[20] = {0};  // Initialize error array
    // int errors[MAX_MODBUS_ERRORS] = {0};  // Initialize error array
    void *devicesarr;
    int sensor_count=0;
    modbus_sensor_type_t *modbus_sensor=(modbus_sensor_type_t*)pArg;
    ESP_LOGI(TAG, "%s%s monitoring task started",modbus_sensor->color,modbus_sensor->modbus_sensor_name);
    
    // Get inverter configuration from global settings
    const general_4modbus_specs_t *sensorinfo = (const general_4modbus_specs_t *)modbus_sensor->modbus_sensor_specs;
    const int refreshrate = sensorinfo->regfresh;

    devicesarr = initialize_sensor_descriptors( sensorinfo,modbus_sensor->modbus_sensor_name,
                    modbus_sensor->modbus_sensor_spec_count,&sensor_count);
    if (devicesarr == NULL )
    {
        ESP_LOGW(TAG, "%s task: No valid sensors configured, task exiting",modbus_sensor->modbus_sensor_name);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "%s task: Initialized %d sensors, refresh rate: %d min", modbus_sensor->modbus_sensor_name, sensor_count, refreshrate);
    
    // Prepare message structure for RS485 communication
    mensaje.numCids = sensor_count;
    mensaje.descriptors = (mb_parameter_descriptor_t*)devicesarr;
    // zeor data structure
    bzero(modbus_sensor->modbus_sensor_data, modbus_sensor->modbus_sensor_data_size);
    mensaje.dataReceiver = (void*)modbus_sensor->modbus_sensor_data;
    mensaje.requester = xTaskGetCurrentTaskHandle();
    mensaje.errCode = &errors[0];

    // Main monitoring loop
    while (true)
    {
        // Send read request to RS485 handler task
        if (xQueueSend(rs485Q, &mensaje, pdMS_TO_TICKS(100)) != pdPASS)
        {
            ESP_LOGE(TAG, "Energy task: Failed to send message to RS485 queue");
        }
        else
        {
            // Wait for RS485 task to complete the read operation
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            
            // Log the received data if debug enabled
            // print_sensor_data(energyData, errors);
            // ESP_LOGE(TAG, "%s task finished",modbus_sensor->modbus_sensor_name);
            if(modbus_sensor->modbus_print_function)
                modbus_sensor->modbus_print_function(modbus_sensor->modbus_sensor_data, errors);

        }
        // Wait before next reading cycle
        vTaskDelay(pdMS_TO_TICKS(refreshrate * 1000 * theConf.minutes));
    }
}