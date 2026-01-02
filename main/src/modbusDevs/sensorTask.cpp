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

// ============================================================================
// Constants
// ============================================================================

static constexpr int MAX_SENSORS = 5;          ///< Maximum number of environmental sensors
static constexpr int MAX_ERRORS = MAX_SENSORS; ///< Error array size
static constexpr uint32_t BYTE_MASK = 0xFF;    ///< Byte mask for data parsing

// ============================================================================
// Type Definitions
// ============================================================================

/// @brief Container for sensor descriptors
// typedef struct {
//     mb_parameter_descriptor_t devices[MAX_SENSORS];
// } descriptor_array_t;

// ============================================================================
// Private Functions
// ============================================================================

/**
 * @brief Initialize environmental sensor descriptors from configuration
 * 
 * Parses the sensor configuration and creates Modbus parameter descriptors
 * for each configured environmental sensor. Allocates memory for labels and
 * configures register addresses, data types, and multipliers. Supports multiple
 * sensor addresses for distributed monitoring.
 * 
 * @param devicesarr Pointer to descriptor array structure
 * @param sensorinfo Pointer to sensor Modbus specifications
 * @return int Number of successfully initialized sensors
 * 
 * @note Memory is allocated for param_key and param_units - caller must free
 * @note Skips sensors with invalid offset (< 0)
 * @note Uses PARAM_TYPE_FLOAT_BADC for sensor readings
 */
static int initialize_sensor_descriptors(descriptor_array_t *devicesarr, const sensors_modbus_specs_t *sensorinfo)
{
    int sensor_count = 0;

    for (int a = 0; a < MAX_SENSORS; a++)
    {
        // Skip sensors with invalid offset
        if (sensorinfo->specs[a].devices[DOFFSET] < 0)
            continue;

        if ((theConf.debug_flags >> dMODBUS) & 1U)
        {
            ESP_LOGI(TAG, "%sSensor Descriptor %d/%d: \tAddr=%d \tOffset=%d \tStart=%d \tPoints=%d \tType %d \tMux=%.02f",BLUE,
                   a, sensor_count, 
                   sensorinfo->specs[a].devices[DADDR],
                   sensorinfo->specs[a].devices[DOFFSET],
                   sensorinfo->specs[a].devices[DSTART],
                   sensorinfo->specs[a].devices[DPOINTS],
                   sensorinfo->specs[a].devices[DTYPE],
                   sensorinfo->specs[a].mux);
        }

        devicesarr->devices[sensor_count].cid = sensor_count;
        
        // Allocate and set parameter key label
        char *label = (char*)calloc(1, 20);
        if (label == NULL)
        {
            ESP_LOGE(TAG, "Sensor task: Failed to allocate param_key for sensor %d", sensor_count);
            continue;
        }
        sprintf(label, "Sensor_%d", sensor_count);
        devicesarr->devices[sensor_count].param_key = label;
        
        // Allocate and set parameter units (stores multiplier)
        char *labelunits = (char*)calloc(1, 20);
        if (labelunits == NULL)
        {
            ESP_LOGE(TAG, "Sensor task: Failed to allocate param_units for sensor %d", sensor_count);
            // free(label);
            continue;
        }
        sprintf(labelunits, "%d", sensorinfo->specs[a].mux);
        devicesarr->devices[sensor_count].param_units = labelunits;
        
        // Configure Modbus parameters (note: sensors have individual slave addresses)
        devicesarr->devices[sensor_count].mb_slave_addr = sensorinfo->specs[a].devices[DADDR];
        devicesarr->devices[sensor_count].mb_param_type = MB_PARAM_HOLDING;
        devicesarr->devices[sensor_count].mb_reg_start = sensorinfo->specs[a].devices[DSTART];
        devicesarr->devices[sensor_count].mb_size = sensorinfo->specs[a].devices[DPOINTS];
        devicesarr->devices[sensor_count].param_offset = sensorinfo->specs[a].devices[DOFFSET] + 1;
        
        switch(sensorinfo->specs[a].devices[DTYPE])
        {
            case 0:
                devicesarr->devices[sensor_count].param_type = PARAM_TYPE_U8;
                break;
            case 1:
                devicesarr->devices[sensor_count].param_type = PARAM_TYPE_U16;
                break;
            case 2:
                devicesarr->devices[sensor_count].param_type = PARAM_TYPE_U32;
                break;
            case 3:
                devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT;
                break;
            case 4:
                devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_ABCD;
                break;
            case 5:
                devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_CDAB;
                break;
            case 6:
                devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_BADC;
                break;
            case 7:
                devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_DCBA;
                break;
            default:
                devicesarr->devices[sensor_count].param_type = PARAM_TYPE_U16;
                break;
        }

        // Environmental sensors use FLOAT_BADC type for byte-swapped floating point
        devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_BADC;
            
        devicesarr->devices[sensor_count].param_size = (mb_descr_size_t)(sensorinfo->specs[a].devices[DPOINTS] * 2);
        devicesarr->devices[sensor_count].access = PAR_PERMS_READ_WRITE;
        
        sensor_count++;
    }
    
    return sensor_count;
}

/**
 * @brief Print environmental sensor data for debugging
 * 
 * Logs environmental monitoring data including water temperature, dissolved
 * oxygen concentration and percentage.
 * 
 * @param sensorData Reference to sensor data structure
 * @param errors Pointer to error code array
 * 
 * @note Checks errors[1] instead of errors[0] for multi-sensor setups
 * @note Only prints if MODBUS debug flag is enabled and no errors occurred
 */
static void print_sensor_data(const sensor_t &sensorData, const int *errors)
{
    if (errors[1] != 0)
        return;  // Skip printing if errors occurred
        
    if (!((theConf.debug_flags >> dMODBUS) & 1U))
        return;  // Skip if debug not enabled

    ESP_LOGI(TAG, "%sSensors - WaterTemp:%.02f°C DO%%.%.02f%% DO:%.02fppm",BLUE,
             sensorData.WTemp,
             sensorData.percentDO * 100.0,
             sensorData.DO);
}

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Main environmental sensor monitoring task
 * 
 * Periodically reads environmental sensor data via RS485/Modbus.
 * Monitors water temperature, dissolved oxygen (concentration and percentage),
 * pH, air temperature, and air humidity from configured sensors.
 * Initializes sensor descriptors from configuration, sends read requests to
 * the RS485 handler task, and logs the received data.
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
    rs485queue_t mensaje;
    int errors[MAX_ERRORS] = {0};  // Initialize error array

    ESP_LOGI(TAG, "%sEnvironmental sensor monitoring task started",BLUE);
    
    // Allocate memory for sensor descriptors
    descriptor_array_t *devicesarr = (descriptor_array_t*)calloc(1, sizeof(descriptor_array_t));
    if (devicesarr == NULL)
    {
        ESP_LOGE(TAG, "Sensor task: Failed to allocate memory for descriptors");
        vTaskDelete(NULL);
        return;
    }

    // Get sensor configuration from global settings
    const sensors_modbus_specs_t *sensorinfo = (const sensors_modbus_specs_t *)&theConf.modbus_sensors;
    const int refreshrate = sensorinfo->regfresh;
    // Initialize sensor descriptors from configuration
    const int sensor_count = initialize_sensor_descriptors(devicesarr, sensorinfo);
    if (sensor_count <= 0)
    {
        ESP_LOGW(TAG, "Sensor task: No valid sensors configured, task exiting");
        free(devicesarr);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "%sSensor task: Initialized %d sensors, refresh rate: %d min", BLUE, sensor_count, refreshrate);

    // Prepare message structure for RS485 communication
    mensaje.numCids = sensor_count;
    mensaje.descriptors = (mb_parameter_descriptor_t*)devicesarr;
    mensaje.dataReceiver = (void*)&sensorData;
    mensaje.requester = xTaskGetCurrentTaskHandle();
    mensaje.errCode = &errors[0];

    // Main monitoring loop
    while (true)
    {
        // Send read request to RS485 handler task
        if (xQueueSend(rs485Q, &mensaje, pdMS_TO_TICKS(100)) != pdPASS)
        {
            ESP_LOGE(TAG, "Sensor task: Failed to send message to RS485 queue");
        }
        else
        {
            // Wait for RS485 task to complete the read operation
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            
            // Log the received data if debug enabled
            print_sensor_data(sensorData, errors);
        }

        // Wait before next reading cycle
        vTaskDelay(pdMS_TO_TICKS(refreshrate * 1000 * theConf.minutes));
    }
}