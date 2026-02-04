#define GLOBAL
#include "esp_log.h"
#include "misparams.h"  // for modbus parameters structures
#include "mbcontroller.h"
#include "defines.h"
#include "BlowerClass.h"
#include "typedef.h"
#include "globals.h"

// Constants
static constexpr int MODBUS_PORT = 1;
static constexpr int MODBUS_BAUD = 9600;
static constexpr mb_mode_type_t MODBUS_MODE = (mb_mode_type_t)0;
static constexpr uart_mode_t MODBUS_UART_MODE = UART_MODE_RS485_HALF_DUPLEX;
static constexpr TickType_t POLL_DELAY_MS = 500;
static constexpr uint8_t ERROR_FILL = 0xFF;
static constexpr size_t ERROR_BUF_LEN = 20;


// Modbus master initialization
static esp_err_t master_init(void)
{
     esp_log_level_set("MB_CONTROLLER_MASTER", (esp_log_level_t)0);

     // Initialize and start Modbus controller
    mb_communication_info_t comm = {
        .mode = MODBUS_MODE,
        .port = (uart_port_t)MODBUS_PORT,
        .baudrate = MODBUS_BAUD,
        .parity = MB_PARITY_NONE
    };
    void* master_handler = NULL;

    esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler);
    MB_RETURN_ON_FALSE((master_handler != NULL), ESP_ERR_INVALID_STATE, TAG,
                                "mb controller initialization fail.");
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller initialization fail, returns(0x%x).", (int)err);
    err = mbc_master_setup((void*)&comm);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller setup fail, returns(0x%x).", (int)err);

                            
    // Set UART pin numbers
    err = uart_set_pin((uart_port_t)MODBUS_PORT, RS485RX, RS485TX,
                              RS485RTS, UART_PIN_NO_CHANGE);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
        "mb serial set pin failure, uart_set_pin() returned (0x%x).", (int)err);

    err = mbc_master_start();
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller start fail, returned (0x%x).", (int)err);

    // Set driver mode to Half Duplex
    err = uart_set_mode((uart_port_t)MODBUS_PORT, MODBUS_UART_MODE);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
            "mb serial set mode failure, uart_set_mode() returned (0x%x).", (int)err);

    MESP_LOGI(TAG, "Modbus master stack initialized...");
    vTaskDelay(pdMS_TO_TICKS(5));
    return err;
}


// User operation function to read slave values and check alarm
void rs485_task_manager(void *arg)
{
    esp_err_t err = ESP_OK;
    const mb_parameter_descriptor_t* param_descriptor = NULL;
    rs485queue_t mensaje;

    ESP_ERROR_CHECK(master_init()); // once only

    MESP_LOGI(TAG, "Start modbus manager..."); 

    while (true) // task loop read queue
    { 
        again: // wait for queue
        if (xQueueReceive(rs485Q, &mensaje, portMAX_DELAY) == pdPASS) // rs485Q has a pointer to the original message. MUST be freed at some point
        {       // got one, lets process it
            // sanity checks
            // printf("RSQueue in\n");
            if (mensaje.numCids <= 0 || mensaje.descriptors == NULL || mensaje.requester == NULL)
            {
                MESP_LOGE(TAG, "rs485_task_manager: Invalid message received.");
                // tell task he screwed up
                memset(mensaje.errCode, ERROR_FILL, ERROR_BUF_LEN);       // all 0xff is general failure
                xTaskNotifyGive(mensaje.requester);
                goto again;
            }
            // printf("RSQueue set descriptors\n");
            // set descriptors for this message
            err = mbc_master_set_descriptor(mensaje.descriptors, mensaje.numCids);
            if (err != ESP_OK)
            {
                MESP_LOGE(TAG, "Set descriptor fail, err = 0x%x (%s)", (int)err, esp_err_to_name(err));
                // tell task he screwed up
                memset(mensaje.errCode, ERROR_FILL, ERROR_BUF_LEN);       // all 0xff is general failure
                xTaskNotifyGive(mensaje.requester);
                goto again;
            }
            // printf("RSQueue read all cids %d\n",mensaje.numCids);
                // Read all found characteristics from slave(s)
            for (uint16_t cid = 0; cid < mensaje.numCids; cid++)
            {
                // Get data from parameters description table
                // and use this information to fill the characteristics description table
                // and having all required fields in just one table
                // printf("RSQueue requesting cid %d\n",cid);
                err = mbc_master_get_cid_info(cid, &param_descriptor);
                if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL) && 
                    (param_descriptor->mb_param_type == MB_PARAM_HOLDING))
                {
                    uint8_t* temp_data_ptr = (uint8_t*)mensaje.dataReceiver + param_descriptor->param_offset - 1;
                    assert(temp_data_ptr);
                    uint8_t type = 0;
                    // printf("RSQueue processing cid %d final ptr %p offset %d original pointer %p\n",cid,temp_data_ptr,
                            // param_descriptor->param_offset,
                            // mensaje.dataReceiver);
        
                        // this is the actual request from matser to slave stupid name for routine
                        err = mbc_master_get_parameter(cid, (char*)param_descriptor->param_key,
                                                            (uint8_t*)temp_data_ptr, &type);
                        if (err == ESP_OK) 
                            mensaje.errCode[cid] = ESP_OK;   // success
                        else 
                        {
                            if ((theConf.debug_flags >> dRS485) & 1U) 
                            {
                                MESP_LOGE(TAG, "FAIL Characteristic #%u (%s) Addr %x read fail, err = 0x%x (%s).",       //timeout here
                                            param_descriptor->cid,
                                            param_descriptor->param_key,
                                            param_descriptor->mb_slave_addr,
                                            (int)err,
                                            esp_err_to_name(err));
                            }
                            mensaje.errCode[cid] = err;   // this error

                        }                   
                }
                else
                {
                    MESP_LOGE(TAG, "Characteristic #%u get Queue Message info fail %d %s)",
                                    cid,
                                    (int)err,
                                    esp_err_to_name(err));
                }
                
                vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS)); // the smallest time between polls... speed is low 9600 usually give it some time

            }
            // notify requester task that we are done
            xTaskNotifyGive(mensaje.requester);
        }
        else
        {
            MESP_LOGE(TAG, "rs485_task_manager: Error receiving from queue.");
        }

    }
// should not happend 
    MESP_LOGI(TAG, "Destroy master...");
    ESP_ERROR_CHECK(mbc_master_destroy());
}
