#define GLOBAL
#include "esp_log.h"
// #include "misparams.h"  // for modbus parameters structures
// #include "mbcontroller.h"
// #include "sdkconfig.h"
#include "defines.h"
// #include "BlowerClass.h"
#include "typedef.h"
#include "globals.h"

void sensor_task(void *pArg)
{
    esp_err_t err;
    rs485queue_t mensaje;
    int refreshrate=0;
    int errors[20];
    typedef struct {mb_parameter_descriptor_t devices[5];} descriptor_array_t;
    descriptor_array_t *devicesarr;
//get configured sensors data descriptors variables
// prepare modbus descriptors for sensors

    devicesarr=(descriptor_array_t*)calloc(1,sizeof(descriptor_array_t));

    sensors_modbus_specs_t *sensorinfo=(sensors_modbus_specs_t *)&theConf.modbus_sensors;    // make it array type for easy management
    refreshrate=sensorinfo->regfresh;
    int son=0;      //since we can skip certain sensors via the Offset being -1 we need a separate counter

    for (int a=0;a<5;a++)
    {
        // printf("Vamos %d vale %d\n",a,sensorinfo->specs[a].devices[3]);
        if(sensorinfo->specs[a].devices[2]>=0)
        {
            if((theConf.debug_flags >> dMODBUS) & 1U)
                    printf("Sensors specs %d/%d: Address %d Offset %d Start %04X Points %d Mux %0.02f\n",a,son,sensorinfo->specs[a].devices[3],sensorinfo->specs[a].devices[2],sensorinfo->specs[a].devices[1],sensorinfo->specs[a].devices[0],sensorinfo->specs[a].mux); 
        
            devicesarr->devices[son].cid=son;
            char *label=(char*)calloc(1,20);
            sprintf(label,"Sensor_%d",son);
            devicesarr->devices[son].param_key=label;
            char *labelunits=(char*)calloc(1,20);
            sprintf(labelunits,"%d",sensorinfo->specs[a].mux);
            devicesarr->devices[son].param_units=labelunits;
            devicesarr->devices[son].mb_slave_addr=sensorinfo->specs[a].devices[3];
            devicesarr->devices[son].mb_param_type=MB_PARAM_HOLDING;
            devicesarr->devices[son].mb_reg_start=sensorinfo->specs[a].devices[1];
            devicesarr->devices[son].mb_size=sensorinfo->specs[a].devices[0];
            devicesarr->devices[son].param_offset=sensorinfo->specs[a].devices[2]+1;
            // TODO this is for testing. we have to have clartiy of the sensors (as we have of DO) of other sensors variable types
            if(sensorinfo->specs[a].devices[3]!=100)
                devicesarr->devices[son].param_type=PARAM_TYPE_FLOAT_BADC;
            else
                devicesarr->devices[son].param_type=PARAM_TYPE_U16;
            devicesarr->devices[son].param_size=(mb_descr_size_t)(sensorinfo->specs[a].devices[0]*2);
            devicesarr->devices[son].access=PAR_PERMS_READ_WRITE;
            son++;
        }
    }
    // esp_log_buffer_hex(TAG,devicesarr,sizeof(mb_parameter_descriptor_t)*son);

    // prepare once 

    mensaje.numCids=son;
    mensaje.descriptors=(mb_parameter_descriptor_t*)devicesarr;
    mensaje.dataReceiver=(void*)&sensorData;
    // mensaje.dataReceiver=(uint8_t*)theBlower.getSensorsDataPtr();
    mensaje.requester=xTaskGetCurrentTaskHandle(); // so we can notify when done
    memset(errors,0,sizeof(errors));
    mensaje.errCode=&errors[0];

    while(true)
    {
        // send message to rs485 task   
        if(xQueueSend(rs485Q, &mensaje, pdMS_TO_TICKS(100))!=pdPASS)
        {
            ESP_LOGE(TAG, "sensor_task: Could not send message to rs485 task.");
        }
        else
        {
            // wait until rs485 task notifies us that it is done
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            if(errors[0]==0)
            {
                printf("Sensor data read: Temp %.02f˚C Percent %.02f%% DO %.02f ppm ",
                    sensorData.WTemp,
                    sensorData.percentDO*100.0,
                    sensorData.DO);
            }
            if(errors[1]==0)
            {
                uint32_t *aca=(uint32_t*)&sensorData.PH;
                printf("SOC %d%% SOH %d%% Error[1]=%d [2]=%d",*aca&0xFF,(*aca>>8)&0xFF,errors[0],errors[1]);
            }
            printf("\n");
        }
        // wait before next reading
        vTaskDelay(pdMS_TO_TICKS(refreshrate*1000));
    }
}