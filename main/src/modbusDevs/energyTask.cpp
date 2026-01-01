#define GLOBAL
#include "esp_log.h"
#include "defines.h"
#include "typedef.h"
#include "globals.h"

// Constants
static constexpr int MAX_SENSORS = 10;
static constexpr int MAX_ERRORS = MAX_SENSORS;
static constexpr uint32_t BYTE_MASK = 0xFF;

typedef struct {
    mb_parameter_descriptor_t devices[MAX_SENSORS];
} descriptor_array_t;

static int initialize_sensor_descriptors(descriptor_array_t *devicesarr, const inverter_modbus_specs_t *sensorinfo)
{
    int sensor_count = 0;

    for (int a = 0; a < MAX_SENSORS; a++)
    {
        // Skip sensors with invalid offset
        if (sensorinfo->specs[a].devices[2] < 0)        // from left to right in column in the web page configuration
            continue;

        if ((theConf.debug_flags >> dMODBUS) & 1U)
        {
            printf("Inverter Descriptors %d/%d: Offset %d Start %d Points %d Mux %0.02f\n",
                   a, sensor_count, 
                   sensorinfo->specs[a].devices[2],
                   sensorinfo->specs[a].devices[1],
                   sensorinfo->specs[a].devices[0],
                   sensorinfo->specs[a].mux);
        }

        devicesarr->devices[sensor_count].cid = sensor_count;
        
        char *label = (char*)calloc(1, 20);
        if (label == NULL)
        {
            ESP_LOGE(TAG, "energy_task: Memory allocation failed for param_key label.");
            continue;
        }
        sprintf(label, "Energy_%d", sensor_count);
        devicesarr->devices[sensor_count].param_key = label;
        
        char *labelunits = (char*)calloc(1, 20);
        if (labelunits == NULL)
        {
            ESP_LOGE(TAG, "energy_task: Memory allocation failed for param_units label.");
            free(label);
            continue;
        }
        sprintf(labelunits, "%d", sensorinfo->specs[a].mux);
        devicesarr->devices[sensor_count].param_units = labelunits;
        
        devicesarr->devices[sensor_count].mb_slave_addr = sensorinfo->addr;
        devicesarr->devices[sensor_count].mb_param_type = MB_PARAM_HOLDING;
        devicesarr->devices[sensor_count].mb_reg_start = sensorinfo->specs[a].devices[1];
        devicesarr->devices[sensor_count].mb_size = sensorinfo->specs[a].devices[0];
        devicesarr->devices[sensor_count].param_offset = sensorinfo->specs[a].devices[2] + 1;
        
        if (sensor_count==0)    
            devicesarr->devices[sensor_count].param_type = PARAM_TYPE_U32;
        else
            devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT;
            
        devicesarr->devices[sensor_count].param_size = (mb_descr_size_t)(sensorinfo->specs[a].devices[0] * 2);
        devicesarr->devices[sensor_count].access = PAR_PERMS_READ_WRITE;
        
        sensor_count++;
    }
    return sensor_count;
}

static void print_sensor_data(const energy_t &energy, const int *errors)
{
    if (errors[0] == 0 )
    {
        if((theConf.debug_flags >> dMODBUS) & 1U)
        {
            printf("Energy Charging Data: BatChgAH(Today:%u Total:%u)"
                   "GenEnergy:%.02fkWh "
                   "BatChg:%.02fkWh \n",
                     energy.batChgAHToday,
                     energy.batChgAHTotal,
                     energy.generateEnergyToday,
                     energy.batChgkWhToday);   
       
            printf("Energy Discharging Data:BatDischgAH(Today:%u Total:%u) "
                   "UsedEnergy:%.02fkWh LoadConsumTotal:%.02fkWh "
                   "BatDischg:%.02fkWh GenLoadConsum:%.02fkWh\n",
                     energy.batDischgAHToday,
                     energy.batDischgAHTotal,
                     energy.usedEnergyToday,
                     energy.gLoadConsumLineTotal,
                     energy.batDischgkWhToday,
                     energy.genLoadConsumToday);   
        }

    }
}

void energy_task(void *pArg)
{
    rs485queue_t mensaje;
    int errors[MAX_ERRORS];
    esp_rom_printf("Energy task started\n");
    // Prepare modbus descriptors for sensors
    descriptor_array_t *devicesarr = (descriptor_array_t*)calloc(1, sizeof(descriptor_array_t));
    if (devicesarr == NULL)
    {
        ESP_LOGE(TAG, "energy_task: Memory allocation failed for devices descriptors.");
        vTaskDelete(NULL);
    }
// get the descriptors as saved from the web configuration and in theConf structure... thsi type is specific for the inverter_modbus_specs_t
    const inverter_modbus_specs_t *sensorinfo = (const inverter_modbus_specs_t *)&theConf.modbus_inverter;
    const int refreshrate = sensorinfo->regfresh;

    const int sensor_count = initialize_sensor_descriptors(devicesarr, sensorinfo);
    if (sensor_count <= 0)
    {
        ESP_LOGW(TAG, "Energy_task: No valid sensors configured. Task exiting.");
        vTaskDelete(NULL);
    }
    // Initialize message structure
    mensaje.numCids = sensor_count;
    mensaje.descriptors = (mb_parameter_descriptor_t*)devicesarr;
    mensaje.dataReceiver = (void*)&energyData;
    mensaje.requester = xTaskGetCurrentTaskHandle();  // so we can notify when done
    memset(errors, 0, sizeof(errors));
    mensaje.errCode = &errors[0];

    while (true)
    {
        // Send message to rs485 task
        if (xQueueSend(rs485Q, &mensaje, pdMS_TO_TICKS(100)) != pdPASS)
        {
            ESP_LOGE(TAG, "Energy_task: Could not send message to rs485 task.");
        }
        else
        {
            // Wait until rs485 task notifies us that it is done
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            print_sensor_data(energyData, errors);
        }

        // Wait before next reading
        vTaskDelay(pdMS_TO_TICKS(refreshrate * 1000*MINUTES));
    }
}