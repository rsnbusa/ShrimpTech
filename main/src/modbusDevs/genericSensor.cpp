/**
 * @file genericSensor.cpp
 * @brief Generic Modbus monitoring task for various sensor types
 * 
 * This task provides a generic wrapper for monitoring any Modbus sensor type.
 * It handles periodic reading of sensor parameters via RS485/Modbus protocol,
 * using configuration passed through the modbus_sensor_type_t structure.
 * 
 * @note Part of the ShrimpIoT Modbus device monitoring system
 */

#define GLOBAL
#include "includes.h"
#include "defines.h"
#include "typedef.h"
#include "globals.h"
#include "forwards.h"

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Generic Modbus monitoring task
 * 
 * Periodically reads sensor parameters via RS485/Modbus using configuration
 * provided through the modbus_sensor_type_t structure. Initializes descriptors,
 * sends read requests to the RS485 handler, and calls the appropriate print function.
 * 
 * @param pArg Pointer to modbus_sensor_type_t structure containing configuration
 * 
 * @note This is a FreeRTOS task that runs indefinitely
 * @note Task deletes itself if initialization fails
 * @note Reading interval is configured via the sensor's refresh rate
 */
void generic_modbus_task(void *pArg)
{
    rs485queue_t mensaje;
    int errors[20] = {0};
    void *descriptors = NULL;
    int sensor_count = 0;
    
    // Extract sensor configuration from task parameter
    modbus_sensor_type_t *modbus_sensor = (modbus_sensor_type_t*)pArg;

    if ((theConf.debug_flags >> dMODBUS) & 1U)
         MESP_LOGI(TAG, "%s%s monitoring task started", modbus_sensor->color, modbus_sensor->modbus_sensor_name);
    
    // Get sensor configuration and refresh rate
    void *sensor_specs = modbus_sensor->modbus_sensor_specs;
    int refresh_rate = ((general_4modbus_specs_t*)sensor_specs)->regfresh;

    // Initialize sensor descriptors from configuration
    descriptors = initialize_sensor_descriptors(
        sensor_specs,
        modbus_sensor->modbus_sensor_name,
        modbus_sensor->modbus_sensor_spec_count,
        modbus_sensor->modbus_sensor_spec_columns,
        &sensor_count
    );
    
    if (descriptors == NULL)
    {
        MESP_LOGW(TAG, "%s task: No valid sensors configured, task exiting", modbus_sensor->modbus_sensor_name);
        vTaskDelete(NULL);
    }
    if ((theConf.debug_flags >> dMODBUS) & 1U)
        MESP_LOGI(TAG, "%s task: Initialized %d descriptors, refresh rate: %d min", 
             modbus_sensor->modbus_sensor_name, sensor_count, refresh_rate);
    
    // Prepare message structure for RS485 communication
    mensaje.numCids = sensor_count;
    mensaje.descriptors = (mb_parameter_descriptor_t*)descriptors;
    mensaje.requester = xTaskGetCurrentTaskHandle();
    mensaje.errCode = &errors[0];
    mensaje.numerrs=sensor_count;
    
    // Zero out data structure before use
    // todo set to zero if read only
    // bzero(modbus_sensor->modbus_sensor_data, modbus_sensor->modbus_sensor_data_size);
    mensaje.dataReceiver = modbus_sensor->modbus_sensor_data;

    vTaskDelay(pdMS_TO_TICKS(400));      // give time to be suspended if caller wants to start it suspended
    // Main monitoring loop
    while (true)
    {
        // Send read request to RS485 handler task
        if (xQueueSend(rs485Q, &mensaje, pdMS_TO_TICKS(100)) != pdPASS)
        {
            MESP_LOGE(TAG, "%s task: Failed to send message to RS485 queue", modbus_sensor->modbus_sensor_name);
        }
        else
        {
            // Wait for RS485 task to complete the read operation
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            
            // Print the received data using sensor-specific print function
            if (modbus_sensor->modbus_print_function)
            {
                modbus_sensor->modbus_print_function(modbus_sensor->modbus_sensor_data, errors,modbus_sensor->color,sensor_count);
            }
        }
        
        // Wait before next reading cycle refresh per configuration * 1000 ms * modbus_mux which should be 60 for minutes
        vTaskDelay(pdMS_TO_TICKS(refresh_rate * 1000));
    }
}