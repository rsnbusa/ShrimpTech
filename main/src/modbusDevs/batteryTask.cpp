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

// ============================================================================
// Constants
// ============================================================================

static constexpr int MAX_SENSORS = 4;          ///< Maximum number of battery sensors
static constexpr int MAX_ERRORS = MAX_SENSORS; ///< Error array size
static constexpr uint32_t BYTE_MASK = 0xFF;    ///< Byte mask for data parsing

// ============================================================================
// Type Definitions
// ============================================================================

/// @brief Container for battery sensor descriptors
typedef struct {
    mb_parameter_descriptor_t devices[MAX_SENSORS];
} descriptor_array_t;

// ============================================================================
// Private Functions
// ============================================================================

/**
 * @brief Initialize battery sensor descriptors from configuration
 * 
 * Parses the battery configuration and creates Modbus parameter descriptors
 * for each configured sensor. Allocates memory for labels and configures
 * register addresses, data types, and multipliers.
 * 
 * @param devicesarr Pointer to descriptor array structure
 * @param sensorinfo Pointer to battery Modbus specifications
 * @return int Number of successfully initialized sensors
 * 
 * @note Memory is allocated for param_key and param_units - caller must free
 * @note Skips sensors with invalid offset (< 0)
 * @note First sensor (temp) is FLOAT, others are U16
 */
static int initialize_sensor_descriptors(descriptor_array_t *devicesarr, const bat_modbus_specs_t *sensorinfo)
{
    int sensor_count = 0;

    for (int a = 0; a < MAX_SENSORS; a++)
    {
        // Skip sensors with invalid offset
        if (sensorinfo->specs[a].devices[2] < 0)
            continue;

        if ((theConf.debug_flags >> dMODBUS) & 1U)
        {
            ESP_LOGI(TAG, "%sBattery Descriptor %d/%d: Offset=%d Start=0x%04X Points=%d Mux=%.02f",GRAY,
                   a, sensor_count, 
                   sensorinfo->specs[a].devices[2],
                   sensorinfo->specs[a].devices[1],
                   sensorinfo->specs[a].devices[0],
                   sensorinfo->specs[a].mux);
        }

        devicesarr->devices[sensor_count].cid = sensor_count;
        
        // Allocate and set parameter key label
        char *label = (char*)calloc(1, 20);
        if (label == NULL)
        {
            ESP_LOGE(TAG, "Battery task: Failed to allocate param_key for sensor %d", sensor_count);
            continue;
        }
        sprintf(label, "Battery_%d", sensor_count);
        devicesarr->devices[sensor_count].param_key = label;
        
        // Allocate and set parameter units (stores multiplier)
        char *labelunits = (char*)calloc(1, 20);
        if (labelunits == NULL)
        {
            ESP_LOGE(TAG, "Battery task: Failed to allocate param_units for sensor %d", sensor_count);
            // free(label);
            continue;
        }
        sprintf(labelunits, "%d", sensorinfo->specs[a].mux);
        devicesarr->devices[sensor_count].param_units = labelunits;
        
        // Configure Modbus parameters
        devicesarr->devices[sensor_count].mb_slave_addr = sensorinfo->addr;
        devicesarr->devices[sensor_count].mb_param_type = MB_PARAM_HOLDING;
        devicesarr->devices[sensor_count].mb_reg_start = sensorinfo->specs[a].devices[1];
        devicesarr->devices[sensor_count].mb_size = sensorinfo->specs[a].devices[0];
        devicesarr->devices[sensor_count].param_offset = sensorinfo->specs[a].devices[2] + 1;
        
        // First sensor (battery temp) is FLOAT, all others are U16
        if (sensor_count == 0)
            devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT;
        else
            devicesarr->devices[sensor_count].param_type = PARAM_TYPE_U16;
            
        devicesarr->devices[sensor_count].param_size = (mb_descr_size_t)(sensorinfo->specs[a].devices[0] * 2);
        devicesarr->devices[sensor_count].access = PAR_PERMS_READ_WRITE;
        
        sensor_count++;
    }
    return sensor_count;
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
static void print_sensor_data(const battery_t &batteryData, const int *errors)
{
    if (errors[0] != 0)
        return;  // Skip printing if errors occurred
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;  // Skip if debug not enabled

    ESP_LOGI(TAG, "%sBattery - SOC:%d%% SOH:%d%% CycleCount:%d BmsTemp:%.02f°C",GRAY,
             batteryData.batSOC,
             batteryData.batSOH,
             batteryData.batteryCycleCount,
             batteryData.batBmsTemp);
}

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
    
    ESP_LOGI(TAG, "%sBattery monitoring task started",GRAY);
    
    // Allocate memory for sensor descriptors
    descriptor_array_t *devicesarr = (descriptor_array_t*)calloc(1, sizeof(descriptor_array_t));
    if (devicesarr == NULL)
    {
        ESP_LOGE(TAG, "Battery task: Failed to allocate memory for descriptors");
        vTaskDelete(NULL);
        return;
    }

    // Get battery configuration from global settings
    const bat_modbus_specs_t *sensorinfo = (const bat_modbus_specs_t *)&theConf.modbus_battery;
    const int refreshrate = sensorinfo->regfresh;
    // Initialize sensor descriptors from configuration
    const int sensor_count = initialize_sensor_descriptors(devicesarr, sensorinfo);
    if (sensor_count <= 0)
    {
        ESP_LOGW(TAG, "Battery task: No valid sensors configured, task exiting");
        free(devicesarr);
        vTaskDelete(NULL);
        return;
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