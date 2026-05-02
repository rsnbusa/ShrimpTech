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
static constexpr uart_mode_t MODBUS_UART_MODE = UART_MODE_RS485_HALF_DUPLEX;
static constexpr TickType_t POLL_DELAY_MS = 500;
static constexpr uint8_t ERROR_FILL = 0xFF;
static constexpr size_t ERROR_BUF_LEN = 20;

const struct {
    const char *name;
    int start;
} kRegisterStartMap[] = {
    {"batSoc", 0},
    {"batSOH", 1},
    {"batChgAHToday", 2},
    {"batDischgAHToday", 3},
    {"batChgAHTotal", 4},
    {"batDischgAHTotal", 5},
    {"batCycleCount", 6},
    {"chargeCurr", 7},
    {"generateEnergyToday", 9},
    {"usedEnergyToday", 11},
    {"gLoadConsumLineTotal", 13},
    {"batChgkWhToday", 15},
    {"batDischgkWhToday", 17},
    {"genLoadConsumToday", 19},
    {"batBmsTemp", 21},
    {"pv1Volts", 23},
    {"pv2Volts", 25},
    {"pv1Amp", 27},
    {"pv2Amp", 29},
    {"frequency", 31},
    {"cmd", 32},
    {"mcurrent", 33},
    {"mvolts", 35},
    {"mpower", 36},
    {"mrpm", 38},
    {"mstatus", 39},
};

static const char *get_register_name_by_start(int start)
{
    for (size_t i = 0; i < (sizeof(kRegisterStartMap) / sizeof(kRegisterStartMap[0])); ++i) {
        if (kRegisterStartMap[i].start == start) {
            return kRegisterStartMap[i].name;
        }
    }
    return "unknown";
}

static void *g_master_handle = NULL;
static bool g_master_started = false;

static void log_modbus_rx_hex(const mb_parameter_descriptor_t *param_descriptor, const uint8_t *data)
{
    if (param_descriptor == NULL || data == NULL || param_descriptor->param_size == 0) {
        return;
    }

    MESP_LOGI(TAG,
              "RX slave=%u start=%u reg=%s cid=%u bytes=%u",
              param_descriptor->mb_slave_addr,
              param_descriptor->mb_reg_start,
              get_register_name_by_start(param_descriptor->mb_reg_start),
              param_descriptor->cid,
              (unsigned)param_descriptor->param_size);

    constexpr size_t kHexLineBytes = 16;
    const size_t total_bytes = (size_t)param_descriptor->param_size;
    for (size_t offset = 0; offset < total_bytes; offset += kHexLineBytes) {
        const size_t line_bytes = (total_bytes - offset > kHexLineBytes) ?
                                   kHexLineBytes : (total_bytes - offset);
        char hex_line[(kHexLineBytes * 3) + 1] = {0};
        size_t pos = 0;

        for (size_t i = 0; i < line_bytes; ++i) {
            pos += (size_t)snprintf(&hex_line[pos],
                                    sizeof(hex_line) - pos,
                                    "%02X ",
                                    data[offset + i]);
        }

        if (pos > 0) {
            hex_line[pos - 1] = '\0';
        }

        MESP_LOGI(TAG, "RX[%u]: %s", (unsigned)offset, hex_line);
    }
}

static void log_modbus_tx_hex(const mb_parameter_descriptor_t *param_descriptor, const uint8_t *data)
{
    if (param_descriptor == NULL || data == NULL || param_descriptor->param_size == 0) {
        return;
    }

    MESP_LOGI(TAG, "TX data bytes=%u", (unsigned)param_descriptor->param_size);

    constexpr size_t kHexLineBytes = 16;
    const size_t total_bytes = (size_t)param_descriptor->param_size;
    for (size_t offset = 0; offset < total_bytes; offset += kHexLineBytes) {
        const size_t line_bytes = (total_bytes - offset > kHexLineBytes) ?
                                   kHexLineBytes : (total_bytes - offset);
        char hex_line[(kHexLineBytes * 3) + 1] = {0};
        size_t pos = 0;

        for (size_t i = 0; i < line_bytes; ++i) {
            pos += (size_t)snprintf(&hex_line[pos],
                                    sizeof(hex_line) - pos,
                                    "%02X ",
                                    data[offset + i]);
        }

        if (pos > 0) {
            hex_line[pos - 1] = '\0';
        }

        MESP_LOGI(TAG, "TX[%u]: %s", (unsigned)offset, hex_line);
    }
}


// Modbus master initialization
static esp_err_t master_init(void)
{
    if (g_master_handle != NULL) {
        return ESP_OK;
    }

    esp_log_level_set("mb_object.master", ESP_LOG_NONE);
    esp_log_level_set("mbc_serial.master", ESP_LOG_NONE);
    esp_log_level_set("MB_CONTROLLER_MASTER", ESP_LOG_NONE);

    // Initialize and start Modbus controller
    mb_communication_info_t comm = {};
    comm.ser_opts.port = (uart_port_t)MODBUS_PORT;
    comm.ser_opts.mode = MB_RTU;
    comm.ser_opts.baudrate = MODBUS_BAUD;
    comm.ser_opts.parity = MB_PARITY_NONE;
    comm.ser_opts.uid = 0;
    comm.ser_opts.response_tout_ms = 1000;
    comm.ser_opts.data_bits = UART_DATA_8_BITS;
    comm.ser_opts.stop_bits = UART_STOP_BITS_1;

    esp_err_t err = mbc_master_create_serial(&comm, &g_master_handle);
    MB_RETURN_ON_FALSE((g_master_handle != NULL), ESP_ERR_INVALID_STATE, TAG,
                                "mb controller initialization fail.");
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
                            "mb controller initialization fail, returns(0x%x).", (int)err);

    // Set UART pin numbers
    err = uart_set_pin((uart_port_t)MODBUS_PORT, RS485RX, RS485TX,
                              RS485RTS, UART_PIN_NO_CHANGE);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
        "mb serial set pin failure, uart_set_pin() returned (0x%x).", (int)err);

    // Set driver mode to Half Duplex
    err = uart_set_mode((uart_port_t)MODBUS_PORT, MODBUS_UART_MODE);
    MB_RETURN_ON_FALSE((err == ESP_OK), ESP_ERR_INVALID_STATE, TAG,
            "mb serial set mode failure, uart_set_mode() returned (0x%x).", (int)err);

    g_master_started = false;
    return err;
}

/* WRITING to slave ...
use mux to define if its a write(0) or read (>0))
will be set in the options parameter of the descriptor 

uint8_t type = 0; // Type of parameter
uint8_t temp_data[4] = {0}; // temporary buffer

esp_err_t err = mbc_master_set_parameter(g_master_handle, CID_TEMP_DATA_2, (uint8_t*)temp_data, &type);
if (err == ESP_OK) {
    ESP_LOGI(TAG, "Set parameter data successfully.");
} else {
    ESP_LOGE(TAG, "Set data fail, err = 0x%x (%s).", (int)err, (char*)esp_err_to_name(err));
}
*/

// User operation function to read slave values and check alarm
void rs485_task_manager(void *arg)
{
    esp_err_t err = ESP_OK;
    const mb_parameter_descriptor_t* param_descriptor = NULL;
    rs485queue_t mensaje;

    ESP_ERROR_CHECK(master_init()); // once only

    if ((theConf.debug_flags >> dMODBUS) & 1U) {
        MESP_LOGI(TAG, "Start modbus manager..."); 
    }

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
            err = mbc_master_set_descriptor(g_master_handle, mensaje.descriptors, mensaje.numCids);
            if (err != ESP_OK)
            {
                MESP_LOGE(TAG, "Set descriptor fail, err = 0x%x (%s)", (int)err, esp_err_to_name(err));
                // tell task he screwed up
                memset(mensaje.errCode, ERROR_FILL, ERROR_BUF_LEN);       // all 0xff is general failure
                xTaskNotifyGive(mensaje.requester);
                goto again;
            }

            if (!g_master_started)
            {
                vTaskDelay(pdMS_TO_TICKS(5));
                err = mbc_master_start(g_master_handle);
                if (err != ESP_OK)
                {
                    MESP_LOGE(TAG, "mb controller start fail, returned (0x%x)", (int)err);
                    memset(mensaje.errCode, ERROR_FILL, ERROR_BUF_LEN);
                    xTaskNotifyGive(mensaje.requester);
                    goto again;
                }
                g_master_started = true;
                if ((theConf.debug_flags >> dMODBUS) & 1U) {
                    MESP_LOGI(TAG, "Modbus master stack initialized...");
                }
            }

            // printf("RSQueue read all cids %d\n",mensaje.numCids);
                // Read all found characteristics from slave(s)
            for (uint16_t cid = 0; cid < mensaje.numCids; cid++)
            {
                // Get data from parameters description table
                // and use this information to fill the characteristics description table
                // and having all required fields in just one table
                // printf("RSQueue requesting cid %d\n",cid);
                err = mbc_master_get_cid_info(g_master_handle, cid, &param_descriptor);
                if ((err != ESP_ERR_NOT_FOUND) && (param_descriptor != NULL) && 
                    (param_descriptor->mb_param_type == MB_PARAM_HOLDING))
                {
                    // printf("Cid %d param key %s offset %d access %d data %p-%p\n",cid,param_descriptor->param_key,param_descriptor->param_offset-1,
                        // param_descriptor->access,mensaje.dataReceiver,&vfdCmdData);
                    uint8_t* temp_data_ptr = (uint8_t*)mensaje.dataReceiver + param_descriptor->param_offset - 1;
                    assert(temp_data_ptr);
                    uint8_t type = 0;
                    const char *op_name = "none";
                    // printf("RSQueue processing cid %d final ptr %p offset %d original pointer %p\n",cid,temp_data_ptr,
                            // param_descriptor->param_offset,
                            // mensaje.dataReceiver);
        
                        // this is the actual request from master to slave (stupid name for routine)
                        if (param_descriptor->access & PAR_PERMS_READ)
                        {
                             op_name = "get";
                             err = mbc_master_get_parameter(g_master_handle, cid,
                                                            (uint8_t*)temp_data_ptr, &type);
                        }
                        else if (param_descriptor->access & PAR_PERMS_WRITE)
                        {
                            op_name = "set";
                            if ((theConf.debug_flags >> dMTXRX) & 1U)
                            {
                                log_modbus_tx_hex(param_descriptor, temp_data_ptr);
                            }
                            err = mbc_master_set_parameter(g_master_handle, cid,
                                                            (uint8_t*)temp_data_ptr, &type);
                            // ESP_LOG_BUFFER_HEX("WRITE", (void*)temp_data_ptr, 10);
                        }
                        if (err == ESP_OK) 
                        {
                            mensaje.errCode[cid] = ESP_OK;   // success
                            if (((theConf.debug_flags >> dMTXRX) & 1U) &&
                                (param_descriptor->access & PAR_PERMS_READ))
                            {
                                log_modbus_rx_hex(param_descriptor, temp_data_ptr);
                            }
                        }
                        else 
                        {
                            const char *reg_name = get_register_name_by_start(param_descriptor->mb_reg_start);
                            ESP_LOGI(TAG,
                                     "mbc_master_%s error=0x%x (%s) slave=%u start=%u reg=%s cid=%u",
                                     op_name,
                                     (int)err,
                                     esp_err_to_name(err),
                                     param_descriptor->mb_slave_addr,
                                     param_descriptor->mb_reg_start,
                                     reg_name,
                                     param_descriptor->cid);
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
    if (g_master_handle != NULL) {
        ESP_ERROR_CHECK(mbc_master_delete(g_master_handle));
        g_master_handle = NULL;
        g_master_started = false;
    }
}
