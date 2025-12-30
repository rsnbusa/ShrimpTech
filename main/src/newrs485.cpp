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

// #define SENSOR_OFFSET(field) ((uint16_t)(offsetof(sensor_t ,field) + 1))
// #define MAXSENSORS              (5)

// int totalcids=0;
// int refreshrate=0;

// local variables for Descriptors like an Array of devices
// typedef struct {mb_parameter_descriptor_t devices[5];} descriptor_array_t;
// descriptor_array_t *devicesarr;

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

    // devicesarr=(descriptor_array_t*)calloc(1,sizeof(descriptor_array_t));

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

    ESP_LOGI(TAG, "Modbus master stack initialized...");
        vTaskDelay(5);
    return err;
}


// User operation function to read slave values and check alarm
void rs485_task_manager(void *arg)
{
    esp_err_t err = ESP_OK;
    float value = 0;
    const mb_parameter_descriptor_t* param_descriptor = NULL;
    rs485queue_t mensaje;

    ESP_ERROR_CHECK(master_init()); // once only

    ESP_LOGI(TAG, "Start modbus manager..."); 

    while(true) // task llop read queue 
    { 
        again:// wait for queue
        if( xQueueReceive( rs485Q, &mensaje,  portMAX_DELAY )==pdPASS)	//rs485Q has a pointer to the original message. MUST be freed at some point
        {       // got one, lets process it
            // sanity checks
            // printf("RSQueue in\n");
            if(mensaje.numCids<=0 || mensaje.descriptors==NULL || mensaje.requester==NULL)
            {
                ESP_LOGE(TAG, "rs485_task_manager: Invalid message received.");
                // tell task he screwed up
                memset(mensaje.errCode,0xff,20);       // all 0xff is general failure
                xTaskNotifyGive(mensaje.requester);
                goto again;
            }
            // printf("RSQueue set descriptors\n");
            // set descriptors for this message
            err = mbc_master_set_descriptor(mensaje.descriptors, mensaje.numCids);
            if (err != ESP_OK)  
            {
                ESP_LOGE(TAG, "Set descriptor fail, err = 0x%x (%s)", (int)err, (char*)esp_err_to_name(err));
                // tell task he screwed up
                memset(mensaje.errCode,0xff,20);       // all 0xff is general failure
                xTaskNotifyGive(mensaje.requester);
                goto again;
            }
            // printf("RSQueue read all cids %d\n",mensaje.numCids);
                // Read all found characteristics from slave(s)
            for (uint16_t cid = 0;  cid < mensaje.numCids; cid++)
            {
                // Get data from parameters description table
                // and use this information to fill the characteristics description table
                // and having all required fields in just one table
                // printf("RSQueue requesting cid %d\n",cid);
                err = mbc_master_get_cid_info(cid, &param_descriptor);
                if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL) && 
                        (param_descriptor->mb_param_type == MB_PARAM_HOLDING)) 
                {
                    void* temp_data_ptr =  (mensaje.dataReceiver+ param_descriptor->param_offset - 1);
                    assert(temp_data_ptr);
                    uint8_t type = 0;
                    // printf("RSQueue processing cid %d final ptr %p offset %d original pointer %p\n",cid,temp_data_ptr,
                    //         param_descriptor->param_offset,
                    //         mensaje.dataReceiver);

        
                        // this is the actual request from matser to slave stupid name for routine
                        err = mbc_master_get_parameter(cid, (char*)param_descriptor->param_key,
                                                            (uint8_t*)temp_data_ptr, &type);
                        if (err == ESP_OK) 
                        {
                            // esp_log_buffer_hex("Value",temp_data_ptr,4);
                            // if (param_descriptor->mb_param_type == MB_PARAM_HOLDING) 
                            // {
                                    // value = *(float*)temp_data_ptr;

                                // if((theConf.debug_flags >> dMODBUS) & 1U)
                                //     ESP_LOGI(TAG, "%s Characteristic #%u %s (%s) ADDR %X value = %f (0x%" PRIx32 ") read successful.",param_descriptor->mb_param_type == MB_PARAM_HOLDING?"Holding":"Input",
                                //                     param_descriptor->cid,
                                //                     param_descriptor->param_key,
                                //                     param_descriptor->param_units,
                                //                     param_descriptor->mb_slave_addr,
                                //                     value,
                                //                     *(uint32_t*)temp_data_ptr);
                                // TODO for testing only

                                // if(param_descriptor->mb_slave_addr==16)
                                // {  
                                //     if((theConf.debug_flags >> dMODBUS) & 1U)
                                //     {
                                //         printf("CID %d Temp %.02f˚C Percent %.02f%% DO %.02f ppm\n",cid,sensorData.WTemp,sensorData.percentDO*100.0,sensorData.DO);
                                //     }
                                // }
                                // else
                                // {
                                //     uint16_t nvalue=*(uint16_t*)temp_data_ptr;
                                //     if((theConf.debug_flags >> dMODBUS) & 1U)
                                //         printf("Battery: SOC %d%% SOH %d%%\n",(nvalue)&0xFF,(nvalue>>8)&0xFF);
                                //         batteryData.batSOC=(nvalue)&0xFF;
                                //         batteryData.batSOH=(nvalue>>8)&0xFF;
                                // }
                            // } 
                            // else 
                            // {
                            //     ESP_LOGW(TAG, "Characteristic #%u (%s) has unsupported param type %u.", 
                            //                     param_descriptor->cid,
                            //                     param_descriptor->param_key,
                            //                     (unsigned)param_descriptor->mb_param_type);
                            // }
                            mensaje.errCode[cid]=ESP_OK;   // success
                            // esp_log_buffer_hex("Sensor Read",(void*)&sensorData,sizeof(sensor_t));
                        }
                        else 
                        {
                            ESP_LOGE(TAG, "FAIL Characteristic #%u (%s) Addr %x read fail, err = 0x%x (%s).",       //timeout here
                                            param_descriptor->cid,
                                            param_descriptor->param_key,
                                            param_descriptor->mb_slave_addr,
                                            (int)err,
                                            (char*)esp_err_to_name(err));
                            mensaje.errCode[cid]=err;   // this error

                        }

                    
                }
                else
                {
                    ESP_LOGE(TAG, "Characteristic #%u get Queue Message info fail %d %s)",
                                    cid,
                                    (int)err,
                                    (char*)esp_err_to_name(err));
                }
                
                vTaskDelay(pdMS_TO_TICKS(500)); // the smallest time between polls... speed is low 9600 usually give it some time

            }
            // vTaskDelay(pdMS_TO_TICKS(refreshrate*1000));
            // notify requester task that we are done
            xTaskNotifyGive(mensaje.requester);
            // printf("RSQueue done\n");
        }
        else
        {
            ESP_LOGE(TAG, "rs485_task_manager: Error receiving from queue.");
        }

    }
// should not happend 
    ESP_LOGI(TAG, "Destroy master...");
    ESP_ERROR_CHECK(mbc_master_destroy());
}
