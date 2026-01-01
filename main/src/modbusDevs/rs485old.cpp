/*
 * SPDX-FileCopyrightText: 2016-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define GLOBAL
#include "esp_log.h"
#include "misparams.h"  // for modbus parameters structures
#include "mbcontroller.h"
// #include "sdkconfig.h"
#include "defines.h"
#include "BlowerClass.h"
#include "typedef.h"
#include "globals.h"

#define SENSOR_OFFSET(field) ((uint16_t)(offsetof(sensor_t ,field) + 1))
#define MAXSENSORS              (5)

int totalcids=0;
int refreshrate=0;

// local variables for Descriptors like an Array of devices
typedef struct {mb_parameter_descriptor_t devices[5];} descriptor_array_t;
descriptor_array_t *devicesarr;

// Modbus master initialization
static esp_err_t master_init(void)
{
    // Initialize and start Modbus controller... take them form theConf  
    mb_communication_info_t comm = {
            .mode = (mb_mode_type_t)0,
            .port = (uart_port_t)1,

            .baudrate = 9600,
            .parity = MB_PARITY_NONE
    };
    void* master_handler = NULL;

    devicesarr=(descriptor_array_t*)calloc(1,sizeof(descriptor_array_t));

    esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler);
    MB_RETURN_ON_FALSE((master_handler != NULL), ESP_ERR_INVALID_STATE, TAG,
                                "mb controller initialization fail.");
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller initialization fail, returns(0x%x).", (int)err);
    err = mbc_master_setup((void*)&comm);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller setup fail, returns(0x%x).", (int)err);

                            
    // Set UART pin numbers
    err = uart_set_pin((uart_port_t)1, RS485RX, RS485TX,
                              RS485RTS, UART_PIN_NO_CHANGE);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
        "mb serial set pin failure, uart_set_pin() returned (0x%x).", (int)err);

    err = mbc_master_start();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller start fail, returned (0x%x).", (int)err);

    // Set driver mode to Half Duplex
    err = uart_set_mode((uart_port_t)1, UART_MODE_RS485_HALF_DUPLEX);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
            "mb serial set mode failure, uart_set_mode() returned (0x%x).", (int)err);

    vTaskDelay(5);


//get configured sensors data descriptors variables
sensors_modbus_specs_t *sensorinfo=(sensors_modbus_specs_t *)&theConf.modbus_sensors;    // make it array type for easy management
refreshrate=sensorinfo->regfresh;
int son=0;      //since we can skip certain sensros via the Offse being -1 we need a separate counter

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
totalcids=son;
}
    err = mbc_master_set_descriptor((mb_parameter_descriptor_t*)devicesarr, son);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                                "mb controller set descriptor fail, returns(0x%x).", (int)err);
    ESP_LOGI(TAG, "Modbus master stack initialized...");
    return err;
}


// User operation function to read slave values and check alarm
void rs485_task(void *arg)
{
    esp_err_t err = ESP_OK;
    float value = 0;
    bool alarm_state = false;
    const mb_parameter_descriptor_t* param_descriptor = NULL;

    ESP_ERROR_CHECK(master_init());
    vTaskDelay(10);

    ESP_LOGI(TAG, "Start modbus test...");

    while(true) {
    // for(uint16_t retry = 0; retry <= MASTER_MAX_RETRY && (!alarm_state); retry++) {
        // Read all found characteristics from slave(s)
        for (uint16_t cid = 0; (err != ESP_ERR_NOT_FOUND) && cid < totalcids; cid++)
        {
            // Get data from parameters description table
            // and use this information to fill the characteristics description table
            // and having all required fields in just one table
            err = mbc_master_get_cid_info(cid, &param_descriptor);
            if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL)) 
            {
                void* temp_data_ptr =  ((void*)&sensorData + param_descriptor->param_offset - 1);
                assert(temp_data_ptr);
                uint8_t type = 0;
    
                    // this is the actual request from matser to slave stupid name for routine
                    err = mbc_master_get_parameter(cid, (char*)param_descriptor->param_key,
                                                        (uint8_t*)temp_data_ptr, &type);
                    if (err == ESP_OK) 
                    {
                        if (param_descriptor->mb_param_type == MB_PARAM_HOLDING) 
                        {
                                value = *(float*)temp_data_ptr;

                            // if((theConf.debug_flags >> dMODBUS) & 1U)
                            //     ESP_LOGI(TAG, "%s Characteristic #%u %s (%s) ADDR %X value = %f (0x%" PRIx32 ") read successful.",param_descriptor->mb_param_type == MB_PARAM_HOLDING?"Holding":"Input",
                            //                     param_descriptor->cid,
                            //                     param_descriptor->param_key,
                            //                     param_descriptor->param_units,
                            //                     param_descriptor->mb_slave_addr,
                            //                     value,
                            //                     *(uint32_t*)temp_data_ptr);
                            // TODO for testing only

                            if(param_descriptor->mb_slave_addr==16)
                            {  
                                if((theConf.debug_flags >> dMODBUS) & 1U)
                                {
                                    printf("CID %d Temp %.02f˚C Percent %.02f%% DO %.02f ppm\n",cid,sensorData.WTemp,sensorData.percentDO*100.0,sensorData.DO);
                                }
                            }
                            else
                            {
                                uint16_t nvalue=*(uint16_t*)temp_data_ptr;
                                if((theConf.debug_flags >> dMODBUS) & 1U)
                                    printf("Battery: SOC %d%% SOH %d%%\n",(nvalue)&0xFF,(nvalue>>8)&0xFF);
                                    batteryData.batSOC=(nvalue)&0xFF;
                                    batteryData.batSOH=(nvalue>>8)&0xFF;
                            }
                        } 
                        else 
                        {
                            ESP_LOGW(TAG, "Characteristic #%u (%s) has unsupported param type %u.", 
                                            param_descriptor->cid,
                                            param_descriptor->param_key,
                                            (unsigned)param_descriptor->mb_param_type);
                        }
                    }
                    else 
                    {
                        ESP_LOGE(TAG, "FAIL Characteristic #%u (%s) Addr %x read fail, err = 0x%x (%s).",       //timeout here
                                        param_descriptor->cid,
                                        param_descriptor->param_key,
                                        param_descriptor->mb_slave_addr,
                                        (int)err,
                                        (char*)esp_err_to_name(err));
                    }
                
            }
            else
            {
                ESP_LOGE(TAG, "Characteristic #%u get cid info fail, err = 0x%x (%s).",
                                cid,
                                (int)err,
                                (char*)esp_err_to_name(err));
            }
            
            vTaskDelay(pdMS_TO_TICKS(500)); // the smallest time between polls... speed is low 9600 usually give it some time

        }
        vTaskDelay(pdMS_TO_TICKS(refreshrate*1000));
    }
// shouyld not happend 
    ESP_LOGI(TAG, "Destroy master...");
    ESP_ERROR_CHECK(mbc_master_destroy());
}
