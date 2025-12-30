#define GLOBAL
#include "esp_log.h"
// #include "misparams.h"  // for modbus parameters structures
// #include "mbcontroller.h"
// #include "sdkconfig.h"
#include "defines.h"
// #include "BlowerClass.h"
#include "typedef.h"
#include "globals.h"

// Constants
static constexpr int MAX_SENSORS = 5;
static constexpr int MAX_ERRORS = 20;
static constexpr int BATTERY_SENSOR_ADDRESS = 100;
static constexpr uint32_t BYTE_MASK = 0xFF;

typedef struct {
    mb_parameter_descriptor_t devices[MAX_SENSORS];
} descriptor_array_t;

static int initialize_sensor_descriptors(descriptor_array_t *devicesarr, const sensors_modbus_specs_t *sensorinfo)
{
    int sensor_count = 0;

    for (int a = 0; a < MAX_SENSORS; a++)
    {
        // Skip sensors with invalid offset
        if (sensorinfo->specs[a].devices[2] < 0)
            continue;

        if ((theConf.debug_flags >> dMODBUS) & 1U)
        {
            printf("Sensors specs %d/%d: Address %d Offset %d Start %04X Points %d Mux %0.02f\n",
                   a, sensor_count, 
                   sensorinfo->specs[a].devices[3],
                   sensorinfo->specs[a].devices[2],
                   sensorinfo->specs[a].devices[1],
                   sensorinfo->specs[a].devices[0],
                   sensorinfo->specs[a].mux);
        }

        devicesarr->devices[sensor_count].cid = sensor_count;
        
        char *label = (char*)calloc(1, 20);
        if (label == NULL)
        {
            ESP_LOGE(TAG, "sensor_task: Memory allocation failed for param_key label.");
            continue;
        }
        sprintf(label, "Sensor_%d", sensor_count);
        devicesarr->devices[sensor_count].param_key = label;
        
        char *labelunits = (char*)calloc(1, 20);
        if (labelunits == NULL)
        {
            ESP_LOGE(TAG, "sensor_task: Memory allocation failed for param_units label.");
            free(label);
            continue;
        }
        sprintf(labelunits, "%d", sensorinfo->specs[a].mux);
        devicesarr->devices[sensor_count].param_units = labelunits;
        
        devicesarr->devices[sensor_count].mb_slave_addr = sensorinfo->specs[a].devices[3];
        devicesarr->devices[sensor_count].mb_param_type = MB_PARAM_HOLDING;
        devicesarr->devices[sensor_count].mb_reg_start = sensorinfo->specs[a].devices[1];
        devicesarr->devices[sensor_count].mb_size = sensorinfo->specs[a].devices[0];
        devicesarr->devices[sensor_count].param_offset = sensorinfo->specs[a].devices[2] + 1;
        
        // TODO this is for testing. we have to have clarity of the sensors (as we have of DO) of other sensors variable types
        if (sensorinfo->specs[a].devices[3] != BATTERY_SENSOR_ADDRESS)
            devicesarr->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_BADC;
        else
            devicesarr->devices[sensor_count].param_type = PARAM_TYPE_U16;
            
        devicesarr->devices[sensor_count].param_size = (mb_descr_size_t)(sensorinfo->specs[a].devices[0] * 2);
        devicesarr->devices[sensor_count].access = PAR_PERMS_READ_WRITE;
        
        sensor_count++;
    }
    
    return sensor_count;
}

static void print_sensor_data(const sensor_t &sensorData, const int *errors)
{
    if (errors[1] == 0 )
    {
        if((theConf.debug_flags >> dMODBUS) & 1U)
            printf("Sensor data read: Temp %.02f˚C Percent %.02f%% DO %.02f ppm ",
                sensorData.WTemp,
                sensorData.percentDO * 100.0,
                sensorData.DO);
    }
    
    if (errors[0] == 0 )
    {
        uint32_t *battery_data = (uint32_t*)&sensorData.PH;
        if ((theConf.debug_flags >> dMODBUS) & 1U)
            printf("SOC %d%% SOH %d%%",
                *battery_data & BYTE_MASK,
                (*battery_data >> 8) & BYTE_MASK);
    
            batteryData.batSOC=*battery_data & BYTE_MASK;
            batteryData.batSOH=(*battery_data >> 8) & BYTE_MASK;
        }
    
    printf("\n");
}

void sensor_task(void *pArg)
{
    rs485queue_t mensaje;
    int errors[MAX_ERRORS];
    // Prepare modbus descriptors for sensors
    descriptor_array_t *devicesarr = (descriptor_array_t*)calloc(1, sizeof(descriptor_array_t));
    if (devicesarr == NULL)
    {
        ESP_LOGE(TAG, "sensor_task: Memory allocation failed for devices descriptors.");
        vTaskDelete(NULL);
    }

    const sensors_modbus_specs_t *sensorinfo = (const sensors_modbus_specs_t *)&theConf.modbus_sensors;
    const int refreshrate = sensorinfo->regfresh;
    
    const int sensor_count = initialize_sensor_descriptors(devicesarr, sensorinfo);
    if (sensor_count <= 0)
    {
        ESP_LOGW(TAG, "sensor_task: No valid sensors configured. Task exiting.");
        vTaskDelete(NULL);
    }

    // Initialize message structure
    mensaje.numCids = sensor_count;
    mensaje.descriptors = (mb_parameter_descriptor_t*)devicesarr;
    mensaje.dataReceiver = (void*)&sensorData;
    mensaje.requester = xTaskGetCurrentTaskHandle();  // so we can notify when done
    memset(errors, 0, sizeof(errors));
    mensaje.errCode = &errors[0];

    while (true)
    {
        // Send message to rs485 task
        if (xQueueSend(rs485Q, &mensaje, pdMS_TO_TICKS(100)) != pdPASS)
        {
            ESP_LOGE(TAG, "sensor_task: Could not send message to rs485 task.");
        }
        else
        {
            // Wait until rs485 task notifies us that it is done
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            print_sensor_data(sensorData, errors);
        }

        // Wait before next reading
        vTaskDelay(pdMS_TO_TICKS(refreshrate * 1000));
    }
}