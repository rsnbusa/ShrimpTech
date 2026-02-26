/**
 * @file desc4build.cpp
 * @brief Modbus descriptor initialization for generic sensor configurations
 * 
 * Provides legacy descriptor initialization supporting both 4-column (shared address)
 * and 5-column (per-device address) sensor specifications.
 * 
 * @note Part of the ShrimpIoT Modbus device monitoring system
 */

#define GLOBAL
#include "includes.h"
#include "defines.h"
#include "typedef.h"
#include "globals.h"

// Modbus parameter type names for debugging
static char TYPES_NAME[][30] = {
    "PARAM_TYPE_U8", 
    "PARAM_TYPE_U16", 
    "PARAM_TYPE_U32", 
    "PARAM_TYPE_FLOAT",
    "PARAM_TYPE_FLOAT_ABCD", 
    "PARAM_TYPE_FLOAT_CDAB",
    "PARAM_TYPE_FLOAT_BADC", 
    "PARAM_TYPE_FLOAT_DCBA"
};

/**
 * @brief Initialize Modbus sensor descriptors from configuration
 * 
 * Builds descriptor array for Modbus communication based on sensor specifications.
 * Supports both 4-column specs (shared address) and 5-column specs (individual addresses).
 * 
 * @param sensorinfoin Pointer to sensor specification structure (4 or 5 column)
 * @param whichDev Device name prefix for parameter labels
 * @param MAXSENSORS Maximum number of sensors to process
 * @param columns Number of columns in spec (4 or 5)
 * @param count Output parameter for number of valid descriptors created
 * @return descriptor_array_t* Pointer to allocated descriptor array, or NULL on failure
 * 
 * @note Caller is responsible for freeing allocated memory
 * @note Skips sensors with offset < 0
 */
descriptor_array_t* initialize_sensor_descriptors(
    void *sensorinfoin,
    char *whichDev,
    int MAXSENSORS,
    int columns,
    int *count)
{
    // Cast input to appropriate structure types
    general_4modbus_specs_t *sensorinfo4 = (general_4modbus_specs_t*)sensorinfoin;
    general_5modbus_specs_t *sensorinfo5 = (general_5modbus_specs_t*)sensorinfoin;

    int sensor_count = 0;

    // Allocate memory for sensor descriptors
    descriptor_array_t *descriptors = (descriptor_array_t*)calloc(1, sizeof(mb_parameter_descriptor_t) * MAXSENSORS);
    if (descriptors == NULL)
    {
        MESP_LOGE(TAG, "%s task: Failed to allocate memory for descriptors", whichDev);
        return NULL;
    }

    for (int a = 0; a < MAXSENSORS; a++)
    {
        // Skip sensors with invalid offset
        int offset = (columns == 4) ? 
            sensorinfo4->specs[a].devices[DOFFSET] : 
            sensorinfo5->specs[a].devices[DOFFSET];
            
        if (offset < 0)
        {
            continue;       //skip
        }

        // Extract sensor parameters based on column count
        int start = (columns == 4) ? 
            sensorinfo4->specs[a].devices[DSTART] : 
            sensorinfo5->specs[a].devices[DSTART];
            
        int points = (columns == 4) ? 
            sensorinfo4->specs[a].devices[DPOINTS] : 
            sensorinfo5->specs[a].devices[DPOINTS];
            
        int type = (columns == 4) ? 
            sensorinfo4->specs[a].devices[DTYPE] : 
            sensorinfo5->specs[a].devices[DTYPE];
            
        double mux = (columns == 4) ? 
            sensorinfo4->specs[a].mux : 
            sensorinfo5->specs[a].mux;
            
        int address = (columns == 4) ? 
            sensorinfo4->addr : 
            sensorinfo5->specs[a].devices[DADDR];

        // Debug logging if enabled
        if ((theConf.debug_flags >> dMODBUS) & 1U)
        {
            if (columns < 5)
            {
                MESP_LOGI(TAG, "%s%s Descriptor %d/%d: \tOffset=%d \tStart=%d \tPoints=%d \tType=%d[%s] \tMux=%.02f",
                    GRAY, whichDev, a, sensor_count, 
                    offset, start, points, type, 
                    TYPES_NAME[type], mux);
            }
            else
            {
                MESP_LOGI(TAG, "%s%s Descriptor %d/%d: \tAddress=%d \tOffset=%d \tStart=%d \tPoints=%d \tType=%d[%s] \tMux=%.02f",
                    GRAY, whichDev, a, sensor_count, 
                    address, offset, start, points, type, 
                    TYPES_NAME[type], mux);
            }
        }

        descriptors->devices[sensor_count].cid = sensor_count;
        // Allocate and set parameter key label
        char *label = (char*)calloc(1, 20);
        if (label == NULL)
        {
            MESP_LOGE(TAG, "%s task: Failed to allocate param_key for sensor %d", whichDev, sensor_count);
            continue;
        }
        sprintf(label, "%s%d", whichDev, sensor_count);
        descriptors->devices[sensor_count].param_key = label;
        
        // Allocate and set parameter units (stores multiplier)
        char *label_units = (char*)calloc(1, 20);
        if (label_units == NULL)
        {
            MESP_LOGE(TAG, "%s task: Failed to allocate param_units for sensor %d", whichDev, sensor_count);
            continue;
        }
        sprintf(label_units, "%d", (int)mux);
        descriptors->devices[sensor_count].param_units = label_units;
        
        // Configure Modbus parameters
        descriptors->devices[sensor_count].mb_slave_addr = address;
        descriptors->devices[sensor_count].mb_param_type = MB_PARAM_HOLDING;        // fixed Function Code
        descriptors->devices[sensor_count].mb_reg_start = start;
        descriptors->devices[sensor_count].mb_size = points;
        descriptors->devices[sensor_count].param_offset = offset + 1;
        
        // Map data type from configuration to Modbus parameter type
        switch (type)
        {
            case 0:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_U8;
                break;
            case 1:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_U16;
                break;
            case 2:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_U32;
                break;
            case 3:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT;
                break;
            case 4:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_ABCD;
                break;
            case 5:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_CDAB;
                break;
            case 6:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_BADC;
                break;
            case 7:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_DCBA;
                break;
            default:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_U16;
                break;
        }
            
        descriptors->devices[sensor_count].param_size = (mb_descr_size_t)(points * sizeof(uint16_t));
        descriptors->devices[sensor_count].access = mux>0.1 ? PAR_PERMS_READ: PAR_PERMS_WRITE;  // if mux is >0 then its writable since we can write the value to be multiplied by mux and get the real value in the sensor. if mux is 0 then its read only since we cant write to it.
        
        sensor_count++;
    }
    
    *count = sensor_count;
    return descriptors;
}
