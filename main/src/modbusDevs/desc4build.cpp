#define GLOBAL
#include "includes.h"
#include "defines.h"
#include "typedef.h"
#include "globals.h"

static char TYPES_NAME[][30] = {"PARAM_TYPE_U8", "PARAM_TYPE_U16", "PARAM_TYPE_U32", "PARAM_TYPE_FLOAT",
                          "PARAM_TYPE_FLOAT_ABCD", "PARAM_TYPE_FLOAT_CDAB",
                          "PARAM_TYPE_FLOAT_BADC", "PARAM_TYPE_FLOAT_DCBA"};

 descriptor_array_t * initialize_sensor_descriptors(const general_4modbus_specs_t *sensorinfo,
                char *whichDev,int MAXSENSORS,int *count)
{
    int sensor_count = 0;

        // Allocate memory for sensor descriptors
    descriptor_array_t *devicesarr = (descriptor_array_t *)calloc(1, sizeof(mb_parameter_descriptor_t) * MAXSENSORS);
    if (devicesarr == NULL)
    {
        ESP_LOGE(TAG, "Battery task: Failed to allocate memory for descriptors");
        return NULL;
    }

    for (int a = 0; a < MAXSENSORS; a++)
    {
        // Skip sensors with invalid offset
        if (sensorinfo->specs[a].devices[DOFFSET] < 0)
            continue;

        if ((theConf.debug_flags >> dMODBUS) & 1U)
        {
            ESP_LOGI(TAG, "%s%s Descriptor %d/%d: \tOffset=%d \tStart=%d \tPoints=%d \tType=%d[%s] \tMux=%.02f",GRAY,whichDev,
                   a, sensor_count, 
                   sensorinfo->specs[a].devices[DOFFSET],
                   sensorinfo->specs[a].devices[DSTART],
                   sensorinfo->specs[a].devices[DPOINTS],
                   sensorinfo->specs[a].devices[DTYPE], 
                   TYPES_NAME[sensorinfo->specs[a].devices[DTYPE]],
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
        devicesarr->devices[sensor_count].mb_reg_start = sensorinfo->specs[a].devices[DSTART];
        devicesarr->devices[sensor_count].mb_size = sensorinfo->specs[a].devices[DPOINTS];
        devicesarr->devices[sensor_count].param_offset = sensorinfo->specs[a].devices[DOFFSET] + 1;
        
        // Type (entry 0 defines var type 0=uint8_t 1=uint16_t 2=uint32_t 3=float)
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

        // if (sensor_count == 0)
        //     devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT;
        // else
        //     devicesarr->devices[sensor_count].param_type = PARAM_TYPE_U16;
            
        devicesarr->devices[sensor_count].param_size = (mb_descr_size_t)(sensorinfo->specs[a].devices[DPOINTS] * 2);
        devicesarr->devices[sensor_count].access = PAR_PERMS_READ_WRITE;
        
        sensor_count++;
    }
    *count=sensor_count;
    return devicesarr;
}
