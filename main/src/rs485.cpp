

#ifndef TYPESMODB_H_
#define TYPESMODB_H_
#define GLOBAL
#include "includes.h" 
#include "globals.h"

void rs485_task(void *arg)
{
    ESP_LOGI(TAG,"Port %d Baud %d TX %d RX %d RTS %D",theConf.port,theConf.baud,UTXD,URXD,URTS);
    const uart_port_t uart_num =(uart_port_t) ECHO_UART_PORT;
    uart_config_t uart_config = {
        .baud_rate = theConf.baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    rs485Q = xQueueCreate( 5, sizeof( answer_t ) );
    if(!rs485Q)
        printf("Failed queue Broadcast\n");
    // Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Start ModBus Emulator");

    // Install UART driver (we don't need an event queue here)
    // In this example we don't even use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(theConf.port, MBUF_SIZE * 2, 0, 0, NULL, 0));
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(theConf.port, &uart_config));

    ESP_LOGI(TAG, "UART set pins, mode and install driver.");

    // Set UART pins as per KConfig settings TRR
    ESP_ERROR_CHECK(uart_set_pin(theConf.port, UTXD, URXD, URTS,UCTS));

    // Set RS485 half duplex mode EXTREMELY important
    ESP_ERROR_CHECK(uart_set_mode(theConf.port, UART_MODE_RS485_HALF_DUPLEX));

    // Set read timeout of UART TOUT feature
    ESP_ERROR_CHECK(uart_set_rx_timeout(theConf.port, ECHO_READ_TOUT));
    // Allocate buffers for UART
    uint8_t* data = (uint8_t*) malloc(MBUF_SIZE);

    ESP_LOGI(TAG, "ModBus waiting input\n");

    while (1) {
        //Read data from UART
        int len = uart_read_bytes(theConf.port, data, MBUF_SIZE, PACKET_READ_TICS);
        if (len > 0) { 
            char *theData=(char*)calloc(1,len+2);
            reply.theanswer=(char*)theData;
            memcpy(theData,data,len);
            reply.len=len;
            time(&reply.ts);
            esp_log_buffer_hex(TAG,theData,len);      //display before sendig else probably erased already

            if(xQueueSend(rs485Q,&reply,0)!=pdTRUE)      //will free todo 
            {
                free(reply.theanswer);
                printf("Error queueing reply\n");
            }
        } else {
           
            ESP_ERROR_CHECK(uart_wait_tx_done(theConf.port, 10));
        }
    }
    //never gets here but....
    vTaskDelete(NULL);
}

#endif