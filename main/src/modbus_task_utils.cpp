/**
 * @file modbus_task_utils.cpp
 * @brief Implementation of shared Modbus monitoring task utilities
 * 
 * Provides generic descriptor initialization and helper functions for all
 * Modbus device monitoring tasks.
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
// Private Constants
// ============================================================================

/// @brief Human-readable names for Modbus parameter types
static const char* TYPES_NAME[] = {
    "PARAM_TYPE_U8",
    "PARAM_TYPE_U16",
    "PARAM_TYPE_U32",
    "PARAM_TYPE_FLOAT",
    "PARAM_TYPE_FLOAT_ABCD",
    "PARAM_TYPE_FLOAT_CDAB",
    "PARAM_TYPE_FLOAT_BADC",
    "PARAM_TYPE_FLOAT_DCBA"
};

// ============================================================================
// Public Functions
// ============================================================================

extern "C" {

const char* modbus_get_type_name(int type_index)
{
    if (type_index < 0 || type_index > 7)
        return "UNKNOWN";
    return TYPES_NAME[type_index];
}

int modbus_init_descriptors(mb_parameter_descriptor_t *descriptors,
                            const modbus_sensor_config_t *config,
                            const char *task_name,
                            const char *log_color)
{
    if (descriptors == NULL || config == NULL || task_name == NULL)
    {
        ESP_LOGE(TAG, "modbus_init_descriptors: Invalid parameters");
        return -1;
    }

    int sensor_count = 0;
    const uint8_t *base_ptr = (const uint8_t*)config->specs;
    const int max_sensors = config->max_sensors;
    const int spec_size = config->spec_size_bytes;

    for (int a = 0; a < max_sensors; a++)
    {
        // Calculate pointer to current spec using byte offset
        const modbus_device_spec_t *spec = (const modbus_device_spec_t*)(base_ptr + (a * spec_size));
        
        // Skip sensors with invalid offset
        if (spec->devices[DOFFSET] < 0)
            continue;

        // Debug logging if enabled
        if ((theConf.debug_flags >> dMODBUS) & 1U)
        {
            if (config->use_per_device_addr)
            {
                ESP_LOGI(TAG, "%s%s Descriptor %d/%d: \tAddr=%d \tOffset=%d \tStart=%d \tPoints=%d \tType=%d[%s] \tMux=%.02f",
                       log_color, task_name, a, sensor_count,
                       spec->devices[DADDR],
                       spec->devices[DOFFSET],
                       spec->devices[DSTART],
                       spec->devices[DPOINTS],
                       spec->devices[DTYPE],
                       modbus_get_type_name(spec->devices[DTYPE]),
                       spec->mux);
            }
            else
            {
                ESP_LOGI(TAG, "%s%s Descriptor %d/%d: \tOffset=%d \tStart=%d \tPoints=%d \tType=%d[%s] \tMux=%.02f",
                       log_color, task_name, a, sensor_count,
                       spec->devices[DOFFSET],
                       spec->devices[DSTART],
                       spec->devices[DPOINTS],
                       spec->devices[DTYPE],
                       modbus_get_type_name(spec->devices[DTYPE]),
                       spec->mux);
            }
        }

        descriptors[sensor_count].cid = sensor_count;
        
        // Allocate and set parameter key label
        char *label = (char*)calloc(1, 20);
        if (label == NULL)
        {
            ESP_LOGE(TAG, "%s task: Failed to allocate param_key for sensor %d", task_name, sensor_count);
            continue;
        }
        sprintf(label, "%s_%d", task_name, sensor_count);
        descriptors[sensor_count].param_key = label;
        
        // Allocate and set parameter units (stores multiplier)
        char *labelunits = (char*)calloc(1, 20);
        if (labelunits == NULL)
        {
            ESP_LOGE(TAG, "%s task: Failed to allocate param_units for sensor %d", task_name, sensor_count);
            continue;
        }
        sprintf(labelunits, "%.0f", spec->mux);
        descriptors[sensor_count].param_units = labelunits;
        
        // Configure Modbus parameters
        // Use per-device address if configured (sensors), otherwise use global address
        if (config->use_per_device_addr)
            descriptors[sensor_count].mb_slave_addr = spec->devices[DADDR];
        else
            descriptors[sensor_count].mb_slave_addr = config->addr;
            
        descriptors[sensor_count].mb_param_type = MB_PARAM_HOLDING;
        descriptors[sensor_count].mb_reg_start = spec->devices[DSTART];
        descriptors[sensor_count].mb_size = spec->devices[DPOINTS];
        descriptors[sensor_count].param_offset = spec->devices[DOFFSET] + 1;
        
        // Map data type from configuration to Modbus parameter type
        switch(spec->devices[DTYPE])
        {
            case 0:
                descriptors[sensor_count].param_type = PARAM_TYPE_U8;
                break;
            case 1:
                descriptors[sensor_count].param_type = PARAM_TYPE_U16;
                break;
            case 2:
                descriptors[sensor_count].param_type = PARAM_TYPE_U32;
                break;
            case 3:
                descriptors[sensor_count].param_type = PARAM_TYPE_FLOAT;
                break;
            case 4:
                descriptors[sensor_count].param_type = PARAM_TYPE_FLOAT_ABCD;
                break;
            case 5:
                descriptors[sensor_count].param_type = PARAM_TYPE_FLOAT_CDAB;
                break;
            case 6:
                descriptors[sensor_count].param_type = PARAM_TYPE_FLOAT_BADC;
                break;
            case 7:
                descriptors[sensor_count].param_type = PARAM_TYPE_FLOAT_DCBA;
                break;
            default:
                descriptors[sensor_count].param_type = PARAM_TYPE_U16;
                break;
        }
        
        descriptors[sensor_count].param_size = (mb_descr_size_t)(spec->devices[DPOINTS] * 2);
        descriptors[sensor_count].access = PAR_PERMS_READ_WRITE;
        
        sensor_count++;
    }
    
    return sensor_count;
}

void modbus_free_descriptors(mb_parameter_descriptor_t *descriptors, int count)
{
    if (descriptors == NULL)
        return;

    for (int i = 0; i < count; i++)
    {
        if (descriptors[i].param_key != NULL)
        {
            free((void*)descriptors[i].param_key);
            descriptors[i].param_key = NULL;
        }
        if (descriptors[i].param_units != NULL)
        {
            free((void*)descriptors[i].param_units);
            descriptors[i].param_units = NULL;
        }
    }
}

void modbus_task_runner(const modbus_sensor_config_t *config,
                       void *data_receiver,
                       print_sensor_data_fn print_callback,
                       int max_sensors,
                       const char *task_name,
                       const char *log_color)
{
    rs485queue_t mensaje;
    int *errors = NULL;
    mb_parameter_descriptor_t *descriptors = NULL;
    
    ESP_LOGI(TAG, "%s%s monitoring task started", log_color, task_name);
    
    // Allocate error array
    errors = (int*)calloc(max_sensors, sizeof(int));
    if (errors == NULL)
    {
        ESP_LOGE(TAG, "%s task: Failed to allocate error array", task_name);
        vTaskDelete(NULL);
        return;
    }
    
    // Allocate memory for sensor descriptors
    descriptors = (mb_parameter_descriptor_t*)calloc(max_sensors, sizeof(mb_parameter_descriptor_t));
    if (descriptors == NULL)
    {
        ESP_LOGE(TAG, "%s task: Failed to allocate memory for descriptors", task_name);
        free(errors);
        vTaskDelete(NULL);
        return;
    }

    // Initialize sensor descriptors from configuration
    const int sensor_count = modbus_init_descriptors(descriptors, config, task_name, log_color);
    if (sensor_count <= 0)
    {
        ESP_LOGW(TAG, "%s task: No valid sensors configured, task exiting", task_name);
        free(descriptors);
        free(errors);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "%s%s task: Initialized %d sensors, refresh rate: %d min", 
             log_color, task_name, sensor_count, config->regfresh);
    
    // Prepare message structure for RS485 communication
    mensaje.numCids = sensor_count;
    mensaje.descriptors = descriptors;
    mensaje.dataReceiver = data_receiver;
    mensaje.requester = xTaskGetCurrentTaskHandle();
    mensaje.errCode = errors;

    // Main monitoring loop
    while (true)
    {
        // Send read request to RS485 handler task
        if (xQueueSend(rs485Q, &mensaje, pdMS_TO_TICKS(100)) != pdPASS)
        {
            ESP_LOGE(TAG, "%s task: Failed to send message to RS485 queue", task_name);
        }
        else
        {
            // Wait for RS485 task to complete the read operation
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            
            // Call the print callback if provided
            if (print_callback != NULL)
            {
                print_callback(data_receiver, errors);
            }
        }

        // Wait before next reading cycle
        vTaskDelay(pdMS_TO_TICKS(config->regfresh * 1000 * theConf.minutes));
    }
}

} // extern "C"
