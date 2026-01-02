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

extern descriptor_array_t * initialize_sensor_descriptors(const general_4modbus_specs_t *sensorinfo,
            char *whichDev,int MAXSENSORS,int *sensor_count);

// ============================================================================
// Constants
// ============================================================================

static constexpr int PANELS_SENSORS = 5;          ///< Maximum number of panel sensors
static constexpr int MAX_ERRORS = PANELS_SENSORS; ///< Error array size
static constexpr uint32_t BYTE_MASK = 0xFF;    ///< Byte mask for data parsing

// /**
//  * @brief Print panel sensor data for debugging
//  * 
//  * Logs solar panel monitoring data including charge state and both PV strings'
//  * voltage and current readings.
//  * 
//  * @param pvPanel Reference to PV panel data structure
//  * @param errors Pointer to error code array
//  * 
//  * @note Only prints if MODBUS debug flag is enabled and no errors occurred
//  */
// static void print_sensor_data(const pvPanel_t &pvPanel, const int *errors)
// {
//     if (errors[0] != 0)
//         return;  // Skip printing if errors occurred
        
//     if (!((theConf.debug_flags >> dMODBUS) & 1U))
//         return;  // Skip if debug not enabled

//     ESP_LOGI(TAG, "%sPanels [%s] - String1:[%.02fV / %.02fA] String2:[%.02fV / %.02fA]",LYELLOW,
//              pvPanel.chargeCurr ? "CHARGING" : "DISCHARGING",
//              pvPanel.pv1Volts,                    
//              pvPanel.pv1Amp,
//              pvPanel.pv2Volts,
//              pvPanel.pv2Amp);
// }

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Main solar panel monitoring task
 * 
 * Periodically reads solar panel parameters from the inverter via RS485/Modbus.
 * Monitors PV string voltages, currents, and charging state for both string 1
 * and string 2. Initializes sensor descriptors from configuration, sends read
 * requests to the RS485 handler task, and logs the received data.
 * 
 * @param pArg Task parameter (unused)
 * 
 * @note This is a FreeRTOS task that runs indefinitely
 * @note Task deletes itself if initialization fails
 * @note Reading interval is configured via theConf.modbus_panels.regfresh
 */
void panels_task(void *pArg)
{
    rs485queue_t mensaje;
    int errors[MAX_ERRORS] = {0};  // Initialize error array
    void *devicesarr;
    int sensor_count=0;

    ESP_LOGI(TAG, "%sPanel monitoring task started",LYELLOW);
    
    // Get panel configuration from global settings
        // Get battery configuration from global settings... its a 4 column spec
    const general_4modbus_specs_t *sensorinfo = (const general_4modbus_specs_t *)&theConf.modbus_panels;
    const int refreshrate = sensorinfo->regfresh;

    devicesarr = initialize_sensor_descriptors( sensorinfo,(char*)"Panels",PANELS_SENSORS,&sensor_count);
    if (devicesarr == NULL )
    {
        ESP_LOGW(TAG, "Panels task: No valid sensors configured, task exiting");
        vTaskDelete(NULL);
    }
    
    ESP_LOGI(TAG, "%sPanel task: Initialized %d sensors, refresh rate: %d min", LYELLOW , sensor_count, refreshrate);
    
    // Prepare message structure for RS485 communication
    mensaje.numCids = sensor_count;
    mensaje.descriptors = (mb_parameter_descriptor_t*)devicesarr;
    mensaje.dataReceiver = (void*)&pvPanelData;
    mensaje.requester = xTaskGetCurrentTaskHandle();
    mensaje.errCode = &errors[0];

    // Main monitoring loop
    while (true)
    {
        // Send read request to RS485 handler task
        if (xQueueSend(rs485Q, &mensaje, pdMS_TO_TICKS(100)) != pdPASS)
        {
            ESP_LOGE(TAG, "Panel task: Failed to send message to RS485 queue");
        }
        else
        {
            // Wait for RS485 task to complete the read operation
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            // Log the received data if debug enabled
            print_sensor_data(pvPanelData, errors);
        }

        // Wait before next reading cycle
        vTaskDelay(pdMS_TO_TICKS(refreshrate * 1000 * theConf.minutes));
    }
}